/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017,2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "usb_host_hid.h"
#include "host_mouse.h"
#include "pin_mux.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "board.h"
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */
#include "app.h"
#if ((!USB_HOST_CONFIG_KHCI) && (!USB_HOST_CONFIG_EHCI) && (!USB_HOST_CONFIG_OHCI) && (!USB_HOST_CONFIG_IP3516HS))
#error Please enable USB_HOST_CONFIG_KHCI, USB_HOST_CONFIG_EHCI, USB_HOST_CONFIG_OHCI, or USB_HOST_CONFIG_IP3516HS in file usb_host_config.
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief host callback function.
 *
 * device attach/detach callback function.
 *
 * @param deviceHandle          device handle.
 * @param configurationHandle   attached device's configuration descriptor information.
 * @param eventCode             callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The application don't support the configuration.
 */
static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode);

/*!
 * @brief application initialization.
 */
static void USB_HostApplicationInit(void);

/*!
 * @brief host freertos task function.
 *
 * @param g_HostHandle   host handle
 */
static void USB_HostTask(void *param);

/*!
 * @brief host mouse freertos task function.
 *
 * @param param   the host mouse instance pointer.
 */
static void USB_HostApplicationTask(void *param);

extern void USB_HostClockInit(void);
extern void USB_HostIsrEnable(void);
extern void USB_HostTaskFn(void *param);
#if (defined(APP_IP3516HS_LPM_ERRATA_WORKAROUND) && (APP_IP3516HS_LPM_ERRATA_WORKAROUND))
extern void USB_HostHidMouseCancelTransfer(void);
#endif
void BOARD_InitHardware(void);

status_t DbgConsole_Deinit(void);

