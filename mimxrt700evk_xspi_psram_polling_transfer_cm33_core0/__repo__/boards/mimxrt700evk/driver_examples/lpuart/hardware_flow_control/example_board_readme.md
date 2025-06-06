Hardware requirements
=====================
- Micro USB cable
- MIMXRT700-EVK board
- Personal Computer

Board settings
============
For EVK, make the following connection for UART loopback.
Open JP47, Open JP49.
Connect JP47-1(RXD) to JP49-1(TXD). 
Connect J6-9(RTS) to J6-10(CTS).

Prepare the Demo
===============
Note: MCUXpresso IDE project default debug console is semihost
1. Connect a micro USB cable between the PC host and the DEBUG PORT(J54) on the board.
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3. Download the program to the target board.
4. Either press the reset button on your board or launch the debugger in your IDE to begin running the demo.

Running the demo
================
When the demo runs successfully, the log would be seen on the MCU-LINK terminal like:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Lpuart functional interrupt example
Board receives characters then sends them out
Now please input:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~