/*
 * Copyright (c) 2014-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 * Copyright 2019 ACRIOS Systems s.r.o.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ERPC_TRANSPORT_SETUP_H_
#define _ERPC_TRANSPORT_SETUP_H_

/*!
 * @addtogroup transport_setup
 * @{
 * @file
 */

////////////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////////////

//! @brief Opaque transport object type.
typedef struct ErpcTransport *erpc_transport_t;
//! @brief Ready callback object type for RPMsg-Lite transport.
typedef void (*rpmsg_ready_cb)(void);

////////////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

//! @name Transport setup
//@{

//! @name DSPI transport setup
//@{

/*!
 * @brief Create a DSPI master transport.
 *
 * Create DSPI master transport instance, to be used at master core.
 *
 * @param[in] baseAddr Base address of DSPI peripheral used in this transport layer.
 * @param[in] baudRate DSPI baud rate.
 * @param[in] srcClock_Hz DSPI source clock in Hz.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_dspi_master_init(void *baseAddr, uint32_t baudRate, uint32_t srcClock_Hz);

/*!
 * @brief Deinitialize DSPI master transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_dspi_master_deinit(erpc_transport_t transport);

/*!
 * @brief Create a DSPI slave transport.
 *
 * Create DSPI slave transport instance, to be used at slave core.
 *
 * @param[in] baseAddr Base address of DSPI peripheral used in this transport layer.
 * @param[in] baudRate DSPI baud rate.
 * @param[in] srcClock_Hz DSPI source clock in Hz.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_dspi_slave_init(void *baseAddr, uint32_t baudRate, uint32_t srcClock_Hz);

/*!
 * @brief Deinitialize DSPI slave transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_dspi_slave_deinit(erpc_transport_t transport);

//@}

//! @name I2C transport setup
//@{

/*!
 * @brief Create an I2C slave transport.
 *
 * Create I2C slave transport instance, to be used at slave core.
 *
 * @param[in] baseAddr Base address of I2C peripheral used in this transport layer.
 * @param[in] baudRate SPI baud rate.
 * @param[in] srcClock_Hz I2C source clock in Hz.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_i2c_slave_init(void *baseAddr, uint32_t baudRate, uint32_t srcClock_Hz);

/*!
 * @brief Deinitialize I2C slave transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_i2c_slave_deinit(erpc_transport_t transport);

//@}

//! @name LPI2C transport setup
//@{

/*!
 * @brief Create an LPI2C slave transport.
 *
 * Create LPI2C slave transport instance, to be used at slave core.
 *
 * @param[in] baseAddr Base address of LPI2C peripheral used in this transport layer.
 * @param[in] baudRate SPI baud rate.
 * @param[in] srcClock_Hz LPI2C source clock in Hz.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_lpi2c_slave_init(void *baseAddr, uint32_t baudRate, uint32_t srcClock_Hz);

/*!
 * @brief Deinitialize LPI2C slave transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_lpi2c_slave_deinit(erpc_transport_t transport);

//@}

//! @name LPSPI transport setup
//@{

/*!
 * @brief Create a LPSPI slave transport.
 *
 * Create LPSPI slave transport instance, to be used at slave core.
 *
 * @param[in] baseAddr Base address of LPSPI peripheral used in this transport layer.
 * @param[in] baudRate LPSPI baud rate.
 * @param[in] srcClock_Hz LPSPI source clock in Hz.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_lpspi_slave_init(void *baseAddr, uint32_t baudRate, uint32_t srcClock_Hz);

/*!
 * @brief Deinitialize LPSPI slave transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_lpspi_slave_deinit(erpc_transport_t transport);

//@}

//! @name MU transport setup
//@{

/*!
 * @brief Create an MU transport.
 *
 * Create Messaging Unit (MU) transport instance, to be used on both the server
 * and the client side. Base address of the MU peripheral needs to be passed.
 *
 * @param[in] baseAddr Base address of MU peripheral.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_mu_init(void *baseAddr);

/*!
 * @brief Deinitialize MU transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_mu_deinit(erpc_transport_t transport);

//@}

//! @name Linux RPMSG endpoint setup
//@{

/*!
 * @brief Create an Linux RPMSG endpoint transport.
 *
 * This function is using RPMSG endpoints based on this implementation:
 * github.com/nxp-mcuxpresso/rpmsg-sysfs/tree/0aa1817545a765c200b1b2f9b6680a420dcf9171 .
 *
 * When local/remote address is set to '-1', then default addresses will be used.
 * When type is set to '0', then Datagram model will be used, else Stream.
 *
 * @param[in] local_addr Local endpoint address.
 * @param[in] type Datagram (0) or Stream (1).
 * @param[in] remote_addr Remote endpoint address.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_rpmsg_linux_init(int16_t local_addr, int8_t type, int16_t remote_addr);

/*!
 * @brief Deinitialize an Linux RPMSG endpoint transport.
 *
 * This function deinitialize the Linux RPMSG endpoint transport.
 *
 * @param[in] transport Transport which was returned from init function.
 */
