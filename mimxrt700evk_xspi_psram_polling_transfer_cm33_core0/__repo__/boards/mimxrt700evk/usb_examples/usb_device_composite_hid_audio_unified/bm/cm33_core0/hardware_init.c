/*
 * Copyright 2024 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*${header:start}*/
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include "pin_mux.h"
#include "usb_phy.h"
#include "clock_config.h"
#include "board.h"
#include "app.h"
#include "fsl_sai.h"
#include "fsl_sai_edma.h"
#include "fsl_codec_common.h"
#include "fsl_wm8962.h"
#include "fsl_adapter_audio.h"
#include "fsl_codec_adapter.h"
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#include "fsl_ctimer.h"
#endif
/*${header:end}*/

/*${prototype:start}*/
extern uint32_t USB_AudioSpeakerBufferSpaceUsed(void);
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
void CTIMER_SOF_TOGGLE_HANDLER_PLL(uint32_t i);
#else
extern void USB_DeviceCalculateFeedback(void);
#endif
/*${prototype:end}*/

/*${variable:start}*/
extern usb_device_composite_struct_t g_composite;
extern uint8_t audioPlayDataBuff[AUDIO_SPEAKER_DATA_WHOLE_BUFFER_COUNT_NORMAL * AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME];
extern uint8_t audioRecDataBuff[AUDIO_RECORDER_DATA_WHOLE_BUFFER_COUNT_NORMAL * FS_ISO_IN_ENDP_PACKET_SIZE];
volatile bool g_ButtonPress = false;
AT_NONCACHEABLE_SECTION_INIT(HAL_AUDIO_HANDLE_DEFINE(audioTxHandle));
AT_NONCACHEABLE_SECTION_INIT(HAL_AUDIO_HANDLE_DEFINE(audioRxHandle));
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t audioPlayDMATempBuff[AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME];
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t audioRecDMATempBuff[FS_ISO_IN_ENDP_PACKET_SIZE];
uint32_t masterClockHz = 0U;
codec_handle_t codecHandle;

hal_audio_dma_channel_mux_config_t dmaTxChannelSource = {
    .dmaChannelMuxConfig.dmaRequestSource = DEMO_SAI_TX_SOURCE,
};

edma_channel_config_t edmaTxChannelConfig = {
    .enableMasterIDReplication = true,
    .securityLevel             = kEDMA_ChannelSecurityLevelSecure,
    .protectionLevel           = kEDMA_ChannelProtectionLevelPrivileged,
};

hal_audio_dma_extra_config_t dmaTxExtraConfig = {
    .edmaExtraConfig.enableMasterIdReplication = true,
};

hal_audio_dma_config_t dmaTxConfig = {
    .instance            = DEMO_DMA_INDEX,
    .channel             = DEMO_DMA_TX_CHANNEL,
    .priority            = kHAL_AudioDmaChannelPriorityDefault,
    .dmaChannelMuxConfig = (void *)&dmaTxChannelSource,
    .dmaChannelConfig    = (void *)&edmaTxChannelConfig,
    .dmaExtraConfig      = (void *)&dmaTxExtraConfig,
};

hal_audio_ip_config_t ipTxConfig = {
    .sai.lineMask = 1U << 0U,
    .sai.syncMode = kHAL_AudioSaiModeAsync,
};

hal_audio_config_t audioTxConfig = {
    .dmaConfig         = &dmaTxConfig,
    .ipConfig          = &ipTxConfig,
    .instance          = DEMO_SAI_INSTANCE_INDEX,
    .srcClock_Hz       = DEMO_SAI_CLK_FREQ,
    .sampleRate_Hz     = (uint32_t)kHAL_AudioSampleRate48KHz,
    .masterSlave       = kHAL_AudioMaster,
    .bclkPolarity      = kHAL_AudioSampleOnRisingEdge,
    .frameSyncWidth    = kHAL_AudioFrameSyncWidthHalfFrame,
    .frameSyncPolarity = kHAL_AudioBeginAtFallingEdge,
    .dataFormat        = kHAL_AudioDataFormatI2sClassic,
    .fifoWatermark     = (uint8_t)(FSL_FEATURE_SAI_FIFO_COUNTn(DEMO_SAI) - 1),
    .bitWidth          = (uint8_t)kHAL_AudioWordWidth16bits,
    .lineChannels      = kHAL_AudioStereo,
};

