/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017,2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
/*${standard_header_anchor}*/
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_audio.h"

#include "usb_device_ch9.h"
#include "usb_audio_config.h"
#include "usb_device_descriptor.h"
#include "fsl_adapter_audio.h"
#include "audio_speaker.h"
#include "fsl_device_registers.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "board.h"
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

#include "app.h"
#if ((defined FSL_FEATURE_SOC_USBPHY_COUNT) && (FSL_FEATURE_SOC_USBPHY_COUNT > 0U))
#include "usb_phy.h"
#endif
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#include "fsl_ctimer.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define AUDIO_SAMPLING_RATE_TO_10_14 (AUDIO_SAMPLING_RATE_KHZ << 10)
#define AUDIO_SAMPLING_RATE_TO_16_16 \
    ((AUDIO_SAMPLING_RATE_KHZ / (AUDIO_OUT_TRANSFER_LENGTH_ONE_FRAME / HS_ISO_OUT_ENDP_PACKET_SIZE)) << 13)

/* audio 2.0 and high speed, use low latency, but IP3511HS controller do not have micro frame count */
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#if (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
volatile static uint8_t s_microFrameCountIp3511HS = 0;
#endif
#endif

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
extern volatile uint8_t feedbackValueUpdating;
#endif

#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#if defined(USB_DEVICE_WORKAROUND_AUDIO_20_WINDOWS) && (USB_DEVICE_WORKAROUND_AUDIO_20_WINDOWS > 0)
/* this is used for windows that supports usb audio 2.0 */
#define AUDIO_UPDATE_FEEDBACK_DATA(m, n)              \
    {                                                 \
        feedbackValueUpdating = 1;                    \
        m[0]                  = ((n << 6U) & 0xFFu);  \
        m[1]                  = ((n >> 2U) & 0xFFu);  \
        m[2]                  = ((n >> 10U) & 0xFFu); \
        m[3]                  = ((n >> 18U) & 0xFFu); \
        feedbackValueUpdating = 0;                    \
    }
#else
/* change 10.10 data to 10.14 or change 12.13 to 16.16 */
#define AUDIO_UPDATE_FEEDBACK_DATA(m, n)                  \
    feedbackValueUpdating = 1;                            \
    if (USB_SPEED_HIGH == g_UsbDeviceAudioSpeaker.speed)  \
    {                                                     \
        m[0] = (((n & 0x00001FFFu) << 3) & 0xFFu);        \
        m[1] = ((((n & 0x00001FFFu) << 3) >> 8) & 0xFFu); \
        m[2] = (((n & 0x01FFE000u) >> 13) & 0xFFu);       \
        m[3] = (((n & 0x01FFE000u) >> 21) & 0xFFu);       \
    }                                                     \
    else                                                  \
    {                                                     \
        m[0] = ((n << 4) & 0xFFU);                        \
        m[1] = (((n << 4) >> 8U) & 0xFFU);                \
        m[2] = (((n << 4) >> 16U) & 0xFFU);               \
    }                                                     \
    feedbackValueUpdating = 0;
#endif
#else
/* change 10.10 data to 10.14 */
#define AUDIO_UPDATE_FEEDBACK_DATA(m, n)                     \
    {                                                        \
        feedbackValueUpdating = 1;                           \
        m[0]                  = ((n << 4) & 0xFFU);          \
        m[1]                  = (((n << 4) >> 8U) & 0xFFU);  \
        m[2]                  = (((n << 4) >> 16U) & 0xFFU); \
        feedbackValueUpdating = 0;                           \
    }
#endif

#define USB_AUDIO_ENTER_CRITICAL() \
                                   \
    OSA_SR_ALLOC();                \
                                   \
    OSA_ENTER_CRITICAL()

#define USB_AUDIO_EXIT_CRITICAL() OSA_EXIT_CRITICAL()
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void BOARD_InitHardware(void);
void USB_DeviceClockInit(void);
void USB_DeviceIsrEnable(void);
#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle);
#endif

#if (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
#if !((defined FSL_FEATURE_SOC_USBPHY_COUNT) && (FSL_FEATURE_SOC_USBPHY_COUNT > 0U))
void USB_DeviceHsPhyChirpIssueWorkaround(void);
void USB_DeviceDisconnected(void);
#endif
#endif
usb_status_t USB_DeviceAudioCallback(class_handle_t handle, uint32_t event, void *param);
usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);

extern void AUDIO_DMA_EDMA_Start();
extern void BOARD_Codec_Init();
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
extern void CTIMER_CaptureInit(void);
#if ((defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)) || \
     (defined(USB_DEVICE_CONFIG_KHCI) && (USB_DEVICE_CONFIG_KHCI > 0U)))
extern void audio_fro_trim_up(void);
extern void audio_fro_trim_down(void);
#endif
extern void USB_AudioPllChange(void);
#endif
/*******************************************************************************
 * Variables
 ******************************************************************************/
AT_NONCACHEABLE_SECTION_ALIGN(uint8_t audioPlayDataBuff[AUDIO_SPEAKER_DATA_WHOLE_BUFFER_COUNT_NORMAL * AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME], 4U);
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t audioPlayPacket[FS_ISO_OUT_ENDP_PACKET_SIZE];
#else
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t audioPlayPacket[FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_FORMAT_CHANNELS * AUDIO_FORMAT_SIZE];
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t usbAudioFeedBackBuffer[4];
USB_RAM_ADDRESS_ALIGNMENT(4) uint8_t audioFeedBackBuffer[4];
volatile uint8_t feedbackValueUpdating;
#endif
extern hal_audio_config_t audioTxConfig;
extern HAL_AUDIO_HANDLE_DEFINE(audioTxHandle);
extern usb_device_class_struct_t g_UsbDeviceAudioClass;

/* Default value of audio generator device struct */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
usb_audio_speaker_struct_t g_UsbDeviceAudioSpeaker = {
    .deviceHandle = NULL,
    .audioHandle  = (class_handle_t)NULL,
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
    .currentStreamOutMaxPacketSize = (FS_ISO_OUT_ENDP_PACKET_SIZE),
#else
    .currentStreamOutMaxPacketSize = (FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_FORMAT_CHANNELS * AUDIO_FORMAT_SIZE),
    .currentFeedbackMaxPacketSize  = FS_ISO_FEEDBACK_ENDP_PACKET_SIZE,
#endif
    .attach               = 0U,
    .copyProtect          = 0x01U,
    .curMute              = 0x00U,
    .curVolume            = {0x00U, 0x1fU},
    .minVolume            = {0x00U, 0x00U},
    .maxVolume            = {0x00U, 0x43U},
    .resVolume            = {0x01U, 0x00U},
    .curBass              = 0x00U,
    .minBass              = 0x80U,
    .maxBass              = 0x7FU,
    .resBass              = 0x01U,
    .curMid               = 0x00U,
    .minMid               = 0x80U,
    .maxMid               = 0x7FU,
    .resMid               = 0x01U,
    .curTreble            = 0x01U,
    .minTreble            = 0x80U,
    .maxTreble            = 0x7FU,
    .resTreble            = 0x01U,
    .curAutomaticGain     = 0x01U,
    .curDelay             = {0x00U, 0x40U},
    .minDelay             = {0x00U, 0x00U},
    .maxDelay             = {0xFFU, 0xFFU},
    .resDelay             = {0x00U, 0x01U},
    .curLoudness          = 0x01U,
    .curSamplingFrequency = {0x00U, 0x00U, 0x01U},
    .minSamplingFrequency = {0x00U, 0x00U, 0x01U},
    .maxSamplingFrequency = {0x00U, 0x00U, 0x01U},
    .resSamplingFrequency = {0x00U, 0x00U, 0x01U},
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
    .curMute20          = 0U,
    .curClockValid      = 1U,
    .curVolume20        = {0x00U, 0x1FU},
    .curSampleFrequency = 48000U, /* This should be changed to 48000 if sampling rate is 48k */
    .freqControlRange   = {1U, 48000U, 48000U, 0U},
    .volumeControlRange = {1U, 0x8001U, 0x7FFFU, 1U},
#endif
    .speed                      = USB_SPEED_FULL,
    .tdWriteNumberPlay          = 0,
    .tdReadNumberPlay           = 0,
    .audioSendCount             = {0, 0},
    .audioSpeakerReadDataCount  = {0, 0},
    .audioSpeakerWriteDataCount = {0, 0},
    .usbRecvCount               = 0,
    .audioSendTimes             = 0,
    .usbRecvTimes               = 0,
    .startPlayFlag              = 0,
    .speakerIntervalCount       = 0,
    .speakerReservedSpace       = 0,
    .speakerDetachOrNoInput     = 0,
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#if ((defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)) || \
     (defined(USB_DEVICE_CONFIG_KHCI) && (USB_DEVICE_CONFIG_KHCI > 0U)))
    .froTrimIntervalCount     = 0,
    .usbFroTicksPrev          = 0,
    .usbFroTicksEma           = AUDIO_FRO_USB_SOF_INTERVAL_TICK_COUNT,
    .usbFroTickEmaFrac        = 0,
    .usbFroTickBasedPrecision = AUDIO_USB_FRO_TRIM_TICK_BASED_PRECISION,
