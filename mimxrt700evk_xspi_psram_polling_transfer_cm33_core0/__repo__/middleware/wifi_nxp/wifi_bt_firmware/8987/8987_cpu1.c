/*
 * Copyright 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>
#include <osa.h>

#if defined(SD8987)

#if (CONFIG_WIFI_IND_DNLD)

const uint8_t fw_cpu1[] = {
#include <sd8987_wlan.bin.inc>
};

const unsigned char *wlan_fw_bin   = (const unsigned char *)(void *)&fw_cpu1[0];
const unsigned int wlan_fw_bin_len = sizeof(fw_cpu1);

#endif

#endif
