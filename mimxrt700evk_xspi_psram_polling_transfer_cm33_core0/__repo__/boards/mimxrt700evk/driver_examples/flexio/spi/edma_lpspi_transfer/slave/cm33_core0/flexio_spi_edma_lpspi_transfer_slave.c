/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "fsl_lpspi.h"
#include "fsl_flexio_spi_edma.h"
#include "board.h"
#include "app.h"
#if defined(FSL_FEATURE_SOC_DMAMUX_COUNT) && FSL_FEATURE_SOC_DMAMUX_COUNT
#include "fsl_dmamux.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/* LPSPI user callback */
void LPSPI_MasterUserCallback(LPSPI_Type *base, lpspi_master_handle_t *handle, status_t status, void *userData);
void FLEXIO_SPI_SlaveUserCallback(FLEXIO_SPI_Type *base,
                                  flexio_spi_slave_edma_handle_t *handle,
                                  status_t status,
                                  void *userData);

/*******************************************************************************
 * Variables
 ******************************************************************************/
AT_NONCACHEABLE_SECTION_INIT(uint8_t masterRxData[TRANSFER_SIZE]) = {0U};
AT_NONCACHEABLE_SECTION_INIT(uint8_t masterTxData[TRANSFER_SIZE]) = {0U};
AT_NONCACHEABLE_SECTION_INIT(uint8_t slaveRxData[TRANSFER_SIZE])  = {0U};
AT_NONCACHEABLE_SECTION_INIT(uint8_t slaveTxData[TRANSFER_SIZE])  = {0U};

lpspi_master_handle_t g_m_handle;
FLEXIO_SPI_Type spiDev;
flexio_spi_slave_edma_handle_t g_s_handle;

edma_handle_t txHandle;
edma_handle_t rxHandle;

volatile bool isSlaveTransferCompleted  = false;
volatile bool isMasterTransferCompleted = false;
bool isMasterIrqInIntmux                = false;

/*******************************************************************************
 * Code
 ******************************************************************************/

void LPSPI_MasterUserCallback(LPSPI_Type *base, lpspi_master_handle_t *handle, status_t status, void *userData)
{
    if (status == kStatus_Success)
    {
        __NOP();
    }
    isMasterTransferCompleted = true;
}

void FLEXIO_SPI_SlaveUserCallback(FLEXIO_SPI_Type *base,
                                  flexio_spi_slave_edma_handle_t *handle,
                                  status_t status,
                                  void *userData)
{
    if (status == kStatus_Success)
    {
        __NOP();
    }

    isSlaveTransferCompleted = true;

    PRINTF("This is FLEXIO SPI slave call back.\r\n");
}

