/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"
#include "app.h"
#include "fsl_pdm.h"
#include "fsl_pdm_edma.h"
#include "fsl_edma.h"
#include "fsl_sai.h"
#include "fsl_sai_edma.h"
#include "fsl_debug_console.h"
#include "fsl_codec_common.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define BUFFER_SIZE   (1024)
#define BUFFER_NUMBER (4)
#ifndef DEMO_CODEC_VOLUME
#define DEMO_CODEC_VOLUME 100U
#endif

#if defined(DEMO_DMA)
#define DEMO_PDM_DMA DEMO_DMA
#define DEMO_SAI_DMA DEMO_DMA
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void pdmCallback(PDM_Type *base, pdm_edma_handle_t *handle, status_t status, void *userData);
static void saiCallback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);
/*******************************************************************************
 * Variables
 ******************************************************************************/
AT_NONCACHEABLE_SECTION_ALIGN(pdm_edma_handle_t s_pdmRxHandle, 4);
AT_NONCACHEABLE_SECTION_ALIGN(edma_handle_t s_pdmDmaHandle, 4);
AT_NONCACHEABLE_SECTION_ALIGN(edma_handle_t s_saiDmaHandle, 4);
#if defined(DEMO_QUICKACCESS_SECTION_CACHEABLE) && DEMO_QUICKACCESS_SECTION_CACHEABLE
AT_NONCACHEABLE_SECTION_ALIGN(sai_edma_handle_t s_saiTxHandle, 4);
AT_NONCACHEABLE_SECTION_ALIGN(edma_tcd_t s_edmaTcd[4], 32U);
#else
AT_QUICKACCESS_SECTION_DATA_ALIGN(sai_edma_handle_t s_saiTxHandle, 4);
AT_QUICKACCESS_SECTION_DATA_ALIGN(edma_tcd_t s_edmaTcd[4], 32U);
#endif
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t s_buffer[BUFFER_SIZE * BUFFER_NUMBER], 4);
static volatile uint32_t s_bufferValidBlock = BUFFER_NUMBER;
static volatile uint32_t s_readIndex        = 0U;
static volatile uint32_t s_writeIndex       = 0U;
static const pdm_config_t pdmConfig         = {
#if defined(FSL_FEATURE_PDM_HAS_DECIMATION_FILTER_BYPASS) && FSL_FEATURE_PDM_HAS_DECIMATION_FILTER_BYPASS
    .enableFilterBypass = false,
#endif
    .enableDoze        = false,
    .fifoWatermark     = DEMO_PDM_FIFO_WATERMARK,
    .qualityMode       = DEMO_PDM_QUALITY_MODE,
    .cicOverSampleRate = DEMO_PDM_CIC_OVERSAMPLE_RATE,
};
static const pdm_channel_config_t channelConfig = {
#if (defined(FSL_FEATURE_PDM_HAS_DC_OUT_CTRL) && (FSL_FEATURE_PDM_HAS_DC_OUT_CTRL))
#ifdef DEMO_PDM_CHANNEL_OUTPUT_CUTOFF_FREQUENCY
    .outputCutOffFreq = DEMO_PDM_CHANNEL_OUTPUT_CUTOFF_FREQUENCY,
#else
    .outputCutOffFreq = kPDM_DcRemoverCutOff40Hz,
#endif
#endif

#if !(defined(FSL_FEATURE_PDM_DC_CTRL_VALUE_FIXED) && (FSL_FEATURE_PDM_DC_CTRL_VALUE_FIXED))
#ifdef DEMO_PDM_CHANNEL_CUTOFF_FREQUENCY
    .cutOffFreq = DEMO_PDM_CHANNEL_CUTOFF_FREQUENCY,
#else
    .cutOffFreq = kPDM_DcRemoverCutOff152Hz,
#endif
#endif

#ifdef DEMO_PDM_CHANNEL_GAIN
    .gain = DEMO_PDM_CHANNEL_GAIN,
#else
    .gain       = kPDM_DfOutputGain7,
#endif
};

codec_handle_t codecHandle;
extern codec_config_t boardCodecConfig;
/*******************************************************************************
 * Code
 ******************************************************************************/
static void pdmCallback(PDM_Type *base, pdm_edma_handle_t *handle, status_t status, void *userData)
{
    if (s_bufferValidBlock)
    {
        s_bufferValidBlock--;
    }
}

static void saiCallback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData)
{
    if (kStatus_SAI_TxError == status)
    {
        /* Handle the error. */
    }
    else
    {
        s_bufferValidBlock++;
    }
}

void DEMO_SAI_IRQ_HANDLER(void)
{
    SAI_TxClearStatusFlags(DEMO_SAI, kSAI_FIFOErrorFlag);
    SDK_ISR_EXIT_BARRIER;
}

/*!
 * @brief Main function
 */