#endif
    .curAudioPllFrac            = AUDIO_PLL_FRACTIONAL_DIVIDER,
    .audioPllTicksPrev          = 0,
    .audioPllTicksEma           = AUDIO_PLL_USB_SOF_INTERVAL_TICK_COUNT,
    .audioPllTickEmaFrac        = 0,
    .audioPllTickBasedPrecision = AUDIO_PLL_FRACTION_TICK_BASED_PRECISION,
    .stopDataLengthAudioAdjust  = 0U,
#endif
};

/* USB device class information */
static usb_device_class_config_struct_t s_audioConfig[1] = {{
    USB_DeviceAudioCallback,
    (class_handle_t)NULL,
    &g_UsbDeviceAudioClass,
}};

/* USB device class configuration information */
static usb_device_class_config_list_struct_t s_audioConfigList = {
    s_audioConfig,
    USB_DeviceCallback,
    1U,
};

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Audio class specific request function.
 *
 * This function handles the Audio class specific requests.
 *
 * @param handle		  The Audio class handle.
 * @param event 		  The Audio class event type.
 * @param param 		  The parameter of the class specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceAudioRequest(class_handle_t handle, uint32_t event, void *param)
{
    usb_device_control_request_struct_t *request = (usb_device_control_request_struct_t *)param;
    usb_status_t error                           = kStatus_USB_Success;

    switch (event)
    {
        case USB_DEVICE_AUDIO_FU_GET_CUR_MUTE_CONTROL:
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
            request->buffer = (uint8_t *)&g_UsbDeviceAudioSpeaker.curMute20;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curMute20);
#else
            request->buffer = &g_UsbDeviceAudioSpeaker.curMute;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curMute);
#endif
            break;
        case USB_DEVICE_AUDIO_FU_GET_CUR_VOLUME_CONTROL:
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
            request->buffer = (uint8_t *)&g_UsbDeviceAudioSpeaker.curVolume20;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curVolume20);
#else
            request->buffer = g_UsbDeviceAudioSpeaker.curVolume;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curVolume);
#endif
            break;
        case USB_DEVICE_AUDIO_FU_GET_CUR_BASS_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.curBass;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curBass);
            break;
        case USB_DEVICE_AUDIO_FU_GET_CUR_MID_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.curMid;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curMid);
            break;
        case USB_DEVICE_AUDIO_FU_GET_CUR_TREBLE_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.curTreble;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curTreble);
            break;
        case USB_DEVICE_AUDIO_FU_GET_CUR_AUTOMATIC_GAIN_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.curAutomaticGain;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curAutomaticGain);
            break;
        case USB_DEVICE_AUDIO_FU_GET_CUR_DELAY_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.curDelay;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curDelay);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MIN_VOLUME_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.minVolume;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.minVolume);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MIN_BASS_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.minBass;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.minBass);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MIN_MID_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.minMid;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.minMid);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MIN_TREBLE_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.minTreble;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.minTreble);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MIN_DELAY_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.minDelay;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.minDelay);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MAX_VOLUME_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.maxVolume;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.maxVolume);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MAX_BASS_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.maxBass;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.maxBass);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MAX_MID_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.maxMid;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.maxMid);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MAX_TREBLE_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.maxTreble;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.maxTreble);
            break;
        case USB_DEVICE_AUDIO_FU_GET_MAX_DELAY_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.maxDelay;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.maxDelay);
            break;
        case USB_DEVICE_AUDIO_FU_GET_RES_VOLUME_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.resVolume;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.resVolume);
            break;
        case USB_DEVICE_AUDIO_FU_GET_RES_BASS_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.resBass;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.resBass);
            break;
        case USB_DEVICE_AUDIO_FU_GET_RES_MID_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.resMid;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.resMid);
            break;
        case USB_DEVICE_AUDIO_FU_GET_RES_TREBLE_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.resTreble;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.resTreble);
            break;
        case USB_DEVICE_AUDIO_FU_GET_RES_DELAY_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.resDelay;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.resDelay);
            break;
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
        case USB_DEVICE_AUDIO_CS_GET_CUR_SAMPLING_FREQ_CONTROL:
            request->buffer = (uint8_t *)&g_UsbDeviceAudioSpeaker.curSampleFrequency;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curSampleFrequency);
            break;
        case USB_DEVICE_AUDIO_CS_SET_CUR_SAMPLING_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = (uint8_t *)&g_UsbDeviceAudioSpeaker.curSampleFrequency;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curSampleFrequency);
            }
            break;
        case USB_DEVICE_AUDIO_CS_GET_CUR_CLOCK_VALID_CONTROL:
            request->buffer = &g_UsbDeviceAudioSpeaker.curClockValid;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curClockValid);
            break;
        case USB_DEVICE_AUDIO_CS_SET_CUR_CLOCK_VALID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.curClockValid;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curClockValid);
            }
            break;
        case USB_DEVICE_AUDIO_CS_GET_RANGE_SAMPLING_FREQ_CONTROL:
            request->buffer = (uint8_t *)&g_UsbDeviceAudioSpeaker.freqControlRange;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.freqControlRange);
            break;
        case USB_DEVICE_AUDIO_FU_GET_RANGE_VOLUME_CONTROL:
            request->buffer = (uint8_t *)&g_UsbDeviceAudioSpeaker.volumeControlRange;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.volumeControlRange);
            break;
#else
        case USB_DEVICE_AUDIO_EP_GET_CUR_SAMPLING_FREQ_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.curSamplingFrequency;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.curSamplingFrequency);
            break;
        case USB_DEVICE_AUDIO_EP_GET_MIN_SAMPLING_FREQ_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.minSamplingFrequency;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.minSamplingFrequency);
            break;
        case USB_DEVICE_AUDIO_EP_GET_MAX_SAMPLING_FREQ_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.maxSamplingFrequency;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.maxSamplingFrequency);
            break;
        case USB_DEVICE_AUDIO_EP_GET_RES_SAMPLING_FREQ_CONTROL:
            request->buffer = g_UsbDeviceAudioSpeaker.resSamplingFrequency;
            request->length = sizeof(g_UsbDeviceAudioSpeaker.resSamplingFrequency);
            break;
        case USB_DEVICE_AUDIO_EP_SET_CUR_SAMPLING_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.curSamplingFrequency;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curSamplingFrequency);
            }
            break;
        case USB_DEVICE_AUDIO_EP_SET_RES_SAMPLING_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.resSamplingFrequency;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.resSamplingFrequency);
            }
            break;
        case USB_DEVICE_AUDIO_EP_SET_MAX_SAMPLING_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.maxSamplingFrequency;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.maxSamplingFrequency);
            }
            break;
        case USB_DEVICE_AUDIO_EP_SET_MIN_SAMPLING_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.minSamplingFrequency;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.minSamplingFrequency);
            }
            break;
#endif
        case USB_DEVICE_AUDIO_FU_SET_CUR_VOLUME_CONTROL:
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.curVolume20;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curVolume20);
            }
            else
            {
                g_UsbDeviceAudioSpeaker.codecTask |= VOLUME_CHANGE_TASK;
            }
#else
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.curVolume;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curVolume);
            }
            else
            {
                uint16_t volume = (uint16_t)((uint16_t)g_UsbDeviceAudioSpeaker.curVolume[1] << 8U);
                volume |= (uint8_t)(g_UsbDeviceAudioSpeaker.curVolume[0]);
                g_UsbDeviceAudioSpeaker.codecTask |= VOLUME_CHANGE_TASK;
            }
#endif
            break;
        case USB_DEVICE_AUDIO_FU_SET_CUR_MUTE_CONTROL:
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.curMute20;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curMute20);
            }
            else
            {
                if (g_UsbDeviceAudioSpeaker.curMute20)
                {
                    g_UsbDeviceAudioSpeaker.codecTask |= MUTE_CODEC_TASK;
                }
                else
                {
                    g_UsbDeviceAudioSpeaker.codecTask |= UNMUTE_CODEC_TASK;
                }
            }
#else
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.curMute;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curMute);
            }
            else
            {
                if (g_UsbDeviceAudioSpeaker.curMute)
                {
                    g_UsbDeviceAudioSpeaker.codecTask |= MUTE_CODEC_TASK;
                }
                else
                {
                    g_UsbDeviceAudioSpeaker.codecTask |= UNMUTE_CODEC_TASK;
                }
            }
#endif
            break;
        case USB_DEVICE_AUDIO_FU_SET_CUR_BASS_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.curBass;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curBass);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_CUR_MID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.curMid;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curMid);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_CUR_TREBLE_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.curTreble;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curTreble);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_CUR_AUTOMATIC_GAIN_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.curAutomaticGain;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curAutomaticGain);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_CUR_DELAY_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.curDelay;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.curDelay);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MIN_VOLUME_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.minVolume;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.minVolume);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MIN_BASS_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.minBass;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.minBass);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MIN_MID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.minMid;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.minMid);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MIN_TREBLE_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.minTreble;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.minTreble);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MIN_DELAY_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.minDelay;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.minDelay);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MAX_VOLUME_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.maxVolume;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.maxVolume);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MAX_BASS_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.maxBass;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.maxBass);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MAX_MID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.maxMid;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.maxMid);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MAX_TREBLE_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.maxTreble;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.maxTreble);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_MAX_DELAY_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.maxDelay;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.maxDelay);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_RES_VOLUME_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.resVolume;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.resVolume);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_RES_BASS_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.resBass;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.resBass);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_RES_MID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.resMid;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.resMid);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_RES_TREBLE_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &g_UsbDeviceAudioSpeaker.resTreble;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.resTreble);
            }
            break;
        case USB_DEVICE_AUDIO_FU_SET_RES_DELAY_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = g_UsbDeviceAudioSpeaker.resDelay;
                request->length = sizeof(g_UsbDeviceAudioSpeaker.resDelay);
            }
            break;
        default:
            error = kStatus_USB_InvalidRequest;
            break;
    }
    return error;
}

