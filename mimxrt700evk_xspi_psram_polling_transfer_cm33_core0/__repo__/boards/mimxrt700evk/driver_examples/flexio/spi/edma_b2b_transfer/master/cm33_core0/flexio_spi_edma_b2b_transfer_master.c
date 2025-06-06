/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_flexio_spi_edma.h"
#include "board.h"
#include "app.h"
#include "fsl_debug_console.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define BUFFER_SIZE (64)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

static flexio_spi_master_edma_handle_t g_spiHandle;
static edma_handle_t txHandle;
static edma_handle_t rxHandle;
FLEXIO_SPI_Type spiDev;
AT_NONCACHEABLE_SECTION_INIT(static uint8_t sendBuff[BUFFER_SIZE]) = {0U};
AT_NONCACHEABLE_SECTION_INIT(static uint8_t recvBuff[BUFFER_SIZE]) = {0U};
volatile bool completeFlag                                         = false;

/*******************************************************************************
 * Code
 ******************************************************************************/

static void spi_master_completionCallback(FLEXIO_SPI_Type *base,
                                          flexio_spi_master_edma_handle_t *handle,
                                          status_t status,
                                          void *userData)
{
    if (status == kStatus_Success)
    {
        completeFlag = true;
    }
}

int main(void)
{
    uint8_t i                  = 0;
    uint8_t err                = 0;
    flexio_spi_transfer_t xfer = {0};
    flexio_spi_master_config_t userConfig;
    dma_request_source_t dma_request_source_tx;
    dma_request_source_t dma_request_source_rx;
    edma_config_t config;

    BOARD_InitHardware();
    PRINTF("\r\nFlexIO SPI edma example\r\n");
    PRINTF("Master Start...\r\n");

    /* Init FlexIO SPI. */
    /*
     * userConfig.enableMaster = true;
     * userConfig.enableInDoze = false;
     * userConfig.enableInDebug = true;
     * userConfig.enableFastAccess = false;
     * userConfig.baudRate_Bps = 500000U;
     * userConfig.phase = kFLEXIO_SPI_ClockPhaseFirstEdge;
     * userConfig.dataMode = kFLEXIO_SPI_8BitMode;
     */
    FLEXIO_SPI_MasterGetDefaultConfig(&userConfig);
    userConfig.baudRate_Bps = 4000000U;

    spiDev.flexioBase      = BOARD_FLEXIO_BASE;
    spiDev.SDOPinIndex     = FLEXIO_SPI_MOSI_PIN;
    spiDev.SDIPinIndex     = FLEXIO_SPI_MISO_PIN;
    spiDev.SCKPinIndex     = FLEXIO_SPI_SCK_PIN;
    spiDev.CSnPinIndex     = FLEXIO_SPI_CSn_PIN;
    spiDev.shifterIndex[0] = FLEXIO_TX_SHIFTER_INDEX;
    spiDev.shifterIndex[1] = FLEXIO_RX_SHIFTER_INDEX;
    spiDev.timerIndex[0]   = 0U;
    spiDev.timerIndex[1]   = 1U;

    dma_request_source_tx = (dma_request_source_t)EXAMPLE_TX_DMA_SOURCE;
    dma_request_source_rx = (dma_request_source_t)EXAMPLE_RX_DMA_SOURCE;

#if defined(FSL_FEATURE_SOC_DMAMUX_COUNT) && FSL_FEATURE_SOC_DMAMUX_COUNT
    /*Init EDMA for example.*/
    DMAMUX_Init(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR);
    /* Request DMA channels for TX & RX. */
    DMAMUX_SetSource(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR, FLEXIO_SPI_TX_DMA_CHANNEL, dma_request_source_tx);
    DMAMUX_SetSource(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR, FLEXIO_SPI_RX_DMA_CHANNEL, dma_request_source_rx);
    DMAMUX_EnableChannel(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR, FLEXIO_SPI_TX_DMA_CHANNEL);
    DMAMUX_EnableChannel(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR, FLEXIO_SPI_RX_DMA_CHANNEL);
#endif
    EDMA_GetDefaultConfig(&config);
    EDMA_Init(EXAMPLE_FLEXIO_SPI_DMA_BASEADDR, &config);
    EDMA_CreateHandle(&txHandle, EXAMPLE_FLEXIO_SPI_DMA_BASEADDR, FLEXIO_SPI_TX_DMA_CHANNEL);
    EDMA_CreateHandle(&rxHandle, EXAMPLE_FLEXIO_SPI_DMA_BASEADDR, FLEXIO_SPI_RX_DMA_CHANNEL);

#if defined(FSL_FEATURE_EDMA_HAS_CHANNEL_MUX) && FSL_FEATURE_EDMA_HAS_CHANNEL_MUX
    EDMA_SetChannelMux(EXAMPLE_FLEXIO_SPI_DMA_BASEADDR, FLEXIO_SPI_TX_DMA_CHANNEL, dma_request_source_tx);
    EDMA_SetChannelMux(EXAMPLE_FLEXIO_SPI_DMA_BASEADDR, FLEXIO_SPI_RX_DMA_CHANNEL, dma_request_source_rx);
#endif

    FLEXIO_SPI_MasterInit(&spiDev, &userConfig, FLEXIO_CLOCK_FREQUENCY);

    /* Init Buffer. */
    for (i = 0; i < BUFFER_SIZE; i++)
    {
        sendBuff[i] = i;
    }

    /* Send to slave. */
    xfer.txData   = sendBuff;
    xfer.rxData   = recvBuff;
    xfer.dataSize = BUFFER_SIZE;
    xfer.flags    = kFLEXIO_SPI_8bitMsb;
    FLEXIO_SPI_MasterTransferCreateHandleEDMA(&spiDev, &g_spiHandle, spi_master_completionCallback, NULL, &txHandle,
                                              &rxHandle);
    FLEXIO_SPI_MasterTransferEDMA(&spiDev, &g_spiHandle, &xfer);
    while (!completeFlag)
    {
    }
    completeFlag = false;

    /* Check if the data is right. */
    for (i = 0; i < BUFFER_SIZE; i++)
    {
        if (sendBuff[i] != recvBuff[i])
        {
            PRINTF("The %d is wrong! data is %d\r\n", i, recvBuff[i]);
            err++;
        }
    }
    if (err == 0)
    {
        PRINTF("\r\nMaster runs successfully!\r\n");
    }

    while (1)
    {
    }
}
