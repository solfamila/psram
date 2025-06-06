/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"
#include "app.h"
#include "fsl_pdm.h"
#include "fsl_debug_console.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define SAMPLE_COUNT (256)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
SDK_ALIGN(static uint32_t txBuff[SAMPLE_COUNT], 4);
#if (defined(FSL_FEATURE_PDM_HAS_STATUS_LOW_FREQ) && (FSL_FEATURE_PDM_HAS_STATUS_LOW_FREQ == 1U))
static volatile bool s_lowFreqFlag = false;
#endif
static volatile bool s_fifoErrorFlag        = false;
static volatile bool s_dataReadFinishedFlag = false;
static volatile uint32_t s_readIndex        = 0U;
static const pdm_config_t pdmConfig         = {
#if defined(FSL_FEATURE_PDM_HAS_DECIMATION_FILTER_BYPASS) && FSL_FEATURE_PDM_HAS_DECIMATION_FILTER_BYPASS
    .enableFilterBypass = false,
#endif
    .enableDoze        = false,
    .fifoWatermark     = DEMO_PDM_FIFO_WATERMARK,
    .qualityMode       = DEMO_PDM_QUALITY_MODE,
    .cicOverSampleRate = DEMO_PDM_CIC_OVERSAMPLE_RATE,
};
static pdm_channel_config_t channelConfig = {
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
/*******************************************************************************
 * Code
 ******************************************************************************/
static void pdm_error_irqHandler(void)
{
    uint32_t status = 0U;

#if (defined(FSL_FEATURE_PDM_HAS_STATUS_LOW_FREQ) && (FSL_FEATURE_PDM_HAS_STATUS_LOW_FREQ == 1U))
    if (PDM_GetStatus(DEMO_PDM) & PDM_STAT_LOWFREQF_MASK)
    {
        PDM_ClearStatus(DEMO_PDM, PDM_STAT_LOWFREQF_MASK);
        s_lowFreqFlag = true;
    }
#endif

    status = PDM_GetFifoStatus(DEMO_PDM);
    if (status != 0U)
    {
        PDM_ClearFIFOStatus(DEMO_PDM, status);
        s_fifoErrorFlag = true;
    }
#if defined(FSL_FEATURE_PDM_HAS_RANGE_CTRL) && FSL_FEATURE_PDM_HAS_RANGE_CTRL
    status = PDM_GetRangeStatus(DEMO_PDM);
    if (status != 0U)
    {
        PDM_ClearRangeStatus(DEMO_PDM, status);
    }
#else
    status = PDM_GetOutputStatus(DEMO_PDM);
    if (status != 0U)
    {
        PDM_ClearOutputStatus(DEMO_PDM, status);
    }
#endif
}

#if !(defined FSL_FEATURE_PDM_HAS_NO_INDEPENDENT_ERROR_IRQ && FSL_FEATURE_PDM_HAS_NO_INDEPENDENT_ERROR_IRQ)
void PDM_ERROR_IRQHandler(void)
{
    pdm_error_irqHandler();
    SDK_ISR_EXIT_BARRIER;
}
#endif

void PDM_EVENT_IRQHandler(void)
{
    uint32_t i = 0U, status = PDM_GetStatus(DEMO_PDM);
    /* recieve data */
    if ((1U << DEMO_PDM_ENABLE_CHANNEL_LEFT) & status)
    {
        for (i = 0U; i < DEMO_PDM_FIFO_WATERMARK; i++)
        {
            if (s_readIndex < SAMPLE_COUNT)
            {
                txBuff[s_readIndex] = PDM_ReadData(DEMO_PDM, DEMO_PDM_ENABLE_CHANNEL_LEFT);
                s_readIndex++;
            }
        }
    }

    if ((1U << DEMO_PDM_ENABLE_CHANNEL_RIGHT) & status)
    {
        for (i = 0U; i < DEMO_PDM_FIFO_WATERMARK; i++)
        {
            if (s_readIndex < SAMPLE_COUNT)
            {
                txBuff[s_readIndex] = PDM_ReadData(DEMO_PDM, DEMO_PDM_ENABLE_CHANNEL_RIGHT);
                s_readIndex++;
            }
        }
    }

    /* handle PDM error status */
#if (defined FSL_FEATURE_PDM_HAS_NO_INDEPENDENT_ERROR_IRQ && FSL_FEATURE_PDM_HAS_NO_INDEPENDENT_ERROR_IRQ)
    pdm_error_irqHandler();
#endif

    PDM_ClearStatus(DEMO_PDM, status);
    if (s_readIndex >= SAMPLE_COUNT)
    {
        s_dataReadFinishedFlag = true;
        PDM_Enable(DEMO_PDM, false);
    }
    SDK_ISR_EXIT_BARRIER;
}

/*!
 * @brief Main function
 */
int main(void)
{
    uint32_t i = 0U, j = 0U;

    BOARD_InitHardware();

    PRINTF("PDM interrupt example started!\n\r");

    memset(txBuff, 0U, sizeof(txBuff));

    /* Set up pdm */
    PDM_Init(DEMO_PDM, &pdmConfig);

    PDM_SetChannelConfig(DEMO_PDM, DEMO_PDM_ENABLE_CHANNEL_LEFT, &channelConfig);
    PDM_SetChannelConfig(DEMO_PDM, DEMO_PDM_ENABLE_CHANNEL_RIGHT, &channelConfig);
    if (PDM_SetSampleRateConfig(DEMO_PDM, DEMO_PDM_CLK_FREQ, DEMO_AUDIO_SAMPLE_RATE) != kStatus_Success)
    {
        PRINTF("PDM configure sample rate failed.\r\n");
        return -1;
    }
    PDM_EnableInterrupts(DEMO_PDM, kPDM_ErrorInterruptEnable | kPDM_FIFOInterruptEnable);

    EnableIRQ(PDM_EVENT_IRQn);
#if !(defined FSL_FEATURE_PDM_HAS_NO_INDEPENDENT_ERROR_IRQ && FSL_FEATURE_PDM_HAS_NO_INDEPENDENT_ERROR_IRQ)
    EnableIRQ(PDM_ERROR_IRQn);
#endif
    PDM_Enable(DEMO_PDM, true);

    /* wait data read finish */
    while (!s_dataReadFinishedFlag)
    {
#if (defined(FSL_FEATURE_PDM_HAS_STATUS_LOW_FREQ) && (FSL_FEATURE_PDM_HAS_STATUS_LOW_FREQ == 1U))
        if (s_lowFreqFlag)
        {
            s_lowFreqFlag = false;
            PRINTF("PDM root clock freq too low, please switch to a higher value\r\n");
        }
#endif
    }

    PRINTF("PDM recieve data:\n\r");
    for (i = 0U; i < SAMPLE_COUNT; i++)
    {
        PRINTF("%6x ", txBuff[i]);
        if (++j > 32U)
        {
            j = 0U;
            PRINTF("\r\n");
        }
    }

    PDM_Deinit(DEMO_PDM);

    PRINTF("\r\nPDM interrupt example finished!\n\r ");
    while (1)
    {
    }
}
