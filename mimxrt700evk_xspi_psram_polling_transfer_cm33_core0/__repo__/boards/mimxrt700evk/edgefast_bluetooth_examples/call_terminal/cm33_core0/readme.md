# call_terminal

## Overview
Application demonstrating how to use the telephone call terminal feature of LE Audio.

## Prepare the Demo

1.  Open example's project and build it.

2.  Connect a USB cable between the PC host and the OpenSDA USB port on the target board.

3.  Provide 5V voltage for the target board.

4.  Open a serial terminal on PC for OpenSDA serial device with these settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control

5.  Download the program to the target board.

6.  Either press the reset button on your board or launch the debugger in your IDE to begin running the example.

Note: the example can work with call_gateway example. Please refer to the readme of call_gateway to prepare the call_gateway exmaple.

## Running the demo
The log below shows the output of the example in the terminal window.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Copyright  2022  NXP

call_terminal>> Bluetooth initialized
Advertising successfully started

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Procedures to run
1 input "help" to show command list,
2 the advertising of the device is started automatically,
3 Aftert the conenction is estabilished, the log is following.
Please note that if the Security is changed with error 0x02, it means the key is missing. Please clear bonding information by sending
commander "unpair". And, retry again.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MTU exchanged: 23/23
Starting TBS server discover
Connected to peer: A0:CD:F3:77:E3:E9 (public)
MTU exchanged: 65/65
Security changed: A0:CD:F3:77:E3:E9 (public) level 2 (error 0)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
4 After the message "Discover complete (err 0)! TBS count 1, GTBS found? Yes" is printed on call_terminal side. All feature are ready.
5.1. Start the call by local.
The commander is showing as "call_outgoing \<telephone bearer index\> \<callee_URI\>". The "\<telephone bearer index\>" is 0 for the application. The "\<callee_URI\>" is the callee URI. Enter "call_outgoing 0 \<XX\>:\<YY\>" on the call_gateway side, or enter "call_outgoing 0 \<XX\>:\<YY\>" on the call_terminal side.
Such as the entered command line is "call_outgoing 0 tel:qq" on the call_gateway side, the "\<telephone bearer index\>" is 0, and the "\<callee_URI\>" is "tel:qq" here.
The log is following,
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 1, flags 1.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 1, flags 1, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 1, flags 1.
List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 2, flags 1.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 2, flags 1, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 2, flags 1.
ASE Codec Config: conn 202DE340 ep 202D9214 dir 2
Codec configurations:
    Frequency 16000
    Duration 10000
    Frame bytes 40
    Frame blocks per SDU 1
    Location 3, channel count 2.
ASE Codec Config: conn 202DE340 ep 202D92DC dir 1
Codec configurations:
    Frequency 16000
    Duration 10000
    Frame bytes 40
    Frame blocks per SDU 1
    Location 3, channel count 2.
QoS: stream 202EFF80 qos 202D9284
    interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000
QoS: stream 202F6350 qos 202D934C
    interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000
Enable: stream 202EFF80 meta_count 1
Enable: stream 202F6350 meta_count 1
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Start: stream 202F6350
Start: stream 202EFF80
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Or, enter "call_outgoing 0 \<XX\>:\<YY\>" on the call_terminal side.
The log is following,
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
outgoing call: callee uri tel:qq, TBS index 0
Return code 0
call_terminal>> List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 1, flags 1.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 1, flags 1, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 1, flags 1.
List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 2, flags 1.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 2, flags 1, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 2, flags 1.
Control Point status update. A call outgoing (err 0). TBS Index 0, call index 1
List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 2, flags 1.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 2, flags 1, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 2, flags 1.
List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 2, flags 1.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 2, flags 1, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 2, flags 1.
ASE Codec Config: conn 202DE340 ep 202D9214 dir 2
Codec configurations:
    Frequency 16000
    Duration 10000
    Frame bytes 40
    Frame blocks per SDU 1
    Location 3, channel count 2.
ASE Codec Config: conn 202DE340 ep 202D92DC dir 1
Codec configurations:
    Frequency 16000
    Duration 10000
    Frame bytes 40
    Frame blocks per SDU 1
    Location 3, channel count 2.
QoS: stream 202EFF80 qos 202D9284
    interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000
QoS: stream 202F6350 qos 202D934C
    interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000
Enable: stream 202EFF80 meta_count 1
Enable: stream 202F6350 meta_count 1
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Start: stream 202F6350
Start: stream 202EFF80
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

5.2 Start a call by remote.
The commander is showing as "remote_call_incoming \<telephone bearer index\> \<callee_URI\> \<caller_URI\> \<caller_name\>".
The "\<telephone bearer index\>" is 0 for the application. The "\<callee_URI\>" and "\<caller_URI\>" are the callee URI and caller URI. "\<caller_name\>" is caller name. Enter "remote_call_incoming 0 \<AA\>:\<BB\> \<CC\>:\<DD\> \<EE\>" on the call_gateway side.
Such as the entered command line is "remote_call_incoming 0 tel:qq tel:qq qq", the "\<telephone bearer index\>" is 0. The "\<callee_URI\>" is "tel:qq". The "\<caller_URI\>" is "tel:qq". "\<caller_name\>" is "qq".
After the command line has been entered by call_gateway, the log is following on call_terminal side,
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Read incoming call URI tel:qq (err 0). TBS Index 0.
incoming call inst_index 0, call_index = 1, uri tel:qq
Read Friendly name qq (err 0). TBS Index 0.
Read incoming call URI tel:qq (err 0). TBS Index 255.
incoming call inst_index 255, call_index = 1, uri tel:qq
Read Friendly name  (err 0). TBS Index 255.
List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 0, flags 0.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 0, flags 0, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 0, flags 0.
ASE Codec Config: conn 202DE340 ep 202D9214 dir 2
Codec configurations:
    Frequency 16000
    Duration 10000
    Frame bytes 40
    Frame blocks per SDU 1
    Location 3, channel count 2.
