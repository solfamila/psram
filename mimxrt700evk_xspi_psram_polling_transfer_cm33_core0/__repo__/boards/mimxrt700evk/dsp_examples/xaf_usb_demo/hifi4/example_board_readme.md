Hardware requirements
=====================
- 2x Micro USB cable
- JTAG/SWD debugger
- MIMXRT700-EVK board
- Personal Computer
- Headphones with 3.5 mm stereo jack

Board settings
==============
1. Set the hardware jumpers (Tower system/base module) to default settings.

Prepare the Demo
================
NOTE: To be able to build the DSP project, please see the document
'Getting Started with Xplorer for MIMXRT700-EVK.pdf'.

1.  Connect headphones to Audio HP connector (J29).
2.  Connect the first micro USB cable between the PC host and the debug USB port (J54) on the board
3.  Connect the second micro USB cable between the PC host and the USB port (J40) on the board.
4.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
5.  Download the program for CM33 core to the target board.
6.  Launch the debugger in your IDE to begin running the demo.
7.  If building debug configuration, start the xt-ocd daemon and download the program for DSP core to the target board.
8.  If building debug configuration, launch the Xtensa IDE or xt-gdb debugger to begin running the demo.

NOTE: DSP image can only be debugged using J-Link debugger. See again
'Getting Started with Xplorer for MIMXRT700-EVK.pdf' for more information.

Known issue: When starting the "usb_speaker" after the "usb_mic" command, the sound output may be distorted.
    Please power cycle the board.

Running the demo CM33
=====================
When the demo runs successfully, the terminal will display similar output:

    ******************************
    DSP audio framework demo start
    ******************************

    [CM33 Main] Configure codec

    [DSP_Main] Cadence Xtensa Audio Framework
    [DSP_Main] Library Name    : Audio Framework (Hostless)
    [DSP_Main] Library Version : 3.5
    [DSP_Main] API Version     : 3.2

    [DSP_Main] start
    [DSP_Main] established RPMsg link
    [CM33 Main] DSP image copied to DSP TCM
    [CM33 Main][APP_DSP_IPC_Task] start
    [CM33 Main][APP_Shell_Task] start

    Copyright  2024  NXP

Demo commands:

"usb_speaker": Perform usb speaker device and playback on DSP
  USAGE: usb_speaker [start|stop]
    start          Start usb speaker device and playback on DSP
    stop           Stop usb speaker device and playback

  When usb_speaker command starts playback successfully, the terminal will display following output:
    [APP_DSP_IPC_Task] response from DSP, cmd: 21, error: 0
    DSP USB playback start
    >>

  Xtensa IDE log when command is playing a file:
    USB speaker start, initial buffer size: 960
    [DSP_USB_SPEAKER] Audio Device Ready
    [DSP_USB_SPEAKER] post-proc/pcm_gain component started
    [DSP_USB_SPEAKER] post-proc/client_proxy component started
    [DSP_USB_SPEAKER] Connected post-proc/pcm_gain -> post-proc/client_proxy
    [DSP_USB_SPEAKER] renderer component started
    [DSP_USB_SPEAKER] Connected post-proc/client_proxy -> renderer
    [DSP_ProcessThread] start
    [DSP_BufferThread] start
    [DSP_CleanupThread] start

  The USB device on your host will be enumerated as XAF USB DEMO.

  Xtensa IDE will not show any additional log entry.

Running the demo DSP
===================
Debug configuration:
When the demo runs successfully, the terminal will display the following:

    Cadence Xtensa Audio Framework
      Library Name    : Audio Framework (Hostless)
      Library Version : 2.6p1
      API Version     : 2.0

    [DSP_Main] start
    [DSP_Main] established RPMsg link