hal_audio_dma_channel_mux_config_t dmaRxChannelSource = {
    .dmaChannelMuxConfig.dmaRequestSource = DEMO_SAI_RX_SOURCE,
};

edma_channel_config_t edmaRxChannelConfig = {
    .enableMasterIDReplication = true,
    .securityLevel             = kEDMA_ChannelSecurityLevelSecure,
    .protectionLevel           = kEDMA_ChannelProtectionLevelPrivileged,
};

hal_audio_dma_extra_config_t dmaRxExtraConfig = {
    .edmaExtraConfig.enableMasterIdReplication = true,
};

hal_audio_dma_config_t dmaRxConfig = {
    .instance            = DEMO_DMA_INDEX,
    .channel             = DEMO_DMA_RX_CHANNEL,
    .priority            = kHAL_AudioDmaChannelPriorityDefault,
    .dmaChannelMuxConfig = (void *)&dmaRxChannelSource,
    .dmaChannelConfig    = (void *)&edmaRxChannelConfig,
    .dmaExtraConfig      = (void *)&dmaRxExtraConfig,
};

hal_audio_ip_config_t ipRxConfig = {
    .sai.lineMask = 1U << 0U,
    .sai.syncMode = kHAL_AudioSaiModeSync,
};

hal_audio_config_t audioRxConfig = {
    .dmaConfig         = &dmaRxConfig,
    .ipConfig          = &ipRxConfig,
    .instance          = DEMO_SAI_INSTANCE_INDEX,
    .srcClock_Hz       = DEMO_SAI_CLK_FREQ,
    .sampleRate_Hz     = (uint32_t)kHAL_AudioSampleRate48KHz,
    .masterSlave       = kHAL_AudioMaster,
    .bclkPolarity      = kHAL_AudioSampleOnRisingEdge,
    .frameSyncWidth    = kHAL_AudioFrameSyncWidthHalfFrame,
    .frameSyncPolarity = kHAL_AudioBeginAtFallingEdge,
    .dataFormat        = kHAL_AudioDataFormatI2sClassic,
    .fifoWatermark     = (uint8_t)(FSL_FEATURE_SAI_FIFO_COUNTn(DEMO_SAI) / 2),
    .bitWidth          = (uint8_t)kHAL_AudioWordWidth16bits,
    .lineChannels      = kHAL_AudioStereo,
};

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
ctimer_callback_t *cb_func_pll[] = {(ctimer_callback_t *)CTIMER_SOF_TOGGLE_HANDLER_PLL};
static ctimer_config_t ctimerInfoPll;
#endif

wm8962_config_t wm8962Config = {
    .i2cConfig = {.codecI2CInstance = BOARD_CODEC_I2C_INSTANCE, .codecI2CSourceClock = DEMO_I2C_CLK_FREQ},
    .route =
        {
            .enableLoopBack            = false,
            .leftInputPGASource        = kWM8962_InputPGASourceInput1,
            .leftInputMixerSource      = kWM8962_InputMixerSourceInputPGA,
            .rightInputPGASource       = kWM8962_InputPGASourceInput3,
            .rightInputMixerSource     = kWM8962_InputMixerSourceInputPGA,
            .leftHeadphoneMixerSource  = kWM8962_OutputMixerDisabled,
            .leftHeadphonePGASource    = kWM8962_OutputPGASourceDAC,
            .rightHeadphoneMixerSource = kWM8962_OutputMixerDisabled,
            .rightHeadphonePGASource   = kWM8962_OutputPGASourceDAC,
        },
    .slaveAddress = WM8962_I2C_ADDR,
    .bus          = kWM8962_BusI2S,
    .format       = {.mclk_HZ    = 24576000U,
               .sampleRate = kWM8962_AudioSampleRate48KHz,
               .bitWidth   = kWM8962_AudioBitWidth16bit},
    .masterSlave  = false,
};
codec_config_t boardCodecConfig = {.codecDevType = kCODEC_WM8962, .codecDevConfig = &wm8962Config};
sai_master_clock_t mclkConfig;
/*${variable:end}*/
/*${function:start}*/