ASE Codec Config: conn 202DE340 ep 202D92DC dir 1
Codec configurations:
    Frequency 16000
    Duration 10000
    Frame bytes 40
    Frame blocks per SDU 1
    Location 3, channel count 2.
QoS: stream 202EFF80 qos 202D9284
    interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000
QoS: stream 202F6350 qos 202D934C
    interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000
Enable: stream 202EFF80 meta_count 1
Enable: stream 202F6350 meta_count 1
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Start: stream 202F6350
Start: stream 202EFF80

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

6.1 Reject/end the remote call, enter "call_term \<call_index\>" on the call_gateway side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Call terminated(err 0). TBS Index 0, call index 1, reason 3.
Speaker mute
Call terminated(err 0). TBS Index 255, call index 1, reason 3.
Disable: stream 202EFF80
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Stop: stream 202EFF80
Disable: stream 202F6350
Audio Stream 202F6350 stopped with reason 0x13
Audio Stream 202EFF80 stopped with reason 0x13
Release: stream 202EFF80
Release: stream 202F6350

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Or, enter "call_term 0 \<call_index\>" on the call_terminal side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Terminate call: TBS index 0, call index 1
Return code 0
call_terminal>> Call terminated(err 0). TBS Index 0, call index 1, reason 6.
Speaker mute
Call terminated(err 0). TBS Index 255, call index 1, reason 6.
Control Point status update. A call has been terminated (err 0). TBS Index 0, call index 1
List current state of current calls (err 0). TBS Index 255, call count 0, call state list,
List current calls (err 0). TBS Index 255, call count 0, call list,
List current state of current calls (err 0). TBS Index 0, call count 0, call state list,
List current calls (err 0). TBS Index 0, call count 0, call list,
Disable: stream 202EFF80
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Stop: stream 202EFF80
Disable: stream 202F6350
Audio Stream 202F6350 stopped with reason 0x13
Audio Stream 202EFF80 stopped with reason 0x13
Release: stream 202EFF80
Release: stream 202F6350

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
6.2 Reject/end the call by remote. enter "remote_call_term \<call_index\>" on the call_gateway side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Call terminated(err 0). TBS Index 0, call index 1, reason 2.
Speaker mute
Call terminated(err 0). TBS Index 255, call index 1, reason 2.
Disable: stream 202EFF80
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Stop: stream 202EFF80
Disable: stream 202F6350
Audio Stream 202F6350 stopped with reason 0x13
Audio Stream 202EFF80 stopped with reason 0x13
Release: stream 202EFF80
Release: stream 202F6350
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

7.1 Accept the remote call. enter "call_accept \<call_index\>" on the call_gateway side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 3, flags 0.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 3, flags 0, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 3, flags 0.
Disable: stream 202EFF80
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Stop: stream 202EFF80
Disable: stream 202F6350
Audio Stream 202F6350 stopped with reason 0x13
Audio Stream 202EFF80 stopped with reason 0x13
Enable: stream 202EFF80 meta_count 1
Enable: stream 202F6350 meta_count 1
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Start: stream 202F6350
Start: stream 202EFF80

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Or, enter "call_accept 0 \<call_index\>" on the call_terminal side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
accept call: TBS index , call index 1
Return code 0
call_terminal>> Control Point status update. A call has been accepted (err 0). TBS Index 0, call index 1
List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 3, flags 0.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 3, flags 0, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 3, flags 0.
Disable: stream 202EFF80
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Stop: stream 202EFF80
Disable: stream 202F6350
Audio Stream 202F6350 stopped with reason 0x13
Audio Stream 202EFF80 stopped with reason 0x13
Enable: stream 202EFF80 meta_count 1
Enable: stream 202F6350 meta_count 1
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Start: stream 202F6350
Start: stream 202EFF80

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
7.2 Accept the call by remote device. enter "remote_call_answer \<call_index\>" on the call_gateway side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
List current state of current calls (err 0). TBS Index 255, call count 1, call state list,
call index 1, state 3, flags 1.
List current calls (err 0). TBS Index 255, call count 1, call list,
call index 1, state 3, flags 1, remote uri tel:qq
List current state of current calls (err 0). TBS Index 0, call count 1, call state list,
call index 1, state 3, flags 1.
Disable: stream 202EFF80
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Fail to send stream (error -77)
Stop: stream 202EFF80
Disable: stream 202F6350
Audio Stream 202F6350 stopped with reason 0x13
Audio Stream 202EFF80 stopped with reason 0x13
Enable: stream 202EFF80 meta_count 1
Enable: stream 202F6350 meta_count 1
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Start: stream 202F6350
Start: stream 202EFF80

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Supported Boards
- [MIMXRT1170-EVKB](../../_boards/evkbmimxrt1170/edgefast_bluetooth_examples/call_terminal/example_board_readme.md)
- [MIMXRT1060-EVKC](../../_boards/evkcmimxrt1060/edgefast_bluetooth_examples/call_terminal/example_board_readme.md)
- [MIMXRT700-EVK](../../_boards/mimxrt700evk/edgefast_bluetooth_examples/call_terminal/example_board_readme.md)
