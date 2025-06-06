/*
 * Copyright 2019, 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*  Standard C Included Files */
#include <string.h>
/*  SDK Included Files */
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_i3c.h"
#include "app.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define I3C_MASTER_SLAVE_ADDR_7BIT 0x1EU
#define I3C_DATA_LENGTH 34U

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#define I3C_VENDOR_ID 0x11BU

/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t g_slave_txBuff[I3C_DATA_LENGTH + 1] = {0};
uint8_t g_slave_rxBuff[I3C_DATA_LENGTH + 1] = {0};
volatile bool g_slaveCompletionFlag         = false;
uint8_t *g_txBuff                = NULL;
uint32_t g_txSize                = I3C_DATA_LENGTH;
volatile uint8_t g_deviceAddress = 0U;
uint8_t *g_deviceBuff            = NULL;
uint8_t g_deviceBuffSize         = I3C_DATA_LENGTH;
static i3c_slave_handle_t g_i3c_s_handle;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void i3c_slave_buildTxBuff(uint8_t *regAddr, uint8_t **txBuff, uint32_t *txBuffSize)
{
    /* If this is a combined frame and have device address information, send out the device buffer content and size.
     The received device address information is loaded in g_slave_rxBuff according to callback implementation of
     kI3C_SlaveReceiveEvent*/
    if ((regAddr != NULL) && (g_slave_rxBuff[0] == g_deviceAddress))
    {
        *txBuff     = g_deviceBuff;
        *txBuffSize = g_deviceBuffSize;
    }
    else
    {
        /* No valid register address information received, send default tx buffer. */
        *txBuff     = g_txBuff;
        *txBuffSize = g_txSize;
    }
}

static void i3c_slave_callback(I3C_Type *base, i3c_slave_transfer_t *xfer, void *userData)
{
    switch ((uint32_t)xfer->event)
    {
        /*  Transmit request */
        case kI3C_SlaveTransmitEvent:
            /*  Update information for transmit process */
            i3c_slave_buildTxBuff(xfer->rxData, &xfer->txData, (uint32_t *)&xfer->txDataSize);
            break;

        /*  Receive request */
        case kI3C_SlaveReceiveEvent:
            /*  Update information for received process */
            xfer->rxData     = g_slave_rxBuff;
            xfer->rxDataSize = I3C_DATA_LENGTH;
            break;

        /*  Transmit request */
        case (kI3C_SlaveTransmitEvent | kI3C_SlaveHDRCommandMatchEvent):
            /*  Update information for transmit process */
            xfer->txData     = g_slave_txBuff;
            xfer->txDataSize = I3C_DATA_LENGTH;
            break;

        /*  Receive request */
        case (kI3C_SlaveReceiveEvent | kI3C_SlaveHDRCommandMatchEvent):
            /*  Update information for received process */
            xfer->rxData     = g_slave_rxBuff;
            xfer->rxDataSize = I3C_DATA_LENGTH;
            break;

        /*  Transfer done */
        case kI3C_SlaveCompletionEvent:
            if (xfer->completionStatus == kStatus_Success)
            {
                g_slaveCompletionFlag = true;
            }
            break;

#if defined(I3C_ASYNC_WAKE_UP_INTR_CLEAR)
        /*  Handle async wake up interrupt on specific platform. */
        case kI3C_SlaveAddressMatchEvent:
            I3C_ASYNC_WAKE_UP_INTR_CLEAR
            break;
#endif

        default:
            break;
    }
}

/*!
 * @brief Main function
 */
