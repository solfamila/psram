Hardware requirements
=====================
- Micro USB cable
- MIMXRT700-EVK board
- Personal Computer

Board settings
============

Prepare the Demo
===============
1.  Connect a micro USB cable between the PC host and the MCU-LINK on the board
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Download the program to the target board.
4.  Launch the debugger in your IDE to begin running the demo.

Running the demo
================
The log below shows the output of the shell demo in the terminal window:
~~~~~~~~~~~~~~~~~~~~~
Copyright  2024  NXP

LOG SHELL>> installbackend debugconsole
LOG SHELL>> log fatal
           0: FATAL log_main.c:83:This is "fatal" log message

LOG SHELL>> log error
           0: ERROR log_main.c:87:This is "error" log message

LOG SHELL>> log warning
           0: WARN  log_main.c:91:This is "warning" log message

LOG SHELL>> log info
           0: INFO  log_main.c:95:This is "info" log message

LOG SHELL>> log debug
           0: DEBUG log_main.c:99:This is "debug" log message

LOG SHELL>> log trace
           0: TRACE log_main.c:103:This is "trace" log message

LOG SHELL>> log test
           0: ERROR log_main.c:107:The input arguement "test" is not valid

LOG SHELL>> uninstallbackend debugconsole
LOG SHELL>> 
~~~~~~~~~~~~~~~~~~~~~
