/** @file
 *  @brief Service Discovery Protocol handling.
 */

/*
 * Copyright 2021, 2024 NXP
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <porting.h>
#include <errno/errno.h>
#include <zephyr/types.h>
#include <sys/byteorder.h>

#include <bluetooth/sdp.h>

#define LOG_ENABLE IS_ENABLED(CONFIG_BT_DEBUG_SDP)
#define LOG_MODULE_NAME bt_sdp
#include "fsl_component_log.h"
LOG_MODULE_DEFINE(LOG_MODULE_NAME, kLOG_LevelTrace);

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
#include "BT_common.h"
#include "bt_pal_hci_core.h"
#include "bt_pal_conn_internal.h"
#include "bt_pal_l2cap_br_internal.h"
#include "bt_pal_sdp_internal.h"
#include "BT_hci_api.h"
#include "BT_sdp_api.h"
#include "db_gen.h"

#define SDP_PSM 0x0001U

#define SDP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_sdp, chan.chan)

#define SDP_DATA_MTU 200

#define SDP_MTU (SDP_DATA_MTU + sizeof(struct bt_sdp_hdr))

#define MAX_NUM_ATT_ID_FILTER 10

#define SDP_SERVICE_HANDLE_BASE 0x10000

#define SDP_DATA_ELEM_NEST_LEVEL_MAX 5

/* Size of Cont state length */
#define SDP_CONT_STATE_LEN_SIZE 1

/* 1 byte for the no. of services searched till this response */
/* 2 bytes for the total no. of matching records */
#define SDP_SS_CONT_STATE_SIZE 3

/* 1 byte for the no. of attributes searched till this response */
#define SDP_SA_CONT_STATE_SIZE 1

/* 1 byte for the no. of services searched till this response */
/* 1 byte for the no. of attributes searched till this response */
#define SDP_SSA_CONT_STATE_SIZE 2

#define SDP_INVALID 0xff

struct bt_sdp_client {
	SDP_CB 						sdb_cb;
	struct bt_conn  				*conn;
	SDP_HANDLE 					sdp_handle;
	/* list of waiting to be resolved UUID params */
	bt_list_t					reqs;
	/* required SDP transaction ID */
	/* uint16_t					tid;*/
	/* UUID params holder being now resolved */
	const struct bt_sdp_discover_params *param;
	/* PDU continuation state object */
	struct bt_sdp_pdu_cstate			cstate;
	/* buffer for collecting record data */
	struct net_buf					*rec_buf;
	/* Buffer for SDP discovery */
	struct net_buf					*buf;
	/* Worker */
	struct k_work					recv;
};

#if (CONFIG_BT_MAX_CONN > BT_MAX_REMOTE_DEVICES)
#error "The library cannot support CONFIG_BT_MAX_CONN"
#endif

static void ethermind_sdp_callback(struct bt_sdp_client *session, uint8_t command, uint8_t * data,
	uint16_t length, uint16_t status);

#define _BT_SDP_CLIENT_CB_DECLARE(index, func)					\
	static void func##index(uint8_t command, uint8_t *data, uint16_t length, uint16_t status)

#define BT_SDP_CLIENT_CB_DECLARE(index, ...) _BT_SDP_CLIENT_CB_DECLARE(index, ##__VA_ARGS__)

LISTIFY(CONFIG_BT_MAX_CONN, BT_SDP_CLIENT_CB_DECLARE, (;), ethermind_sdp_callback);

#define _BT_SDP_CLIENT_DECLARE(index, func) {.sdb_cb = func##index}
#define BT_SDP_CLIENT_DECLARE(index, ...) _BT_SDP_CLIENT_DECLARE(index, ##__VA_ARGS__)

static struct bt_sdp_client bt_sdp_client_pool[CONFIG_BT_MAX_CONN] = {
	LISTIFY(CONFIG_BT_MAX_CONN, BT_SDP_CLIENT_DECLARE, (,), ethermind_sdp_callback),
};

#define _BT_SDP_CLIENT_CB_DEFINE(index, func)					\
	static void func##index(uint8_t command, uint8_t *data, 		\
		uint16_t length, uint16_t status)				\
	{									\
		func(&bt_sdp_client_pool[index],				\
			command, data, length, status);				\
	}
#define BT_SDP_CLIENT_CB_DEFINE(index, ...) _BT_SDP_CLIENT_CB_DEFINE(index, ##__VA_ARGS__)

LISTIFY(CONFIG_BT_MAX_CONN, BT_SDP_CLIENT_CB_DEFINE, (;), ethermind_sdp_callback);

/* SDP Attribute data size */
#define SDP_ATTRIB_DATALEN		1024
/* SDP Attribute Data */
struct bt_sdp_meta {
	uint16_t status;
};
NET_BUF_POOL_FIXED_DEFINE(sdp_pool, 1, SDP_ATTRIB_DATALEN, sizeof(struct bt_sdp_meta), NULL);

#define BT_SDP(buf) ((struct bt_sdp_meta *)net_buf_user_data(buf))

#define SDP_BUFF_RESERVE_FOR_HEAD_LEN (9U)

static void sdp_client_params_iterator(struct bt_sdp_client *session);

enum uuid_state {
	UUID_NOT_RESOLVED,
	UUID_RESOLVED,
};