void BOARD_MASTER_CLOCK_CONFIG(void)
{
    mclkConfig.mclkOutputEnable = true;
    mclkConfig.mclkHz = 24576000U;
    mclkConfig.mclkSourceClkHz = 24576000U;
    SAI_SetMasterClockConfig(DEMO_SAI, &mclkConfig);
}

void GPIO00_IRQHandler(void)
{
#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT) || \
    (!defined(FSL_FEATURE_SOC_PORT_COUNT))
    /* Clear external interrupt flag. */
    GPIO_GpioClearInterruptFlags(BOARD_SW5_GPIO, 1U << BOARD_SW5_GPIO_PIN);
#else
    /* Clear external interrupt flag. */
    GPIO_PortClearInterruptFlags(BOARD_SW5_GPIO, 1U << BOARD_SW5_GPIO_PIN);
#endif
    /* Change state of button. */
    g_ButtonPress = true;
    SDK_ISR_EXIT_BARRIER;
}

void BOARD_USB_AUDIO_KEYBOARD_Init(void)
{
    /* Define the init structure for the input switch pin */
    gpio_pin_config_t sw_config = {
        kGPIO_DigitalInput,
        0,
    };
    
    RESET_ClearPeripheralReset(kGPIO0_RST_SHIFT_RSTn);
    CLOCK_EnableClock(kCLOCK_Gpio0);
    
    /* Init input switch GPIO. */
#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT) || \
    (!defined(FSL_FEATURE_SOC_PORT_COUNT))
    GPIO_SetPinInterruptConfig(BOARD_SW5_GPIO, BOARD_SW5_GPIO_PIN, kGPIO_InterruptFallingEdge);
#else
    PORT_SetPinInterruptConfig(BOARD_SW_PORT, BOARD_SW5_GPIO_PIN, kPORT_InterruptFallingEdge);
#endif
     EnableIRQ(GPIO00_IRQn);
     GPIO_PinInit(BOARD_SW5_GPIO, BOARD_SW5_GPIO_PIN, &sw_config);
}

char *SW_GetName(void)
{
    return "SW5";
}

void BOARD_InitHardware(void)
{
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();

    BOARD_InitDebugConsole();
    BOARD_InitAHBSC();
	
    /*Clock setting for LPI2C */
    CLOCK_AttachClk(kFCCLK0_to_FLEXCOMM2);

    /* SAI clock 368.64 / 15 = 24.576MHz */
    CLOCK_AttachClk(kAUDIO_PLL_PFD3_to_AUDIO_VDD2);
    CLOCK_AttachClk(kAUDIO_VDD2_to_SAI012);
    CLOCK_SetClkDiv(kCLOCK_DivSai012Clk, 15U);

    RESET_ClearPeripheralReset(kSAI0_RST_SHIFT_RSTn);
    EDMA_EnableRequest(DMA0, DEMO_SAI_TX_SOURCE);
    EDMA_EnableRequest(DMA0, DEMO_SAI_RX_SOURCE);

    SYSCON0->SAI0_MCLK_CTRL |= SYSCON0_SAI0_MCLK_CTRL_SAIMCLKDIR_MASK;
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
    /* attach AUDIO PLL clock to CTimer CAP0. */
    CLOCK_AttachClk(kAUDIO_VDD2_to_CTIMER0);
	/* Ctimer clock 368.64 / 15 = 24.576MHz  */
    CLOCK_SetClkDiv(kCLOCK_DivCtimer0Clk, 15U);
    RESET_ClearPeripheralReset(kCTIMER0_RST_SHIFT_RSTn);
    g_composite.audioUnified.curAudioPllFrac = CLKCTL2->AUDIOPLL0NUM;
#endif
    BOARD_USB_AUDIO_KEYBOARD_Init();
}

void BOARD_Codec_Init()
{
    if (CODEC_Init(&codecHandle, &boardCodecConfig) != kStatus_Success)
    {
        assert(false);
    }
    if (CODEC_SetVolume(&codecHandle, kCODEC_PlayChannelHeadphoneLeft | kCODEC_PlayChannelHeadphoneRight, 50U) !=
        kStatus_Success)
    {
        assert(false);
    }
}

