# call_gateway

## Overview
Application demonstrating how to use the telephone call gateway feature of LE Audio.

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

Note: the example can work with call_terminal example. Please refer to the readme of call_terminal to prepare the call_terminal exmaple.

## Running the demo
The log below shows the output of the example in the terminal window.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Copyright  2022  NXP

call_gateway>> Bluetooth initialized
Get required Source Capability from codec. Codec configurations:
    Frequency 16000
    Duration 10000
    Frame bytes 40
    Frame blocks per SDU 1
    Location 3, channel count 2.
Get required Sink Capability from codec. Codec configurations:
    Frequency 16000
    Duration 10000
    Frame bytes 40
    Frame blocks per SDU 1
    Location 3, channel count 2.
Scanning started

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Procedures to run
1. input "help" to show command list,
2. the scanning of the device is started automatically. It starts to scann the call_terminal device,
3. After the conenction is estabilished, the log is following.

Please note that if the Security is changed with error 0x02, it means the key is missing. Please clear bonding information by sending
commander "unpair". And, retry again.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Found device: A0:CD:F3:77:E6:15 (public)MTU exchanged: 23/23
Connected to peer: A0:CD:F3:77:E6:15 (public)
MTU exchanged: 65/65
Security changed: A0:CD:F3:77:E6:15 (public) level 2 (error 0)
codec capabilities on conn 202DE3C0 dir 1 codec 2000D054. Codec configurations:
    Frequency 8000, 16000, 24000, 32000, 44100, 48000,
    Duration 10000,
    Channel count 2.
    Frame length min 40, max 120
    Frame blocks per SDU 1
    Pref context 0x206
conn 202DE3C0 dir 1 loc 3
conn 202DE3C0 snk ctx 519 src ctx 3
conn 202DE3C0 dir 1 ep 202DAC30
Discover sinks complete: err 0
codec capabilities on conn 202DE3C0 dir 2 codec 2000D054. Codec configurations:
    Frequency 8000, 16000, 24000, 32000, 44100, 48000,
    Duration 10000,
    Channel count 2.
    Frame length min 40, max 120
    Frame blocks per SDU 1
    Pref context 0x206
conn 202DE3C0 dir 2 loc 3
conn 202DE3C0 snk ctx 519 src ctx 3
conn 202DE3C0 dir 2 ep 202DAE38
Discover sources complete: err 0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
4 After the message "Discover complete (err 0)! TBS count 1, GTBS found? Yes" is printed on call_terminal side. All features are ready.

5.1. Start the call by local.
The commander is showing as "call_outgoing \<telephone bearer index\> \<callee_URI\>". The "\<telephone bearer index\>" is 0 for the application. The "\<callee_URI\>" is the callee URI. Enter "call_outgoing 0 \<XX\>:\<YY\>" on the call_gateway side, or enter "call_outgoing 0 \<XX\>:\<YY\>" on the call_terminal side.
Such as the entered command line is "call_outgoing 0 tel:qq", the "\<telephone bearer index\>" is 0, and the "\<callee_URI\>" is "tel:qq" here.
The log is following,
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
call_outgoing 0 tel:qq
Start a outgoing call, call index 1, callee tel:qq
Audio Stream 202F0688 configured
Audio Stream 202F0650 configured
Audio Stream 202F0688 QoS set
Audio Stream 202F0650 QoS set
Audio Stream 202F0688 enabled
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Audio Stream 202F0650 enabled
Audio Stream 202F0650 started
Audio Stream 202F0688 started
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
5.2 Start a call by remote.
The commander is showing as "remote_call_incoming \<telephone bearer index\> \<callee_URI\> \<caller_URI\> \<caller_name\>".
The "\<telephone bearer index\>" is 0 for the application. The "\<callee_URI\>" and "\<caller_URI\>" are the callee URI and caller URI. "\<caller_name\>" is caller name. Enter "remote_call_incoming 0 \<AA\>:\<BB\> \<CC\>:\<DD\> \<EE\>" on the call_gateway side.
Such as the entered command line is "remote_call_incoming 0 tel:qq tel:qq qq", the "\<telephone bearer index\>" is 0. The "\<callee_URI\>" is "tel:qq". The "\<caller_URI\>" is "tel:qq". "\<caller_name\>" is "qq".
The log is following,
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
remote_call_incoming 0 tel:qq tel:qq qq
incoming call: callee uri tel:qq, caller uri tel:qq
Audio Stream 202F0688 configured
Audio Stream 202F0650 configured
Audio Stream 202F0688 QoS set
Audio Stream 202F0650 QoS set
Audio Stream 202F0688 enabled
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Audio Stream 202F0650 enabled
Audio Stream 202F0650 started
Audio Stream 202F0688 started
done, call index is 0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