/* The USB_AudioSpeakerBufferSpaceUsed() function gets the used speaker ringbuffer size */
uint32_t USB_AudioSpeakerBufferSpaceUsed(void)
{
    uint64_t write_count = 0U;
    uint64_t read_count  = 0U;

    write_count = (uint64_t)((((uint64_t)g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[1]) << 32U) |
                             g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0]);
    read_count  = (uint64_t)((((uint64_t)g_UsbDeviceAudioSpeaker.audioSpeakerReadDataCount[1]) << 32U) |
                            g_UsbDeviceAudioSpeaker.audioSpeakerReadDataCount[0]);

    if (write_count >= read_count)
    {
        return (uint32_t)(write_count - read_count);
    }
    else
    {
        return 0;
    }
}

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
void USB_DeviceCalculateFeedback(void)
{
    volatile static uint64_t totalFrameValue = 0U;
    volatile static uint32_t frameDistance   = 0U;
    volatile static uint32_t feedbackValue   = 0U;

    uint32_t audioSpeakerUsedSpace = 0U;

    /* feedback interval is AUDIO_CALCULATE_Ff_INTERVAL */
    if (USB_SPEED_HIGH == g_UsbDeviceAudioSpeaker.speed)
    {
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0) /* high speed, feedback interval is AUDIO_CALCULATE_Ff_INTERVAL ms */
        if (g_UsbDeviceAudioSpeaker.speakerIntervalCount !=
            AUDIO_CALCULATE_Ff_INTERVAL *
                (AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME / g_UsbDeviceAudioSpeaker.audioPlayTransferSize))
#else
        if (g_UsbDeviceAudioSpeaker.speakerIntervalCount != AUDIO_CALCULATE_Ff_INTERVAL)
#endif
        {
            g_UsbDeviceAudioSpeaker.speakerIntervalCount++;
            return;
        }
    }
    else /* full speed, feedback interval is AUDIO_CALCULATE_Ff_INTERVAL ms */
    {
        if (g_UsbDeviceAudioSpeaker.speakerIntervalCount != AUDIO_CALCULATE_Ff_INTERVAL)
        {
            g_UsbDeviceAudioSpeaker.speakerIntervalCount++;
            return;
        }
    }

    if (0U == g_UsbDeviceAudioSpeaker.firstCalculateFeedback)
    {
        g_UsbDeviceAudioSpeaker.speakerIntervalCount = 0;
        g_UsbDeviceAudioSpeaker.currentFrameCount    = 0;
        g_UsbDeviceAudioSpeaker.audioSendCount[0]    = 0;
        g_UsbDeviceAudioSpeaker.audioSendCount[1]    = 0;
        totalFrameValue                              = 0;
        frameDistance                                = 0;
        feedbackValue                                = 0;
        USB_DeviceClassGetCurrentFrameCount(CONTROLLER_ID, (uint32_t *)&g_UsbDeviceAudioSpeaker.lastFrameCount);
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#if (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
        if (USB_SPEED_HIGH == g_UsbDeviceAudioSpeaker.speed)
        {
            g_UsbDeviceAudioSpeaker.lastFrameCount += s_microFrameCountIp3511HS;
        }
#endif
#endif
        g_UsbDeviceAudioSpeaker.firstCalculateFeedback = 1U;
        return;
    }
    g_UsbDeviceAudioSpeaker.speakerIntervalCount = 0;
    USB_DeviceClassGetCurrentFrameCount(CONTROLLER_ID, (uint32_t *)&g_UsbDeviceAudioSpeaker.currentFrameCount);
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#if (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
    if (USB_SPEED_HIGH == g_UsbDeviceAudioSpeaker.speed)
    {
        g_UsbDeviceAudioSpeaker.currentFrameCount += s_microFrameCountIp3511HS;
    }
#endif
#endif
    frameDistance = ((g_UsbDeviceAudioSpeaker.currentFrameCount + USB_DEVICE_MAX_FRAME_COUNT + 1U -
                      g_UsbDeviceAudioSpeaker.lastFrameCount) &
                     USB_DEVICE_MAX_FRAME_COUNT);
    g_UsbDeviceAudioSpeaker.lastFrameCount = g_UsbDeviceAudioSpeaker.currentFrameCount;

    totalFrameValue += frameDistance;

    if (1U == g_UsbDeviceAudioSpeaker.stopFeedbackUpdate)
    {
        return;
    }
    if (1U == g_UsbDeviceAudioSpeaker.feedbackDiscardFlag)
    {
        if (0 != g_UsbDeviceAudioSpeaker.feedbackDiscardTimes)
        {
            g_UsbDeviceAudioSpeaker.feedbackDiscardTimes--;
            if (0 != g_UsbDeviceAudioSpeaker.lastFeedbackValue)
            {
                AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, g_UsbDeviceAudioSpeaker.lastFeedbackValue);
            }
            return;
        }
        else
        {
            g_UsbDeviceAudioSpeaker.feedbackDiscardFlag = 0;
        }
    }

    if (USB_SPEED_HIGH == g_UsbDeviceAudioSpeaker.speed)
    {
#if ((defined(USB_AUDIO_CHANNEL7_1) && (USB_AUDIO_CHANNEL7_1 > 0U)) || \
     (defined(USB_AUDIO_CHANNEL5_1) && (USB_AUDIO_CHANNEL5_1 > 0U)) || \
     (defined(USB_AUDIO_CHANNEL3_1) && (USB_AUDIO_CHANNEL3_1 > 0U)))
#if (defined(AUDIO_SPEAKER_MULTI_CHANNEL_PLAY) && (AUDIO_SPEAKER_MULTI_CHANNEL_PLAY > 0U))
        feedbackValue =
            (uint32_t)(((((uint64_t)g_UsbDeviceAudioSpeaker.audioSendCount[1]) << 32U) |
                        g_UsbDeviceAudioSpeaker.audioSendCount[0]) *
                       1024UL * 8UL / totalFrameValue / ((uint64_t)(DEMO_TDM_AUDIO_OUTPUT_CHANNEL_COUNT * 4U)));
#else
        feedbackValue = (uint32_t)(((((uint64_t)g_UsbDeviceAudioSpeaker.audioSendCount[1]) << 32U) |
                                    g_UsbDeviceAudioSpeaker.audioSendCount[0]) *
                                   1024UL * 8UL / totalFrameValue / ((uint64_t)(2U * AUDIO_FORMAT_SIZE)));
#endif
#else
        feedbackValue =
            (uint32_t)(((((uint64_t)g_UsbDeviceAudioSpeaker.audioSendCount[1]) << 32U) |
                        g_UsbDeviceAudioSpeaker.audioSendCount[0]) *
                       1024UL * 8UL / totalFrameValue / ((uint64_t)(AUDIO_FORMAT_CHANNELS * AUDIO_FORMAT_SIZE)));
#endif
    }
    else
    {
#if ((defined(USB_AUDIO_CHANNEL7_1) && (USB_AUDIO_CHANNEL7_1 > 0U)) || \
     (defined(USB_AUDIO_CHANNEL5_1) && (USB_AUDIO_CHANNEL5_1 > 0U)) || \
     (defined(USB_AUDIO_CHANNEL3_1) && (USB_AUDIO_CHANNEL3_1 > 0U)))
#if (defined(AUDIO_SPEAKER_MULTI_CHANNEL_PLAY) && (AUDIO_SPEAKER_MULTI_CHANNEL_PLAY > 0U))
        feedbackValue = (uint32_t)(((((uint64_t)g_UsbDeviceAudioSpeaker.audioSendCount[1]) << 32U) |
                                    g_UsbDeviceAudioSpeaker.audioSendCount[0]) *
                                   1024UL / totalFrameValue / ((uint64_t)(DEMO_TDM_AUDIO_OUTPUT_CHANNEL_COUNT * 4U)));
#else
        feedbackValue = (uint32_t)(((((uint64_t)g_UsbDeviceAudioSpeaker.audioSendCount[1]) << 32U) |
                                    g_UsbDeviceAudioSpeaker.audioSendCount[0]) *
                                   1024UL / totalFrameValue / ((uint64_t)(2U * AUDIO_FORMAT_SIZE)));
#endif
#else
        feedbackValue = (uint32_t)(((((uint64_t)g_UsbDeviceAudioSpeaker.audioSendCount[1]) << 32U) |
                                    g_UsbDeviceAudioSpeaker.audioSendCount[0]) *
                                   1024UL / totalFrameValue / ((uint64_t)(AUDIO_FORMAT_CHANNELS * AUDIO_FORMAT_SIZE)));
