Hardware requirements
=====================
- Micro USB cable
- MIMXRT700-EVK board
- Personal Computer

Board settings
============

Prepare the Demo
===============
1.  Connect a micro USB cable between the PC host and the MCU-LINK USB port on the board
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
When the demo runs successfully, the log would be seen on the terminal like:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
FATFS example to demonstrate how to use FATFS with MMC card.

Make file system......The time may be long if the card capacity is big.

Create directory......

Create a file in that directory......

List the file in that directory......
General file : F_1.DAT.

 Write/read file until encounters error......

Write to above created file.
Read from above created file.
Compare the read/write content.
The read/write content is consistent.

Input 'q' to quit read/write.
Input other char to read/write file again.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