void BOARD_InitPins(void);
void BOARD_DeinitPins(void);
void SW_IntControl(uint8_t enable);
char *SW_GetName(void);
void HW_TimerControl(uint8_t enable);
void USB_LowpowerModeInit(void);
void USB_PreLowpowerMode(void);
uint8_t USB_EnterLowpowerMode(void);
void USB_PostLowpowerMode(void);
void USB_ControllerSuspended(void);
void USB_WaitClockLocked(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief USB host mouse instance global variable */
extern usb_host_mouse_instance_t g_HostHidMouse;
usb_host_handle g_HostHandle;
#if (defined(APP_IP3516HS_LPM_ERRATA_WORKAROUND) && (APP_IP3516HS_LPM_ERRATA_WORKAROUND))
extern volatile uint8_t g_prime_forbid;
#endif
/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief USB isr function.
 */
void *USB_AppMemoryAllocate(uint32_t length)
{
    void *p       = (void *)pvPortMalloc(length);
    uint8_t *temp = (uint8_t *)p;
    if (p)
    {
        for (uint32_t count = 0U; count < length; count++)
        {
            temp[count] = 0U;
        }
    }
    return p;
}
void USB_AppMemoryFree(void *p)
{
    vPortFree(p);
}
static void USB_HostHidControlGetBOSCallback(void *param, usb_host_transfer_t *transfer, usb_status_t status)
{
    usb_host_mouse_instance_t *mouseInstance = &g_HostHidMouse;
    if (NULL == param)
    {
        return;
    }
    USB_HostFreeTransfer(param, transfer);
    if (mouseInstance->L1sleepResumeState ==
        kUSB_HostRunWaitGetBOSDescriptor5) /* get the first five byte of bos descriptor finish */
    {
        if (kStatus_USB_Success == status)
        {
            mouseInstance->L1sleepResumeState = kUSB_HostRunGetBOSDescriptor5Done;
        }
        else
        {
            usb_echo("Can't not get Device BOS descriptor\r\n");
            mouseInstance->L1sleepResumeState = kStatus_Idle;
        }
    }
    else if (mouseInstance->L1sleepResumeState == kUSB_HostRunWaitGetBOSDescriptor) /* get the bos descriptor finish */
    {
        mouseInstance->L1sleepResumeState = kUSB_HostRunGetBOSDescriptorDone;
    }
    else
    {
    }
}

usb_status_t USB_HostParseBOSDescriptorLPMFeature(usb_device_handle deviceHandle, uint8_t *bosDescriptor)
{
    usb_descriptor_union_t *descriptorHead;
    usb_descriptor_union_t *descriptorTail;
    usb_descriptor_usb20_extension_t *usbExtension;
    usb_status_t usb_error;

    usb_error      = kStatus_USB_Error;
    descriptorHead = (usb_descriptor_union_t *)bosDescriptor;
    descriptorTail = (usb_descriptor_union_t *)((uint8_t *)bosDescriptor +
                                                USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(
                                                    ((usb_descriptor_bos_t *)bosDescriptor)->wTotalLength) -
                                                1);
    if (1U == g_HostHidMouse.getBosretryDone)
    {
        if (USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(((usb_descriptor_bos_t *)bosDescriptor)->wTotalLength) !=
            (g_HostHidMouse.mouseBosHeadBuffer[2] + (g_HostHidMouse.mouseBosHeadBuffer[3] << 8)))
        {
            return usb_error;
        }
        if (descriptorHead->common.bDescriptorType != USB_DESCRIPTOR_TYPE_BOS)
        {
            return usb_error;
        }
    }
    while (descriptorHead < descriptorTail)
    {
        if (descriptorHead->common.bDescriptorType == USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY)
        {
            if (descriptorHead->common.bData[0] == USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY_USB20_EXTENSION)
            {
                usbExtension = (usb_descriptor_usb20_extension_t *)descriptorHead;
                if (usbExtension->bmAttributes[0] & USB_DESCRIPTOR_DEVICE_CAPABILITY_USB20_EXTENSION_LPM_MASK)
                {
                    usb_error = kStatus_USB_Success;
                    break;
                }
            }
        }
        descriptorHead = (usb_descriptor_union_t *)((uint8_t *)descriptorHead + descriptorHead->common.bLength);
    }
    return usb_error;
}

usb_status_t USB_HostControlGetBOSDescriptor(usb_host_handle hostHandle,
                                             usb_device_handle deviceHandle,
                                             host_inner_transfer_callback_t callbackFn,
                                             void *callbackParam,
                                             void *buffer,
                                             uint32_t length)
{
    usb_host_transfer_t *transfer;
    uint32_t infoValue = 0U;

    if ((hostHandle == NULL) || (deviceHandle == NULL))
    {
        return kStatus_USB_InvalidHandle;
    }

    /* malloc one transfer */
    if (USB_HostMallocTransfer(hostHandle, &transfer) != kStatus_USB_Success)
    {
#ifdef HOST_ECHO
        usb_echo("error to get transfer\r\n");
#endif
        return kStatus_USB_Busy;
    }
    /* initialize transfer */
    transfer->transferBuffer = buffer;
    transfer->transferLength = length;
    transfer->callbackFn     = callbackFn;
    transfer->callbackParam  = callbackParam;
    transfer->setupPacket->bmRequestType =
        USB_REQUEST_TYPE_RECIPIENT_DEVICE | USB_REQUEST_TYPE_DIR_IN | USB_REQUEST_TYPE_TYPE_STANDARD;
    transfer->setupPacket->bRequest = USB_REQUEST_STANDARD_GET_DESCRIPTOR;
    transfer->setupPacket->wValue   = USB_SHORT_TO_LITTLE_ENDIAN((uint16_t)(USB_DESCRIPTOR_TYPE_BOS << 8));
    transfer->setupPacket->wIndex   = USB_SHORT_TO_LITTLE_ENDIAN(0x00U);
    transfer->setupPacket->wLength  = USB_SHORT_TO_LITTLE_ENDIAN(length);
    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceControlPipe, &infoValue);

    if (USB_HostSendSetup(hostHandle, (usb_host_pipe_handle)infoValue, transfer) !=
        kStatus_USB_Success) /* call host driver api */
    {
#ifdef HOST_ECHO
        usb_echo("failed for USB_HostControlRemoteWakeup\r\n");
#endif
        USB_HostFreeTransfer(hostHandle, transfer);
        return kStatus_USB_Error;
    }
    return kStatus_USB_Success;
}