static void sdp_client_notify_result(struct bt_sdp_client *session, enum uuid_state state);
#ifdef SDP_DYNAMIC_DB
static uint32_t lookfor_uuid_16(struct bt_sdp_data_elem *elem, uint16_t *uuid_16,
             uint8_t *count, uint8_t nest_level)
{
    const uint8_t *cur_elem;
    uint32_t seq_size, size;

    /* Limit recursion depth to avoid stack overflows */
    if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
        return 0;
    }

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_UUID_UNSPEC) {
        if (seq_size == 2U) {
            *uuid_16 = *((uint16_t *)cur_elem);
            *count = *count + 1;
            uuid_16++;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_SEQ_UNSPEC ||
        (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_ALT_UNSPEC) {
        do {

            /* Recursively parse data elements */
            size = lookfor_uuid_16((struct bt_sdp_data_elem *)cur_elem,
                       uuid_16 + *count, count, nest_level + 1);
            cur_elem += sizeof(struct bt_sdp_data_elem);
            seq_size -= size;
        } while (seq_size);
    }

    return elem->total_size;
}

static uint32_t lookfor_service_uuids(struct bt_sdp_data_elem *elem, DB_SERVICE_CLASS_UUID_ELEM *service_uuids,
             uint8_t *count, uint8_t nest_level)
{
    const uint8_t *cur_elem;
    uint32_t seq_size, size;

    /* Limit recursion depth to avoid stack overflows */
    if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
        return 0;
    }

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_UUID_UNSPEC) {
         /* For BT_SDP_UUID16 case */
        if (seq_size == 2U) {
            service_uuids[*count].uuid_len = 2U;
            service_uuids[*count].uuid_16 = *((uint16_t *)cur_elem);
            *count = *count + 1;
        } else if (seq_size == 16U) {
            /* For BT_SDP_UUID128 case */
            service_uuids[*count].uuid_len = 16U;
            memcpy(&service_uuids[*count].uuid_128[0], cur_elem, 16U);
            *count = *count + 1;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_SEQ_UNSPEC ||
        (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_ALT_UNSPEC) {
        do {

            /* Recursively parse data elements */
            size = lookfor_service_uuids((struct bt_sdp_data_elem *)cur_elem,
                       service_uuids, count, nest_level + 1);
            cur_elem += sizeof(struct bt_sdp_data_elem);
            seq_size -= size;
        } while (seq_size);
    }

    return elem->total_size;
}

static uint32_t lookfor_languagebase_attr_id(struct bt_sdp_data_elem *elem, uint16_t *language,
               uint16_t *char_enc,
               uint16_t *base_id, uint8_t *count, uint8_t nest_level)
{
    const uint8_t *cur_elem;
    uint32_t seq_size, size;

    /* Limit recursion depth to avoid stack overflows */
    if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
        return 0;
    }

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if ((elem->type ) == BT_SDP_UINT16) {
        if (seq_size == 2U) {
            if (*count == 0) {
                *language = *((uint16_t *)cur_elem);
            }else if (*count == 1) {
                *char_enc = *((uint16_t *)cur_elem);
            }else if (*count == 2) {
                *base_id = *((uint16_t *)cur_elem);
            }
            *count = *count +1;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_SEQ_UNSPEC ||
        (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_ALT_UNSPEC) {
        do {
            /* Recursively parse data elements */
            size = lookfor_languagebase_attr_id((struct bt_sdp_data_elem *)cur_elem,
                       language, char_enc, base_id, count, nest_level + 1);
            cur_elem += sizeof(struct bt_sdp_data_elem);
            seq_size -= size;
        } while (seq_size);
    }

    return elem->total_size;
}
#ifndef SDP_DB_ADD_PROFILE_DESC_LIST_UUID_128_BIT_SUPPORT
static uint32_t lookfor_profile_descriptor_list(struct bt_sdp_data_elem *elem, uint16_t *profile_uuid,
               uint16_t *version, uint8_t *count, uint8_t nest_level)
{
    const uint8_t *cur_elem;
    uint32_t seq_size, size;

    /* Limit recursion depth to avoid stack overflows */
    if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
        return 0;
    }

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if (((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_UUID_UNSPEC) || (elem->type == BT_SDP_UINT16)) {
        if (seq_size == 2U) {
            if (*count == 0) {
                *profile_uuid = *((uint16_t *)cur_elem);
            }else if (*count == 1){
                *version = *((uint16_t *)cur_elem);
            }
            *count = *count +1;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_SEQ_UNSPEC ||
        (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_ALT_UNSPEC) {
        do {
            /* Recursively parse data elements */
            size = lookfor_profile_descriptor_list((struct bt_sdp_data_elem *)cur_elem,
                       profile_uuid, version, count, nest_level + 1);
            cur_elem += sizeof(struct bt_sdp_data_elem);
            seq_size -= size;
        } while (seq_size);
    }

    return elem->total_size;
}
#else
static uint32_t lookfor_profile_descriptor_list_ex(struct bt_sdp_data_elem *elem, S_UUID *profile_uuid,
               uint16_t *version, uint8_t *count, uint8_t nest_level)
{
    const uint8_t *cur_elem;
    uint32_t seq_size, size;

    /* Limit recursion depth to avoid stack overflows */
    if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
        return 0;
    }

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if (((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_UUID_UNSPEC) || (elem->type == BT_SDP_UINT16)) {
        if (seq_size == 2U) {
            if (*count == 0) {
                profile_uuid->uuid_type = UUID_16;
                memcpy(&profile_uuid->uuid_union.uuid_16, cur_elem, 2U);
            } else if (*count == 1){  
                *version = *((uint16_t *)cur_elem); 
            }
            *count = *count +1;
        } else if (seq_size == 16U) {
            if (*count == 0) {
                profile_uuid->uuid_type = UUID_128;
                 memcpy(&profile_uuid->uuid_union.uuid_128, cur_elem, 16U);
            }
            *count = *count +1;
        } else { 
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_SEQ_UNSPEC ||
        (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_ALT_UNSPEC) {
        do {
            /* Recursively parse data elements */
            size = lookfor_profile_descriptor_list_ex((struct bt_sdp_data_elem *)cur_elem,
                       profile_uuid, version, count, nest_level + 1);
            cur_elem += sizeof(struct bt_sdp_data_elem);
            seq_size -= size;
        } while (seq_size);
    }

    return elem->total_size;
}
#endif
static uint32_t lookfor_service_name(struct bt_sdp_data_elem *elem,
               uint8_t *name, uint8_t *count)
{
    const uint8_t *cur_elem;
    uint32_t seq_size;

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if (elem->type == BT_SDP_TEXT_STR8) {
        if (seq_size > 0U) {
           memcpy(name, cur_elem, seq_size + 1);
           *count = seq_size + 1;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    return elem->total_size;
}

static uint32_t lookfor_uint8_data(struct bt_sdp_data_elem *elem,
               uint8_t *uint8_data, uint8_t *count)
{
    const uint8_t *cur_elem;
    uint32_t seq_size;

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if (elem->type == BT_SDP_UINT8) {
        if (seq_size > 0U) {
           memcpy(uint8_data, cur_elem, seq_size);
           *count = seq_size;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    return elem->total_size;
}

static uint32_t lookfor_uint16_data(struct bt_sdp_data_elem *elem,
               uint8_t *uint16_data, uint8_t *count)
{
    const uint8_t *cur_elem;
    uint32_t seq_size;

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if (elem->type == BT_SDP_UINT16) {
        if (seq_size > 0U) {
           memcpy(uint16_data, cur_elem, seq_size);
           sys_mem_swap(uint16_data, seq_size);
           *count = seq_size;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    return elem->total_size;
}

static uint32_t lookfor_uint32_data(struct bt_sdp_data_elem *elem,
               uint8_t *uint32_data, uint8_t *count)
{
    const uint8_t *cur_elem;
    uint32_t seq_size;

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if (elem->type == BT_SDP_UINT32) {
        if (seq_size > 0U) {
           memcpy(uint32_data, cur_elem, seq_size);
           sys_mem_swap(uint32_data, seq_size);
           *count = seq_size;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    return elem->total_size;
}

static uint32_t lookfor_supp_features(struct bt_sdp_data_elem *elem,
               uint8_t *supp_features, uint8_t *count)
{
    const uint16_t *cur_elem;
    uint32_t seq_size;

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if (elem->type == BT_SDP_UINT16) {
        if (seq_size > 0U) {
           sys_put_be16(*cur_elem, supp_features);
           *count = seq_size;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    return elem->total_size;
}
static uint32_t lookfor_add_proto_desc_list(struct bt_sdp_data_elem *elem,
               DB_PROTOCOL_ELEM *db_pro_elem, uint8_t *count, uint8_t nest_level)
{
    const uint8_t *cur_elem;
    uint32_t seq_size, size;


    /* Limit recursion depth to avoid stack overflows */
    if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
        return 0;
    }

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_UUID_UNSPEC) {
        db_pro_elem = db_pro_elem + *count;
        if (seq_size == 2U) {
            db_pro_elem->protocol_uuid = *((uint16_t *)cur_elem);
            *count = *count + 1;
         } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }
    if (elem->type == BT_SDP_UINT16) {
        db_pro_elem = db_pro_elem + (*count -1);
        if (seq_size == 2U) {
            db_pro_elem->params[db_pro_elem->num_params] = *((uint16_t *)cur_elem);
            db_pro_elem->num_params += 1;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }
    if (elem->type == BT_SDP_UINT8) {
        db_pro_elem = db_pro_elem + (*count -1);
        if (seq_size == 1U) {
            db_pro_elem->params[db_pro_elem->num_params] = *((uint8_t *)cur_elem);
            db_pro_elem->num_params += 1;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }
    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_SEQ_UNSPEC ||
        (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_ALT_UNSPEC) {
        do {
            /* Recursively parse data elements */
            size = lookfor_add_proto_desc_list((struct bt_sdp_data_elem *)cur_elem,
                                               db_pro_elem, count, nest_level + 1);
            cur_elem += sizeof(struct bt_sdp_data_elem);
            seq_size -= size;
        } while (seq_size);
    }

    return elem->total_size;
}

static uint32_t lookfor_additional_proto_list_elems(struct bt_sdp_data_elem *elem,
               DB_PROTO_LIST_ELEM *db_pro_list_elem, uint8_t nest_level)
{
    const uint8_t *cur_elem;
    uint32_t seq_size, size;

    /* Limit recursion depth to avoid stack overflows */
    if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
        return 0;
    }

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_UUID_UNSPEC) {
        if (seq_size == 2U) {
            db_pro_list_elem->elem[db_pro_list_elem->num_elems].protocol_uuid = *((uint16_t *)cur_elem);
            db_pro_list_elem->num_elems += 1;
         } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }
    if (elem->type == BT_SDP_UINT16) {
        if (seq_size == 2U) {
            db_pro_list_elem->elem[db_pro_list_elem->num_elems - 1].params[db_pro_list_elem->elem[db_pro_list_elem->num_elems - 1].num_params] = *((uint16_t *)cur_elem);
            db_pro_list_elem->elem[db_pro_list_elem->num_elems - 1].num_params += 1;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }
    if (elem->type == BT_SDP_UINT8) {
        if (seq_size == 1U) {
            db_pro_list_elem->elem[db_pro_list_elem->num_elems - 1].params[db_pro_list_elem->elem[db_pro_list_elem->num_elems - 1].num_params] = *((uint16_t *)cur_elem);
            db_pro_list_elem->elem[db_pro_list_elem->num_elems - 1].num_params += 1;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }
    if ((elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_SEQ_UNSPEC ||
        (elem->type & BT_SDP_TYPE_DESC_MASK) == BT_SDP_ALT_UNSPEC) {
        do {
            /* Recursively parse data elements */
            size = lookfor_additional_proto_list_elems((struct bt_sdp_data_elem *)cur_elem,
                                               db_pro_list_elem, nest_level + 1);
            cur_elem += sizeof(struct bt_sdp_data_elem);
            seq_size -= size;
        } while (seq_size);
    }

    return elem->total_size;
}
static uint32_t lookfor_url_buf(struct bt_sdp_data_elem *elem,
               uint8_t *url_buf, uint8_t *count, uint8_t nest_level)
{
    const uint8_t *cur_elem;
    uint32_t seq_size;

    /* Limit recursion depth to avoid stack overflows */
    if (nest_level == SDP_DATA_ELEM_NEST_LEVEL_MAX) {
        return 0;
    }

    seq_size = elem->data_size;
    cur_elem = elem->data;

    if (elem->type == BT_SDP_TEXT_STR8) {
        if (seq_size > 0U) {
           memcpy(url_buf, cur_elem, seq_size);
           *count = seq_size;
        } else {
            LOG_WRN("Invalid UUID size in local database");
            assert(0);
        }
    }

    return elem->total_size;
}
#else
#if (defined(CONFIG_BT_AVRCP) && ((CONFIG_BT_AVRCP) > 0U))
static uint8_t ct_additional_protocol_descriptor_list[] =
{0x35, 0x12, 0x35, 0x10, 0x35, 0x06, 0x19, 0x01, 0x00, 0x09,
 0x00, 0x1B, 0x35, 0x06, 0x19, 0x00, 0x17, 0x09, 0x01, 0x04};

static uint8_t tg_additional_protocol_descriptor_list[] =
{0x35, 0x12, 0x35, 0x10, 0x35, 0x06, 0x19, 0x01, 0x00, 0x09,
 0x00, 0x1B, 0x35, 0x06, 0x19, 0x00, 0x17, 0x09, 0x01, 0x04};

static uint8_t tg_cover_art_additional_protocol_desc_list[] =
{0x35, 0x21, 0x35, 0x10, 0x35, 0x06, 0x19, 0x01, 0x00, 0x09, 0x00, 0x1B,
 0x35, 0x06, 0x19, 0x00, 0x17, 0x09, 0x01, 0x04, 0x35, 0x0D, 0x35, 0x06,
 0x19, 0x01, 0x00, 0x09, (uint8_t)(0x1005u >> 8),
 (uint8_t)0x1005u, 0x35, 0x03, 0x19, 0x00, 0x08};
#endif

extern const uint8_t uuid_indices_arr[];
extern SDP_RECORD dbase[DB_MAX_RECORDS];
extern struct SDP_ATTR attr_arr[];
#endif /* SDP_DYNAMIC_DB */

void bt_sdp_init(void)
{
#if (CONFIG_BT_MAX_CONN >= 1U)
		bt_sdp_client_pool[0].sdb_cb = ethermind_sdp_callback0;
#endif
#if (CONFIG_BT_MAX_CONN >= 2U)
		bt_sdp_client_pool[1].sdb_cb = ethermind_sdp_callback1;
#endif
#if (CONFIG_BT_MAX_CONN >= 3U)
		bt_sdp_client_pool[2].sdb_cb = ethermind_sdp_callback2;
#endif
#if (CONFIG_BT_MAX_CONN >= 4U)
		bt_sdp_client_pool[3].sdb_cb = ethermind_sdp_callback3;
#endif
#if (CONFIG_BT_MAX_CONN >= 5U)
		bt_sdp_client_pool[4].sdb_cb = ethermind_sdp_callback4;
#endif
#if (CONFIG_BT_MAX_CONN >= 6U)
		bt_sdp_client_pool[5].sdb_cb = ethermind_sdp_callback5;
#endif
#if (CONFIG_BT_MAX_CONN >= 7U)
		bt_sdp_client_pool[6].sdb_cb = ethermind_sdp_callback6;
#endif
#if (CONFIG_BT_MAX_CONN > 7U)
#error "please add the callback instances"
#endif

}

int bt_sdp_register_service(struct bt_sdp_record *service)
{
#ifdef SDP_DYNAMIC_DB
    UINT32 record_handle;
    API_RESULT retval;
    uint32_t index = 0;
    uint16_t serviceID;
    DB_SERVICE_CLASS_UUID_ELEM service_uuids[5] = {0};
    uint16_t browse_group_uuids[5] = {0};
    uint8_t count;
    uint16_t language;
    uint16_t char_enc;
    uint16_t base_id;
#ifndef SDP_DB_ADD_PROFILE_DESC_LIST_UUID_128_BIT_SUPPORT 
    uint16_t profile_uuid;
#else
    S_UUID  profile_uuid;
#endif
    uint16_t version;
    uint8_t service_name[20] = {0};
    uint8_t url_buf[50] = {0};
    DB_PROTOCOL_ELEM elems[10U] = {0};
    DB_PROTO_LIST_ELEM list_elems = {0};
    uint8_t supp_features_buf[4] = {0};
    uint8_t uint8_data = 0;
    uint8_t l2cap_psm[2] = {0};
    uint8_t value_network;
    /* Create Record */
    for (index = 0; index < service->attr_count; index++)
    {
        if (service->attrs[index].id == BT_SDP_ATTR_SVCLASS_ID_LIST)
        {
            count = 0;
            lookfor_service_uuids(&service->attrs[index].val, &service_uuids[0], &count, 1);
            switch(service_uuids[0].uuid_16)
            {
                case BT_SDP_PBAP_PSE_SVCLASS:
                    retval = BT_dbase_create_record(DB_RECORD_PBAP_PSE, 0U, &record_handle);
                    break;

                case BT_SDP_PBAP_PCE_SVCLASS:
                    retval = BT_dbase_create_record(DB_RECORD_PBAP_PCE, 0U, &record_handle);
                    break;

                case BT_SDP_MAP_MSE_SVCLASS:
                    for (index = 0; index < MAP_MAS_NUM_ENTITIES; index++)
                    {
                            retval = BT_dbase_get_record_handle(DB_RECORD_MAP_MSE, index, &record_handle);
                            if(retval != API_SUCCESS)
                            {
                                    break;
                            }
                    }
                    if(index < MAP_MAS_NUM_ENTITIES)
                    {
                            retval = BT_dbase_create_record(DB_RECORD_MAP_MSE, index, &record_handle);
                    }
                    else
                    {
                            return -EBUSY;
                    }
                    break;
                case BT_SDP_MAP_MCE_SVCLASS:
                    retval = BT_dbase_create_record(DB_RECORD_MAP_MCE, 0U, &record_handle);
                    break;
                default :
                    retval = BT_dbase_create_record(DB_RECORD_SDP, 0U, &record_handle);
                    break;
            }
            if (API_SUCCESS != retval)
            {
                    LOG_ERR("BT_dbase_create_record FAILED");
                    return -1;
            }
            retval = BT_dbase_add_service_class_id_list_ex
            (
                record_handle,
                count,
                service_uuids
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_dbase_add_service_class_id_list_ex FAILED");
                return -1;
            }
            break;
        }
    }
    while (index < service->attr_count)
    {
        serviceID = service->attrs[index].id;
        count = 0;
        switch (serviceID)
        {
        case BT_SDP_ATTR_ADD_PROTO_DESC_LIST:
            lookfor_additional_proto_list_elems(&service->attrs[index].val,
               &list_elems, 1);

            retval =  BT_dbase_add_additional_proto_desc_list
                      (
                          record_handle,
                          1,
                          &list_elems
                      );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_dbase_add_additional_proto_desc_list FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_PROTO_DESC_LIST:
           lookfor_add_proto_desc_list(&service->attrs[index].val,
               elems, &count, 1);

            retval =  BT_dbase_add_proto_desc_list
                      (
                          record_handle,
                          count,
                          elems
                      );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_dbase_add_proto_desc_list FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_PROFILE_DESC_LIST:
#ifndef SDP_DB_ADD_PROFILE_DESC_LIST_UUID_128_BIT_SUPPORT     
            lookfor_profile_descriptor_list(&service->attrs[index].val, &profile_uuid,
               &version,  &count, 1);
            retval = BT_dbase_add_profile_descriptor_list
                      (
                          record_handle,
                          profile_uuid,
                          version
                      );
#else
           lookfor_profile_descriptor_list_ex(&service->attrs[index].val, &profile_uuid,
                &version,  &count, 1);
            retval = BT_dbase_add_profile_descriptor_list_ex
                      (
                          record_handle, 
                          &profile_uuid,
                          version
                      );
#endif
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_dbase_add_profile_descriptor_list FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST:
            lookfor_languagebase_attr_id(&service->attrs[index].val, &language,
                &char_enc, &base_id, &count, 1);
            retval =  BT_dbase_add_languagebase_attr_id_list
            (
              record_handle,
              language,
              char_enc,
              base_id
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_dbase_add_languagebase_attr_id_list FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_BROWSE_GRP_LIST:
            lookfor_uuid_16(&service->attrs[index].val,
                &browse_group_uuids[0], &count, 1);
            retval = BT_dbase_add_browse_group_list
            (
                record_handle,
                count,
                browse_group_uuids
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_dbase_add_browse_group_list FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_SVCNAME_PRIMARY:
            lookfor_service_name(&service->attrs[index].val,
                service_name, &count);
            retval = BT_dbase_add_service_name
            (
                record_handle,
                count,
                service_name
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_dbase_add_service_name FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_SUPPORTED_FEATURES:
            lookfor_supp_features(&service->attrs[index].val,
                supp_features_buf, &count);
            retval = BT_dbase_add_attribute_type_uint
            (
                record_handle,
                BT_SDP_ATTR_SUPPORTED_FEATURES,
                count,
                supp_features_buf
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_SDP_ATTR_SUPPORTED_FEATURES BT_dbase_add_attribute_type_uint  FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_PBAP_SUPPORTED_FEATURES:
            lookfor_uint32_data(&service->attrs[index].val,
                supp_features_buf, &count);
            retval = BT_dbase_add_attribute_type_uint
            (
                record_handle,
                BT_SDP_ATTR_PBAP_SUPPORTED_FEATURES,
                count,
                supp_features_buf
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_SDP_ATTR_SUPPORTED_FEATURES BT_SDP_ATTR_PBAP_SUPPORTED_FEATURES  FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_DOC_URL:
        case BT_SDP_ATTR_CLNT_EXEC_URL:
        case BT_SDP_ATTR_ICON_URL:
            lookfor_url_buf(&service->attrs[index].val,
                url_buf, &count, 1);
            retval = BT_dbase_add_attribute_type_url
            (
                record_handle,
                serviceID,
                count,
                url_buf
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_dbase_add_attribute_type_url FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_MAS_INSTANCE_ID:
            lookfor_uint8_data(&service->attrs[index].val,
                &uint8_data, &count);
            retval = BT_dbase_add_attribute_type_uint
            (
                record_handle,
                BT_SDP_ATTR_MAS_INSTANCE_ID,
                count,
                &uint8_data
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_SDP_ATTR_SUPPORTED_FEATURES BT_SDP_ATTR_MAS_INSTANCE_ID  FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_SUPPORTED_MESSAGE_TYPES:
            lookfor_uint8_data(&service->attrs[index].val,
                &uint8_data, &count);
            retval = BT_dbase_add_attribute_type_uint
            (
                record_handle,
                BT_SDP_ATTR_SUPPORTED_MESSAGE_TYPES,
                count,
                &uint8_data
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_SDP_ATTR_SUPPORTED_FEATURES BT_SDP_ATTR_SUPPORTED_MESSAGE_TYPES  FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_GOEP_L2CAP_PSM:
            lookfor_uint16_data(&service->attrs[index].val,
                l2cap_psm, &count);
            retval = BT_dbase_add_attribute_type_uint
            (
                record_handle,
                BT_SDP_ATTR_GOEP_L2CAP_PSM,
                count,
                l2cap_psm
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_SDP_ATTR_SUPPORTED_FEATURES BT_SDP_ATTR_GOEP_L2CAP_PSM  FAILED");
                return -1;
            }
            break;

        case BT_SDP_ATTR_SUPPORTED_REPOSITORIES:
            lookfor_uint8_data(&service->attrs[index].val,
                &uint8_data, &count);
            retval = BT_dbase_add_attribute_type_uint
            (
                record_handle,
                BT_SDP_ATTR_SUPPORTED_REPOSITORIES,
                count,
                &uint8_data
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_SDP_ATTR_SUPPORTED_FEATURES BT_SDP_ATTR_SUPPORTED_REPOSITORIES  FAILED");
                return -1;
            }
            break;
        case BT_SDP_ATTR_EXTERNAL_NETWORK:
            lookfor_uint8_data(&service->attrs[index].val,
                &value_network, &count);
            retval = BT_dbase_add_attribute_type_uint
            (
                record_handle,
                BT_SDP_ATTR_EXTERNAL_NETWORK,
                count,
                &value_network
            );
            if (API_SUCCESS != retval)
            {
                LOG_ERR("BT_dbase_add_attribute_type_uint BT_SDP_ATTR_EXTERNAL_NETWORK  FAILED");
                return -1;
            }
            break;
        default:
          /* MISRA 16.4 : The switch statement does not have a non-empty default clause. */
            break;
        }
        index++;
    }

    /* Activate the record */
    BT_dbase_activate_record(record_handle);

    return 0;
#else
	uint32_t record_handle = 0xFFFFFFFFu;
	uint32_t index;
    uint16_t tempVal;

	for (index = 0; index < service->attr_count; index++)
	{
		if ((service->attrs[index].id == BT_SDP_ATTR_SVCLASS_ID_LIST) &&
			(service->attrs[index].val.data != NULL) &&
			(((struct bt_sdp_data_elem *)(service->attrs[index].val.data))->data != NULL))
		{
			tempVal = ((uint8_t *)(((struct bt_sdp_data_elem *)(service->attrs[index].val.data))->data))[0];
			tempVal |= ((uint8_t *)(((struct bt_sdp_data_elem *)(service->attrs[index].val.data))->data))[1] << 8U;
			switch (tempVal) {
			case BT_SDP_AUDIO_SINK_SVCLASS:
				BT_dbase_get_record_handle(DB_RECORD_A2DP_SINK, 0,&record_handle);
				break;
			case BT_SDP_AUDIO_SOURCE_SVCLASS:
				BT_dbase_get_record_handle(DB_RECORD_A2DP_SOURCE, 0,&record_handle);
				break;
			case BT_SDP_SERIAL_PORT_SVCLASS:
			{
				uint32_t channel = 0U;
				uint32_t index2;
				struct bt_sdp_data_elem *dataEle;
				struct bt_sdp_data_elem *dataEle2;
				uint32_t total_size;
				uint32_t dataEleIndex;
				uint8_t found = 0;

				for (index2 = 0; index2 < service->attr_count; index2++) {
					if ((service->attrs[index2].id == BT_SDP_ATTR_PROTO_DESC_LIST) &&
						(service->attrs[index2].val.data != NULL)) {
						dataEle = (struct bt_sdp_data_elem *)(service->attrs[index2].val.data);
						dataEle2 = NULL;
						total_size = service->attrs[index2].val.total_size;
						dataEleIndex = 0U;

						if ((dataEle == NULL) ||
							(service->attrs[index2].val.type != BT_SDP_SEQ8) ||
							(total_size < 2U)) {
							continue;
						}

						total_size -= 2U;
						while (total_size > 0U) {
							if (dataEle[dataEleIndex].type == BT_SDP_SEQ8) {
								if (total_size >= dataEle[dataEleIndex].total_size) {
									total_size -= dataEle[dataEleIndex].total_size;
								} else {
									break;
								}

								dataEle2 = (struct bt_sdp_data_elem *)(dataEle[dataEleIndex].data);
								if ((dataEle2 != NULL) && (dataEle2[0].type == BT_SDP_UUID16)) {
									tempVal = ((uint8_t *)(dataEle2[0].data))[0];
									tempVal |= ((uint8_t *)(dataEle2[0].data))[1] << 8U;
									if (tempVal == BT_SDP_PROTO_RFCOMM) {
										if (dataEle2[1].type == BT_SDP_UINT8) {
											channel = ((uint8_t *)(dataEle2[1].data))[0];
											break;
										}
									}
								}
							}
							dataEleIndex++;
						}
					}

					if (channel != 0U) {
						break;
					}
				}

				for (index2 = 0U; index2 < DB_MAX_RECORDS; index2++) {
					uint16_t num_attrs;
					uint16_t attr_offset;

					BT_dbase_get_record_handle(DB_RECORD_SPP, index2, &record_handle);
					if (record_handle == 0xFFFFFFFFu) {
						break;
					}

					if ((record_handle & 0x0000FFFFU) >= DB_MAX_RECORDS) {
						break;
					}

					attr_offset = dbase[record_handle & 0x0000FFFFU].attr_offset;
					num_attrs = dbase[record_handle & 0x0000FFFFU].num_attrs;
					for (uint16_t index3 = 0; index3 < num_attrs; index3++) {
						if (0x0004u == attr_arr[attr_offset + index3].attr_id) {
							uint8_t *value = attr_arr[attr_offset + index3].value;
							uint16_t len = attr_arr[attr_offset + index3].len;

							if (value[len - 1u] == channel) {
								found = 1u;
								break;
							}
						}
						if (found == 1u) {
							break;
						}
					}

					if (found == 1u) {
						break;
					}
				}

				if (found != 1u) {
					record_handle = 0xFFFFFFFFu;
				}
				break;
			}
			case BT_SDP_HANDSFREE_SVCLASS:
				BT_dbase_get_record_handle(DB_RECORD_HFU, 0,&record_handle);
				break;
			case BT_SDP_HANDSFREE_AGW_SVCLASS:
				BT_dbase_get_record_handle(DB_RECORD_HFAG, 0,&record_handle);
				break;
#if (defined(CONFIG_BT_AVRCP) && ((CONFIG_BT_AVRCP) > 0U))
			case BT_SDP_AV_REMOTE_TARGET_SVCLASS:
			{
				uint16_t find_feature = 0;
				uint8_t update_attr[3];

				BT_dbase_get_record_handle(DB_RECORD_AVRCP_TARGET, 0, &record_handle);
				if (record_handle == 0xFFFFFFFFu)
				{
					break;
				}

				for (index = 0; index < service->attr_count; index++)
				{
					if ((service->attrs[index].id == BT_SDP_ATTR_SUPPORTED_FEATURES) &&
						(service->attrs[index].val.data != NULL) &&
						(service->attrs[index].val.type == BT_SDP_UINT16))
					{
						find_feature = *((uint16_t*)service->attrs[index].val.data);
						break;
					}
				}

				update_attr[0] = 0x09u;
				sys_put_be16(find_feature, &update_attr[1]);
				BT_dbase_update_attr_value(record_handle, BT_SDP_ATTR_SUPPORTED_FEATURES, &update_attr[0], sizeof(update_attr));
				/* bit8 is "Supports Cover Art" */
				if (find_feature & (0x0001 << 8u))
				{
					BT_dbase_change_attr_value(record_handle, ADDITIONAL_PROT_DESC_LIST_ID, tg_cover_art_additional_protocol_desc_list,
						sizeof(tg_cover_art_additional_protocol_desc_list));
				}
				else
				{
					BT_dbase_change_attr_value(record_handle, ADDITIONAL_PROT_DESC_LIST_ID, tg_additional_protocol_descriptor_list,
						sizeof(tg_additional_protocol_descriptor_list));
				}
				break;
			}
			case BT_SDP_AV_REMOTE_CONTROLLER_SVCLASS:
			{
				uint16_t find_feature = 0;
				uint8_t update_attr[3];

				BT_dbase_get_record_handle(DB_RECORD_AVRCP_CONTROLLER, 0, &record_handle);
				if (record_handle == 0xFFFFFFFFu)
				{
					break;
				}

				for (index = 0; index < service->attr_count; index++)
				{
					if ((service->attrs[index].id == BT_SDP_ATTR_SUPPORTED_FEATURES) &&
						(service->attrs[index].val.data != NULL) &&
						(service->attrs[index].val.type == BT_SDP_UINT16))
					{
						find_feature = *((uint16_t*)service->attrs[index].val.data);
						break;
					}
				}

				update_attr[0] = 0x09u;
				sys_put_be16(find_feature, &update_attr[1]);
				BT_dbase_update_attr_value(record_handle, BT_SDP_ATTR_SUPPORTED_FEATURES, &update_attr[0], sizeof(update_attr));
				BT_dbase_change_attr_value(record_handle, ADDITIONAL_PROT_DESC_LIST_ID, ct_additional_protocol_descriptor_list,
					sizeof(ct_additional_protocol_descriptor_list));
				break;
			}
#endif
			case 0xBDDB:
				BT_dbase_get_record_handle(DB_RECORD_BQB_PTS_TEST_SDDB, 0,&record_handle);
				break;
			default:
			  /* MISRA 16.4 : The switch statement does not have a non-empty default clause. */
				break;
			}
		}

		if (record_handle != 0xFFFFFFFFu)
		{
			break;
		}
	}
	if (record_handle != 0xFFFFFFFFu)
	{
		BT_dbase_activate_record(record_handle);
	}
	return 0;
#endif /* SDP_DYNAMIC_DB */
}

#define GET_PARAM(__node) \
	CONTAINER_OF((__node), struct bt_sdp_discover_params, _node)

/* ServiceSearchAttribute PDU, ref to BT Core 4.2, Vol 3, part B, 4.7.1 */
static int sdp_client_ssa_search(struct bt_sdp_client *session)
{
	const struct bt_sdp_discover_params *param;
	uint32_t attribute_range = 0x0000FFFFu;
	/* SDP Attribute Datalen */
	uint16_t appl_sdp_attrib_datalen;
	S_UUID uuid;
	uint16_t num_uuids;
	API_RESULT retval;

	/*
	 * Select proper user params, if session->param is invalid it means
	 * getting new UUID from top of to be resolved params list. Otherwise
	 * the context is in a middle of partial SDP PDU responses and cached
	 * value from context can be used.
	 */
	if (!session->param) {
		param = GET_PARAM(bt_list_peek_head(&session->reqs));
	} else {
		param = session->param;
	}

	if (!param) {
		LOG_WRN("No UUIDs to be resolved on remote");
		return -EINVAL;
	}

	switch (param->uuid->type)
	{
	case BT_UUID_TYPE_16:
		uuid.uuid_type = UUID_16;
		uuid.uuid_union.uuid_16 = ((struct bt_uuid_16 *)(param->uuid))->val;
		break;
	case BT_UUID_TYPE_32:
		uuid.uuid_type = UUID_32;
		uuid.uuid_union.uuid_32 = ((struct bt_uuid_32 *)(param->uuid))->val;
		break;
	case BT_UUID_TYPE_128:
		uuid.uuid_type = UUID_128;
		memcpy(&uuid.uuid_union.uuid_128, ((struct bt_uuid_128 *)(param->uuid))->val, 16U);
		break;
	default:
		/* MISRA 16.4 : The switch statement does not have a non-empty default clause. */
		LOG_ERR("Unknown UUID type %u", param->uuid->type);
		return -EINVAL;
	}

	session->param = param;
	num_uuids = 0x01;
	net_buf_reset(session->buf);
	net_buf_reserve(session->buf, SDP_BUFF_RESERVE_FOR_HEAD_LEN);
	appl_sdp_attrib_datalen = net_buf_tailroom(session->buf);
	/* Do Service Search Request */
	retval = BT_sdp_servicesearchattributerequest
				(
					&session->sdp_handle,
					&uuid,
					num_uuids,
					NULL,
					0U,
					&attribute_range,
					0x01,
					session->buf->data,
					&appl_sdp_attrib_datalen
				);

	if (API_SUCCESS != retval)
	{
		LOG_ERR("> ** BT_sdp_servicesearchattributerequest Failed\n");

		sdp_client_notify_result(session, UUID_NOT_RESOLVED);
		/* Get next UUID and start resolving it */
		sdp_client_params_iterator(session);
		return -EIO;
	}

	return 0;
}

static void sdp_client_params_iterator(struct bt_sdp_client *session)
{
	struct bt_sdp_discover_params *param, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->reqs, param, tmp, _node) {
		if (param != session->param) {
			continue;
		}

		LOG_DBG("");

		/* Remove already checked UUID node */
		bt_list_remove(&session->reqs, NULL, &param->_node);
		/* Invalidate cached param in context */
		session->param = NULL;
		/* Reset continuation state in current context */
		(void)memset(&session->cstate, 0, sizeof(session->cstate));

		/* Check if there's valid next UUID */
		if (!bt_list_is_empty(&session->reqs)) {
			sdp_client_ssa_search(session);
			return;
		}

		/* No UUID items, disconnect channel */
		BT_sdp_close(&session->sdp_handle);
		break;
	}
}

static uint16_t sdp_client_get_total(struct bt_sdp_client *session,
				  struct net_buf *buf, uint16_t *total)
{
	uint16_t pulled;
	uint8_t seq;

	/*
	 * Pull value of total octets of all attributes available to be
	 * collected when response gets completed for given UUID. Such info can
	 * be get from the very first response frame after initial SSA request
	 * was sent. For subsequent calls related to the same SSA request input
	 * buf and in/out function parameters stays neutral.
	 */
	if (session->cstate.length == 0U) {
		seq = net_buf_pull_u8(buf);
		pulled = 1U;
		switch (seq) {
		case BT_SDP_SEQ8:
			*total = net_buf_pull_u8(buf);
			pulled += 1U;
			break;
		case BT_SDP_SEQ16:
			*total = net_buf_pull_be16(buf);
			pulled += 2U;
			break;
		default:
			LOG_WRN("Sequence type 0x%02x not handled", seq);
			*total = 0U;
			break;
		}

		LOG_DBG("Total %u octets of all attributes", *total);
	} else {
		pulled = 0U;
		*total = 0U;
	}

	return pulled;
}

static uint16_t get_record_len(struct net_buf *buf)
{
	uint16_t len;
	uint8_t seq;

	seq = net_buf_pull_u8(buf);

	switch (seq) {
	case BT_SDP_SEQ8:
		len = net_buf_pull_u8(buf);
		break;
	case BT_SDP_SEQ16:
		len = net_buf_pull_be16(buf);
		break;
	case BT_SDP_SEQ32:
		len = net_buf_pull_be32(buf);
		break;
	default:
		LOG_WRN("Sequence type 0x%02x not handled", seq);
		len = 0U;
		break;
	}

	LOG_DBG("Record len %u", len);

	return len;
}

static void sdp_client_notify_result(struct bt_sdp_client *session,
					 enum uuid_state state)
{
	struct bt_conn *conn = session->conn;
	struct bt_sdp_client_result result;
	uint16_t rec_len;
	uint8_t user_ret;

	result.uuid = session->param->uuid;

	if (state == UUID_NOT_RESOLVED) {
		result.resp_buf = NULL;
		result.next_record_hint = false;
		session->param->func(conn, &result);
		return;
	}

	while (session->rec_buf->len) {
		struct net_buf_simple_state buf_state;

		rec_len = get_record_len(session->rec_buf);
		/* tell the user about multi record resolution */
		if (session->rec_buf->len > rec_len) {
			result.next_record_hint = true;
		} else {
			result.next_record_hint = false;
		}

		/* save the original session buffer */
		net_buf_simple_save(&session->rec_buf->b, &buf_state);
		/* initialize internal result buffer instead of memcpy */
		result.resp_buf = session->rec_buf;
		/*
		 * Set user internal result buffer length as same as record
		 * length to fake user. User will see the individual record
		 * length as rec_len insted of whole session rec_buf length.
		 */
		result.resp_buf->len = rec_len;

		user_ret = session->param->func(conn, &result);

		/* restore original session buffer */
		net_buf_simple_restore(&session->rec_buf->b, &buf_state);
		/*
		 * sync session buffer data length with next record chunk not
		 * send to user so far
		 */
		net_buf_pull(session->rec_buf, rec_len);
		if (user_ret == BT_SDP_DISCOVER_UUID_STOP) {
			break;
		}
	}
}

static int sdp_client_receive(struct bt_sdp_client *session, struct net_buf *buf, uint16_t status)
{
	struct bt_sdp_hdr *hdr;
	struct bt_sdp_pdu_cstate *cstate;
	uint16_t len, frame_len;
	uint16_t total;

	if (API_SUCCESS != status)
	{
		sdp_client_notify_result(session, UUID_NOT_RESOLVED);
		/* Get next UUID and start resolving it */
		sdp_client_params_iterator(session);
		return 0;
	}

	LOG_DBG("session %p buf %p", session, buf);

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small SDP PDU");
		return 0;
	}

	hdr = (struct bt_sdp_hdr *)net_buf_pull_mem(buf, sizeof(*hdr));
	if (hdr->op_code == BT_SDP_ERROR_RSP) {
		LOG_INF("Error SDP PDU response");
		return 0;
	}

	len = sys_be16_to_cpu(hdr->param_len);
	//tid = sys_be16_to_cpu(hdr->tid);

	LOG_DBG("SDP PDU tid %u len %u", tid, len);

	if (buf->len != len) {
		LOG_ERR("SDP PDU length mismatch (%u != %u)", buf->len, len);
		return 0;
	}

	//if (tid != session->tid) {
	//	LOG_ERR("Mismatch transaction ID value in SDP PDU");
	//	return 0;
	//}

	switch (hdr->op_code) {
	case BT_SDP_SVC_SEARCH_ATTR_RSP:
		/* Get number of attributes in this frame. */
		frame_len = net_buf_pull_be16(buf);
		/* Check valid buf len for attribute list and cont state */
		if (buf->len < frame_len + SDP_CONT_STATE_LEN_SIZE) {
			LOG_ERR("Invalid frame payload length");
			return 0;
		}
		/* Check valid range of attributes length */
		if (frame_len < 2) {
			LOG_ERR("Invalid attributes data length");
			return 0;
		}

		/* Get PDU continuation state */
		cstate = (struct bt_sdp_pdu_cstate *)(buf->data + frame_len);

		if (cstate->length > BT_SDP_MAX_PDU_CSTATE_LEN) {
			LOG_ERR("Invalid SDP PDU Continuation State length %u",
				   cstate->length);
			return 0;
		}

		if ((frame_len + SDP_CONT_STATE_LEN_SIZE + cstate->length) >
			 buf->len) {
			LOG_ERR("Invalid frame payload length");
			return 0;
		}

		/*
		 * No record found for given UUID. The check catches case when
		 * current response frame has Continuation State shortest and
		 * valid and this is the first response frame as well.
		 */
		if (frame_len == 2U && cstate->length == 0U &&
			session->cstate.length == 0U) {
			LOG_DBG("record for UUID 0x%s not found",
				bt_uuid_str(session->param->uuid));
			/* Call user UUID handler */
			sdp_client_notify_result(session, UUID_NOT_RESOLVED);
			net_buf_pull(buf, frame_len + sizeof(cstate->length));
			goto iterate;
		}

		/* Get total value of all attributes to be collected */
		frame_len -= sdp_client_get_total(session, buf, &total);

		if (total > net_buf_tailroom(session->rec_buf)) {
			LOG_WRN("Not enough room for getting records data");
			goto iterate;
		}

		net_buf_add_mem(session->rec_buf, buf->data, frame_len);
		net_buf_pull(buf, frame_len);

		/*
		 * check if current response says there's next portion to be
		 * fetched
		 */
		if (cstate->length) {
			/* Cache original Continuation State in context */
			memcpy(&session->cstate, cstate,
				   sizeof(struct bt_sdp_pdu_cstate));

			net_buf_pull(buf, cstate->length +
					 sizeof(cstate->length));

			/* Request for next portion of attributes data */
			sdp_client_ssa_search(session);
			break;
		}

		net_buf_pull(buf, sizeof(cstate->length));

		LOG_DBG("UUID 0x%s resolved", bt_uuid_str(session->param->uuid));
		sdp_client_notify_result(session, UUID_RESOLVED);
iterate:
		/* Get next UUID and start resolving it */
		sdp_client_params_iterator(session);
		break;
	default:
		LOG_DBG("PDU 0x%0x response not handled", hdr->op_code);
		break;
	}

	return 0;
}

static int sdp_client_chan_connect(struct bt_sdp_client *session)
{
	/* Set the SDP handle */
	SDP_SET_HANDLE(session->sdp_handle, &session->conn->br.dst, session->sdb_cb);
	if (API_SUCCESS != BT_sdp_open(&session->sdp_handle))
	{
		return -1;
	}
	return 0;
}

static struct net_buf *sdp_client_alloc_buf(struct bt_sdp_client *session)
{
	struct net_buf *buf;

	LOG_DBG("session %p chan %p", session, chan);

	session->param = GET_PARAM(sys_slist_peek_head(&session->reqs));

	buf = net_buf_alloc(session->param->pool, osaWaitForever_c);
	assert(buf);

	return buf;
}

static void sdp_client_disconnected(struct bt_sdp_client *session)
{
	struct bt_sdp_discover_params *param, *next;

	LOG_DBG("session %p disconnected", session);

	/* callback all the sdp reqs */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->reqs, param, next, _node) {
		session->param = param;

		sdp_client_notify_result(session, UUID_NOT_RESOLVED);

		/* Remove already callbacked UUID node */
		sys_slist_find_and_remove(&session->reqs, &param->_node);
	}

	if (session->rec_buf != NULL) {
		net_buf_unref(session->rec_buf);
	}

	if (session->buf != NULL) {
		net_buf_unref(session->buf);
	}

	/*
	 * Reset session excluding L2CAP channel member. Let's the channel
	 * resets autonomous.
	 */
	(void)memset(&session->conn, 0,
			 sizeof(*session) - sizeof(session->sdb_cb));
}

static void sdp_client_connected(struct bt_sdp_client *session, uint16_t status)
{
	if (API_SUCCESS != status)
	{
		LOG_DBG("Fail to create SDP connection: 0x%04X\n", status);
		goto failed;
	}

	LOG_DBG("session %p chan %p connected", session, chan);

	session->rec_buf = sdp_client_alloc_buf(session);
	if (session->rec_buf == NULL) {
		/* No UUID items, disconnect channel */
		BT_sdp_close(&session->sdp_handle);
		goto failed;
	}

	if (sdp_client_ssa_search(session) != 0)
	{
		/* No UUID items, disconnect channel */
		BT_sdp_close(&session->sdp_handle);
		goto failed;
	}
	return;

failed:
	LOG_DBG("> ** FAILED performing SDP Operation: %02X\n", command);
	LOG_DBG("> Return Value : 0x%04X\n", status);
	sdp_client_notify_result(session, UUID_NOT_RESOLVED);
	sdp_client_disconnected(session);
}

static void bt_sdp_recv_handler(struct k_work *work)
{
	struct bt_sdp_client *session = (struct bt_sdp_client *)CONTAINER_OF(work, struct bt_sdp_client, recv);

	sdp_client_receive(session, session->buf, BT_SDP(session->buf)->status);
}

static struct bt_sdp_client *sdp_client_new_session(struct bt_conn *conn, const struct bt_sdp_discover_params *params)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_sdp_client_pool); i++) {
		struct bt_sdp_client *session = &bt_sdp_client_pool[i];
		struct net_buf *buf;

		int err;

		if (session->conn) {
			continue;
		}

		buf = net_buf_alloc(&sdp_pool, K_NO_WAIT);
		if (buf == NULL) {
			return NULL;
		}

		bt_list_init(&session->reqs);
		bt_list_append(&session->reqs, (bt_list_node_t *)&params->_node);

		k_work_init(&session->recv, bt_sdp_recv_handler);

		session->conn = conn;
		session->buf = buf;

		err = sdp_client_chan_connect(session);
		if (err) {
			net_buf_unref(buf);
			(void)memset(session, 0, sizeof(*session));
			LOG_ERR("Cannot connect %d", err);
			return NULL;
		}

		return session;
	}

	LOG_ERR("No available SDP client context");

	return NULL;
}

static struct bt_sdp_client *sdp_client_get_session(struct bt_conn *conn, const struct bt_sdp_discover_params *params)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_sdp_client_pool); i++) {
		if (bt_sdp_client_pool[i].conn == conn) {
			bt_list_append(&bt_sdp_client_pool[i].reqs, (bt_list_node_t *)&params->_node);
			return &bt_sdp_client_pool[i];
		}
	}

	/*
	 * Try to allocate session context since not found in pool and attempt
	 * connect to remote SDP endpoint.
	 */
	return sdp_client_new_session(conn, params);
}

int bt_sdp_discover(struct bt_conn *conn,
			const struct bt_sdp_discover_params *params)
{
	struct bt_sdp_client *session;

	if (!params || !params->uuid || !params->func || !params->pool) {
		LOG_WRN("Invalid user params");
		return -EINVAL;
	}

	session = sdp_client_get_session(conn, params);
	if (!session) {
		return -ENOMEM;
	}

	return 0;
}

/* Helper getting length of data determined by DTD for integers */
static inline ssize_t sdp_get_int_len(const uint8_t *data, size_t len)
{
	assert(data);

	switch (data[0]) {
	case BT_SDP_DATA_NIL:
		return 1;
	case BT_SDP_BOOL:
	case BT_SDP_INT8:
	case BT_SDP_UINT8:
		if (len < 2) {
			break;
		}

		return 2;
	case BT_SDP_INT16:
	case BT_SDP_UINT16:
		if (len < 3) {
			break;
		}

		return 3;
	case BT_SDP_INT32:
	case BT_SDP_UINT32:
		if (len < 5) {
			break;
		}

		return 5;
	case BT_SDP_INT64:
	case BT_SDP_UINT64:
		if (len < 9) {
			break;
		}

		return 9;
	case BT_SDP_INT128:
	case BT_SDP_UINT128:
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", data[0]);
		return -EINVAL;
	}

	LOG_ERR("Too short buffer length %lu", len);
	return -EMSGSIZE;
}

/* Helper getting length of data determined by DTD for UUID */
static inline ssize_t sdp_get_uuid_len(const uint8_t *data, size_t len)
{
	assert(data);

	switch (data[0]) {
	case BT_SDP_UUID16:
		if (len < 3) {
			break;
		}

		return 3;
	case BT_SDP_UUID32:
		if (len < 5) {
			break;
		}

		return 5;
	case BT_SDP_UUID128:
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", data[0]);
		return -EINVAL;
	}

	LOG_ERR("Too short buffer length %lu", len);
	return -EMSGSIZE;
}

/* Helper getting length of data determined by DTD for strings */
static inline ssize_t sdp_get_str_len(const uint8_t *data, size_t len)
{
	const uint8_t *pnext;

	assert(data);

	/* validate len for pnext safe use to read next 8bit value */
	if (len < 2) {
		goto err;
	}

	pnext = data + sizeof(uint8_t);

	switch (data[0]) {
	case BT_SDP_TEXT_STR8:
	case BT_SDP_URL_STR8:
		if (len < (2 + pnext[0])) {
			break;
		}

		return 2 + pnext[0];
	case BT_SDP_TEXT_STR16:
	case BT_SDP_URL_STR16:
		/* validate len for pnext safe use to read 16bit value */
		if (len < 3) {
			break;
		}

		if (len < (3 + sys_get_be16(pnext))) {
			break;
		}

		return 3 + sys_get_be16(pnext);
	case BT_SDP_TEXT_STR32:
	case BT_SDP_URL_STR32:
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", data[0]);
		return -EINVAL;
	}
err:
	LOG_ERR("Too short buffer length %lu", len);
	return -EMSGSIZE;
}

/* Helper getting length of data determined by DTD for sequences */
static inline ssize_t sdp_get_seq_len(const uint8_t *data, size_t len)
{
	const uint8_t *pnext;

	assert(data);

	/* validate len for pnext safe use to read 8bit bit value */
	if (len < 2) {
		goto err;
	}

	pnext = data + sizeof(uint8_t);

	switch (data[0]) {
	case BT_SDP_SEQ8:
	case BT_SDP_ALT8:
		if (len < (2 + pnext[0])) {
			break;
		}

		return 2 + pnext[0];
	case BT_SDP_SEQ16:
	case BT_SDP_ALT16:
		/* validate len for pnext safe use to read 16bit value */
		if (len < 3) {
			break;
		}

		if (len < (3 + sys_get_be16(pnext))) {
			break;
		}

		return 3 + sys_get_be16(pnext);
	case BT_SDP_SEQ32:
	case BT_SDP_ALT32:
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", data[0]);
		return -EINVAL;
	}
err:
	LOG_ERR("Too short buffer length %lu", len);
	return -EMSGSIZE;
}

/* Helper getting length of attribute value data */
static ssize_t sdp_get_attr_value_len(const uint8_t *data, size_t len)
{
	assert(data);

	LOG_DBG("Attr val DTD 0x%02x", data[0]);

	switch (data[0]) {
	case BT_SDP_DATA_NIL:
	case BT_SDP_BOOL:
	case BT_SDP_UINT8:
	case BT_SDP_UINT16:
	case BT_SDP_UINT32:
	case BT_SDP_UINT64:
	case BT_SDP_UINT128:
	case BT_SDP_INT8:
	case BT_SDP_INT16:
	case BT_SDP_INT32:
	case BT_SDP_INT64:
	case BT_SDP_INT128:
		return sdp_get_int_len(data, len);
	case BT_SDP_UUID16:
	case BT_SDP_UUID32:
	case BT_SDP_UUID128:
		return sdp_get_uuid_len(data, len);
	case BT_SDP_TEXT_STR8:
	case BT_SDP_TEXT_STR16:
	case BT_SDP_TEXT_STR32:
	case BT_SDP_URL_STR8:
	case BT_SDP_URL_STR16:
	case BT_SDP_URL_STR32:
		return sdp_get_str_len(data, len);
	case BT_SDP_SEQ8:
	case BT_SDP_SEQ16:
	case BT_SDP_SEQ32:
	case BT_SDP_ALT8:
	case BT_SDP_ALT16:
	case BT_SDP_ALT32:
		return sdp_get_seq_len(data, len);
	default:
		LOG_ERR("Unknown DTD 0x%02x", data[0]);
		return -EINVAL;
	}
}

/* Type holding UUID item and related to it specific information. */
struct bt_sdp_uuid_desc {
	union {
		struct bt_uuid	uuid;
		struct bt_uuid_16 uuid16;
		struct bt_uuid_32 uuid32;
		  };
	uint16_t					 attr_id;
	uint8_t					 *params;
	uint16_t					 params_len;
};

/* Generic attribute item collector. */
struct bt_sdp_attr_item {
	/*  Attribute identifier. */
	uint16_t				  attr_id;
	/*  Address of beginning attribute value taken from original buffer
	 *  holding response from server.
	 */
	uint8_t				  *val;
	/*  Says about the length of attribute value. */
	uint16_t				  len;
};

static int bt_sdp_get_attr(const struct net_buf *buf,
			   struct bt_sdp_attr_item *attr, uint16_t attr_id)
{
	uint8_t *data;
	uint16_t id;

	data = buf->data;
	while (data - buf->data < buf->len) {
		ssize_t dlen;

		/* data need to point to attribute id descriptor field (DTD)*/
		if (data[0] != BT_SDP_UINT16) {
			LOG_ERR("Invalid descriptor 0x%02x", data[0]);
			return -EINVAL;
		}

		data += sizeof(uint8_t);
		id = sys_get_be16(data);
		LOG_DBG("Attribute ID 0x%04x", id);
		data += sizeof(uint16_t);

		dlen = sdp_get_attr_value_len(data,
						  buf->len - (data - buf->data));
		if (dlen < 0) {
			LOG_ERR("Invalid attribute value data");
			return -EINVAL;
		}

		if (id == attr_id) {
			LOG_DBG("Attribute ID 0x%04x Value found", id);
			/*
			 * Initialize attribute value buffer data using selected
			 * data slice from original buffer.
			 */
			attr->val = data;
			attr->len = dlen;
			attr->attr_id = id;
			return 0;
		}

		data += dlen;
	}

	return -ENOENT;
}

/* reads SEQ item length, moves input buffer data reader forward */
static ssize_t sdp_get_seq_len_item(uint8_t **data, size_t len)
{
	const uint8_t *pnext;

	assert(data);
	assert(*data);

	/* validate len for pnext safe use to read 8bit bit value */
	if (len < 2) {
		goto err;
	}

	pnext = *data + sizeof(uint8_t);

	switch (*data[0]) {
	case BT_SDP_SEQ8:
		if (len < (2 + pnext[0])) {
			break;
		}

		*data += 2;
		return pnext[0];
	case BT_SDP_SEQ16:
		/* validate len for pnext safe use to read 16bit value */
		if (len < 3) {
			break;
		}

		if (len < (3 + sys_get_be16(pnext))) {
			break;
		}

		*data += 3;
		return sys_get_be16(pnext);
	case BT_SDP_SEQ32:
		/* validate len for pnext safe use to read 32bit value */
		if (len < 5) {
			break;
		}

		if (len < (5 + sys_get_be32(pnext))) {
			break;
		}

		*data += 5;
		return sys_get_be32(pnext);
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x", *data[0]);
		return -EINVAL;
	}
err:
	LOG_ERR("Too short buffer length %lu", len);
	return -EMSGSIZE;
}

static int sdp_loop_seqs(uint8_t **data, size_t len)
{
	ssize_t slen;
	ssize_t pre_slen;
	uint8_t *end;

	if (len <= 0) {
		return -EMSGSIZE;
	}

	pre_slen = -EINVAL;
	slen = -EINVAL;
	end = *data + len;
	/* loop all the SEQ */
	while (*data < end) {
		/* how long is current UUID's item data associated to */
		slen = sdp_get_seq_len_item(data, end - *data);
		if (slen < 0) {
			break;
		}
		pre_slen = slen;
	}

	/* return the last seq len */
	if (pre_slen < 0) {
		return slen;
	}

	return pre_slen;
}

static int sdp_get_uuid_data(const struct bt_sdp_attr_item *attr,
			     struct bt_sdp_uuid_desc *pd,
			     uint16_t proto_profile,
			     uint8_t proto_profile_index)
{
	/* get start address of attribute value */
	uint8_t *p = attr->val;
	ssize_t slen;

	assert(p);
	memset(pd, 0, sizeof(struct bt_sdp_uuid_desc));
	/* start reading stacked UUIDs in analyzed sequences tree */
	while (p - attr->val < attr->len) {
		size_t to_end, left = 0;
		uint8_t dtd;

		/* to_end tells how far to the end of input buffer */
		to_end = attr->len - (p - attr->val);
		/* loop all the SEQ, get the last SEQ len */
		slen = sdp_loop_seqs(&p, to_end);

		if (slen < 0) {
			return slen;
		}

		/* left tells how far is to the end of current UUID */
		left = slen;

		/* check if at least DTD + UUID16 can be read safely */
		if (left < (sizeof(dtd) + BT_UUID_SIZE_16)) {
			return -EMSGSIZE;
		}

		/* check DTD and get stacked UUID value */
		dtd = p[0];
		p++;
		/* include last DTD in p[0] size itself updating left */
		left -= sizeof(dtd);
		switch (dtd) {
		case BT_SDP_UUID16:
			memcpy(&pd->uuid16,
				BT_UUID_DECLARE_16(sys_get_be16(p)),
				sizeof(struct bt_uuid_16));
			p += sizeof(uint16_t);
			left -= sizeof(uint16_t);
			break;
		case BT_SDP_UUID32:
			/* check if valid UUID32 can be read safely */
			if (left < BT_UUID_SIZE_32) {
				return -EMSGSIZE;
			}

			memcpy(&pd->uuid32,
				BT_UUID_DECLARE_32(sys_get_be32(p)),
				sizeof(struct bt_uuid_32));
			p += sizeof(BT_UUID_SIZE_32);
			left -= sizeof(BT_UUID_SIZE_32);
			break;
		default:
			LOG_ERR("Invalid/unhandled DTD 0x%02x\n", dtd);
			return -EINVAL;
		}

		/*
			* Check if current UUID value matches input one given by user.
			* If found save it's location and length and return.
			*/
		if ((proto_profile == BT_UUID_16(&pd->uuid)->val) ||
			(proto_profile == BT_UUID_32(&pd->uuid)->val)) {
			pd->params = p;
			pd->params_len = left;

			LOG_DBG("UUID 0x%s found", bt_uuid_str(&pd->uuid));
			if (proto_profile_index > 0U) {
				proto_profile_index--;
				p += left;
				continue;
			} else {
				return 0;
			}
		}

		/* skip left octets to point beginning of next UUID in tree */
		p += left;
	}

	LOG_DBG("Value 0x%04x index %d not found", proto_profile, proto_profile_index);
	return -ENOENT;
}

/*
 * Helper extracting specific parameters associated with UUID node given in
 * protocol descriptor list or profile descriptor list.
 */
static int sdp_get_param_item(struct bt_sdp_uuid_desc *pd_item, uint16_t *param)
{
	const uint8_t *p = pd_item->params;
	bool len_err = false;

	assert(p);

	LOG_DBG("Getting UUID's 0x%s params", bt_uuid_str(&pd_item->uuid));

	switch (p[0]) {
	case BT_SDP_UINT8:
		/* check if 8bits value can be read safely */
		if (pd_item->params_len < 2) {
			len_err = true;
			break;
		}
		*param = (++p)[0];
		p += sizeof(uint8_t);
		break;
	case BT_SDP_UINT16:
		/* check if 16bits value can be read safely */
		if (pd_item->params_len < 3) {
			len_err = true;
			break;
		}
		*param = sys_get_be16(++p);
		p += sizeof(uint16_t);
		break;
	case BT_SDP_UINT32:
		/* check if 32bits value can be read safely */
		if (pd_item->params_len < 5) {
			len_err = true;
			break;
		}
		*param = sys_get_be32(++p);
		p += sizeof(uint32_t);
		break;
	default:
		LOG_ERR("Invalid/unhandled DTD 0x%02x\n", p[0]);
		return -EINVAL;
	}
	/*
	 * Check if no more data than already read is associated with UUID. In
	 * valid case after getting parameter we should reach data buf end.
	 */
	if (p - pd_item->params != pd_item->params_len || len_err) {
		LOG_DBG("Invalid param buffer length");
		return -EMSGSIZE;
	}

	return 0;
}

int bt_sdp_get_proto_param(const struct net_buf *buf, enum bt_sdp_proto proto,
			   uint16_t *param)
{
	struct bt_sdp_attr_item attr;
	struct bt_sdp_uuid_desc pd;
	int res;

	if (proto != BT_SDP_PROTO_RFCOMM && proto != BT_SDP_PROTO_L2CAP) {
		LOG_ERR("Invalid protocol specifier");
		return -EINVAL;
	}

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_PROTO_DESC_LIST);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_PROTO_DESC_LIST, res);
		return res;
	}

	res = sdp_get_uuid_data(&attr, &pd, proto, 0U);
	if (res < 0) {
		LOG_WRN("Protocol specifier 0x%04x not found, err %d", proto,
			res);
		return res;
	}

	return sdp_get_param_item(&pd, param);
}

int bt_sdp_get_addl_proto_param(const struct net_buf *buf, enum bt_sdp_proto proto,
				uint8_t param_index, uint16_t *param)
{
	struct bt_sdp_attr_item attr;
	struct bt_sdp_uuid_desc pd;
	int res;

	if (proto != BT_SDP_PROTO_RFCOMM && proto != BT_SDP_PROTO_L2CAP) {
		LOG_ERR("Invalid protocol specifier");
		return -EINVAL;
	}

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_ADD_PROTO_DESC_LIST);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_PROTO_DESC_LIST, res);
		return res;
	}

	res = sdp_get_uuid_data(&attr, &pd, proto, param_index);
	if (res < 0) {
		LOG_WRN("Protocol specifier 0x%04x not found, err %d", proto,
			res);
		return res;
	}

	return sdp_get_param_item(&pd, param);
}

int bt_sdp_get_profile_version(const struct net_buf *buf, uint16_t profile,
				   uint16_t *version)
{
	struct bt_sdp_attr_item attr;
	struct bt_sdp_uuid_desc pd;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_PROFILE_DESC_LIST);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_PROFILE_DESC_LIST, res);
		return res;
	}

	res = sdp_get_uuid_data(&attr, &pd, profile, 0U);
	if (res < 0) {
		LOG_WRN("Profile 0x%04x not found, err %d", profile, res);
		return res;
	}

	return sdp_get_param_item(&pd, version);
}

int bt_sdp_get_features(const struct net_buf *buf, uint16_t *features)
{
	struct bt_sdp_attr_item attr;
	const uint8_t *p;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_SUPPORTED_FEATURES);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_SUPPORTED_FEATURES, res);
		return res;
	}

	/* assert 16bit can be read safely */
	if (attr.len < 3) {
		LOG_ERR("Data length too short %u", attr.len);
		return -EMSGSIZE;
	}

	p = attr.val;
	assert(p);

	if (p[0] != BT_SDP_UINT16) {
		LOG_ERR("Invalid DTD 0x%02x", p[0]);
		return -EINVAL;
	}

	*features = sys_get_be16(++p);
	p += sizeof(uint16_t);

	if (p - attr.val != attr.len) {
		LOG_ERR("Invalid data length %u", attr.len);
		return -EMSGSIZE;
	}

	return 0;
}

int bt_sdp_get_goep_l2cap_psm(const struct net_buf *buf, uint16_t *l2cap_psm)
{
    struct bt_sdp_attr_item attr;
	const uint8_t *p;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_GOEP_L2CAP_PSM);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_GOEP_L2CAP_PSM, res);
		return res;
	}

	/* assert 16bit can be read safely */
	if (attr.len < 3) {
		LOG_ERR("Data length too short %u", attr.len);
		return -EMSGSIZE;
	}

	p = attr.val;
	assert(p);

	if (p[0] != BT_SDP_UINT16) {
		LOG_ERR("Invalid DTD 0x%02x", p[0]);
		return -EINVAL;
	}

	*l2cap_psm = sys_get_be16(++p);
	p += sizeof(uint16_t);

	if (p - attr.val != attr.len) {
		LOG_ERR("Invalid data length %u", attr.len);
		return -EMSGSIZE;
	}

	return 0;
}

int bt_sdp_get_supported_repositories(const struct net_buf *buf, uint8_t *supported_repositories)
{
	struct bt_sdp_attr_item attr;
	const uint8_t *p;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_SUPPORTED_REPOSITORIES);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_SUPPORTED_REPOSITORIES, res);
		return res;
	}

	/* assert 8bit can be read safely */
	if (attr.len < 2) {
		LOG_ERR("Data length too short %u", attr.len);
		return -EMSGSIZE;
	}

	p = attr.val;
	assert(p);

	if (p[0] != BT_SDP_UINT8) {
		LOG_ERR("Invalid DTD 0x%02x", p[0]);
		return -EINVAL;
	}

	*supported_repositories = *++p;
	p += sizeof(uint8_t);

	if (p - attr.val != attr.len) {
		LOG_ERR("Invalid data length %u", attr.len);
		return -EMSGSIZE;
	}

	return 0;
}