#endif
    }

    audioSpeakerUsedSpace = USB_AudioSpeakerBufferSpaceUsed();
    if (audioSpeakerUsedSpace <=
        (g_UsbDeviceAudioSpeaker.audioPlayTransferSize * USB_AUDIO_PLAY_BUFFER_FEEDBACK_TOLERANCE_THRESHOLD))
    {
        feedbackValue += AUDIO_ADJUST_MIN_STEP;
    }

    if ((audioSpeakerUsedSpace +
         (g_UsbDeviceAudioSpeaker.audioPlayTransferSize * USB_AUDIO_PLAY_BUFFER_FEEDBACK_TOLERANCE_THRESHOLD)) >=
        g_UsbDeviceAudioSpeaker.audioPlayBufferSize)
    {
        feedbackValue -= AUDIO_ADJUST_MIN_STEP;
    }
    g_UsbDeviceAudioSpeaker.lastFeedbackValue = feedbackValue;
    AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, feedbackValue);
}
#endif

/* The USB_AudioSpeakerPutBuffer() function fills the audioPlayDataBuff with audioPlayPacket in every callback*/
void USB_AudioSpeakerPutBuffer(uint8_t *buffer, uint32_t size)
{
    uint32_t remainBufferSpace;
    uint32_t audioSpeakerPreWriteDataCount;

#if ((defined(USB_AUDIO_CHANNEL7_1) && (USB_AUDIO_CHANNEL7_1 > 0U)) || \
     (defined(USB_AUDIO_CHANNEL5_1) && (USB_AUDIO_CHANNEL5_1 > 0U)) || \
     (defined(USB_AUDIO_CHANNEL3_1) && (USB_AUDIO_CHANNEL3_1 > 0U)))
#if (defined(AUDIO_SPEAKER_MULTI_CHANNEL_PLAY) && (AUDIO_SPEAKER_MULTI_CHANNEL_PLAY > 0U))
    uint8_t empty_channel = DEMO_TDM_AUDIO_OUTPUT_CHANNEL_COUNT - AUDIO_FORMAT_CHANNELS;
    uint8_t *buffer_temp;
    uint32_t buffer_length = 0;
    uint8_t fill_zero      = 1U;
    uint8_t channel_data   = 0U;
    uint32_t total_data    = 0U;

    remainBufferSpace = g_UsbDeviceAudioSpeaker.audioPlayBufferSize - USB_AudioSpeakerBufferSpaceUsed();
    if (size > remainBufferSpace) /* discard the overflow data */
    {
        if (remainBufferSpace > (AUDIO_FORMAT_CHANNELS * AUDIO_FORMAT_SIZE))
        {
            size = (remainBufferSpace - (AUDIO_FORMAT_CHANNELS * AUDIO_FORMAT_SIZE));
        }
        else
        {
            size = 0;
        }
    }
    for (uint32_t i = 0; i < size; i += (AUDIO_FORMAT_SIZE * AUDIO_FORMAT_CHANNELS))
    {
        buffer_temp  = buffer + i;
        fill_zero    = 1U;
        channel_data = 0U;
        for (uint32_t j = 0; j < (AUDIO_FORMAT_SIZE * AUDIO_FORMAT_CHANNELS); j++)
        {
            /* make up 32 bits */
            if (1U == fill_zero)
            {
                for (uint8_t k = 0U; k < (4U - AUDIO_FORMAT_SIZE); k++)
                {
                    audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay] = 0;
                    g_UsbDeviceAudioSpeaker.tdWriteNumberPlay++;
                    total_data++;
                    if (g_UsbDeviceAudioSpeaker.tdWriteNumberPlay >= g_UsbDeviceAudioSpeaker.audioPlayBufferSize)
                    {
                        g_UsbDeviceAudioSpeaker.tdWriteNumberPlay = 0;
                    }
                }
                fill_zero = 0;
            }
            audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay] = *(buffer_temp + j);
            g_UsbDeviceAudioSpeaker.tdWriteNumberPlay++;
            total_data++;
            if (g_UsbDeviceAudioSpeaker.tdWriteNumberPlay >= g_UsbDeviceAudioSpeaker.audioPlayBufferSize)
            {
                g_UsbDeviceAudioSpeaker.tdWriteNumberPlay = 0;
            }

            if (4U != AUDIO_FORMAT_SIZE)
            {
                channel_data++;
                if (channel_data == AUDIO_FORMAT_SIZE)
                {
                    fill_zero    = 1U;
                    channel_data = 0U;
                }
            }
        }
        /* if totoal channel is not 8, fill 0 to meet 8 channels for TDM */
        if (empty_channel)
        {
            buffer_length = g_UsbDeviceAudioSpeaker.tdWriteNumberPlay + (4U * empty_channel);
            if (buffer_length < g_UsbDeviceAudioSpeaker.audioPlayBufferSize)
            {
                memset((void *)(&audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay]), 0U, 4U * empty_channel);
                g_UsbDeviceAudioSpeaker.tdWriteNumberPlay += (4U * empty_channel);
                total_data += (4U * empty_channel);
            }
            else
            {
                uint32_t firstLength =
                    g_UsbDeviceAudioSpeaker.audioPlayBufferSize - g_UsbDeviceAudioSpeaker.tdWriteNumberPlay;
                memset((void *)(&audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay]), 0U, firstLength);
                total_data += firstLength;
                buffer_length = (4U * empty_channel) - firstLength; /* the remain data length */
                if ((buffer_length) > 0U)
                {
                    memset((void *)(&audioPlayDataBuff[0]), 0U, buffer_length);
                    g_UsbDeviceAudioSpeaker.tdWriteNumberPlay = buffer_length;
                    total_data += buffer_length;
                }
                else
                {
                    g_UsbDeviceAudioSpeaker.tdWriteNumberPlay = 0;
                }
            }
        }
    }
    audioSpeakerPreWriteDataCount = g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0];
    g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0] += total_data;
    if (audioSpeakerPreWriteDataCount > g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0])
    {
        g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[1] += 1U;
    }
#else
    uint8_t *buffer_temp;
    uint32_t play2ChannelLength;
    remainBufferSpace = g_UsbDeviceAudioSpeaker.audioPlayBufferSize - USB_AudioSpeakerBufferSpaceUsed();
    if ((size % (AUDIO_FORMAT_SIZE * AUDIO_FORMAT_CHANNELS)) != 0U)
    {
        size = (size / (AUDIO_FORMAT_SIZE * AUDIO_FORMAT_CHANNELS)) * (AUDIO_FORMAT_SIZE * AUDIO_FORMAT_CHANNELS);
    }
    play2ChannelLength = size / (AUDIO_FORMAT_CHANNELS / 2U); /* only play the selected two channels */
    if (play2ChannelLength > remainBufferSpace)               /* discard the overflow data */
    {
        play2ChannelLength = remainBufferSpace;
    }
    for (uint32_t i = 0; i < size; i += AUDIO_FORMAT_SIZE * AUDIO_FORMAT_CHANNELS)
    {
        buffer_temp = buffer + i;
        if (play2ChannelLength <= 0U)
        {
            break;
        }
        for (uint32_t j = 0; j < AUDIO_FORMAT_SIZE * 2; j++)
        {
            if (play2ChannelLength)
            {
#if ((defined(USB_AUDIO_7_1_CHANNEL_PAIR_SEL) && (0x01 == USB_AUDIO_7_1_CHANNEL_PAIR_SEL)) || \
     (defined(USB_AUDIO_5_1_CHANNEL_PAIR_SEL) && (0x01 == USB_AUDIO_5_1_CHANNEL_PAIR_SEL)) || \
     (defined(USB_AUDIO_3_1_CHANNEL_PAIR_SEL) && (0x01 == USB_AUDIO_3_1_CHANNEL_PAIR_SEL)))
                audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay] = *(buffer_temp + j);
#endif
#if ((defined(USB_AUDIO_7_1_CHANNEL_PAIR_SEL) && (0x02 == USB_AUDIO_7_1_CHANNEL_PAIR_SEL)) || \
     (defined(USB_AUDIO_5_1_CHANNEL_PAIR_SEL) && (0x02 == USB_AUDIO_5_1_CHANNEL_PAIR_SEL)) || \
     (defined(USB_AUDIO_3_1_CHANNEL_PAIR_SEL) && (0x02 == USB_AUDIO_3_1_CHANNEL_PAIR_SEL)))
                audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay] =
                    *(buffer_temp + j + AUDIO_FORMAT_SIZE * 2);
#endif
#if ((defined(USB_AUDIO_7_1_CHANNEL_PAIR_SEL) && (0x03 == USB_AUDIO_7_1_CHANNEL_PAIR_SEL)) || \
     (defined(USB_AUDIO_5_1_CHANNEL_PAIR_SEL) && (0x03 == USB_AUDIO_5_1_CHANNEL_PAIR_SEL)))
                audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay] =
                    *(buffer_temp + j + AUDIO_FORMAT_SIZE * 4);
#elif (defined(USB_AUDIO_7_1_CHANNEL_PAIR_SEL) && (0x04 == USB_AUDIO_7_1_CHANNEL_PAIR_SEL))
                audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay] =
                    *(buffer_temp + j + AUDIO_FORMAT_SIZE * 6);
#endif
                play2ChannelLength--;
                g_UsbDeviceAudioSpeaker.tdWriteNumberPlay++;
                if (g_UsbDeviceAudioSpeaker.tdWriteNumberPlay >= g_UsbDeviceAudioSpeaker.audioPlayBufferSize)
                {
                    g_UsbDeviceAudioSpeaker.tdWriteNumberPlay = 0;
                }
            }
            else
            {
                break;
            }
        }
    }
    audioSpeakerPreWriteDataCount = g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0];
    g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0] += (size / (AUDIO_FORMAT_CHANNELS / 2U));
    if (audioSpeakerPreWriteDataCount > g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0])
    {
        g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[1] += 1U;
    }