int main(void)
{
    uint32_t eventMask = kI3C_SlaveCompletionEvent;
#if defined(I3C_ASYNC_WAKE_UP_INTR_CLEAR)
    eventMask |= kI3C_SlaveAddressMatchEvent;
#endif
    i3c_slave_config_t slaveConfig;

    BOARD_InitHardware();

    PRINTF("\r\nI3C board2board polling example -- Slave transfer.\r\n\r\n");

    I3C_SlaveGetDefaultConfig(&slaveConfig);
    slaveConfig.staticAddr = I3C_MASTER_SLAVE_ADDR_7BIT;
    slaveConfig.vendorID   = I3C_VENDOR_ID;
    slaveConfig.offline    = false;
    I3C_SlaveInit(EXAMPLE_SLAVE, &slaveConfig, I3C_SLAVE_CLOCK_FREQUENCY);
    I3C_SlaveTransferCreateHandle(EXAMPLE_SLAVE, &g_i3c_s_handle, i3c_slave_callback, NULL);

    g_txBuff = g_slave_txBuff;
    /* Start slave non-blocking transfer. */
    I3C_SlaveTransferNonBlocking(EXAMPLE_SLAVE, &g_i3c_s_handle, eventMask);

    PRINTF("Check I3C master I2C transfer.\r\n");

    /* For I2C transfer check, master board will always send one byte subaddress(device address). The first transfer is
    a I2C write transfer, master will send one byte device address + one byte transmit size + several bytes transmit
    buffer content. */
    memset(g_slave_rxBuff, 0, sizeof(g_slave_rxBuff));
    /* Wait for master transmit completed. */
    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

    memcpy(g_slave_txBuff, g_slave_rxBuff, I3C_DATA_LENGTH);
    /* Preapre slave tx buffer, the first byte is received device address, second byte is transmit size, following bytes
    are transmit buffer content. Save the received device address, prepare device buffer and buffer size to echo back
    the transmit buffer content to the master. */
    g_deviceAddress  = g_slave_txBuff[0];
    g_deviceBuff     = &g_slave_txBuff[2];
    g_deviceBuffSize = g_slave_txBuff[1];

    PRINTF("Slave received data :");
    for (uint32_t i = 0U; i < I3C_DATA_LENGTH; i++)
    {
        if (i % 8U == 0U)
        {
            PRINTF("\r\n");
        }
        PRINTF("0x%2x  ", g_slave_rxBuff[i]);
    }

    /* The second transfer is a I2C read transfer, master will send one byte device address, then send repeated start
     * and read back the transmit buffer to that device address. */
    /* Wait for slave transmit completed. */
    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

    PRINTF("\r\nI3C master I2C transfer finished.\r\n");

    PRINTF("\r\nCheck I3C master I3C SDR transfer.\r\n");

    /* For I3C SDR transfer check, master board will not send subaddress(device address). The first transfer is a
    I3C SDR write transfer, master will send one byte transmit size + several bytes transmit buffer content. */
    /* Wait for master transmit completed. */
    memset(g_slave_rxBuff, 0, sizeof(g_slave_rxBuff));
    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

    /* Update slave tx buffer according to the received buffer, the first byte is transmit data size, the following
     * bytes are transmit buffer content. */
    memcpy(g_slave_txBuff, g_slave_rxBuff, I3C_DATA_LENGTH);
    g_txBuff = &g_slave_txBuff[1];
    g_txSize = g_slave_txBuff[0];

    PRINTF("Slave received data :");
    for (uint32_t i = 0U; i < g_slave_rxBuff[0]; i++)
    {
        if (i % 8U == 0U)
        {
            PRINTF("\r\n");
        }
        PRINTF("0x%2x  ", g_slave_rxBuff[i + 1]);
    }

    /* The second transfer is a I3C SDR read transfer, master will read back the transmit buffer content just sent. */
    /* Wait for slave transmit completed. */
    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

    PRINTF("\r\nI3C master I3C SDR transfer finished.\r\n");

#if defined(EXAMPLE_I3C_HDR_SUPPORT) && (EXAMPLE_I3C_HDR_SUPPORT)
    PRINTF("\r\nCheck I3C master I3C HDR transfer.\r\n");

    memset(g_slave_rxBuff, 0, sizeof(g_slave_rxBuff));
    /* Wait for master HDR-DDR transmit completed. */
    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

    /* Update slave tx buffer according to the received buffer, the first byte is transmit data size, the following
     * bytes are transmit buffer content. */
    memcpy(g_slave_txBuff, g_slave_rxBuff, I3C_DATA_LENGTH);
    g_txBuff = &g_slave_txBuff[2];
    g_txSize = g_slave_txBuff[1];

    PRINTF("Slave received data :");
    for (uint32_t i = 0U; i < g_slave_rxBuff[1]; i++)
    {
        if (i % 8U == 0U)
        {
            PRINTF("\r\n");
        }
        PRINTF("0x%2x  ", g_slave_rxBuff[i + 2]);
    }

    /* The second transfer is a I3C SDR read transfer, master will read back the transmit buffer content just sent. */
    /* Wait for slave transmit completed. */
    while (!g_slaveCompletionFlag)
    {
    }
    g_slaveCompletionFlag = false;

    PRINTF("\r\nI3C master I3C HDR transfer finished.\r\n");
#endif

    while (1)
    {
    }
}