int main(void)
{
    pdm_edma_transfer_t pdmXfer;
    edma_config_t dmaConfig = {0};
    sai_transfer_t saiXfer;
    sai_transceiver_t config;

    BOARD_InitHardware();

    PRINTF("PDM SAI Edma example started!\n\r");

    /* Create EDMA handle */
    /*
     * dmaConfig.enableRoundRobinArbitration = false;
     * dmaConfig.enableHaltOnError = true;
     * dmaConfig.enableContinuousLinkMode = false;
     * dmaConfig.enableDebugMode = false;
     */
    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_Init(DEMO_PDM_DMA, &dmaConfig);
    if (((void *)DEMO_PDM_DMA) != ((void *)DEMO_SAI_DMA))
    {
        EDMA_Init(DEMO_SAI_DMA, &dmaConfig);
    }
    EDMA_CreateHandle(&s_pdmDmaHandle, DEMO_PDM_DMA, DEMO_PDM_EDMA_CHANNEL);
    EDMA_CreateHandle(&s_saiDmaHandle, DEMO_SAI_DMA, DEMO_SAI_EDMA_CHANNEL);
#if defined(FSL_FEATURE_EDMA_HAS_CHANNEL_MUX) && FSL_FEATURE_EDMA_HAS_CHANNEL_MUX
    EDMA_SetChannelMux(DEMO_PDM_DMA, DEMO_PDM_EDMA_CHANNEL, DEMO_PDM_EDMA_SOURCE);
    EDMA_SetChannelMux(DEMO_SAI_DMA, DEMO_SAI_EDMA_CHANNEL, DEMO_SAI_EDMA_SOURCE);
#endif
    /* SAI init */
    SAI_Init(DEMO_SAI);
    SAI_TransferTxCreateHandleEDMA(DEMO_SAI, &s_saiTxHandle, saiCallback, NULL, &s_saiDmaHandle);
    /* I2S mode configurations */
    SAI_GetClassicI2SConfig(&config, DEMO_AUDIO_BIT_WIDTH, kSAI_MonoLeft, 1U << DEMO_SAI_CHANNEL);

    config.bitClock.bclkSource = DEMO_SAI_CLOCK_SOURCE;
    config.masterSlave         = DEMO_SAI_MASTER_SLAVE;
#if defined BOARD_SAI_RXCONFIG
    config.syncMode = DEMO_SAI_TX_SYNC_MODE;
#endif

    SAI_TransferTxSetConfigEDMA(DEMO_SAI, &s_saiTxHandle, &config);

    /* set bit clock divider */
    SAI_TxSetBitClockRate(DEMO_SAI, DEMO_AUDIO_MASTER_CLOCK, DEMO_AUDIO_SAMPLE_RATE, DEMO_AUDIO_BIT_WIDTH,
                          DEMO_AUDIO_DATA_CHANNEL);
#if defined BOARD_SAI_RXCONFIG
    BOARD_SAI_RXCONFIG(&config, DEMO_SAI_RX_SYNC_MODE);
#endif
    /* master clock configurations */
    BOARD_MasterClockConfig();

    if (CODEC_Init(&codecHandle, &boardCodecConfig) != kStatus_Success)
    {
        assert(false);
    }
    if (CODEC_SetVolume(&codecHandle, kCODEC_PlayChannelHeadphoneLeft | kCODEC_PlayChannelHeadphoneRight,
                        DEMO_CODEC_VOLUME) != kStatus_Success)
    {
        assert(false);
    }
    /* Setup pdm */
    PDM_Init(DEMO_PDM, &pdmConfig);
    PDM_TransferCreateHandleEDMA(DEMO_PDM, &s_pdmRxHandle, pdmCallback, NULL, &s_pdmDmaHandle);
    PDM_TransferInstallEDMATCDMemory(&s_pdmRxHandle, s_edmaTcd, 4);
#if defined DEMO_PDM_ENABLE_CHANNEL
    PDM_TransferSetChannelConfigEDMA(DEMO_PDM, &s_pdmRxHandle, DEMO_PDM_ENABLE_CHANNEL, &channelConfig);
#else
    PDM_TransferSetChannelConfigEDMA(DEMO_PDM, &s_pdmRxHandle, DEMO_PDM_ENABLE_CHANNEL_LEFT, &channelConfig);
#endif
    if (PDM_SetSampleRateConfig(DEMO_PDM, DEMO_PDM_CLK_FREQ, DEMO_AUDIO_SAMPLE_RATE) != kStatus_Success)
    {
        PRINTF("PDM configure sample rate failed.\r\n");
        return -1;
    }
    PDM_Reset(DEMO_PDM);

    while (1)
    {
        /* wait one buffer idle to recieve data */
        if (s_bufferValidBlock > 0)
        {
            pdmXfer.data         = (uint8_t *)((uint32_t)s_buffer + s_readIndex * BUFFER_SIZE);
            pdmXfer.dataSize     = BUFFER_SIZE;
            pdmXfer.linkTransfer = NULL;
            if (kStatus_Success == PDM_TransferReceiveEDMA(DEMO_PDM, &s_pdmRxHandle, &pdmXfer))
            {
                s_readIndex++;
            }
            if (s_readIndex == BUFFER_NUMBER)
            {
                s_readIndex = 0U;
            }
        }
        /* wait one buffer busy to send data */
        if (s_bufferValidBlock < BUFFER_NUMBER)
        {
            saiXfer.data     = (uint8_t *)((uint32_t)s_buffer + s_writeIndex * BUFFER_SIZE);
            saiXfer.dataSize = BUFFER_SIZE;
            if (kStatus_Success == SAI_TransferSendEDMA(DEMO_SAI, &s_saiTxHandle, &saiXfer))
            {
                s_writeIndex++;
            }
            if (s_writeIndex == BUFFER_NUMBER)
            {
                s_writeIndex = 0U;
            }
        }
    }
}