void BOARD_SetCodecMuteUnmute(bool mute)
{
    if (CODEC_SetMute(&codecHandle, kCODEC_PlayChannelHeadphoneLeft | kCODEC_PlayChannelHeadphoneRight, mute) !=
        kStatus_Success)
    {
        assert(false);
    }
}

static void txCallback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
    uint32_t audioSpeakerPreReadDataCount = 0U;
    uint32_t preAudioSendCount            = 0U;
    hal_audio_transfer_t xfer             = {0};

    if ((USB_AudioSpeakerBufferSpaceUsed() < (g_composite.audioUnified.audioPlayTransferSize)) &&
        (g_composite.audioUnified.startPlayFlag == 1U))
    {
        g_composite.audioUnified.startPlayFlag          = 0;
        g_composite.audioUnified.speakerDetachOrNoInput = 1;
    }
    if (0U != g_composite.audioUnified.startPlayFlag)
    {
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
        USB_DeviceCalculateFeedback();
#endif
        xfer.dataSize     = g_composite.audioUnified.audioPlayTransferSize;
        xfer.data         = audioPlayDataBuff + g_composite.audioUnified.tdReadNumberPlay;
        preAudioSendCount = g_composite.audioUnified.audioSendCount[0];
        g_composite.audioUnified.audioSendCount[0] += g_composite.audioUnified.audioPlayTransferSize;
        if (preAudioSendCount > g_composite.audioUnified.audioSendCount[0])
        {
            g_composite.audioUnified.audioSendCount[1] += 1U;
        }
        g_composite.audioUnified.audioSendTimes++;
        g_composite.audioUnified.tdReadNumberPlay += g_composite.audioUnified.audioPlayTransferSize;
        if (g_composite.audioUnified.tdReadNumberPlay >= g_composite.audioUnified.audioPlayBufferSize)
        {
            g_composite.audioUnified.tdReadNumberPlay = 0;
        }
        audioSpeakerPreReadDataCount = g_composite.audioUnified.audioSpeakerReadDataCount[0];
        g_composite.audioUnified.audioSpeakerReadDataCount[0] += g_composite.audioUnified.audioPlayTransferSize;
        if (audioSpeakerPreReadDataCount > g_composite.audioUnified.audioSpeakerReadDataCount[0])
        {
            g_composite.audioUnified.audioSpeakerReadDataCount[1] += 1U;
        }
    }
    else
    {
        if (0U != g_composite.audioUnified.audioPlayTransferSize)
        {
            xfer.dataSize = g_composite.audioUnified.audioPlayTransferSize;
        }
        else
        {
            xfer.dataSize = AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME / 8U;
        }
        xfer.data = audioPlayDMATempBuff;
    }
    HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&audioTxHandle[0], &xfer);
}

static void rxCallback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
    hal_audio_transfer_t xfer = {0};

    if (g_composite.audioUnified.startRec)
    {
        xfer.dataSize = FS_ISO_IN_ENDP_PACKET_SIZE;
        xfer.data     = audioRecDataBuff + g_composite.audioUnified.tdWriteNumberRec;
        g_composite.audioUnified.tdWriteNumberRec += FS_ISO_IN_ENDP_PACKET_SIZE;
        if (g_composite.audioUnified.tdWriteNumberRec >=
            AUDIO_RECORDER_DATA_WHOLE_BUFFER_COUNT_NORMAL * FS_ISO_IN_ENDP_PACKET_SIZE)
        {
            g_composite.audioUnified.tdWriteNumberRec = 0;
        }
    }
    else
    {
        xfer.dataSize = FS_ISO_IN_ENDP_PACKET_SIZE;
        xfer.data     = audioRecDMATempBuff;
    }
    HAL_AudioTransferReceiveNonBlocking(handle, &xfer);
}

