# This is an ATMEL 32U4 based MPPT Buck Solar Charge Controller
## Features:
- Programmable with Arduino IDE
- Simple MPPT (Maximum Power Point Tracking) solar charge controller for 18V solar panels
- Proper buck converter topology, which increases the current on the output side, not just PWM
- Sparkfun Pro Micro 5V, 16MHz
- ACS712 current sensor (5A version) on the panel side
- Voltage dividers for voltage measurement on panel and output side
- N-channel mosfet in GND line, freewheel diode, inductor
- Supplied by the panel voltage, can't drain your battery
- Working frequency 31.5kHz
- WARNING! This device is not intended to drive 5V USB devices directly!
- Always use a regulated 5V USB adapter on the output! Otherwise, voltage glichtes may damage your USB device!
- WARNING! This controller is COMMON POSITIVE!


New in V 1.0:
- Initial commit, tested with my 10W and 20W solar panels, charging my DIY USB power bank with 8 18650 cells in parallel.

New in V 1.1:
- Improved MPPT strategy
- LED now also indicating controller mode (MPPT mode = volts, otherwise flickering)

New in V 1.2:
- Changed behavior below 0.2W input power

New in V 1.3:
- MPPT algorithm rewritten
- The resolution of the ACS712 is still problematic for MPPT tracking

## Usage

See pictures
![](https://github.com/TheDIYGuy999/MPPT_Buck_Converter_ACS712/blob/master/1.jpg)
![](https://github.com/TheDIYGuy999/MPPT_Buck_Converter_ACS712/blob/master/Board.png)

Also have a look at the pdf schematic.

(c) 2018 TheDIYGuy999