6.1 Reject/end the remote call, enter "call_term \<call_index\>" on the call_gateway side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
terminate the call: call index 1
Audio Stream 202F0688 disabled
Audio Stream 202F0688 QoS set
Audio Stream 202F0650 disabled
Fail to stop stream (err -77)
Audio Stream 202F0650 QoS set
Audio Stream 202F0688 stopped with reason 0x13
Audio Stream 202F0650 stopped with reason 0x13
Audio Stream 202F0688 released
Audio Stream 202F0650 released
Return code 0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Or, enter "call_term 0 \<call_index\>" on the call_terminal side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Terminate a call, call index 1, reason 6
Audio Stream 202F0688 disabled
Audio Stream 202F0688 QoS set
Audio Stream 202F0650 disabled
Fail to stop stream (err -77)
Audio Stream 202F0650 QoS set
Audio Stream 202F0688 stopped with reason 0x13
Audio Stream 202F0650 stopped with reason 0x13
Audio Stream 202F0688 released
Audio Stream 202F0650 released
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
6.2 Reject/end the call by remote. enter "remote_call_term \<call_index\>" on the call_gateway side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Remove terminate the call: call index 1
Audio Stream 202F0688 disabled
Audio Stream 202F0688 QoS set
Audio Stream 202F0650 disabled
Fail to stop stream (err -77)
Audio Stream 202F0650 QoS set
Audio Stream 202F0688 stopped with reason 0x13
Audio Stream 202F0650 stopped with reason 0x13
Audio Stream 202F0688 released
Audio Stream 202F0650 released
Return code 0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

7.1 Accept the remote call. enter "call_accept \<call_index\>" on the call_gateway side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
accept the call: call index 1
Audio Stream 202F0688 disabled
Audio Stream 202F0688 QoS set
Audio Stream 202F0650 disabled
Fail to stop stream (err -77)
Audio Stream 202F0650 QoS set
Audio Stream 202F0688 stopped with reason 0x13
Audio Stream 202F0650 stopped with reason 0x13
Audio Stream 202F0688 enabled
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Audio Stream 202F0650 enabled
Audio Stream 202F0650 started
Audio Stream 202F0688 started
Return code 0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Or, enter "call_accept 0 \<call_index\>" on the call_terminal side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Audio Stream 202F0688 disabled
Audio Stream 202F0688 QoS set
Audio Stream 202F0650 disabled
Fail to stop stream (err -77)
Audio Stream 202F0650 QoS set
Audio Stream 202F0688 stopped with reason 0x13
Audio Stream 202F0650 stopped with reason 0x13
Audio Stream 202F0688 enabled
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Audio Stream 202F0650 enabled
Audio Stream 202F0650 started
Audio Stream 202F0688 started
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
7.2 Accept the call by remote device. enter "remote_call_answer \<call_index\>" on the call_gateway side
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Remove answer the call: call index 1
Audio Stream 202F0688 disabled
Audio Stream 202F0688 QoS set
Audio Stream 202F0650 disabled
Fail to stop stream (err -77)
Audio Stream 202F0650 QoS set
Audio Stream 202F0688 stopped with reason 0x13
Audio Stream 202F0650 stopped with reason 0x13
Audio Stream 202F0688 enabled
Init Audio SAI and CODEC, samplingRate :16000  bitWidth:16
Set default headphone volume 70
Audio Stream 202F0650 enabled
Audio Stream 202F0650 started
Audio Stream 202F0688 started
Return code 0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Supported Boards
- [MIMXRT1170-EVKB](../../_boards/evkbmimxrt1170/edgefast_bluetooth_examples/call_gateway/example_board_readme.md)
- [MIMXRT1060-EVKC](../../_boards/evkcmimxrt1060/edgefast_bluetooth_examples/call_gateway/example_board_readme.md)
- [MIMXRT700-EVK](../../_boards/mimxrt700evk/edgefast_bluetooth_examples/call_gateway/example_board_readme.md)