int bt_sdp_get_pbap_map_ctn_features(const struct net_buf *buf, uint32_t *features)
{
	struct bt_sdp_attr_item attr;
	const uint8_t *p;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_PBAP_SUPPORTED_FEATURES);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_SUPPORTED_FEATURES, res);
		return res;
	}

	/* assert 32bit can be read safely */
	if (attr.len < 5) {
		LOG_ERR("Data length too short %u", attr.len);
		return -EMSGSIZE;
	}

	p = attr.val;
	assert(p);

	if (p[0] != BT_SDP_UINT32) {
		LOG_ERR("Invalid DTD 0x%02x", p[0]);
		return -EINVAL;
	}

	*features = sys_get_be32(++p);
	p += sizeof(uint32_t);

	if (p - attr.val != attr.len) {
		LOG_ERR("Invalid data length %u", attr.len);
		return -EMSGSIZE;
	}

	return 0;
}

int bt_sdp_get_instance_id(const struct net_buf *buf, uint8_t *id)
{
	struct bt_sdp_attr_item attr;
	const uint8_t *p;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_MAS_INSTANCE_ID);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_MAS_INSTANCE_ID, res);
		return res;
	}

	/* assert 8bit can be read safely */
	if (attr.len < 2) {
		LOG_ERR("Data length too short %u", attr.len);
		return -EMSGSIZE;
	}

	p = attr.val;
	assert(p);

	if (p[0] != BT_SDP_UINT8) {
		LOG_ERR("Invalid DTD 0x%02x", p[0]);
		return -EINVAL;
	}

	*id = *++p;
	p += sizeof(uint8_t);

	if (p - attr.val != attr.len) {
		LOG_ERR("Invalid data length %u", attr.len);
		return -EMSGSIZE;
	}

	return 0;
}

