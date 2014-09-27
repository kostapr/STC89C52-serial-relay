STC89C52-serial-relay
=====================

Relay output controlled by commands from UART. HW based on STC89C52RC

HOW to build
============
In order to build the HEX file, install SDCC compiler and MCU 8051 IDE.
Open the *.mcu8051ide project file using MCU 8051 IDE and compile the project.
The resulting *.hex file can be burned into target board using STC-ISD 
application version 4.83 (Windows). Later releases do not support STC89C52RC MCU.
This application has Chines GUI, but later English versions can be used for
understanding the interface elements (for instance versions 4.86 or 4.88).

HOW to use
==========
Use UART connection @ 9600 baud, 8 bit data, 1 stop bit, not flow control.
The following commands are supported:

Nx  - Turn ON port x. If x == 0, turn ON all ports 1 to 8

Fx  - Turn OFF port x. If x == 0, turn OFF all ports 1 to 8

Tx  - Toggle port x. If x == 0, toggle all ports 1 to 8

RHH - Set output port according to HEX bitmask HH (H = [0-9]|[A-F])

Sx  - Get status of port x. If x == 0, return status of all ports 1 to 8

This software can be used with wide range of cheap MCU boards (aka Chinese PLC).
It should not be too hard to modify this code for implementing 16 relays output.

Enjoy!
