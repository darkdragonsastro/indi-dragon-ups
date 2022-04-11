/*******************************************************************************
 Copyright(c) 2021 Rick Bassham. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "dragonups.h"

#include "connectionplugins/connectiontcp.h"
#include "indicom.h"

#include <cmath>
#include <cstring>
#include <ctime>
#include <memory>
#include <termios.h>

using namespace INDI;

// 0x23 is the stop char '#'
#define DRIVER_STOP_CHAR char(0x23)
// Wait up to a maximum of 3 seconds for serial input
#define DRIVER_TIMEOUT 3
// Maximum buffer for sending/receving.
#define DRIVER_LEN 64

// We declare an auto pointer to DragonUPS.
std::unique_ptr<INDI::DragonUPS> DragonUPS(new INDI::DragonUPS());

DragonUPS::DragonUPS() : WeatherInterface(this)
{
    setVersion(DLR_VERSION_MAJOR, DLR_VERSION_MINOR);
}

/************************************************************************************
 *
 * ***********************************************************************************/
bool DragonUPS::initProperties()
{
    INDI::DefaultDevice::initProperties();
    INDI::WeatherInterface::initProperties("Settings", "Settings");

    setDriverInterface(AUX_INTERFACE | WEATHER_INTERFACE);

    addAuxControls();

    IUFillNumber(&VoltageSensorN[0], "SENSOR_VOLTAGE", "Voltage (V)", "%4.1f", 0, 999, 100, 0);
    IUFillNumberVector(&VoltageSensorNP, VoltageSensorN, 1, getDeviceName(), "VOLTAGE_SENSORS", "Sensors", MAIN_CONTROL_TAB, IP_RO,
                       60, IPS_IDLE);

    // WeatherInterface
    addParameter("SENSOR_VOLTAGE", "Voltage (V)", 12.5, 14.0, 5);
    setCriticalParameter("SENSOR_VOLTAGE");

    tcpConnection = new Connection::TCP(this);
    tcpConnection->registerHandshake([&]() {
        return Handshake();
    });
    registerConnection(tcpConnection);

    return true;
}

const char *DragonUPS::getDefaultName()
{
    return (const char *)"Dragon UPS";
}

bool DragonUPS::updateProperties()
{
    INDI::DefaultDevice::updateProperties();
    INDI::WeatherInterface::updateProperties();

    if (isConnected())
    {
        defineProperty(&VoltageSensorNP);
    }
    else
    {
        deleteProperty(VoltageSensorNP.name);
    }

    return true;
}

bool DragonUPS::Handshake()
{
    PortFD = tcpConnection->getPortFD();

    bool rc = checkStatus();

    if (rc)
        connectRetries = 0;

    return rc;
}

bool DragonUPS::checkStatus()
{
    return sendCommandAndProcessStatus(":CS#");
}

void DragonUPS::TimerHit()
{
    if (!isConnected())
        return;

    checkStatus();

    SetTimer(getCurrentPollingPeriod());
}

void DragonUPS::processStatus(const char *status)
{
    unsigned int voltage = 0;

    sscanf(status, "CV%X", &voltage);

    VoltageSensorN[0].value = voltage / 10.0;
    VoltageSensorNP.s = IPS_OK;
    IDSetNumber(&VoltageSensorNP, nullptr);

    // WeatherInterface
    setParameterValue("SENSOR_VOLTAGE", VoltageSensorN[0].value);

    INDI::WeatherInterface::updateProperties();
    if (INDI::WeatherInterface::syncCriticalParameters())
        IDSetLight(&critialParametersLP, nullptr);

    ParametersNP.s = IPS_OK;
    IDSetNumber(&ParametersNP, nullptr);
}

void DragonUPS::abnormalDisconnectCallback(void *userpointer)
{
    DragonUPS *p = static_cast<DragonUPS *>(userpointer);

    if (p->connectRetries < 10 && p->Connect())
    {
        p->setConnected(true, IPS_OK);
        p->updateProperties();
    }
    else
    {
        // Reconnect in 2 seconds
        p->connectRetries += 1;
        IEAddTimer(2000, abnormalDisconnectCallback, p);
    }
}

bool DragonUPS::ISNewNumber(const char * dev, const char * name, double values[], char * names[], int n)
{
    if (dev && !strcmp(dev, getDeviceName()))
    {
        if (strstr(name, "SENSOR_VOLTAGE"))
            return INDI::WeatherInterface::processNumber(dev, name, values, names, n);
    }
    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

void DragonUPS::abnormalDisconnect()
{
    // Ignore disconnect errors
    Disconnect();

    // Set Disconnected
    setConnected(false, IPS_IDLE);
    // Update properties
    updateProperties();

    // Reconnect in 2 seconds
    IEAddTimer(2000, abnormalDisconnectCallback, this);
}

bool DragonUPS::sendCommandAndProcessStatus(const char *cmd)
{
    char res[DRIVER_LEN] = {};
    if (!sendCommand(cmd, res))
        return false;

    processStatus(res);

    return true;
}

bool DragonUPS::sendCommand(const char *cmd, char *res)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;

    tcflush(PortFD, TCIOFLUSH);

    LOGF_DEBUG("CMD %s", cmd);
    rc = tty_write_string(PortFD, cmd, &nbytes_written);

    if (rc != TTY_OK)
    {
        char errstr[MAXRBUF] = {};
        tty_error_msg(rc, errstr, MAXRBUF);
        LOGF_ERROR("write error: %s.", errstr);

        abnormalDisconnect();

        return false;
    }

    if (res == nullptr)
        return true;

    rc = tty_nread_section(PortFD, res, DRIVER_LEN, DRIVER_STOP_CHAR, DRIVER_TIMEOUT, &nbytes_read);

    if (rc != TTY_OK)
    {
        char errstr[MAXRBUF] = {};
        tty_error_msg(rc, errstr, MAXRBUF);
        LOGF_ERROR("read error: %s.", errstr);

        abnormalDisconnect();

        return false;
    }

    // Remove extra #
    res[nbytes_read - 1] = 0;
    LOGF_DEBUG("RES %s", res);

    tcflush(PortFD, TCIOFLUSH);

    return true;
}