#endif /* AUDIO_SPEAKER_MULTI_CHANNEL_PLAY */
#else
    uint32_t buffer_length = 0;
    remainBufferSpace      = g_UsbDeviceAudioSpeaker.audioPlayBufferSize - USB_AudioSpeakerBufferSpaceUsed();
    if (size > remainBufferSpace) /* discard the overflow data */
    {
        if (remainBufferSpace > (AUDIO_FORMAT_CHANNELS * AUDIO_FORMAT_SIZE))
        {
            size = (remainBufferSpace - (AUDIO_FORMAT_CHANNELS * AUDIO_FORMAT_SIZE));
        }
        else
        {
            size = 0;
        }
    }
    if (size > 0)
    {
        buffer_length = g_UsbDeviceAudioSpeaker.tdWriteNumberPlay + size;
        if (buffer_length < g_UsbDeviceAudioSpeaker.audioPlayBufferSize)
        {
            memcpy((void *)(&audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay]), (void *)(&buffer[0]), size);
            g_UsbDeviceAudioSpeaker.tdWriteNumberPlay += size;
        }
        else
        {
            uint32_t firstLength =
                g_UsbDeviceAudioSpeaker.audioPlayBufferSize - g_UsbDeviceAudioSpeaker.tdWriteNumberPlay;
            memcpy((void *)(&audioPlayDataBuff[g_UsbDeviceAudioSpeaker.tdWriteNumberPlay]), (void *)(&buffer[0]),
                   firstLength);
            buffer_length = size - firstLength; /* the remain data length */
            if ((buffer_length) > 0U)
            {
                memcpy((void *)(&audioPlayDataBuff[0]), (void *)((uint8_t *)(&buffer[0]) + firstLength), buffer_length);
                g_UsbDeviceAudioSpeaker.tdWriteNumberPlay = buffer_length;
            }
            else
            {
                g_UsbDeviceAudioSpeaker.tdWriteNumberPlay = 0;
            }
        }
    }
    audioSpeakerPreWriteDataCount = g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0];
    g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0] += size;
    if (audioSpeakerPreWriteDataCount > g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0])
    {
        g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[1] += 1U;
    }
#endif
}

/*!
 * @brief Audio class specific callback function.
 *
 * This function handles the Audio class specific requests.
 *
 * @param handle		  The Audio class handle.
 * @param event 		  The Audio class event type.
 * @param param 		  The parameter of the class specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */

usb_status_t USB_DeviceAudioCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_InvalidRequest;
    usb_device_endpoint_callback_message_struct_t *ep_cb_param;
    ep_cb_param = (usb_device_endpoint_callback_message_struct_t *)param;

    switch (event)
    {
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
        case kUSB_DeviceAudioEventStreamSendResponse:
            /* endpoint callback length is USB_CANCELLED_TRANSFER_LENGTH (0xFFFFFFFFU) when transfer is canceled */
            if ((g_UsbDeviceAudioSpeaker.attach) && (ep_cb_param->length != (USB_CANCELLED_TRANSFER_LENGTH)))
            {
                if (ep_cb_param->length == g_UsbDeviceAudioSpeaker.currentFeedbackMaxPacketSize)
                {
                    if (!feedbackValueUpdating)
                    {
                        *((uint32_t *)&usbAudioFeedBackBuffer[0]) = *((uint32_t *)&audioFeedBackBuffer[0]);
                    }
                    error = USB_DeviceAudioSend(handle, USB_AUDIO_SPEAKER_FEEDBACK_ENDPOINT, usbAudioFeedBackBuffer,
                                                g_UsbDeviceAudioSpeaker.currentFeedbackMaxPacketSize);
                }
            }
            break;
#endif
        case kUSB_DeviceAudioEventStreamRecvResponse:
            /* endpoint callback length is USB_CANCELLED_TRANSFER_LENGTH (0xFFFFFFFFU) when transfer is canceled */
            if ((g_UsbDeviceAudioSpeaker.attach) && (ep_cb_param->length != (USB_CANCELLED_TRANSFER_LENGTH)))
            {
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#if (defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)) || \
    (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
                if (USB_SPEED_HIGH ==
                    g_UsbDeviceAudioSpeaker.speed) /* high speed and audio 2.0, use low latency solution  */
                {
                    if (g_UsbDeviceAudioSpeaker.tdWriteNumberPlay >=
                        (g_UsbDeviceAudioSpeaker.audioPlayTransferSize * AUDIO_CLASS_2_0_HS_LOW_LATENCY_TRANSFER_COUNT))
                    {
                        g_UsbDeviceAudioSpeaker.startPlayFlag = 1;
                    }
#if (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
#if (1U == HS_ISO_OUT_ENDP_INTERVAL)
                    if (s_microFrameCountIp3511HS < 7U)
                    {
                        s_microFrameCountIp3511HS++;
                    }
                    else
                    {
                        s_microFrameCountIp3511HS = 0U;
                    }
#elif (2U == HS_ISO_OUT_ENDP_INTERVAL)
                    if (s_microFrameCountIp3511HS < 6U)
                    {
                        s_microFrameCountIp3511HS += 2U;
                    }
                    else
                    {
                        s_microFrameCountIp3511HS = 0U;
                    }
#elif (3U == HS_ISO_OUT_ENDP_INTERVAL)
                    if (s_microFrameCountIp3511HS < 4U)
                    {
                        s_microFrameCountIp3511HS += 4U;
                    }
                    else
                    {
                        s_microFrameCountIp3511HS = 0U;
                    }
#else
                    s_microFrameCountIp3511HS = 0;
#endif
#endif
                }
                else
                {
                    if ((g_UsbDeviceAudioSpeaker.tdWriteNumberPlay >=
                         (g_UsbDeviceAudioSpeaker.audioPlayBufferSize / 2U)) &&
                        (g_UsbDeviceAudioSpeaker.startPlayFlag == 0))
                    {
                        g_UsbDeviceAudioSpeaker.startPlayFlag = 1;
                    }
                }
#else
                if ((g_UsbDeviceAudioSpeaker.tdWriteNumberPlay >= (g_UsbDeviceAudioSpeaker.audioPlayBufferSize / 2U)) &&
                    (g_UsbDeviceAudioSpeaker.startPlayFlag == 0))
                {
                    g_UsbDeviceAudioSpeaker.startPlayFlag = 1;
                }
#endif /* USB_DEVICE_CONFIG_EHCI, USB_DEVICE_CONFIG_LPCIP3511HS */
#else
                if ((g_UsbDeviceAudioSpeaker.tdWriteNumberPlay >= (g_UsbDeviceAudioSpeaker.audioPlayBufferSize / 2U)) &&
                    (g_UsbDeviceAudioSpeaker.startPlayFlag == 0))
                {
                    g_UsbDeviceAudioSpeaker.startPlayFlag = 1;
                }
#endif /* USB_DEVICE_CONFIG_AUDIO_CLASS_2_0 */
                USB_AudioSpeakerPutBuffer(audioPlayPacket, ep_cb_param->length);
                g_UsbDeviceAudioSpeaker.usbRecvCount += ep_cb_param->length;
                g_UsbDeviceAudioSpeaker.usbRecvTimes++;
                error = USB_DeviceAudioRecv(handle, USB_AUDIO_SPEAKER_STREAM_ENDPOINT, &audioPlayPacket[0],
                                            g_UsbDeviceAudioSpeaker.currentStreamOutMaxPacketSize);
            }
            break;

        default:
            if (param && (event > 0xFF))
            {
                error = USB_DeviceAudioRequest(handle, event, param);
            }
            break;
    }

    return error;
}

/* The USB_DeviceAudioSpeakerStatusReset() function resets the audio speaker status to the initialized status */
void USB_DeviceAudioSpeakerStatusReset(void)
{
    g_UsbDeviceAudioSpeaker.startPlayFlag                 = 0;
    g_UsbDeviceAudioSpeaker.tdWriteNumberPlay             = 0;
    g_UsbDeviceAudioSpeaker.tdReadNumberPlay              = 0;
    g_UsbDeviceAudioSpeaker.audioSendCount[0]             = 0;
    g_UsbDeviceAudioSpeaker.audioSendCount[1]             = 0;
    g_UsbDeviceAudioSpeaker.audioSpeakerReadDataCount[0]  = 0;
    g_UsbDeviceAudioSpeaker.audioSpeakerReadDataCount[1]  = 0;
    g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[0] = 0;
    g_UsbDeviceAudioSpeaker.audioSpeakerWriteDataCount[1] = 0;
    g_UsbDeviceAudioSpeaker.usbRecvCount                  = 0;
    g_UsbDeviceAudioSpeaker.audioSendTimes                = 0;
    g_UsbDeviceAudioSpeaker.usbRecvTimes                  = 0;
    g_UsbDeviceAudioSpeaker.speakerIntervalCount          = 0;
    g_UsbDeviceAudioSpeaker.speakerReservedSpace          = 0;
    g_UsbDeviceAudioSpeaker.speakerDetachOrNoInput        = 0;
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
    g_UsbDeviceAudioSpeaker.audioPllTicksPrev = 0U;
#if ((defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)) || \
     (defined(USB_DEVICE_CONFIG_KHCI) && (USB_DEVICE_CONFIG_KHCI > 0U)))
    g_UsbDeviceAudioSpeaker.usbFroTicksPrev          = 0U;
    g_UsbDeviceAudioSpeaker.usbFroTicksEma           = AUDIO_FRO_USB_SOF_INTERVAL_TICK_COUNT;
    g_UsbDeviceAudioSpeaker.usbFroTickEmaFrac        = 0U;
    g_UsbDeviceAudioSpeaker.usbFroTickBasedPrecision = AUDIO_USB_FRO_TRIM_TICK_BASED_PRECISION;
