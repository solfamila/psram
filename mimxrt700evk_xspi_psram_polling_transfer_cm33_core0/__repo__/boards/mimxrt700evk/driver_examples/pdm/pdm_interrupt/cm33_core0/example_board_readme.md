Hardware requirements
=====================
- Micro USB cable
- MIMXRT700-EVK board
- Personal Computer

Board settings
============


Prepare the Demo
===============
1.  Connect a micro USB cable between the PC host and the MCU-LINK USB port (J54) on the board
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Download the program to the target board.
4.  Launch the debugger in your IDE to begin running the demo.


Running the demo
===============
When the demo runs successfully,  the log would be seen on the terminal like:
~~~~~~~~~~~~~~~~~~~
PDM interrupt example started!
PDM recieve data:
XXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXX
PDM interrupt example finished!
~~~~~~~~~~~~~~~~~~~