int main(void)
{
    BOARD_InitHardware();

    PRINTF("LPSPI Master interrupt - FLEXIO SPI Slave edma example start.\r\n");
    PRINTF("This example use one lpspi instance as master and one flexio spi slave on one board.\r\n");
    PRINTF("Master uses interrupt and slave uses edma way.\r\n");
    PRINTF("Please make sure you make the correct line connection. Basically, the connection is: \r\n");
    PRINTF("LPSPI_master -- FLEXIO_SPI_slave   \r\n");
    PRINTF("   CLK      --    CLK  \r\n");
    PRINTF("   PCS      --    PCS  \r\n");
    PRINTF("   SOUT     --    SIN  \r\n");
    PRINTF("   SIN      --    SOUT \r\n");

    uint32_t errorCount;
    uint32_t i;
    lpspi_master_config_t masterConfig;
    flexio_spi_slave_config_t slaveConfig;
    lpspi_transfer_t masterXfer;
    flexio_spi_transfer_t slaveXfer;
    edma_config_t config;

    /*Master config*/
    LPSPI_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate     = TRANSFER_BAUDRATE;
    masterConfig.bitsPerFrame = 8;
    masterConfig.cpol         = kLPSPI_ClockPolarityActiveHigh;
    masterConfig.cpha         = kLPSPI_ClockPhaseFirstEdge;
    masterConfig.direction    = kLPSPI_MsbFirst;

    masterConfig.pcsToSckDelayInNanoSec        = 1000000000 / masterConfig.baudRate;
    masterConfig.lastSckToPcsDelayInNanoSec    = 1000000000 / masterConfig.baudRate;
    masterConfig.betweenTransferDelayInNanoSec = 1000000000 / masterConfig.baudRate;

    masterConfig.whichPcs           = MASTER_LPSPI_PCS_FOR_INIT;
    masterConfig.pcsActiveHighOrLow = kLPSPI_PcsActiveLow;

    masterConfig.pinCfg           = kLPSPI_SdiInSdoOut;
    masterConfig.dataOutConfig    = kLpspiDataOutRetained;
    masterConfig.enableInputDelay = false;

    LPSPI_MasterInit(MASTER_LPSPI_BASEADDR, &masterConfig, MASTER_LPSPI_CLOCK_FREQUENCY);

    /* Slave config */
    FLEXIO_SPI_SlaveGetDefaultConfig(&slaveConfig);

    spiDev.flexioBase      = SLAVE_FLEXIO_SPI_BASEADDR;
    spiDev.SDOPinIndex     = FLEXIO_SPI_SOUT_PIN;
    spiDev.SDIPinIndex     = FLEXIO_SPI_SIN_PIN;
    spiDev.SCKPinIndex     = FLEXIO_SPI_CLK_PIN;
    spiDev.CSnPinIndex     = FLEXIO_SPI_PCS_PIN;
    spiDev.shifterIndex[0] = FLEXIO_TX_SHIFTER_INDEX;
    spiDev.shifterIndex[1] = FLEXIO_RX_SHIFTER_INDEX;
    spiDev.timerIndex[0]   = 0U;
    FLEXIO_SPI_SlaveInit(&spiDev, &slaveConfig);

    /* Set up the transfer data */
    for (i = 0U; i < TRANSFER_SIZE; i++)
    {
        masterTxData[i] = i % 256U;
        masterRxData[i] = 0U;

        slaveTxData[i] = ~masterTxData[i];
        slaveRxData[i] = 0U;
    }

    isSlaveTransferCompleted = false;

#if defined(FSL_FEATURE_SOC_DMAMUX_COUNT) && FSL_FEATURE_SOC_DMAMUX_COUNT
    /* Init DMAMUX */
    DMAMUX_Init(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR);

    /* Set channel for FLEXIO */
    DMAMUX_SetSource(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR, FLEXIO_SPI_TX_DMA_LPSPI_CHANNEL, EXAMPLE_TX_DMA_SOURCE);
    DMAMUX_SetSource(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR, FLEXIO_SPI_RX_DMA_LPSPI_CHANNEL, EXAMPLE_RX_DMA_SOURCE);
    DMAMUX_EnableChannel(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR, FLEXIO_SPI_TX_DMA_LPSPI_CHANNEL);
    DMAMUX_EnableChannel(EXAMPLE_FLEXIO_SPI_DMAMUX_BASEADDR, FLEXIO_SPI_RX_DMA_LPSPI_CHANNEL);
#endif

    /* Init the EDMA module */
    EDMA_GetDefaultConfig(&config);
#if defined(BOARD_GetEDMAConfig)
    BOARD_GetEDMAConfig(config);
#endif
    EDMA_Init(EXAMPLE_FLEXIO_SPI_DMA_LPSPI_BASEADDR, &config);

    EDMA_CreateHandle(&txHandle, EXAMPLE_FLEXIO_SPI_DMA_LPSPI_BASEADDR, FLEXIO_SPI_TX_DMA_LPSPI_CHANNEL);
    EDMA_CreateHandle(&rxHandle, EXAMPLE_FLEXIO_SPI_DMA_LPSPI_BASEADDR, FLEXIO_SPI_RX_DMA_LPSPI_CHANNEL);

#if defined(FSL_FEATURE_EDMA_HAS_CHANNEL_MUX) && FSL_FEATURE_EDMA_HAS_CHANNEL_MUX
    EDMA_SetChannelMux(EXAMPLE_FLEXIO_SPI_DMA_LPSPI_BASEADDR, FLEXIO_SPI_TX_DMA_LPSPI_CHANNEL, EXAMPLE_TX_DMA_SOURCE);
    EDMA_SetChannelMux(EXAMPLE_FLEXIO_SPI_DMA_LPSPI_BASEADDR, FLEXIO_SPI_RX_DMA_LPSPI_CHANNEL, EXAMPLE_RX_DMA_SOURCE);
#endif

    /* Set up slave first */
    FLEXIO_SPI_SlaveTransferCreateHandleEDMA(&spiDev, &g_s_handle, FLEXIO_SPI_SlaveUserCallback, NULL, &txHandle,
                                             &rxHandle);

    /*Set slave transfer ready to receive/send data*/
    slaveXfer.txData   = slaveTxData;
    slaveXfer.rxData   = slaveRxData;
    slaveXfer.dataSize = TRANSFER_SIZE;
    slaveXfer.flags    = kFLEXIO_SPI_8bitMsb;

    FLEXIO_SPI_SlaveTransferEDMA(&spiDev, &g_s_handle, &slaveXfer);

    /* Set up master transfer */
    LPSPI_MasterTransferCreateHandle(MASTER_LPSPI_BASEADDR, &g_m_handle, LPSPI_MasterUserCallback, NULL);

    /*Start master transfer*/
    masterXfer.txData      = masterTxData;
    masterXfer.rxData      = masterRxData;
    masterXfer.dataSize    = TRANSFER_SIZE;
    masterXfer.configFlags = MASTER_LPSPI_PCS_FOR_TRANSFER | kLPSPI_MasterByteSwap;

    LPSPI_MasterTransferNonBlocking(MASTER_LPSPI_BASEADDR, &g_m_handle, &masterXfer);

    /* Wait slave received all data. */
    while (!(isSlaveTransferCompleted && isMasterTransferCompleted))
    {
    }

    errorCount = 0U;
    for (i = 0U; i < TRANSFER_SIZE; i++)
    {
        if (masterTxData[i] != slaveRxData[i])
        {
            errorCount++;
        }

        if (slaveTxData[i] != masterRxData[i])
        {
            errorCount++;
        }
    }
    if (errorCount == 0U)
    {
        PRINTF("LPSPI master <-> FLEXIO SPI slave transfer all data matched!\r\n");
    }
    else
    {
        PRINTF("Error occurred in LPSPI master <-> FLEXIO SPI slave transfer!\r\n");
    }

    LPSPI_Deinit(MASTER_LPSPI_BASEADDR);
    FLEXIO_SPI_SlaveDeinit(&spiDev);

    PRINTF("\r\nEnd of Example. \r\n");

    while (1)
    {
    }
}