#endif
    g_UsbDeviceAudioSpeaker.audioPllTicksEma           = AUDIO_PLL_USB_SOF_INTERVAL_TICK_COUNT;
    g_UsbDeviceAudioSpeaker.audioPllTickEmaFrac        = 0U;
    g_UsbDeviceAudioSpeaker.audioPllTickBasedPrecision = AUDIO_PLL_FRACTION_TICK_BASED_PRECISION;
    g_UsbDeviceAudioSpeaker.stopDataLengthAudioAdjust  = 0U;
#else
    g_UsbDeviceAudioSpeaker.firstCalculateFeedback = 0U;
    g_UsbDeviceAudioSpeaker.lastFrameCount         = 0U;
    g_UsbDeviceAudioSpeaker.currentFrameCount      = 0U;
    g_UsbDeviceAudioSpeaker.feedbackDiscardFlag    = 0U;
    g_UsbDeviceAudioSpeaker.feedbackDiscardTimes   = AUDIO_SPEAKER_FEEDBACK_DISCARD_COUNT;

    /* use the last saved feedback value */
    if (g_UsbDeviceAudioSpeaker.lastFeedbackValue)
    {
        AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, g_UsbDeviceAudioSpeaker.lastFeedbackValue);
    }
#endif
}

/*!
 * @brief USB device callback function.
 *
 * This function handles the usb device specific requests.
 *
 * @param handle		  The USB device handle.
 * @param event 		  The USB device event type.
 * @param param 		  The parameter of the device specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_InvalidRequest;
    uint8_t *temp8     = (uint8_t *)param;
    uint16_t *temp16   = (uint16_t *)param;
    uint8_t count      = 0U;

    switch (event)
    {
        case kUSB_DeviceEventBusReset:
        {
            for (count = 0U; count < USB_AUDIO_SPEAKER_INTERFACE_COUNT; count++)
            {
                g_UsbDeviceAudioSpeaker.currentInterfaceAlternateSetting[count] = 0U;
            }
            /* reset audio speaker status to be the initialized status */
            USB_DeviceAudioSpeakerStatusReset();
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
            /* reset the the last feedback value */
            g_UsbDeviceAudioSpeaker.lastFeedbackValue = 0U;
#endif
            g_UsbDeviceAudioSpeaker.attach               = 0U;
            g_UsbDeviceAudioSpeaker.currentConfiguration = 0U;
            error                                        = kStatus_USB_Success;

#if (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
#if !((defined FSL_FEATURE_SOC_USBPHY_COUNT) && (FSL_FEATURE_SOC_USBPHY_COUNT > 0U))
            /* The work-around is used to fix the HS device Chirping issue.
             * Please refer to the implementation for the detail information.
             */
            USB_DeviceHsPhyChirpIssueWorkaround();
#endif
#endif

#if (defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)) || \
    (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
            /* Get USB speed to configure the device, including max packet size and interval of the endpoints. */
            if (kStatus_USB_Success == USB_DeviceClassGetSpeed(CONTROLLER_ID, &g_UsbDeviceAudioSpeaker.speed))
            {
                USB_DeviceSetSpeed(handle, g_UsbDeviceAudioSpeaker.speed);
            }
            if (USB_SPEED_HIGH == g_UsbDeviceAudioSpeaker.speed)
            {
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
                g_UsbDeviceAudioSpeaker.currentStreamOutMaxPacketSize = (HS_ISO_OUT_ENDP_PACKET_SIZE);
#else
                g_UsbDeviceAudioSpeaker.currentStreamOutMaxPacketSize =
                    (HS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_FORMAT_CHANNELS * AUDIO_FORMAT_SIZE);
                g_UsbDeviceAudioSpeaker.currentFeedbackMaxPacketSize = HS_ISO_FEEDBACK_ENDP_PACKET_SIZE;
#endif /* USB_DEVICE_AUDIO_USE_SYNC_MODE */
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
                AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_TO_16_16);
#endif
                /* high speed and audio 2.0, audio play interval is set by HS EP packet size */
#if ((defined(USB_AUDIO_CHANNEL7_1) && (USB_AUDIO_CHANNEL7_1 > 0U)) || \
     (defined(USB_AUDIO_CHANNEL5_1) && (USB_AUDIO_CHANNEL5_1 > 0U)) || \
     (defined(USB_AUDIO_CHANNEL3_1) && (USB_AUDIO_CHANNEL3_1 > 0U)))
#if defined(AUDIO_SPEAKER_MULTI_CHANNEL_PLAY) && (AUDIO_SPEAKER_MULTI_CHANNEL_PLAY > 0U)
                g_UsbDeviceAudioSpeaker.audioPlayTransferSize =
                    (HS_ISO_OUT_ENDP_PACKET_SIZE / AUDIO_FORMAT_SIZE / AUDIO_FORMAT_CHANNELS) * 4U *
                    8U; /* TDM accepts 32-bit width */
#else
                g_UsbDeviceAudioSpeaker.audioPlayTransferSize =
                    HS_ISO_OUT_ENDP_PACKET_SIZE / (AUDIO_FORMAT_CHANNELS / 2U);
#endif
#else
                g_UsbDeviceAudioSpeaker.audioPlayTransferSize = HS_ISO_OUT_ENDP_PACKET_SIZE;
#endif
                /* use short play buffer size, only use two elements */
                g_UsbDeviceAudioSpeaker.audioPlayBufferSize =
                    AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME * AUDIO_SPEAKER_DATA_WHOLE_BUFFER_COUNT;
#else
                /* high speed and audio 1.0, audio play interval is 1 ms using this play size */
                g_UsbDeviceAudioSpeaker.audioPlayTransferSize = AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME;
                /* use the whole play buffer size */
                g_UsbDeviceAudioSpeaker.audioPlayBufferSize =
                    AUDIO_SPEAKER_DATA_WHOLE_BUFFER_COUNT_NORMAL * AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME;
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
                AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_TO_10_14);
#endif
#endif /* USB_DEVICE_CONFIG_AUDIO_CLASS_2_0 */
            }
            else
            {
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
                g_UsbDeviceAudioSpeaker.currentFeedbackMaxPacketSize = FS_ISO_FEEDBACK_ENDP_PACKET_SIZE;
                AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_TO_10_14);
#endif
                /* full speed, audio play interval is 1 ms using this play size */
                g_UsbDeviceAudioSpeaker.audioPlayTransferSize = AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME;
                /* use the whole play buffer size */
                g_UsbDeviceAudioSpeaker.audioPlayBufferSize =
                    AUDIO_SPEAKER_DATA_WHOLE_BUFFER_COUNT_NORMAL * AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME;
            }
#else
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
            g_UsbDeviceAudioSpeaker.currentFeedbackMaxPacketSize = FS_ISO_FEEDBACK_ENDP_PACKET_SIZE;
            AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_TO_10_14);
#endif
            /* full speed, audio play interval is 1 ms using this play size */
            g_UsbDeviceAudioSpeaker.audioPlayTransferSize = AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME;
            /* use the whole play buffer size */
            g_UsbDeviceAudioSpeaker.audioPlayBufferSize =
                AUDIO_SPEAKER_DATA_WHOLE_BUFFER_COUNT_NORMAL * AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME;
#endif /* USB_DEVICE_CONFIG_EHCI, USB_DEVICE_CONFIG_LPCIP3511HS */
        }
        break;
#if (defined(USB_DEVICE_CONFIG_DETACH_ENABLE) && (USB_DEVICE_CONFIG_DETACH_ENABLE > 0U))
        case kUSB_DeviceEventDetach:
        {
#if (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
#if !((defined FSL_FEATURE_SOC_USBPHY_COUNT) && (FSL_FEATURE_SOC_USBPHY_COUNT > 0U))
            USB_DeviceDisconnected();
#endif
#endif
            error = kStatus_USB_Success;
        }
        break;
