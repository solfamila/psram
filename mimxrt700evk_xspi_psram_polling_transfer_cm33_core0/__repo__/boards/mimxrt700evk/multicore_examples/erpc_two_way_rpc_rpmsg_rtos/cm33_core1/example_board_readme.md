Hardware requirements
=====================
- Mini/micro USB cable
- MIMXRT700-EVK board
- Personal Computer

Board settings
==============
The Multicore eRPC Two Way RPC RTOS project does not call for any special hardware configurations.
Although not required, the recommendation is to leave the development board jumper settings and
configurations in default state when running this demo.

Prepare the Demo
================
1.  Connect the PC host and the board
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Download the program to the target board.
4.  Either press the reset button on your board or launch the debugger in your IDE to begin running the demo.


Running the demo
================
The log below shows the output of the eRPC Two Way RPC RTOS demo in the terminal window:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Primary core started

Get number from core1:
Got number: 1
Start normal rpc call example.
RPC call example finished.

Get number from core1:
Got number: 2
Start normal rpc call example.
RPC call example finished.

Get number from core1:
Got number: 3
Start normal rpc call example.
RPC call example finished.
.
.
.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
