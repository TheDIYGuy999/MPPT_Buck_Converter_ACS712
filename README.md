# This is an ATMEL 32U4 based MPPT Buck Solar Charge controller
## Features:
- Programmable with Arduino IDE
- Simple MPPT solar charge controller for 18V solar panels
- Sparkfun Pro Micro 5V, 16MHz
- ACS712 current sensor on the panel side
- Voltage dividers for voltage measurement on panel and output side
- N-channel mosfet in GND line, freewheel diode, inductor
- Working frequency 31.5kHz
- WARNING! This device is not intended to drive 5V USB devices directly!
- Always use a regulated 5V USB adapter on the output! Otherwise, voltage glichtes may damage your USB device!
- WARNING! This controller is COMMON POSITIVE!



New in V 1.0:
- Initial commit, tested with my 10W and 20W solar panels, charging my DIY USB power bank with 8 18650 cells in parallel.

## Usage

See pictures
![](https://github.com/TheDIYGuy999/MPPT_Buck_Converter/blob/master/1.jpg)
![](https://github.com/TheDIYGuy999/MPPT_Buck_Converter/blob/master/Board.jpg)
![](https://github.com/TheDIYGuy999/MPPT_Buck_Converter/blob/master/Schematic.pdf)


(c) 2018 TheDIYGuy999
