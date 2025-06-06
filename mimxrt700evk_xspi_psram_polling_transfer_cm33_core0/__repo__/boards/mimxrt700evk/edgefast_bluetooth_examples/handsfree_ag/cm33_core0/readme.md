# handsfree_ag

## Overview
This example demonstrates the HFP Ag basic functionality, currently support simulate an incoming call, and the call could be answered and ended.

The HFP Ag can connected a HFP HF device like headphone or device running HFP HF device.

## Prepare the Demo

1.  Open example's project and build it.

2.  Connect a USB cable between the PC host and the OpenSDA USB port on the target board.

3.  Open a serial terminal on PC for OpenSDA serial device with these settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control

4.  Download the program to the target board.

5.  Either press the reset button on your board or launch the debugger in your IDE to begin running the example.

## Running the demo
The log below shows the output of the example in the terminal window. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
USB Host stack successfully initialized
Bluetooth initialized

SHELL build: Mar  1 2021
Copyright  2020  NXP

>>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
the bellow commands have been supported:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"bt": BT related function
  USAGE: bt [discover|connect|disconnect|delete]
    discover             start to find BT devices
    connect              connect to the device that is found, for example: bt connect n (from 1)
    openaudio            open audio connection without calls
    closeaudio           close audio connection without calls 
    sincall              start an incoming call.
    aincall              accept the call.
    eincall              end an call.
    set_tag              set phone num tag, for example: bt set_tag 123456789.
    select_codec         codec select for codec Negotiation, for example: bt select_codec 2, it will select the codec 2 as codec.
    set_mic_volume       update mic Volume, for example: bt set_mic_volume 14.
    set_speaker_volume   update Speaker Volume, for example: bt set_speaker_volume 14.
    stwcincall           start multiple an incoming call.
    disconnect           disconnect current connection.
    delete               delete all devices. Ensure to disconnect the HCI link connection with the peer device before attempting to delete the bonding information.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
1) "discover" start to find BT devices, it will list all device can be found, usage
2) "connect" is used to connect to the device that is found, for example: bt connect n (from 1), usage :
   bt connect 1
3) "openaudio" is used to open audio connection without calls
4) "closeaudio" is used to close audio connection without calls 
5) "sincall" is used to start an incoming call
6) "aincall" is used to accept an incoming call
7) "eincall" is used to end or reject an incoming call
8) "set_tag" is used to set phone num tag, for example: bt set_tag 123456789
9) "select_codec" is used to  codec select for codec Negotiation, for example: bt select_codec 2, it will select the codec 2 as codec, usage:
   bt select_codec 2
10) "set_mic_volume" is used to set mic volume, the value  is from 1 to 15, usage as:
    bt set_mic_volume 8
11) "set_speaker_volume" is used to set speaker volume, the value is from 1 to 15, usage as:
    bt set_speaker_volume 8
12) "stwcincall"  to start multiple an incoming call, need run "sincall" is used to start an incoming call before run the command
13) "disconnect"  to disconnect current connection
14) "delete" is used to delete all devices. Ensure to disconnect the HCI link connection with the peer device before attempting to delete the bonding information.

Note:
1. There is a short noise can be heard at headset at the begin audio streaming when in running HFP Ag . 
2. The codec power on pop noise cannot eliminate.

## Supported Boards
- [EVKB-IMXRT1050](../../_boards/evkbimxrt1050/edgefast_bluetooth_examples/handsfree_ag/example_board_readme.md)
- [MIMXRT1170-EVKB](../../_boards/evkbmimxrt1170/edgefast_bluetooth_examples/handsfree_ag/example_board_readme.md)
- [MIMXRT1060-EVKC](../../_boards/evkcmimxrt1060/edgefast_bluetooth_examples/handsfree_ag/example_board_readme.md)
- [MIMXRT1040-EVK](../../_boards/evkmimxrt1040/edgefast_bluetooth_examples/handsfree_ag/example_board_readme.md)
- [MIMXRT1180-EVK](../../_boards/evkmimxrt1180/edgefast_bluetooth_examples/handsfree_ag/example_board_readme.md)
- [EVK-MIMXRT595](../../_boards/evkmimxrt595/edgefast_bluetooth_examples/handsfree_ag/example_board_readme.md)
- [EVK-MIMXRT685](../../_boards/evkmimxrt685/edgefast_bluetooth_examples/handsfree_ag/example_board_readme.md)
- [MCX-N5XX-EVK](../../_boards/mcxn5xxevk/edgefast_bluetooth_examples/handsfree_ag/example_board_readme.md)
- [MIMXRT700-EVK](../../_boards/mimxrt700evk/edgefast_bluetooth_examples/handsfree_ag/example_board_readme.md)
