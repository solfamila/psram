/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _EDGEFAST_BLUETOOTH_AUDIO_CONFIG_H_
#define _EDGEFAST_BLUETOOTH_AUDIO_CONFIG_H_

#define CONFIG_BT_AUDIO 1
#define CONFIG_BT_AUDIO_RX 1
#define CONFIG_BT_AUDIO_TX 1
#define CONFIG_BT_AUDIO_NOTIFY_RETRY_DELAY 20
#define CONFIG_BT_CCID 1
#define CONFIG_BT_BAP_UNICAST 1
#define CONFIG_BT_BAP_UNICAST_CLIENT 1
#define CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE 19
#define CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE 4
#define CONFIG_BT_BAP_BASS_MAX_SUBGROUPS 1
#define CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE 19
#define CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE 4
#define CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_COUNT 1
#define CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT 2
#define CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT 2
#define CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT 2
#define CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK 1
#define CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC 1
// #define CONFIG_BT_BAP_BROADCAST_SOURCE 0
// #define CONFIG_BT_BAP_SCAN_DELEGATOR 0
// #define CONFIG_BT_BAP_BROADCAST_ASSISTANT 0
#define CONFIG_BT_BAP_STREAM 1
// #define CONFIG_BT_PAC_SNK 0
// #define CONFIG_BT_PAC_SRC 0
// #define CONFIG_BT_ASCS 0
#define CONFIG_BT_VOCS_MAX_INSTANCE_COUNT 0
#define CONFIG_BT_VOCS_CLIENT_MAX_INSTANCE_COUNT 0
#define CONFIG_BT_AICS_MAX_INSTANCE_COUNT 0
#define CONFIG_BT_AICS_CLIENT_MAX_INSTANCE_COUNT 0
// #define CONFIG_BT_VCP_VOL_REND 0
#define CONFIG_BT_VCP_VOL_CTLR 1
#define CONFIG_BT_VCP_VOL_CTLR_MAX_VOCS_INST 0
#define CONFIG_BT_VCP_VOL_CTLR_MAX_AICS_INST 0
// #define CONFIG_BT_MICP_MIC_DEV 0
// #define CONFIG_BT_MICP_MIC_CTLR 0
// #define CONFIG_BT_CSIP_SET_MEMBER 0
#define CONFIG_BT_CSIP_SET_COORDINATOR 1
// #define CONFIG_BT_CSIP_SET_COORDINATOR_TEST_SAMPLE_DATA 0
#define CONFIG_BT_CSIP_SET_COORDINATOR_MAX_CSIS_INSTANCES 1
#define CONFIG_BT_CSIP_SET_COORDINATOR_ENC_SIRK_SUPPORT 1
#define CONFIG_BT_TBS 1
#define CONFIG_BT_GTBS 1
#define CONFIG_BT_TBS_PROVIDER_NAME "Unknown"
#define CONFIG_BT_TBS_UCI "un000"
#define CONFIG_BT_TBS_TECHNOLOGY 1
#define CONFIG_BT_TBS_URI_SCHEMES_LIST "tel,skype"
#define CONFIG_BT_TBS_SIGNAL_STRENGTH_INTERVAL 0
#define CONFIG_BT_TBS_STATUS_FLAGS 0
#define CONFIG_BT_TBS_SUPPORTED_FEATURES 1
#define CONFIG_BT_TBS_MAX_CALLS 3
#define CONFIG_BT_TBS_BEARER_COUNT 1
#define CONFIG_BT_TBS_SERVICE_COUNT 1
#define CONFIG_BT_TBS_MAX_SCHEME_LIST_LENGTH 30
// #define CONFIG_BT_TBS_AUTHORIZATION 0
// #define CONFIG_BT_TBS_CLIENT_GTBS 0
// #define CONFIG_BT_TBS_CLIENT_TBS 0
#define CONFIG_BT_TBS_MAX_URI_LENGTH 30
#define CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH 30
#define CONFIG_BT_MCS 1
// #define CONFIG_BT_MCC 0
// #define CONFIG_BT_HAS_CLIENT 0
#define CONFIG_BT_MPL 1
#define CONFIG_BT_MPL_MEDIA_PLAYER_NAME "Player0"
#define CONFIG_BT_MPL_MEDIA_PLAYER_NAME_MAX 20
#define CONFIG_BT_MPL_ICON_URL "http://server.some.where/path/icon.png"
#define CONFIG_BT_MPL_ICON_URL_MAX 40
#define CONFIG_BT_MPL_TRACK_TITLE_MAX 40
#define CONFIG_BT_MPL_SEGMENT_NAME_MAX 25
#define CONFIG_BT_MPL_GROUP_TITLE_MAX 40
#define CONFIG_MCTL 1
#define CONFIG_MCTL_LOCAL_PLAYER_CONTROL 1
#define CONFIG_MCTL_LOCAL_PLAYER_LOCAL_CONTROL 1
#define CONFIG_MCTL_LOCAL_PLAYER_REMOTE_CONTROL 1
// #define CONFIG_BT_CAP_INITIATOR 0
// #define CONFIG_BT_CAP_COMMANDER 0
// #define CONFIG_BT_PBP 0
// #define LE_AUDIO_SYNC_ENABLE 0
// #define CONFIG_BT_AUDIO_CAPABILITY 0
#define CONFIG_BT_AUDIO_BROADCAST_SRC_SUBGROUP_COUNT 1
// #define CONFIG_BT_AUDIO_BROADCAST_SOURCE 0
// #define CONFIG_BT_AUDIO_BROADCAST_SINK 0
#define CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT 2
#define CONFIG_UTF8 1
#define LE_CONN_COUNT 0
#define CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT 0
#define CONFIG_BT_PACS_SNK_CONTEXT 0x0007
#define CONFIG_BT_PACS_SRC_CONTEXT 0x0007
// #define CONFIG_TMAP_PERIPHERAL_SINGLE 0
// #define CONFIG_BT_OTS 0
// #define CONFIG_BT_OTS_CLIENT 0

#endif /* _EDGEFAST_BLUETOOTH_AUDIO_CONFIG_H_ */
