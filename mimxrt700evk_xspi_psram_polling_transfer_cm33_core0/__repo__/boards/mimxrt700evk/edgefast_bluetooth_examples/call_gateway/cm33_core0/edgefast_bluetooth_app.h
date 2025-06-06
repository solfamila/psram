/*
 *  Copyright 2024 NXP
 *  All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "edgefast_bluetooth_config.h"
#include "edgefast_bluetooth_audio_config.h"

#define GENERIC_LIST_LIGHT 0

#define PRINTF_FLOAT_ENABLE 1
#define PRINTF_ADVANCED_ENABLE 1

#if defined(WIFI_IW612_BOARD_MURATA_2EL_M2)
#include "wifi_bt_module_config.h"
#include "wifi_config.h"
#else
#error The transceiver module is unsupported
#endif

#if defined(WIFI_IW612_BOARD_MURATA_2EL_M2)
#undef SD_TIMING_MAX
#define SD_TIMING_MAX kSD_TimingDDR50Mode
#endif /*#define WIFI_IW612_BOARD_MURATA_2EL_M2*/

/* CIS */

/* BIS */

/* VCP */

/* MCS */

/* MPL */

/* TBS */

/* CSIP set coordinator */

/* The noise issue will occur if the BT SNOOP feature is enabled */

/* Debug Settings */
#if 0
#undef CONFIG_BT_DEBUG
#define CONFIG_BT_DEBUG 1
#undef CONFIG_BT_DEBUG_TBS_CLIENT
#define CONFIG_BT_DEBUG_TBS_CLIENT 1
#undef CONFIG_BT_DEBUG_ATT
#define CONFIG_BT_DEBUG_ATT 1
#undef CONFIG_BT_DEBUG_CONN
#define CONFIG_BT_DEBUG_CONN 1
#undef CONFIG_BT_DEBUG_GATT
#define CONFIG_BT_DEBUG_GATT 1
#undef CONFIG_BT_DEBUG_HCI_CORE
#define CONFIG_BT_DEBUG_HCI_CORE 1
#undef CONFIG_BT_DEBUG_KEYS
#define CONFIG_BT_DEBUG_KEYS 1
#undef CONFIG_BT_DEBUG_RPA
#define CONFIG_BT_DEBUG_RPA 1
#define CONFIG_NET_BUF_LOG 1
#undef CONFIG_BT_DEBUG_SMP
#define CONFIG_BT_DEBUG_SMP 1
#undef CONFIG_BT_DEBUG_L2CAP
#define CONFIG_BT_DEBUG_L2CAP 1
#define CONFIG_BT_DEBUG_FIFO 1

#define LOG_MAX_BUFF_LOG_COUNT 128

#define BOARD_DEBUG_UART_BAUDRATE 1000000

#endif

#define CONFIG_LITTLE_ENDIAN 1

#define CONFIG_WORK_QUEUE_MSG_QUEUE_COUNT 128

#define LOG_MAX_BUFF_LOG_COUNT 128