void erpc_transport_rpmsg_linux_deinit(erpc_transport_t transport);

//@}

//! @name RPMsg-Lite transport setup
//@{

/*!
 * @brief Create an RPMsg-Lite transport.
 *
 * Create RPMsg-Lite baremetal transport instance, to be used at master core.
 *
 * @param[in] src_addr Address of local RPMsg endpoint used for communication.
 * @param[in] dst_addr Address of remote RPMsg endpoint used for communication.
 * @param[in] rpmsg_link_id Link ID used to define the rpmsg-lite instance, see
 *                          rpmsg_platform.h
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_rpmsg_lite_master_init(uint32_t src_addr, uint32_t dst_addr, uint32_t rpmsg_link_id);

/*!
 * @brief Deinitialize RPMsg-Lite transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_rpmsg_lite_master_deinit(erpc_transport_t transport);

/*!
 * @brief Create an RPMsg-Lite transport.
 *
 * Create RPMsg-Lite baremetal transport instance, to be used at slave/remote
 * core.
 *
 * @param[in] src_addr Address of local RPMsg endpoint used for communication.
 * @param[in] dst_addr Address of remote RPMsg endpoint used for communication.
 * @param[in] start_address Shared memory base address used for this instance of
 *                          RPMsg-Lite.
 * @param[in] rpmsg_link_id Link ID used to define the rpmsg-lite instance, see
 *                          rpmsg_platform.h.
 * @param[in] ready Callback function, which gets called, when RPMsg is
 *                  initialized and master core can be notified.
 * @param[in] nameservice_name Name of the nameservice channel to be announced
 *                             to the other core.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_rpmsg_lite_remote_init(uint32_t src_addr, uint32_t dst_addr, void *start_address,
                                                       uint32_t rpmsg_link_id, rpmsg_ready_cb ready,
                                                       char *nameservice_name);

/*!
 * @brief Deinitialize RPMsg-Lite transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_rpmsg_lite_remote_deinit(erpc_transport_t transport);

/*!
 * @brief Create an RPMsg-Lite RTOS transport.
 *
 * Create RPMsg-Lite RTOS transport instance, to be used at master core.
 *
 * @param[in] src_addr Address of local RPMsg endpoint used for communication.
 * @param[in] dst_addr Address of remote RPMsg endpoint used for communication.
 * @param[in] rpmsg_link_id Link ID used to define the rpmsg-lite instance, see
 *                          rpmsg_platform.h
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_rpmsg_lite_rtos_master_init(uint32_t src_addr, uint32_t dst_addr,
                                                            uint32_t rpmsg_link_id);

/*!
 * @brief Deinitialize RPMsg-Lite RTOS transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_rpmsg_lite_rtos_master_deinit(erpc_transport_t transport);

/*!
 * @brief Create an RPMsg-Lite RTOS transport.
 *
 * Create RPMsg-Lite RTOS transport instance, to be used at slave/remote core.
 *
 * @param[in] src_addr Address of local RPMsg endpoint used for communication.
 * @param[in] dst_addr Address of remote RPMsg endpoint used for communication.
 * @param[in] start_address Shared memory base address used for this instance of
 *                          RPMsg-Lite.
 * @param[in] rpmsg_link_id Link ID used to define the rpmsg-lite instance, see
 *                          rpmsg_platform.h.
 * @param[in] ready Callback function, which gets called, when RPMsg is
 *                  initialized and master core can be notified.
 * @param[in] nameservice_name Name of the nameservice channel to be announced
 *                             to the other core.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_rpmsg_lite_rtos_remote_init(uint32_t src_addr, uint32_t dst_addr, void *start_address,
                                                            uint32_t rpmsg_link_id, rpmsg_ready_cb ready,
                                                            char *nameservice_name);

/*!
 * @brief Deinitialize RPMsg-Lite RTOS transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_rpmsg_lite_rtos_remote_deinit(erpc_transport_t transport);

/*!
 * @brief Create an RPMsg-Lite TTY transport.
 *
 * Create RPMsg-Lite TTY transport instance, to be used at slave/remote core.
 * This function is mainly used with Linux running on the master core.
 *
 * @param[in] src_addr Address of local RPMsg endpoint used for communication.
 * @param[in] dst_addr Address of remote RPMsg endpoint used for communication.
 * @param[in] start_address Shared memory base address used for this instance of
 *                          RPMsg-Lite.
 * @param[in] rpmsg_link_id Link ID used to define the rpmsg-lite instance, see
 *                          rpmsg_platform.h.
 * @param[in] ready Callback function, which gets called, when RPMsg is
 *                  initialized and master core can be notified.
 * @param[in] nameservice_name Name of the nameservice channel to be announced
 *                             to the other core.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_rpmsg_lite_tty_rtos_remote_init(uint32_t src_addr, uint32_t dst_addr,
                                                                void *start_address, uint32_t rpmsg_link_id,
                                                                rpmsg_ready_cb ready, char *nameservice_name);

/*!
 * @brief Deinitialize an RPMSG lite tty rtos transport.
 *
 * This function deinitialize the RPMSG lite tty rtos transport.
 *
 * @param[in] transport Transport which was returned from init function.
 */
