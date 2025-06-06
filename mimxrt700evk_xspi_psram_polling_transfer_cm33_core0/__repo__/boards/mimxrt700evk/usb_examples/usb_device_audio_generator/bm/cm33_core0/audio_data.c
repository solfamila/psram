/*
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
 * Copyright 2016,2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_descriptor.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U) || \
    (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
#define AUDIO_ENDPOINT_MAX_PACKET_SIZE \
    (FS_ISO_IN_ENDP_PACKET_SIZE > HS_ISO_IN_ENDP_PACKET_SIZE ? FS_ISO_IN_ENDP_PACKET_SIZE : HS_ISO_IN_ENDP_PACKET_SIZE)
#endif

#if defined(USB_DEVICE_CONFIG_KHCI) && (USB_DEVICE_CONFIG_KHCI > 0U)
#define AUDIO_ENDPOINT_MAX_PACKET_SIZE (FS_ISO_IN_ENDP_PACKET_SIZE)
#endif

#if defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)
#define AUDIO_ENDPOINT_MAX_PACKET_SIZE (FS_ISO_IN_ENDP_PACKET_SIZE)
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* data of the sound */
#if defined(__cpluscplus)
extern const unsigned char wavData[] = {
#else
const unsigned char wavData[] = {
#endif
    0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x7f, 0x7f, 0x7e, 0x7e, 0x7f, 0x7e, 0x7e, 0x7f, 0x7f, 0x7e, 0x7f, 0x7f, 0x7f,
    0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x80, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7f, 0x7e, 0x7e, 0x7f, 0x7f, 0x7e,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x80, 0x80, 0x81, 0x86, 0x8f, 0x73, 0x68, 0x8d, 0x84,
    0x78, 0x85, 0x80, 0x78, 0x80, 0x8c, 0x7d, 0x75, 0x81, 0x88, 0x7f, 0x7b, 0x80, 0x82, 0x7e, 0x81, 0x83, 0x7d, 0x7e,
    0x82, 0x84, 0x7f, 0x7c, 0x80, 0x81, 0x85, 0x88, 0x87, 0x7f, 0x72, 0x85, 0x82, 0x70, 0x80, 0x83, 0x7d, 0x7a, 0x85,
    0x85, 0x76, 0x7e, 0x87, 0x7e, 0x7a, 0x84, 0x84, 0x7c, 0x7d, 0x84, 0x80, 0x7d, 0x7f, 0x88, 0x89, 0x6c, 0x71, 0x8c,
    0x86, 0x82, 0x80, 0x75, 0x72, 0x80, 0x8c, 0x80, 0x75, 0x7b, 0x84, 0x82, 0x7c, 0x7d, 0x80, 0x82, 0x88, 0x96, 0x7a,
    0x62, 0x83, 0x7f, 0x6d, 0x81, 0x92, 0x84, 0x68, 0x6e, 0xa2, 0xb6, 0x7b, 0x64, 0x88, 0x76, 0x6a, 0x8c, 0x87, 0x64,
    0x70, 0x94, 0x88, 0x70, 0x80, 0x8b, 0x7b, 0x7e, 0x82, 0x80, 0x7b, 0x7e, 0x8b, 0x7f, 0x72, 0x7e, 0x8a, 0x79, 0x76,
    0x8b, 0x80, 0x72, 0x81, 0x89, 0x80, 0x74, 0x85, 0x85, 0x77, 0x80, 0x86, 0x7f, 0x77, 0x81, 0x84, 0x7b, 0x7c, 0x7f,
    0x7f, 0x80, 0x7c, 0x7e, 0x80, 0x7e, 0x7f, 0x84, 0x83, 0x81, 0x7c, 0x7a, 0x89, 0x7e, 0x79, 0x83, 0x80, 0x83, 0x7b,
    0x87, 0x8c, 0x75, 0x85, 0x8a, 0x80, 0x7a, 0x81, 0x83, 0x78, 0x82, 0x7f, 0x79, 0x87, 0x80, 0x7d, 0x8b, 0x7b, 0x7a,
    0x7d, 0x7f, 0x7b, 0x89, 0x83, 0x74, 0x9f, 0x82, 0x71, 0x80, 0x7e, 0x80, 0x76, 0x7d, 0x84, 0x80, 0x7b, 0x86, 0x86,
    0x7d, 0x70, 0x86, 0x8c, 0x72, 0x7f, 0x89, 0x83, 0x82, 0x83, 0x7a, 0x7c, 0x89, 0x82, 0x7f, 0x84, 0x7b, 0x7b, 0x88,
    0x78, 0x78, 0x87, 0x7b, 0x7c, 0x82, 0x84, 0x85, 0x79, 0x7f, 0x82, 0x7e, 0x85, 0x7f, 0x7b, 0x81, 0x86, 0x7b, 0x7a,
    0x86, 0x7d, 0x7b, 0x79, 0x86, 0x7f, 0x6f, 0x84, 0x85, 0x84, 0x73, 0x7f, 0x99, 0x67, 0x7d, 0x99, 0x6d, 0x76, 0x79,
    0x98, 0x84, 0x65, 0x7c, 0x8c, 0x94, 0x68, 0x79, 0x8e, 0x89, 0x80, 0x7a, 0x7e, 0x79, 0x85, 0x79, 0x73, 0x8b, 0x8a,
    0x79, 0x79, 0x7d, 0x88, 0x86, 0x7b, 0x79, 0x7a, 0x85, 0x7e, 0x74, 0x86, 0x7f, 0x83, 0x73, 0x7f, 0x8c, 0x7f, 0x89,
    0x6d, 0x85, 0x88, 0x7d, 0x83, 0x7e, 0x88, 0x78, 0x7e, 0x88, 0x83, 0x7a, 0x7d, 0x80, 0x7e, 0x83, 0x81, 0x7d, 0x7c,
    0x88, 0x7c, 0x77, 0x81, 0x86, 0x84, 0x74, 0x78, 0x8f, 0x85, 0x73, 0x85, 0x83, 0x81, 0x7d, 0x7a, 0x81, 0x81, 0x88,
    0x75, 0x83, 0x88, 0x77, 0x86, 0x7c, 0x7a, 0x7d, 0x83, 0x84, 0x7a, 0x82, 0x81, 0x7e, 0x85, 0x80, 0x80, 0x82, 0x7f,
    0x80, 0x79, 0x85, 0x80, 0x7a, 0x84, 0x7a, 0x83, 0x80, 0x80, 0x80, 0x7b, 0x85, 0x80, 0x79, 0x81, 0x7f, 0x80, 0x83,
    0x80, 0x80, 0x7e, 0x82, 0x7d, 0x7d, 0x81, 0x7e, 0x7e, 0x7e, 0x7f, 0x82, 0x81, 0x7c, 0x7f, 0x86, 0x82, 0x7b, 0x7c,
    0x7f, 0x82, 0x7e, 0x7c, 0x80, 0x81, 0x80, 0x7f, 0x7d, 0x7f, 0x82, 0x83, 0x7a, 0x7e, 0x82, 0x75, 0x85, 0x7f, 0x7c,
    0x87, 0x7f, 0x7f, 0x83, 0x81, 0x7b, 0x84, 0x7f, 0x81, 0x7f, 0x77, 0x82, 0x7f, 0x7d, 0x80, 0x7f, 0x85, 0x81, 0x80,
    0x82, 0x7b, 0x82, 0x7c, 0x7b, 0x82, 0x7c, 0x82, 0x81, 0x7a, 0x80, 0x84, 0x7b, 0x7f, 0x81, 0x7c, 0x81, 0x80, 0x80,
    0x7f, 0x7c, 0x80, 0x81, 0x7a, 0x82, 0x80, 0x7d, 0x87, 0x7f, 0x7e, 0x80, 0x81, 0x7e, 0x7c, 0x82, 0x7d, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7d, 0x82, 0x80, 0x81, 0x85, 0x7a, 0x7d, 0x86, 0x7e, 0x7d, 0x83, 0x7e, 0x82, 0x81, 0x7c, 0x81, 0x84,
    0x7e, 0x7e, 0x83, 0x7c, 0x7e, 0x7f, 0x7f, 0x83, 0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x7e, 0x7f, 0x7f, 0x84, 0x85, 0x7c,
    0x7e, 0x81, 0x81, 0x83, 0x82, 0x7e, 0x7e, 0x83, 0x81, 0x7f, 0x80, 0x7e, 0x7e, 0x81, 0x82, 0x7f, 0x82, 0x84, 0x7d,
    0x7e, 0x84, 0x82, 0x7d, 0x7d, 0x81, 0x80, 0x7e, 0x7e, 0x80, 0x81, 0x7f, 0x82, 0x7c, 0x7e, 0x81, 0x7e, 0x81, 0x84,
    0x80, 0x7d, 0x7e, 0x81, 0x7f, 0x7c, 0x7f, 0x7e, 0x82, 0x81, 0x7b, 0x82, 0x80, 0x7d, 0x80, 0x7e, 0x7d, 0x81, 0x7d,
    0x7e, 0x82, 0x7c, 0x81, 0x82, 0x7e, 0x80, 0x7e, 0x7d, 0x80, 0x82, 0x81, 0x7e, 0x81, 0x84, 0x7b, 0x7e, 0x7e, 0x7c,
    0x82, 0x80, 0x7d, 0x7b, 0x82, 0x7c, 0x7a, 0x83, 0x7c, 0x7e, 0x84, 0x7d, 0x80, 0x82, 0x7b, 0x83, 0x85, 0x7a, 0x7d,
    0x83, 0x7a, 0x7f, 0x83, 0x7d, 0x82, 0x7f, 0x7b, 0x81, 0x83, 0x7d, 0x7b, 0x81, 0x7e, 0x7e, 0x82, 0x7c, 0x83, 0x83,
    0x7f, 0x80, 0x7c, 0x81, 0x81, 0x7d, 0x7f, 0x80, 0x80, 0x82, 0x82, 0x7d, 0x82, 0x81, 0x7e, 0x7f, 0x7f, 0x83, 0x81,
    0x7d, 0x80, 0x81, 0x80, 0x82, 0x7d, 0x7e, 0x82, 0x7f, 0x81, 0x83, 0x7e, 0x7e, 0x81, 0x80, 0x7e, 0x80, 0x7f, 0x81,
    0x82, 0x80, 0x82, 0x80, 0x7f, 0x82, 0x7f, 0x7e, 0x7e, 0x7f, 0x82, 0x82, 0x82, 0x81, 0x7d, 0x7f, 0x80, 0x7e, 0x81,
    0x7f, 0x82, 0x81, 0x80, 0x81, 0x7f, 0x7e, 0x81, 0x80, 0x7d, 0x7f, 0x82, 0x7f, 0x7e, 0x81, 0x80, 0x80, 0x80, 0x80,
    0x7d, 0x80, 0x83, 0x7f, 0x7c, 0x80, 0x7f, 0x7f, 0x81, 0x7e, 0x81, 0x83, 0x80, 0x7d, 0x80, 0x7f, 0x80, 0x7e, 0x7b,
    0x82, 0x7f, 0x7e, 0x7f, 0x7f, 0x7f, 0x82, 0x80, 0x7f, 0x82, 0x7e, 0x7f, 0x80, 0x7d, 0x7d, 0x83, 0x7f, 0x7d, 0x7f,
    0x7f, 0x7f, 0x7f, 0x82, 0x7f, 0x81, 0x7c, 0x7e, 0x81, 0x7d, 0x7b, 0x83, 0x7f, 0x7c, 0x84, 0x7d, 0x7d, 0x7f, 0x81,
    0x7e, 0x80, 0x7f, 0x7d, 0x7e, 0x7e, 0x82, 0x7e, 0x7c, 0x7f, 0x80, 0x7c, 0x7f, 0x80, 0x7d, 0x80, 0x83, 0x7f, 0x7e,
    0x7d, 0x81, 0x83, 0x7b, 0x81, 0x7f, 0x7e, 0x81, 0x80, 0x82, 0x7d, 0x7e, 0x84, 0x7c, 0x7e, 0x7f, 0x7b, 0x82, 0x7e,
    0x7f, 0x83, 0x7f, 0x81, 0x85, 0x81, 0x80, 0x7f, 0x7f, 0x81, 0x7f, 0x81, 0x7b, 0x7d, 0x81, 0x7e, 0x7d, 0x81, 0x7e,
    0x82, 0x84, 0x7f, 0x80, 0x82, 0x82, 0x81, 0x82, 0x7e, 0x80, 0x81, 0x7e, 0x81, 0x7e, 0x7d, 0x7f, 0x80, 0x7d, 0x7e,
    0x80, 0x7e, 0x81, 0x81, 0x81, 0x81, 0x83, 0x7f, 0x81, 0x82, 0x7f, 0x80, 0x7d, 0x82, 0x80, 0x80, 0x7f, 0x7e, 0x81,
    0x7f, 0x81, 0x80, 0x7f, 0x7f, 0x7f, 0x80, 0x82, 0x7f, 0x7e, 0x80, 0x82, 0x81, 0x7e, 0x7f, 0x7f, 0x82, 0x80, 0x7f,
    0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x81, 0x80, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x80, 0x7f, 0x80, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x7f,
    0x80, 0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x7e, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x80, 0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f,
    0x7f, 0x80, 0x80, 0x80, 0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x81, 0x80, 0x80, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x7f, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x80, 0x80, 0x80, 0x7f,
    0x7f, 0x7f, 0x7e, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x80, 0x82, 0x82, 0x82, 0x82, 0x81, 0x80, 0x80, 0x7e, 0x7e, 0x7d,
    0x7c, 0x7d, 0x7d, 0x7d, 0x7e, 0x7f, 0x7f, 0x80, 0x81, 0x81, 0x82, 0x82, 0x81, 0x80, 0x7f, 0x7e, 0x7d, 0x7d, 0x7c,
    0x7c, 0x7c, 0x7c, 0x7e, 0x7e, 0x80, 0x81, 0x82, 0x83, 0x82, 0x82, 0x81, 0x80, 0x7f, 0x7e, 0x7d, 0x7b, 0x7b, 0x7c,
    0x7d, 0x7f, 0x80, 0x81, 0x81, 0x81, 0x82, 0x82, 0x81, 0x80, 0x7f, 0x7d, 0x7c, 0x7d, 0x7d, 0x7e, 0x7f, 0x7f, 0x80,
    0x81, 0x82, 0x83, 0x83, 0x82, 0x82, 0x80, 0x80, 0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x7c, 0x7c, 0x7d, 0x7e, 0x7f, 0x7f,
    0x80, 0x81, 0x84, 0x88, 0x86, 0x84, 0x83, 0x7d, 0x7e, 0x7e, 0x7c, 0x7e, 0x7c, 0x7b, 0x7c, 0x7e, 0x81, 0x83, 0x85,
    0x84, 0x83, 0x82, 0x80, 0x81, 0x80, 0x7f, 0x7e, 0x7b, 0x7b, 0x7c, 0x7e, 0x80, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82,
    0x83, 0x81, 0x80, 0x7e, 0x7c, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x80, 0x80, 0x81, 0x82, 0x83, 0x82, 0x81, 0x80, 0x7f,
    0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x82, 0x81, 0x80, 0x80, 0x7f, 0x7e, 0x7f, 0x7e,
    0x7d, 0x7e, 0x7e, 0x7e, 0x80, 0x80, 0x81, 0x81, 0x81, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x7e, 0x7d, 0x7e, 0x7e, 0x7e,
    0x80, 0x80, 0x80, 0x80, 0x81, 0x82, 0x81, 0x82, 0x80, 0x7e, 0x80, 0x7f, 0x81, 0x7f, 0x7e, 0x7e, 0x7a, 0x7e, 0x7e,
    0x7e, 0x81, 0x7e, 0x7f, 0x7e, 0x7f, 0x80, 0x7f, 0x80, 0x7e, 0x7e, 0x7c, 0x7c, 0x7e, 0x7d, 0x7e, 0x7f, 0x7d, 0x7d,
    0x7e, 0x7f, 0x80, 0x85, 0x87, 0x85, 0x87, 0x82, 0x7f, 0x80, 0x7d, 0x7d, 0x7c, 0x7b, 0x7a, 0x7a, 0x7d, 0x7e, 0x83,
    0x84, 0x84, 0x85, 0x82, 0x82, 0x81, 0x80, 0x7f, 0x7e, 0x7c, 0x7a, 0x7b, 0x7d, 0x7f, 0x84, 0x84, 0x84, 0x86, 0x84,
    0x84, 0x84, 0x81, 0x7f, 0x7d, 0x7c, 0x7a, 0x7b, 0x7c, 0x7c, 0x7f, 0x7e, 0x80, 0x81, 0x81, 0x82, 0x81, 0x7e, 0x7c,
    0x7a, 0x76, 0x77, 0x78, 0x79, 0x82, 0x8f, 0x92, 0x91, 0x95, 0x84, 0x80, 0x82, 0x76, 0x7c, 0x78, 0x73, 0x72, 0x73,
    0x78, 0x7d, 0x8a, 0x8a, 0x8d, 0x8f, 0x85, 0x86, 0x83, 0x7f, 0x7f, 0x7a, 0x76, 0x72, 0x74, 0x76, 0x7c, 0x84, 0x85,
    0x89, 0x88, 0x85, 0x86, 0x85, 0x83, 0x81, 0x7d, 0x77, 0x75, 0x75, 0x76, 0x7c, 0x81, 0x85, 0x89, 0x89, 0x89, 0x88,
    0x86, 0x83, 0x80, 0x7b, 0x77, 0x75, 0x74, 0x77, 0x7b, 0x7d, 0x7f, 0x7f, 0x7f, 0x80, 0x81, 0x80, 0x80, 0x7c, 0x73,
    0x71, 0x70, 0x70, 0x7d, 0x8d, 0x9a, 0x9b, 0x97, 0x92, 0x7d, 0x81, 0x7b, 0x76, 0x7c, 0x6e, 0x6d, 0x6a, 0x6f, 0x79,
    0x85, 0x92, 0x8f, 0x94, 0x8a, 0x84, 0x86, 0x7f, 0x80, 0x7b, 0x74, 0x6f, 0x6c, 0x70, 0x76, 0x82, 0x88, 0x8b, 0x8d,
    0x88, 0x88, 0x87, 0x87, 0x88, 0x85, 0x80, 0x77, 0x73, 0x72, 0x75, 0x7c, 0x80, 0x84, 0x85, 0x84, 0x83, 0x83, 0x83,
    0x81, 0x7f, 0x79, 0x74, 0x71, 0x6f, 0x6d, 0x6d, 0x6f, 0x70, 0x7d, 0x8e, 0xa9, 0xb7, 0xa9, 0xab, 0x88, 0x73, 0x77,
    0x60, 0x6e, 0x6a, 0x64, 0x66, 0x67, 0x75, 0x81, 0x9f, 0xa0, 0xa6, 0xa4, 0x89, 0x88, 0x79, 0x74, 0x75, 0x6e, 0x69,
    0x65, 0x69, 0x6c, 0x80, 0x8e, 0x95, 0xa1, 0x9a, 0x97, 0x94, 0x8a, 0x86, 0x7d, 0x72, 0x69, 0x67, 0x67, 0x70, 0x7d,
    0x83, 0x8c, 0x8b, 0x86, 0x82, 0x7b, 0x78, 0x75, 0x71, 0x66, 0x5f, 0x5b, 0x63, 0x8e, 0xad, 0xbf, 0xc7, 0xad, 0x8f,
    0x7a, 0x6f, 0x69, 0x71, 0x6d, 0x5e, 0x5d, 0x54, 0x62, 0x7e, 0x95, 0xad, 0xb2, 0xa7, 0x94, 0x87, 0x7b, 0x78, 0x7b,
    0x70, 0x69, 0x5f, 0x58, 0x62, 0x73, 0x88, 0x9c, 0xa3, 0x9f, 0x96, 0x8c, 0x84, 0x89, 0x89, 0x81, 0x79, 0x6a, 0x5f,
    0x62, 0x6c, 0x7c, 0x8e, 0x93, 0x91, 0x8a, 0x7f, 0x7b, 0x7a, 0x78, 0x76, 0x6f, 0x64, 0x5d, 0x57, 0x57, 0x67, 0x86,
    0xa8, 0xcb, 0xd2, 0xbd, 0xab, 0x83, 0x6e, 0x6e, 0x5d, 0x63, 0x5a, 0x4f, 0x52, 0x5b, 0x74, 0x90, 0xb1, 0xb8, 0xb9,
    0xaa, 0x8e, 0x86, 0x76, 0x71, 0x6f, 0x63, 0x5a, 0x55, 0x5a, 0x68, 0x83, 0x99, 0xa7, 0xaf, 0xa5, 0x9f, 0x9a, 0x8c,
    0x86, 0x7c, 0x67, 0x60, 0x5a, 0x5a, 0x6f, 0x7c, 0x8b, 0x95, 0x8f, 0x88, 0x7e, 0x74, 0x6e, 0x6e, 0x68, 0x5e, 0x5d,
    0x61, 0x77, 0x9e, 0xc2, 0xd3, 0xc6, 0xb6, 0x89, 0x6d, 0x6c, 0x57, 0x68, 0x62, 0x58, 0x5b, 0x57, 0x6c, 0x84, 0xa8,
    0xb5, 0xbd, 0xb2, 0x91, 0x87, 0x71, 0x6c, 0x6f, 0x66, 0x64, 0x5c, 0x5b, 0x64, 0x7a, 0x90, 0xa4, 0xb5, 0xb1, 0xab,
    0x99, 0x88, 0x7a, 0x6e, 0x68, 0x5f, 0x60, 0x60, 0x69, 0x77, 0x81, 0x8e, 0x8b, 0x85, 0x77, 0x6a, 0x62, 0x5b, 0x60,
    0x67, 0x80, 0x99, 0xb3, 0xd0, 0xb5, 0xb5, 0x9b, 0x6c, 0x7c, 0x5b, 0x5d, 0x64, 0x50, 0x5d, 0x5b, 0x71, 0x7f, 0xa2,
    0xae, 0xad, 0xb7, 0x91, 0x8e, 0x7e, 0x6a, 0x70, 0x62, 0x5f, 0x5c, 0x60, 0x65, 0x79, 0x8f, 0x9a, 0xb2, 0xaf, 0xa9,
    0xa3, 0x8a, 0x7e, 0x70, 0x62, 0x5b, 0x5f, 0x5e, 0x6b, 0x7c, 0x7f, 0x8d, 0x86, 0x7c, 0x74, 0x63, 0x5d, 0x58, 0x5e,
    0x76, 0x93, 0xad, 0xd5, 0xc9, 0xb2, 0xb2, 0x71, 0x72, 0x6e, 0x4e, 0x6a, 0x53, 0x53, 0x5a, 0x64, 0x79, 0x96, 0xb6,
    0xad, 0xbf, 0xa4, 0x8a, 0x8a, 0x6a, 0x6e, 0x6a, 0x5e, 0x5d, 0x5b, 0x60, 0x6d, 0x89, 0x97, 0xb1, 0xbd, 0xaf, 0xad,
    0x92, 0x7f, 0x72, 0x5e, 0x5b, 0x58, 0x5d, 0x69, 0x78, 0x82, 0x89, 0x89, 0x78, 0x71, 0x5f, 0x53, 0x58, 0x65, 0x8d,
    0xa6, 0xcd, 0xe0, 0xb3, 0xb9, 0x8a, 0x62, 0x7d, 0x51, 0x64, 0x60, 0x4b, 0x57, 0x56, 0x72, 0x83, 0xb2, 0xb5, 0xb9,
    0xbb, 0x8b, 0x8d, 0x74, 0x68, 0x71, 0x60, 0x60, 0x58, 0x5a, 0x61, 0x79, 0x93, 0xa8, 0xc4, 0xb7, 0xb3, 0xa0, 0x7b,
    0x72, 0x5c, 0x55, 0x5a, 0x5d, 0x67, 0x74, 0x7d, 0x7e, 0x83, 0x74, 0x6b, 0x5f, 0x50, 0x5e, 0x79, 0x9e, 0xbd, 0xe5,
    0xcd, 0xaf, 0xa9, 0x5c, 0x6c, 0x65, 0x4c, 0x74, 0x4f, 0x56, 0x57, 0x5e, 0x7a, 0x98, 0xbd, 0xb6, 0xc6, 0xa2, 0x85,
    0x81, 0x5f, 0x6c, 0x68, 0x60, 0x65, 0x59, 0x5e, 0x6b, 0x87, 0xa0, 0xba, 0xc3, 0xb5, 0xa7, 0x80, 0x6d, 0x5f, 0x54,
    0x62, 0x62, 0x6c, 0x71, 0x71, 0x73, 0x71, 0x71, 0x67, 0x64, 0x5b, 0x6e, 0x96, 0xa7, 0xdd, 0xe1, 0xb7, 0xbc, 0x75,
    0x5a, 0x67, 0x43, 0x64, 0x60, 0x59, 0x5f, 0x5f, 0x6f, 0x83, 0xb2, 0xb3, 0xc8, 0xbe, 0x93, 0x8d, 0x66, 0x5f, 0x63,
    0x5f, 0x67, 0x64, 0x66, 0x69, 0x7c, 0x92, 0xa7, 0xbf, 0xbc, 0xb0, 0x94, 0x76, 0x60, 0x55, 0x5a, 0x61, 0x6f, 0x74,
    0x72, 0x6e, 0x65, 0x62, 0x5b, 0x5f, 0x6e, 0x92, 0xb3, 0xca, 0xe8, 0xc2, 0xb1, 0x98, 0x5a, 0x6c, 0x50, 0x50, 0x63,
    0x4d, 0x5e, 0x59, 0x6d, 0x7f, 0xa0, 0xbb, 0xba, 0xc8, 0xa4, 0x91, 0x7e, 0x60, 0x66, 0x5b, 0x5e, 0x61, 0x60, 0x6a,
    0x73, 0x8c, 0x9a, 0xaf, 0xba, 0xac, 0xa3, 0x85, 0x6d, 0x60, 0x58, 0x5c, 0x63, 0x6b, 0x6b, 0x6b, 0x63, 0x5b, 0x59,
    0x5c, 0x77, 0x9f, 0xbe, 0xe3, 0xe2, 0xc0, 0xae, 0x76, 0x5b, 0x5d, 0x48, 0x5d, 0x5a, 0x54, 0x5a, 0x59, 0x6c, 0x85,
    0xaa, 0xbb, 0xca, 0xc2, 0xa2, 0x90, 0x6f, 0x60, 0x60, 0x5b, 0x61, 0x63, 0x64, 0x69, 0x75, 0x86, 0x9a, 0xb0, 0xb6,
    0xb1, 0xa0, 0x84, 0x6c, 0x5d, 0x57, 0x5b, 0x65, 0x68, 0x69, 0x64, 0x59, 0x53, 0x56, 0x6f, 0x9a, 0xc2, 0xe8, 0xec,
    0xcb, 0xb2, 0x78, 0x5a, 0x58, 0x47, 0x5e, 0x5c, 0x57, 0x58, 0x54, 0x65, 0x7b, 0xa5, 0xbb, 0xcf, 0xcc, 0xac, 0x97,
    0x73, 0x62, 0x5f, 0x5d, 0x66, 0x66, 0x67, 0x67, 0x6c, 0x7b, 0x8e, 0xa7, 0xb4, 0xb6, 0xaa, 0x8e, 0x75, 0x60, 0x57,
    0x59, 0x61, 0x67, 0x67, 0x62, 0x55, 0x4f, 0x5b, 0x7a, 0xa8, 0xd7, 0xf3, 0xe4, 0xcc, 0x9b, 0x65, 0x5d, 0x45, 0x53,
    0x62, 0x59, 0x61, 0x52, 0x57, 0x64, 0x84, 0xa8, 0xc2, 0xd7, 0xc3, 0xaf, 0x8d, 0x69, 0x62, 0x59, 0x62, 0x6a, 0x6b,
    0x6c, 0x66, 0x6b, 0x76, 0x8c, 0xa3, 0xb0, 0xb6, 0xa5, 0x8d, 0x73, 0x5c, 0x56, 0x56, 0x5f, 0x65, 0x65, 0x5c, 0x53,
    0x58, 0x71, 0x9a, 0xcb, 0xe8, 0xe4, 0xd6, 0x9f, 0x74, 0x5c, 0x40, 0x53, 0x58, 0x5f, 0x66, 0x58, 0x5a, 0x5d, 0x78,
    0x94, 0xb7, 0xce, 0xc7, 0xbd, 0x97, 0x77, 0x65, 0x56, 0x5f, 0x67, 0x6f, 0x71, 0x6b, 0x6b, 0x6c, 0x7c, 0x90, 0xa3,
    0xb2, 0xab, 0x9b, 0x81, 0x66, 0x56, 0x50, 0x55, 0x5f, 0x63, 0x61, 0x63, 0x6b, 0x7f, 0xa5, 0xc3, 0xd2, 0xdc, 0xbe,
    0x9a, 0x7e, 0x54, 0x50, 0x51, 0x53, 0x63, 0x63, 0x64, 0x64, 0x6d, 0x7b, 0x94, 0xb0, 0xba, 0xc3, 0xb4, 0x99, 0x83,
    0x69, 0x5f, 0x61, 0x66, 0x6f, 0x73, 0x72, 0x70, 0x73, 0x7b, 0x88, 0x9a, 0xa3, 0xa4, 0x9a, 0x83, 0x6e, 0x5b, 0x51,
    0x53, 0x57, 0x5f, 0x6c, 0x7b, 0x8d, 0xa7, 0xb3, 0xba, 0xc2, 0xac, 0x9f, 0x8d, 0x6e, 0x69, 0x59, 0x55, 0x59, 0x5a,
    0x61, 0x68, 0x77, 0x7f, 0x94, 0xa0, 0xa5, 0xb0, 0xa6, 0x9d, 0x90, 0x7d, 0x73, 0x6b, 0x68, 0x69, 0x6c, 0x6e, 0x71,
    0x77, 0x7d, 0x85, 0x8f, 0x94, 0x96, 0x93, 0x87, 0x7b, 0x6c, 0x5f, 0x59, 0x55, 0x57, 0x63, 0x76, 0x8b, 0xa8, 0xb4,
    0xb6, 0xb9, 0xa0, 0x95, 0x88, 0x73, 0x74, 0x68, 0x63, 0x60, 0x5a, 0x5b, 0x5f, 0x6e, 0x79, 0x90, 0x9e, 0xa3, 0xaa,
    0xa0, 0x97, 0x8e, 0x81, 0x7b, 0x76, 0x73, 0x70, 0x6e, 0x6a, 0x6a, 0x70, 0x75, 0x82, 0x8c, 0x92, 0x95, 0x91, 0x87,
    0x7c, 0x72, 0x68, 0x62, 0x5e, 0x5c, 0x5d, 0x6a, 0x7c, 0x91, 0xac, 0xb1, 0xb8, 0xb2, 0x98, 0x92, 0x7b, 0x71, 0x71,
    0x67, 0x69, 0x62, 0x61, 0x5d, 0x62, 0x6f, 0x78, 0x91, 0x9b, 0xa4, 0xa9, 0x9e, 0x97, 0x8b, 0x81, 0x7c, 0x79, 0x78,
    0x74, 0x73, 0x6c, 0x6b, 0x6f, 0x71, 0x7f, 0x88, 0x8f, 0x94, 0x91, 0x89, 0x80, 0x76, 0x6b, 0x67, 0x62, 0x5e, 0x60,
    0x68, 0x79, 0x8c, 0xa6, 0xad, 0xb4, 0xb3, 0x9a, 0x96, 0x82, 0x77, 0x78, 0x6d, 0x6f, 0x67, 0x65, 0x5f, 0x60, 0x6b,
    0x70, 0x87, 0x92, 0x9c, 0xa6, 0x9d, 0x9a, 0x91, 0x88, 0x84, 0x80, 0x80, 0x79, 0x79, 0x70, 0x6a, 0x6e, 0x6b, 0x76,
    0x7f, 0x85, 0x8d, 0x8e, 0x8b, 0x84, 0x7e, 0x74, 0x6f, 0x6b, 0x62, 0x62, 0x63, 0x70, 0x81, 0x94, 0xa9, 0xa9, 0xb1,
    0xa4, 0x93, 0x90, 0x7c, 0x7d, 0x78, 0x74, 0x72, 0x68, 0x66, 0x5b, 0x62, 0x67, 0x72, 0x87, 0x8d, 0x9a, 0x9c, 0x97,
    0x95, 0x8d, 0x8a, 0x86, 0x87, 0x84, 0x7f, 0x7d, 0x6f, 0x6c, 0x6c, 0x6a, 0x75, 0x7b, 0x81, 0x87, 0x88, 0x85, 0x81,
    0x7f, 0x77, 0x74, 0x70, 0x67, 0x66, 0x63, 0x6d, 0x7d, 0x8f, 0xa4, 0xa7, 0xad, 0xa4, 0x92, 0x8f, 0x7f, 0x7f, 0x7d,
    0x7a, 0x78, 0x6d, 0x69, 0x5d, 0x5f, 0x64, 0x6b, 0x7f, 0x88, 0x93, 0x97, 0x94, 0x92, 0x8c, 0x8d, 0x8a, 0x8c, 0x8d,
    0x87, 0x84, 0x78, 0x70, 0x6e, 0x6b, 0x70, 0x77, 0x7c, 0x80, 0x82, 0x81, 0x7e, 0x7f, 0x7d, 0x7c, 0x7a, 0x73, 0x6c,
    0x66, 0x65, 0x6e, 0x7e, 0x92, 0xa5, 0xa9, 0xab, 0xa3, 0x91, 0x8c, 0x82, 0x82, 0x83, 0x82, 0x7d, 0x71, 0x6a, 0x5d,
    0x5c, 0x63, 0x6c, 0x7d, 0x87, 0x8e, 0x90, 0x8d, 0x8b, 0x89, 0x8d, 0x90, 0x93, 0x95, 0x8f, 0x88, 0x7e, 0x74, 0x71,
    0x70, 0x74, 0x78, 0x79, 0x79, 0x77, 0x77, 0x77, 0x7c, 0x81, 0x81, 0x7f, 0x79, 0x6f, 0x68, 0x66, 0x67, 0x75, 0x88,
    0x99, 0xa7, 0xa4, 0xa3, 0x95, 0x8a, 0x8a, 0x85, 0x8b, 0x89, 0x87, 0x7b, 0x6c, 0x65, 0x5a, 0x61, 0x6a, 0x75, 0x80,
    0x83, 0x84, 0x80, 0x80, 0x84, 0x89, 0x93, 0x96, 0x97, 0x94, 0x8b, 0x84, 0x7d, 0x7d, 0x7d, 0x7b, 0x7c, 0x76, 0x6f,
    0x6c, 0x6c, 0x70, 0x76, 0x7f, 0x81, 0x80, 0x7c, 0x72, 0x70, 0x6d, 0x6d, 0x71, 0x77, 0x80, 0x89, 0x95, 0x9a, 0x95,
    0x9b, 0x94, 0x8f, 0x93, 0x8e, 0x8c, 0x83, 0x80, 0x72, 0x6b, 0x6e, 0x67, 0x6d, 0x70, 0x73, 0x73, 0x76, 0x7a, 0x7b,
    0x83, 0x8a, 0x8d, 0x90, 0x91, 0x8e, 0x8b, 0x8e, 0x8d, 0x8c, 0x8c, 0x84, 0x7b, 0x75, 0x6f, 0x6c, 0x6f, 0x73, 0x74,
    0x77, 0x77, 0x75, 0x76, 0x7a, 0x7b, 0x7b, 0x79, 0x76, 0x73, 0x73, 0x74, 0x79, 0x84, 0x94, 0x9f, 0x98, 0x9f, 0x97,
    0x8c, 0x92, 0x90, 0x91, 0x89, 0x89, 0x7d, 0x6d, 0x70, 0x6b, 0x6c, 0x71, 0x76, 0x72, 0x6f, 0x74, 0x74, 0x7a, 0x86,
    0x8c, 0x8c, 0x8d, 0x8d, 0x8c, 0x8f, 0x96, 0x96, 0x91, 0x8a, 0x80, 0x74, 0x72, 0x74, 0x73, 0x74, 0x72, 0x6f, 0x6c,
    0x70, 0x76, 0x7c, 0x7f, 0x7e, 0x7b, 0x77, 0x76, 0x7a, 0x79, 0x77, 0x77, 0x76, 0x7e, 0x92, 0x9d, 0x96, 0x9b, 0x98,
    0x88, 0x8f, 0x94, 0x94, 0x8b, 0x88, 0x80, 0x6f, 0x71, 0x74, 0x75, 0x71, 0x74, 0x6f, 0x69, 0x6f, 0x77, 0x7c, 0x81,
    0x87, 0x82, 0x84, 0x88, 0x8e, 0x94, 0x99, 0x92, 0x88, 0x82, 0x7e, 0x7d, 0x7c, 0x7d, 0x76, 0x70, 0x6c, 0x6d, 0x70,
    0x76, 0x78, 0x78, 0x79, 0x7a, 0x7f, 0x81, 0x84, 0x84, 0x82, 0x80, 0x7a, 0x76, 0x7a, 0x7c, 0x7f, 0x86, 0x8d, 0x90,
    0x87, 0x86, 0x89, 0x8b, 0x8a, 0x83, 0x7b, 0x78, 0x7e, 0x81, 0x81, 0x80, 0x7e, 0x7c, 0x7b, 0x7d, 0x7e, 0x80, 0x80,
    0x7f, 0x7e, 0x7f, 0x80, 0x80, 0x82, 0x81, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x81, 0x7f, 0x7e, 0x7e, 0x7e, 0x7f, 0x80,
    0x7f, 0x7f, 0x7f, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x7e, 0x7e,
    0x7e, 0x7f, 0x80, 0x81, 0x82, 0x82, 0x82, 0x82, 0x82, 0x83, 0x84, 0x82, 0x81, 0x82, 0x83, 0x83, 0x83, 0x81, 0x80,
    0x81, 0x81, 0x81, 0x81, 0x7f, 0x7e, 0x7f, 0x80, 0x7f, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e,
    0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x7e, 0x7e, 0x7e,
    0x7e, 0x7d, 0x7e, 0x7f, 0x81, 0x81, 0x81, 0x81, 0x82, 0x83, 0x83, 0x82, 0x81, 0x81, 0x82, 0x82, 0x81, 0x80, 0x7f,
    0x7f, 0x80, 0x7f, 0x7e, 0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x7c, 0x7c, 0x7d, 0x7e, 0x7d, 0x7d, 0x7d, 0x7d, 0x7e, 0x7e,
    0x7e, 0x7e, 0x7e, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x7f, 0x7f, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x80, 0x80, 0x81, 0x81, 0x81, 0x82, 0x82, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e,
    0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7d, 0x7d, 0x7d, 0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x80, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x81, 0x81, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x80, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x80, 0x7f, 0x81, 0x80, 0x88, 0x85, 0x74, 0x97, 0x72, 0x6a, 0x93, 0x71,
    0x82, 0x7c, 0x82, 0x82, 0x7b, 0x82, 0x7e, 0x8a, 0x82, 0x85, 0x7c, 0x73, 0x7a, 0x7d, 0x71, 0x8a, 0x83, 0x72, 0x92,
    0x75, 0x68, 0x88, 0x82, 0x70, 0x83, 0x86, 0x80, 0x8b, 0x75, 0x7b, 0x8f, 0x83, 0x73, 0x7c, 0x83, 0x75, 0x78, 0x7e,
    0x83, 0x82, 0x7e, 0x7c, 0x85, 0x8e, 0x7c, 0x7a, 0x87, 0x84, 0x7f, 0x7c, 0x81, 0x7e, 0x80, 0x7b, 0x7b, 0x87, 0x7c,
    0x78, 0x7d, 0x84, 0x80, 0x7b, 0x81, 0x81, 0x82, 0x7f, 0x7c, 0x80, 0x84, 0x7d, 0x7e, 0x81, 0x79, 0x7c, 0x7b, 0x7b,
    0x81, 0x7e, 0x7c, 0x86, 0x7f, 0x80, 0x80, 0x80, 0x82, 0x7d, 0x82, 0x79, 0x82, 0x79, 0x7e, 0x88, 0x77, 0x7c, 0x81,
    0x83, 0x7f, 0x81, 0x79, 0x7f, 0x7e, 0x85, 0x92, 0x72, 0x8b, 0x9f, 0x81, 0x82, 0x98, 0x8f, 0x77, 0x8d, 0x8a, 0x7c,
    0x86, 0x77, 0x75, 0x81, 0x7f, 0x69, 0x76, 0x81, 0x6f, 0x73, 0x81, 0x7d, 0x78, 0x80, 0x7e, 0x89, 0x83, 0x7c, 0x85,
    0x8f, 0x86, 0x75, 0x87, 0x8d, 0x7a, 0x79, 0x84, 0x82, 0x81, 0x78, 0x71, 0x81, 0x86, 0x66, 0x6f, 0x8a, 0x7a, 0x69,
    0x7b, 0x88, 0x84, 0x84, 0x7a, 0x88, 0x8b, 0x7a, 0x84, 0x8e, 0x7d, 0x7c, 0x81, 0x7b, 0x80, 0x7f, 0x6f, 0x74, 0x81,
    0x6c, 0x7a, 0x92, 0x90, 0x74, 0x8a, 0xb0, 0x86, 0x81, 0x9e, 0x9a, 0x7e, 0x83, 0x95, 0x7e, 0x7b, 0x7a, 0x78, 0x79,
    0x74, 0x64, 0x6e, 0x7e, 0x64, 0x68, 0x7e, 0x7c, 0x68, 0x79, 0x89, 0x7f, 0x76, 0x84, 0x91, 0x85, 0x8a, 0x8b, 0x99,
    0x98, 0x79, 0x86, 0xa4, 0x7a, 0x66, 0x93, 0x82, 0x5d, 0x79, 0x86, 0x68, 0x6f, 0x77, 0x70, 0x76, 0x6d, 0x66, 0x81,
    0x7f, 0x6d, 0x8c, 0x97, 0x7f, 0x7d, 0x90, 0x8b, 0x85, 0x81, 0x73, 0x84, 0x78, 0x65,

};
#if defined(__cpluscplus)
extern const uint16_t wavSize = sizeof(wavData) / sizeof(wavData[0]);
#else
const uint16_t wavSize        = sizeof(wavData) / sizeof(wavData[0]);
#endif

USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE) uint8_t s_wavBuff[AUDIO_ENDPOINT_MAX_PACKET_SIZE];
uint32_t audioPosition = 0U;
void USB_AudioRecorderGetBuffer(uint8_t *buffer, uint32_t size);
/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Audio wav data prepare function.
 *
 * This function prepare audio wav data before send.
 */
void USB_AudioRecorderGetBuffer(uint8_t *buffer, uint32_t size)
{
    uint8_t k;
    /* copy audio wav data from flash to buffer */
    for (k = 0U; k < size; k++)
    {
        if (audioPosition > (wavSize - 1U))
        {
            audioPosition = 0U;
        }
        *(buffer + k) = wavData[audioPosition];
        audioPosition++;
    }
}