#endif
        case kUSB_DeviceEventSetConfiguration:
            if (0U == (*temp8))
            {
                g_UsbDeviceAudioSpeaker.attach               = 0U;
                g_UsbDeviceAudioSpeaker.currentConfiguration = 0U;
                error                                        = kStatus_USB_Success;
            }
            else if (USB_AUDIO_SPEAKER_CONFIGURE_INDEX == (*temp8))
            {
                g_UsbDeviceAudioSpeaker.attach               = 1U;
                g_UsbDeviceAudioSpeaker.currentConfiguration = *temp8;
                error                                        = kStatus_USB_Success;
            }
            else
            {
                /* no action, invalid request */
            }
            break;
        case kUSB_DeviceEventSetInterface:
            if (g_UsbDeviceAudioSpeaker.attach)
            {
                uint8_t interface        = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FFU);

                if (USB_AUDIO_SPEAKER_STREAM_INTERFACE_INDEX == interface)
                {
                    if (alternateSetting < USB_AUDIO_SPEAKER_STREAM_INTERFACE_ALTERNATE_COUNT)
                    {
                        if (g_UsbDeviceAudioSpeaker.currentInterfaceAlternateSetting[interface] == alternateSetting)
                        {
                            error = kStatus_USB_Success;
                            break;
                        }
                        if (USB_AUDIO_SPEAKER_STREAM_INTERFACE_ALTERNATE_1 == alternateSetting)
                        {
                            error = USB_DeviceAudioRecv(g_UsbDeviceAudioSpeaker.audioHandle,
                                                        USB_AUDIO_SPEAKER_STREAM_ENDPOINT, &audioPlayDataBuff[0],
                                                        g_UsbDeviceAudioSpeaker.currentStreamOutMaxPacketSize);
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
                            g_UsbDeviceAudioSpeaker.stopDataLengthAudioAdjust = 0U;
#else
                            if (!feedbackValueUpdating)
                            {
                                *((uint32_t *)&usbAudioFeedBackBuffer[0]) = *((uint32_t *)&audioFeedBackBuffer[0]);
                            }
                            USB_DeviceAudioSend(g_UsbDeviceAudioSpeaker.audioHandle,
                                                USB_AUDIO_SPEAKER_FEEDBACK_ENDPOINT, usbAudioFeedBackBuffer,
                                                g_UsbDeviceAudioSpeaker.currentFeedbackMaxPacketSize);
#endif
                        }
                        else
                        {
                            error = USB_DeviceDeinitEndpoint(
                                g_UsbDeviceAudioSpeaker.deviceHandle,
                                USB_AUDIO_SPEAKER_STREAM_ENDPOINT |
                                    (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT));
                        }
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
                        /* USB transfer stops so stop adjusting PLL using data length */
                        if (USB_AUDIO_SPEAKER_STREAM_INTERFACE_ALTERNATE_0 == alternateSetting)
                        {
                            g_UsbDeviceAudioSpeaker.stopDataLengthAudioAdjust = 1U;
                        }
#else
                        /* usb host stops the speaker, so there is no need for feedback */
                        if ((1U == g_UsbDeviceAudioSpeaker.startPlayFlag) &&
                            (USB_AUDIO_SPEAKER_STREAM_INTERFACE_ALTERNATE_0 == alternateSetting))
                        {
                            g_UsbDeviceAudioSpeaker.stopFeedbackUpdate = 1U;
                        }

                        /* usb host start the speaker, discard the feedback for AUDIO_SPEAKER_FEEDBACK_DISCARD_COUNT
                         * times */
                        if (USB_AUDIO_SPEAKER_STREAM_INTERFACE_ALTERNATE_1 == alternateSetting)
                        {
                            g_UsbDeviceAudioSpeaker.stopFeedbackUpdate   = 0U;
                            g_UsbDeviceAudioSpeaker.feedbackDiscardFlag  = 1U;
                            g_UsbDeviceAudioSpeaker.feedbackDiscardTimes = AUDIO_SPEAKER_FEEDBACK_DISCARD_COUNT;
                        }
#endif
                        g_UsbDeviceAudioSpeaker.currentInterfaceAlternateSetting[interface] = alternateSetting;
                    }
                }
                else if (USB_AUDIO_CONTROL_INTERFACE_INDEX == interface)
                {
                    if (alternateSetting < USB_AUDIO_CONTROL_INTERFACE_ALTERNATE_COUNT)
                    {
                        g_UsbDeviceAudioSpeaker.currentInterfaceAlternateSetting[interface] = alternateSetting;
                        error                                                               = kStatus_USB_Success;
                    }
                }
                else
                {
                    /* no action, invalid request */
                }
            }
            break;
        case kUSB_DeviceEventGetConfiguration:
            if (param)
            {
                *temp8 = g_UsbDeviceAudioSpeaker.currentConfiguration;
                error  = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventGetInterface:
            if (param)
            {
                /* Get current alternate setting of the interface request */
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                if (interface < USB_AUDIO_SPEAKER_INTERFACE_COUNT)
                {
                    *temp16 = (*temp16 & 0xFF00U) | g_UsbDeviceAudioSpeaker.currentInterfaceAlternateSetting[interface];
                    error   = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetDeviceDescriptor:
            if (param)
            {
                /* Get device descriptor request */
                error = USB_DeviceGetDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetConfigurationDescriptor:
            if (param)
            {
                /* Get device configuration descriptor request */
                error = USB_DeviceGetConfigurationDescriptor(handle,
                                                             (usb_device_get_configuration_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetStringDescriptor:
            if (param)
            {
                /* Get device string descriptor request */
                error = USB_DeviceGetStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
            }
            break;
        default:
            break;
    }

    return error;
}

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#if (((defined USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS)) || \
     ((defined USB_DEVICE_CONFIG_KHCI) && (USB_DEVICE_CONFIG_KHCI)))
void CTIMER_SOF_TOGGLE_HANDLER_FRO(uint32_t i)
{
    uint32_t currentCtCap = 0, pllCountPeriod = 0;
    uint32_t usedSpace      = 0;
    static int32_t pllCount = 0, pllDiff = 0;
    static int32_t err, abs_err;
    uint32_t up_change                 = 0;
    uint32_t down_change               = 0;
    static uint32_t delay_adj_up       = 0;
    static uint32_t delay_adj_down     = 0;
    static uint32_t FroPreUsbRecvCount = 0U;

    if (CTIMER_GetStatusFlags(CTIMER1) & (1 << 4U))
    {
        /* Clear interrupt flag.*/
        CTIMER_ClearStatusFlags(CTIMER1, (1 << 4U));
    }

    if (g_UsbDeviceAudioSpeaker.froTrimIntervalCount != AUDIO_FRO_ADJUST_INTERVAL)
    {
        g_UsbDeviceAudioSpeaker.froTrimIntervalCount++;
        return;
    }

    g_UsbDeviceAudioSpeaker.froTrimIntervalCount = 1;
    currentCtCap                                 = CTIMER1->CR[0];
    pllCountPeriod                               = currentCtCap - g_UsbDeviceAudioSpeaker.usbFroTicksPrev;
    g_UsbDeviceAudioSpeaker.usbFroTicksPrev      = currentCtCap;
    pllCount                                     = pllCountPeriod;

    if (g_UsbDeviceAudioSpeaker.attach)
    {
        if (abs(pllCount - AUDIO_FRO_USB_SOF_INTERVAL_TICK_COUNT) < AUDIO_FRO_USB_SOF_INTERVAL_VALID_DEVIATION)
        {
            pllDiff = pllCount - g_UsbDeviceAudioSpeaker.usbFroTicksEma;
            g_UsbDeviceAudioSpeaker.usbFroTickEmaFrac += (pllDiff % 8);
            g_UsbDeviceAudioSpeaker.usbFroTicksEma += (pllDiff / 8) + g_UsbDeviceAudioSpeaker.usbFroTickEmaFrac / 8;
            g_UsbDeviceAudioSpeaker.usbFroTickEmaFrac = (g_UsbDeviceAudioSpeaker.usbFroTickEmaFrac % 8);

            err     = g_UsbDeviceAudioSpeaker.usbFroTicksEma - AUDIO_FRO_USB_SOF_INTERVAL_TICK_COUNT;
            abs_err = abs(err);
            if (abs_err >= g_UsbDeviceAudioSpeaker.usbFroTickBasedPrecision)
            {
                if (err > 0)
                {
                    down_change = 1;
                }
                else
                {
                    up_change = 1;
                }
            }

            if (g_UsbDeviceAudioSpeaker.startPlayFlag)
            {
                /* if USB transfer stops, can not use data length to do adjustment */
                if (0U == g_UsbDeviceAudioSpeaker.stopDataLengthAudioAdjust)
                {
                    /* USB is transferring */
                    if (FroPreUsbRecvCount != g_UsbDeviceAudioSpeaker.usbRecvCount)
                    {
                        FroPreUsbRecvCount = g_UsbDeviceAudioSpeaker.usbRecvCount;
                        usedSpace          = USB_AudioSpeakerBufferSpaceUsed();
                        if ((usedSpace + (g_UsbDeviceAudioSpeaker.audioPlayTransferSize *
                                          AUDIO_SYNC_DATA_BASED_ADJUST_THRESHOLD)) >=
                            g_UsbDeviceAudioSpeaker.audioPlayBufferSize)
                        {
                            if (delay_adj_up == 0)
                            {
                                delay_adj_down = 0;
                                delay_adj_up   = AUDIO_FRO_TRIM_DATA_BASED_INTERVAL;
                                up_change      = 1;
                            }
                            else
                            {
                                delay_adj_up--;
                            }
                        }
                        else if (usedSpace <= (g_UsbDeviceAudioSpeaker.audioPlayTransferSize *
                                               AUDIO_SYNC_DATA_BASED_ADJUST_THRESHOLD))
                        {
                            if (delay_adj_down == 0)
                            {
                                delay_adj_up   = 0;
                                delay_adj_down = AUDIO_FRO_TRIM_DATA_BASED_INTERVAL;
                                down_change    = 1;
                            }
                            else
                            {
                                delay_adj_down--;
                            }
                        }
                        else
                        {
                            /* no action */
                        }
                    }
                }
            }
        }

        if (down_change)
        {
            audio_fro_trim_down();
        }
        if (up_change)
        {
            audio_fro_trim_up();
        }
    }
}
#endif /* USB_DEVICE_CONFIG_LPCIP3511FS  USB_DEVICE_CONFIG_KHCI*/

void CTIMER_SOF_TOGGLE_HANDLER_PLL(uint32_t i)
{
    uint32_t currentCtCap = 0, pllCountPeriod = 0, pll_change = 0;
    uint32_t usedSpace      = 0;
    static int32_t pllCount = 0, pllDiff = 0;
    static int32_t err, abs_err;
    static uint32_t delay_adj_up       = 0;
    static uint32_t delay_adj_down     = 0;
    static uint32_t PllPreUsbRecvCount = 0U;

    if (CTIMER_GetStatusFlags(CTIMER0) & (1 << 4U))
    {
        /* Clear interrupt flag.*/
        CTIMER_ClearStatusFlags(CTIMER0, (1 << 4U));
    }

    if (g_UsbDeviceAudioSpeaker.speakerIntervalCount != AUDIO_PLL_ADJUST_INTERVAL)
    {
        g_UsbDeviceAudioSpeaker.speakerIntervalCount++;
        return;
    }

    g_UsbDeviceAudioSpeaker.speakerIntervalCount = 1;
    currentCtCap                                 = CTIMER0->CR[0];
    pllCountPeriod                               = currentCtCap - g_UsbDeviceAudioSpeaker.audioPllTicksPrev;
    g_UsbDeviceAudioSpeaker.audioPllTicksPrev    = currentCtCap;
    pllCount                                     = pllCountPeriod;
    if (g_UsbDeviceAudioSpeaker.attach)
    {
        if (abs(pllCount - AUDIO_PLL_USB_SOF_INTERVAL_TICK_COUNT) < AUDIO_PLL_USB_SOF_INTERVAL_VALID_DEVIATION)
        {
            pllDiff = pllCount - g_UsbDeviceAudioSpeaker.audioPllTicksEma;
            g_UsbDeviceAudioSpeaker.audioPllTickEmaFrac += (pllDiff % 8);
            g_UsbDeviceAudioSpeaker.audioPllTicksEma += (pllDiff / 8) + g_UsbDeviceAudioSpeaker.audioPllTickEmaFrac / 8;
            g_UsbDeviceAudioSpeaker.audioPllTickEmaFrac = (g_UsbDeviceAudioSpeaker.audioPllTickEmaFrac % 8);

            err     = g_UsbDeviceAudioSpeaker.audioPllTicksEma - AUDIO_PLL_USB_SOF_INTERVAL_TICK_COUNT;
            abs_err = abs(err);
            if (abs_err >= g_UsbDeviceAudioSpeaker.audioPllTickBasedPrecision)
            {
                if (err > 0)
                {
                    g_UsbDeviceAudioSpeaker.curAudioPllFrac -=
                        abs_err / g_UsbDeviceAudioSpeaker.audioPllTickBasedPrecision;
                }
                else
                {
                    g_UsbDeviceAudioSpeaker.curAudioPllFrac +=
                        abs_err / g_UsbDeviceAudioSpeaker.audioPllTickBasedPrecision;
                }
                pll_change = 1;
            }

            if (0U != g_UsbDeviceAudioSpeaker.startPlayFlag)
            {
                /* if USB transfer stops, can not use data length to do adjustment */
                if (0U == g_UsbDeviceAudioSpeaker.stopDataLengthAudioAdjust)
                {
                    /* USB is transferring */
                    if (PllPreUsbRecvCount != g_UsbDeviceAudioSpeaker.usbRecvCount)
                    {
                        PllPreUsbRecvCount = g_UsbDeviceAudioSpeaker.usbRecvCount;
                        usedSpace          = USB_AudioSpeakerBufferSpaceUsed();
                        if (usedSpace <=
                            (g_UsbDeviceAudioSpeaker.audioPlayTransferSize * AUDIO_SYNC_DATA_BASED_ADJUST_THRESHOLD))
                        {
                            if (delay_adj_down == 0)
                            {
                                delay_adj_up   = 0;
                                delay_adj_down = AUDIO_PLL_ADJUST_DATA_BASED_INTERVAL;
                                g_UsbDeviceAudioSpeaker.curAudioPllFrac -= AUDIO_PLL_ADJUST_DATA_BASED_STEP;
                                pll_change = 1;
                            }
                            else
                            {
                                delay_adj_down--;
                            }
                        }
                        else if ((usedSpace + (g_UsbDeviceAudioSpeaker.audioPlayTransferSize *
                                               AUDIO_SYNC_DATA_BASED_ADJUST_THRESHOLD)) >=
                                 g_UsbDeviceAudioSpeaker.audioPlayBufferSize)
                        {
                            if (delay_adj_up == 0)
                            {
                                delay_adj_down = 0;
                                delay_adj_up   = AUDIO_PLL_ADJUST_DATA_BASED_INTERVAL;
                                g_UsbDeviceAudioSpeaker.curAudioPllFrac += AUDIO_PLL_ADJUST_DATA_BASED_STEP;
                                pll_change = 1;
                            }
                            else
                            {
                                delay_adj_up--;
                            }
                        }
                    }
                }
            }
        }

        if (pll_change)
        {
            USB_AudioPllChange();
        }
    }
}
#endif /* USB_DEVICE_AUDIO_USE_SYNC_MODE */

/*!
 * @brief Application initialization function.
 *
 * This function initializes the application.
 *
 * @return None.
 */
void APPInit(void)
{
    USB_DeviceClockInit();
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
    SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */
    /*Initialize audio interface and codec.*/
    HAL_AudioTxInit((hal_audio_handle_t)audioTxHandle, &audioTxConfig);
    BOARD_Codec_Init();
    AUDIO_DMA_EDMA_Start();
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
    CTIMER_CaptureInit();
#else
    AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_TO_10_14);
#endif

    if (kStatus_USB_Success !=
        USB_DeviceClassInit(CONTROLLER_ID, &s_audioConfigList, &g_UsbDeviceAudioSpeaker.deviceHandle))
    {
        usb_echo("USB device failed\r\n");
        return;
    }
    else
    {
        usb_echo("USB device audio speaker demo\r\n");
        g_UsbDeviceAudioSpeaker.audioHandle = s_audioConfigList.config->classHandle;
    }

    /* Install isr, set priority, and enable IRQ. */
    USB_DeviceIsrEnable();

    /*Add one delay here to make the DP pull down long enough to allow host to detect the previous disconnection.*/
    SDK_DelayAtLeastUs(5000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    USB_DeviceRun(g_UsbDeviceAudioSpeaker.deviceHandle);
}

void USB_AudioCodecTask(void)
{
    if (g_UsbDeviceAudioSpeaker.codecTask & MUTE_CODEC_TASK)
    {
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
        usb_echo("Set Cur Mute : %x\r\n", g_UsbDeviceAudioSpeaker.curMute20);
#else
        usb_echo("Set Cur Mute : %x\r\n", g_UsbDeviceAudioSpeaker.curMute);
#endif
        g_UsbDeviceAudioSpeaker.codecTask &= ~MUTE_CODEC_TASK;
    }
    if (g_UsbDeviceAudioSpeaker.codecTask & UNMUTE_CODEC_TASK)
    {
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
        usb_echo("Set Cur Mute : %x\r\n", g_UsbDeviceAudioSpeaker.curMute20);
#else
        usb_echo("Set Cur Mute : %x\r\n", g_UsbDeviceAudioSpeaker.curMute);
#endif
        g_UsbDeviceAudioSpeaker.codecTask &= ~UNMUTE_CODEC_TASK;
    }
    if (g_UsbDeviceAudioSpeaker.codecTask & VOLUME_CHANGE_TASK)
    {
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
        usb_echo("Set Cur Volume : %x\r\n",
                 (uint16_t)(g_UsbDeviceAudioSpeaker.curVolume20[1] << 8U) | g_UsbDeviceAudioSpeaker.curVolume20[0]);
#else
        usb_echo("Set Cur Volume : %x\r\n",
                 (uint16_t)(g_UsbDeviceAudioSpeaker.curVolume[1] << 8U) | g_UsbDeviceAudioSpeaker.curVolume[0]);
#endif
        g_UsbDeviceAudioSpeaker.codecTask &= ~VOLUME_CHANGE_TASK;
    }
}

void USB_AudioSpeakerResetTask(void)
{
    if (g_UsbDeviceAudioSpeaker.speakerDetachOrNoInput)
    {
        USB_DeviceAudioSpeakerStatusReset();
    }
}
/*!
 * @brief Application task function.
 *
 * This function runs the task for application.
 *
 * @return None.
 */
#if defined(__CC_ARM) || (defined(__ARMCC_VERSION)) || defined(__GNUC__)
int main(void)
#else
void main(void)
#endif
{
    BOARD_InitHardware();

    APPInit();

    while (1)
    {
        USB_AudioCodecTask();

        USB_AudioSpeakerResetTask();

#if USB_DEVICE_CONFIG_USE_TASK
        USB_DeviceTaskFn(g_UsbDeviceAudioSpeaker.deviceHandle);
#endif
    }
}
