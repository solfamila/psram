# unicast_media_receiver

## Overview
Application demonstrating how to use the unicast media receiver feature.

There should be three boards: 1 UMS + 1 UMR(left) + 1 UMR(right).
- UMS: unicast stereo audio stream, left channel on first CIS and right channel on another CIS.
- UMR: receive one of CIS channel and render it.

## Prepare the Demo

1.  Open example's project and build it.

2.  Connect a USB cable between the PC host and the OpenSDA USB port on the target board.

3.  Provide 5V voltage for the target board.

4.  Connect speaker to Audio Jack.

5.  Open a serial terminal on PC for OpenSDA serial device with these settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control

6.  Download the program to the target board.

7.  Either press the reset button on your board or launch the debugger in your IDE to begin running the example.

## Running the demo
The log below shows the output of the example in the terminal window.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Copyright  2024  NXP

UMR>> 
Unicast Media Receiver.

Please select sink role "left"|"right" use "init" command.

UMR>> init left

UMR@left>> UMR@left>> Bluetooth initialized
Set info:
	sirk: 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 
	set_size: 2
	rank: 1
	lockable: 1
Location successfully set
Supported contexts successfully set
Available contexts successfully set
Advertising successfully started
Connected: D0:17:69:EE:71:13 (public)
Security changed: D0:17:69:EE:71:13 (public) level 2 (error 0)
MCS server discover:
MCS server discovered.
ASE Codec Config: conn 202D2A0C ep 202D133C dir 1
codec_cfg 0x06 cid 0x0000 vid 0x0000 count 19
data: type 0x01 value_len 1
07
data: type 0x02 value_len 1
01
data: type 0x03 value_len 4
01000000
data: type 0x04 value_len 2
8200
data: type 0x05 value_len 1
01
  Frequency: 44100 Hz
  Frame Duration: 10000 us
  Channel allocation: 0x1
  Octets per frame: 130 (negative means value not pressent)
  Frames per SDU: 1
ASE Codec Config stream 20306D38
QoS: stream 20306D38 qos 202DD770
QoS: interval 10884 framing 0x01 phy 0x02 sdu 130 rtn 1 latency 31 pd 40000
Enable: stream 20306D38 meta_len 4
	Codec: freq 44100, channel count 1, duration 10000, channel alloc 0x00000001, frame len 130, frame blocks per sdu 1
Unicast stream started
Stream 20306D38 started
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Procedures to run
1. input "help" to show command list
2. input "init left" or "init right" to start sink role and start to advertise, audio will start playing after connect and configure finished.
3. input "pause" to stop playing.
4. input "play" to start playing.
5. input "vol_up", "vol_down", "vol_set" to set volume.
6. input "vol_mute", "vol_unmute" to set mute.

Other cmds could be used:
1. "sync_info" used to get iso_interval/sync_delay/pd/ts, and this cmd should be used after the audio stream start.
2. "sync_test_mode" used to set the test mode, and this cmd should be used before init.
3. "set_sirk" used to set sirk, and this cmd should be used before init.

Note:
1. "exit" command is a shell internal command, only used to exit shell module and could not used to exit demo.
2. There is a delay in synchronizing volume/mute status between receivers. Setting a smaller connection interval can reduce the delay, but may introduce other timing issues.

## Supported Boards
- [MIMXRT1170-EVKB](../../_boards/evkbmimxrt1170/edgefast_bluetooth_examples/unicast_media_receiver/example_board_readme.md)
- [MIMXRT1060-EVKC](../../_boards/evkcmimxrt1060/edgefast_bluetooth_examples/unicast_media_receiver/example_board_readme.md)
- [MIMXRT700-EVK](../../_boards/mimxrt700evk/edgefast_bluetooth_examples/unicast_media_receiver/example_board_readme.md)
