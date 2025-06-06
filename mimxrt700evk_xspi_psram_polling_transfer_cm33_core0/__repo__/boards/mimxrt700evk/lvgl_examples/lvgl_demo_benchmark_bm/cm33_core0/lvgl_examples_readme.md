Hardware requirements
===================
- Micro USB cable
- TFT Proto 5" CAPACITIVE board HW REV 1.01 by Mikroelektronika. (Named as SSD1963 panel in project. Not necessary if use MIPI panel)
- MIMXRT700-EVK
- Personal Computer
- RK055AHD091 MIPI panel (Not necessary if use other panel)
- RK055IQH091 MIPI panel (Not necessary if use other panel)
- RK055MHD091 MIPI panel (Not necessary if use other panel)
- RM67162 smart MIPI panel (Not necessary if use other panel)
- RaspberryPi Panel (Not necessary if use other panel)
- ZC143AC72MIPI DBI panel (Not necessary if use other panel)

Board settings
============
To use SSD1963 panel:
Connect SSD1963 panel to J4. Make sure to connect JP7 2&3 to use 3.3v interface.

To use MIPI panel:
Connect MIPI panel to J52.

To use Raspberry panel:
Connect the panel to J8. Then connect the panel's 5V pin to JP43-1, GND pin to JP43-2.
Make sure the R75, R76, R79, R80 are connected.

Prepare the Demo
===============
1.  Connect a USB cable between the host PC and the OpenSDA USB port on the target board.
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Download the program to the target board.
4.  Either press the reset button on your board or launch the debugger in your IDE to begin running the demo.

Steps to configure the panels
===============
The demo use MIPI RK055MHD091 panel by default.
If Kconfig is used, select the panel in Kconfig "Panel Selection".
To use SSD1963 DBI panel, change the macro DEMO_PANEL to 4 in mcux_config.h.
To use MIPI RK055AHD091 panel, change the macro DEMO_PANEL to 0 in mcux_config.h.
To use MIPI RK055IQH091 panel, change the macro DEMO_PANEL to 1 in mcux_config.h.
To use RM67162 MIPI smart DBI panel, change the macro DEMO_PANEL to 3 in mcux_config.h.
To use MIPI RK055MHD091 panel, change the macro DEMO_PANEL to 2 in mcux_config.h(default).
To use RaspberryPi, change the macro DEMO_PANEL to 5 in mcux_config.h.
To use ZC143AC72MIPI DBI panel, change the macro DEMO_PANEL to 6 in mcux_config.h.

When package is used
----------------
Modify the mcux_config.h.
To use DBI panels: SSD1963, RM67162 and ZC143AC72MIPI, below change is needed:
```
#define CONFIG_LV_USE_PRIVATE_API 1
```

Note:
When RGB565 or XRGB8888 is selected, changing `LV_COLOR_DEPTH` in Kconfig or mcux_config.h
doesn't affect the real pixel format.

When RGB888 is selected.

#define CONFIG_LV_COLOR_DEPTH_24 1
#define CONFIG_LV_COLOR_DEPTH 24
#define CONFIG_LV_DRAW_BUF_ALIGN 192

For SSD1963 DBI panel:
#define DEMO_SSD1963_BUFFER_FORMAT 1
For RM67162 MIPI smart DBI panel:
#define DEMO_RM67162_BUFFER_FORMAT 1
For ZC143AC72MIPI DBI panel:
#define DEMO_CO5300_BUFFER_FORMAT  1


Running the demo
===============
There is tearing effect when using the SSD1963 panel, because the panel video buffer
could not be fully updated during fresh interval.
