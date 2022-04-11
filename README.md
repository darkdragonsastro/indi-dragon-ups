# Dark Dragon's Astronomy

## Dragon UPS INDI Driver

This driver monitors the voltage of a battery and uses the Weather interface to
make conditions unsafe when the voltage drops below a configurable value (defaults
to 12.5V).

### Building

```
rm -Rf build
mkdir build
cd build
cmake ..
cmake --build .
cpack .
```
