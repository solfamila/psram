/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __USB_DISK_H__
#define __USB_DISK_H__ 1

/* @TEST_ANCHOR */

#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0)
#ifndef CONTROLLER_ID
#define CONTROLLER_ID kUSB_ControllerEhci0
#endif
#endif
#if defined(USB_DEVICE_CONFIG_KHCI) && (USB_DEVICE_CONFIG_KHCI > 0)
#ifndef CONTROLLER_ID
#define CONTROLLER_ID kUSB_ControllerKhci0
#endif
#endif
#if defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)
#ifndef CONTROLLER_ID
#define CONTROLLER_ID kUSB_ControllerLpcIp3511Fs0
#endif
#endif
#if defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U)
#ifndef CONTROLLER_ID
#define CONTROLLER_ID kUSB_ControllerLpcIp3511Hs0
#endif
#endif

#define USB_DEVICE_INTERRUPT_PRIORITY                                                                    \
    (3U) /*! @brief enable the write task. 1U supported, 0U not supported . if this macro is enable ,the \
USB_DEVICE_CONFIG_USE_TASK macro should also be enable.*/
#define USB_DEVICE_MSC_USE_WRITE_TASK (0U)
#define USB_DEVICE_MSC_BUFFER_NUMBER  (3U)

#define LOGICAL_UNIT_SUPPORTED (1U)

/****************** Macro definitions ***************/
#include "usb_device_msc_config.h"

#define USB_DEVICE_SDCARD_BLOCK_SIZE_POWER (9U)
#define USB_DEVICE_MSC_ADMA_TABLE_WORDS    (8U)
typedef struct _usb_msc_buffer_struct
{
    uint32_t offset; /*!< Offset of the block need to access*/
    uint32_t size;   /*!< Size of the transfered data*/
    struct _usb_msc_buffer_struct *next;
    uint8_t *buffer; /*!< Buffer address of the transferred data*/
} usb_msc_buffer_struct_t;
typedef struct _usb_msc_struct
{
    usb_device_handle deviceHandle;
    usb_device_msc_struct_t mscStruct;

    uint8_t diskLock;
    uint8_t read_write_error;
    uint8_t currentConfiguration;
    uint8_t currentInterfaceAlternateSetting[USB_MSC_INTERFACE_COUNT];
    uint8_t speed;
    uint8_t attach;
    uint8_t stop; /* indicates this media keeps stop or not, 1: stop, 0: start */
    usb_msc_buffer_struct_t *headlist;
    usb_msc_buffer_struct_t *taillist;
    usb_msc_buffer_struct_t *transferlist;
} usb_msc_struct_t;

#endif