static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode)
{
    usb_status_t status = kStatus_USB_Success;

    switch (eventCode & 0x0000FFFFU)
    {
        case kUSB_HostEventAttach:
            g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
            g_HostHidMouse.getBosretryDone    = 0U;
            g_HostHidMouse.supportLPM         = 0U;
            status                            = USB_HostHidMouseEvent(deviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventNotSupported:
            usb_echo("device not supported.\r\n");
            break;

        case kUSB_HostEventEnumerationDone:
            status = USB_HostHidMouseEvent(deviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventDetach:
            g_HostHidMouse.getBosretryDone = 0U;
            g_HostHidMouse.supportLPM      = 0U;
            if (g_HostHidMouse.device_bos_descriptor)
            {
#if ((defined(USB_HOST_CONFIG_BUFFER_PROPERTY_CACHEABLE)) && (USB_HOST_CONFIG_BUFFER_PROPERTY_CACHEABLE > 0U))
                SDK_Free(g_HostHidMouse.device_bos_descriptor);
#else
                USB_AppMemoryFree(g_HostHidMouse.device_bos_descriptor);
#endif
                g_HostHidMouse.device_bos_descriptor = NULL;
            }
            status = USB_HostHidMouseEvent(deviceHandle, configurationHandle, eventCode);
            break;
        case kUSB_HostEventL1SleepNotSupport:
            if (kStatus_Idle != g_HostHidMouse.L1sleepResumeState)
            {
                usb_echo("Device Don't Support LPM.\r\n");
            }
            g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
            break;
        case kUSB_HostEventL1SleepNYET:
            if (kStatus_Idle != g_HostHidMouse.L1sleepResumeState)
            {
                usb_echo("Device was unable to enter the L1 state at this time.\r\n");
            }
            break;

        case kUSB_HostEventL1Sleeped:
            if (kStatus_Idle != g_HostHidMouse.L1sleepResumeState)
            {
                USB_ControllerSuspended();
                g_HostHidMouse.L1sleepResumeState = kStatus_L1Sleeped;
            }
            else
            {
                g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
            }
            break;
        case kUSB_HostEventL1SleepError:
            if (kStatus_Idle != g_HostHidMouse.L1sleepResumeState)
            {
                usb_echo("Device failed to respond or an error occurred\r\n");
                g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
            }
            break;

        case kUSB_HostEventDetectResume:
            if (kStatus_Idle != g_HostHidMouse.L1sleepResumeState)
            {
                USB_WaitClockLocked();
            }
            break;
        case kUSB_HostEventL1Resumed:
            if (kStatus_Idle != g_HostHidMouse.L1sleepResumeState)
            {
                if (g_HostHidMouse.L1sleepBus)
                {
                    usb_echo("BUS has been resumed.\r\n");
                }
                else
                {
                    usb_echo("Device has been resumed.\r\n");
                }
            }
            g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
            
            /* allow host mouse to prime next transfer */
#if (defined(APP_IP3516HS_LPM_ERRATA_WORKAROUND) && (APP_IP3516HS_LPM_ERRATA_WORKAROUND))
            g_prime_forbid = 0U;
#endif
            break;
        case kUSB_HostEventEnumerationFail:
            usb_echo("enumeration failed\r\n");
            break;

        default:
            break;
    }
    return status;
}

static void USB_HostApplicationInit(void)
{
    usb_status_t status = kStatus_USB_Success;

    USB_HostClockInit();

#if ((defined FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT))
    SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

    status = USB_HostInit(CONTROLLER_ID, &g_HostHandle, USB_HostEvent);
    if (status != kStatus_USB_Success)
    {
        usb_echo("host init error\r\n");
        return;
    }
    USB_HostIsrEnable();

    usb_echo("host init done\r\n");
}

void USB_PowerPreSwitchHook(void)
{
    HW_TimerControl(0U);

    DbgConsole_Deinit();

    BOARD_DeinitPins();

    USB_PreLowpowerMode();

    vTaskSuspendAll();
}

void USB_PowerPostSwitchHook(void)
{
    USB_WaitClockLocked();
    USB_PostLowpowerMode();
    BOARD_InitPins();
    BOARD_InitDebugConsole();
    HW_TimerControl(1U);
    xTaskResumeAll();
}

void USB_HostL1SleepResumeTask(void)
{
    uint8_t command;

    if (kStatus_USB_Success != DbgConsole_TryGetchar((char *)&command))
    {
        command = 0;
    }
    switch (g_HostHidMouse.L1sleepResumeState)
    {
        case kStatus_Idle:
            if ('s' == command)
            {
                if (g_HostHidMouse.deviceState == kStatus_DEV_Attached)
                {
                    g_HostHidMouse.L1sleepResumeState = kStatus_L1Sleepding;
                    usb_echo("Start suspend USB BUS...\r\n");
                }
                else
                {
                    usb_echo("Device is not attached\r\n");
                }
            }
            else
            {
                if (command)
                {
                    usb_echo("Please Enter 's' to start suspend test\r\n");
                }
            }
            break;
        case kStatus_L1Sleepding:
            if (g_HostHidMouse.deviceSupportRemoteWakeup)
            {
                usb_echo(
                    "\r\nPlease Enter: \r\n\t1. Enable remote wakeup feature.\r\n\t2. Disable remote wakeup "
                    "feature.\r\n");
                g_HostHidMouse.L1sleepResumeState = kStatus_SuspendSetRemoteWakeup;
            }
            else
            {
                g_HostHidMouse.L1sleepResumeState = kUSB_HostRunStartGetBOSDescriptor5;
            }
            break;

        case kStatus_SuspendSetRemoteWakeup:
            if ('1' == command)
            {
                usb_echo("1");
                g_HostHidMouse.L1SetRemoteWakeup  = 1U;
                g_HostHidMouse.L1sleepResumeState = kUSB_HostRunStartGetBOSDescriptor5;
            }
            else if ('2' == command)
            {
                usb_echo("2");
                g_HostHidMouse.L1SetRemoteWakeup  = 0U;
                g_HostHidMouse.L1sleepResumeState = kUSB_HostRunStartGetBOSDescriptor5;
            }
            else
            {
            }
            break;
        case kUSB_HostRunStartGetBOSDescriptor5:
#if (!(defined(APP_IP3516HS_LPM_ERRATA_WORKAROUND) && (APP_IP3516HS_LPM_ERRATA_WORKAROUND)))
            if (1U == g_HostHidMouse.getBosretryDone)
            {
                g_HostHidMouse.L1sleepResumeState = kUSB_HostRunGetBOSDescriptorDone;
            }
            else
#endif
            {
                g_HostHidMouse.L1sleepResumeState = kUSB_HostRunGetBOSDescriptor5;
            }
            break;
        case kUSB_HostRunGetBOSDescriptor5:
            g_HostHidMouse.L1sleepResumeState = kUSB_HostRunWaitGetBOSDescriptor5;
#if (defined(APP_IP3516HS_LPM_ERRATA_WORKAROUND) && (APP_IP3516HS_LPM_ERRATA_WORKAROUND))
           USB_HostHidMouseCancelTransfer();
#endif
            if (kStatus_USB_Success != USB_HostControlGetBOSDescriptor(g_HostHandle, g_HostHidMouse.deviceHandle,
                                                                       USB_HostHidControlGetBOSCallback, g_HostHandle,
                                                                       (void *)&g_HostHidMouse.mouseBosHeadBuffer[0],
                                                                       USB_DESCRIPTOR_LENGTH_BOS_DESCRIPTOR))
            {
                g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
                usb_echo("error in get bos descriptor\r\n");
            }
            break;
        case kUSB_HostRunWaitGetBOSDescriptor5:
            break;
        case kUSB_HostRunGetBOSDescriptor5Done:
            g_HostHidMouse.L1sleepResumeState = kUSB_HostRunWaitGetBOSDescriptor;
            uint32_t bosLenght;
            bosLenght = g_HostHidMouse.mouseBosHeadBuffer[2] + (g_HostHidMouse.mouseBosHeadBuffer[3] << 8);
#if ((defined(USB_HOST_CONFIG_BUFFER_PROPERTY_CACHEABLE)) && (USB_HOST_CONFIG_BUFFER_PROPERTY_CACHEABLE > 0U))
            g_HostHidMouse.device_bos_descriptor = (uint8_t *)SDK_Malloc((bosLenght & 0xFFFFFFFCu) + 4, USB_CACHE_LINESIZE);
#else
            g_HostHidMouse.device_bos_descriptor = USB_AppMemoryAllocate(bosLenght);
#endif
            if (NULL == g_HostHidMouse.device_bos_descriptor)
            {
                usb_echo("Error in malloc\r\n");
                g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
            }
            else
            {
                if (kStatus_USB_Success !=
                    USB_HostControlGetBOSDescriptor(g_HostHandle, g_HostHidMouse.deviceHandle,
                                                    USB_HostHidControlGetBOSCallback, g_HostHandle,
                                                    g_HostHidMouse.device_bos_descriptor, bosLenght))
                {
                    USB_AppMemoryFree(g_HostHidMouse.device_bos_descriptor);
                    g_HostHidMouse.device_bos_descriptor = NULL;
                    usb_echo("error in get bos descriptor\r\n");
                }
            }
            break;
        case kUSB_HostRunWaitGetBOSDescriptor:
            break;
        case kUSB_HostRunGetBOSDescriptorDone:
            g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
            g_HostHidMouse.getBosretryDone    = 1;
            if (1U == g_HostHidMouse.supportLPM)
            {
                g_HostHidMouse.L1sleepResumeState = kStatus_L1StartSleep;
            }
            else
            {
                if (kStatus_USB_Success != USB_HostParseBOSDescriptorLPMFeature(g_HostHidMouse.deviceHandle,
                                                                                g_HostHidMouse.device_bos_descriptor))
                {
                    usb_echo("Device don't support Linker Power Managent(LPM)\r\n");
                    g_HostHidMouse.getBosretryDone = 0;
                }
                else
                {
                    g_HostHidMouse.L1sleepResumeState = kStatus_L1StartSleep;
                    g_HostHidMouse.supportLPM         = 1U;
                }
            }
            DbgConsole_Flush();
            break;
        case kStatus_L1StartSleep:
            g_HostHidMouse.L1sleepBus         = 0;
            g_HostHidMouse.L1sleepResumeState = kStatus_L1SleepRequest;
            uint8_t hirdValue;
            uint8_t lpmParam;
            /*hird value should be 0~15.please refer to usb LPM spec*/
            hirdValue = LPM_HIRD_VALUE;
            lpmParam  = (uint8_t)(hirdValue | (g_HostHidMouse.L1SetRemoteWakeup << 7));
            USB_HostL1SleepDeviceResquestConfig(g_HostHandle, &lpmParam);

            if (kStatus_USB_Success ==
                USB_HostL1SleepDeviceResquest(g_HostHandle, g_HostHidMouse.deviceHandle, g_HostHidMouse.L1sleepBus))
            {
            }
            else
            {
                usb_echo("Send L1 sleep request failed.\r\n");
                g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
            }
            DbgConsole_Flush();
            break;
        case kStatus_L1SleepRequest:
            break;
        case kStatus_L1Sleeped:
            DbgConsole_Flush();
            if (g_HostHidMouse.L1sleepBus)
            {
                usb_echo("BUS has been suspended.\r\n");
            }
            else
            {
                usb_echo("Device has been suspended.\r\n");
            }
            DbgConsole_Flush();
            usb_echo("Please Press wakeup switch(%s) to start resume test.\r\n", SW_GetName());
            if (g_HostHidMouse.L1SetRemoteWakeup)
            {
                usb_echo("Or, wait for device sends resume signal.\r\n");
            }
            DbgConsole_Flush();
            USB_PowerPreSwitchHook();
            SW_IntControl(1);
            g_HostHidMouse.L1sleepResumeState = kStatus_L1WaitResume;
            if (kStatus_Success != USB_EnterLowpowerMode())
            {
                g_HostHidMouse.selfWakeup = 1U;
                USB_PowerPostSwitchHook();
                usb_echo("Enter VLPS mode failed!\r\n");
            }
            else
            {
                USB_PowerPostSwitchHook();
            }
            break;
        case kStatus_L1WaitResume:
            if (g_HostHidMouse.selfWakeup)
            {
                g_HostHidMouse.selfWakeup = 0U;
                usb_echo("Start L1 resume the device.\r\n");
                g_HostHidMouse.L1sleepResumeState = kStatus_L1ResumeRequest;
                if (kStatus_USB_Success == USB_HostL1ResumeDeviceResquest(g_HostHandle, g_HostHidMouse.deviceHandle,
                                                                          g_HostHidMouse.L1sleepBus))
                {
                }
                else
                {
                    g_HostHidMouse.L1sleepResumeState = kStatus_Idle;
                    usb_echo("Send resume signal failed.\r\n");
                }
            }
            break;
        case kStatus_L1ResumeRequest:
            break;
        default:
            break;
    }
    command = 0;
}

static void USB_HostTask(void *param)
{
    while (1)
    {
        USB_HostTaskFn(param);
    }
}

static void USB_HostSleepResume(void *param)
{
    while (1)
    {
        USB_HostL1SleepResumeTask();
        vTaskDelay(1);
    }
}

static void USB_HostApplicationTask(void *param)
{
#if ((defined(USB_HOST_CONFIG_LOW_POWER_MODE)) && (USB_HOST_CONFIG_LOW_POWER_MODE > 0U))
    USB_LowpowerModeInit();
#endif

    USB_HostApplicationInit();

#if ((defined(USB_HOST_CONFIG_LOW_POWER_MODE)) && (USB_HOST_CONFIG_LOW_POWER_MODE > 0U))
    HW_TimerControl(1);
    usb_echo("Please Enter 's' to start L1 suspend test\r\n");
#endif

    if (xTaskCreate(USB_HostTask, "usb host task", 2000L / sizeof(portSTACK_TYPE), g_HostHandle, 4, NULL) != pdPASS)
    {
        usb_echo("usb host task create failed!\r\n");
        return;
    }

    if (xTaskCreate(USB_HostSleepResume, "host sleep resume task", 2000L / sizeof(portSTACK_TYPE), param, 4, NULL) !=
        pdPASS)
    {
        usb_echo("usb host sleep/resume task create failed!\r\n");
        return;
    }

    while (1)
    {
        USB_HostHidMouseTask(param);
    }
}

int main(void)
{
    BOARD_InitHardware();

    if (xTaskCreate(USB_HostApplicationTask, "app task", 2000L / sizeof(portSTACK_TYPE), &g_HostHidMouse, 3, NULL) !=
        pdPASS)
    {
        usb_echo("create mouse task error\r\n");
    }

    vTaskStartScheduler();

    while (1)
    {
        ;
    }
}