int bt_sdp_get_supported_msg_type(const struct net_buf *buf, uint8_t *supported_msg_type)
{
	struct bt_sdp_attr_item attr;
	const uint8_t *p;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_MAS_INSTANCE_ID);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_MAS_INSTANCE_ID, res);
		return res;
	}

	/* assert 8bit can be read safely */
	if (attr.len < 2) {
		LOG_ERR("Data length too short %u", attr.len);
		return -EMSGSIZE;
	}

	p = attr.val;
	assert(p);

	if (p[0] != BT_SDP_UINT8) {
		LOG_ERR("Invalid DTD 0x%02x", p[0]);
		return -EINVAL;
	}

	*supported_msg_type = *++p;
	p += sizeof(uint8_t);

	if (p - attr.val != attr.len) {
		LOG_ERR("Invalid data length %u", attr.len);
		return -EMSGSIZE;
	}

	return 0;
}

int bt_sdp_get_service_name(const struct net_buf *buf, const char **name)
{
	struct bt_sdp_attr_item attr;
	const uint8_t *p;
	int res;

	res = bt_sdp_get_attr(buf, &attr, BT_SDP_ATTR_SVCNAME_PRIMARY);
	if (res < 0) {
		LOG_WRN("Attribute 0x%04x not found, err %d",
			BT_SDP_ATTR_SVCNAME_PRIMARY, res);
		return res;
	}

	p = attr.val;
	assert(p);

	if (p[0] != BT_SDP_TEXT_STR8) {
		LOG_ERR("Invalid DTD 0x%02x", p[0]);
		return -EINVAL;
	}

	/* assert Text string with 8-bit length can be read safely */
	if (attr.len < p[1] + 2U) {
		LOG_ERR("Data length too short %u", attr.len);
		return -EMSGSIZE;
	}

	p += 2;
	*name = (const char *)p;
	p += strlen(*name) + 1U;

	if (p - attr.val != attr.len) {
		LOG_ERR("Invalid data length %u", attr.len);
		return -EMSGSIZE;
	}

	return 0;
}

