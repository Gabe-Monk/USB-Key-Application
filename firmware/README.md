# firmware/
This directory stores the files which live on the USB key itself.

## Instructions
The system is built off of a Raspberyy Pi Pico microcontroller. That microcontroller must have Adafruit's CircuitPython firmware installed. The installation instructions can be found [here](https://learn.adafruit.com/welcome-to-circuitpython/installing-circuitpython). The following libraries will also need to be installed on the board afterwards (by putting them into the `lib` directory on the board's file system), and they can be downloaded [here](https://circuitpython.org/libraries).

- `adafruit_bus_device.i2c_device`