void AUDIO_DMA_EDMA_Start()
{
    BOARD_MASTER_CLOCK_CONFIG();
    usb_echo("Init Audio SAI and CODEC\r\n");
    hal_audio_transfer_t xfer = {0};
    memset(audioPlayDMATempBuff, 0, AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME);
    memset(audioRecDMATempBuff, 0, FS_ISO_IN_ENDP_PACKET_SIZE);
    xfer.dataSize = AUDIO_PLAY_BUFFER_SIZE_ONE_FRAME / 8U;
    xfer.data     = audioPlayDMATempBuff;
    HAL_AudioTxInstallCallback((hal_audio_handle_t)&audioTxHandle[0], txCallback, NULL);
    HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&audioTxHandle[0], &xfer);
    xfer.dataSize = FS_ISO_IN_ENDP_PACKET_SIZE;
    xfer.data     = audioRecDMATempBuff;
    HAL_AudioRxInstallCallback((hal_audio_handle_t)&audioRxHandle[0], rxCallback, NULL);
    HAL_AudioTransferReceiveNonBlocking((hal_audio_handle_t)&audioRxHandle[0], &xfer);
}


#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
void USB_AudioPllChange(void)
{
    CLKCTL2->AUDIOPLL0NUM = g_composite.audioUnified.curAudioPllFrac;
}

void CTIMER_CaptureInit(void)
{
    CTIMER_GetDefaultConfig(&ctimerInfoPll);

    /* Initialize CTimer module */
    CTIMER_Init(CTIMER0, &ctimerInfoPll);
    
    CLOCK_EnableClock(kCLOCK_InputMux);
    RESET_ClearPeripheralReset(kINPUTMUX0_RST_SHIFT_RSTn);
    /* 1_0110b - Selects USB0 start of frame.*/
    INPUTMUX0->CTIMER[0].CAP[0] = 0x16U;

    CTIMER_SetupCapture(CTIMER0, kCTIMER_Capture_0, kCTIMER_Capture_RiseEdge, true);

    CTIMER_RegisterCallBack(CTIMER0, (ctimer_callback_t *)&cb_func_pll[0], kCTIMER_SingleCallback);

    /* Start the L counter */
    CTIMER_StartTimer(CTIMER0);
}
#endif

void USB0_IRQHandler(void)
{
    USB_DeviceEhciIsrFunction(g_composite.deviceHandle);
}

void USB1_IRQHandler(void)
{
    USB_DeviceEhciIsrFunction(g_composite.deviceHandle);
}

void USB_DeviceClockInit(void)
{
    uint32_t usbClockFreq = 24000000;
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };

    /* Power on COM VDDN domain for USB */
    POWER_DisablePD(kPDRUNCFG_DSR_VDDN_COM);    
        
    if (CONTROLLER_ID == kUSB_ControllerEhci0)
    { 
        /* Power on usb ram araay as need, powered USB0RAM array*/
        POWER_DisablePD(kPDRUNCFG_APD_USB0_SRAM);
        POWER_DisablePD(kPDRUNCFG_PPD_USB0_SRAM);
        /* Apply the config */
        POWER_ApplyPD();
        /* disable the read and write gate */
        SYSCON4->USB0_MEM_CTRL |= (SYSCON4_USB0_MEM_CTRL_MEM_WIG_MASK | SYSCON4_USB0_MEM_CTRL_MEM_RIG_MASK |
                                     SYSCON4_USB0_MEM_CTRL_MEM_STDBY_MASK);
        /* Enable the USBPHY0 CLOCK */
        SYSCON4->USBPHY0_CLK_ACTIVE |= SYSCON4_USBPHY0_CLK_ACTIVE_IPG_CLK_ACTIVE_MASK;
        CLOCK_AttachClk(k32KHZ_WAKE_to_USB);
        CLOCK_AttachClk(kOSC_CLK_to_USB_24MHZ);
        CLOCK_EnableClock(kCLOCK_Usb0);
        CLOCK_EnableClock(kCLOCK_UsbphyRef);
        RESET_PeripheralReset(kUSB0_RST_SHIFT_RSTn);
        RESET_PeripheralReset(kUSBPHY0_RST_SHIFT_RSTn);
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, usbClockFreq);
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, usbClockFreq);
        USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL_SYS_CLK_HZ, &phyConfig);
    }

}

void USB_DeviceIsrEnable(void)
{
    uint8_t irqNumber;

    uint8_t usbDeviceEhciIrq[] = USBHS_IRQS;
    irqNumber                  = usbDeviceEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];

    /* Install isr, set priority, and enable IRQ. */
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    EnableIRQ((IRQn_Type)irqNumber);
}
#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle)
{
    USB_DeviceEhciTaskFunction(deviceHandle);
}
#endif
/*${function:end}*/
