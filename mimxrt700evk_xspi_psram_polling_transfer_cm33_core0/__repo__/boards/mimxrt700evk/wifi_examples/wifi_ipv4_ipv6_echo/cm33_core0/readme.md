# wifi_ipv4_ipv6_echo

## Overview
The wifi_ipv4_ipv6_echo application demonstrates a TCP and UDP echo on the lwIP TCP/IP stack with FreeRTOS.
The demo can use both TCP or UDP protocol over IPv4 or IPv6 and acts as an echo server. The application sends back
the packets received from the PC, which can be used to test whether a TCP or UDP connection is available.

Before building the example application select Wi-Fi module:
- build from repository - select Wi-Fi module in Kconfig GUI
- build from zip package - change project macro related to Wi-Fi module `WIFI_<SoC Name>_BOARD_<Module Name>`, macro can be found in components\wifi_bt_module\incl\wifi_bt_module_config.h

For more information about Wi-Fi module connection see:
- readme_modules.md (select supported board bellow)
- [Getting started guide](https://www.nxp.com/document/guide/getting-started-with-nxp-wi-fi-modules-using-i-mx-rt-platform:GS-WIFI-MODULES-IMXRT-PLATFORM)

## A few notes about IPv6
The demo generates a link-local address (the one from range FE80::/10) after the start. To send something to this (demo) address
from your computer you need to specify the interface over which the demo is reachable by appending % followed by zone index.
- On Windows, the zone index is a number. You can get it from the output of the ipconfig command.
- On Linux, the zone index is an interface name.
To connect to board with address FE80::12:13FF:FE10:1511
- over interface 21 on your Windows machine specify address as FE80::12:13FF:FE10:1511%21
- over interface eth on your Linux or Mac machine specify address as FE80::12:13FF:FE10:1511%eth0
But the demo has only a single interface, so do not append zone ID to any address typed to the demo terminal.

The LwIP stack is trying to get an IPv6 address automatically by neighbor discovery in the background.
This takes some time. You can print all addresses using the command print_ip_cfg any time.

## Tools
It is necessary to have installed tools capable of sending and receiving data over TCP or UDP to interact with the demo.
- ncat - Recommended tool. Supports both IPv4 and IPv6. It is part of nmap tools. It can be found at https://nmap.org/download.html.
- nc (netcat) - Basically, the same as ncat, but a lot of antiviruses consider this a virus.
- echotool - Supports only IPv4 and only for Windows. It can be obtained from https://github.com/PavelBansky/EchoTool

## Prepare the Demo
    
1.  Connect a micro USB cable between the PC host and the CMSIS DAP USB port on the board.
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Connect the Wi-Fi module. Refer to readme_modules.md and the [Supported boards](#supported-boards) section.
4.  Download the program to the target board.
5.  Either press the reset button on your board or launch the debugger in your IDE to begin running the demo.

## Running the demo

1.  When the demo starts, a welcome message would appear on the terminal, press enter for the command prompt:
    Press tab or type help to list out all available CLI commands.
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ========================================
    Initialize WLAN
    ========================================
    MAC Address: 9C:50:D1:45:4D:87
    Initialize CLI
    ========================================

    Copyright  2022  NXP

    SHELL>> help

    "help": List all the registered commands

    "exit": Exit program

    "echo_tcp_client ip_addr port":
    Connects to specified server and sends back every received data.
    Usage:
    ip_addr:     IPv6 or IPv4 server address
    port:        TCP port number

    "echo_tcp_server port":
    Listens for one incoming connection and sends back every received data.
    Usage:
    port:        TCP port number

    "echo_udp port":
    Waits for datagrams and sends them back.
    Usage:
    port:        UDP port number

    "end": Ends echo_* command.

    "print_ip_cfg": Prints IP configuration.

    "wlan_scan": Scans networks.

    "wlan_connect ssid":
    Connects to the specified network without password.
    Usage:
    ssid:        network SSID

    "wlan_connect_with_password ssid password":
    Connects to the specified network with password.
    Usage:
    ssid:        network SSID
    password:    password

    "wlan_disconnect":
    Disconnect from connected network
    SHELL>>
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

2.  You can list all available networks using wlan_scan command.
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    SHELL>> wlan_scan

    Initiating scan...
    MyWifi
        BSSID         : 00:5F:67:8B:25:8E
        RSSI          : -66dBm
        Channel       : 1
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

3.  Connect to the network using one of the following commands:
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    wlan_connect <ssid>
    wlan_connect_with_password <ssid> <password>

    Use SSID (the name of your network) to join the network.

    Example of successful wlan_connect_with_password command:	

    SHELL>> wlan_connect_with_password MyWifi pass0123456
    Joining: MyWifi
    Network joined
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    Note: If you see your network in the list of scanned networks but demo 
    prints "Failed to join network!", make sure you entered the correct SSID and password.
    If that did not help, try to restart your Wi-Fi access point.

4.  TCP client echo

    1. Launch `ncat -v -l -p 10001` on your computer.
    2. Run the command `echo_tcp_client <PC IPv4 addr> 10001` in demo shell.
    3. You should see on your PC `Ncat: Connection from <Demo IPv4 addr>.`
    4. Type some text into ncat (not demo shell) and the demo will send your line back
       when you hit enter.
    5. You can terminate the connection by pressing ctrl+c in ncat or typing end to the demo shell.
    
    For IPv6 just replace `<PC IPv4 addr>` with IPv6 address of your PC.
    
5.  TCP server echo

    1. Run the command `echo_tcp_server 10001` in demo shell.
    2. Launch `ncat -v <Demo IPv4 addr> 10001` on your computer.
    3. You should see on your PC `Ncat: Connected to <Demo IPv4 addr>:10001`
    4. Type some text into ncat (not demo shell) and the demo will send your line back
       when you hit enter.
    5. You can terminate the connection by pressing ctrl+c in ncat or typing end to the demo shell.
    
    For IPv6, just replace `<Demo IPv4 addr>` with the IPv6 address of the demo. In case of
    link-local IPv6 address, don't forget to append % followed by zone id of your PC.
    
6.  UDP echo

    1. Run the command `echo_udp 10001` in demo shell.
    2. On your computer launch `ncat -v -u <Demo IPv4 addr> 10001`
    3. You should see on your PC `Ncat: Connected to <Demo IPv4 addr>:10001`
    4. Type some text into ncat (not the demo shell) and the demo will send your line back
       when you hit enter.
    5. To terminate is necessary by doing both, pressing ctrl+c in ncat and typing end to demo shell.
    
    For IPv6, just replace `<Demo IPv4 addr>` with the IPv6 address of the demo. In case of
    link-local IPv6 address, don't forget to append % followed by zone id of your PC.

## Supported Boards
- [EVKB-IMXRT1050](../../_boards/evkbimxrt1050/wifi_examples/common/wifi_examples_readme.md)
- [MIMXRT1060-EVKB](../../_boards/evkbmimxrt1060/wifi_examples/common/wifi_examples_readme.md)
- [MIMXRT1170-EVKB](../../_boards/evkbmimxrt1170/wifi_examples/common/wifi_examples_readme.md)
- [MIMXRT1060-EVKC](../../_boards/evkcmimxrt1060/wifi_examples/common/wifi_examples_readme.md)
- [MIMXRT1040-EVK](../../_boards/evkmimxrt1040/wifi_examples/common/wifi_examples_readme.md)
- [EVK-MIMXRT1064](../../_boards/evkmimxrt1064/wifi_examples/common/wifi_examples_readme.md)
- [MIMXRT1160-EVK](../../_boards/evkmimxrt1160/wifi_examples/common/wifi_examples_readme.md)
- [MIMXRT1180-EVK](../../_boards/evkmimxrt1180/wifi_examples/common/wifi_examples_readme.md)
- [EVK-MIMXRT595](../../_boards/evkmimxrt595/wifi_examples/common/wifi_examples_readme.md)
- [EVK-MIMXRT685](../../_boards/evkmimxrt685/wifi_examples/common/wifi_examples_readme.md)
- [FRDM-RW612](../../_boards/frdmrw612/wifi_examples/common/wifi_examples_readme.md)
- [MCX-N5XX-EVK](../../_boards/mcxn5xxevk/wifi_examples/common/wifi_examples_readme.md)
- [MCX-N9XX-EVK](../../_boards/mcxn9xxevk/wifi_examples/common/wifi_examples_readme.md)
- [FRDM-MCXN947](../../_boards/frdmmcxn947/wifi_examples/common/wifi_examples_readme.md)
- [MIMXRT700-EVK](../../_boards/mimxrt700evk/wifi_examples/common/wifi_examples_readme.md)
- [RD-RW612-BGA](../../_boards/rdrw612bga/wifi_examples/common/wifi_examples_readme.md)
