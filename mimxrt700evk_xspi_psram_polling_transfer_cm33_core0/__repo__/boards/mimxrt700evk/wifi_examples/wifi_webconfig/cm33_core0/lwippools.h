/* @file lwippools.h
 *
 *  @brief This file contains custom LwIP memory pool definitions
 *
 *  Copyright 2020 NXP
 *  All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __LWIPPOOLS_H__
#define __LWIPPOOLS_H__

#ifdef MEMP_USE_CUSTOM_POOLS
/*
 * Place large LwIP memory pools to .wlan_data section to
 * avoid memory overflow of region with bss/zi section.
 */
extern unsigned char __attribute__((section(".wlan_data"))) memp_memory_PBUF_POOL_base[];
extern unsigned char __attribute__((section(".wlan_data"))) memp_memory_TCP_PCB_POOL_base[];

#endif /* MEMP_USE_CUSTOM_POOLS */

#endif /* __LWIPPOOLS_H__ */