void erpc_transport_rpmsg_lite_tty_rtos_remote_deinit(erpc_transport_t transport);
//@}

//! @name Host PC serial port transport setup
//@{

/*!
 * @brief Create a host PC serial port transport.
 *
 * Create a host PC serial port transport instance.
 *
 * @param[in] portName Port name.
 * @param[in] baudRate Baud rate.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_serial_init(const char *portName, long baudRate);

/*!
 * @brief Deinitialize a host PC serial port transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_serial_deinit(erpc_transport_t transport);

//@}

//! @name SPI transport setup
//@{

/*!
 * @brief Create a SPI master transport.
 *
 * Create SPI master transport instance, to be used at master core.
 *
 * @param[in] baseAddr Base address of SPI peripheral used in this transport layer.
 * @param[in] baudRate SPI baud rate.
 * @param[in] srcClock_Hz SPI source clock in Hz.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_spi_master_init(void *baseAddr, uint32_t baudRate, uint32_t srcClock_Hz);

/*!
 * @brief Deinitialize SPI master transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_spi_master_deinit(erpc_transport_t transport);

/*!
 * @brief Create a SPI slave transport.
 *
 * Create SPI slave transport instance, to be used at slave core.
 *
 * @param[in] baseAddr Base address of SPI peripheral used in this transport layer.
 * @param[in] baudRate SPI baud rate.
 * @param[in] srcClock_Hz SPI source clock in Hz.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_spi_slave_init(void *baseAddr, uint32_t baudRate, uint32_t srcClock_Hz);

/*!
 * @brief Deinitialize SPI slave transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_spi_slave_deinit(erpc_transport_t transport);

//@}

//! @name SPIdev transport setup
//@{

/*!
 * @brief Create a SPIdev transport.
 *
 * Create SPIdev master transport instance, to be used at master core.
 *
 * @param[in] spidev SPI device name.
 * @param[in] speed_Hz SPI clock speed in Hz.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_spidev_master_init(const char *spidev, uint32_t speed_Hz);

/*!
 * @brief Deinitialize SPIdev transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_spidev_master_deinit(erpc_transport_t transport);

//@}

//! @name TCP transport setup
//@{

/*!
 * @brief Create and open TCP transport
 *
 * For server, create a TCP listen socket and wait for connections
 * For client, connect to server
 *
 * @param[in] host hostname/IP address to listen on or server to connect to
 * @param[in] port port to listen on or server to connect to
 * @param[in] isServer true if we are a server
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_tcp_init(const char *host, uint16_t port, bool isServer);

/*!
 * @brief Close TCP connection
 *
 * For server, stop listening and close all sockets. Note that the server mode
 * uses and accept() which is a not-recommended blocking method so we can't exit
 * until a connection attempts is made. This is a deadlock but assuming that TCP
 * code is supposed to be for test, I assume it's acceptable. Otherwise a non-blocking
 * socket or select() shoudl be used
 * For client, close server connection
 *
 * @param[in] transport Transport which was returned from init function.
 */
