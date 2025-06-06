/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _USB_PD_CONFIG_H_
#define _USB_PD_CONFIG_H_

#define PD_CONFIG_PTN5110_PORT 1
#define PD_CONFIG_MAX_PORT (PD_CONFIG_PTN5110_PORT)
#define PD_CONFIG_PHY_LOW_POWER_LEVEL 0
#define PD_CONFIG_REVISION 2
#define PD_CONFIG_STRUCTURED_VDM_VERSION 1
// #define PD_CONFIG_SOURCE_ROLE_ENABLE 0
#define PD_CONFIG_SINK_ROLE_ENABLE 1
// #define PD_CONFIG_DUAL_POWER_ROLE_ENABLE 0
// #define PD_CONFIG_DUAL_DATA_ROLE_ENABLE 0
// #define PD_CONFIG_ENABLE_AUTO_POLICY 0
// #define PD_CONFIG_ALT_MODE_SUPPORT 0
#define PD_CONFIG_SINK_DETACH_DETECT_WAY 1
// #define PD_CONFIG_VBUS_ALARM_SUPPORT 0
// #define PD_CONFIG_VCONN_SUPPORT 0
// #define PD_CONFIG_SRC_AUTO_DISCOVER_CABLE_PLUG 0
// #define PD_CONFIG_CABLE_COMMUNICATION_ENABLE 0
#define PD_CONFIG_MIN_DISCHARGE_TIME_ENABLE 1
#define PD_CONFIG_VENDOR_DEFINED_MESSAGE_ENABLE 1
// #define PD_CONFIG_COMMON_TASK 0
// #define PD_CONFIG_EXTERNAL_POWER_DETECTION_SUPPORT 0
// #define PD_CONFIG_EXTENDED_MSG_SUPPORT 0
// #define PD_CONFIG_PD3_FAST_ROLE_SWAP_ENABLE 0
#define PD_CONFIG_PD3_AMS_COLLISION_AVOID_ENABLE 1
// #define PD_CONFIG_PD3_PPS_ENABLE 0
// #define PD_CONFIG_TRY_SNK_SUPPORT 0
// #define PD_CONFIG_TRY_SRC_SUPPORT 0
// #define PD_CONFIG_SINK_ACCESSORY_SUPPORT 0
// #define PD_CONFIG_AUDIO_ACCESSORY_SUPPORT 0
// #define PD_CONFIG_DEBUG_ACCESSORY_SUPPORT 0
// #define PD_CONFIG_COMPLIANCE_TEST_ENABLE 0

#endif /* _USB_PD_CONFIG_H_ */
