/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * FreeMASTER Communication Driver - User Configuration File
 */

#ifndef __FREEMASTER_CFG_H
#define __FREEMASTER_CFG_H

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

#define FMSTR_PLATFORM_CORTEX_M 1 /* Cortex-M platform (see freemaster.h for list of all supported platforms) */

//! Set the demo application configuration
#define FMSTR_DEMO_ENOUGH_ROM 1 /* Platform has enough ROM to show most of the FreeMASTER features */
#define FMSTR_DEMO_LARGE_ROM  1 /* Platform has large ROM enough to store the extended data structures */
#define FMSTR_DEMO_SUPPORT_I64 1 /* support for long long type */
#define FMSTR_DEMO_SUPPORT_FLT 1 /* support for float type */
#define FMSTR_DEMO_SUPPORT_DBL 1 /* support for double type */

//! Enable/Disable FreeMASTER support as a whole
#define FMSTR_DISABLE 0 // To disable all FreeMASTER functionalities

//! Select interrupt or poll-driven serial communication
#define FMSTR_LONG_INTR   0 // Complete message processing in interrupt
#define FMSTR_SHORT_INTR  0 // Queuing done in interrupt
#define FMSTR_POLL_DRIVEN 1 // No interrupt needed, polling only

// List of standard FreeMASTER transports and their low-level drivers. See more options in src/drivers.
// FMSTR_SERIAL   -   Standard serial transport protocol (Used by various types of UART peripherals as USB CDC
// implementation)
//    FMSTR_SERIAL_MCUX_UART   -   MCUXSDK driver for UART peripheral
//    FMSTR_SERIAL_MCUX_LPUART -   MCUXSDK driver for LPUART peripheral
//    FMSTR_SERIAL_MCUX_USART  -   MCUXSDK driver for USART peripheral
//    FMSTR_SERIAL_MCUX_MINIUSART -MCUXSDK driver for MINIUSART peripheral
//    FMSTR_SERIAL_MCUX_USB    -   MCUXSDK driver for USB peripheral with CDC class
// FMSTR_CAN      -   Standard CAN transport protocol (Used by various types of CAN peripherals)
//    FMSTR_CAN_MCUX_FLEXCAN   -   MCUXSDK driver for FlexCAN peripheral
//    FMSTR_CAN_MCUX_MCAN      -   MCUXSDK driver for MCAN peripheral
//    FMSTR_CAN_MCUX_MSCAN     -   MCUXSDK driver for msCAN peripheral
// FMSTR_PDBDM    -   Packet Driven BDM (direct memory access via JTAG, SWD or BDM debug probes). No low-level driver
// used. FMSTR_NET      -   Network communication using TCP or UDP protocol
//    FMSTR_NET_LWIP_TCP       -   TCP using lwIP stack
//    FMSTR_NET_LWIP_UDP       -   UDP using lwIP stack
//    FMSTR_NET_SEGGER_RTT     -   SEGGER RTT using J-Link debugger

//! Count of protocol sessions (minimum is 1)
#define FMSTR_SESSION_COUNT 1

//! Select communication interface
#define FMSTR_TRANSPORT FMSTR_NET            // Use network transport layer
#define FMSTR_NET_DRV   FMSTR_NET_SEGGER_RTT // Use network driver for SEGGER RTT

//! SEGGER RTT-specific communication options
// RTT Buffer Index
// =0 is default terminal buffer, this buffer is allocated always
// >0 will add additional buffer for transport, see freemaster_net_segger_rtt.c
#define FMSTR_NET_SEGGER_RTT_BUFFER_INDEX 0
// Put RTT control block variable to noncacheable memory section
#define SEGGER_RTT_SECTION                "NonCacheable"

//! Input/output communication buffer size
#define FMSTR_COMM_BUFFER_SIZE 508 // Set to 0 for "automatic"

//! Support for Application Commands
#define FMSTR_USE_APPCMD       1  // Enable/disable App.Commands support
#define FMSTR_APPCMD_BUFF_SIZE 32 // App.Command data buffer size
#define FMSTR_MAX_APPCMD_CALLS 4  // How many app.cmd callbacks? (0=disable)

//! Oscilloscope support
#define FMSTR_USE_SCOPE      2 // Specify number of supported oscilloscopes
#define FMSTR_MAX_SCOPE_VARS 8 // Specify maximum number of scope variables per one oscilloscope

//! Recorder support
#define FMSTR_USE_RECORDER 2 // Specify number of supported recorders

//! Built-in recorder buffer
#define FMSTR_REC_BUFF_SIZE 1024 // Built-in buffer size of recorder #0. Set to 0 to use runtime settings.

//! Recorder time base, specifies how often the recorder is called in the user app.
#define FMSTR_REC_TIMEBASE   FMSTR_REC_BASE_MILLISEC(0) // 0 = "unknown"
#define FMSTR_REC_FLOAT_TRIG 1                          // Enable/disable floating point triggering

// Target-side address translation (TSA)
#define FMSTR_USE_TSA         1 // Enable TSA functionality
#define FMSTR_USE_TSA_INROM   1 // TSA tables declared as const (put to ROM)
#define FMSTR_USE_TSA_SAFETY  1 // Enable/Disable TSA memory protection
#define FMSTR_USE_TSA_DYNAMIC 1 // Enable/Disable TSA entries to be added also in runtime

// Pipes as data streaming over FreeMASTER protocol
#define FMSTR_USE_PIPES 3 // Specify number of supported pipe objects

// Enable/Disable read/write memory commands
#define FMSTR_USE_READMEM      1 // Enable read memory commands
#define FMSTR_USE_WRITEMEM     1 // Enable write memory commands
#define FMSTR_USE_WRITEMEMMASK 1 // Enable write memory bits commands

// Define password for access levels to protect them. AVOID SHORT PASSWORDS in production version.
// Passwords should be at least 20 characters long to prevent dictionary attacks.
#if 0
#define FMSTR_RESTRICTED_ACCESS_R_PASSWORD    "r"   // Read-only access level password. 
#define FMSTR_RESTRICTED_ACCESS_RW_PASSWORD   "rw"  // Write access level password. 
#define FMSTR_RESTRICTED_ACCESS_RWF_PASSWORD  "rwf" // Flash access level password. 
#endif

// Storing cleartext passwords in Flash memory is not safe, consider storing their SHA1 hash instead
// Even with this option, the hash must be generated from reasonably complex password to prevent dictionary attack.
// When non-zero, the passwords above are specified as a pointer to 20-byte SHA1 hash of password text
#define FMSTR_USE_HASHED_PASSWORDS 0 

#endif /* __FREEMASTER_CFG_H */

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