void erpc_transport_tcp_close(erpc_transport_t transport);

/*!
 * @brief Deinitialize TCP transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_tcp_deinit(erpc_transport_t transport);

//@}

//! @name CMSIS UART transport setup
//@{

/*!
 * @brief Create a CMSIS UART transport.
 *
 * Create a CMSIS UART transport instance, to be used on both the server
 * and the client side.
 *
 * @param[in] uartDrv CMSIS USART driver structure address (Driver Control Block).
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_cmsis_uart_init(void *uartDrv);

/*!
 * @brief Deinitialize CMSIS UART transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_cmsis_uart_deinit(erpc_transport_t transport);

//@}

//! @name Zephyr transports setup
//@{

/*!
 * @brief Create a Zephyr UART transport.
 *
 * Create a Zephyr UART transport instance, to be used on both the server
 * and the client side.
 *
 * @param[in] dev Zephyr UART device address.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_zephyr_uart_init(void *dev);

/*!
 * @brief Deinitialize Zephyr UART transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_zephyr_uart_deinit(erpc_transport_t transport);

/*!
 * @brief Create a Zephyr MBOX transport.
 *
 * Create a Zephyr MBOX transport instance, to be used on both the server
 * and the client side.
 *
 * @param[in] dev Zephyr MBOX device address.
 * @param[in] tx_channel Zephyr MBOX transmit channel.
 * @param[in] rx_channel Zephyr MBOX receive channel.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_zephyr_mbox_init(void *dev, void *tx_channel, void *rx_channel);

/*!
 * @brief Deinitialize Zephyr MBOX transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_zephyr_mbox_deinit(erpc_transport_t transport);

//@}

//! @name USB CDC transport setup
//@{

/*!
 * @brief Create an USB CDC transport.
 *
 * Create an USB CDC transport instance.
 *
 * @param[in] serialHandle Pointer to point to a memory space of size #SERIAL_MANAGER_HANDLE_SIZE allocated by the
 * caller, see serial manager header file.
 * @param[in] serialConfig Pointer to user-defined configuration structure allocated by the caller, see serial manager
 * header file.
 * @param[in] usbCdcConfig Pointer to serial port usb config structure allocated by the caller, see serial manager
 * header file.
 * @param[in] usbRingBuffer Pointer to point serial manager ring buffer allocated by the caller, see serial manager
 * header file.
 * @param[in] usbRingBufferLength Serial manager ring buffer size.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_transport_usb_cdc_init(void *serialHandle, void *serialConfig, void *usbCdcConfig,
                                             uint8_t *usbRingBuffer, uint32_t usbRingBufferLength);

/*!
 * @brief Deinitialize USB CDC transport.
 *
 * @param[in] transport Transport which was initialized with init function.
 */
void erpc_transport_usb_cdc_deinit(erpc_transport_t transport);

//@}

//@}

#ifdef __cplusplus
}
#endif

/*! @} */

#endif // _ERPC_TRANSPORT_SETUP_H_
