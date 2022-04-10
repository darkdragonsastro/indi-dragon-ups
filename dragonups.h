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

#include "config.h"
#include <libindi/defaultdevice.h>
#include <libindi/indiweatherinterface.h>

namespace INDI
{

/**
 * @brief The DragonUPS class interfaces with the Dark Dragons Astronomy
 * Dragon UPS Controller.
 *
 * @author Rick Bassham
 */
class DragonUPS : public INDI::DefaultDevice, public INDI::WeatherInterface
{
public:
    DragonUPS();
    virtual ~DragonUPS() = default;

    virtual bool initProperties() override;
    const char *getDefaultName() override;
    virtual bool updateProperties() override;

    static void abnormalDisconnectCallback(void *userpointer);

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;

protected:
    bool Handshake();
    void TimerHit() override;
    virtual IPState updateWeather() override
    {
        return IPS_OK;
    }

private:
    bool sendCommandAndProcessStatus(const char *cmd);

    bool sendCommand(const char *cmd, char *res);

    bool checkStatus();

    void processStatus(const char *status);
    void abnormalDisconnect();

    int connectRetries = 0;

    Connection::TCP *tcpConnection{ nullptr };

    INumber VoltageSensorN[1];
    INumberVectorProperty VoltageSensorNP;

    int PortFD { -1 };

}; // class DragonUPS

}; // namespace INDI