static void ethermind_sdp_callback(struct bt_sdp_client *session, uint8_t command, uint8_t * data,
	uint16_t length, uint16_t status)
{
	switch(command)
	{
	case SDP_Open : /* SDP open callback */
		sdp_client_connected(session, status);
		break;

	case SDP_Close: /* SDP Close callback */
		sdp_client_disconnected(session);
		break;

	/* Service Search Attribute callback */
	case SDP_ServiceSearchAttributeResponse:
		if (API_SUCCESS != status)
		{
			sdp_client_receive(session, NULL, status);
			break;
		}
		else
		{
			net_buf_add(session->buf, length);
			net_buf_push_be16(session->buf, length);
			net_buf_push_be16(session->buf, length + 3);
			net_buf_push_be16(session->buf, 0);
			net_buf_push_u8(session->buf, BT_SDP_SVC_SEARCH_ATTR_RSP);
			if (net_buf_tailroom(session->buf) > 0) {
				net_buf_add_u8(session->buf, 0);
			}
			BT_SDP(session->buf)->status = status;

			k_work_submit(&session->recv);
		}
		break;


	case SDP_ErrorResponse:
		LOG_ERR("> ** ERROR occoured in SDP Query\n");
		BT_sdp_close(&session->sdp_handle);
		break;

	default: /* Invalid: Nothing to do */
		break;

	}

	return;
}
#endif /* CONFIG_BT_CLASSIC */