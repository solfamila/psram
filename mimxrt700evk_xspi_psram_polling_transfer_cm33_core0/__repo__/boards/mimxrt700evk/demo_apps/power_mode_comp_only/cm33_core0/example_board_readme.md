Hardware requirements
=====================
- Micro USB cable
- MIMXRT700-EVK board
- Personal Computer

Board settings
============
Make sure JP7(2-3) connected for 3.3V supply for VDDIO_2. 
JP55(1-2) disconnected to ommit the impact of MCU-LINK during DPD and FDPD mode.

The board supports multiple power supply options. 
The code should be aligned with power supply on the board.

1. Default power supply, VDDN with PMIC, VDD1 and VDD2 with PMC.
   Jumper setting - JP1(Open), JP2(2-3), JP3(Open), JP4(1-2)
   Code setting(power_demo_config.h) - #define DEMO_POWER_SUPPLY_OPTION DEMO_POWER_SUPPLY_MIXED
2. Use PMIC to supply all. 
   Jumper setting - JP1(1-2), JP2(2-3), JP3(1-2), JP4(1-2)
   Code setting(power_demo_config.h) - #define DEMO_POWER_SUPPLY_OPTION DEMO_POWER_SUPPLY_PMIC


NOTE,
1. Please disconnect the debugger when measuring the power consumption.
2. When measuring power consumption for Full Deep Power Down mode, please power the board using 5V adapter and
   disconnect MCU-LINK and JP55 to ommit the impact of MCU-LINK circute.

Prepare the Demo
===============
1.  Connect a micro USB cable between the PC host and the MCU-LINK on the board
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Build cm33_core1 project first, then the cm33_core0 project.
4.  Download the core0 program to the target board.
5.  Either press the reset button on your board or launch the debugger in your IDE to
    begin running the demo.

Running the demo
================
The log below shows the output of the demo in the cm33_core0 terminal window:
```

####################  Power Mode Demo - CPU0 ####################

    Build Time: Feb  4 2024--14:01:54 
    Core Clock: 192000000Hz 
Select an option
    1. Sleep mode
    2. Deep Sleep mode
    3. Deep Sleep Retention mode
    4. Deep power down mode
    5. Full deep power down mode
    6. Active test mode

```

The cm33_core1 terminal window shows the below log,
```
####################  Power Mode Demo - CPU1 ####################

    Build Time: Feb  4 2024--13:36:32 
    Core Clock: 32000000Hz 
Entering Deep sleep...
```
