# This is an ATMEL 32U4 based MPPT Buck Solar Charge Controller
## Features:
- Programmable with Arduino IDE
- Simple MPPT (Maximum Power Point Tracking) solar charge controller for 18V solar panels
- Proper buck converter topology, which increases the current on the output side, not just PWM
- Sparkfun Pro Micro 5V, 16MHz or 3.3V, 8MHz (3.3v recommended, more efficient)
- Also compatible with Arduino Pro Mini 3.3V, 8MHz or 5V, 16MHz
- ACS712 current sensor (5A version) on the output side (changed in V1.5)
- Voltage dividers for voltage measurement on panel and output side
- N-channel mosfet in GND line, freewheel diode, inductor
- Supplied by the panel voltage, so it can't drain your battery during the night
- Working frequency 31.5kHz
- WARNING! This device is not intended to drive 5V USB devices directly. Do it at your own risk!
- Always use a regulated 5V USB adapter on the output! Otherwise, voltage glichtes may damage your USB device!
- WARNING! This controller is COMMON POSITIVE!
- 3 opertation modes: MPPT, CV, CC allows to charge batteries directly, without an additional charger. Do it at your own risk!!
- WARNING! Always adjust output voltage and output current limits according to your battery type!!


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

New in V 1.4:
- CC (Constant Current) mode added. Allows to charge a lithium battery directly from your solar panel with CV - CC (do it at your own risk!)
- The output current is calculated depending on input voltage, input current & output voltage, so it is not very accurate.
- Warning! An anti backfeed diode is not yet implemented!
- For 3.0A output current limit, the diode requires a heat sink!

New in V 1.5:
- Diode BYT79 replaced with 5A / 60V Schottky Diode SB560. Efficiency raised from 70% to 82%
- ACS712 current sensor moved to output side, software changed accordingly. MPPT tracking improved.

New in V 1.6:
- Mosfet STP35NF10 replaced with Logic Level compatible IRLZ44NPBF. Allows to use 3.3V MCU
- A 3.3V, 8MHz MCU is now recommended, because it uses significantly less current
- Code for Arduino Pro Mini support added

## Usage

See pictures
![](https://github.com/TheDIYGuy999/MPPT_Buck_Converter_ACS712/blob/master/1.jpg)
![](https://github.com/TheDIYGuy999/MPPT_Buck_Converter_ACS712/blob/master/Board.png)

Also have a look at the pdf schematic.

(c) 2018 TheDIYGuy999
