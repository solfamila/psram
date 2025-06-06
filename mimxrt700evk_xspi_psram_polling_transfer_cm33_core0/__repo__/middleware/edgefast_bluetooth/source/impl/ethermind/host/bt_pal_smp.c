/**
 * @file smp.c
 * Security Manager Protocol implementation
 */

/*
 * Copyright 2021, 2024 NXP
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <porting.h>
#include <stddef.h>
#include <errno/errno.h>
#include <string.h>
#include <sys/atomic.h>
#include <sys/util.h>
#include <sys/byteorder.h>

#include <net/buf.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/buf.h>
#include "BT_common.h"
#if !(defined(CONFIG_BT_BLE_DISABLE) && ((CONFIG_BT_BLE_DISABLE) > 0U))
#include "BT_smp_api.h"
#endif
#include "BT_sm_api.h"
#include "sm_internal.h"
#if !(defined(CONFIG_BT_BLE_DISABLE) && ((CONFIG_BT_BLE_DISABLE) > 0U))
#include "smp_pl.h"
#endif
#include "bt_crypto.h"
#include "bt_pal_l2cap_br_interface.h"
#include "smp_internal.h"
#include "smp_extern.h"

#if (defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U))

#define LOG_ENABLE IS_ENABLED(CONFIG_BT_DEBUG_SMP)
#define LOG_MODULE_NAME bt_smp
#include "fsl_component_log.h"
LOG_MODULE_DEFINE(LOG_MODULE_NAME, kLOG_LevelTrace);

#include "bt_pal_hci_core.h"
#include "bt_pal_ecc.h"
#include "bt_pal_keys.h"
#include "bt_pal_conn_internal.h"
#include "bt_pal_l2cap_internal.h"
#include "bt_pal_crypto_internal.h"
#include "bt_pal_ssp.h"
#include "bt_pal_smp.h"

#define SMP_TIMEOUT BT_SECONDS(30)

#define BT_SMP_KEYS_REMOTE_ENCKEY                       0x01U
#define BT_SMP_KEYS_REMOTE_IDKEY                        0x02U
#define BT_SMP_KEYS_REMOTE_SIGNKEY                      0x04U

#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
#define SIGN_DIST BT_SMP_DIST_SIGN
#else
#define SIGN_DIST 0
#endif

#if (defined(CONFIG_BT_PRIVACY) && ((CONFIG_BT_PRIVACY) > 0U))
#define ID_DIST BT_SMP_DIST_ID_KEY
#else
#define ID_DIST 0
#endif

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
#define LINK_DIST BT_SMP_DIST_LINK_KEY
#else
#define LINK_DIST 0
#endif

#define RECV_KEYS (BT_SMP_DIST_ENC_KEY | BT_SMP_DIST_ID_KEY | SIGN_DIST |\
		   LINK_DIST)
#define SEND_KEYS (BT_SMP_DIST_ENC_KEY | ID_DIST | SIGN_DIST | LINK_DIST)

#define RECV_KEYS_SC (RECV_KEYS & ~(BT_SMP_DIST_ENC_KEY))
#define SEND_KEYS_SC (SEND_KEYS & ~(BT_SMP_DIST_ENC_KEY))

#define BR_RECV_KEYS_SC (RECV_KEYS & ~(LINK_DIST))
#define BR_SEND_KEYS_SC (SEND_KEYS & ~(LINK_DIST))

#define BT_SMP_AUTH_MASK	0x07

#if (defined(CONFIG_BT_BONDABLE) && ((CONFIG_BT_BONDABLE) > 0U))
#define BT_SMP_AUTH_BONDING_FLAGS BT_SMP_AUTH_BONDING
#else
#define BT_SMP_AUTH_BONDING_FLAGS 0
#endif /* CONFIG_BT_BONDABLE */

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))

#define BT_SMP_AUTH_MASK_SC	0x2f
#if (defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) && ((CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) > 0U))
#define BT_SMP_AUTH_DEFAULT (BT_SMP_AUTH_BONDING_FLAGS | BT_SMP_AUTH_CT2)
#else
#define BT_SMP_AUTH_DEFAULT (BT_SMP_AUTH_BONDING_FLAGS | BT_SMP_AUTH_CT2 |\
			     BT_SMP_AUTH_SC)
#endif /* CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

#else

#define BT_SMP_AUTH_MASK_SC	0x0f
#if (defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) && ((CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) > 0U))
#define BT_SMP_AUTH_DEFAULT (BT_SMP_AUTH_BONDING_FLAGS)
#else
#define BT_SMP_AUTH_DEFAULT (BT_SMP_AUTH_BONDING_FLAGS | BT_SMP_AUTH_SC)
#endif /* CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

#endif /* CONFIG_BT_CLASSIC */

enum pairing_method {
	JUST_WORKS,		/* JustWorks pairing */
	PASSKEY_INPUT,		/* Passkey Entry input */
	PASSKEY_DISPLAY,	/* Passkey Entry display */
	PASSKEY_CONFIRM,	/* Passkey confirm */
	PASSKEY_ROLE,		/* Passkey Entry depends on role */
	LE_SC_OOB,		/* LESC Out of Band */
	LEGACY_OOB,		/* Legacy Out of Band */
};

enum pairing_confirm_type {
	CONFIRM_TYPE_NONE = 0,  /* No confirm type */
	CONFIRM_TYPE_PAIRING,   /* Pairing confirm type */
	CONFIRM_TYPE_PASSKEY,   /* Passkey confirm type */
};

enum {
	SMP_FLAG_CFM_DELAYED,	/* if confirm should be send when TK is valid */
	SMP_FLAG_ENC_PENDING,	/* if waiting for an encryption change event */
	SMP_FLAG_KEYS_DISTR,	/* if keys distribution phase is in progress */
	SMP_FLAG_PAIRING,	/* if pairing is in progress */
	SMP_FLAG_TIMEOUT,	/* if SMP timeout occurred */
	SMP_FLAG_SC,		/* if LE Secure Connections is used */
	SMP_FLAG_PKEY_SEND,	/* if should send Public Key when available */
	SMP_FLAG_DHKEY_PENDING,	/* if waiting for local DHKey */
	SMP_FLAG_DHKEY_GEN,     /* if generating DHKey */
	SMP_FLAG_DHKEY_SEND,	/* if should generate and send DHKey Check */
	SMP_FLAG_USER,		/* if waiting for user input */
	SMP_FLAG_DISPLAY,       /* if display_passkey() callback was called */
	SMP_FLAG_OOB_PENDING,	/* if waiting for OOB data */
	SMP_FLAG_BOND,		/* if bonding */
	SMP_FLAG_SC_DEBUG_KEY,	/* if Secure Connection are using debug key */
	SMP_FLAG_SEC_REQ,	/* if Security Request was sent/received */
	SMP_FLAG_DHCHECK_WAIT,	/* if waiting for remote DHCheck (as periph) */
	SMP_FLAG_DERIVE_LK,	/* if Link Key should be derived */
	SMP_FLAG_BR_CONNECTED,	/* if BR/EDR channel is connected */
	SMP_FLAG_BR_PAIR,	/* if should start BR/EDR pairing */
	SMP_FLAG_CT2,		/* if should use H7 for keys derivation */

	/* Total number of flags - must be at the end */
	SMP_NUM_FLAGS,
};

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
static API_RESULT ethermind_br_sm_ui_notify_cb
		   (
			   UCHAR	  event_type,
			   UCHAR *	bd_addr,
			   UCHAR *	event_data
		   )
{
#ifdef BT_SSP
	UINT32 numeric_val;
#endif /* BT_SSP */

	API_RESULT retval;
	struct bt_hci_evt_link_key_notify link_key;
	struct bt_conn *conn;
	bt_addr_t peer;

	LOG_DBG("Received SM Service UI Notification. Event Type 0x%02X", event_type);

	memcpy(peer.val, bd_addr, sizeof(peer));

	conn = bt_conn_lookup_addr_br(&peer);
	LOG_DBG("conn = 0x%08X", conn);

	retval = API_SUCCESS;

	if ((NULL != conn) && (BT_CONN_TYPE_BR != conn->type))
	{
		bt_conn_unref(conn);
		return SMP_INVALID_PARAMETERS;
	}

	switch (event_type)
	{
	case SM_ACL_CONNECT_REQUEST_NTF:
		LOG_DBG("Received UI Connection Request from SM");
		LOG_DBG(BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER, BT_DEVICE_ADDR_ONLY_PRINT_STR (bd_addr));

		if (NULL == conn)
		{
			bt_br_acl_link_connect_req(&peer, *((uint32_t *)event_data));
#if 0
			conn = bt_conn_lookup_addr_br(&peer);
#endif
		}
		break;
	case SM_AUTHORIZATION_REQUEST_NTF:
		LOG_DBG("Received Authorization Request from SM");
		LOG_DBG( BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER, BT_DEVICE_ADDR_ONLY_PRINT_STR (bd_addr));
		retval = BT_sm_authorization_request_reply(bd_addr, 1);
		if (API_SUCCESS == retval)
		{
		}

	case SM_PIN_CODE_REQUEST_NTF:
		LOG_DBG("Received UI PIN Code Request from SM");
		LOG_DBG( BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER, BT_DEVICE_ADDR_ONLY_PRINT_STR (bd_addr));

		/* Get PIN Code from SM */
		if (NULL != conn)
		{
			pin_code_req(conn);
		}
		break;

	case SM_LINK_KEY_REQUEST_NTF:
		LOG_DBG("Received UI Link Key Request from SM");
		LOG_DBG( BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER, BT_DEVICE_ADDR_ONLY_PRINT_STR (bd_addr));

		memcpy(link_key.bdaddr.val, bd_addr, sizeof(link_key.bdaddr));
		(void)BT_sm_get_device_link_key_and_type(bd_addr, link_key.link_key, &link_key.key_type);
		(void)ethermind_hci_event_callback(BT_HCI_EVT_LINK_KEY_REQ, (uint8_t *)&link_key, sizeof(link_key.bdaddr.val));
		break;

#ifdef BT_SSP
#ifdef BT_SSP_NC
	case SM_USER_CONF_REQUEST_NTF:
		LOG_DBG("Received UI User Conf Request from SM");
		LOG_DBG(BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER, BT_DEVICE_ADDR_ONLY_PRINT_STR (bd_addr));
		/* Get Numeric Value */
		numeric_val   = event_data[3];
		numeric_val <<= 8;
		numeric_val  |= event_data[2];
		numeric_val <<= 8;
		numeric_val  |= event_data[1];
		numeric_val <<= 8;
		numeric_val  |= event_data[0];
		LOG_DBG("Numeric Value = %06d (0x%08X)",
			   (unsigned int) numeric_val, (unsigned int) numeric_val);
		if (NULL != conn)
		{
			user_confirm_req(conn, numeric_val);
		}
		break;
#endif /* BT_SSP_NC */

#ifdef BT_SSP_PE
	case SM_USER_PASSKEY_REQUEST_NTF:
		LOG_DBG("Received UI User Passkey Request from SM");
		LOG_DBG(BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER, BT_DEVICE_ADDR_ONLY_PRINT_STR (bd_addr));
		/* Save BD_ADDR for Menu use */
		if (NULL != conn)
		{
			user_passkey_req(conn);
		}
		break;

	case SM_USER_PASSKEY_NTF:
		LOG_DBG("Received UI User Passkey Notification from SM");
		LOG_DBG(BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER,
		BT_DEVICE_ADDR_ONLY_PRINT_STR (bd_addr));

		/* Get Numeric Value */
		numeric_val   = event_data[3];
		numeric_val <<= 8;
		numeric_val  |= event_data[2];
		numeric_val <<= 8;
		numeric_val  |= event_data[1];
		numeric_val <<= 8;
		numeric_val  |= event_data[0];
		LOG_DBG("Numeric Value = %u (0x%08X)",
		(unsigned int) numeric_val, (unsigned int) numeric_val);
		if (NULL != conn)
		{
			user_passkey_ntf(conn, numeric_val);
		}
		break;
#endif /* BT_SSP_PE */

#if 0
#ifdef BT_SSP_OOB
	case SM_REMOTE_OOB_DATA_REQUEST_NTF:
		break;
#endif /* BT_SSP_OOB */

	case SM_SIMPLE_PAIRING_COMPLETE_NTF:
		LOG_DBG("Received UI Simple Pairing Complete from SM");
		LOG_DBG(BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER, BT_DEVICE_ADDR_ONLY_PRINT_STR (bd_addr));
		LOG_DBG("Status = 0x%02X", *event_data);
		if (NULL != conn)
		{
			uint8_t status = *event_data;
			ssp_pairing_complete(conn, bt_security_err_get(status));
			if (status) {
				bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
			}
		}
		break;
#endif /* 0 */
#endif /* BT_SSP */


	default:
		LOG_DBG("*** Unknown/Undefined Event Type 0x%02X", event_type);
		break;
	}

	if (NULL != conn)
	{
		bt_conn_unref(conn);
	}

	return retval;
}

static int ethermind_bt_sm_init(void)
{
	API_RESULT retVal = API_SUCCESS;

#if (defined(CONFIG_BT_SMP_SC_ONLY) && (CONFIG_BT_SMP_SC_ONLY > 0))
#ifdef BT_BRSC
	retVal = BT_sm_set_secure_connections_only_mode(0x01U);
#endif /* BT_BRSC */
#endif /* (defined(CONFIG_BT_SMP_SC_ONLY) && (CONFIG_BT_SMP_SC_ONLY > 0)) */

	if (API_SUCCESS == retVal)
	{
		retVal = BT_sm_set_local_io_cap(SM_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
	}

	if (API_SUCCESS == retVal)
	{
#ifdef CLASSIC_SEC_MANAGER
		/* Register with BR/EDR Security Module */
		retVal = BT_sm_register_user_interface (ethermind_br_sm_ui_notify_cb);
#endif /* CLASSIC_SEC_MANAGER */
	}

	if (API_SUCCESS == retVal)
	{
		return 0;
	}
	else
	{
		return -EIO;
	}
}

static void bt_smp_br_update_io_cap(const struct bt_conn_auth_cb *auth)
{
	uint8_t ioCap;

	if (NULL == auth)
	{
		ioCap = SM_IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
	}
	else
	{
		if ((NULL != auth->passkey_display)
		&& (NULL != auth->passkey_confirm))
		{
			ioCap = SM_IO_CAPABILITY_DISPLAY_YES_NO;
		}
		else if ((NULL != auth->passkey_entry))
		{
			ioCap = SM_IO_CAPABILITY_KEYBOARD_ONLY;
		}
		else if ((NULL != auth->passkey_display))
		{
			ioCap = SM_IO_CAPABILITY_DISPLAY_ONLY;
		}
		else
		{
			ioCap = SM_IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
		}
	}
	BT_sm_set_local_io_cap(ioCap);
}

#endif /* CONFIG_BT_CLASSIC */

#if (defined(CONFIG_BT_BLE_DISABLE) && ((CONFIG_BT_BLE_DISABLE) > 0U))

static bool bondable = IS_ENABLED(CONFIG_BT_BONDABLE);

void bt_smp_update_io_cap(const struct bt_conn_auth_cb *auth)
{
	bt_smp_br_update_io_cap(auth);
}

int bt_smp_init(void)
{
	int ret;

	ret = ethermind_bt_sm_init();
	assert (0 == ret);
	if (0 != ret)
	{
		return ret;
	}

	return 0;
}

int bt_smp_auth_cb_overlay(struct bt_conn *conn, const struct bt_conn_auth_cb *cb)
{
	return -ENOTSUP;
}

int bt_smp_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	return -ENOTSUP;
}

int bt_smp_auth_passkey_confirm(struct bt_conn *conn)
{
	return -ENOTSUP;
}
int bt_smp_auth_cancel(struct bt_conn *conn)
{
	return -ENOTSUP;
}
int bt_smp_auth_pairing_confirm(struct bt_conn *conn)
{
	return -ENOTSUP;
}
int bt_smp_start_security(struct bt_conn *conn)
{
	return -ENOTSUP;
}

int bt_smp_le_oob_set_tk(struct bt_conn *conn, const uint8_t *tk)
{
	return -ENOTSUP;
}
int bt_smp_le_oob_set_sc_data(struct bt_conn *conn,
				  const struct bt_le_oob_sc_data *oobd_local,
				  const struct bt_le_oob_sc_data *oobd_remote)
{
	return -ENOTSUP;
}

int bt_smp_le_oob_get_sc_data(struct bt_conn *conn,
				  const struct bt_le_oob_sc_data **oobd_local,
				  const struct bt_le_oob_sc_data **oobd_remote)
{
	return -ENOTSUP;
}

void bt_set_bondable(bool enable)
{
	bondable = enable;
	BT_sm_set_pairable((true == bondable)? SM_PAIRABLE_AND_BONDABLE : SM_PAIRABLE_AND_NON_BONDABLE);
}

#else

/* SMP channel specific context */
struct bt_smp {
	/* The channel this context is associated with. */
	struct bt_l2cap_le_chan		chan;

	SMP_AUTH_INFO			auth;

	/* Delayed work for timeout handling */
#if 0
	struct k_work_delayable		work;
#endif

	/* Delayed work for id add */
	struct k_work_delayable		id_add;

	/* status of auth complete */
	API_RESULT			status;

	/* Used Bluetooth authentication callbacks. */
	atomic_ptr_t			auth_cb;

	/* Commands that remote is allowed to send */
	ATOMIC_DEFINE(allowed_cmds, BT_SMP_NUM_CMDS);

	/* Flags for SMP state machine */
	ATOMIC_DEFINE(flags, SMP_NUM_FLAGS);

	/* Type of method used for pairing */
	uint8_t				method;

	/* Type of confirm */
	uint8_t				confirm_type;

	/* Pairing Request PDU */
	uint8_t				preq[7];

	/* Pairing Response PDU */
	uint8_t				prsp[7];

	/* Pairing Confirm PDU */
	uint8_t				pcnf[16];

	/* Local random number */
	uint8_t				prnd[16];

	/* Remote random number */
	uint8_t				rrnd[16];

	/* Temporary key */
	uint8_t				tk[16];

	/* Remote Public Key for LE SC */
	uint8_t				pkey[BT_PUB_KEY_LEN];

	/* DHKey */
	uint8_t				dhkey[BT_DH_KEY_LEN];

	/* Remote DHKey check */
	uint8_t				e[16];

	/* MacKey */
	uint8_t				mackey[16];

	/* LE SC passkey */
	uint32_t			passkey;

	/* LE SC passkey round */
	uint8_t				passkey_round;

	/* LE SC local OOB data */
	const struct bt_le_oob_sc_data	*oobd_local;

	/* LE SC remote OOB data */
	const struct bt_le_oob_sc_data	*oobd_remote;

	/* Local key distribution */
	uint8_t				local_dist;

	/* Remote key distribution */
	uint8_t				remote_dist;
	/* Bondable flag */
	atomic_t			bondable;
};

/* Global BD Address of the SMP procedure */
#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
DECL_STATIC BT_DEVICE_ADDR bt_smp_bd_addr;
DECL_STATIC UCHAR local_keys;
DECL_STATIC SMP_KEY_DIST peer_key_info; /* static to reduce stack usage */
#endif
static unsigned int fixed_passkey = BT_PASSKEY_INVALID;

#define DISPLAY_FIXED(smp) (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) && \
			    fixed_passkey != BT_PASSKEY_INVALID && \
			    (smp)->method == PASSKEY_DISPLAY)

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
/* based on table 2.8 Core Spec 2.3.5.1 Vol. 3 Part H */
static const uint8_t gen_method_legacy[5 /* remote */][5 /* local */] = {
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_DISPLAY },
	{ JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_ROLE },
};
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */

#if !(defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) && (CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY > 0))
/* based on table 2.8 Core Spec 2.3.5.1 Vol. 3 Part H */
static const uint8_t gen_method_sc[5 /* remote */][5 /* local */] = {
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ JUST_WORKS, PASSKEY_CONFIRM, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_CONFIRM },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_DISPLAY },
	{ JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS },
	{ PASSKEY_DISPLAY, PASSKEY_CONFIRM, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_CONFIRM },
};
#endif /* !CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

#endif /* CONFIG_BT_PERIPHERAL */

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
/* SMP over BR/EDR channel specific context */
struct bt_smp_br {
	/* The channel this context is associated with. */
	struct bt_l2cap_br_chan	chan;

	SMP_AUTH_INFO			auth;

	/* Delayed work for auth timeout handling */
	struct k_work_delayable		auth_timeout;

	/* pairing result */
	uint8_t				status;

	/* Commands that remote is allowed to send */
	ATOMIC_DEFINE(allowed_cmds, BT_SMP_NUM_CMDS);

	/* Flags for SMP state machine */
	ATOMIC_DEFINE(flags, SMP_NUM_FLAGS);

	/* Type of method used for pairing */
	uint8_t				method;

	/* Type of confirm */
	uint8_t				confirm_type;

	/* passkey */
	uint32_t			passkey;

	/* Local key distribution */
	uint8_t				local_dist;

	/* Remote key distribution */
	uint8_t				remote_dist;

	/* Encryption Key Size used for connection */
	uint8_t 			enc_key_size;

#if 0
	/* Delayed work for timeout handling */
	struct k_work_delayable 	work;
#endif
};

static struct bt_smp_br bt_smp_br_pool[CONFIG_BT_MAX_CONN];
#endif /* CONFIG_BT_CLASSIC */

static struct bt_smp bt_smp_pool[CONFIG_BT_MAX_CONN];
static bool bondable = IS_ENABLED(CONFIG_BT_BONDABLE);
static bool sc_oobd_present;
static bool legacy_oobd_present;
static bool sc_supported;
static const uint8_t *sc_public_key;
static osa_semaphore_handle_t sc_local_pkey_ready;
static OSA_SEMAPHORE_HANDLE_DEFINE(sc_local_pkey_ready_handle);
static osa_semaphore_handle_t sc_local_oobe_ready;
static OSA_SEMAPHORE_HANDLE_DEFINE(sc_local_oobe_ready_handle);
static struct bt_le_oob_sc_data current_oob_data;

static void bt_smp_get_auth_info(struct bt_conn *conn);
#if 0
static void le_sc_oob_config_set(struct bt_smp *smp,
				 struct bt_conn_oob_info *info);
#endif

#define SMP_LE_RX_PDU 256

struct smp_le_rx_pdu
{
    SMP_BD_HANDLE bd_handle;
    UCHAR      event;
    API_RESULT status;
};

struct bt_smp_hdr_sumilation
{
    struct bt_hci_acl_hdr_simulation hdr;
    struct smp_le_rx_pdu pdu;
};
#if (CONFIG_BT_MAX_CONN > 4)
#define BT_SMP_LE_RX_POOL_COUNT (CONFIG_BT_MAX_CONN * 2)
#else
#define BT_SMP_LE_RX_POOL_COUNT (CONFIG_BT_MAX_CONN * 4)
#endif

/* Pool for incoming SMP packets */
NET_BUF_POOL_DEFINE(smp_le_rx_pool, BT_SMP_LE_RX_POOL_COUNT, (sizeof(struct bt_smp_hdr_sumilation) + SMP_LE_RX_PDU + CONFIG_BT_HCI_RESERVE),
		    CONFIG_NET_BUF_USER_DATA_SIZE, NULL);

/* Pointer to internal data is used to mark that callbacks of given SMP channel are not initialized.
 * Value of NULL represents no authenticaiton capabilities and cannot be used for that purpose.
 */
#define BT_SMP_AUTH_CB_UNINITIALIZED	((atomic_ptr_val_t)bt_smp_pool)

/* Value used to mark that per-connection bondable flag is not initialized.
 * Value false/true represent if flag is cleared or set and cannot be used for that purpose.
 */
#define BT_SMP_BONDABLE_UNINITIALIZED	((atomic_val_t)-1)

static bool le_sc_supported(void)
{
	/*
	 * If controller based ECC is to be used it must support
	 * "LE Read Local P-256 Public Key" and "LE Generate DH Key" commands.
	 * Otherwise LE SC are not supported.
	 */
	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		return false;
	}

	return BT_CMD_TEST(bt_dev.supported_commands, 34, 1) &&
	       BT_CMD_TEST(bt_dev.supported_commands, 34, 2);
}

static const struct bt_conn_auth_cb *latch_auth_cb(struct bt_smp *smp)
{
	(void)atomic_ptr_cas(&smp->auth_cb, BT_SMP_AUTH_CB_UNINITIALIZED,
			     (atomic_ptr_val_t)bt_auth);

	return atomic_ptr_get(&smp->auth_cb);
}

static bool latch_bondable(struct bt_smp *smp)
{
	(void)atomic_cas(&smp->bondable, BT_SMP_BONDABLE_UNINITIALIZED, (atomic_val_t)bondable);

	return atomic_get(&smp->bondable);
}

static uint8_t get_io_capa(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);

	if (!smp_auth_cb) {
		goto no_callbacks;
	}

	/* Passkey Confirmation is valid only for LE SC */
	if (smp_auth_cb->passkey_display && smp_auth_cb->passkey_entry &&
	    (smp_auth_cb->passkey_confirm || !sc_supported)) {
		return BT_SMP_IO_KEYBOARD_DISPLAY;
	}

	/* DisplayYesNo is useful only for LE SC */
	if (sc_supported && smp_auth_cb->passkey_display &&
	    smp_auth_cb->passkey_confirm) {
		return BT_SMP_IO_DISPLAY_YESNO;
	}

	if (smp_auth_cb->passkey_entry) {
		if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
		    fixed_passkey != BT_PASSKEY_INVALID) {
			return BT_SMP_IO_KEYBOARD_DISPLAY;
		} else {
			return BT_SMP_IO_KEYBOARD_ONLY;
		}
	}

	if (smp_auth_cb->passkey_display) {
		return BT_SMP_IO_DISPLAY_ONLY;
	}

no_callbacks:
	if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
	    fixed_passkey != BT_PASSKEY_INVALID) {
		return BT_SMP_IO_DISPLAY_ONLY;
	} else {
		return BT_SMP_IO_NO_INPUT_OUTPUT;
	}
}

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
static uint8_t legacy_get_pair_method(struct bt_smp *smp, uint8_t remote_io);
#endif
#endif /* CONFIG_BT_PERIPHERAL */

static bool smp_keys_check(struct bt_conn *conn)
{
	if (atomic_test_bit(conn->flags, BT_CONN_FORCE_PAIR)) {
		return false;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_find(BT_KEYS_LTK_P256,
						     conn->id, &conn->le.dst);
		if (!conn->le.keys) {
			conn->le.keys = bt_keys_find(BT_KEYS_LTK,
						     conn->id,
						     &conn->le.dst);
		}
	}

	if (!conn->le.keys ||
	    !(conn->le.keys->keys & (BT_KEYS_LTK | BT_KEYS_LTK_P256))) {
		return false;
	}

	if (conn->required_sec_level >= BT_SECURITY_L3 &&
	    !(conn->le.keys->flags & BT_KEYS_AUTHENTICATED)) {
		return false;
	}

	if (conn->required_sec_level >= BT_SECURITY_L4 &&
	    !((conn->le.keys->flags & BT_KEYS_AUTHENTICATED) &&
	      (conn->le.keys->keys & BT_KEYS_LTK_P256) &&
	      (conn->le.keys->enc_size == BT_SMP_MAX_ENC_KEY_SIZE))) {
		return false;
	}

	return true;
}

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
static uint8_t get_pair_method(struct bt_smp *smp, uint8_t remote_io)
{
#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return legacy_get_pair_method(smp, remote_io);
	}
#endif /* CONFIG_BT_PERIPHERAL */

#if !(defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) && (CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY > 0))
	struct bt_smp_pairing *req, *rsp;

	req = (struct bt_smp_pairing *)&smp->preq[1];
	rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	if ((req->auth_req & rsp->auth_req) & BT_SMP_AUTH_SC) {
		/* if one side has OOB data use OOB */
		if ((req->oob_flag | rsp->oob_flag) & BT_SMP_OOB_DATA_MASK) {
			return LE_SC_OOB;
		}
	}

	if (remote_io > BT_SMP_IO_KEYBOARD_DISPLAY) {
		return JUST_WORKS;
	}

	/* if none side requires MITM use JustWorks */
	if (!((req->auth_req | rsp->auth_req) & BT_SMP_AUTH_MITM)) {
		return JUST_WORKS;
	}

	return gen_method_sc[remote_io][get_io_capa(smp)];
#else
	return JUST_WORKS;
#endif
}
#endif /* CONFIG_BT_PERIPHERAL */

static enum bt_security_err security_err_get(uint8_t smp_err)
{
	switch (smp_err) {
    case BT_SMP_ERR_SUCCESS:
        return BT_SECURITY_ERR_SUCCESS;
	case BT_SMP_ERR_PASSKEY_ENTRY_FAILED:
	case BT_SMP_ERR_DHKEY_CHECK_FAILED:
	case BT_SMP_ERR_NUMERIC_COMP_FAILED:
	case BT_SMP_ERR_CONFIRM_FAILED:
		return BT_SECURITY_ERR_AUTH_FAIL;
	case BT_SMP_ERR_OOB_NOT_AVAIL:
		return BT_SECURITY_ERR_OOB_NOT_AVAILABLE;
	case BT_SMP_ERR_AUTH_REQUIREMENTS:
	case BT_SMP_ERR_ENC_KEY_SIZE:
		return BT_SECURITY_ERR_AUTH_REQUIREMENT;
	case BT_SMP_ERR_PAIRING_NOTSUPP:
	case BT_SMP_ERR_CMD_NOTSUPP:
		return BT_SECURITY_ERR_PAIR_NOT_SUPPORTED;
	case BT_SMP_ERR_REPEATED_ATTEMPTS:
	case BT_SMP_ERR_BREDR_PAIRING_IN_PROGRESS:
	case BT_SMP_ERR_CROSS_TRANSP_NOT_ALLOWED:
		return BT_SECURITY_ERR_PAIR_NOT_ALLOWED;
	case BT_SMP_ERR_INVALID_PARAMS:
		return BT_SECURITY_ERR_INVALID_PARAM;
	case BT_SMP_ERR_KEY_REJECTED:
		return BT_SECURITY_ERR_KEY_REJECTED;
	case BT_SMP_ERR_REMOTE_SIDE_PIN_KEY_MISSING:
		return BT_SECURITY_ERR_PIN_OR_KEY_MISSING;
	case BT_SMP_ERR_UNSPECIFIED:
	default:
		return BT_SECURITY_ERR_UNSPECIFIED;
	}
}

static uint8_t smp_err_get(enum bt_security_err auth_err)
{
	switch (auth_err) {
	case BT_SECURITY_ERR_OOB_NOT_AVAILABLE:
		return BT_SMP_ERR_OOB_NOT_AVAIL;

	case BT_SECURITY_ERR_AUTH_FAIL:
	case BT_SECURITY_ERR_AUTH_REQUIREMENT:
		return BT_SMP_ERR_AUTH_REQUIREMENTS;

	case BT_SECURITY_ERR_PAIR_NOT_SUPPORTED:
		return BT_SMP_ERR_PAIRING_NOTSUPP;

	case BT_SECURITY_ERR_INVALID_PARAM:
		return BT_SMP_ERR_INVALID_PARAMS;

	case BT_SECURITY_ERR_PIN_OR_KEY_MISSING:
	case BT_SECURITY_ERR_PAIR_NOT_ALLOWED:
	case BT_SECURITY_ERR_UNSPECIFIED:
		return BT_SMP_ERR_UNSPECIFIED;
	default:
		return 0;
	}
}

#if 0
static struct net_buf *smp_create_pdu(struct bt_smp *smp, uint8_t op, size_t len)
{
	struct bt_smp_hdr *hdr;
	struct net_buf *buf;
	size_t timeout;

	/* Don't if session had already timed out */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		timeout = osaWaitNone_c;
	} else {
		timeout = SMP_TIMEOUT;
	}

	/* Use smaller timeout if returning an error since that could be
	 * caused by lack of buffers.
	 */
	buf = bt_l2cap_create_pdu_timeout(NULL, 0, timeout);
	if (!buf) {
		/* If it was not possible to allocate a buffer within the
		 * timeout marked it as timed out.
		 */
		atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
		return NULL;
	}

	hdr = (struct bt_smp_hdr *)net_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}
#endif

#if defined(CONFIG_BT_SMP_SELFTEST) && (CONFIG_BT_SMP_SELFTEST > 0U)
/* Cypher based Message Authentication Code (CMAC) with AES 128 bit
 *
 * Input    : key    ( 128-bit key )
 *          : in     ( message to be authenticated )
 *          : len    ( length of the message in octets )
 * Output   : out    ( message authentication code )
 */
static int bt_smp_aes_cmac(const uint8_t *key, const uint8_t *in, size_t len,
			   uint8_t *out)
{
    return bt_aes_128_cmac_be(key, in, len, out);
}
#endif

static int smp_d1(const uint8_t *key, uint16_t d, uint16_t r, uint8_t res[16])
{
	int err;

	LOG_DBG("key %s d %u r %u", bt_hex(key, 16), d, r);

	sys_put_le16(d, &res[0]);
	sys_put_le16(r, &res[2]);
	memset(&res[4], 0, 16 - 4);

	err = bt_encrypt_le(key, res, res);
	if (err) {
		return err;
	}

	LOG_DBG("res %s", bt_hex(res, 16));
	return 0;
}


static uint8_t get_encryption_key_size(struct bt_smp *smp)
{
	struct bt_smp_pairing *req, *rsp;

	req = (struct bt_smp_pairing *)&smp->preq[1];
	rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	/*
	 * The smaller value of the initiating and responding devices maximum
	 * encryption key length parameters shall be used as the encryption key
	 * size.
	 */
	return MIN(req->max_key_size, rsp->max_key_size);
}
#if 0
/* Check that if a new pairing procedure with an existing bond will not lower
 * the established security level of the bond.
 */
static bool update_keys_check(struct bt_smp *smp, struct bt_keys *keys)
{
	if (IS_ENABLED(CONFIG_BT_SMP_DISABLE_LEGACY_JW_PASSKEY) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SC) &&
	    smp->method != LEGACY_OOB) {
		return false;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) &&
	    smp->method != LEGACY_OOB) {
		return false;
	}

	if (!keys ||
	    !(keys->keys & (BT_KEYS_LTK_P256 | BT_KEYS_LTK))) {
		return true;
	}

	if (keys->enc_size > get_encryption_key_size(smp)) {
		return false;
	}

	if ((keys->keys & BT_KEYS_LTK_P256) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return false;
	}

	if ((keys->flags & BT_KEYS_AUTHENTICATED) &&
	     smp->method == JUST_WORKS) {
		return false;
	}

	if (!IS_ENABLED(CONFIG_BT_SMP_ALLOW_UNAUTH_OVERWRITE) &&
	    (!(keys->flags & BT_KEYS_AUTHENTICATED)
	     && smp->method == JUST_WORKS)) {
		if (!IS_ENABLED(CONFIG_BT_ID_ALLOW_UNAUTH_OVERWRITE) ||
		    (keys->id == smp->chan.chan.conn->id)) {
			return false;
		}
	}

	return true;
}
#endif
#if 0
static bool update_debug_keys_check(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
	}

	if (!conn->le.keys ||
	    !(conn->le.keys->keys & (BT_KEYS_LTK_P256 | BT_KEYS_LTK))) {
		return true;
	}

	if (conn->le.keys->flags & BT_KEYS_DEBUG) {
		return true;
	}

	return false;
}
#endif
#if (defined(CONFIG_BT_PRIVACY) && ((CONFIG_BT_PRIVACY) > 0U)) || (defined(CONFIG_BT_SIGNING) && (CONFIG_BT_SIGNING > 0U)) || \
	!(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0U))
/* For TX callbacks */
static void smp_pairing_complete(struct bt_smp *smp, uint8_t status);
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
#if 0
static void smp_pairing_br_complete(struct bt_smp_br *smp, uint8_t status);
#endif
#endif

#if 0
static void smp_check_complete(struct bt_conn *conn, uint8_t dist_complete)
{
	struct bt_l2cap_chan *chan;

	if (conn->type == BT_CONN_TYPE_LE) {
		struct bt_smp *smp;

		chan = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_SMP);
		__ASSERT(chan, "No SMP channel found");

		smp = CONTAINER_OF(chan, struct bt_smp, chan.chan);
		smp->local_dist &= ~dist_complete;

		/* if all keys were distributed, pairing is done */
		if (!smp->local_dist && !smp->remote_dist) {
			smp_pairing_complete(smp, 0);
		}

		return;
	}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	if (conn->type == BT_CONN_TYPE_BR) {
		struct bt_smp_br *smp;

		chan = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_BR_SMP);
		__ASSERT(chan, "No SMP channel found");

		smp = CONTAINER_OF(chan, struct bt_smp_br, chan.chan);
		smp->local_dist &= ~dist_complete;

		/* if all keys were distributed, pairing is done */
		if (!smp->local_dist && !smp->remote_dist) {
			smp_pairing_br_complete(smp, 0);
		}
	}
#endif
}
#endif
#endif

#if (defined(CONFIG_BT_PRIVACY) && ((CONFIG_BT_PRIVACY) > 0U))
#if 0
static void smp_id_sent(struct bt_conn *conn, void *user_data)
{
	if (!err) {
		smp_check_complete(conn, BT_SMP_DIST_ID_KEY);
	}
}
#endif
#endif /* CONFIG_BT_PRIVACY */

#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
#if 0
static void smp_sign_info_sent(struct bt_conn *conn, void *user_data)
{
	if (!err) {
		smp_check_complete(conn, BT_SMP_DIST_SIGN);
	}
}
#endif
#endif /* CONFIG_BT_SIGNING */

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))

static void sc_derive_link_key(struct bt_smp *smp)
{
	/* constants as specified in Core Spec Vol.3 Part H 2.4.2.4 */
	static const uint8_t lebr[4] = { 0x72, 0x62, 0x65, 0x6c };
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys_link_key *link_key;
	uint8_t ilk[16];

	LOG_DBG("");

	/* TODO handle errors? */

	/*
	 * At this point remote device identity is known so we can use
	 * destination address here
	 */
	link_key = bt_keys_get_link_key(&conn->le.dst.a);
	if (!link_key) {
		return;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_CT2)) {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.4 */
		static const uint8_t salt[16] = { 0x31, 0x70, 0x6d, 0x74,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00 };

		if (bt_crypto_h7(salt, conn->le.keys->ltk.val, ilk)) {
			bt_keys_link_key_clear(link_key);
			return;
		}
	} else {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.4 */
		static const uint8_t tmp1[4] = { 0x31, 0x70, 0x6d, 0x74 };

		if (bt_crypto_h6(conn->le.keys->ltk.val, tmp1, ilk)) {
			bt_keys_link_key_clear(link_key);
			return;
		}
	}

	if (bt_crypto_h6(ilk, lebr, link_key->val)) {
		bt_keys_link_key_clear(link_key);
	}

	link_key->flags |= BT_LINK_KEY_SC;

	if (conn->le.keys->flags & BT_KEYS_AUTHENTICATED) {
		link_key->flags |= BT_LINK_KEY_AUTHENTICATED;
	} else {
		link_key->flags &= ~BT_LINK_KEY_AUTHENTICATED;
	}
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_keys_link_key_store(link_key);
	}
}

static void smp_br_reset(struct bt_smp_br *smp)
{
	/* Clear flags first in case canceling of timeout fails. The SMP context
	 * shall be marked as timed out in that case.
	 */
	atomic_set(smp->flags, 0);

	/* If canceling fails the timeout handler will set the timeout flag and
	 * mark the it as timed out. No new pairing procedures shall be started
	 * on this connection if that happens.
	 */
#if 0
	(void)k_work_cancel_delayable(&smp->work);
#endif

	atomic_set(smp->allowed_cmds, 0);

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);
}

static void smp_pairing_br_complete(struct bt_smp_br *smp, uint8_t status)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys;
	bt_addr_le_t addr;

	LOG_DBG("status 0x%x", status);

	/* For dualmode devices LE address is same as BR/EDR address
	 * and is of public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;
	keys = bt_keys_find_addr(conn->id, &addr);

	if (status) {
		struct bt_conn_auth_info_cb *listener, *next;

		if (keys) {
			bt_keys_clear(keys);
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_auth_info_cbs, listener,
						  next, node) {
			if (listener->pairing_failed) {
				listener->pairing_failed(smp->chan.chan.conn,
							 security_err_get(status));
			}
		}
	} else {
		bool bond_flag = atomic_test_bit(smp->flags, SMP_FLAG_BOND);
		struct bt_conn_auth_info_cb *listener, *next;

		if (bond_flag && keys) {
			bt_keys_store(keys);
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_auth_info_cbs, listener,
						  next, node) {
			if (listener->pairing_complete) {
				listener->pairing_complete(smp->chan.chan.conn,
							   bond_flag);
			}
		}
	}

	smp_br_reset(smp);
}

#if 0
static void smp_br_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_smp_br *smp = CONTAINER_OF(dwork, struct bt_smp_br, work);

	LOG_ERR("SMP Timeout");

	smp_pairing_br_complete(smp, BT_SMP_ERR_UNSPECIFIED);
	atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
}
#endif
static void smp_br_send(struct bt_smp_br *smp, struct net_buf *buf,
			bt_conn_tx_cb_t cb)
{
	int err = bt_l2cap_send_cb(smp->chan.chan.conn, BT_L2CAP_CID_BR_SMP, buf, cb, NULL);

	if (err) {
		if (err == -ENOBUFS) {
			LOG_ERR("Ran out of TX buffers or contexts.");
		}

		net_buf_unref(buf);
		return;
	}
#if 0
	k_work_reschedule(&smp->work, SMP_TIMEOUT);
#endif
}

static uint8_t smp_br_pairing_req(struct bt_smp_br *smp, struct bt_smp_pairing *req, SMP_AUTH_INFO *auth);

static void smp_br_auth_starting(struct bt_smp_br *smp)
{
	struct bt_conn *conn;
	int ret;
	API_RESULT retval;

	conn = smp->chan.chan.conn;

	if (BT_HCI_ROLE_CENTRAL == conn->role)
	{
#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0U))
		ret = bt_smp_br_send_pairing_req(conn);
#else
		ret = 0;
#endif
	}
	else
	{
		uint8_t preq[7];
		retval = BT_smp_get_pairing_req_pdu((SMP_BD_HANDLE *)&conn->deviceId, (UCHAR *)&preq[0]);
		if (API_SUCCESS == retval)
		{
			ret = smp_br_pairing_req(smp, (struct bt_smp_pairing*)&preq[1], &smp->auth);
			if (ret == 0)
			{
				k_work_schedule(&smp->auth_timeout, SMP_TIMEOUT);
			}
		}
		else
		{
			ret = -1;
		}
	}

	if (0 != ret)
	{
		smp->auth.param = ret;
		retval = BT_smp_authentication_request_reply
		(
			(SMP_BD_HANDLE *)&conn->deviceId,
			&smp->auth
		);
	}
}

static void smp_br_auth_complete(struct bt_smp_br *smp)
{
	smp_pairing_br_complete(smp, smp->status);
}

static void smp_br_auth_timeout(struct k_work *work)
{
	struct bt_smp_br *smp = CONTAINER_OF(work, struct bt_smp_br, auth_timeout);

	smp_pairing_br_complete(smp, BT_SMP_ERR_UNSPECIFIED);
}

static void bt_smp_br_connected(struct bt_l2cap_chan *chan)
{
	struct bt_smp_br *smp = CONTAINER_OF(chan, struct bt_smp_br, chan.chan);

	LOG_DBG("chan %p cid 0x%04x", chan,
	       CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan)->tx.cid);

	atomic_set_bit(smp->flags, SMP_FLAG_BR_CONNECTED);

	k_work_init_delayable(&smp->auth_timeout, smp_br_auth_timeout);

	/*
	 * if this flag is set it means pairing was requested before channel
	 * was connected
	 */
#if 0
	if (atomic_test_bit(smp->flags, SMP_FLAG_BR_PAIR)) {
		bt_smp_br_send_pairing_req(chan->conn);
	}
#endif
}

static void bt_smp_br_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_smp_br *smp = CONTAINER_OF(chan, struct bt_smp_br, chan.chan);

	LOG_DBG("chan %p cid 0x%04x", chan,
	       CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan)->tx.cid);
#if 0
	/* Channel disconnected callback is always called from a work handler
	 * so canceling of the timeout work should always succeed.
	 */
	(void)k_work_cancel_delayable(&smp->work);
#endif

	k_work_cancel_delayable(&smp->auth_timeout);

	(void)memset(smp, 0, sizeof(*smp));
}

static void smp_br_init(struct bt_smp_br *smp)
{
	/* Initialize SMP context without clearing L2CAP channel context */
	(void)memset(((uint8_t *)(void *)smp) + offsetof(struct bt_smp_br, allowed_cmds), 0,
		     sizeof(*smp) - offsetof(struct bt_smp, allowed_cmds));

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_FAIL);
}
#if 0
static void smp_br_derive_ltk(struct bt_smp_br *smp)
{
	/* constants as specified in Core Spec Vol.3 Part H 2.4.2.5 */
	static const uint8_t brle[4] = { 0x65, 0x6c, 0x72, 0x62 };
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys_link_key *link_key = conn->br.link_key;
	struct bt_keys *keys;
	bt_addr_le_t addr;
	uint8_t ilk[16];

	LOG_DBG("");

	if (!link_key) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_FORCE_BREDR) && conn->encrypt != 0x02) {
		LOG_WRN("Using P192 Link Key for P256 LTK derivation");
	}

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_LTK_P256, conn->id, &addr);
	if (!keys) {
		LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&addr));
		return;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_CT2)) {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.5 */
		static const uint8_t salt[16] = { 0x32, 0x70, 0x6d, 0x74,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00,
					       0x00, 0x00, 0x00, 0x00 };

		if (bt_crypto_h7(salt, link_key->val, ilk)) {
			bt_keys_link_key_clear(link_key);
			return;
		}
	} else {
		/* constants as specified in Core Spec Vol.3 Part H 2.4.2.5 */
		static const uint8_t tmp2[4] = { 0x32, 0x70, 0x6d, 0x74 };

		if (bt_crypto_h6(link_key->val, tmp2, ilk)) {
			bt_keys_clear(keys);
			return;
		}
	}

	if (bt_crypto_h6(ilk, brle, keys->ltk.val)) {
		bt_keys_clear(keys);
		return;
	}

	(void)memset(keys->ltk.ediv, 0, sizeof(keys->ltk.ediv));
	(void)memset(keys->ltk.rand, 0, sizeof(keys->ltk.rand));
	keys->enc_size = smp->enc_key_size;

	if (link_key->flags & BT_LINK_KEY_AUTHENTICATED) {
		keys->flags |= BT_KEYS_AUTHENTICATED;
	} else {
		keys->flags &= ~BT_KEYS_AUTHENTICATED;
	}

	LOG_DBG("LTK derived from LinkKey");
}
#endif
static struct net_buf *smp_br_create_pdu(struct bt_smp_br *smp, uint8_t op,
					 size_t len)
{
	struct bt_smp_hdr *hdr;
	struct net_buf *buf;
	size_t timeout;

	/* Don't if session had already timed out */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		timeout = osaWaitNone_c;
	} else {
		timeout = SMP_TIMEOUT;
	}

	/* Use smaller timeout if returning an error since that could be
	 * caused by lack of buffers.
	 */
	buf = bt_l2cap_create_pdu_timeout(NULL, 0, timeout);
	if (!buf) {
		/* If it was not possible to allocate a buffer within the
		 * timeout marked it as timed out.
		 */
		atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
		return NULL;
	}

	hdr = (struct bt_smp_hdr *)net_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}
#if 0
static void smp_br_distribute_keys(struct bt_smp_br *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys;
	bt_addr_le_t addr;

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_addr(conn->id, &addr);
	if (!keys) {
		LOG_ERR("No keys space for %s", bt_addr_le_str(&addr));
		return;
	}

#if (defined(CONFIG_BT_PRIVACY) && ((CONFIG_BT_PRIVACY) > 0U))
	if (smp->local_dist & BT_SMP_DIST_ID_KEY) {
		struct bt_smp_ident_info *id_info;
		struct bt_smp_ident_addr_info *id_addr_info;
		struct net_buf *buf;

		smp->local_dist &= ~BT_SMP_DIST_ID_KEY;

		buf = smp_br_create_pdu(smp, BT_SMP_CMD_IDENT_INFO,
					sizeof(*id_info));
		if (!buf) {
			LOG_ERR("Unable to allocate Ident Info buffer");
			return;
		}

		id_info = net_buf_add(buf, sizeof(*id_info));
		memcpy(id_info->irk, bt_dev.irk[conn->id], 16);

		smp_br_send(smp, buf, NULL);

		buf = smp_br_create_pdu(smp, BT_SMP_CMD_IDENT_ADDR_INFO,
				     sizeof(*id_addr_info));
		if (!buf) {
			LOG_ERR("Unable to allocate Ident Addr Info buffer");
			return;
		}

		id_addr_info = net_buf_add(buf, sizeof(*id_addr_info));
		bt_addr_le_copy(&id_addr_info->addr, &bt_dev.id_addr[conn->id]);

		smp_br_send(smp, buf, smp_id_sent);
	}
#endif /* CONFIG_BT_PRIVACY */

#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
	if (smp->local_dist & BT_SMP_DIST_SIGN) {
		struct bt_smp_signing_info *info;
		struct net_buf *buf;

		smp->local_dist &= ~BT_SMP_DIST_SIGN;

		buf = smp_br_create_pdu(smp, BT_SMP_CMD_SIGNING_INFO,
					sizeof(*info));
		if (!buf) {
			LOG_ERR("Unable to allocate Signing Info buffer");
			return;
		}

		info = net_buf_add(buf, sizeof(*info));

		if (bt_rand(info->csrk, sizeof(info->csrk))) {
			LOG_ERR("Unable to get random bytes");
			return;
		}

		if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
			bt_keys_add_type(keys, BT_KEYS_LOCAL_CSRK);
			memcpy(keys->local_csrk.val, info->csrk, 16);
			keys->local_csrk.cnt = 0U;
		}

		smp_br_send(smp, buf, smp_sign_info_sent);
	}
#endif /* CONFIG_BT_SIGNING */
}
#endif
static bool smp_br_pairing_allowed(struct bt_smp_br *smp)
{
	if (smp->chan.chan.conn->encrypt == 0x02) {
		return true;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_FORCE_BREDR) &&
	    smp->chan.chan.conn->encrypt == 0x01) {
		LOG_WRN("Allowing BR/EDR SMP with P-192 key");
		return true;
	}

	return false;
}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
static uint8_t send_br_pairing_rsp(struct bt_smp_br *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
    API_RESULT retval;

	retval = BT_smp_authentication_request_reply
			(
				&conn->deviceId,
				&smp->auth
			);
	if (API_SUCCESS != retval)
	{
		return (uint8_t)(retval & 0x00FF);
	}
	else
	{
		return 0;
	}
}

static uint8_t smp_br_pairing_req(struct bt_smp_br *smp, struct bt_smp_pairing *req, SMP_AUTH_INFO *auth)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t max_key_size;
	struct bt_smp_pairing rsp[1];
	uint8_t keyDistribution = 0;
    API_RESULT retVal;

	LOG_DBG("req: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		req->io_capability, req->oob_flag, req->auth_req,
		req->max_key_size, req->init_key_dist, req->resp_key_dist);

	auth->param = SMP_ERROR_NONE;

	/*
	 * If a Pairing Request is received over the BR/EDR transport when
	 * either cross-transport key derivation/generation is not supported or
	 * the BR/EDR transport is not encrypted using a Link Key generated
	 * using P256, a Pairing Failed shall be sent with the error code
	 * "Cross-transport Key Derivation/Generation not allowed" (0x0E)."
	 */
#if 0
	if (!smp_br_pairing_allowed(smp)) {
		return BT_SMP_ERR_CROSS_TRANSP_NOT_ALLOWED;
	}
#endif

	max_key_size = bt_conn_enc_key_size(conn);
	if (!max_key_size) {
		LOG_DBG("Invalid encryption key size");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (req->max_key_size != max_key_size) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

#if 0
	smp_br_init(smp);
	smp->enc_key_size = max_key_size;
#endif

	/*
	 * If Secure Connections pairing has been initiated over BR/EDR, the IO
	 * Capability, OOB data flag and Auth Req fields of the SM Pairing
	 * Request/Response PDU shall be set to zero on transmission, and
	 * ignored on reception.
	 */
#if 0
	rsp = net_buf_add(rsp_buf, sizeof(*rsp));
#endif

	rsp->auth_req = 0x00;
	rsp->io_capability = 0x00;
	rsp->oob_flag = 0x00;
	rsp->max_key_size = max_key_size;
	rsp->init_key_dist = (req->init_key_dist & BR_RECV_KEYS_SC);
	rsp->resp_key_dist = (req->resp_key_dist & BR_RECV_KEYS_SC);

	smp->local_dist = rsp->resp_key_dist;
	smp->remote_dist = rsp->init_key_dist;

	/* for Local */
	keyDistribution = rsp->resp_key_dist;
	/* for Remote */
	keyDistribution |= (rsp->init_key_dist) << 4;
	retVal = BT_smp_set_key_distribution_flag_pl(keyDistribution);
	if (API_SUCCESS != retVal)
	{
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if ((rsp->auth_req & BT_SMP_AUTH_CT2) &&
	    (req->auth_req & BT_SMP_AUTH_CT2)) {
		atomic_set_bit(smp->flags, SMP_FLAG_CT2);
	}
#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
	/* Enable H7 support if Cross transport keygen is requested */
	if (auth->xtx_info & SMP_XTX_KEYGEN_MASK)
	{
		auth->xtx_info |= SMP_XTX_H7_MASK;
	}
#endif

	auth->bonding = SMP_BONDING_NONE;

	memcpy(&smp->auth, auth, sizeof(smp->auth));

	LOG_DBG("rsp: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		rsp->io_capability, rsp->oob_flag, rsp->auth_req,
		rsp->max_key_size, rsp->init_key_dist, rsp->resp_key_dist);
	send_br_pairing_rsp(smp);

#if 0
	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);

	/* derive LTK if requested and clear distribution bits */
	if ((smp->local_dist & BT_SMP_DIST_ENC_KEY) &&
	    (smp->remote_dist & BT_SMP_DIST_ENC_KEY)) {
		smp_br_derive_ltk(smp);
	}
	smp->local_dist &= ~BT_SMP_DIST_ENC_KEY;
	smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;

	/* BR/EDR acceptor is like LE Peripheral and distributes keys first */
	smp_br_distribute_keys(smp);

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_br_complete(smp, 0);
	}
#endif
    (void)rsp;
	return 0;
}
#endif /* CONFIG_BT_CLASSIC */

#if 0
static uint8_t smp_br_pairing_rsp(struct bt_smp_br *smp, struct net_buf *buf)
{
	struct bt_smp_pairing *rsp = (void *)buf->data;
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t max_key_size;

	LOG_DBG("rsp: io_capability 0x%02X, oob_flag 0x%02X, auth_req 0x%02X, "
		"max_key_size 0x%02X, init_key_dist 0x%02X, resp_key_dist 0x%02X",
		rsp->io_capability, rsp->oob_flag, rsp->auth_req,
		rsp->max_key_size, rsp->init_key_dist, rsp->resp_key_dist);

	max_key_size = bt_conn_enc_key_size(conn);
	if (!max_key_size) {
		LOG_DBG("Invalid encryption key size");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (rsp->max_key_size != max_key_size) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	smp->local_dist &= rsp->init_key_dist;
	smp->remote_dist &= rsp->resp_key_dist;

	smp->local_dist &= SEND_KEYS_SC;
	smp->remote_dist &= RECV_KEYS_SC;

	/* Peripheral distributes its keys first */

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	/* derive LTK if requested and clear distribution bits */
	if ((smp->local_dist & BT_SMP_DIST_ENC_KEY) &&
	    (smp->remote_dist & BT_SMP_DIST_ENC_KEY)) {
		smp_br_derive_ltk(smp);
	}
	smp->local_dist &= ~BT_SMP_DIST_ENC_KEY;
	smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;

	/* Pairing acceptor distributes it's keys first */
	if (smp->remote_dist) {
		return 0;
	}

	smp_br_distribute_keys(smp);

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_br_complete(smp, 0);
	}

	return 0;
}

static uint8_t smp_br_pairing_failed(struct bt_smp_br *smp, struct net_buf *buf)
{
	struct bt_smp_pairing_fail *req = (void *)buf->data;

	LOG_ERR("pairing failed (peer reason 0x%x)", req->reason);

	smp_pairing_br_complete(smp, req->reason);
	smp_br_reset(smp);

	/* return no error to avoid sending Pairing Failed in response */
	return 0;
}

static uint8_t smp_br_ident_info(struct bt_smp_br *smp, struct net_buf *buf)
{
	struct bt_smp_ident_info *req = (void *)buf->data;
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys;
	bt_addr_le_t addr;

	LOG_DBG("");

	/* TODO should we resolve LE address if matching RPA is connected? */

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &addr);
	if (!keys) {
		LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&addr));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->irk.val, req->irk, sizeof(keys->irk.val));

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_ADDR_INFO);

	return 0;
}

static uint8_t smp_br_ident_addr_info(struct bt_smp_br *smp,
				      struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_ident_addr_info *req = (void *)buf->data;
	bt_addr_le_t addr;

	LOG_DBG("identity %s", bt_addr_le_str(&req->addr));

	/*
	 * For dual mode device identity address must be same as BR/EDR address
	 * and be of public type. So if received one doesn't match BR/EDR
	 * address we fail.
	 */

	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	if (!bt_addr_le_eq(&addr, &req->addr)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp->remote_dist &= ~BT_SMP_DIST_ID_KEY;

	if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (conn->role == BT_CONN_ROLE_CENTRAL && !smp->remote_dist) {
		smp_br_distribute_keys(smp);
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_br_complete(smp, 0);
	}

	return 0;
}
#endif
#if 0
#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
static uint8_t smp_br_signing_info(struct bt_smp_br *smp, struct net_buf *buf)
{
	struct bt_smp_signing_info *req = (void *)buf->data;
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys;
	bt_addr_le_t addr;

	LOG_DBG("");

	/*
	 * For dualmode devices LE address is same as BR/EDR address and is of
	 * public type.
	 */
	bt_addr_copy(&addr.a, &conn->br.dst);
	addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_REMOTE_CSRK, conn->id, &addr);
	if (!keys) {
		LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&addr));
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(keys->remote_csrk.val, req->csrk, sizeof(keys->remote_csrk.val));

	smp->remote_dist &= ~BT_SMP_DIST_SIGN;

	if (conn->role == BT_CONN_ROLE_CENTRAL && !smp->remote_dist) {
		smp_br_distribute_keys(smp);
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_br_complete(smp, 0);
	}

	return 0;
}
#else
static uint8_t smp_br_signing_info(struct bt_smp_br *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BT_SIGNING */
#endif
#if 0
static const struct {
	uint8_t  (*func)(struct bt_smp_br *smp, struct net_buf *buf);
	uint8_t  expect_len;
} br_handlers[] = {
	{ }, /* No op-code defined for 0x00 */
	{ smp_br_pairing_req,      sizeof(struct bt_smp_pairing) },
	{ smp_br_pairing_rsp,      sizeof(struct bt_smp_pairing) },
	{ }, /* pairing confirm not used over BR/EDR */
	{ }, /* pairing random not used over BR/EDR */
	{ smp_br_pairing_failed,   sizeof(struct bt_smp_pairing_fail) },
	{ }, /* encrypt info not used over BR/EDR */
	{ }, /* central ident not used over BR/EDR */
	{ smp_br_ident_info,       sizeof(struct bt_smp_ident_info) },
	{ smp_br_ident_addr_info,  sizeof(struct bt_smp_ident_addr_info) },
	{ smp_br_signing_info,     sizeof(struct bt_smp_signing_info) },
	/* security request not used over BR/EDR */
	/* public key not used over BR/EDR */
	/* DHKey check not used over BR/EDR */
};
#endif
#if 0
static int smp_br_error(struct bt_smp_br *smp, uint8_t reason)
{
	struct bt_smp_pairing_fail *rsp;
	struct net_buf *buf;

	/* reset context and report */
	smp_br_reset(smp);

	buf = smp_br_create_pdu(smp, BT_SMP_CMD_PAIRING_FAIL, sizeof(*rsp));
	if (!buf) {
		return -ENOBUFS;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->reason = reason;

	/*
	 * SMP timer is not restarted for PairingFailed so don't use
	 * smp_br_send
	 */
	if (bt_l2cap_send(smp->chan.chan.conn, BT_L2CAP_CID_SMP, buf)) {
		net_buf_unref(buf);
	}

	return 0;
}
#endif
#if 0
static int bt_smp_br_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_smp_br *smp = CONTAINER_OF(chan, struct bt_smp_br, chan);
	struct bt_smp_hdr *hdr;
	uint8_t err;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small SMP PDU received");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	LOG_DBG("Received SMP code 0x%02x len %u", hdr->code, buf->len);

	/*
	 * If SMP timeout occurred "no further SMP commands shall be sent over
	 * the L2CAP Security Manager Channel. A new SM procedure shall only be
	 * performed when a new physical link has been established."
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		LOG_WRN("SMP command (code 0x%02x) received after timeout",
			hdr->code);
		return 0;
	}

	if (hdr->code >= ARRAY_SIZE(br_handlers) ||
	    !br_handlers[hdr->code].func) {
		LOG_WRN("Unhandled SMP code 0x%02x", hdr->code);
		smp_br_error(smp, BT_SMP_ERR_CMD_NOTSUPP);
		return 0;
	}

	if (!atomic_test_and_clear_bit(smp->allowed_cmds, hdr->code)) {
		LOG_WRN("Unexpected SMP code 0x%02x", hdr->code);
		smp_br_error(smp, BT_SMP_ERR_UNSPECIFIED);
		return 0;
	}

	if (buf->len != br_handlers[hdr->code].expect_len) {
		LOG_ERR("Invalid len %u for code 0x%02x", buf->len, hdr->code);
		smp_br_error(smp, BT_SMP_ERR_INVALID_PARAMS);
		return 0;
	}

	err = br_handlers[hdr->code].func(smp, buf);
	if (err) {
		smp_br_error(smp, err);
	}

	return 0;
}
#endif
static bool br_sc_supported(void)
{
	if (IS_ENABLED(CONFIG_BT_SMP_FORCE_BREDR)) {
		LOG_WRN("Enabling BR/EDR SMP without BR/EDR SC support");
		return true;
	}

	return BT_FEAT_SC(bt_dev.features);
}

static int bt_smp_br_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_smp_br_connected,
		.disconnected = bt_smp_br_disconnected,
		.recv = NULL,
	};
	int i;

	/* Check BR/EDR SC is supported */
	if (!br_sc_supported()) {
		return -ENOTSUP;
	}

	LOG_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_smp_br_pool); i++) {
		struct bt_smp_br *smp = &bt_smp_br_pool[i];

		if (smp->chan.chan.conn) {
			continue;
		}

		smp->chan.chan.ops = &ops;

		*chan = &smp->chan.chan;
#if 0
		k_work_init_delayable(&smp->work, smp_br_timeout);
#endif
		smp_br_reset(smp);

		return 0;
	}

	LOG_ERR("No available SMP context for conn %p", conn);

	return -ENOMEM;
}

static struct bt_smp_br *smp_br_chan_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SMP);
	if (!chan) {
		LOG_ERR("Unable to find SMP channel");
		return NULL;
	}

	return CONTAINER_OF(chan, struct bt_smp_br, chan);
}

int bt_smp_br_send_pairing_req(struct bt_conn *conn)
{
#if 0
	struct bt_smp_pairing *req;
	struct net_buf *req_buf;
	uint8_t max_key_size;
	struct bt_smp_br *smp;

	smp = smp_br_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	/* SMP Timeout */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		return -EIO;
	}

	/* pairing is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	/* check if we are allowed to start SMP over BR/EDR */
	if (!smp_br_pairing_allowed(smp)) {
		return 0;
	}

	/* Channel not yet connected, will start pairing once connected */
	if (!atomic_test_bit(smp->flags, SMP_FLAG_BR_CONNECTED)) {
		atomic_set_bit(smp->flags, SMP_FLAG_BR_PAIR);
		return 0;
	}

	max_key_size = bt_conn_enc_key_size(conn);
	if (!max_key_size) {
		LOG_DBG("Invalid encryption key size");
		return -EIO;
	}

	smp_br_init(smp);
	smp->enc_key_size = max_key_size;

	req_buf = smp_br_create_pdu(smp, BT_SMP_CMD_PAIRING_REQ, sizeof(*req));
	if (!req_buf) {
		return -ENOBUFS;
	}

	req = (struct bt_smp_pairing *)net_buf_add(req_buf, sizeof(*req));

	/*
	 * If Secure Connections pairing has been initiated over BR/EDR, the IO
	 * Capability, OOB data flag and Auth Req fields of the SM Pairing
	 * Request/Response PDU shall be set to zero on transmission, and
	 * ignored on reception.
	 */

	req->auth_req = 0x00;
	req->io_capability = 0x00;
	req->oob_flag = 0x00;
	req->max_key_size = max_key_size;
	req->init_key_dist = BR_SEND_KEYS_SC;
	req->resp_key_dist = BR_RECV_KEYS_SC;

	smp_br_send(smp, req_buf, NULL);

	smp->local_dist = BR_SEND_KEYS_SC;
	smp->remote_dist = BR_RECV_KEYS_SC;

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RSP);

	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);
#else
    struct bt_smp_br *smp;
    SMP_AUTH_INFO auth;
    SMP_BD_HANDLE bd_handle;
    auth.pair_mode = SMP_LESC_MODE;
    auth.security = conn->required_sec_level;

    smp = smp_br_chan_get(conn);
    if (!smp) {
            return -ENOTCONN;
    }
#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
    UCHAR lkey[BT_LINK_KEY_SIZE];
    UCHAR lkey_type;
     //   if (BT_TRUE == appl_smp_xtxp_enable)

    auth.transport = (UCHAR)1;
    auth.ekey_size = 16U;
    auth.xtx_info =  SMP_XTX_KEYGEN_MASK;
    if (atomic_test_bit(smp->flags, SMP_FLAG_CT2)) {
        auth.xtx_info |= 0x2;
   }

#ifdef SMP_ENABLE_BLURTOOTH_VU_UPDATE
      auth.role = 0;
#else
#endif /* SMP_ENABLE_BLURTOOTH_VU_UPDATE */
       BT_sm_get_device_link_key_and_type(conn->br.dst.val, lkey, &lkey_type);
       sm_get_device_handle ((UCHAR *)conn->br.dst.val, &bd_handle);

#endif
      auth.pair_mode = SMP_LESC_MODE;
      auth.transport = SMP_LINK_BREDR;

      API_RESULT  retval = BT_smp_authenticate
                       (
                           &bd_handle,
                           &auth
                       );
	if (API_SUCCESS != retval)
	{
		return (uint8_t)(retval & 0x00FF);
	}
#endif
	return 0;
}
#endif /* CONFIG_BT_CLASSIC */

static void smp_reset(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	/* Clear flags first in case canceling of timeout fails. The SMP context
	 * shall be marked as timed out in that case.
	 */
	atomic_set(smp->flags, 0);

	/* If canceling fails the timeout handler will set the timeout flag and
	 * mark the it as timed out. No new pairing procedures shall be started
	 * on this connection if that happens.
	 */
#if 0
	(void)k_work_cancel_delayable(&smp->work);
#endif

	smp->method = JUST_WORKS;
	atomic_set(smp->allowed_cmds, 0);

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);
	}
}

static uint8_t hci_err_get(enum bt_security_err err)
{
	switch (err) {
	case BT_SECURITY_ERR_SUCCESS:
		return BT_HCI_ERR_SUCCESS;
	case BT_SECURITY_ERR_AUTH_FAIL:
		return BT_HCI_ERR_AUTH_FAIL;
	case BT_SECURITY_ERR_PIN_OR_KEY_MISSING:
		return BT_HCI_ERR_PIN_OR_KEY_MISSING;
	case BT_SECURITY_ERR_PAIR_NOT_SUPPORTED:
		return BT_HCI_ERR_PAIRING_NOT_SUPPORTED;
	case BT_SECURITY_ERR_PAIR_NOT_ALLOWED:
		return BT_HCI_ERR_PAIRING_NOT_ALLOWED;
	case BT_SECURITY_ERR_INVALID_PARAM:
		return BT_HCI_ERR_INVALID_PARAM;
	default:
		return BT_HCI_ERR_UNSPECIFIED;
	}
}

/* Note: This function not only does set the status but also calls smp_reset
 * at the end which clears any flags previously set.
 */
static void smp_pairing_complete(struct bt_smp *smp, uint8_t status)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	LOG_DBG("got status 0x%x", status);

	if (conn->le.keys == NULL) {
		/* We can get here if the application calls `bt_unpair` in the
		 * `security_changed` callback.
		 */
		LOG_WRN("The in-progress pairing has been deleted!");
		status = BT_SMP_ERR_UNSPECIFIED;
	}

	if (!status) {
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
		/*
		 * Don't derive if Debug Keys are used.
		 * TODO should we allow this if BR/EDR is already connected?
		 */
		if (atomic_test_bit(smp->flags, SMP_FLAG_DERIVE_LK) &&
		    (!atomic_test_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY) ||
		     IS_ENABLED(CONFIG_BT_STORE_DEBUG_KEYS))) {
			sc_derive_link_key(smp);
		}
#endif /* CONFIG_BT_CLASSIC */
		bool bond_flag = atomic_test_bit(smp->flags, SMP_FLAG_BOND);
		struct bt_conn_auth_info_cb *listener, *next;

#if (defined(CONFIG_BT_LOG_SNIFFER_INFO) && ((CONFIG_BT_LOG_SNIFFER_INFO) > 0U))
		if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
			bt_keys_show_sniffer_info(conn->le.keys, NULL);
		}
#endif /* CONFIG_BT_LOG_SNIFFER_INFO */

		if (bond_flag && conn->le.keys) {
			bt_keys_store(conn->le.keys);
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_auth_info_cbs, listener,
						  next, node) {
			if (listener->pairing_complete) {
				listener->pairing_complete(conn, bond_flag);
			}
		}
	} else {
		enum bt_security_err security_err = security_err_get(status);

		/* Clear the key pool entry in case of pairing failure if the
		 * keys already existed before the pairing procedure or the
		 * pairing failed during key distribution.
		 */
		if (conn->le.keys &&
		    (!conn->le.keys->enc_size ||
		     atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR))) {
			/* bt_unpair(smp->chan.chan.conn->id, &smp->chan.chan.conn->le.dst); */
			atomic_set_bit(conn->flags, BT_CONN_UNPAIRING);
			bt_keys_clear(conn->le.keys);
			conn->le.keys = NULL;
		}

		if (!atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR)) {
			bt_conn_security_changed(conn,
						 hci_err_get(security_err),
						 security_err);
		}

		/* Check SMP_FLAG_PAIRING as bt_conn_security_changed may
		 * have called the pairing_failed callback already.
		 */
		if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
			struct bt_conn_auth_info_cb *listener, *next;

			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_auth_info_cbs,
							  listener, next,
							  node) {
				if (listener->pairing_failed) {
					listener->pairing_failed(conn, security_err);
				}
			}
		}
	}

	smp_reset(smp);

	if (conn->sec_level != conn->required_sec_level) {
		bt_smp_start_security(conn);
	}
}
#if 0
static void smp_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_smp *smp = CONTAINER_OF(dwork, struct bt_smp, work);

	LOG_ERR("SMP Timeout");

	smp_pairing_complete(smp, BT_SMP_ERR_UNSPECIFIED);

	/* smp_pairing_complete clears flags so setting timeout flag must come
	 * after it.
	 */
	atomic_set_bit(smp->flags, SMP_FLAG_TIMEOUT);
}
#endif

static void smp_auth_complete(struct bt_smp *smp)
{
	struct bt_conn *conn;
	uint8_t smp_err;
	enum bt_security_err security_err;

	conn = smp->chan.chan.conn;

	conn->encrypt = (API_SUCCESS == smp->status) ? 1u : 0u;

	smp_err = (uint8_t)smp->status;
	security_err = security_err_get(smp_err);

	if (NULL == conn->le.keys)
	{
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
	}

	if (conn->encrypt > 0)
	{
		bt_smp_update_keys(conn);

		if (!update_sec_level(conn)) {
			security_err = BT_SECURITY_ERR_AUTH_FAIL;
		}
	}

	bt_conn_security_changed(conn, hci_err_get(security_err), security_err);

	if (API_SUCCESS == smp->status)
	{
		if (security_err) {
			LOG_ERR("Failed to set required security level");
			/* bt_conn_disconnect(conn, hci_err_get(security_err)); */
		}
	}

}

static void smp_id_add(struct k_work *work)
{
	struct bt_smp *smp = CONTAINER_OF(work, struct bt_smp, id_add);
	struct bt_conn *conn;

	conn = smp->chan.chan.conn;

	bt_id_add(conn->le.keys);
}

#if 0
static void smp_send(struct bt_smp *smp, struct net_buf *buf,
		     bt_conn_tx_cb_t cb, void *user_data)
{
#if 0
	if (bt_l2cap_send_cb(smp->chan.chan.conn, BT_L2CAP_CID_SMP, buf, cb, NULL)) {
		net_buf_unref(buf);
		return;
	}
#else
    net_buf_unref(buf);
#endif
#if 0
	k_work_reschedule(&smp->work, SMP_TIMEOUT);
#endif
}

static int smp_error(struct bt_smp *smp, uint8_t reason)
{
	struct bt_smp_pairing_fail *rsp;
	struct net_buf *buf;
	bool remote_already_completed;

	/* By spec, SMP "pairing process" completes successfully when the last
	 * key to distribute is acknowledged at link-layer.
	 */
	remote_already_completed = (atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR) &&
				    !smp->local_dist && !smp->remote_dist);

	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING) ||
	    atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING) ||
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ)) {
		/* reset context and report */
		smp_pairing_complete(smp, reason);
	}

	if (remote_already_completed) {
		LOG_WRN("SMP does not allow a pairing failure at this point. Known issue. "
			"Disconnecting instead.");
		/* We are probably here because we are, as a peripheral, rejecting a pairing based
		 * on the central's identity address information, but that was the last key to
		 * be transmitted. In that case, the pairing process is already completed.
		 * The SMP protocol states that the pairing process is completed the moment the
		 * peripheral link-layer confirmed the reception of the PDU with the last key.
		 */
		bt_conn_disconnect(smp->chan.chan.conn, BT_HCI_ERR_AUTH_FAIL);
		return 0;
	}
	buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_FAIL, sizeof(*rsp));
	if (!buf) {
		return -ENOBUFS;
	}

	rsp = (struct bt_smp_pairing_fail *)net_buf_add(buf, sizeof(*rsp));
	rsp->reason = reason;

	/* SMP timer is not restarted for PairingFailed so don't use smp_send */
	if (bt_l2cap_send(smp->chan.chan.conn, BT_L2CAP_CID_SMP, buf)) {
		net_buf_unref(buf);
	}

	return 0;
}

static uint8_t smp_send_pairing_random(struct bt_smp *smp)
{
	struct bt_smp_pairing_random *req;
	struct net_buf *rsp_buf;

	rsp_buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_RANDOM, sizeof(*req));
	if (!rsp_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(rsp_buf, sizeof(*req));
	memcpy(req->val, smp->prnd, sizeof(req->val));

	smp_send(smp, rsp_buf, NULL, NULL);

	return 0;
}
#endif

#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
#if 0

static int smp_c1(const uint8_t k[16], const uint8_t r[16],
		  const uint8_t preq[7], const uint8_t pres[7],
		  const bt_addr_le_t *ia, const bt_addr_le_t *ra,
		  uint8_t enc_data[16])
{
	uint8_t p1[16], p2[16];
	int err;

	LOG_DBG("k %s", bt_hex(k, 16));
	LOG_DBG("r %s", bt_hex(r, 16));
	LOG_DBG("ia %s", bt_addr_le_str(ia));
	LOG_DBG("ra %s", bt_addr_le_str(ra));
	LOG_DBG("preq %s", bt_hex(preq, 7));
	LOG_DBG("pres %s", bt_hex(pres, 7));

	/* pres, preq, rat and iat are concatenated to generate p1 */
	p1[0] = ia->type;
	p1[1] = ra->type;
	memcpy(p1 + 2, preq, 7);
	memcpy(p1 + 9, pres, 7);

	LOG_DBG("p1 %s", bt_hex(p1, 16));

	/* c1 = e(k, e(k, r XOR p1) XOR p2) */

	/* Using enc_data as temporary output buffer */
	mem_xor_128(enc_data, r, p1);

	err = bt_encrypt_le(k, enc_data, enc_data);
	if (err) {
		return err;
	}

	/* ra is concatenated with ia and padding to generate p2 */
	memcpy(p2, ra->a.val, 6);
	memcpy(p2 + 6, ia->a.val, 6);
	(void)memset(p2 + 12, 0, 4);

	LOG_DBG("p2 %s", bt_hex(p2, 16));

	mem_xor_128(enc_data, p2, enc_data);

	return bt_encrypt_le(k, enc_data, enc_data);
}
#endif

#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

#if 0
static uint8_t smp_send_pairing_confirm(struct bt_smp *smp)
{
	struct bt_smp_pairing_confirm *req;
	struct net_buf *buf;
	uint8_t r;

	switch (smp->method) {
	case PASSKEY_CONFIRM:
	case JUST_WORKS:
		r = 0U;
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		/*
		 * In the Passkey Entry protocol, the most significant
		 * bit of Z is set equal to one and the least
		 * significant bit is made up from one bit of the
		 * passkey e.g. if the passkey bit is 1, then Z = 0x81
		 * and if the passkey bit is 0, then Z = 0x80.
		 */
		r = (smp->passkey >> smp->passkey_round) & 0x01;
		r |= 0x80;
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_CONFIRM, sizeof(*req));
	if (!buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(buf, sizeof(*req));

	if (bt_crypto_f4(sc_public_key, smp->pkey, smp->prnd, r, req->val)) {
		net_buf_unref(buf);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp_send(smp, buf, NULL, NULL);

	atomic_clear_bit(smp->flags, SMP_FLAG_CFM_DELAYED);

	return 0;
}
#endif
#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
#if 0
static void smp_ident_sent(struct bt_conn *conn, void *user_data)
{
	if (!err) {
		smp_check_complete(conn, BT_SMP_DIST_ENC_KEY);
	}
}

static void legacy_distribute_keys(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys = conn->le.keys;

	if (smp->local_dist & BT_SMP_DIST_ENC_KEY) {
		struct bt_smp_encrypt_info *info;
		struct bt_smp_central_ident *ident;
		struct net_buf *buf;
		/* Use struct to get randomness in single call to bt_rand */
		struct {
			uint8_t key[16];
			uint8_t rand[8];
			uint8_t ediv[2];
		} rand;

		if (bt_rand((void *)&rand, sizeof(rand))) {
			LOG_ERR("Unable to get random bytes");
			return;
		}

		buf = smp_create_pdu(smp, BT_SMP_CMD_ENCRYPT_INFO,
				     sizeof(*info));
		if (!buf) {
			LOG_ERR("Unable to allocate Encrypt Info buffer");
			return;
		}

		info = (struct bt_smp_encrypt_info *)net_buf_add(buf, sizeof(*info));

		/* distributed only enc_size bytes of key */
		memcpy(info->ltk, rand.key, keys->enc_size);
		if (keys->enc_size < sizeof(info->ltk)) {
			(void)memset(info->ltk + keys->enc_size, 0,
				     sizeof(info->ltk) - keys->enc_size);
		}

		smp_send(smp, buf, NULL, NULL);

		buf = smp_create_pdu(smp, BT_SMP_CMD_CENTRAL_IDENT,
				     sizeof(*ident));
		if (!buf) {
			LOG_ERR("Unable to allocate Central Ident buffer");
			return;
		}

		ident = (struct bt_smp_central_ident *)net_buf_add(buf, sizeof(*ident));
		memcpy(ident->rand, rand.rand, sizeof(ident->rand));
		memcpy(ident->ediv, rand.ediv, sizeof(ident->ediv));

		smp_send(smp, buf, smp_ident_sent, NULL);

		if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
			bt_keys_add_type(keys, BT_KEYS_PERIPH_LTK);

			memcpy(keys->periph_ltk.val, rand.key,
			       sizeof(keys->periph_ltk.val));
			memcpy(keys->periph_ltk.rand, rand.rand,
			       sizeof(keys->periph_ltk.rand));
			memcpy(keys->periph_ltk.ediv, rand.ediv,
			       sizeof(keys->periph_ltk.ediv));
		}
	}
}
#endif
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

static uint8_t bt_smp_distribute_keys(struct bt_smp *smp)
{
#if 0
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_keys *keys = conn->le.keys;

	if (!keys) {
		LOG_ERR("No keys space for %s", bt_addr_le_str(&conn->le.dst));
		return BT_SMP_ERR_UNSPECIFIED;
	}

#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
	/* Distribute legacy pairing specific keys */
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		legacy_distribute_keys(smp);
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

#if (defined(CONFIG_BT_PRIVACY) && ((CONFIG_BT_PRIVACY) > 0U))
	if (smp->local_dist & BT_SMP_DIST_ID_KEY) {
		struct bt_smp_ident_info *id_info;
		struct bt_smp_ident_addr_info *id_addr_info;
		struct net_buf *buf;

		buf = smp_create_pdu(smp, BT_SMP_CMD_IDENT_INFO,
				     sizeof(*id_info));
		if (!buf) {
			LOG_ERR("Unable to allocate Ident Info buffer");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		id_info = (struct bt_smp_ident_info *)net_buf_add(buf, sizeof(*id_info));
		memcpy(id_info->irk, bt_dev.irk[conn->id], 16);

		smp_send(smp, buf, NULL, NULL);

		buf = smp_create_pdu(smp, BT_SMP_CMD_IDENT_ADDR_INFO,
				     sizeof(*id_addr_info));
		if (!buf) {
			LOG_ERR("Unable to allocate Ident Addr Info buffer");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		id_addr_info = (struct bt_smp_ident_addr_info *)net_buf_add(buf, sizeof(*id_addr_info));
		bt_addr_le_copy(&id_addr_info->addr, &bt_dev.id_addr[conn->id]);

		smp_send(smp, buf, smp_id_sent, NULL);
	}
#endif /* CONFIG_BT_PRIVACY */

#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
	if (smp->local_dist & BT_SMP_DIST_SIGN) {
		struct bt_smp_signing_info *info;
		struct net_buf *buf;

		buf = smp_create_pdu(smp, BT_SMP_CMD_SIGNING_INFO,
				     sizeof(*info));
		if (!buf) {
			LOG_ERR("Unable to allocate Signing Info buffer");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		info = (struct bt_smp_signing_info *)net_buf_add(buf, sizeof(*info));

		if (bt_rand(info->csrk, sizeof(info->csrk))) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
			bt_keys_add_type(keys, BT_KEYS_LOCAL_CSRK);
			memcpy(keys->local_csrk.val, info->csrk, 16);
			keys->local_csrk.cnt = 0U;
		}

		smp_send(smp, buf, smp_sign_info_sent, NULL);
	}
#endif /* CONFIG_BT_SIGNING */
#endif
	return 0;
}

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
static uint8_t send_pairing_rsp(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
    API_RESULT retval;

	retval = BT_smp_authentication_request_reply
			(
				&conn->deviceId,
				&smp->auth
			);
	if (API_SUCCESS != retval)
	{
		return (uint8_t)(retval & 0x00FF);
	}
	else
	{
		return 0;
	}
}

static uint8_t smp_pairing_accept_query(struct bt_smp *smp, struct bt_smp_pairing *pairing)
{
#if (defined(CONFIG_BT_SMP_APP_PAIRING_ACCEPT) && ((CONFIG_BT_SMP_APP_PAIRING_ACCEPT) > 0U))
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_conn *conn = smp->chan.chan.conn;

	if (smp_auth_cb && smp_auth_cb->pairing_accept) {
		const struct bt_conn_pairing_feat feat = {
			.io_capability = pairing->io_capability,
			.oob_data_flag = pairing->oob_flag,
			.auth_req = pairing->auth_req,
			.max_enc_key_size = pairing->max_key_size,
			.init_key_dist = pairing->init_key_dist,
			.resp_key_dist = pairing->resp_key_dist
		};

		return smp_err_get(smp_auth_cb->pairing_accept(conn, &feat));
	}
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */
	return 0;
}
#endif /* CONFIG_BT_PERIPHERAL */

#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
#if 0
static int smp_s1(const uint8_t k[16], const uint8_t r1[16],
		  const uint8_t r2[16], uint8_t out[16])
{
	/* The most significant 64-bits of r1 are discarded to generate
	 * r1' and the most significant 64-bits of r2 are discarded to
	 * generate r2'.
	 * r1' is concatenated with r2' to generate r' which is used as
	 * the 128-bit input parameter plaintextData to security function e:
	 *
	 *    r' = r1' || r2'
	 */
	memcpy(out, r2, 8);
	memcpy(out + 8, r1, 8);

	/* s1(k, r1 , r2) = e(k, r') */
	return bt_encrypt_le(k, out, out);
}
#endif

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
static uint8_t legacy_get_pair_method(struct bt_smp *smp, uint8_t remote_io)
{
	struct bt_smp_pairing *req, *rsp;
	uint8_t method;

	if (remote_io > BT_SMP_IO_KEYBOARD_DISPLAY) {
		return JUST_WORKS;
	}

	req = (struct bt_smp_pairing *)&smp->preq[1];
	rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	/* if both sides have OOB data use OOB */
	if ((req->oob_flag & rsp->oob_flag) & BT_SMP_OOB_DATA_MASK) {
		return LEGACY_OOB;
	}

	/* if none side requires MITM use JustWorks */
	if (!((req->auth_req | rsp->auth_req) & BT_SMP_AUTH_MITM)) {
		return JUST_WORKS;
	}

	method = gen_method_legacy[remote_io][get_io_capa(smp)];

	/* if both sides have KeyboardDisplay capabilities, initiator displays
	 * and responder inputs
	 */
	if (method == PASSKEY_ROLE) {
		if (smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
			method = PASSKEY_DISPLAY;
		} else {
			method = PASSKEY_INPUT;
		}
	}

	return method;
}
#endif /* CONFIG_BT_PERIPHERAL */

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
static uint8_t legacy_request_tk(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_keys *keys;
	uint32_t passkey;

	/*
	 * Fail if we have keys that are stronger than keys that will be
	 * distributed in new pairing. This is to avoid replacing authenticated
	 * keys with unauthenticated ones.
	  */
	keys = bt_keys_find_addr(conn->id, &conn->le.dst);
	if (keys && (keys->flags & BT_KEYS_AUTHENTICATED) &&
	    smp->method == JUST_WORKS) {
		LOG_ERR("JustWorks failed, authenticated keys present");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	switch (smp->method) {
	case LEGACY_OOB:
		if (smp_auth_cb && smp_auth_cb->oob_data_request) {
			struct bt_conn_oob_info info = {
				.type = BT_CONN_OOB_LE_LEGACY,
			};

			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			smp_auth_cb->oob_data_request(smp->chan.chan.conn, &info);
		} else {
			return BT_SMP_ERR_OOB_NOT_AVAIL;
		}

		break;
	case PASSKEY_DISPLAY:
		if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
		    fixed_passkey != BT_PASSKEY_INVALID) {
			passkey = fixed_passkey;
		} else  {
			if (bt_rand(&passkey, sizeof(passkey))) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			passkey %= 1000000;
		}

		if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
			LOG_INF("Legacy passkey %u", passkey);
		}

		if (smp_auth_cb && smp_auth_cb->passkey_display) {
			atomic_set_bit(smp->flags, SMP_FLAG_DISPLAY);
			smp_auth_cb->passkey_display(conn, passkey);
		}

		sys_put_le32(passkey, smp->tk);

		break;
	case PASSKEY_INPUT:
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->passkey_entry(conn);
		break;
	case JUST_WORKS:
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	return 0;
}
#endif /* CONFIG_BT_PERIPHERAL */
#if 0
static uint8_t legacy_send_pairing_confirm(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_pairing_confirm *req;
	struct net_buf *buf;

	buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_CONFIRM, sizeof(*req));
	if (!buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = (struct bt_smp_pairing_confirm *)net_buf_add(buf, sizeof(*req));

	if (smp_c1(smp->tk, smp->prnd, smp->preq, smp->prsp,
		   &conn->le.init_addr, &conn->le.resp_addr, req->val)) {
		net_buf_unref(buf);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	smp_send(smp, buf, NULL, NULL);

	atomic_clear_bit(smp->flags, SMP_FLAG_CFM_DELAYED);

	return 0;
}
#endif

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
static uint8_t legacy_pairing_req(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	uint8_t ret;

	LOG_DBG("");

	ret = legacy_request_tk(smp);
	if (ret) {
		return ret;
	}

	/* ask for consent if pairing is not due to sending SecReq*/
	if ((DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    smp_auth_cb && smp_auth_cb->pairing_confirm) {
		smp->confirm_type = CONFIRM_TYPE_PAIRING;
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->pairing_confirm(smp->chan.chan.conn);
		return 0;
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
	atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
	return send_pairing_rsp(smp);
}
#endif /* CONFIG_BT_PERIPHERAL */

#if 0
static uint8_t legacy_pairing_random(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t tmp[16];
	int err;

	LOG_DBG("");

	/* calculate confirmation */
	err = smp_c1(smp->tk, smp->rrnd, smp->preq, smp->prsp,
		     &conn->le.init_addr, &conn->le.resp_addr, tmp);
	if (err) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	LOG_DBG("pcnf %s", bt_hex(smp->pcnf, 16));
	LOG_DBG("cfm %s", bt_hex(tmp, 16));

	if (memcmp(smp->pcnf, tmp, sizeof(smp->pcnf))) {
		return BT_SMP_ERR_CONFIRM_FAILED;
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL) {
		uint8_t ediv[2], rand[8];

		/* No need to store central STK */
		err = smp_s1(smp->tk, smp->rrnd, smp->prnd, tmp);
		if (err) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* Rand and EDiv are 0 for the STK */
		(void)memset(ediv, 0, sizeof(ediv));
		(void)memset(rand, 0, sizeof(rand));
		if (bt_conn_le_start_encryption(conn, rand, ediv, tmp,
						get_encryption_key_size(smp))) {
			LOG_ERR("Failed to start encryption");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

		if (IS_ENABLED(CONFIG_BT_SMP_USB_HCI_CTLR_WORKAROUND)) {
			if (smp->remote_dist & BT_SMP_DIST_ENC_KEY) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_ENCRYPT_INFO);
			} else if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_IDENT_INFO);
			} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_SIGNING_INFO);
			}
		}

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		err = smp_s1(smp->tk, smp->prnd, smp->rrnd, tmp);
		if (err) {
			LOG_ERR("Calculate STK failed");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(smp->tk, tmp, sizeof(smp->tk));
		LOG_DBG("generated STK %s", bt_hex(smp->tk, 16));

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

		return smp_send_pairing_random(smp);
	}

	return 0;
}

static uint8_t legacy_pairing_confirm(struct bt_smp *smp)
{
	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		return legacy_send_pairing_confirm(smp);
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		if (!atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_RANDOM);
			return legacy_send_pairing_confirm(smp);
		}

		atomic_set_bit(smp->flags, SMP_FLAG_CFM_DELAYED);
	}

	return 0;
}

static void legacy_user_tk_entry(struct bt_smp *smp)
{
	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_CFM_DELAYED)) {
		return;
	}

	/* if confirm failed ie. due to invalid passkey, cancel pairing */
	if (legacy_pairing_confirm(smp)) {
		smp_error(smp, BT_SMP_ERR_PASSKEY_ENTRY_FAILED);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
	}
}

static void legacy_passkey_entry(struct bt_smp *smp, unsigned int passkey)
{
	passkey = sys_cpu_to_le32(passkey);
	memcpy(smp->tk, &passkey, sizeof(passkey));

	legacy_user_tk_entry(smp);
}

static uint8_t smp_encrypt_info(struct bt_smp *smp, struct net_buf *buf)
{
	LOG_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_encrypt_info *req = (void *)buf->data;
		struct bt_conn *conn = smp->chan.chan.conn;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_LTK, conn->id, &conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->ltk.val, req->ltk, 16);
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_CENTRAL_IDENT);

	return 0;
}

static uint8_t smp_central_ident(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t err;

	LOG_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_central_ident *req = (void *)buf->data;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_LTK, conn->id, &conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->ltk.ediv, req->ediv, sizeof(keys->ltk.ediv));
		memcpy(keys->ltk.rand, req->rand, sizeof(req->rand));
	}

	smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;

	if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL && !smp->remote_dist) {
		err = bt_smp_distribute_keys(smp);
		if (err) {
			return err;
		}
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_complete(smp, 0);
	}

	return 0;
}
#endif

#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0U))
#if 0
static uint8_t legacy_pairing_rsp(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	uint8_t ret;

	LOG_DBG("");

	ret = legacy_request_tk(smp);
	if (ret) {
		return ret;
	}

	/* ask for consent if this is due to received SecReq */
	if ((DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    smp_auth_cb && smp_auth_cb->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->pairing_confirm(smp->chan.chan.conn);
		return 0;
	}

	if (!atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
		return legacy_send_pairing_confirm(smp);
	}

	atomic_set_bit(smp->flags, SMP_FLAG_CFM_DELAYED);

	return 0;
}
#endif
#endif /* CONFIG_BT_CENTRAL */
#else
#if 0
static uint8_t smp_encrypt_info(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}

static uint8_t smp_central_ident(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

static int smp_init(struct bt_smp *smp)
{
	/* Initialize SMP context without clearing L2CAP channel context */
	(void)memset(((uint8_t *)(void *)smp) + offsetof(struct bt_smp, allowed_cmds), 0,
		     sizeof(*smp) - offsetof(struct bt_smp, allowed_cmds));

	/* Generate local random number */
	if (bt_rand(smp->prnd, 16)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	LOG_DBG("prnd %s", bt_hex(smp->prnd, 16));

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_FAIL);

#if !(defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) && (CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY > 0))
	sc_public_key = bt_pub_key_get();
#endif

	return 0;
}

void bt_set_bondable(bool enable)
{
	bondable = enable;
#if defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U)
    BT_sm_set_pairable((true == bondable)? SM_PAIRABLE_AND_BONDABLE : SM_PAIRABLE_AND_NON_BONDABLE);
#endif /* defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U) */
}

void bt_le_oob_set_sc_flag(bool enable)
{
	sc_oobd_present = enable;
}

void bt_le_oob_set_legacy_flag(bool enable)
{
	legacy_oobd_present = enable;
}

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
static uint8_t get_auth(struct bt_smp *smp, uint8_t auth)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	if (sc_supported) {
		auth &= BT_SMP_AUTH_MASK_SC;
	} else {
		auth &= BT_SMP_AUTH_MASK;
	}

	if ((get_io_capa(smp) == BT_SMP_IO_NO_INPUT_OUTPUT) ||
	    (!IS_ENABLED(CONFIG_BT_SMP_ENFORCE_MITM) &&
	    (conn->required_sec_level < BT_SECURITY_L3))) {
		auth &= ~(BT_SMP_AUTH_MITM);
	} else {
		auth |= BT_SMP_AUTH_MITM;
	}

	if (bondable) {
		auth |= BT_SMP_AUTH_BONDING;
	} else {
		auth &= ~BT_SMP_AUTH_BONDING;
	}

	if (IS_ENABLED(CONFIG_BT_PASSKEY_KEYPRESS)) {
		auth |= BT_SMP_AUTH_KEYPRESS;
	} else {
		auth &= ~BT_SMP_AUTH_KEYPRESS;
	}

	return auth;
}
#endif /* CONFIG_BT_PERIPHERAL */

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
static uint8_t remote_sec_level_reachable(struct bt_smp *smp)
{
	bt_security_t sec = smp->chan.chan.conn->required_sec_level;

	if (IS_ENABLED(CONFIG_BT_SMP_SC_ONLY)) {
		sec = BT_SECURITY_L4;
	}
	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		sec = BT_SECURITY_L3;
	}

	switch (sec) {
	case BT_SECURITY_L1:
	case BT_SECURITY_L2:
		return 0;

	case BT_SECURITY_L4:
		if (get_encryption_key_size(smp) != BT_SMP_MAX_ENC_KEY_SIZE) {
			return BT_SMP_ERR_ENC_KEY_SIZE;
		}

		if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
			return BT_SMP_ERR_AUTH_REQUIREMENTS;
		}
		__fallthrough;
	case BT_SECURITY_L3:
		if (smp->method == JUST_WORKS) {
			return BT_SMP_ERR_AUTH_REQUIREMENTS;
		}

		return 0;
	default:
		return BT_SMP_ERR_UNSPECIFIED;
	}
}
#endif /* CONFIG_BT_PERIPHERAL */

static bool sec_level_reachable(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);

	switch (smp->chan.chan.conn->required_sec_level) {
	case BT_SECURITY_L1:
	case BT_SECURITY_L2:
		return true;
	case BT_SECURITY_L3:
		return get_io_capa(smp) != BT_SMP_IO_NO_INPUT_OUTPUT ||
		       (smp_auth_cb && smp_auth_cb->oob_data_request);
	case BT_SECURITY_L4:
		return (get_io_capa(smp) != BT_SMP_IO_NO_INPUT_OUTPUT ||
		       (smp_auth_cb && smp_auth_cb->oob_data_request)) && sc_supported;
	default:
		return false;
	}
}

static struct bt_smp *smp_chan_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_SMP);
	if (!chan) {
		LOG_ERR("Unable to find SMP channel");
		return NULL;
	}

	return CONTAINER_OF(chan, struct bt_smp, chan);
}

bool bt_smp_request_ltk(struct bt_conn *conn, uint64_t rand, uint16_t ediv, uint8_t *ltk)
{
	struct bt_smp *smp;
	uint8_t enc_size;

	smp = smp_chan_get(conn);
	if (!smp) {
		return false;
	}

	/*
	 * Both legacy STK and LE SC LTK have rand and ediv equal to zero.
	 * If pairing is in progress use the TK for encryption.
	 */
	if (ediv == 0U && rand == 0U &&
	    atomic_test_bit(smp->flags, SMP_FLAG_PAIRING) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		enc_size = get_encryption_key_size(smp);

		/*
		 * We keep both legacy STK and LE SC LTK in TK.
		 * Also use only enc_size bytes of key for encryption.
		 */
		memcpy(ltk, smp->tk, enc_size);
		if (enc_size < BT_SMP_MAX_ENC_KEY_SIZE) {
			(void)memset(ltk + enc_size, 0,
				     BT_SMP_MAX_ENC_KEY_SIZE - enc_size);
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		return true;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_find(BT_KEYS_LTK_P256, conn->id,
					     &conn->le.dst);
		if (!conn->le.keys) {
			conn->le.keys = bt_keys_find(BT_KEYS_PERIPH_LTK,
						     conn->id, &conn->le.dst);
		}
	}

	if (ediv == 0U && rand == 0U &&
	    conn->le.keys && (conn->le.keys->keys & BT_KEYS_LTK_P256)) {
		enc_size = conn->le.keys->enc_size;

		memcpy(ltk, conn->le.keys->ltk.val, enc_size);
		if (enc_size < BT_SMP_MAX_ENC_KEY_SIZE) {
			(void)memset(ltk + enc_size, 0,
				     BT_SMP_MAX_ENC_KEY_SIZE - enc_size);
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		return true;
	}

#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
	if (conn->le.keys && (conn->le.keys->keys & BT_KEYS_PERIPH_LTK) &&
	    !memcmp(conn->le.keys->periph_ltk.rand, &rand, 8) &&
	    !memcmp(conn->le.keys->periph_ltk.ediv, &ediv, 2)) {
		enc_size = conn->le.keys->enc_size;

		memcpy(ltk, conn->le.keys->periph_ltk.val, enc_size);
		if (enc_size < BT_SMP_MAX_ENC_KEY_SIZE) {
			(void)memset(ltk + enc_size, 0,
				     BT_SMP_MAX_ENC_KEY_SIZE - enc_size);
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		return true;
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

	if (atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ)) {
		/* Notify higher level that security failed if security was
		 * initiated by peripheral.
		 */
		bt_conn_security_changed(conn, BT_HCI_ERR_PIN_OR_KEY_MISSING,
					 BT_SECURITY_ERR_PIN_OR_KEY_MISSING);
	}

	smp_reset(smp);
	return false;
}

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
#if 0
static int smp_send_security_req(struct bt_conn *conn)
{
	struct bt_smp *smp;
	struct bt_smp_security_request *req;
	struct net_buf *req_buf;
	int err;

	LOG_DBG("");
	smp = smp_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	/* SMP Timeout */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		return -EIO;
	}

	/* pairing is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		return -EBUSY;
	}

	/* early verify if required sec level if reachable */
	if (!(sec_level_reachable(smp) || smp_keys_check(conn))) {
		return -EINVAL;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
		if (!conn->le.keys) {
			return -ENOMEM;
		}
	}

	if (smp_init(smp) != 0) {
		return -ENOBUFS;
	}

	req_buf = smp_create_pdu(smp, BT_SMP_CMD_SECURITY_REQUEST,
				 sizeof(*req));
	if (!req_buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(req_buf, sizeof(*req));
	req->auth_req = get_auth(smp, BT_SMP_AUTH_DEFAULT);

	/* SMP timer is not restarted for SecRequest so don't use smp_send */
	err = bt_l2cap_send(conn, BT_L2CAP_CID_SMP, req_buf);
	if (err) {
		net_buf_unref(req_buf);
		return err;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_SEC_REQ);
	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);

	return 0;
}
#endif
static uint8_t smp_pairing_req(struct bt_smp *smp, struct bt_smp_pairing *req, SMP_AUTH_INFO *auth)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);

	struct bt_smp_pairing *rsp;
	uint8_t err;
	uint8_t keyDistribution = 0;
	API_RESULT retVal;

	LOG_DBG("");

	auth->param = SMP_ERROR_NONE;

	if ((req->max_key_size > BT_SMP_MAX_ENC_KEY_SIZE) ||
	    (req->max_key_size < BT_SMP_MIN_ENC_KEY_SIZE)) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
		if (!conn->le.keys) {
			LOG_DBG("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}
	}

	/* If we already sent a security request then the SMP context
	 * is already initialized.
	 */
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ)) {
		int ret = smp_init(smp);

		if (ret) {
			return ret;
		}
	}

	/* Store req for later use */
	smp->preq[0] = BT_SMP_CMD_PAIRING_REQ;
	memcpy(smp->preq + 1, req, sizeof(*req));

	/* create rsp, it will be used later on */
	smp->prsp[0] = BT_SMP_CMD_PAIRING_RSP;
	rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	rsp->auth_req = get_auth(smp, req->auth_req);
	/* according to get_auth function, add follow codes to set MITM.
	 * In Ethermind, when security == SMP_SEC_LEVEL_2, MITM is set.
	*/
	if ((get_io_capa(smp) == BT_SMP_IO_NO_INPUT_OUTPUT) ||
	    (!IS_ENABLED(CONFIG_BT_SMP_ENFORCE_MITM) &&
	    (conn->required_sec_level < BT_SECURITY_L3))) {
	} else {
		auth->security = SMP_SEC_LEVEL_2;
	}
	rsp->io_capability = get_io_capa(smp);

	rsp->max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	rsp->init_key_dist = (req->init_key_dist & RECV_KEYS);
	rsp->resp_key_dist = (req->resp_key_dist & SEND_KEYS);

	if ((rsp->auth_req & BT_SMP_AUTH_SC) &&
	    (req->auth_req & BT_SMP_AUTH_SC)) {
		atomic_set_bit(smp->flags, SMP_FLAG_SC);

		rsp->init_key_dist &= RECV_KEYS_SC;
		rsp->resp_key_dist &= SEND_KEYS_SC;

		auth->pair_mode = SMP_LESC_MODE;
	}
	else
	{
		auth->pair_mode = SMP_LEGACY_MODE;
	}

#if 0
    if (atomic_test_bit(smp->flags, SMP_FLAG_SC))
    {
        rsp->oob_flag = sc_oobd_present ? BT_SMP_OOB_PRESENT :
                BT_SMP_OOB_NOT_PRESENT;
    }
    else
    {
        rsp->oob_flag = legacy_oobd_present ? BT_SMP_OOB_PRESENT :
                BT_SMP_OOB_NOT_PRESENT;
    }
#else
    rsp->oob_flag = (sc_oobd_present || legacy_oobd_present) ? BT_SMP_OOB_PRESENT : BT_SMP_OOB_NOT_PRESENT;
#endif

#if 1

    if(sc_oobd_present || legacy_oobd_present)
    {
        if (bt_auth && bt_auth->oob_data_request)
        {
            struct bt_conn_oob_info info =
            {
                .type = BT_CONN_OOB_LE_SC,
                .lesc.oob_config = BT_CONN_OOB_BOTH_PEERS,
            };

#if 0
            le_sc_oob_config_set(smp, &info);
#endif

            smp->oobd_local = NULL;
            smp->oobd_remote = NULL;

            atomic_set_bit(smp->flags, SMP_FLAG_OOB_PENDING);
            bt_auth->oob_data_request(smp->chan.chan.conn, &info);
        }
    }
#endif

	/* for Local */
	keyDistribution = rsp->resp_key_dist;
	/* for Remote */
	keyDistribution |= (rsp->init_key_dist) << 4;
	retVal = BT_smp_set_key_distribution_flag_pl(keyDistribution);
	if (API_SUCCESS != retVal)
	{
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if ((rsp->auth_req & BT_SMP_AUTH_CT2) &&
	    (req->auth_req & BT_SMP_AUTH_CT2)) {
		atomic_set_bit(smp->flags, SMP_FLAG_CT2);
	}
#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
	/* Enable H7 support if Cross transport keygen is requested */
	if (auth->xtx_info & SMP_XTX_KEYGEN_MASK)
	{
		auth->xtx_info |= SMP_XTX_H7_MASK;
	}
#endif

	if ((rsp->auth_req & BT_SMP_AUTH_BONDING) &&
	    (req->auth_req & BT_SMP_AUTH_BONDING)) {
		atomic_set_bit(smp->flags, SMP_FLAG_BOND);
	} else if (IS_ENABLED(CONFIG_BT_BONDING_REQUIRED)) {
		/* Reject pairing req if not both intend to bond */
		LOG_DBG("Bonding required");
		return BT_SMP_ERR_UNSPECIFIED;
	} else {
		rsp->init_key_dist = 0;
		rsp->resp_key_dist = 0;
	}

	smp->local_dist = rsp->resp_key_dist;
	smp->remote_dist = rsp->init_key_dist;
	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);
	atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

	smp->method = get_pair_method(smp, req->io_capability);

#if 0
	if (!update_keys_check(smp, conn->le.keys)) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}
#endif

	err = remote_sec_level_reachable(smp);
	if (err) {
		return err;
	}

    if (!atomic_test_bit(smp->flags, SMP_FLAG_BOND))
    {
        auth->bonding = SMP_BONDING_NONE;
    }

	memcpy(&smp->auth, auth, sizeof(smp->auth));

	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
#if (defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && ((CONFIG_BT_SMP_SC_PAIR_ONLY) > 0U))
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
#else
		if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
			err = smp_pairing_accept_query(smp, req);
			if (err) {
				return err;
			}
		}

		return legacy_pairing_req(smp);
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */
	}

	if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
		err = smp_pairing_accept_query(smp, req);
		if (err) {
			return err;
		}
	}

	if (!IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) &&
	    (DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    !atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    smp_auth_cb && smp_auth_cb->pairing_confirm) {
		smp->confirm_type = CONFIRM_TYPE_PAIRING;
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->pairing_confirm(conn);
		return 0;
	}

	return send_pairing_rsp(smp);
}
#else
static uint8_t smp_pairing_req(struct bt_smp *smp, struct bt_smp_pairing *req, SMP_AUTH_INFO *auth)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BT_PERIPHERAL */
#if 0
static uint8_t sc_send_public_key(struct bt_smp *smp)
{
	struct bt_smp_public_key *req;
	struct net_buf *req_buf;

	req_buf = smp_create_pdu(smp, BT_SMP_CMD_PUBLIC_KEY, sizeof(*req));
	if (!req_buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(req_buf, sizeof(*req));

	memcpy(req->x, sc_public_key, sizeof(req->x));
	memcpy(req->y, &sc_public_key[32], sizeof(req->y));

	smp_send(smp, req_buf, NULL, NULL);

	if (IS_ENABLED(CONFIG_BT_USE_DEBUG_KEYS)) {
		atomic_set_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY);
	}

	return 0;
}
#endif
#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0U))
static int smp_send_pairing_req(struct bt_conn *conn)
{
	struct bt_smp *smp;
        API_RESULT retval;

	LOG_DBG("");

	smp = smp_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	/* SMP Timeout */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		return -EIO;
	}

	/* A higher security level is requested during the key distribution
	 * phase, once pairing is complete a new pairing procedure will start.
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR)) {
		return 0;
	}

	/* pairing is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	/* Encryption is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		return -EBUSY;
	}

	/* early verify if required sec level if reachable */
	if (!sec_level_reachable(smp)) {
		return -EINVAL;
	}

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
		if (!conn->le.keys) {
			return -ENOMEM;
		}
	}

	if (smp_init(smp)) {
		return -ENOBUFS;
	}

#if 1
	if(legacy_oobd_present || sc_oobd_present)
	{
		if (bt_auth && bt_auth->oob_data_request) {
			struct bt_conn_oob_info info = {
				.type = BT_CONN_OOB_LE_SC,
				.lesc.oob_config = BT_CONN_OOB_BOTH_PEERS,
			};

			smp->oobd_local = NULL;
			smp->oobd_remote = NULL;

			atomic_set_bit(smp->flags, SMP_FLAG_OOB_PENDING);
			bt_auth->oob_data_request(smp->chan.chan.conn, &info);
		}
	}
#endif

#if 0
	req_buf = smp_create_pdu(smp, BT_SMP_CMD_PAIRING_REQ, sizeof(*req));
	if (!req_buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(req_buf, sizeof(*req));

	req->auth_req = get_auth(smp, BT_SMP_AUTH_DEFAULT);
	req->io_capability = get_io_capa(smp);
	/* At this point is it unknown if pairing will be legacy or LE SC so
	 * set OOB flag if any OOB data is present and assume to peer device
	 * provides OOB data that will match it's pairing type.
	 */
	req->oob_flag = (legacy_oobd_present || sc_oobd_present) ?
				BT_SMP_OOB_PRESENT : BT_SMP_OOB_NOT_PRESENT;
	req->max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	if (req->auth_req & BT_SMP_AUTH_BONDING) {
		req->init_key_dist = SEND_KEYS;
		req->resp_key_dist = RECV_KEYS;
	} else {
		req->init_key_dist = 0;
		req->resp_key_dist = 0;
	}

	smp->local_dist = req->init_key_dist;
	smp->remote_dist = req->resp_key_dist;

	/* Store req for later use */
	smp->preq[0] = BT_SMP_CMD_PAIRING_REQ;
	memcpy(smp->preq + 1, req, sizeof(*req));

	smp_send(smp, req_buf, NULL, NULL);

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RSP);
	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
#else
	retval = BT_smp_authentication_request_reply
			(
				&conn->deviceId,
				&smp->auth
			);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
#endif
	atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
	atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);
	return 0;
}
#if 0
static uint8_t smp_pairing_rsp(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_smp_pairing *rsp = (void *)buf->data;
	struct bt_smp_pairing *req = (struct bt_smp_pairing *)&smp->preq[1];
	uint8_t err;

	LOG_DBG("");

	if ((rsp->max_key_size > BT_SMP_MAX_ENC_KEY_SIZE) ||
	    (rsp->max_key_size < BT_SMP_MIN_ENC_KEY_SIZE)) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	smp->local_dist &= rsp->init_key_dist;
	smp->remote_dist &= rsp->resp_key_dist;

	/* Store rsp for later use */
	smp->prsp[0] = BT_SMP_CMD_PAIRING_RSP;
	memcpy(smp->prsp + 1, rsp, sizeof(*rsp));

	if ((rsp->auth_req & BT_SMP_AUTH_SC) &&
	    (req->auth_req & BT_SMP_AUTH_SC)) {
		atomic_set_bit(smp->flags, SMP_FLAG_SC);
	}

	if ((rsp->auth_req & BT_SMP_AUTH_CT2) &&
	    (req->auth_req & BT_SMP_AUTH_CT2)) {
		atomic_set_bit(smp->flags, SMP_FLAG_CT2);
	}

	if ((rsp->auth_req & BT_SMP_AUTH_BONDING) &&
	    (req->auth_req & BT_SMP_AUTH_BONDING)) {
		atomic_set_bit(smp->flags, SMP_FLAG_BOND);
	} else if (IS_ENABLED(CONFIG_BT_BONDING_REQUIRED)) {
		/* Reject pairing req if not both intend to bond */
		LOG_DBG("Bonding required");
		return BT_SMP_ERR_UNSPECIFIED;
	} else {
		smp->local_dist = 0;
		smp->remote_dist = 0;
	}

	smp->method = get_pair_method(smp, rsp->io_capability);

	if (!update_keys_check(smp, conn->le.keys)) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	err = remote_sec_level_reachable(smp);
	if (err) {
		return err;
	}

	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
#if (defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && ((CONFIG_BT_SMP_SC_PAIR_ONLY) > 0U))
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
#else
		if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
			err = smp_pairing_accept_query(smp, rsp);
			if (err) {
				return err;
			}
		}

		return legacy_pairing_rsp(smp);
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */
	}

	smp->local_dist &= SEND_KEYS_SC;
	smp->remote_dist &= RECV_KEYS_SC;

	if (IS_ENABLED(CONFIG_BT_SMP_APP_PAIRING_ACCEPT)) {
		err = smp_pairing_accept_query(conn, rsp);
		if (err) {
			return err;
		}
	}

	if (!IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) &&
	    (DISPLAY_FIXED(smp) || smp->method == JUST_WORKS) &&
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ) &&
	    smp_auth_cb && smp_auth_cb->pairing_confirm) {
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->pairing_confirm(conn);
		return 0;
	}

	if (!sc_public_key) {
		atomic_set_bit(smp->flags, SMP_FLAG_PKEY_SEND);
		return 0;
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
	atomic_clear_bit(smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);

	return sc_send_public_key(smp);
}
#endif
#else
#if 0
static uint8_t smp_pairing_rsp(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif
#endif /* CONFIG_BT_CENTRAL */

#if 0
static uint8_t smp_pairing_confirm(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_smp_pairing_confirm *req = (void *)buf->data;

	LOG_DBG("");

	atomic_clear_bit(smp->flags, SMP_FLAG_DISPLAY);

	memcpy(smp->pcnf, req->val, sizeof(smp->pcnf));

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_random(smp);
	}

	if (!IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		return 0;
	}

#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return legacy_pairing_confirm(smp);
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

	switch (smp->method) {
	case PASSKEY_DISPLAY:
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_confirm(smp);
	case PASSKEY_INPUT:
		if (atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
			atomic_set_bit(smp->flags, SMP_FLAG_CFM_DELAYED);
			return 0;
		}

		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		return smp_send_pairing_confirm(smp);
	case JUST_WORKS:
	case PASSKEY_CONFIRM:
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}
}

static uint8_t sc_smp_send_dhkey_check(struct bt_smp *smp, const uint8_t *e)
{
	struct bt_smp_dhkey_check *req;
	struct net_buf *buf;

	LOG_DBG("");

	buf = smp_create_pdu(smp, BT_SMP_DHKEY_CHECK, sizeof(*req));
	if (!buf) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	req = net_buf_add(buf, sizeof(*req));
	memcpy(req->e, e, sizeof(req->e));

	smp_send(smp, buf, NULL, NULL);

	return 0;
}

#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0U))
static uint8_t compute_and_send_central_dhcheck(struct bt_smp *smp)
{
	uint8_t e[16], r[16];

	(void)memset(r, 0, sizeof(r));

	switch (smp->method) {
	case JUST_WORKS:
	case PASSKEY_CONFIRM:
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		memcpy(r, &smp->passkey, sizeof(smp->passkey));
		break;
	case LE_SC_OOB:
		if (smp->oobd_remote) {
			memcpy(r, smp->oobd_remote->r, sizeof(r));
		}
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* calculate LTK and mackey */
	if (bt_crypto_f5(smp->dhkey, smp->prnd, smp->rrnd,
		   &smp->chan.chan.conn->le.init_addr,
		   &smp->chan.chan.conn->le.resp_addr, smp->mackey,
		   smp->tk)) {
		LOG_ERR("Calculate LTK failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}
	/* calculate local DHKey check */
	if (bt_crypto_f6(smp->mackey, smp->prnd, smp->rrnd, r, &smp->preq[1],
		   &smp->chan.chan.conn->le.init_addr,
		   &smp->chan.chan.conn->le.resp_addr, e)) {
		LOG_ERR("Calculate local DHKey check failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
	return sc_smp_send_dhkey_check(smp, e);
}
#endif /* CONFIG_BT_CENTRAL */

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
static uint8_t compute_and_check_and_send_periph_dhcheck(struct bt_smp *smp)
{
	uint8_t re[16], e[16], r[16];
	uint8_t err;

	(void)memset(r, 0, sizeof(r));

	switch (smp->method) {
	case JUST_WORKS:
	case PASSKEY_CONFIRM:
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		memcpy(r, &smp->passkey, sizeof(smp->passkey));
		break;
	case LE_SC_OOB:
		if (smp->oobd_remote) {
			memcpy(r, smp->oobd_remote->r, sizeof(r));
		}
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* calculate LTK and mackey */
	if (bt_crypto_f5(smp->dhkey, smp->rrnd, smp->prnd,
		   &smp->chan.chan.conn->le.init_addr,
		   &smp->chan.chan.conn->le.resp_addr, smp->mackey,
		   smp->tk)) {
		LOG_ERR("Calculate LTK failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* calculate local DHKey check */
	if (bt_crypto_f6(smp->mackey, smp->prnd, smp->rrnd, r, &smp->prsp[1],
		   &smp->chan.chan.conn->le.resp_addr,
		   &smp->chan.chan.conn->le.init_addr, e)) {
		LOG_ERR("Calculate local DHKey check failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (smp->method == LE_SC_OOB) {
		if (smp->oobd_local) {
			memcpy(r, smp->oobd_local->r, sizeof(r));
		} else {
			memset(r, 0, sizeof(r));
		}
	}

	/* calculate remote DHKey check */
	if (bt_crypto_f6(smp->mackey, smp->rrnd, smp->prnd, r, &smp->preq[1],
		   &smp->chan.chan.conn->le.init_addr,
		   &smp->chan.chan.conn->le.resp_addr, re)) {
		LOG_ERR("Calculate remote DHKey check failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	/* compare received E with calculated remote */
	if (memcmp(smp->e, re, 16)) {
		return BT_SMP_ERR_DHKEY_CHECK_FAILED;
	}

	/* send local e */
	err = sc_smp_send_dhkey_check(smp, e);
	if (err) {
		return err;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
	return 0;
}
#endif /* CONFIG_BT_PERIPHERAL */

static void bt_smp_dhkey_ready(const uint8_t *dhkey);
static uint8_t smp_dhkey_generate(struct bt_smp *smp)
{
	int err;

	atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_GEN);
	err = bt_dh_key_gen(smp->pkey, bt_smp_dhkey_ready);
	if (err) {
		atomic_clear_bit(smp->flags, SMP_FLAG_DHKEY_GEN);

		LOG_ERR("Failed to generate DHKey");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	return 0;
}

static uint8_t smp_dhkey_ready(struct bt_smp *smp, const uint8_t *dhkey)
{
	if (!dhkey) {
		return BT_SMP_ERR_DHKEY_CHECK_FAILED;
	}

	atomic_clear_bit(smp->flags, SMP_FLAG_DHKEY_PENDING);
	memcpy(smp->dhkey, dhkey, BT_DH_KEY_LEN);

	/* wait for user passkey confirmation */
	if (atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
		atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
		return 0;
	}

	/* wait for remote DHKey Check */
	if (atomic_test_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT)) {
		atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
		return 0;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_SEND)) {
#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0))
		if (smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
			return compute_and_send_central_dhcheck(smp);
		}

#endif /* CONFIG_BT_CENTRAL */

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0))
		return  compute_and_check_and_send_periph_dhcheck(smp);
#endif /* CONFIG_BT_PERIPHERAL */
	}

	return 0;
}

static struct bt_smp *smp_find(int flag)
{
	for (int i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		if (atomic_test_bit(bt_smp_pool[i].flags, flag)) {
			return &bt_smp_pool[i];
		}
	}

	return NULL;
}

static void bt_smp_dhkey_ready(const uint8_t *dhkey)
{
	LOG_DBG("%p", dhkey);
	int err;

	struct bt_smp *smp = smp_find(SMP_FLAG_DHKEY_GEN);
	if (smp) {
		atomic_clear_bit(smp->flags, SMP_FLAG_DHKEY_GEN);
		err = smp_dhkey_ready(smp, dhkey);
		if (err) {
			smp_error(smp, err);
		}
	}

	err = 0;
	do {
		smp = smp_find(SMP_FLAG_DHKEY_PENDING);
		if (smp) {
			err = smp_dhkey_generate(smp);
			if (err) {
				smp_error(smp, err);
			}
		}
	} while (smp && err);
}

static uint8_t sc_smp_check_confirm(struct bt_smp *smp)
{
	uint8_t cfm[16];
	uint8_t r;

	switch (smp->method) {
	case LE_SC_OOB:
		return 0;
	case PASSKEY_CONFIRM:
	case JUST_WORKS:
		r = 0U;
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		/*
		 * In the Passkey Entry protocol, the most significant
		 * bit of Z is set equal to one and the least
		 * significant bit is made up from one bit of the
		 * passkey e.g. if the passkey bit is 1, then Z = 0x81
		 * and if the passkey bit is 0, then Z = 0x80.
		 */
		r = (smp->passkey >> smp->passkey_round) & 0x01;
		r |= 0x80;
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	if (bt_crypto_f4(smp->pkey, sc_public_key, smp->rrnd, r, cfm)) {
		LOG_ERR("Calculate confirm failed");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	LOG_DBG("pcnf %s", bt_hex(smp->pcnf, 16));
	LOG_DBG("cfm %s", bt_hex(cfm, 16));

	if (memcmp(smp->pcnf, cfm, 16)) {
		return BT_SMP_ERR_CONFIRM_FAILED;
	}

	return 0;
}
#endif

static bool le_sc_oob_data_req_check(struct bt_smp *smp)
{
	struct bt_smp_pairing *req = (struct bt_smp_pairing *)&smp->preq[1];

	return ((req->oob_flag & BT_SMP_OOB_DATA_MASK) == BT_SMP_OOB_PRESENT);
}

static bool le_sc_oob_data_rsp_check(struct bt_smp *smp)
{
	struct bt_smp_pairing *rsp = (struct bt_smp_pairing *)&smp->prsp[1];

	return ((rsp->oob_flag & BT_SMP_OOB_DATA_MASK) == BT_SMP_OOB_PRESENT);
}

#if 0
static void le_sc_oob_config_set(struct bt_smp *smp,
				 struct bt_conn_oob_info *info)
{
	bool req_oob_present = le_sc_oob_data_req_check(smp);
	bool rsp_oob_present = le_sc_oob_data_rsp_check(smp);
	int oob_config = BT_CONN_OOB_NO_DATA;

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		oob_config = req_oob_present ? BT_CONN_OOB_REMOTE_ONLY :
					       BT_CONN_OOB_NO_DATA;

		if (rsp_oob_present) {
			oob_config = (oob_config == BT_CONN_OOB_REMOTE_ONLY) ?
				     BT_CONN_OOB_BOTH_PEERS :
				     BT_CONN_OOB_LOCAL_ONLY;
		}
	} else if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		oob_config = req_oob_present ? BT_CONN_OOB_LOCAL_ONLY :
					       BT_CONN_OOB_NO_DATA;

		if (rsp_oob_present) {
			oob_config = (oob_config == BT_CONN_OOB_LOCAL_ONLY) ?
				     BT_CONN_OOB_BOTH_PEERS :
				     BT_CONN_OOB_REMOTE_ONLY;
		}
	}

	info->lesc.oob_config = oob_config;
}

static uint8_t smp_pairing_random(struct bt_smp *smp, struct net_buf *buf)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_smp_pairing_random *req = (void *)buf->data;
	uint32_t passkey;
	uint8_t err;

	LOG_DBG("");

	memcpy(smp->rrnd, req->val, sizeof(smp->rrnd));

#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		return legacy_pairing_random(smp);
	}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0U))
	if (smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		err = sc_smp_check_confirm(smp);
		if (err) {
			return err;
		}

		switch (smp->method) {
		case PASSKEY_CONFIRM:
			/* compare passkey before calculating LTK */
			if (bt_crypto_g2(sc_public_key, smp->pkey, smp->prnd,
				   smp->rrnd, &passkey)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			smp_auth_cb->passkey_confirm(smp->chan.chan.conn, passkey);
			return 0;
		case JUST_WORKS:
			break;
		case LE_SC_OOB:
			break;
		case PASSKEY_DISPLAY:
		case PASSKEY_INPUT:
			smp->passkey_round++;
			if (smp->passkey_round == 20U) {
				break;
			}

			if (bt_rand(smp->prnd, 16)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);
			return smp_send_pairing_confirm(smp);
		default:
			LOG_ERR("Unknown pairing method (%u)", smp->method);
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* wait for DHKey being generated */
		if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_PENDING)) {
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			return 0;
		}

		return compute_and_send_central_dhcheck(smp);
	}
#endif /* CONFIG_BT_CENTRAL */

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
	switch (smp->method) {
	case PASSKEY_CONFIRM:
		if (bt_crypto_g2(smp->pkey, sc_public_key, smp->rrnd, smp->prnd,
			   &passkey)) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->passkey_confirm(smp->chan.chan.conn, passkey);
		break;
	case JUST_WORKS:
		break;
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
		err = sc_smp_check_confirm(smp);
		if (err) {
			return err;
		}

		atomic_set_bit(smp->allowed_cmds,
			       BT_SMP_CMD_PAIRING_CONFIRM);
		err = smp_send_pairing_random(smp);
		if (err) {
			return err;
		}

		smp->passkey_round++;
		if (smp->passkey_round == 20U) {
			atomic_set_bit(smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
			atomic_set_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT);
			return 0;
		}

		if (bt_rand(smp->prnd, 16)) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		return 0;
	case LE_SC_OOB:
		/* Step 6: Select random N */
		if (bt_rand(smp->prnd, 16)) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		if (smp_auth_cb && smp_auth_cb->oob_data_request) {
			struct bt_conn_oob_info info = {
				.type = BT_CONN_OOB_LE_SC,
				.lesc.oob_config = BT_CONN_OOB_NO_DATA,
			};

			le_sc_oob_config_set(smp, &info);

			smp->oobd_local = NULL;
			smp->oobd_remote = NULL;

			atomic_set_bit(smp->flags, SMP_FLAG_OOB_PENDING);
			smp_auth_cb->oob_data_request(smp->chan.chan.conn, &info);

			return 0;
		} else {
			return BT_SMP_ERR_OOB_NOT_AVAIL;
		}
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
	atomic_set_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT);
	return smp_send_pairing_random(smp);
#else
	return BT_SMP_ERR_PAIRING_NOTSUPP;
#endif /* CONFIG_BT_PERIPHERAL */
}

static uint8_t smp_pairing_failed(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_smp_pairing_fail *req = (void *)buf->data;

	LOG_ERR("reason 0x%x", req->reason);

	if (atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER) ||
	    atomic_test_and_clear_bit(smp->flags, SMP_FLAG_DISPLAY)) {
		if (smp_auth_cb && smp_auth_cb->cancel) {
			smp_auth_cb->cancel(conn);
		}
	}

	smp_pairing_complete(smp, req->reason);

	/* return no error to avoid sending Pairing Failed in response */
	return 0;
}

static uint8_t smp_ident_info(struct bt_smp *smp, struct net_buf *buf)
{
	LOG_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_ident_info *req = (void *)buf->data;
		struct bt_conn *conn = smp->chan.chan.conn;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->irk.val, req->irk, 16);
	}

	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_ADDR_INFO);

static uint8_t smp_id_add_replace(struct bt_smp *smp, struct bt_keys *new_bond)
{
	struct bt_keys *conflict;

	/* Sanity check: It does not make sense to finalize a bond before we
	 * have the remote identity.
	 */
	__ASSERT_NO_MSG(!(smp->remote_dist & BT_SMP_DIST_ID_KEY));

	conflict = bt_id_find_conflict(new_bond);
	if (conflict) {
		LOG_DBG("New bond conflicts with a bond on id %d.", conflict->id);
	}
	if (conflict && !IS_ENABLED(CONFIG_BT_ID_UNPAIR_MATCHING_BONDS)) {
		LOG_WRN("Refusing new pairing. The old bond must be unpaired first.");
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	if (conflict && IS_ENABLED(CONFIG_BT_ID_UNPAIR_MATCHING_BONDS)) {
		bool trust_ok;
		int unpair_err;

		trust_ok = update_keys_check(smp, conflict);
		if (!trust_ok) {
			LOG_WRN("Refusing new pairing. The old bond has more trust.");
			return BT_SMP_ERR_AUTH_REQUIREMENTS;
		}

		LOG_DBG("Un-pairing old conflicting bond and finalizing new.");

		unpair_err = bt_unpair(conflict->id, &conflict->addr);
		__ASSERT_NO_MSG(!unpair_err);
	}

	__ASSERT_NO_MSG(!bt_id_find_conflict(new_bond));
	bt_id_add(new_bond);
	return 0;
}

struct addr_match {
	const bt_addr_le_t *rpa;
	const bt_addr_le_t *id_addr;
};

static void convert_to_id_on_match(struct bt_conn *conn, void *data)
{
	struct addr_match *addr_match = data;

	if (bt_addr_le_eq(&conn->le.dst, addr_match->rpa)) {
		bt_addr_le_copy(&conn->le.dst, addr_match->id_addr);
	}
}

static uint8_t smp_ident_addr_info(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_ident_addr_info *req = (void *)buf->data;
	uint8_t err;

	LOG_DBG("identity %s", bt_addr_le_str(&req->addr));

	smp->remote_dist &= ~BT_SMP_DIST_ID_KEY;

	if (!bt_addr_le_is_identity(&req->addr)) {
		LOG_ERR("Invalid identity %s", bt_addr_le_str(&req->addr));
		LOG_ERR(" for %s", bt_addr_le_str(&conn->le.dst));
		return BT_SMP_ERR_INVALID_PARAMS;
	}

	if (!bt_addr_le_eq(&conn->le.dst, &req->addr)) {
		struct bt_keys *keys = bt_keys_find_addr(conn->id, &req->addr);

		if (keys) {
			if (!update_keys_check(smp, keys)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			bt_keys_clear(keys);
		}
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		const bt_addr_le_t *dst;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/*
		 * We can't use conn->dst here as this might already contain
		 * identity address known from previous pairing. Since all keys
		 * are cleared on re-pairing we wouldn't store IRK distributed
		 * in new pairing.
		 */
		if (conn->role == BT_HCI_ROLE_CENTRAL) {
			dst = &conn->le.resp_addr;
		} else {
			dst = &conn->le.init_addr;
		}

		if (bt_addr_le_is_rpa(dst)) {
			/* always update last use RPA */
			bt_addr_copy(&keys->irk.rpa, &dst->a);

			/*
			 * Update connection address and notify about identity
			 * resolved only if connection wasn't already reported
			 * with identity address. This may happen if IRK was
			 * present before ie. due to re-pairing.
			 */
			if (!bt_addr_le_is_identity(&conn->le.dst)) {
				struct addr_match addr_match = {
					.rpa = &conn->le.dst,
					.id_addr = &req->addr,
				};

				bt_conn_foreach(BT_CONN_TYPE_LE,
						convert_to_id_on_match,
						&addr_match);
				bt_addr_le_copy(&keys->addr, &req->addr);

				bt_conn_identity_resolved(conn);
			}
		}

		err = smp_id_add_replace(smp, keys);
		if (err) {
			return err;
		}
	}

	if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL && !smp->remote_dist) {
		err = bt_smp_distribute_keys(smp);
		if (err) {
			return err;
		}
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_complete(smp, 0);
	}

	return 0;
}

#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
static uint8_t smp_signing_info(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	uint8_t err;

	LOG_DBG("");

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_smp_signing_info *req = (void *)buf->data;
		struct bt_keys *keys;

		keys = bt_keys_get_type(BT_KEYS_REMOTE_CSRK, conn->id,
					&conn->le.dst);
		if (!keys) {
			LOG_ERR("Unable to get keys for %s",
			       bt_addr_le_str(&conn->le.dst));
			return BT_SMP_ERR_UNSPECIFIED;
		}

		memcpy(keys->remote_csrk.val, req->csrk,
		       sizeof(keys->remote_csrk.val));
	}

	smp->remote_dist &= ~BT_SMP_DIST_SIGN;

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL && !smp->remote_dist) {
		err = bt_smp_distribute_keys(smp);
		if (err) {
			return err;
		}
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_complete(smp, 0);
	}

	return 0;
}
#else
static uint8_t smp_signing_info(struct bt_smp *smp, struct net_buf *buf)
{
	return BT_SMP_ERR_CMD_NOTSUPP;
}
#endif /* CONFIG_BT_SIGNING */
#endif

#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0U))
static uint8_t smp_security_request(struct bt_smp *smp, SMP_AUTH_INFO *auth)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	LOG_DBG("");

	/* A higher security level is requested during the key distribution
	 * phase, once pairing is complete a new pairing procedure will start.
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_KEYS_DISTR)) {
		return 0;
	}

	auth->param = SMP_ERROR_NONE;

	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		/* We have already started pairing process */
		return 0;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		/* We have already started encryption procedure */
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) && (SMP_LESC_MODE != auth->pair_mode)) {
		return BT_SMP_ERR_AUTH_REQUIREMENTS;
	}

	if (IS_ENABLED(CONFIG_BT_BONDING_REQUIRED) &&
	    !(bondable && (SMP_BONDING == auth->bonding))) {
		/* Reject security req if not both intend to bond */
		LOG_DBG("Bonding required");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	memcpy(&smp->auth, auth, sizeof(smp->auth));

	if (smp_send_pairing_req(conn) < 0) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_SEC_REQ);

	return 0;
}
#endif /* CONFIG_BT_CENTRAL */

#if 0
static uint8_t generate_dhkey(struct bt_smp *smp)
{
	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		return BT_SMP_ERR_UNSPECIFIED;
	}

	atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_PENDING);
	if (!smp_find(SMP_FLAG_DHKEY_GEN)) {
		return smp_dhkey_generate(smp);
	}

	return 0;
}

static uint8_t display_passkey(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);

	if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
	    fixed_passkey != BT_PASSKEY_INVALID) {
		smp->passkey = fixed_passkey;
	} else {
		if (bt_rand(&smp->passkey, sizeof(smp->passkey))) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		smp->passkey %= 1000000;
	}

	smp->passkey_round = 0U;

	if (smp_auth_cb && smp_auth_cb->passkey_display) {
		atomic_set_bit(smp->flags, SMP_FLAG_DISPLAY);
		smp_auth_cb->passkey_display(conn, smp->passkey);
	}

	smp->passkey = sys_cpu_to_le32(smp->passkey);

	return 0;
}

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
static uint8_t smp_public_key_periph(struct bt_smp *smp)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	uint8_t err;

	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY) &&
	    memcmp(smp->pkey, sc_public_key, BT_PUB_KEY_COORD_LEN) == 0) {
		/* Deny public key with identitcal X coordinate unless it is the
		 * debug public key.
		 */
		LOG_WRN("Remote public key rejected");
		return BT_SMP_ERR_UNSPECIFIED;
	}

	err = sc_send_public_key(smp);
	if (err) {
		return err;
	}

	switch (smp->method) {
	case PASSKEY_CONFIRM:
	case JUST_WORKS:
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);

		err = smp_send_pairing_confirm(smp);
		if (err) {
			return err;
		}
		break;
	case PASSKEY_DISPLAY:
		err = display_passkey(smp);
		if (err) {
			return err;
		}

		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
		break;
	case PASSKEY_INPUT:
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_CONFIRM);
		atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
		atomic_set_bit(smp->flags, SMP_FLAG_USER);
		smp_auth_cb->passkey_entry(smp->chan.chan.conn);
		break;
	case LE_SC_OOB:
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
		break;
	default:
		LOG_ERR("Unknown pairing method (%u)", smp->method);
		return BT_SMP_ERR_UNSPECIFIED;
	}

	return generate_dhkey(smp);
}
#endif /* CONFIG_BT_PERIPHERAL */

static uint8_t smp_public_key(struct bt_smp *smp, struct net_buf *buf)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_smp_public_key *req = (void *)buf->data;
	uint8_t err;

	LOG_DBG("");

	memcpy(smp->pkey, req->x, BT_PUB_KEY_COORD_LEN);
	memcpy(&smp->pkey[BT_PUB_KEY_COORD_LEN], req->y, BT_PUB_KEY_COORD_LEN);

	/* mark key as debug if remote is using it */
	if (bt_pub_key_is_debug(smp->pkey)) {
		LOG_INF("Remote is using Debug Public key");
		atomic_set_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY);

		/* Don't allow a bond established without debug key to be
		 * updated using LTK generated from debug key.
		 */
		if (!update_debug_keys_check(smp)) {
			return BT_SMP_ERR_AUTH_REQUIREMENTS;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		if (!atomic_test_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY) &&
		    memcmp(smp->pkey, sc_public_key, BT_PUB_KEY_COORD_LEN) == 0) {
			/* Deny public key with identitcal X coordinate unless
			 * it is the debug public key.
			 */
			LOG_WRN("Remote public key rejected");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		switch (smp->method) {
		case PASSKEY_CONFIRM:
		case JUST_WORKS:
			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);
			break;
		case PASSKEY_DISPLAY:
			err = display_passkey(smp);
			if (err) {
				return err;
			}

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_KEYPRESS_NOTIFICATION);

			err = smp_send_pairing_confirm(smp);
			if (err) {
				return err;
			}
			break;
		case PASSKEY_INPUT:
			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			smp_auth_cb->passkey_entry(smp->chan.chan.conn);

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_KEYPRESS_NOTIFICATION);

			break;
		case LE_SC_OOB:
			/* Step 6: Select random N */
			if (bt_rand(smp->prnd, 16)) {
				return BT_SMP_ERR_UNSPECIFIED;
			}

			if (smp_auth_cb && smp_auth_cb->oob_data_request) {
				struct bt_conn_oob_info info = {
					.type = BT_CONN_OOB_LE_SC,
					.lesc.oob_config = BT_CONN_OOB_NO_DATA,
				};

				le_sc_oob_config_set(smp, &info);

				smp->oobd_local = NULL;
				smp->oobd_remote = NULL;

				atomic_set_bit(smp->flags,
					       SMP_FLAG_OOB_PENDING);
				smp_auth_cb->oob_data_request(smp->chan.chan.conn, &info);
			} else {
				return BT_SMP_ERR_OOB_NOT_AVAIL;
			}
			break;
		default:
			LOG_ERR("Unknown pairing method (%u)", smp->method);
			return BT_SMP_ERR_UNSPECIFIED;
		}

		return generate_dhkey(smp);
	}

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
	if (!sc_public_key) {
		atomic_set_bit(smp->flags, SMP_FLAG_PKEY_SEND);
		return 0;
	}

	err = smp_public_key_periph(smp);
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return 0;
}

static uint8_t smp_dhkey_check(struct bt_smp *smp, struct net_buf *buf)
{
	struct bt_smp_dhkey_check *req = (void *)buf->data;

	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		uint8_t e[16], r[16], enc_size;
		uint8_t ediv[2], rand[8];

		(void)memset(r, 0, sizeof(r));

		switch (smp->method) {
		case JUST_WORKS:
		case PASSKEY_CONFIRM:
			break;
		case PASSKEY_DISPLAY:
		case PASSKEY_INPUT:
			memcpy(r, &smp->passkey, sizeof(smp->passkey));
			break;
		case LE_SC_OOB:
			if (smp->oobd_local) {
				memcpy(r, smp->oobd_local->r, sizeof(r));
			}
			break;
		default:
			LOG_ERR("Unknown pairing method (%u)", smp->method);
			return BT_SMP_ERR_UNSPECIFIED;
		}

		/* calculate remote DHKey check for comparison */
		if (bt_crypto_f6(smp->mackey, smp->rrnd, smp->prnd, r, &smp->prsp[1],
			   &smp->chan.chan.conn->le.resp_addr,
			   &smp->chan.chan.conn->le.init_addr, e)) {
			return BT_SMP_ERR_UNSPECIFIED;
		}

		if (memcmp(e, req->e, 16)) {
			return BT_SMP_ERR_DHKEY_CHECK_FAILED;
		}

		enc_size = get_encryption_key_size(smp);

		/* Rand and EDiv are 0 */
		(void)memset(ediv, 0, sizeof(ediv));
		(void)memset(rand, 0, sizeof(rand));
		if (bt_conn_le_start_encryption(smp->chan.chan.conn, rand, ediv,
						smp->tk, enc_size) < 0) {
			LOG_ERR("Failed to start encryption");
			return BT_SMP_ERR_UNSPECIFIED;
		}

		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);

		if (IS_ENABLED(CONFIG_BT_SMP_USB_HCI_CTLR_WORKAROUND)) {
			if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_IDENT_INFO);
			} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
				atomic_set_bit(smp->allowed_cmds,
					       BT_SMP_CMD_SIGNING_INFO);
			}
		}

		return 0;
	}

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
	if (smp->chan.chan.conn->role == BT_HCI_ROLE_PERIPHERAL) {
		atomic_clear_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT);
		memcpy(smp->e, req->e, sizeof(smp->e));

		/* wait for DHKey being generated */
		if (atomic_test_bit(smp->flags, SMP_FLAG_DHKEY_PENDING)) {
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			return 0;
		}

		/* waiting for user to confirm passkey */
		if (atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
			atomic_set_bit(smp->flags, SMP_FLAG_DHKEY_SEND);
			return 0;
		}

		return compute_and_check_and_send_periph_dhcheck(smp);
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return 0;
}
#endif

#if 0
#if defined(CONFIG_BT_PASSKEY_KEYPRESS) && (CONFIG_BT_PASSKEY_KEYPRESS > 0)
static uint8_t smp_keypress_notif(struct bt_smp *smp, struct net_buf *buf)
{
	const struct bt_conn_auth_cb *smp_auth_cb = latch_auth_cb(smp);
	struct bt_conn *conn = smp->chan.chan.conn;
	struct bt_smp_keypress_notif *notif = (void *)buf->data;
	enum bt_conn_auth_keypress type = notif->type;

	LOG_DBG("Keypress from conn %u, type %u", bt_conn_index(conn), type);

	/* For now, keypress notifications are always accepted. In the future we
	 * should be smarter about this. We might also want to enforce something
	 * about the 'start' and 'end' messages.
	 */
	atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);

	if (!IN_RANGE(type,
		      BT_CONN_AUTH_KEYPRESS_ENTRY_STARTED,
		      BT_CONN_AUTH_KEYPRESS_ENTRY_COMPLETED)) {
		LOG_WRN("Received unknown keypress event type %u. Discarding.", type);
		return BT_SMP_ERR_INVALID_PARAMS;
	}

	/* Reset SMP timeout, like the spec says. */
	k_work_reschedule(&smp->work, SMP_TIMEOUT);

	if (smp_auth_cb->passkey_display_keypress) {
		smp_auth_cb->passkey_display_keypress(conn, type);
	}

	return 0;
}
#else
static uint8_t smp_keypress_notif(struct bt_smp *smp, struct net_buf *buf)
{
	ARG_UNUSED(smp);
	ARG_UNUSED(buf);

	LOG_DBG("");

	/* Ignore packets until keypress notifications are fully supported. */
	atomic_set_bit(smp->allowed_cmds, BT_SMP_KEYPRESS_NOTIFICATION);
	return 0;
}
#endif

static const struct {
	uint8_t  (*func)(struct bt_smp *smp, struct net_buf *buf);
	uint8_t  expect_len;
} handlers[] = {
	{ NULL, 0}, /* No op-code defined for 0x00 */
	{ smp_pairing_req,         sizeof(struct bt_smp_pairing) },
	{ smp_pairing_rsp,         sizeof(struct bt_smp_pairing) },
	{ smp_pairing_confirm,     sizeof(struct bt_smp_pairing_confirm) },
	{ smp_pairing_random,      sizeof(struct bt_smp_pairing_random) },
	{ smp_pairing_failed,      sizeof(struct bt_smp_pairing_fail) },
	{ smp_encrypt_info,        sizeof(struct bt_smp_encrypt_info) },
	{ smp_central_ident,        sizeof(struct bt_smp_central_ident) },
	{ smp_ident_info,          sizeof(struct bt_smp_ident_info) },
	{ smp_ident_addr_info,     sizeof(struct bt_smp_ident_addr_info) },
	{ smp_signing_info,        sizeof(struct bt_smp_signing_info) },
	{ smp_security_request,    sizeof(struct bt_smp_security_request) },
	{ smp_public_key,          sizeof(struct bt_smp_public_key) },
	{ smp_dhkey_check,         sizeof(struct bt_smp_dhkey_check) },
	{ smp_keypress_notif,      sizeof(struct bt_smp_keypress_notif) },
};
static bool is_in_pairing_procedure(struct bt_smp *smp)
{
	return atomic_test_bit(smp->flags, SMP_FLAG_PAIRING);
}

static int bt_smp_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan);
	struct bt_smp_hdr *hdr;
	uint8_t err;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small SMP PDU received");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	LOG_DBG("Received SMP code 0x%02x len %u", hdr->code, buf->len);

	/*
	 * If SMP timeout occurred "no further SMP commands shall be sent over
	 * the L2CAP Security Manager Channel. A new SM procedure shall only be
	 * performed when a new physical link has been established."
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_TIMEOUT)) {
		LOG_WRN("SMP command (code 0x%02x) received after timeout",
			hdr->code);
		return 0;
	}

	/*
	 * Bluetooth Core Specification Version 5.2, Vol 3, Part H, page 1667:
	 * If a packet is received with a Code that is reserved for future use
	 * it shall be ignored.
	 */
	if (hdr->code >= ARRAY_SIZE(handlers)) {
		LOG_WRN("Received reserved SMP code 0x%02x", hdr->code);
		return 0;
	}

	if (!handlers[hdr->code].func) {
		LOG_WRN("Unhandled SMP code 0x%02x", hdr->code);
		smp_error(smp, BT_SMP_ERR_CMD_NOTSUPP);
		return 0;
	}

	if (!atomic_test_and_clear_bit(smp->allowed_cmds, hdr->code)) {
		LOG_WRN("Unexpected SMP code 0x%02x", hdr->code);
		/* Don't send error responses to error PDUs */
		if (hdr->code != BT_SMP_CMD_PAIRING_FAIL) {
			smp_error(smp, BT_SMP_ERR_UNSPECIFIED);
		}
		return 0;
	}

	if (buf->len != handlers[hdr->code].expect_len) {
		LOG_ERR("Invalid len %u for code 0x%02x", buf->len, hdr->code);
		smp_error(smp, BT_SMP_ERR_INVALID_PARAMS);
		return 0;
	}

	err = handlers[hdr->code].func(smp, buf);
	if (err) {
		smp_error(smp, err);
	}

	return 0;
}

static void bt_smp_pkey_ready(const uint8_t *pkey)
{
	int i;

	LOG_DBG("");

	sc_public_key = pkey;

	if (!pkey) {
		LOG_WRN("Public key not available");
		return;
	}

	OSA_SemaphorePost(sc_local_pkey_ready);

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		struct bt_smp *smp = &bt_smp_pool[i];
		uint8_t err;

		if (!atomic_test_bit(smp->flags, SMP_FLAG_PKEY_SEND)) {
			continue;
		}

		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
			err = sc_send_public_key(smp);
			if (err) {
				smp_error(smp, err);
			}

			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PUBLIC_KEY);
			continue;
		}

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
		err = smp_public_key_periph(smp);
		if (err) {
			smp_error(smp, err);
		}
#endif /* CONFIG_BT_PERIPHERAL */
	}
}
#endif

static void smp_auth_starting(struct bt_smp *smp);

static void bt_smp_connected(struct bt_l2cap_chan *chan)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan);

	LOG_DBG("chan %p cid 0x%04x", chan,
	       CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan)->tx.cid);
#if 0
	k_work_init_delayable(&smp->work, smp_timeout);
#endif
	k_work_init_delayable(&smp->id_add, smp_id_add);
	smp_reset(smp);

	atomic_ptr_set(&smp->auth_cb, BT_SMP_AUTH_CB_UNINITIALIZED);
}

static void bt_smp_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan);
	struct bt_keys *keys = chan->conn->le.keys;

	LOG_DBG("chan %p cid 0x%04x", chan,
	       CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan)->tx.cid);
#if 0
	/* Channel disconnected callback is always called from a work handler
	 * so canceling of the timeout work should always succeed.
	 */
	(void)k_work_cancel_delayable(&smp->work);
#endif

	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING) ||
	    atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING) ||
	    atomic_test_bit(smp->flags, SMP_FLAG_SEC_REQ)) {
		/* reset context and report */
		smp_pairing_complete(smp, BT_SMP_ERR_UNSPECIFIED);
	}

	k_work_cancel_delayable(&smp->id_add);
	if (keys) {
		/*
		 * If debug keys were used for pairing remove them.
		 * No keys indicate no bonding so free keys storage.
		 */
		if (!keys->keys || (!IS_ENABLED(CONFIG_BT_STORE_DEBUG_KEYS) &&
		    (keys->flags & BT_KEYS_DEBUG))) {
			bt_keys_clear(keys);
		}
	}

	(void)memset(smp, 0, sizeof(*smp));
}

static void bt_smp_encrypt_change(struct bt_l2cap_chan *chan,
				  uint8_t hci_status)
{
	struct bt_smp *smp = CONTAINER_OF(chan, struct bt_smp, chan);
	struct bt_conn *conn = chan->conn;

	LOG_DBG("chan %p conn %p handle %u encrypt 0x%02x hci status 0x%02x",
	       chan, conn, conn->handle, conn->encrypt, hci_status);

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		/* We where not waiting for encryption procedure.
		 * This happens when encrypt change is called to notify that
		 * security has failed before starting encryption.
		 */
		return;
	}

	if (hci_status) {
		if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
			uint8_t smp_err = smp_err_get(
				bt_security_err_get(hci_status));

			/* Fail as if it happened during key distribution */
			atomic_set_bit(smp->flags, SMP_FLAG_KEYS_DISTR);
			smp_pairing_complete(smp, smp_err);
		}

		return;
	}

	if (!conn->encrypt) {
		return;
	}

	/* We were waiting for encryption but with no pairing in progress.
	 * This can happen if paired peripheral sent Security Request and we
	 * enabled encryption.
	 */
	if (!atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		smp_reset(smp);
		return;
	}

	/* derive BR/EDR LinkKey if supported by both sides */
	if (atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		if ((smp->local_dist & BT_SMP_DIST_LINK_KEY) &&
		    (smp->remote_dist & BT_SMP_DIST_LINK_KEY)) {
			/*
			 * Link Key will be derived after key distribution to
			 * make sure remote device identity is known
			 */
			atomic_set_bit(smp->flags, SMP_FLAG_DERIVE_LK);
		}
		/*
		 * Those are used as pairing finished indicator so generated
		 * but not distributed keys must be cleared here.
		 */
		smp->local_dist &= ~BT_SMP_DIST_LINK_KEY;
		smp->remote_dist &= ~BT_SMP_DIST_LINK_KEY;
	}
	else
	{
		/*
		 * Link key should be ignored for legacy pairing.
		 */
		smp->local_dist &= ~BT_SMP_DIST_LINK_KEY;
		smp->remote_dist &= ~BT_SMP_DIST_LINK_KEY;
	}

	if (smp->remote_dist & BT_SMP_DIST_ENC_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_ENCRYPT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_ID_KEY) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_IDENT_INFO);
	} else if (smp->remote_dist & BT_SMP_DIST_SIGN) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SIGNING_INFO);
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    !(smp->remote_dist & BT_SMP_DIST_ID_KEY)) {
		/* To resolve directed advertising we need our local IRK
		 * in the controllers resolving list, add it now since the
		 * peer has no identity key.
		 */
		bt_id_add(conn->le.keys);
	}

	atomic_set_bit(smp->flags, SMP_FLAG_KEYS_DISTR);

	/* Peripheral distributes it's keys first */
	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL && smp->remote_dist) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		/* Avoid the HCI-USB race condition where HCI data and
		 * HCI events can be re-ordered, and pairing information appears
		 * to be sent unencrypted.
		 */
		k_sleep(BT_MSEC(100));
	}

	if (bt_smp_distribute_keys(smp)) {
		return;
	}

	/* if all keys were distributed, pairing is done */
	if (!smp->local_dist && !smp->remote_dist) {
		smp_pairing_complete(smp, 0);
	}
}

#if ((defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U)) || (defined(CONFIG_BT_SMP_SELFTEST) && ((CONFIG_BT_SMP_SELFTEST) > 0U)))
/* Sign message using msg as a buffer, len is a size of the message,
 * msg buffer contains message itself, 32 bit count and signature,
 * so total buffer size is len + 4 + 8 octets.
 * API is Little Endian to make it suitable for Bluetooth.
 */
static int smp_sign_buf(const uint8_t *key, uint8_t *msg, uint16_t len)
{
	uint8_t *m = msg;
	uint32_t cnt = UNALIGNED_GET((uint32_t *)&msg[len]);
	uint8_t *sig = msg + len;
	uint8_t key_s[16], tmp[16];
	int err;

	LOG_DBG("Signing msg %s len %u key %s", bt_hex(msg, len), len,
	       bt_hex(key, 16));

	sys_mem_swap(m, len + sizeof(cnt));
	sys_memcpy_swap(key_s, key, 16);

	err = bt_crypto_aes_cmac(key_s, m, len + sizeof(cnt), tmp);
	if (err) {
		LOG_ERR("Data signing failed");
		return err;
	}

	sys_mem_swap(tmp, sizeof(tmp));
	memcpy(tmp + 4, &cnt, sizeof(cnt));

	/* Swap original message back */
	sys_mem_swap(m, len + sizeof(cnt));

	memcpy(sig, tmp + 4, 12);

	LOG_DBG("sig %s", bt_hex(sig, 12));

	return 0;
}
#endif

#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
int bt_smp_sign_verify(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_keys *keys;
	uint8_t sig[12];
	uint32_t cnt;
	uint32_t tempCnt;
	int err;

	/* Store signature incl. count */
	memcpy(sig, net_buf_tail(buf) - sizeof(sig), sizeof(sig));

	keys = bt_keys_find(BT_KEYS_REMOTE_CSRK, conn->id, &conn->le.dst);
	if (!keys) {
		LOG_ERR("Unable to find Remote CSRK for %s",
		       bt_addr_le_str(&conn->le.dst));
		return -ENOENT;
	}

	/* Copy signing count */
	memcpy(&tempCnt, net_buf_tail(buf) - sizeof(sig) , sizeof(tempCnt));

	tempCnt = sys_le32_to_cpu(tempCnt);

	if (tempCnt > keys->remote_csrk.cnt)
	{
		cnt = sys_cpu_to_le32(tempCnt);
	}
	else
	{
		cnt = sys_cpu_to_le32(keys->remote_csrk.cnt);
		memcpy(net_buf_tail(buf) - sizeof(sig), &cnt, sizeof(cnt));
	}

	LOG_DBG("Sign data len %lu key %s count %u", buf->len - sizeof(sig),
	       bt_hex(keys->remote_csrk.val, 16), sys_le32_to_cpu(cnt));

	err = smp_sign_buf(keys->remote_csrk.val, buf->data,
			   buf->len - sizeof(sig));
	if (err) {
		LOG_ERR("Unable to create signature for %s",
		       bt_addr_le_str(&conn->le.dst));
		return -EIO;
	}

	if (memcmp(sig, net_buf_tail(buf) - sizeof(sig), sizeof(sig))) {
		LOG_ERR("Unable to verify signature for %s",
		       bt_addr_le_str(&conn->le.dst));
		return -EBADMSG;
	}

	keys->remote_csrk.cnt = sys_le32_to_cpu(cnt) + 1;

	return 0;
}

int bt_smp_sign(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_keys *keys;
	uint32_t cnt;
	int err;

	keys = bt_keys_find(BT_KEYS_LOCAL_CSRK, conn->id, &conn->le.dst);
	if (!keys) {
		LOG_ERR("Unable to find local CSRK for %s",
		       bt_addr_le_str(&conn->le.dst));
		return -ENOENT;
	}

	/* Reserve space for data signature */
	net_buf_add(buf, 12);

	/* Copy signing count */
	cnt = sys_cpu_to_le32(keys->local_csrk.cnt);
	memcpy(net_buf_tail(buf) - 12, &cnt, sizeof(cnt));

	LOG_DBG("Sign data len %u key %s count %u", buf->len,
	       bt_hex(keys->local_csrk.val, 16), keys->local_csrk.cnt);

	err = smp_sign_buf(keys->local_csrk.val, buf->data, buf->len - 12);
	if (err) {
		LOG_ERR("Unable to create signature for %s",
		       bt_addr_le_str(&conn->le.dst));
		return -EIO;
	}

	keys->local_csrk.cnt++;

	return 0;
}
#else
int bt_smp_sign_verify(struct bt_conn *conn, struct net_buf *buf)
{
	return -ENOTSUP;
}

int bt_smp_sign(struct bt_conn *conn, struct net_buf *buf)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_SIGNING */

int bt_smp_irk_get(uint8_t *ir, uint8_t *irk)
{
	uint8_t invalid_ir[16] = { 0 };

	if (!memcmp(ir, invalid_ir, 16)) {
		return -EINVAL;
	}

	return smp_d1(ir, 1, 0, irk);
}

#if (defined(CONFIG_BT_SMP_SELFTEST) && ((CONFIG_BT_SMP_SELFTEST) > 0U))
static void smp_selftest_thread(void *param);

static OSA_TASK_HANDLE_DEFINE(smp_selftest_data);
static OSA_TASK_DEFINE( smp_selftest_thread, (OSA_TASK_PRIORITY_MIN - 1), 1, (1024 * 2), 0);
/* Test vectors are taken from RFC 4493
 * https://tools.ietf.org/html/rfc4493
 * Same mentioned in the Bluetooth Spec.
 */
static const uint8_t key[] = {
	0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
	0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static const uint8_t M[] = {
	0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
	0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
	0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
	0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
	0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
	0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
	0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
	0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
};

static int aes_test(const char *prefix, const uint8_t *in_key, const uint8_t *m,
		    uint16_t len, const uint8_t *mac)
{
	uint8_t out[16];

	LOG_DBG("%s: AES CMAC of message with len %u", prefix, len);

	bt_smp_aes_cmac(in_key, m, len, out);
	if (!memcmp(out, mac, 16)) {
		LOG_DBG("%s: Success", prefix);
	} else {
		LOG_ERR("%s: Failed", prefix);
		return -1;
	}

	return 0;
}

static int smp_aes_cmac_null_msg_test(void)
{
    bt_aes_128_cmac_state_t state;
#ifndef BUF_LEN
#define BUF_LEN 16
#endif
    memset(&state, 0, sizeof(state));
    const uint8_t key[BUF_LEN] = {
		0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
		0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
	};
	const uint8_t tag[BUF_LEN] = {
		0xbb, 0x1d, 0x69, 0x29, 0xe9, 0x59, 0x37, 0x28,
		0x7f, 0xa3, 0x7d, 0x12, 0x9b, 0x75, 0x67, 0x46
	};
	uint8_t Tag[BUF_LEN];

    bt_aes_128_cmac_setup(&state, key);
    bt_aes_128_cmac_init(&state);
    bt_aes_128_cmac_update(&state, NULL, 0);
    bt_aes_128_cmac_update(&state, NULL, 0);
    bt_aes_128_cmac_update(&state, NULL, 0);
    bt_aes_128_cmac_final(Tag, &state);
    if (memcmp(Tag, tag, BUF_LEN) != 0) {
        return -1;
    }
    return 0;
}


static int smp_aes_cmac_128_bit_msg_test(void)
{
    bt_aes_128_cmac_state_t state;
#ifndef BUF_LEN
#define BUF_LEN 16
#endif
    memset(&state, 0, sizeof(state));
    const uint8_t key[BUF_LEN] = {
		0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
		0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
	};
	const uint8_t msg1[] = {
		0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
		0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
	};
	const uint8_t tag[BUF_LEN] = {
		0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44,
		0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c
	};
	uint8_t Tag[BUF_LEN];

    bt_aes_128_cmac_setup(&state, key);
    bt_aes_128_cmac_init(&state);
    bt_aes_128_cmac_update(&state, msg1, sizeof(msg1));
    bt_aes_128_cmac_update(&state, NULL, 0);
    bt_aes_128_cmac_update(&state, NULL, 0);
    bt_aes_128_cmac_final(Tag, &state);
    if (memcmp(Tag, tag, BUF_LEN) != 0) {
        return -1;
    }
    return 0;
}

static int smp_aes_cmac_320_bit_msg_test(void)
{
    bt_aes_128_cmac_state_t state;
#ifndef BUF_LEN
#define BUF_LEN 16
#endif
    memset(&state, 0, sizeof(state));
    const uint8_t key[BUF_LEN] = {
		0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
		0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
	};
	const uint8_t msg1[] = {
		0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
	};
	const uint8_t msg2[] = {
		0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
		0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
		0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
	};
	const uint8_t msg3[] = {
		0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11
	};
	const uint8_t tag[BUF_LEN] = {
		0xdf, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30,
		0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27
	};
	uint8_t Tag[BUF_LEN];

    bt_aes_128_cmac_setup(&state, key);
    bt_aes_128_cmac_init(&state);
    bt_aes_128_cmac_update(&state, msg1, sizeof(msg1));
    bt_aes_128_cmac_update(&state, msg2, sizeof(msg2));
    bt_aes_128_cmac_update(&state, msg3, sizeof(msg3));
    bt_aes_128_cmac_update(&state, NULL, 0);

    bt_aes_128_cmac_final(Tag, &state);
    if (memcmp(Tag, tag, BUF_LEN) != 0) {
        return -1;
    }
    return 0;
}

static int smp_aes_cmac_512_bit_msg_test(void)
{
    bt_aes_128_cmac_state_t state;
#ifndef BUF_LEN
#define BUF_LEN 16
#endif
    memset(&state, 0, sizeof(state));
    const uint8_t key[BUF_LEN] = {
		0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
		0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
	};
	const uint8_t msg1[] = {
		0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
		0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
	};
	const uint8_t msg2[] = {
		0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03
	};
	const uint8_t msg3[] = {
        0xac, 0x9c,
		0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
	};
	const uint8_t msg4[] = {
		0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
	};
	const uint8_t msg5[] = {
		0xe5, 0xfb, 0xc1, 0x19, 0x1a,
	};
	const uint8_t msg6[] = {
        0x0a, 0x52, 0xef,
		0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
		0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
	};
	const uint8_t tag[BUF_LEN] = {
		0x51, 0xf0, 0xbe, 0xbf, 0x7e, 0x3b, 0x9d, 0x92,
		0xfc, 0x49, 0x74, 0x17, 0x79, 0x36, 0x3c, 0xfe
	};
	uint8_t Tag[BUF_LEN];

    bt_aes_128_cmac_setup(&state, key);
    bt_aes_128_cmac_init(&state);
    bt_aes_128_cmac_update(&state, msg1, sizeof(msg1));
    bt_aes_128_cmac_update(&state, msg2, sizeof(msg2));
    bt_aes_128_cmac_update(&state, msg3, sizeof(msg3));
    bt_aes_128_cmac_update(&state, NULL, 0);
    bt_aes_128_cmac_update(&state, msg4, sizeof(msg4));
    bt_aes_128_cmac_update(&state, NULL, 0);
    bt_aes_128_cmac_update(&state, msg5, sizeof(msg5));
    bt_aes_128_cmac_update(&state, NULL, 0);
    bt_aes_128_cmac_update(&state, msg6, sizeof(msg6));
    bt_aes_128_cmac_update(&state, NULL, 0);
    bt_aes_128_cmac_final(Tag, &state);
    if (memcmp(Tag, tag, BUF_LEN) != 0) {
        return -1;
    }
    return 0;
}

static int smp_aes_cmac_test(void)
{
	uint8_t mac1[] = {
		0xbb, 0x1d, 0x69, 0x29, 0xe9, 0x59, 0x37, 0x28,
		0x7f, 0xa3, 0x7d, 0x12, 0x9b, 0x75, 0x67, 0x46
	};
	uint8_t mac2[] = {
		0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44,
		0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c
	};
	uint8_t mac3[] = {
		0xdf, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30,
		0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27
	};
	uint8_t mac4[] = {
		0x51, 0xf0, 0xbe, 0xbf, 0x7e, 0x3b, 0x9d, 0x92,
		0xfc, 0x49, 0x74, 0x17, 0x79, 0x36, 0x3c, 0xfe
	};
	int err;

	err = aes_test("Test aes-cmac0", key, M, 0, mac1);
	if (err) {
		return err;
	}

	err = aes_test("Test aes-cmac16", key, M, 16, mac2);
	if (err) {
		return err;
	}

	err = aes_test("Test aes-cmac40", key, M, 40, mac3);
	if (err) {
		return err;
	}

	err = aes_test("Test aes-cmac64", key, M, 64, mac4);
	if (err) {
		return err;
	}

	return 0;
}

static int sign_test(const char *prefix, const uint8_t *sign_key, const uint8_t *m,
		     uint16_t len, const uint8_t *sig)
{
	uint8_t msg[len + sizeof(uint32_t) + 8];
	uint8_t orig[len + sizeof(uint32_t) + 8];
	uint8_t *out = msg + len;
	int err;

	LOG_DBG("%s: Sign message with len %u", prefix, len);

	(void)memset(msg, 0, sizeof(msg));
	memcpy(msg, m, len);
	(void)memset(msg + len, 0, sizeof(uint32_t));

	memcpy(orig, msg, sizeof(msg));

	err = smp_sign_buf(sign_key, msg, len);
	if (err) {
		return err;
	}

	/* Check original message */
	if (!memcmp(msg, orig, len + sizeof(uint32_t))) {
		LOG_DBG("%s: Original message intact", prefix);
	} else {
		LOG_ERR("%s: Original message modified", prefix);
		LOG_DBG("%s: orig %s", prefix, bt_hex(orig, sizeof(orig)));
		LOG_DBG("%s: msg %s", prefix, bt_hex(msg, sizeof(msg)));
		return -1;
	}

	if (!memcmp(out, sig, 12)) {
		LOG_DBG("%s: Success", prefix);
	} else {
		LOG_ERR("%s: Failed", prefix);
		return -1;
	}

	return 0;
}

static int smp_sign_test(void)
{
	const uint8_t sig1[] = {
		0x00, 0x00, 0x00, 0x00, 0xb3, 0xa8, 0x59, 0x41,
		0x27, 0xeb, 0xc2, 0xc0
	};
	const uint8_t sig2[] = {
		0x00, 0x00, 0x00, 0x00, 0x27, 0x39, 0x74, 0xf4,
		0x39, 0x2a, 0x23, 0x2a
	};
	const uint8_t sig3[] = {
		0x00, 0x00, 0x00, 0x00, 0xb7, 0xca, 0x94, 0xab,
		0x87, 0xc7, 0x82, 0x18
	};
	const uint8_t sig4[] = {
		0x00, 0x00, 0x00, 0x00, 0x44, 0xe1, 0xe6, 0xce,
		0x1d, 0xf5, 0x13, 0x68
	};
	uint8_t key_s[16];
	int err;

	/* Use the same key as aes-cmac but swap bytes */
	sys_memcpy_swap(key_s, key, 16);

	err = sign_test("Test sign0", key_s, M, 0, sig1);
	if (err) {
		return err;
	}

	err = sign_test("Test sign16", key_s, M, 16, sig2);
	if (err) {
		return err;
	}

	err = sign_test("Test sign40", key_s, M, 40, sig3);
	if (err) {
		return err;
	}

	err = sign_test("Test sign64", key_s, M, 64, sig4);
	if (err) {
		return err;
	}

	return 0;
}

static int smp_f4_test(void)
{
	uint8_t u[32] = { 0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc,
			  0xdb, 0xfd, 0xf4, 0xac, 0x11, 0x91, 0xf4, 0xef,
			  0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
			  0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20 };
	uint8_t v[32] = { 0xfd, 0xc5, 0x7f, 0xf4, 0x49, 0xdd, 0x4f, 0x6b,
			  0xfb, 0x7c, 0x9d, 0xf1, 0xc2, 0x9a, 0xcb, 0x59,
			  0x2a, 0xe7, 0xd4, 0xee, 0xfb, 0xfc, 0x0a, 0x90,
			  0x9a, 0xbb, 0xf6, 0x32, 0x3d, 0x8b, 0x18, 0x55 };
	uint8_t x[16] = { 0xab, 0xae, 0x2b, 0x71, 0xec, 0xb2, 0xff, 0xff,
			  0x3e, 0x73, 0x77, 0xd1, 0x54, 0x84, 0xcb, 0xd5 };
	uint8_t z = 0x00;
	uint8_t exp[16] = { 0x2d, 0x87, 0x74, 0xa9, 0xbe, 0xa1, 0xed, 0xf1,
			    0x1c, 0xbd, 0xa9, 0x07, 0xf1, 0x16, 0xc9, 0xf2 };
	uint8_t res[16];
	int err;

	err = bt_crypto_f4(u, v, x, z, res);
	if (err) {
		return err;
	}

	if (memcmp(res, exp, 16)) {
		return -EINVAL;
	}

	return 0;
}

static int smp_f5_test(void)
{
	uint8_t w[32] = { 0x98, 0xa6, 0xbf, 0x73, 0xf3, 0x34, 0x8d, 0x86,
			  0xf1, 0x66, 0xf8, 0xb4, 0x13, 0x6b, 0x79, 0x99,
			  0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
			  0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec };
	uint8_t n1[16] = { 0xab, 0xae, 0x2b, 0x71, 0xec, 0xb2, 0xff, 0xff,
			   0x3e, 0x73, 0x77, 0xd1, 0x54, 0x84, 0xcb, 0xd5 };
	uint8_t n2[16] = { 0xcf, 0xc4, 0x3d, 0xff, 0xf7, 0x83, 0x65, 0x21,
			   0x6e, 0x5f, 0xa7, 0x25, 0xcc, 0xe7, 0xe8, 0xa6 };
	bt_addr_le_t a1 = { .type = 0x00,
			    .a.val = { 0xce, 0xbf, 0x37, 0x37, 0x12, 0x56 } };
	bt_addr_le_t a2 = { .type = 0x00,
			    .a.val = {0xc1, 0xcf, 0x2d, 0x70, 0x13, 0xa7 } };
	uint8_t exp_ltk[16] = { 0x38, 0x0a, 0x75, 0x94, 0xb5, 0x22, 0x05,
				0x98, 0x23, 0xcd, 0xd7, 0x69, 0x11, 0x79,
				0x86, 0x69 };
	uint8_t exp_mackey[16] = { 0x20, 0x6e, 0x63, 0xce, 0x20, 0x6a, 0x3f,
				   0xfd, 0x02, 0x4a, 0x08, 0xa1, 0x76, 0xf1,
				   0x65, 0x29 };
	uint8_t mackey[16], ltk[16];
	int err;

	err = bt_crypto_f5(w, n1, n2, &a1, &a2, mackey, ltk);
	if (err) {
		return err;
	}

	if (memcmp(mackey, exp_mackey, 16) || memcmp(ltk, exp_ltk, 16)) {
		return -EINVAL;
	}

	return 0;
}

static int smp_f6_test(void)
{
	uint8_t w[16] = { 0x20, 0x6e, 0x63, 0xce, 0x20, 0x6a, 0x3f, 0xfd,
			  0x02, 0x4a, 0x08, 0xa1, 0x76, 0xf1, 0x65, 0x29 };
	uint8_t n1[16] = { 0xab, 0xae, 0x2b, 0x71, 0xec, 0xb2, 0xff, 0xff,
			   0x3e, 0x73, 0x77, 0xd1, 0x54, 0x84, 0xcb, 0xd5 };
	uint8_t n2[16] = { 0xcf, 0xc4, 0x3d, 0xff, 0xf7, 0x83, 0x65, 0x21,
			   0x6e, 0x5f, 0xa7, 0x25, 0xcc, 0xe7, 0xe8, 0xa6 };
	uint8_t r[16] = { 0xc8, 0x0f, 0x2d, 0x0c, 0xd2, 0x42, 0xda, 0x08,
			  0x54, 0xbb, 0x53, 0xb4, 0x3b, 0x34, 0xa3, 0x12 };
	uint8_t io_cap[3] = { 0x02, 0x01, 0x01 };
	bt_addr_le_t a1 = { .type = 0x00,
			    .a.val = { 0xce, 0xbf, 0x37, 0x37, 0x12, 0x56 } };
	bt_addr_le_t a2 = { .type = 0x00,
			    .a.val = {0xc1, 0xcf, 0x2d, 0x70, 0x13, 0xa7 } };
	uint8_t exp[16] = { 0x61, 0x8f, 0x95, 0xda, 0x09, 0x0b, 0x6c, 0xd2,
			    0xc5, 0xe8, 0xd0, 0x9c, 0x98, 0x73, 0xc4, 0xe3 };
	uint8_t res[16];
	int err;

	err = bt_crypto_f6(w, n1, n2, r, io_cap, &a1, &a2, res);
	if (err)
		return err;

	if (memcmp(res, exp, 16))
		return -EINVAL;

	return 0;
}

static int smp_g2_test(void)
{
	uint8_t u[32] = { 0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc,
			  0xdb, 0xfd, 0xf4, 0xac, 0x11, 0x91, 0xf4, 0xef,
			  0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
			  0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20 };
	uint8_t v[32] = { 0xfd, 0xc5, 0x7f, 0xf4, 0x49, 0xdd, 0x4f, 0x6b,
			  0xfb, 0x7c, 0x9d, 0xf1, 0xc2, 0x9a, 0xcb, 0x59,
			  0x2a, 0xe7, 0xd4, 0xee, 0xfb, 0xfc, 0x0a, 0x90,
			  0x9a, 0xbb, 0xf6, 0x32, 0x3d, 0x8b, 0x18, 0x55 };
	uint8_t x[16] = { 0xab, 0xae, 0x2b, 0x71, 0xec, 0xb2, 0xff, 0xff,
			  0x3e, 0x73, 0x77, 0xd1, 0x54, 0x84, 0xcb, 0xd5 };
	uint8_t y[16] = { 0xcf, 0xc4, 0x3d, 0xff, 0xf7, 0x83, 0x65, 0x21,
			  0x6e, 0x5f, 0xa7, 0x25, 0xcc, 0xe7, 0xe8, 0xa6 };
	uint32_t exp_val = 0x2f9ed5ba % 1000000;
	uint32_t val;
	int err;

	err = bt_crypto_g2(u, v, x, y, &val);
	if (err) {
		return err;
	}

	if (val != exp_val) {
		return -EINVAL;
	}

	return 0;
}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
static int smp_h6_test(void)
{
	uint8_t w[16] = { 0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
			  0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec };
	uint8_t key_id[4] = { 0x72, 0x62, 0x65, 0x6c };
	uint8_t exp_res[16] = { 0x99, 0x63, 0xb1, 0x80, 0xe2, 0xa9, 0xd3, 0xe8,
				0x1c, 0xc9, 0x6d, 0xe7, 0x02, 0xe1, 0x9a, 0x2d};
	uint8_t res[16];
	int err;

	err = bt_crypto_h6(w, key_id, res);
	if (err) {
		return err;
	}

	if (memcmp(res, exp_res, 16)) {
		return -EINVAL;
	}

	return 0;
}

static int smp_h7_test(void)
{
	uint8_t salt[16] = { 0x31, 0x70, 0x6d, 0x74, 0x00, 0x00, 0x00, 0x00,
			     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t w[16] = { 0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
			  0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec };
	uint8_t exp_res[16] = { 0x11, 0x70, 0xa5, 0x75, 0x2a, 0x8c, 0x99, 0xd2,
				0xec, 0xc0, 0xa3, 0xc6, 0x97, 0x35, 0x17, 0xfb};
	uint8_t res[16];
	int err;

	err = bt_crypto_h7(salt, w, res);
	if (err) {
		return err;
	}

	if (memcmp(res, exp_res, 16)) {
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_BT_CLASSIC */

static void smp_selftest_thread(void *param)
{
	int err;
	while (true)
	{
		OSA_TimeDelay(1);
		err = smp_aes_cmac_null_msg_test();
		if (0 != err) {
			LOG_ERR("SMP AES-CMAC self tests failed");
#if 0
			PRINTF("[%d]SMP AES-CMAC self tests failed\r\n", OSA_TimeGetMsec());
#endif
		}
	}
}

static int smp_self_test(void)
{
	osa_status_t ret;
	int err;

	err = smp_aes_cmac_null_msg_test();
	if (err) {
		LOG_ERR("SMP AES-CMAC self tests failed");
		return err;
	}
	err = smp_aes_cmac_128_bit_msg_test();
	if (err) {
		LOG_ERR("SMP AES-CMAC self tests failed");
		return err;
	}
	err = smp_aes_cmac_320_bit_msg_test();
	if (err) {
		LOG_ERR("SMP AES-CMAC self tests failed");
		return err;
	}
	err = smp_aes_cmac_512_bit_msg_test();
	if (err) {
		LOG_ERR("SMP AES-CMAC self tests failed");
		return err;
	}

	err = smp_aes_cmac_test();
	if (err) {
		LOG_ERR("SMP AES-CMAC self tests failed");
		return err;
	}

	err = smp_sign_test();
	if (err) {
		LOG_ERR("SMP signing self tests failed");
		return err;
	}

	err = smp_f4_test();
	if (err) {
		LOG_ERR("SMP f4 self test failed");
		return err;
	}

	err = smp_f5_test();
	if (err) {
		LOG_ERR("SMP f5 self test failed");
		return err;
	}

	err = smp_f6_test();
	if (err) {
		LOG_ERR("SMP f6 self test failed");
		return err;
	}

	err = smp_g2_test();
	if (err) {
		LOG_ERR("SMP g2 self test failed");
		return err;
	}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	err = smp_h6_test();
	if (err) {
		LOG_ERR("SMP h6 self test failed");
		return err;
	}

	err = smp_h7_test();
	if (err) {
		LOG_ERR("SMP h7 self test failed");
		return err;
	}
#endif /* CONFIG_BT_CLASSIC */

	/* Create a task for periodic test */
	ret = OSA_TaskCreate((osa_task_handle_t)smp_selftest_data, OSA_TASK(smp_selftest_thread), NULL);
	assert(KOSA_StatusSuccess == ret);

	(void)ret;
	return 0;
}
#else
static inline int smp_self_test(void)
{
	return 0;
}
#endif
#if (defined(CONFIG_BT_BONDABLE_PER_CONNECTION) && ((CONFIG_BT_BONDABLE_PER_CONNECTION) > 0U))
int bt_conn_set_bondable(struct bt_conn *conn, bool enable)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (atomic_cas(&smp->bondable, BT_SMP_BONDABLE_UNINITIALIZED, (atomic_val_t)enable)) {
		return 0;
	} else {
		return -EALREADY;
	}
}
#endif

int bt_smp_auth_cb_overlay(struct bt_conn *conn, const struct bt_conn_auth_cb *cb)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (atomic_ptr_cas(&smp->auth_cb, BT_SMP_AUTH_CB_UNINITIALIZED, (atomic_ptr_val_t)cb)) {
		return 0;
	} else {
		return -EALREADY;
	}
}

#if (defined(CONFIG_BT_PASSKEY_KEYPRESS) && (CONFIG_BT_PASSKEY_KEYPRESS > 0))
static int smp_send_keypress_notif(struct bt_smp *smp, uint8_t type)
{
	struct bt_smp_keypress_notif *req;
	struct net_buf *buf;

	buf = smp_create_pdu(smp, BT_SMP_KEYPRESS_NOTIFICATION, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->type = type;

	smp_send(smp, buf, NULL, NULL);

	return 0;
}
#endif
#if (defined(CONFIG_BT_PASSKEY_KEYPRESS) && (CONFIG_BT_PASSKEY_KEYPRESS > 0))
int bt_smp_auth_keypress_notify(struct bt_conn *conn, enum bt_conn_auth_keypress type)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}
	CHECKIF(!IN_RANGE(type,
			  BT_CONN_AUTH_KEYPRESS_ENTRY_STARTED,
			  BT_CONN_AUTH_KEYPRESS_ENTRY_COMPLETED)) {
		LOG_ERR("Refusing to send unknown event type %u", type);
		return -EINVAL;
	}

	if (smp->method != PASSKEY_INPUT ||
	    !atomic_test_bit(smp->flags, SMP_FLAG_USER)) {
		LOG_ERR("Refusing to send keypress: Not waiting for passkey input.");
		return -EINVAL;
	}

	return smp_send_keypress_notif(smp, type);
}
#endif
int bt_smp_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}

	smp->passkey = sys_cpu_to_le32(passkey);

	(void)BT_smp_passkey_entry_request_reply
			(
				&conn->deviceId,
				&smp->passkey,
				(0 == passkey)? SMP_FALSE : SMP_TRUE
			);

	return 0;
}

int bt_smp_auth_passkey_confirm(struct bt_conn *conn)
{
	struct bt_smp *smp;
    UCHAR accept;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (CONFIRM_TYPE_PASSKEY != smp->confirm_type)
	{
		return -EINVAL;
	}
	smp->confirm_type = CONFIRM_TYPE_NONE;

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}

	accept = SMP_NUM_COMP_CNF_POSITIVE;
	(void)BT_smp_nkey_comp_cnf_reply
				(
					&conn->deviceId,
					(void *)&accept
				);
	return 0;
}

#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
int bt_smp_le_oob_set_tk(struct bt_conn *conn, const uint8_t *tk)
{
	SMP_BD_ADDR bdaddr;
	API_RESULT retval;
	SMP_OOB_DATA oob;
	struct bt_smp *smp;
	uint8_t oob_flag;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	LOG_DBG("%s", bt_hex(tk, 16));

	memset(&oob, 0, sizeof(SMP_OOB_DATA));
	memcpy(bdaddr.addr, conn->le.dst.a.val, sizeof(bdaddr.addr));
	bdaddr.type = conn->le.dst.type;

	retval = BT_smp_get_oob_data_pl(&bdaddr, &oob_flag, &oob, NULL, NULL);
	if (API_SUCCESS != retval)
	{
		retval = BT_smp_add_device_pl(&bdaddr);
		if (API_SUCCESS != retval)
		{
			return -EIO;
		}
	}

	if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
		uint8_t oob[16];

		sys_memcpy_swap(oob, tk, 16);
		LOG_INF("Legacy OOB data 0x%s", bt_hex(oob, 16));
	}

	memcpy(smp->tk, tk, 16*sizeof(uint8_t));

	memcpy(oob.temp_key, tk, sizeof(oob.temp_key));
	retval = BT_smp_set_oob_data_pl (&bdaddr, (UCHAR)1, &oob, NULL, NULL);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}

#if 0
	legacy_user_tk_entry(smp);
#endif
	return 0;
}
#endif /* !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) */

static void bt_smp_le_oob_generate_complete(SMP_LESC_OOB_DATA_PL * lesc_oob)
{
	memcpy(current_oob_data.r, lesc_oob->rand, sizeof(current_oob_data.r));
	memcpy(current_oob_data.c, lesc_oob->cnf_val, sizeof(current_oob_data.c));
	OSA_SemaphorePost(sc_local_oobe_ready);
}

int bt_smp_le_oob_generate_sc_data(struct bt_le_oob_sc_data *le_sc_oob)
{
	int err;

	if (!le_sc_supported()) {
		return -ENOTSUP;
	}

	(void)BT_smp_generate_lesc_oob_local_data_pl((SMP_LESC_OOB_GEN_COMPLETE_CB) bt_smp_le_oob_generate_complete);

	err = OSA_SemaphoreWait(sc_local_oobe_ready, osaWaitForever_c);
	if (err) {
		return err;
	}

	memcpy(le_sc_oob->r, current_oob_data.r, sizeof(le_sc_oob->r));
	memcpy(le_sc_oob->c, current_oob_data.c, sizeof(le_sc_oob->c));
	(void)sc_public_key;
#if 0
	if (!sc_public_key) {
		err = OSA_SemaphoreWait(sc_local_pkey_ready, osaWaitForever_c);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_OOB_DATA_FIXED)) {
		uint8_t rand_num[] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		};

		memcpy(le_sc_oob->r, rand_num, sizeof(le_sc_oob->r));
	} else {
		err = bt_rand(le_sc_oob->r, 16);
		if (err) {
			return err;
		}
	}

	err = bt_crypto_f4(sc_public_key, sc_public_key, le_sc_oob->r, 0,
		     le_sc_oob->c);
	if (err) {
		return err;
	}
#endif
	return 0;
}

#if !(defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) && (CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY > 0))

static bool le_sc_oob_data_check(struct bt_smp *smp, bool oobd_local_present,
				 bool oobd_remote_present)
{
	bool req_oob_present = le_sc_oob_data_req_check(smp);
	bool rsp_oob_present = le_sc_oob_data_rsp_check(smp);

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		if ((req_oob_present != oobd_remote_present) &&
		    (rsp_oob_present != oobd_local_present)) {
			return false;
		}
	} else if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		if ((req_oob_present != oobd_local_present) &&
		    (rsp_oob_present != oobd_remote_present)) {
			return false;
		}
	}

	return true;
}

#if 0
static int le_sc_oob_pairing_continue(struct bt_smp *smp)
{
	if (smp->oobd_remote) {
		int err;
		uint8_t c[16];

		err = bt_crypto_f4(smp->pkey, smp->pkey, smp->oobd_remote->r, 0, c);
		if (err) {
			return err;
		}

		bool match = (memcmp(c, smp->oobd_remote->c, sizeof(c)) == 0);

		if (!match) {
			smp_error(smp, BT_SMP_ERR_CONFIRM_FAILED);
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    smp->chan.chan.conn->role == BT_HCI_ROLE_CENTRAL) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PAIRING_RANDOM);
	} else if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		atomic_set_bit(smp->allowed_cmds, BT_SMP_DHKEY_CHECK);
		atomic_set_bit(smp->flags, SMP_FLAG_DHCHECK_WAIT);
	}

	return smp_send_pairing_random(smp);
}
#endif
int bt_smp_le_oob_set_sc_data(struct bt_conn *conn,
			      const struct bt_le_oob_sc_data *oobd_local,
			      const struct bt_le_oob_sc_data *oobd_remote)
{
#if 0
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!le_sc_oob_data_check(smp, (oobd_local != NULL),
				  (oobd_remote != NULL))) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_OOB_PENDING)) {
		return -EINVAL;
	}

	smp->oobd_local = oobd_local;
	smp->oobd_remote = oobd_remote;

	return le_sc_oob_pairing_continue(smp);
#else
	SMP_BD_ADDR bdaddr;
    API_RESULT retval;
	SMP_OOB_DATA oob;
	struct bt_smp *smp;
	uint8_t oob_flag;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	memset(&oob, 0, sizeof(SMP_OOB_DATA));
	memcpy(bdaddr.addr, conn->le.dst.a.val, sizeof(bdaddr.addr));
	bdaddr.type = conn->le.dst.type;

	retval = BT_smp_get_oob_data_pl(&bdaddr, &oob_flag, &oob, NULL, NULL);
	if (API_SUCCESS != retval)
	{
		retval = BT_smp_add_device_pl(&bdaddr);
		if (API_SUCCESS != retval)
		{
			return -EIO;
		}
	}

	memcpy(oob.lesc_cnf_val, oobd_remote->c, sizeof(oob.lesc_cnf_val));
	memcpy(oob.lesc_rand, oobd_remote->r, sizeof(oob.lesc_rand));
	retval = BT_smp_set_oob_data_pl (&bdaddr, (UCHAR)1, &oob, NULL, NULL);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}

	if (!le_sc_oob_data_check(smp, (oobd_local != NULL),
				  (oobd_remote != NULL))) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_OOB_PENDING)) {
		return -EINVAL;
	}

	smp->oobd_local = oobd_local;
	smp->oobd_remote = oobd_remote;

    return 0;
#endif
}

int bt_smp_le_oob_get_sc_data(struct bt_conn *conn,
			      const struct bt_le_oob_sc_data **oobd_local,
			      const struct bt_le_oob_sc_data **oobd_remote)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!smp->oobd_local && !smp->oobd_remote) {
		return -ESRCH;
	}

	if (oobd_local) {
		*oobd_local = smp->oobd_local;
	}

	if (oobd_remote) {
		*oobd_remote = smp->oobd_remote;
	}

	return 0;
}
#endif /* !CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY */

int bt_smp_auth_cancel(struct bt_conn *conn)
{
	struct bt_smp *smp;
	API_RESULT retval;
	CHAR accept;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}

	LOG_DBG("");

	switch (smp->method) {
	case PASSKEY_CONFIRM:
		accept = SMP_NUM_COMP_CNF_NEGATIVE;
		retval = BT_smp_nkey_comp_cnf_reply
			(
				&conn->deviceId,
				(void *)&accept
			);
		return (API_SUCCESS == retval) ? 0 : -EIO;
	case PASSKEY_INPUT:
	case PASSKEY_DISPLAY:
		smp->auth.param = BT_SMP_ERR_PASSKEY_ENTRY_FAILED;
	case LE_SC_OOB:
	case LEGACY_OOB:
		smp->auth.param = BT_SMP_ERR_OOB_NOT_AVAIL;
	case JUST_WORKS:
		smp->auth.param = BT_SMP_ERR_UNSPECIFIED;
		retval = BT_smp_authentication_request_reply
                  (
                      &conn->deviceId,
                      &smp->auth
                  );
		return (API_SUCCESS == retval) ? 0 : -EIO;
	default:
		return 0;
	}
}

#if !(defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0))
int bt_smp_auth_pairing_confirm(struct bt_conn *conn)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (CONFIRM_TYPE_PAIRING != smp->confirm_type)
	{
		return -EINVAL;
	}

	smp->confirm_type = CONFIRM_TYPE_NONE;

	if (!atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER)) {
		return -EINVAL;
	}
#if 0
	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_CONN_ROLE_CENTRAL) {
		if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
			atomic_set_bit(smp->allowed_cmds,
				       BT_SMP_CMD_PAIRING_CONFIRM);
			return legacy_send_pairing_confirm(smp);
		}

		if (!sc_public_key) {
			atomic_set_bit(smp->flags, SMP_FLAG_PKEY_SEND);
			return 0;
		}

		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
		return sc_send_public_key(smp);
	}
#endif

#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
	if (!atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
#if 0
		atomic_set_bit(smp->allowed_cmds,
			       BT_SMP_CMD_PAIRING_CONFIRM);
#endif
		return send_pairing_rsp(smp);
	}
#if 0
	atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_PUBLIC_KEY);
#endif
	if (send_pairing_rsp(smp)) {
		return -EIO;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return 0;
}
#else
int bt_smp_auth_pairing_confirm(struct bt_conn *conn)
{
	/* confirm_pairing will never be called in LE SC only mode */
	return -EINVAL;
}
#endif /* !CONFIG_BT_SMP_SC_PAIR_ONLY */

#if (defined(CONFIG_BT_FIXED_PASSKEY) && ((CONFIG_BT_FIXED_PASSKEY) > 0U))
int bt_passkey_set(unsigned int passkey)
{
	if (passkey == BT_PASSKEY_INVALID) {
		fixed_passkey = BT_PASSKEY_INVALID;
		return 0;
	}

	if (passkey > 999999) {
		return -EINVAL;
	}

	fixed_passkey = passkey;
	return 0;
}
#endif /* CONFIG_BT_FIXED_PASSKEY */

int bt_smp_start_security(struct bt_conn *conn)
{
#if 0
	switch (conn->role) {
#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0U))
	case BT_HCI_ROLE_CENTRAL:
	{
		int err;
		struct bt_smp *smp;

		smp = smp_chan_get(conn);
		if (!smp) {
			return -ENOTCONN;
		}

		/* pairing is in progress */
		if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
			return -EBUSY;
		}

		/* Encryption is in progress */
		if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
			return -EBUSY;
		}

		if (!smp_keys_check(conn)) {
			return smp_send_pairing_req(conn);
		}

		/* LE SC LTK and legacy central LTK are stored in same place */
		err = bt_conn_le_start_encryption(conn,
						  conn->le.keys->ltk.rand,
						  conn->le.keys->ltk.ediv,
						  conn->le.keys->ltk.val,
						  conn->le.keys->enc_size);
		if (err) {
			return err;
		}

		atomic_set_bit(smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		return 0;
	}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_SMP */
#if (defined(CONFIG_BT_PERIPHERAL) && ((CONFIG_BT_PERIPHERAL) > 0U))
	case BT_HCI_ROLE_PERIPHERAL:
		return smp_send_security_req(conn);
#endif /* CONFIG_BT_PERIPHERAL && CONFIG_BT_SMP */
	default:
		return -EINVAL;
	}
#else
	struct bt_smp *smp;
    SMP_AUTH_INFO auth;
    API_RESULT    retval;

	LOG_DBG("");
	smp = smp_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	/* pairing is in progress */
	if (atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	if (atomic_test_bit(smp->flags, SMP_FLAG_ENC_PENDING)) {
		return -EBUSY;
	}

	/* early verify if required sec level if reachable */
	if (!(sec_level_reachable(smp) || smp_keys_check(conn))) {
		return -EINVAL;
	}

	if (BT_SECURITY_L0 == conn->required_sec_level)
	{
		return -EINVAL;
	}
    memset(&auth, 0, sizeof(auth));
	/* fix GAP/SEC/AUT/BV-21-C */
	if ((smp_keys_check(conn)) && (!(conn->le.keys->flags & BT_KEYS_SC)))
	{
		auth.pair_mode = SMP_LEGACY_MODE;
	}
	else
	{
	    auth.pair_mode = SMP_LESC_MODE;
	}
	auth.security = ((uint8_t)conn->required_sec_level) - 1u;
    if (SMP_SEC_LEVEL_3 == auth.security)
    {
        auth.security = SMP_SEC_LEVEL_2;
    }
    /* according to get_auth function, add follow codes to set MITM.
     * In Ethermind, when security == SMP_SEC_LEVEL_2, MITM is set.
    */
    if ((get_io_capa(smp) == BT_SMP_IO_NO_INPUT_OUTPUT) ||
        (!IS_ENABLED(CONFIG_BT_SMP_ENFORCE_MITM) &&
        (conn->required_sec_level < BT_SECURITY_L3))) {
    } else {
            auth.security = SMP_SEC_LEVEL_2;
    }
	auth.bonding = ((BT_SMP_AUTH_BONDING_FLAGS & BT_SMP_AUTH_BONDING) && (true == bondable)) ? SMP_BONDING : SMP_BONDING_NONE;
	/* fix SM/CEN/EKS/BV-01-C */
	auth.ekey_size = BT_SMP_MIN_ENC_KEY_SIZE;
#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
    auth.transport = SMP_LINK_LE;
    auth.xtx_info = SMP_XTX_H7_MASK;
#endif

    if(sc_oobd_present || legacy_oobd_present)
	{
		if (bt_auth && bt_auth->oob_data_request) {
			struct bt_conn_oob_info info = {
				.type = BT_CONN_OOB_LE_SC,
				.lesc.oob_config = BT_CONN_OOB_BOTH_PEERS,
			};

			smp->oobd_local = NULL;
			smp->oobd_remote = NULL;

			atomic_set_bit(smp->flags, SMP_FLAG_OOB_PENDING);
			bt_auth->oob_data_request(conn, &info);
		}
	}

	retval = BT_smp_authenticate
			(
				&conn->deviceId,
				&auth
			);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		LOG_DBG("start authenticate");
		atomic_set_bit(smp->flags, SMP_FLAG_PAIRING);
		atomic_set_bit(smp->flags, SMP_FLAG_ENC_PENDING);
		atomic_set_bit(smp->flags, SMP_FLAG_SEC_REQ);
		return 0;
	}
#endif
}

void bt_smp_update_keys(struct bt_conn *conn)
{
	struct bt_smp *smp;

	smp = smp_chan_get(conn);
	if (!smp) {
		return;
	}

	if (!atomic_test_bit(smp->flags, SMP_FLAG_PAIRING)) {
		return;
	}

	/*
	 * If link was successfully encrypted cleanup old keys as from now on
	 * only keys distributed in this pairing or LTK from LE SC will be used.
	 */
	if (conn->le.keys) {
		bt_keys_clear(conn->le.keys);
	}

	conn->le.keys = bt_keys_get_addr(conn->id, &conn->le.dst);
	if (!conn->le.keys) {
		LOG_ERR("Unable to get keys for %s",
		       bt_addr_le_str(&conn->le.dst));
#if 0
		smp_error(smp, BT_SMP_ERR_UNSPECIFIED);
#endif
		return;
	}

    bt_smp_get_auth_info(conn);

	/* mark keys as debug */
	if (atomic_test_bit(smp->flags, SMP_FLAG_SC_DEBUG_KEY)) {
		conn->le.keys->flags |= BT_KEYS_DEBUG;
	}
#if 0
	/*
	 * store key type deducted from pairing method used
	 * it is important to store it since type is used to determine
	 * security level upon encryption
	 */
	switch (smp->method) {
	case LE_SC_OOB:
	case LEGACY_OOB:
		conn->le.keys->flags |= BT_KEYS_OOB;
		/* fallthrough */
	case PASSKEY_DISPLAY:
	case PASSKEY_INPUT:
	case PASSKEY_CONFIRM:
		conn->le.keys->flags |= BT_KEYS_AUTHENTICATED;
		break;
	case JUST_WORKS:
	default:
		/* unauthenticated key, clear it */
		conn->le.keys->flags &= ~BT_KEYS_OOB;
		conn->le.keys->flags &= ~BT_KEYS_AUTHENTICATED;
		break;
	}

	conn->le.keys->enc_size = get_encryption_key_size(smp);

	/*
	 * Store LTK if LE SC is used, this is safe since LE SC is mutually
	 * exclusive with legacy pairing. Other keys are added on keys
	 * distribution.
	 */
	if (atomic_test_bit(smp->flags, SMP_FLAG_SC)) {
		conn->le.keys->flags |= BT_KEYS_SC;

		if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
			bt_keys_add_type(conn->le.keys, BT_KEYS_LTK_P256);
			memcpy(conn->le.keys->ltk.val, smp->tk,
			       sizeof(conn->le.keys->ltk.val));
			(void)memset(conn->le.keys->ltk.rand, 0,
				     sizeof(conn->le.keys->ltk.rand));
			(void)memset(conn->le.keys->ltk.ediv, 0,
				     sizeof(conn->le.keys->ltk.ediv));
		} else if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
			uint8_t ltk[16];

			sys_memcpy_swap(ltk, smp->tk, conn->le.keys->enc_size);
			LOG_INF("SC LTK: 0x%s (No bonding)",
				bt_hex(ltk, conn->le.keys->enc_size));
		}
	} else {
		conn->le.keys->flags &= ~BT_KEYS_SC;
	}
#endif
}

static int bt_smp_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_smp_connected,
		.disconnected = bt_smp_disconnected,
		.encrypt_change = bt_smp_encrypt_change,
		.recv = NULL,
	};

	LOG_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		struct bt_smp *smp = &bt_smp_pool[i];

		if (smp->chan.chan.conn) {
			continue;
		}

		smp->chan.chan.ops = &ops;

		*chan = &smp->chan.chan;

		return 0;
	}

	LOG_ERR("No available SMP context for conn %p", conn);

	return -ENOMEM;
}

BT_L2CAP_CHANNEL_DEFINE(smp_fixed_chan, BT_L2CAP_CID_SMP, bt_smp_accept, NULL);

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
BT_L2CAP_BR_CHANNEL_DEFINE(smp_br_fixed_chan, BT_L2CAP_CID_BR_SMP, bt_smp_br_accept);
#endif /* CONFIG_BT_CLASSIC */

#if LOG_ENABLE
static char *ethermind_bt_smp_event_get_name(UCHAR event)
{
    char *name = NULL;
    switch (event)
    {
    case SMP_AUTHENTICATION_COMPLETE:
      name = "SMP_AUTHENTICATION_COMPLETE";
      break;
    case SMP_AUTHENTICATION_REQUEST:
      name = "SMP_AUTHENTICATION_REQUEST";
      break;
    case SMP_PASSKEY_ENTRY_REQUEST:
      name = "SMP_PASSKEY_ENTRY_REQUEST";
      break;
    case SMP_PASSKEY_DISPLAY_REQUEST:
      name = "SMP_PASSKEY_DISPLAY_REQUEST";
      break;
    case SMP_LONG_TERM_KEY_REQUEST:
      name = "SMP_LONG_TERM_KEY_REQUEST";
      break;
    case SMP_KEY_EXCHANGE_INFO_REQUEST:
      name = "SMP_KEY_EXCHANGE_INFO_REQUEST";
      break;
    case SMP_KEY_EXCHANGE_INFO:
      name = "SMP_KEY_EXCHANGE_INFO";
      break;
    case SMP_RESOLVABLE_PVT_ADDR_CREATE_CNF:
      name = "SMP_RESOLVABLE_PVT_ADDR_CREATE_CNF";
      break;
    case SMP_RESOLVABLE_PVT_ADDR_VERIFY_CNF:
      name = "SMP_RESOLVABLE_PVT_ADDR_VERIFY_CNF";
      break;
    case SMP_DATA_SIGNING_COMPLETE:
      name = "SMP_DATA_SIGNING_COMPLETE";
      break;
    case SMP_SIGN_DATA_VERIFICATION_COMPLETE:
      name = "SMP_SIGN_DATA_VERIFICATION_COMPLETE";
      break;
    case SMP_NUMERIC_KEY_COMPARISON_CNF_REQUEST:
      name = "SMP_NUMERIC_KEY_COMPARISON_CNF_REQUEST";
      break;
    case SMP_KEY_PRESS_NOTIFICATION_EVENT:
      name = "SMP_KEY_PRESS_NOTIFICATION_EVENT";
      break;
    default:
      name = "UNKNOWN";
      break;
    }
    return name;
}
#endif

static void bt_smp_get_auth_info(struct bt_conn *conn)
{
	struct bt_smp *smp;

	SMP_AUTH_INFO le_auth_info;
	SMP_KEY_DIST  p_key_info;
    UCHAR         p_keys;

	API_RESULT retval;

    smp = smp_chan_get(conn);

	LOG_DBG("update auth info smp %p", smp);

	retval = BT_smp_get_device_security_info (&conn->deviceId, &le_auth_info);
	if (API_SUCCESS == retval)
	{
		if ((NULL != conn->le.keys) && (0 == conn->le.keys->keys))
		{
			if (SMP_SEC_LEVEL_2 == (le_auth_info.security & 0x0F))
			{
				conn->le.keys->flags |= BT_KEYS_AUTHENTICATED;
			}
			conn->le.keys->enc_size = le_auth_info.ekey_size;
			if (SMP_LESC_MODE == le_auth_info.pair_mode)
			{
				conn->le.keys->flags |= BT_KEYS_SC;
                atomic_set_bit(smp->flags, SMP_FLAG_SC);
			}
			else
			{
                atomic_clear_bit(smp->flags, SMP_FLAG_SC);
				conn->le.keys->flags &= ~BT_KEYS_SC;
			}

            if (SMP_TRUE == le_auth_info.bonding)
            {
                atomic_set_bit(smp->flags, SMP_FLAG_BOND);
            }

			/* Check if the link is authenticated */
			if ((SMP_ENTITY_AUTH_ON == le_auth_info.param) &&
				(SMP_TRUE == le_auth_info.bonding))
			{
                retval = BT_smp_get_device_keys
                            (
                                &conn->deviceId,
                                &p_keys,
                                &p_key_info
                            );

                if (API_SUCCESS == retval)
                {
                    const bt_addr_le_t *dst;
                    bt_addr_le_t id_addr;
#if (defined(CONFIG_BT_SIGNING) && (CONFIG_BT_SIGNING> 0))
                    SMP_KEY_DIST * local_key_info;
#endif
                    if (BT_SMP_MAX_ENC_KEY_SIZE == le_auth_info.ekey_size)
                    {
                        /* SC pairing */
                        bt_keys_add_type(conn->le.keys, BT_KEYS_LTK_P256);
                        LOG_DBG("SC pairing");
                    }
                    else
                    {
                        /* Legacy pairing */
                        bt_keys_add_type(conn->le.keys, BT_KEYS_LTK);
                        LOG_DBG("Legacy pairing");
                    }
#if 0
					if (BT_SMP_KEYS_REMOTE_ENCKEY & p_keys)
#endif
					{
						LOG_DBG("Add LTK");
						memcpy(conn->le.keys->ltk.val, p_key_info.enc_info,
								MIN(sizeof(conn->le.keys->ltk.val), le_auth_info.ekey_size));
						memcpy(conn->le.keys->ltk.rand, &p_key_info.mid_info[2],
								sizeof(conn->le.keys->ltk.rand));
						memcpy(conn->le.keys->ltk.ediv, &p_key_info.mid_info[0],
								sizeof(conn->le.keys->ltk.ediv));
						smp->local_dist &= ~BT_SMP_DIST_ENC_KEY;
						smp->remote_dist &= ~BT_SMP_DIST_ENC_KEY;
					}
					if (BT_SMP_KEYS_REMOTE_IDKEY & p_keys)
					{
						bt_keys_add_type(conn->le.keys, BT_KEYS_IRK);
						LOG_DBG("Add IRK");
						memcpy(conn->le.keys->irk.val, p_key_info.id_info,
								sizeof(conn->le.keys->irk.val));
						id_addr.type = p_key_info.id_addr_info[0];
						memcpy(conn->le.keys->irk.rpa.val, &p_key_info.id_addr_info[1],
								sizeof(conn->le.keys->irk.rpa.val));
						memcpy(&id_addr.a.val[0], &p_key_info.id_addr_info[1],
								sizeof(id_addr.a.val));
						smp->local_dist &= ~BT_SMP_DIST_ID_KEY;
						smp->remote_dist &= ~BT_SMP_DIST_ID_KEY;
					}
#if (defined(CONFIG_BT_SIGNING) && (CONFIG_BT_SIGNING> 0))
                    if (API_SUCCESS == BT_smp_get_key_exchange_info_pl(&local_key_info))
                    {
						LOG_DBG("Add Local CSRK");
                        bt_keys_add_type(conn->le.keys, BT_KEYS_LOCAL_CSRK);
                        memcpy(conn->le.keys->local_csrk.val, &local_key_info->sign_info[0],
                                sizeof(conn->le.keys->local_csrk.val));
                        conn->le.keys->local_csrk.cnt = 0;
						smp->local_dist &= ~BT_SMP_DIST_SIGN;
                    }
					if (BT_SMP_KEYS_REMOTE_SIGNKEY & p_keys)
					{
						LOG_DBG("Add Remote CSRK");
						bt_keys_add_type(conn->le.keys, BT_KEYS_REMOTE_CSRK);
						memcpy(conn->le.keys->remote_csrk.val, &p_key_info.sign_info[0],
								sizeof(conn->le.keys->remote_csrk.val));
						conn->le.keys->remote_csrk.cnt = 0;
						smp->remote_dist &= ~BT_SMP_DIST_SIGN;
					}
#endif
					/*
					* We can't use conn->dst here as this might already contain
					* identity address known from previous pairing. Since all keys
					* are cleared on re-pairing we wouldn't store IRK distributed
					* in new pairing.
					*/
					if (conn->role == BT_HCI_ROLE_CENTRAL) {
						dst = &conn->le.resp_addr;
					} else {
						dst = &conn->le.init_addr;
					}

					if (bt_addr_le_is_rpa(dst)) {
						/* always update last use RPA */
						bt_addr_copy(&conn->le.keys->irk.rpa, &dst->a);

						/*
						* Update connection address and notify about identity
						* resolved only if connection wasn't already reported
						* with identity address. This may happen if IRK was
						* present before ie. due to re-pairing.
						*/
						if (!bt_addr_le_is_identity(&conn->le.dst)) {
							bt_addr_le_copy(&conn->le.keys->addr, &id_addr);
							bt_addr_le_copy(&conn->le.dst, &id_addr);

							bt_conn_identity_resolved(conn);
						}
					}

					bt_keys_store(conn->le.keys);
					if (BT_SMP_KEYS_REMOTE_IDKEY & p_keys)
					{
						/* bt_id_add moved to a delay task to previent HCI sync command block this task context,
							which will affect security level update timing. */
						k_work_schedule(&smp->id_add, BT_MSEC(1));
					}
				}
			}
		}
	}
}

static void smp_auth_starting(struct bt_smp *smp)
{
	struct bt_conn *conn;
	int ret;
	API_RESULT retval;

	conn = smp->chan.chan.conn;

	if (BT_HCI_ROLE_CENTRAL == conn->role)
	{
#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0U))
		ret = smp_security_request(smp, &smp->auth);
#else
		ret = 0;
#endif
	}
	else
	{
		uint8_t preq[7];
		retval = BT_smp_get_pairing_req_pdu((SMP_BD_HANDLE *)&conn->deviceId, (UCHAR *)&preq[0]);
		if (API_SUCCESS == retval)
		{
			ret = smp_pairing_req(smp, (struct bt_smp_pairing*)&preq[1], &smp->auth);
		}
		else
		{
			ret = -1;
		}
	}

	if (0 != ret)
	{
		smp->auth.param = ret;
		retval = BT_smp_authentication_request_reply
		(
			(SMP_BD_HANDLE *)&conn->deviceId,
			&smp->auth
		);
	}
}

#ifdef SMP_LESC
#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
void appl_smp_lesc_xtxp_ltk_complete(SMP_LESC_LK_LTK_GEN_PL * xtxp)
{
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
    API_RESULT retval;
    SMP_BD_HANDLE bd_handle;
    SMP_AUTH_INFO auth_info;
    UCHAR lkey[BT_LINK_KEY_SIZE];
    UCHAR lkey_type;
    struct bt_conn *conn;
    bt_addr_le_t peer_addr;
    struct bt_keys *keys;
    struct bt_smp_br *smp;
    DEVICE_HANDLE deviceHandle;
    UCHAR peer_keys;
    UCHAR di;

    LOG_DBG("\n LTK of the device is ...\n");
    LOG_DBG("\n LK of the device is ...\n");

    retval = device_queue_search_br_edr_remote_addr(&deviceHandle, &bt_smp_bd_addr);
    if (API_SUCCESS != retval)
    {
        LOG_ERR("The address cannot be found");
        return;
    }

    conn = bt_conn_lookup_device_id(deviceHandle);
    if (NULL == conn)
    {
        LOG_ERR("Connect is not found, invalid bd handle 0x%02X", deviceHandle);
        return;
    }

    bt_conn_unref(conn);

    smp = smp_br_chan_get(conn);
    if (smp == NULL)
    {
        LOG_ERR("SMP of conn %p cannot be found", conn);
        return;
    }

    retval = BT_sm_get_device_link_key_and_type(bt_smp_bd_addr.addr, lkey, &lkey_type);

    if ((API_SUCCESS == retval) &&
        ((HCI_LINK_KEY_AUTHENTICATED_P_256 == lkey_type) ||
        (HCI_LINK_KEY_UNAUTHENTICATED_P_256 == lkey_type)))
    {
        retval = BT_smp_search_identity_addr(&bt_smp_bd_addr, DQ_LE_LINK, &bd_handle);
        if (API_SUCCESS != retval)
        {
            (BT_IGNORE_RETURN_VALUE)BT_smp_add_device(&bt_smp_bd_addr, &bd_handle);
        }

        auth_info.bonding = atomic_test_bit(smp->flags, SMP_FLAG_BOND) ? SMP_BONDING : SMP_BONDING_NONE;
        auth_info.pair_mode = SMP_LESC_MODE;
        auth_info.security = (HCI_LINK_KEY_AUTHENTICATED_P_256 == lkey_type)?
            SMP_SEC_LEVEL_2: SMP_SEC_LEVEL_1;

        /* Update the keys */
        BT_smp_get_device_keys(&deviceHandle, &peer_keys, &peer_key_info);
        BT_mem_copy(peer_key_info.enc_info, xtxp->ltk, 16U);
        (BT_IGNORE_RETURN_VALUE)BT_smp_update_security_info
        (
            &bd_handle,
            &auth_info,
            16U,
            local_keys,
            peer_keys,
            &peer_key_info
        );

	/* Search device index */
	di = smp_search_device (&bd_handle, SMP_L2CAP_INVALID_SIG_ID);

	/* If device not found in database */
	if(SMP_MAX_DEVICES != di)
	{
		/* Lock SMP */
		smp_lock();
		smp_update_device_attr_pl(SMP_DEVICE_ATTR_PL_AUTHENTICATION_COMPLETE, di);
		/* Unlock SMP */
		smp_unlock();
	}

	bt_addr_copy(&peer_addr.a, &conn->br.dst);
	peer_addr.type = BT_ADDR_LE_PUBLIC;

	keys = bt_keys_get_type(BT_KEYS_LTK, conn->id, &peer_addr);
	if (!keys)
	{
		LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&peer_addr));
		return;
	}
	memcpy(keys->ltk.val, xtxp->ltk, sizeof(keys->ltk.val));

	if (lkey_type == HCI_LINK_KEY_AUTHENTICATED_P_256) {
		keys->flags |= BT_KEYS_AUTHENTICATED;
	} else {
		keys->flags &= ~BT_KEYS_AUTHENTICATED;
	}

	k_work_cancel_delayable(&smp->auth_timeout);
	smp_br_auth_complete(smp);
    }
#endif
}
void appl_smp_lesc_xtxp_lk_complete(SMP_LESC_LK_LTK_GEN_PL * xtxp)
{
    API_RESULT retval;
    SMP_BD_HANDLE bd_handle;
    SMP_AUTH_INFO auth;
    UCHAR type;

    LOG_DBG("\n LK of the device is ...\n");

    LOG_DBG("\n LTK of the device is ...\n");

    /* Update the LinkKey. Key strength checked before generation */
    LOG_DBG("Adding Device to Device SM DB .. %02X:%02X:%02X:%02X:%02X:%02X\n",
    bt_smp_bd_addr.addr[0], bt_smp_bd_addr.addr[1], bt_smp_bd_addr.addr[2],
    bt_smp_bd_addr.addr[3], bt_smp_bd_addr.addr[4], bt_smp_bd_addr.addr[5]);

    /* Get the BD handle */
    (BT_IGNORE_RETURN_VALUE)BT_smp_get_bd_handle(&bt_smp_bd_addr, &bd_handle);

    /* Initialize */
    BT_mem_set(&auth, 0x00, sizeof(SMP_AUTH_INFO));

    retval = BT_smp_get_device_security_info
             (
                 &bd_handle,
                 &auth
             );
    if (API_SUCCESS == retval)
    {
        type = (SMP_SEC_LEVEL_2 == auth.security) ?
            HCI_LINK_KEY_AUTHENTICATED_P_256 : HCI_LINK_KEY_UNAUTHENTICATED_P_256;

        (BT_IGNORE_RETURN_VALUE) BT_sm_add_device(bt_smp_bd_addr.addr);
        (BT_IGNORE_RETURN_VALUE) BT_sm_set_device_link_key_and_type(bt_smp_bd_addr.addr, xtxp->lk, &type);
    }
}
#endif
#endif

DECL_STATIC SMP_KEY_DIST local_key_info; /* static to reduce stack usage */
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
static void hci_acl_smp_br_handler(struct net_buf *buf)
{
    API_RESULT retval;

    UINT16   ediv;
    UCHAR  * peer_rand;
    UCHAR    ltk[SMP_LTK_SIZE];
    UCHAR    ltk_null;

    SMP_KEY_DIST * key_info;
    SMP_AUTH_INFO * auth;
    SMP_AUTH_INFO info;
    SMP_BD_ADDR bdaddr;
    SMP_KEY_XCHG_PARAM * kx_param;

    struct bt_conn *conn;
	struct bt_smp_br *smp;

    UCHAR * bd_addr;
    UCHAR   bd_addr_type;

    UCHAR * event_data;

	SMP_KEY_DIST  p_key_info;
    UCHAR         p_keys;

	struct bt_keys *keys;
	bt_addr_le_t peer_addr;

    struct bt_smp_hdr_sumilation *hdr;
#ifdef SMP_LESC

#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
        UCHAR link_key[BT_LINK_KEY_SIZE];
        UCHAR lk_type;
#endif /* SMP_LESC_CROSS_TXP_KEY_GEN */
#endif /* SMP_LESC */

    hdr = (struct bt_smp_hdr_sumilation *)buf->data;

    event_data = (hdr->hdr.len > sizeof(hdr->pdu)) ? (UCHAR *)&buf->data[sizeof(*hdr)] : NULL;

    /* Get the BD Address from handle */
    BT_smp_get_bd_addr (&hdr->pdu.bd_handle, &bdaddr);

    bd_addr = bdaddr.addr;
    bd_addr_type = bdaddr.type;

    (void)bd_addr;
    (void)bd_addr_type;

    LOG_DBG("SMP event =  %s, status %d", ethermind_bt_smp_event_get_name(hdr->pdu.event), hdr->pdu.status);

    conn = bt_conn_lookup_device_id(hdr->pdu.bd_handle);
	if (NULL == conn)
    {
        LOG_ERR("Connect is not found, invalid bd handle 0x%02X", hdr->pdu.bd_handle);
        __ASM("NOP");
        return;
    }

	bt_addr_copy(&peer_addr.a, &conn->br.dst);
	peer_addr.type = BT_ADDR_LE_PUBLIC;

    LOG_DBG("conn = 0x%08X", conn);

	smp = smp_br_chan_get(conn);
	if (smp == NULL)
	{
		LOG_ERR("SMP of conn %p cannot be found", conn);
		bt_conn_unref(conn);
		return;
	}

    retval = API_SUCCESS;

    switch(hdr->pdu.event)
    {
    case SMP_AUTHENTICATION_COMPLETE:
        LOG_DBG("Recvd SMP_AUTHENTICATION_COMPLETE");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");
        LOG_DBG("Status : %04X", hdr->pdu.status);

        if (NULL != event_data)
        {
            if(API_SUCCESS == hdr->pdu.status)
            {
                auth = (SMP_AUTH_INFO *)event_data;

                LOG_DBG("Authentication type : %s",
                (SMP_SEC_LEVEL_2 == (auth->security & 0x0F))?  "With MITM":
                "Encryption Only (without MITM)");

#ifdef SMP_LESC
                LOG_DBG("Pairing Mode : %s",
                (SMP_LESC_MODE == (auth->pair_mode))? "LE SEC Pairing Mode":
                "LEGACY Pairing Mode");
#endif /* SMP_LESC */

                LOG_DBG("Bonding type : %s",
                (auth->bonding)? "Bonding": "Non-Bonding");

				if(auth->bonding)
				{
					atomic_set_bit(smp->flags, SMP_FLAG_BOND);
				}
				else
				{
					atomic_clear_bit(smp->flags, SMP_FLAG_BOND);
				}

                LOG_DBG("Encryption Key size : %d", auth->ekey_size);
#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
                LOG_DBG("Transport : %d\n", auth->transport);
                LOG_DBG("Cross Transport info: %d\n", auth->xtx_info);

                if (0U != (SMP_XTX_KEYGEN_MASK & auth->xtx_info))
                {
                    /* Save the BD Address */
                    BT_COPY_BD_ADDR_AND_TYPE(&bt_smp_bd_addr, &bdaddr);

#ifdef CLASSIC_SEC_MANAGER
                    /* Compare key strengths before generating */
                    if (SMP_LINK_BREDR == auth->transport)
                    {
#ifdef BTSIG_ERRATA_11838
                        SM_DEVICE_STATE state;

                        retval = BT_sm_get_device_security_state(bd_addr, &state);
                        if ((API_SUCCESS != retval) || (16U != state.ekey_size))
                        {
                            LOG_ERR("EncKey Size check failed for LTK generation.\n");
                            break;
                        }
#endif /* BTSIG_ERRATA_11838 */

                        /* Get the BREDR link key for the device */
                        retval = BT_sm_get_device_link_key_and_type(bd_addr, link_key, &lk_type);
                        if (API_SUCCESS != retval)
                        {
                            LOG_ERR("FAILED ! Reason = 0x%04X\n", retval);
                            break;
                        }
                        else
                        {
                            SMP_BD_HANDLE handle;

                            /* Check for the BLE handle of the same BD Address to get security info */
                            retval = BT_smp_get_bd_handle(&bdaddr, &handle);

                            if (API_SUCCESS == retval)
                            {
                                /* Check if the device already has LE LTK which is stronger than the CTKD LTK */
                                retval = BT_smp_get_device_security_info
                                         (
                                             &handle,
                                             &info
                                         );
                                if (API_SUCCESS == retval)
                                {
                                    if ((SMP_SEC_LEVEL_2 == info.security) && (HCI_LINK_KEY_AUTHENTICATED_P_256 != lk_type))
                                    {
                                        retval = API_SUCCESS;
                                    }
                                    else
                                    {
                                        retval = API_FAILURE;
                                    }
                                }
                            }

                            if (API_SUCCESS != retval)
                            {
                                smp->status = (uint8_t)(hdr->pdu.status & 0xFF);

                                /* From the spec, Only CT2 bit is valid in BR smp AuthReq field.
                                 * If Secure Connections pairing has been initiated over BR/EDR, the following fields of
                                 * the SM Pairing Request PDU are reserved for future use:
                                 *  - the IO Capability field,
                                 *  - the OOB data flag field, and
                                 *  - all bits in the Auth Req field except the CT2 bit.
                                 * So the Bonding_Flags of AuthReq is not used in the in cross transport key derivation case,
                                 * so use BR's SMP_FLAG_BOND flag to determind whether saving LE keys here.
                                 */
                                if (!atomic_test_bit(conn->flags, BT_CONN_BR_NOBOND)) {
                                    atomic_set_bit(smp->flags, SMP_FLAG_BOND);
                                } else {
                                    atomic_clear_bit(smp->flags, SMP_FLAG_BOND);
                                }

                                (BT_IGNORE_RETURN_VALUE)BT_smp_get_ltk_from_lk_pl
                                (
                                    link_key,
                                    appl_smp_lesc_xtxp_ltk_complete,
                                    (auth->xtx_info & SMP_XTX_H7_MASK)
                                );
                            }
                        }
                    }
                    else
#endif /* CLASSIC_SEC_MANAGER */
                    {
                        SMP_BD_HANDLE handle;

                        /* Check for the BLE handle of the same BD Address to get security info */
                        retval = BT_smp_get_bd_handle(&bdaddr, &handle);

                        retval = BT_smp_get_device_keys
                                 (
                                     &handle,
                                     &p_keys,
                                     &p_key_info
                                 );

                        if (API_SUCCESS != retval)
                        {
                            LOG_ERR("Failed to get Peer Device Keys!!\n");
                        }
                        else
                        {
                            if (16U != auth->ekey_size)
                            {
#ifdef APPL_SMP_VALIDATE_KEYSIZE_FOR_CTKD
                                LOG_ERR("EncKey Size check failed for LinkKey generation.\n");
                                break;
#else /* APPL_SMP_VALIDATE_KEYSIZE_FOR_CTKD */
                                BT_smp_get_raw_lesc_ltk(&handle, p_key_info.enc_info);
#endif /* APPL_SMP_VALIDATE_KEYSIZE_FOR_CTKD */
                            }

                            /* Save the Identity BD Address if valid */
                            if (SMP_DIST_MASK_ID_KEY & p_keys)
                            {
                                BT_COPY_BD_ADDR(bt_smp_bd_addr.addr, &p_key_info.id_addr_info[1]);
                                bt_smp_bd_addr.type = p_key_info.id_addr_info[0];
                            }

                            /* Check if the device already has BR LK which is stronger than the CTKD LK */
                            retval = BT_sm_get_device_link_key_and_type(bd_addr, link_key, &lk_type);
                            if (API_SUCCESS == retval)
                            {
                                if ((HCI_LINK_KEY_AUTHENTICATED_P_256 == lk_type) && (SMP_SEC_LEVEL_2 != auth->security))
                                {
                                    retval = API_SUCCESS;
                                }
                                else
                                {
                                    retval = API_FAILURE;
                                }
                            }

                            if (API_SUCCESS != retval)
                            {
                                (BT_IGNORE_RETURN_VALUE)BT_smp_get_lk_from_ltk_pl
                                (
                                    p_key_info.enc_info,
                                    appl_smp_lesc_xtxp_lk_complete,
                                    (auth->xtx_info & SMP_XTX_H7_MASK)
                                );
                            }
                        }
                    }
                }
#endif /* SMP_LESC_CROSS_TXP_KEY_GEN */
            }
            else
            {
            }
        }
		else
		{
			if(API_SUCCESS == hdr->pdu.status)
            {
				if (atomic_test_and_clear_bit(smp->flags, SMP_FLAG_PAIRING))
				{
					LOG_DBG("Clear the pairing status");
				}
			}
		}

        if (SMP_REMOTE_SIDE_PIN_KEY_MISSING == hdr->pdu.status)
        {
            LOG_DBG("Peer Device Lost previous Bonding Information!");
            LOG_DBG("Deleting Local Bond Information of Peer...");

            retval = BT_smp_mark_device_untrusted_pl(&hdr->pdu.bd_handle);

            LOG_DBG("Marked Device Untrusted with result 0x%04X",retval);

            if (API_SUCCESS == retval)
            {
                LOG_DBG("Initiate Pairing Again...");
            }
        }

	/* Take the semaphore until security level updated, don't need wait too long. */
        int err = k_sem_take(&conn->sec_lvl_updated, K_MSEC(1));
	if(err < 0)
	{
		LOG_ERR("conn: %p, security level semaphore wait fail %d", conn, err);
	}
        break;

	case SMP_AUTHENTICATION_ERROR:
#if 0
		smp_auth_cb = latch_auth_cb(smp);
		conn = smp->chan.chan.conn;
		smp->status = hdr->pdu.status;

		if (atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER) ||
			atomic_test_and_clear_bit(smp->flags, SMP_FLAG_DISPLAY)) {
			if (smp_auth_cb && smp_auth_cb->cancel) {
				smp_auth_cb->cancel(conn);
			}
		}
		smp_pairing_complete(smp, smp->status);
#endif
        /* Misra check */
		break;

	case SMP_AUTHENTICATION_RESPONSE:
		/* Misra check */
		break;

    case SMP_AUTHENTICATION_REQUEST:
        LOG_DBG("Recvd SMP_AUTHENTICATION_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

        auth = (SMP_AUTH_INFO *)event_data;

        LOG_DBG("Authentication type : %s",
        (SMP_SEC_LEVEL_2 == (auth->security & 0x0F))?  "With MITM":
        "Encryption Only (without MITM)");

        LOG_DBG("Bonding type : %s",
        (auth->bonding)? "Bonding": "Non-Bonding");

		/* Get current security state for the link */
		retval = BT_smp_get_device_security_info (&hdr->pdu.bd_handle, &info);
		if (API_SUCCESS == retval)
		{
			LOG_DBG("Security state %d, bonding %d", info.param, info.bonding);
			/* Check if the link is authenticated */
			if (info.param != SMP_ENTITY_AUTH_ON)
			{
				/* Check if bonded */
				if (SMP_BONDING == info.bonding)
				{
					if (conn->role == BT_CONN_ROLE_PERIPHERAL)
					{
						retval = BT_smp_mark_device_untrusted_pl (&hdr->pdu.bd_handle);

						if (API_SUCCESS == retval)
						{
							/* Misra check */
						}
					}
					else
					{
						LOG_DBG("Received security request request");
					}
				}
			}
		}

	memcpy(&smp->auth, auth, sizeof(smp->auth));
	smp_br_auth_starting(smp);

        break;

    case SMP_PASSKEY_ENTRY_REQUEST:
        LOG_DBG("Event   : SMP_PASSKEY_ENTRY_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

		smp->method = PASSKEY_INPUT;

        if ((NULL == bt_auth) || (NULL == bt_auth->passkey_entry))
        {
            retval = BT_smp_passkey_entry_request_reply
            (
                &hdr->pdu.bd_handle,
                NULL,
                SMP_FALSE
            );
        }
		else
		{
			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			bt_auth->passkey_entry(conn);
		}
        break;

    case SMP_PASSKEY_DISPLAY_REQUEST:
        LOG_DBG("Event   : SMP_PASSKEY_DISPLAY_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

        LOG_DBG("Passkey : %06u", (*((UINT32 *)event_data) % 1000000));

		smp->method = PASSKEY_DISPLAY;

		smp->passkey = (*((UINT32 *)event_data) % 1000000);
		if (bt_auth && bt_auth->passkey_display) {
			atomic_set_bit(smp->flags, SMP_FLAG_DISPLAY);
			bt_auth->passkey_display(smp->chan.chan.conn, smp->passkey);
		}
		smp->passkey = sys_cpu_to_le32(smp->passkey);
        break;

    case SMP_KEY_EXCHANGE_INFO_REQUEST:
        LOG_DBG("Event   : SMP_KEY_EXCHANGE_INFO_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

        /* Reference the event data */
        kx_param = (SMP_KEY_XCHG_PARAM *) event_data;

        LOG_DBG("Local keys negotiated - 0x%02X", kx_param->keys);
        LOG_DBG("Encryption Key Size negotiated - 0x%02X",
                kx_param->ekey_size);

#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
        /* Save the local key distribution information */
        local_keys = kx_param->keys;
#endif

        /* Get platform data of key informations */
        BT_smp_get_key_exchange_info_pl (&key_info);

        /* fix GAP/SEC/AUT/BV-20-C */
        bt_rand(key_info->enc_info, SMP_LTK_SIZE);
        /* Copy the Local Key Info into a local struct */
        BT_mem_copy(&local_key_info,key_info,sizeof(local_key_info));
#if (defined(CONFIG_BT_PRIVACY) && ((CONFIG_BT_PRIVACY) > 0U))
		keys = bt_keys_get_type(BT_KEYS_LOCAL_CSRK, conn->id, &peer_addr);
		if (!keys)
		{
			LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&peer_addr));
			BT_mem_set(local_key_info.id_info, 0x00,sizeof(local_key_info.id_info));
		}
		else
		{
#if (defined(CONFIG_BT_SIGNING) && (CONFIG_BT_SIGNING> 0))
			memcpy(keys->local_csrk.val, &bt_dev.irk[conn->id][0], sizeof(keys->local_csrk.val));
			bt_keys_add_type(keys, BT_KEYS_LOCAL_CSRK);
#endif /* CONFIG_BT_SIGNING */
			BT_mem_copy(local_key_info.id_info, &bt_dev.irk[conn->id][0],sizeof(local_key_info.id_info));
		}
#else
        BT_mem_set(local_key_info.id_info, 0x00,sizeof(local_key_info.id_info));
#endif /* CONFIG_BT_PRIVACY */
        /* Mask the to be exchanged LTK according to the negotiated key size */
        BT_mem_set
        (
            (&local_key_info.enc_info[0] + kx_param->ekey_size),
            0x00,
            (SMP_LTK_SIZE - kx_param->ekey_size)
        );

        BT_smp_key_exchange_info_request_reply (&hdr->pdu.bd_handle, &local_key_info);
        break;

    case SMP_LONG_TERM_KEY_REQUEST:

        /* Copy parameters to local variables */
        smp_unpack_2_byte_param(&ediv, &event_data[8]);
        peer_rand = event_data;

        LOG_DBG("Event   : SMP_LONG_TERM_KEY_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");
        LOG_DBG("Div  : 0x%04X", ediv);
        LOG_DBG("Rand : %02X %02X %02X %02X %02X %02X %02X %02X",
        peer_rand[0], peer_rand[1], peer_rand[2], peer_rand[3],
        peer_rand[4], peer_rand[5], peer_rand[6], peer_rand[7]);

        /* Do not process if status is failre */
        if (API_SUCCESS != hdr->pdu.status)
        {
            LOG_DBG("Long Term Key request with Error - 0x%04X. Dropping.", hdr->pdu.status);
            break;
        }

		retval = BT_smp_get_device_security_info (&hdr->pdu.bd_handle, &info);

		if (API_SUCCESS == retval)
		{
			/* Check if the link is authenticated */
			if ((SMP_ENTITY_AUTH_ON == info.param) ||
				(SMP_TRUE == info.bonding))
			{
#ifdef SMP_LESC
				if (info.pair_mode == SMP_LESC_MODE)
				{
					retval = BT_smp_get_device_keys
								(
									&hdr->pdu.bd_handle,
									&p_keys,
									&p_key_info
								);

					if (API_SUCCESS != retval)
					{
						LOG_ERR("Failed to get Peer Device Keys!!");
					}
					else
					{
						BT_mem_copy(ltk, p_key_info.enc_info, 16);

						/*
							* NOTE: To check if masking of LTK according to negotiated key size
							*       is needed when in secure connections only mode.
							*/
					}
				}
				else
#endif /* SMP_LESC */
				{
					/* Get LTK for remote device */
					retval = BT_smp_get_long_term_key_pl
								(
									peer_rand,
									ediv,
									ltk
								);

					if(API_SUCCESS == retval)
					{
						/* Mask the key according to the negotiated key size */
						BT_mem_set
						(
							(ltk + info.ekey_size),
							0x00,
							(SMP_LTK_SIZE - info.ekey_size)
						);
					}
				}

				if (API_SUCCESS == retval)
				{
					LOG_DBG("Sending +ve LTK request reply.");
					retval = BT_smp_long_term_key_request_reply
								(
									&hdr->pdu.bd_handle,
									ltk,
									SMP_TRUE
								);
				}
			}
			else
			{
				retval = API_FAILURE;
			}
		}

		if (API_SUCCESS != retval)
		{
			LOG_DBG("Sending -ve LTK request reply.");
			ltk_null = 0;
			retval = BT_smp_long_term_key_request_reply
						(
							&hdr->pdu.bd_handle,
							&ltk_null,
							SMP_FALSE
						);
			/*smp->status = SMP_REMOTE_SIDE_PIN_KEY_MISSING;
			 TODO: If needed, handle encryption failed */
		}
        break;

    case SMP_KEY_EXCHANGE_INFO:
        LOG_DBG ("Recvd SMP_KEY_EXCHANGE_INFO");
        LOG_DBG ("Status - 0x%04X", hdr->pdu.status);

        /* Reference the event data */
        kx_param = (SMP_KEY_XCHG_PARAM *) event_data;

        LOG_DBG("Remote keys negotiated - 0x%02X", kx_param->keys);
        LOG_DBG("Encryption Key Size negotiated - 0x%02X",
                kx_param->ekey_size);

        /* Reference the key information */
        key_info = kx_param->keys_info;

        /* Store the peer keys */
        LOG_HEXDUMP_DBG(key_info->enc_info, sizeof (key_info->enc_info), "Encryption Info:");
        LOG_HEXDUMP_DBG(key_info->mid_info, sizeof (key_info->mid_info), "Master Identification Info:");
        LOG_HEXDUMP_DBG(key_info->id_info, sizeof (key_info->id_info), "Identity Info:");
        LOG_HEXDUMP_DBG(key_info->id_addr_info, sizeof (key_info->id_addr_info), "Identity Address Info:");
        LOG_HEXDUMP_DBG(key_info->sign_info, sizeof (key_info->sign_info), "Signature Info:");
		keys = bt_keys_get_type(BT_KEYS_IRK, conn->id, &peer_addr);
		if (!keys)
		{
			LOG_ERR("Unable to get keys for %s", bt_addr_le_str(&peer_addr));
		}
		else
		{
			(void)memset(keys->ltk.ediv, 0, sizeof(keys->ltk.ediv));
			(void)memset(keys->ltk.rand, 0, sizeof(keys->ltk.rand));
			keys->enc_size = kx_param->ekey_size;
			memcpy(keys->irk.val, key_info->id_info, sizeof(keys->irk.val));
			bt_keys_add_type(keys, BT_KEYS_IRK);
			bt_id_add(keys);
#if (defined(CONFIG_BT_SIGNING) && (CONFIG_BT_SIGNING > 0U))
			memcpy(keys->remote_csrk.val, key_info->sign_info, sizeof(keys->remote_csrk.val));
			bt_keys_add_type(keys, BT_KEYS_REMOTE_CSRK);
#endif /* CONFIG_BT_SIGNING */
		}

        break;

#ifdef SMP_LESC
    case SMP_NUMERIC_KEY_COMPARISON_CNF_REQUEST:
        LOG_DBG("Event   : SMP_NUMERIC_KEY_COMPARISON_CNF_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

		smp->method = PASSKEY_CONFIRM;
		smp->confirm_type = CONFIRM_TYPE_PASSKEY;

        LOG_DBG("Numeric Code : %06u", (*((UINT32 *)event_data) % 1000000));
		if (bt_auth && bt_auth->passkey_confirm) {
			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			bt_auth->passkey_confirm(smp->chan.chan.conn, (*((UINT32 *)event_data) % 1000000));
		}
        break;

    case SMP_KEY_PRESS_NOTIFICATION_EVENT:
        LOG_DBG("Event   : SMP_KEY_PRESS_NOTIFICATION_EVENT");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

        LOG_DBG("KeyPress Notification Value is:");

        switch(*(UCHAR *)event_data)
        {
            case SMP_LESC_PASSKEY_ENTRY_STARTED:
                LOG_DBG("SMP_LESC_PASSKEY_ENTRY_STARTED");
                break;
            case SMP_LESC_PASSKEY_DIGIT_ENTERED:
                LOG_DBG("SMP_LESC_PASSKEY_DIGIT_ENTERED");
                break;
            case SMP_LESC_PASSKEY_DIGIT_ERASED:
                LOG_DBG("SMP_LESC_PASSKEY_DIGIT_ERASED");
                break;
            case SMP_LESC_PASSKEY_CLEARED:
                LOG_DBG("SMP_LESC_PASSKEY_CLEARED");
                break;
            case SMP_LESC_PASSKEY_ENTRY_COMPLETED:
                LOG_DBG("SMP_LESC_PASSKEY_ENTRY_COMPLETED");
                break;
            default:
                LOG_DBG("Unknown KeyPress Value 0x%02X Received",*(UCHAR *)event_data);
                break;
        }
        break;
#endif /* SMP_LESC */

    default:
        LOG_DBG("ERROR!!! Received unknown event. event = %02X", hdr->pdu.event);
    }

    LOG_DBG("ret = %d", retval);
    bt_conn_unref(conn);
}
#endif /* CONFIG_BT_CLASSIC */

static void hci_acl_smp_handler(struct net_buf *buf)
{
    API_RESULT retval;

    UINT16   ediv;
    UCHAR  * peer_rand;
    UCHAR    ltk[SMP_LTK_SIZE];
    UCHAR    ltk_null;

    SMP_KEY_DIST * key_info;
    SMP_AUTH_INFO * auth;
    SMP_AUTH_INFO info;
    SMP_BD_ADDR bdaddr;
    SMP_KEY_XCHG_PARAM * kx_param;

    struct bt_conn *conn;
	struct bt_smp *smp;

    UCHAR * bd_addr;
    UCHAR   bd_addr_type;

    UCHAR * event_data;

	SMP_KEY_DIST  p_key_info;
    UCHAR         p_keys;

    struct bt_smp_hdr_sumilation *hdr;
#ifdef SMP_LESC

#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
        UCHAR link_key[BT_LINK_KEY_SIZE];
        UCHAR lk_type;
#endif /* SMP_LESC_CROSS_TXP_KEY_GEN */
#endif /* SMP_LESC */

    const struct bt_conn_auth_cb *smp_auth_cb;

    hdr = (struct bt_smp_hdr_sumilation *)buf->data;

    event_data = (hdr->hdr.len > sizeof(hdr->pdu)) ? (UCHAR *)&buf->data[sizeof(*hdr)] : NULL;

    /* Get the BD Address from handle */
    BT_smp_get_bd_addr (&hdr->pdu.bd_handle, &bdaddr);

    bd_addr = bdaddr.addr;
    bd_addr_type = bdaddr.type;

    (void)bd_addr;
    (void)bd_addr_type;

    LOG_DBG("SMP event =  %s, status %d", ethermind_bt_smp_event_get_name(hdr->pdu.event), hdr->pdu.status);

    conn = bt_conn_lookup_device_id(hdr->pdu.bd_handle);
	if (NULL == conn)
    {
        LOG_ERR("Connect is not found, invalid bd handle 0x%02X", hdr->pdu.bd_handle);
        __ASM("NOP");
        return;
    }


	if (BT_CONN_TYPE_BR == conn->type)
	{
		bt_conn_unref(conn);
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
		hci_acl_smp_br_handler(buf);
#endif /* CONFIG_BT_CLASSIC */
		return;
	}

    LOG_DBG("conn = 0x%08X", conn);
#if 0
    if ((NULL != conn) && (BT_CONN_TYPE_LE != conn->type) && (SMP_AUTHENTICATION_REQUEST != hdr->pdu.event))
    {
        bt_conn_unref(conn);
        return SMP_INVALID_PARAMETERS;
    }
#endif
	smp = smp_chan_get(conn);

    if ((NULL == smp) && (NULL != conn) && (BT_CONN_TYPE_LE != conn->type))
    {
        struct bt_l2cap_le_chan *ch;
        struct bt_l2cap_chan *chan;

		if (smp_fixed_chan.accept(conn, &chan) >= 0)
        {
            ch = BT_L2CAP_LE_CHAN(chan);

            /* Fill up remaining fixed channel context attached in
             * fchan->accept()
             */
            ch->rx.cid = smp_fixed_chan.cid;
            ch->tx.cid = smp_fixed_chan.cid;

            bt_l2cap_chan_add(conn, chan, smp_fixed_chan.destroy);
            {
#if 1
                if (chan->ops->connected) {
                    chan->ops->connected(chan);
                }
#endif
                smp = smp_chan_get(conn);
            }
        }
    }
    retval = API_SUCCESS;

    switch(hdr->pdu.event)
    {
    case SMP_AUTHENTICATION_COMPLETE:
        LOG_DBG("Recvd SMP_AUTHENTICATION_COMPLETE");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");
        LOG_DBG("Status : %04X", hdr->pdu.status);

        if (NULL != event_data)
        {
            if(API_SUCCESS == hdr->pdu.status)
            {
                auth = (SMP_AUTH_INFO *)event_data;

                LOG_DBG("Authentication type : %s",
                (SMP_SEC_LEVEL_2 == (auth->security & 0x0F))?  "With MITM":
                "Encryption Only (without MITM)");

#ifdef SMP_LESC
                LOG_DBG("Pairing Mode : %s",
                (SMP_LESC_MODE == (auth->pair_mode))? "LE SEC Pairing Mode":
                "LEGACY Pairing Mode");
#endif /* SMP_LESC */

                LOG_DBG("Bonding type : %s",
                (auth->bonding)? "Bonding": "Non-Bonding");

                LOG_DBG("Encryption Key size : %d", auth->ekey_size);
#ifdef SMP_LESC_CROSS_TXP_KEY_GEN
                LOG_DBG("Transport : %d\n", auth->transport);
                LOG_DBG("Cross Transport info: %d\n", auth->xtx_info);

                if (0U != (SMP_XTX_KEYGEN_MASK & auth->xtx_info))
                {
                    /* Save the BD Address */
                    BT_COPY_BD_ADDR_AND_TYPE(&bt_smp_bd_addr, &bdaddr);

#ifdef CLASSIC_SEC_MANAGER
                    /* Compare key strengths before generating */
                    if (SMP_LINK_BREDR == auth->transport)
                    {
#ifdef BTSIG_ERRATA_11838
                        SM_DEVICE_STATE state;

                        retval = BT_sm_get_device_security_state(bd_addr, &state);
                        if ((API_SUCCESS != retval) || (16U != state.ekey_size))
                        {
                            LOG_ERR("EncKey Size check failed for LTK generation.\n");
                            break;
                        }
#endif /* BTSIG_ERRATA_11838 */

                        /* Get the BREDR link key for the device */
                        retval = BT_sm_get_device_link_key_and_type(bd_addr, link_key, &lk_type);
                        if (API_SUCCESS != retval)
                        {
                            LOG_ERR("FAILED ! Reason = 0x%04X\n", retval);
                            break;
                        }
                        else
                        {
                            SMP_BD_HANDLE handle;

                            /* Check for the BLE handle of the same BD Address to get security info */
                            retval = BT_smp_get_bd_handle(&bdaddr, &handle);

                            if (API_SUCCESS == retval)
                            {
                                /* Check if the device already has LE LTK which is stronger than the CTKD LTK */
                                retval = BT_smp_get_device_security_info
                                         (
                                             &handle,
                                             &info
                                         );
                                if (API_SUCCESS == retval)
                                {
                                    if ((SMP_SEC_LEVEL_2 == info.security) && (HCI_LINK_KEY_AUTHENTICATED_P_256 != lk_type))
                                    {
                                        retval = API_SUCCESS;
                                    }
                                    else
                                    {
                                        retval = API_FAILURE;
                                    }
                                }
                            }

                            if (API_SUCCESS != retval)
                            {
                                (BT_IGNORE_RETURN_VALUE)BT_smp_get_ltk_from_lk_pl
                                (
                                    link_key,
                                    appl_smp_lesc_xtxp_ltk_complete,
                                    (auth->xtx_info & SMP_XTX_H7_MASK)
                                );
                            }
                        }
                    }
                    else
#endif /* CLASSIC_SEC_MANAGER */
                    {
                        SMP_BD_HANDLE handle;

                        /* Check for the BLE handle of the same BD Address to get security info */
                        retval = BT_smp_get_bd_handle(&bdaddr, &handle);

                        retval = BT_smp_get_device_keys
                                 (
                                     &handle,
                                     &p_keys,
                                     &p_key_info
                                 );

                        if (API_SUCCESS != retval)
                        {
                            LOG_ERR("Failed to get Peer Device Keys!!\n");
                        }
                        else
                        {
                            if (16U != auth->ekey_size)
                            {
#ifdef APPL_SMP_VALIDATE_KEYSIZE_FOR_CTKD
                                LOG_ERR("EncKey Size check failed for LinkKey generation.\n");
                                break;
#else /* APPL_SMP_VALIDATE_KEYSIZE_FOR_CTKD */
                                BT_smp_get_raw_lesc_ltk(&handle, p_key_info.enc_info);
#endif /* APPL_SMP_VALIDATE_KEYSIZE_FOR_CTKD */
                            }

                            /* Save the Identity BD Address if valid */
                            if (SMP_DIST_MASK_ID_KEY & p_keys)
                            {
                                BT_COPY_BD_ADDR(bt_smp_bd_addr.addr, &p_key_info.id_addr_info[1]);
                                bt_smp_bd_addr.type = p_key_info.id_addr_info[0];
                            }

                            /* Check if the device already has BR LK which is stronger than the CTKD LK */
                            retval = BT_sm_get_device_link_key_and_type(bd_addr, link_key, &lk_type);
                            if (API_SUCCESS == retval)
                            {
                                if ((HCI_LINK_KEY_AUTHENTICATED_P_256 == lk_type) && (SMP_SEC_LEVEL_2 != auth->security))
                                {
                                    retval = API_SUCCESS;
                                }
                                else
                                {
                                    retval = API_FAILURE;
                                }
                            }

                            if (API_SUCCESS != retval)
                            {
                                (BT_IGNORE_RETURN_VALUE)BT_smp_get_lk_from_ltk_pl
                                (
                                    p_key_info.enc_info,
                                    appl_smp_lesc_xtxp_lk_complete,
                                    (auth->xtx_info & SMP_XTX_H7_MASK)
                                );
                            }
                        }
                    }
                }
#endif /* SMP_LESC_CROSS_TXP_KEY_GEN */
#if 0
                /**
                 * If the Bonding Feature is Set for this Pairing, then Automatically
                 * 1. Added the device to Resolving.
                 * 2. Set the privacy Mode to Device Privacy Mode.
                 */
                if (auth->bonding)
                {
                    SMP_BD_ADDR t_bd_addr;

                    /* check if the device has shared the keys */
                    /* Get platform data of key informations */
                    BT_smp_get_key_exchange_info_pl (&key_info);

                    BT_COPY_BD_ADDR(&t_bd_addr.addr, &peer_key_info.id_addr_info[1]);
                    BT_COPY_TYPE(t_bd_addr.type, peer_key_info.id_addr_info[0]);

                    retval = BT_hci_le_add_device_to_resolving_list
                             (
                                 t_bd_addr.type,
                                 &t_bd_addr.addr[0],
                                 peer_key_info.id_info,
                                 key_info->id_info
                             );

                    if(API_SUCCESS != retval)
                    {
                        LOG_DBG("FAILED !!! Error code = %04X", retval);
                    }
                    else
                    {
                        LOG_DBG("Added Device with Identity Address: "
                        BT_DEVICE_ADDR_FRMT_SPECIFIER " to Resolving List!",
                        BT_DEVICE_ADDR_PRINT_STR(&t_bd_addr));

                        retval = BT_hci_le_set_privacy_mode
                                 (
                                     peer_identity_addr_type,
                                     peer_identity_addr,
                                     0x01 /* Device Privacy Mode */
                                 );

                        LOG_DBG("Setting Device Privacy Mode returned with "
                        "0x%04X", retval);

                    }
                }
#endif
            }
            else
            {
            }
        }
		else
		{
			if(API_SUCCESS == hdr->pdu.status)
            {
				if (conn->le.keys != NULL)
				{
					if (atomic_test_and_clear_bit(smp->flags, SMP_FLAG_PAIRING))
					{
						LOG_DBG("Clear the pairing status");
					}
				}
			}
		}

        if (SMP_REMOTE_SIDE_PIN_KEY_MISSING == hdr->pdu.status)
        {
            LOG_DBG("Peer Device Lost previous Bonding Information!");
            LOG_DBG("Deleting Local Bond Information of Peer...");

            retval = BT_smp_mark_device_untrusted_pl(&hdr->pdu.bd_handle);

            LOG_DBG("Marked Device Untrusted with result 0x%04X",retval);

            if (API_SUCCESS == retval)
            {
                LOG_DBG("Initiate Pairing Again...");
            }
        }

        smp->status = hdr->pdu.status;
	smp_auth_complete(smp);

        break;

	case SMP_AUTHENTICATION_ERROR:
		smp_auth_cb = latch_auth_cb(smp);
		conn = smp->chan.chan.conn;
		smp->status = hdr->pdu.status;

		if (atomic_test_and_clear_bit(smp->flags, SMP_FLAG_USER) ||
			atomic_test_and_clear_bit(smp->flags, SMP_FLAG_DISPLAY)) {
			if (smp_auth_cb && smp_auth_cb->cancel) {
				smp_auth_cb->cancel(conn);
			}
		}
		smp_pairing_complete(smp, smp->status);
		break;

	case SMP_AUTHENTICATION_RESPONSE:
		auth = (SMP_AUTH_INFO *)event_data;
        if(legacy_oobd_present || sc_oobd_present)
		{
			if (bt_auth && bt_auth->oob_data_request) {
				struct bt_conn_oob_info info = {
					.type = BT_CONN_OOB_LE_SC,
					.lesc.oob_config = BT_CONN_OOB_BOTH_PEERS,
				};

				if(SMP_LEGACY_MODE == auth->pair_mode)
				{
					info.type = BT_CONN_OOB_LE_LEGACY;
				}

				smp->oobd_local = NULL;
				smp->oobd_remote = NULL;

				atomic_set_bit(smp->flags, SMP_FLAG_OOB_PENDING);
				bt_auth->oob_data_request(conn, &info);
			}
		}
		break;

    case SMP_AUTHENTICATION_REQUEST:
        LOG_DBG("Recvd SMP_AUTHENTICATION_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

        auth = (SMP_AUTH_INFO *)event_data;

        LOG_DBG("Authentication type : %s",
        (SMP_SEC_LEVEL_2 == (auth->security & 0x0F))?  "With MITM":
        "Encryption Only (without MITM)");

        LOG_DBG("Bonding type : %s",
        (auth->bonding)? "Bonding": "Non-Bonding");

		/* Get current security state for the link */
		retval = BT_smp_get_device_security_info (&hdr->pdu.bd_handle, &info);
		if (API_SUCCESS == retval)
		{
			LOG_DBG("Security state %d, bonding %d", info.param, info.bonding);
			/* Check if the link is authenticated */
			if (info.param != SMP_ENTITY_AUTH_ON)
			{
				/**
				 *  Here, the application logic selected is to delete
				 *  bond of an exisitng peer device from the bonded
				 *  device list on receiving an Authentication Request
				 *  from it again on an UnAuthenticated link.
				 *  i.e. If a device initiates Pairing again after a
				 *  disconnection, even though it was previously bonded.
				 *  This could happen, if the peer device has lost the
				 *  Bonding informations inbetween connections.
				 *  Typically, popular smart phones/commercial devices and application
				 *  guidelines will not delete the bonding information here
				 *  without the approval/intervention from the user and
				 *  send out a "PAIRING FAILED" error with error code
				 *   - "SMP_ERROR_UNSPECIFIED_REASON", or
				 *   - "SMP_ERROR_PAIRING_NOT_SUPPORTED".
				 *  Also, such implementations would have some UI control to delete the
				 *  bonding on local side (eg: Button press combinations
				 *  etc.).
				 *  The following application logic is added, in this example
				 *  application which can also run on embedded or constrained system
				 *  without scope of User Intervention.
				 *
				 *  Application writer, needs to choose a better
				 *  alternative approach instead of deleteing peer bond information
				 *  automatically.
				 *
				 *  NOTE-1: This application logic/rational choosen for ease
				 *  of use, has security holes and power inefficiency as
				 *  pairing is allowed to happen multiple times with the
				 *  same device.
				 *
				 *  NOTE-2: Inorder to send pairing failure in case of
				 *  pairing request from an existing bonded device,
				 *  the application can set,
				 *  auth->param = SMP_ERROR_UNSPECIFIED_REASON or
				 *  auth->param = SMP_ERROR_PAIRING_NOT_SUPPORTED
				 *
				 *  while calling BT_smp_authentication_request_reply(...) API
				 *  is called.
				 */
				/* Check if bonded */
				if (SMP_BONDING == info.bonding)
				{
					if (conn->role == BT_CONN_ROLE_PERIPHERAL)
					{
						/**
						 * The application logic choosen here is to delete the
						 * bonding information of the peer device.
						 * This needs to be modified according to the
						 * suitable requirements and platform capabilities
						 * according to the Application writer as explained in
						 * the above comment.
						 */
						retval = BT_smp_mark_device_untrusted_pl (&hdr->pdu.bd_handle);

						if (API_SUCCESS == retval)
						{
							/**
							 * On Successfull deletion of bond of a given peer
							 * device. We are clearning the data maintained by
							 * GATT with respect to that peer.
							 * This will reset all the CCCD etc. which are
							 * configured by that peer.
							 * i.e. all the "Indications" or "Notifications"
							 * enabled will be "Turned-OFF" at this location.
							 *
							 * As mentioned above, this section also needs to
							 * be modified according to the requirements and
							 * platform capabilities by the Application writer.
							 */
							/* TODO: Device untrusted */
						}
					}
					else
					{
						LOG_DBG("Received security request request");
					}
				}
			}
		}

	memcpy(&smp->auth, auth, sizeof(smp->auth));
	smp_auth_starting(smp);
        break;

    case SMP_PASSKEY_ENTRY_REQUEST:
        LOG_DBG("Event   : SMP_PASSKEY_ENTRY_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

		smp->method = PASSKEY_INPUT;

        if ((NULL == bt_auth) || (NULL == bt_auth->passkey_entry))
        {
            retval = BT_smp_passkey_entry_request_reply
            (
                &hdr->pdu.bd_handle,
                NULL,
                SMP_FALSE
            );
        }
		else
		{
			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			bt_auth->passkey_entry(conn);
		}
        break;

    case SMP_PASSKEY_DISPLAY_REQUEST:
        LOG_DBG("Event   : SMP_PASSKEY_DISPLAY_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

        LOG_DBG("Passkey : %06u", (*((UINT32 *)event_data) % 1000000));

		smp->method = PASSKEY_DISPLAY;

		smp->passkey = (*((UINT32 *)event_data) % 1000000);
		if (bt_auth && bt_auth->passkey_display) {
			atomic_set_bit(smp->flags, SMP_FLAG_DISPLAY);
			bt_auth->passkey_display(smp->chan.chan.conn, smp->passkey);
		}
		smp->passkey = sys_cpu_to_le32(smp->passkey);
        break;

    case SMP_KEY_EXCHANGE_INFO_REQUEST:
        LOG_DBG("Event   : SMP_KEY_EXCHANGE_INFO_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

        /* Reference the event data */
        kx_param = (SMP_KEY_XCHG_PARAM *) event_data;

        LOG_DBG("Local keys negotiated - 0x%02X", kx_param->keys);
        LOG_DBG("Encryption Key Size negotiated - 0x%02X",
                kx_param->ekey_size);

        /* Get platform data of key informations */
        BT_smp_get_key_exchange_info_pl (&key_info);

        /* fix GAP/SEC/AUT/BV-20-C */
        bt_rand(key_info->enc_info, SMP_LTK_SIZE);
        /* Copy the Local Key Info into a local struct */
        BT_mem_copy(&local_key_info,key_info,sizeof(local_key_info));
#if (defined(CONFIG_BT_PRIVACY) && ((CONFIG_BT_PRIVACY) > 0U))
        BT_mem_copy(local_key_info.id_info, &bt_dev.irk[conn->id][0],sizeof(local_key_info.id_info));
#else
        BT_mem_set(local_key_info.id_info, 0x00,sizeof(local_key_info.id_info));
#endif /* CONFIG_BT_PRIVACY */
        /* Mask the to be exchanged LTK according to the negotiated key size */
        BT_mem_set
        (
            (&local_key_info.enc_info[0] + kx_param->ekey_size),
            0x00,
            (SMP_LTK_SIZE - kx_param->ekey_size)
        );

        BT_smp_key_exchange_info_request_reply (&hdr->pdu.bd_handle, &local_key_info);
        break;

    case SMP_LONG_TERM_KEY_REQUEST:

        /* Copy parameters to local variables */
        smp_unpack_2_byte_param(&ediv, &event_data[8]);
        peer_rand = event_data;

        LOG_DBG("Event   : SMP_LONG_TERM_KEY_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");
        LOG_DBG("Div  : 0x%04X", ediv);
        LOG_DBG("Rand : %02X %02X %02X %02X %02X %02X %02X %02X",
        peer_rand[0], peer_rand[1], peer_rand[2], peer_rand[3],
        peer_rand[4], peer_rand[5], peer_rand[6], peer_rand[7]);

        /* Do not process if status is failre */
        if (API_SUCCESS != hdr->pdu.status)
        {
            LOG_DBG("Long Term Key request with Error - 0x%04X. Dropping.", hdr->pdu.status);
            break;
        }

		retval = BT_smp_get_device_security_info (&hdr->pdu.bd_handle, &info);

		if (API_SUCCESS == retval)
		{
			/* Check if the link is authenticated */
			if ((SMP_ENTITY_AUTH_ON == info.param) ||
				(SMP_TRUE == info.bonding))
			{
#ifdef SMP_LESC
				if (info.pair_mode == SMP_LESC_MODE)
				{
					retval = BT_smp_get_device_keys
								(
									&hdr->pdu.bd_handle,
									&p_keys,
									&p_key_info
								);

					if (API_SUCCESS != retval)
					{
						LOG_ERR("Failed to get Peer Device Keys!!");
					}
					else
					{
						BT_mem_copy(ltk, p_key_info.enc_info, 16);

						/*
							* NOTE: To check if masking of LTK according to negotiated key size
							*       is needed when in secure connections only mode.
							*/
					}
				}
				else
#endif /* SMP_LESC */
				{
					/* Get LTK for remote device */
					retval = BT_smp_get_long_term_key_pl
								(
									peer_rand,
									ediv,
									ltk
								);

					if(API_SUCCESS == retval)
					{
						/* Mask the key according to the negotiated key size */
						BT_mem_set
						(
							(ltk + info.ekey_size),
							0x00,
							(SMP_LTK_SIZE - info.ekey_size)
						);
					}
				}

				if (API_SUCCESS == retval)
				{
					LOG_DBG("Sending +ve LTK request reply.");
					retval = BT_smp_long_term_key_request_reply
								(
									&hdr->pdu.bd_handle,
									ltk,
									SMP_TRUE
								);
				}
			}
			else
			{
				retval = API_FAILURE;
			}
		}

		if (API_SUCCESS != retval)
		{
			LOG_DBG("Sending -ve LTK request reply.");
			ltk_null = 0;
			retval = BT_smp_long_term_key_request_reply
						(
							&hdr->pdu.bd_handle,
							&ltk_null,
							SMP_FALSE
						);
			smp->status = SMP_REMOTE_SIDE_PIN_KEY_MISSING;
			smp_auth_complete(smp);
		}
        break;


    case SMP_RESOLVABLE_PVT_ADDR_CREATE_CNF:
        LOG_DBG("Event   : SMP_RESOLVABLE_PVT_ADDR_CREATE_CNF");
        LOG_DBG("Status : %04X", hdr->pdu.status);
        if(API_SUCCESS == hdr->pdu.status)
        {
            LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
            bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],
            bd_addr[5]);
        }
        break;

    case SMP_RESOLVABLE_PVT_ADDR_VERIFY_CNF:
        LOG_DBG("Event   : SMP_RESOLVABLE_PVT_ADDR_VERIFY_CNF");
        LOG_DBG("Status : %04X", hdr->pdu.status);
        if(API_SUCCESS == hdr->pdu.status)
        {
            LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
            bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],
            bd_addr[5]);
        }
        break;

#if 0 /* TODO: SMP_DATA_SIGNING */
    case SMP_DATA_SIGNING_COMPLETE:
        LOG_DBG("Event   : SMP_SIGNING_DATA_COMPLETE");
        LOG_DBG("Status : %04X", hdr->pdu.status);
        if(API_SUCCESS == hdr->pdu.status)
        {
            /* Update sign counter */
            sign_create_counter ++;

            LOG_DBG("Message Authentication Code : ");
            ethermind_dump_bytes
            (
                event_data,
                data_len
            );

            /* Form the signature */
            BT_mem_copy
            (&signature[sizeof(sign_create_counter)], event_data, data_len);

            LOG_DBG("Message Authentication Code : ");
            ethermind_dump_bytes
            (
                signature,
                sizeof (signature)
            );
        }

        /* Call gatt signature procedure handler */
        appl_gatt_signing_complete
        (hdr->pdu.status, signature, sizeof(signature));

        if (NULL != sign_data)
        {
            BT_free_mem (sign_data);
            sign_data = NULL;
        }

        break;

    case SMP_SIGN_DATA_VERIFICATION_COMPLETE:
        LOG_DBG("Event   : SMP_SIGN_DATA_VERIFICATION_COMPLETE");
        LOG_DBG("Status : %04X", hdr->pdu.status);

        if (API_SUCCESS == hdr->pdu.status)
        {
            /* Form the signature */
            BT_PACK_LE_4_BYTE(signature, &sign_verify_counter);
            BT_mem_copy
            (&signature[sizeof(sign_verify_counter)], event_data, data_len);

            /* Update sign counter */
            sign_verify_counter ++;

            LOG_DBG("Message Authentication Code : ");
            ethermind_dump_bytes
            (
                event_data,
                data_len
            );
        }

        /* Call gatt signature procedure handler */
        appl_gatt_signing_verification_complete
        (hdr->pdu.status, signature, sizeof(signature));

        if (NULL != sign_data)
        {
            BT_free_mem (sign_data);
            sign_data = NULL;
        }

        break;
#endif /* SMP_DATA_SIGNING */

    case SMP_KEY_EXCHANGE_INFO:
        LOG_DBG ("Recvd SMP_KEY_EXCHANGE_INFO");
        LOG_DBG ("Status - 0x%04X", hdr->pdu.status);

        /* Reference the event data */
        kx_param = (SMP_KEY_XCHG_PARAM *) event_data;

        LOG_DBG("Remote keys negotiated - 0x%02X", kx_param->keys);
        LOG_DBG("Encryption Key Size negotiated - 0x%02X",
                kx_param->ekey_size);

        /* Reference the key information */
        key_info = kx_param->keys_info;

        /* Store the peer keys */
        LOG_HEXDUMP_DBG(key_info->enc_info, sizeof (key_info->enc_info), "Encryption Info:");
        LOG_HEXDUMP_DBG(key_info->mid_info, sizeof (key_info->mid_info), "Master Identification Info:");
        LOG_HEXDUMP_DBG(key_info->id_info, sizeof (key_info->id_info), "Identity Info:");
        LOG_HEXDUMP_DBG(key_info->id_addr_info, sizeof (key_info->id_addr_info), "Identity Address Info:");
        LOG_HEXDUMP_DBG(key_info->sign_info, sizeof (key_info->sign_info), "Signature Info:");
        break;

#ifdef SMP_LESC
    case SMP_NUMERIC_KEY_COMPARISON_CNF_REQUEST:
        LOG_DBG("Event   : SMP_NUMERIC_KEY_COMPARISON_CNF_REQUEST");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

		smp->method = PASSKEY_CONFIRM;
		smp->confirm_type = CONFIRM_TYPE_PASSKEY;

        LOG_DBG("Numeric Code : %06u", (*((UINT32 *)event_data) % 1000000));
		if (bt_auth && bt_auth->passkey_confirm) {
			atomic_set_bit(smp->flags, SMP_FLAG_USER);
			bt_auth->passkey_confirm(smp->chan.chan.conn, (*((UINT32 *)event_data) % 1000000));
		}
        break;

    case SMP_KEY_PRESS_NOTIFICATION_EVENT:
        LOG_DBG("Event   : SMP_KEY_PRESS_NOTIFICATION_EVENT");
        LOG_DBG("BD Address : %02X %02X %02X %02X %02X %02X",
        bd_addr[0],bd_addr[1],bd_addr[2],bd_addr[3],bd_addr[4],bd_addr[5]);
        LOG_DBG("BD addr type : %s",
        (0 == bd_addr_type)? "Public Address": "Random Address");

        LOG_DBG("KeyPress Notification Value is:");

        switch(*(UCHAR *)event_data)
        {
            case SMP_LESC_PASSKEY_ENTRY_STARTED:
                LOG_DBG("SMP_LESC_PASSKEY_ENTRY_STARTED");
                break;
            case SMP_LESC_PASSKEY_DIGIT_ENTERED:
                LOG_DBG("SMP_LESC_PASSKEY_DIGIT_ENTERED");
                break;
            case SMP_LESC_PASSKEY_DIGIT_ERASED:
                LOG_DBG("SMP_LESC_PASSKEY_DIGIT_ERASED");
                break;
            case SMP_LESC_PASSKEY_CLEARED:
                LOG_DBG("SMP_LESC_PASSKEY_CLEARED");
                break;
            case SMP_LESC_PASSKEY_ENTRY_COMPLETED:
                LOG_DBG("SMP_LESC_PASSKEY_ENTRY_COMPLETED");
                break;
            default:
                LOG_DBG("Unknown KeyPress Value 0x%02X Received",*(UCHAR *)event_data);
                break;
        }
        break;
#endif /* SMP_LESC */

    default:
        LOG_DBG("ERROR!!! Received unknown event. event = %02X", hdr->pdu.event);
    }

    LOG_DBG("ret = %d", retval);
    bt_conn_unref(conn);
}

static API_RESULT ethermind_bt_smp_cb
           (
               /* IN */ SMP_BD_HANDLE   * bd_handle,
               /* IN */ UCHAR      event,
               /* IN */ API_RESULT status,
               /* IN */ void     * eventdata,
               /* IN */ UINT16     data_len
           )
{
    struct net_buf *buf;

    assert(data_len <= SMP_LE_RX_PDU);

    LOG_DBG("event %d", event);

    buf = net_buf_alloc(&smp_le_rx_pool, 0);
    if (NULL != buf)
    {
        struct bt_smp_hdr_sumilation hdr;

        net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, BT_BUF_ACL_IN);

        hdr.hdr.handler = hci_acl_smp_handler;
        hdr.hdr.len = sizeof(struct smp_le_rx_pdu) + data_len;
        hdr.pdu.bd_handle = *bd_handle;
        hdr.pdu.event = event;
        hdr.pdu.status = status;

        (void)net_buf_add_mem(buf, &hdr, sizeof(hdr));
        (void)net_buf_add_mem(buf, eventdata, data_len);
        LOG_DBG("RX queue put buf %p", buf);
        bt_recv(buf);
        return API_SUCCESS;
    }
    else
    {
        LOG_ERR("Get RX buf failure!");
        return API_FAILURE;
    }
}

static int ethermind_bt_smp_init(void)
{
    SMP_BD_ADDR localAddr;
    API_RESULT retVal;
    UCHAR keyDistribution = 0;
#if (defined(CONFIG_BT_SMP_SC_ONLY) && (CONFIG_BT_SMP_SC_ONLY > 0))
    /* for Local */
    keyDistribution = SEND_KEYS_SC;
    /* for Remote */
    keyDistribution |= (RECV_KEYS_SC) << 4;
#else
    /* for Local */
    keyDistribution = SEND_KEYS;
    /* for Remote */
    keyDistribution |= (RECV_KEYS) << 4;
#endif /* (defined(CONFIG_BT_SMP_SC_ONLY) && (CONFIG_BT_SMP_SC_ONLY > 0)) */
    retVal = BT_smp_set_key_distribution_flag_pl(keyDistribution);
    if (API_SUCCESS != retVal)
    {
        return -EIO;
    }

    retVal = BT_smp_set_io_cap_pl(SMP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    if (API_SUCCESS == retVal)
    {
        /* Setting the local Public Address as Identity Address */
        retVal = BT_hci_get_local_bd_addr
        (
            localAddr.addr
        );
    }

    if (API_SUCCESS == retVal)
    {
        localAddr.type = BT_BD_PUBLIC_ADDRESS_TYPE;
        retVal = BT_smp_set_local_identity_addr(&localAddr);
    }
#if 1
    if (API_SUCCESS == retVal)
    {
        retVal = BT_smp_register_user_interface (ethermind_bt_smp_cb);
    }
#endif
    if (API_SUCCESS == retVal)
    {
        return 0;
    }
    else
    {
        return -EIO;
    }
}

int bt_smp_init(void)
{
#if 0
	static struct bt_pub_key_cb pub_key_cb = {
		.func           = bt_smp_pkey_ready,
	};
#endif
	int ret;

    if (NULL == sc_local_pkey_ready)
    {
        osa_status_t ret = OSA_SemaphoreCreate((osa_semaphore_handle_t)sc_local_pkey_ready_handle, 0);
        assert(KOSA_StatusSuccess == ret);

        if (KOSA_StatusSuccess == ret)
        {
            sc_local_pkey_ready = (osa_semaphore_handle_t)sc_local_pkey_ready_handle;
        }
    }

    if (NULL == sc_local_oobe_ready)
    {
        osa_status_t ret = OSA_SemaphoreCreate((osa_semaphore_handle_t)sc_local_oobe_ready_handle, 0);
        assert(KOSA_StatusSuccess == ret);

        if (KOSA_StatusSuccess == ret)
        {
            sc_local_oobe_ready = (osa_semaphore_handle_t)sc_local_oobe_ready_handle;
        }
    }

	ret = ethermind_bt_smp_init();
	assert (0 == ret);
	if (0 != ret)
	{
		return ret;
	}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	ret = ethermind_bt_sm_init();
	assert (0 == ret);
	if (0 != ret)
	{
		return ret;
	}
#endif

#if (defined(CONFIG_BT_SMP_SC_PAIR_ONLY) && (CONFIG_BT_SMP_SC_PAIR_ONLY > 0U))
    /* fix GAP/SEC/SEM/BV-28-C */
    BT_smp_set_lesc_policy_pl(SMP_PL_LESC_STRICT);
#endif
	/* pass SM/CEN/SCJW/BI-01-C */
	BT_smp_set_mitm_policy_pl(0);
	sc_supported = le_sc_supported();
	if (IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY) && !sc_supported) {
		LOG_ERR("SC Pair Only Mode selected but LE SC not supported");
		return -ENOENT;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_USB_HCI_CTLR_WORKAROUND)) {
		LOG_WRN("BT_SMP_USB_HCI_CTLR_WORKAROUND is enabled, which "
			"exposes a security vulnerability!");
	}

	LOG_DBG("LE SC %s", sc_supported ? "enabled" : "disabled");
#if 0
	if (!IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		bt_pub_key_gen(&pub_key_cb);
	}
#endif

	(void)sc_public_key;

	return smp_self_test();
}


static void bt_smp_le_update_io_cap(const struct bt_conn_auth_cb *auth)
{
    uint8_t ioCap;

    if (NULL == auth)
	{
        if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
            fixed_passkey != BT_PASSKEY_INVALID)
        {
            ioCap = SMP_IO_CAPABILITY_KEYBOARD_ONLY;
        }
        else
        {
            ioCap = SMP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
        }
	}
	else
	{
		if ((NULL != auth->passkey_display)
		&& (NULL != auth->passkey_entry)
        && ((NULL != auth->passkey_confirm) || (0U == sc_supported)))
		{
			ioCap = SMP_IO_CAPABILITY_KEYBOARD_DISPLAY;
		}
		else if ((NULL != auth->passkey_display)
        && (NULL != auth->passkey_confirm)
        && (1U == sc_supported))
		{
            ioCap = SMP_IO_CAPABILITY_DISPLAY_YESNO;
        }
        else if ((NULL != auth->passkey_entry))
        {
			if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
				fixed_passkey != BT_PASSKEY_INVALID)
			{
				ioCap = SMP_IO_CAPABILITY_KEYBOARD_DISPLAY;
			}
			else
			{
				ioCap = SMP_IO_CAPABILITY_KEYBOARD_ONLY;
			}
		}
		else if ((NULL != auth->passkey_display))
		{
			ioCap = SMP_IO_CAPABILITY_DISPLAY_ONLY;
		}
		else
		{
            if (IS_ENABLED(CONFIG_BT_FIXED_PASSKEY) &&
                fixed_passkey != BT_PASSKEY_INVALID)
            {
                ioCap = SMP_IO_CAPABILITY_KEYBOARD_ONLY;
            }
            else
            {
                ioCap = SMP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
            }
		}
	}
	BT_smp_set_io_cap_pl(ioCap);
}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
int bt_smp_set_ct2(struct bt_conn *conn, uint8_t enable)
{
    struct bt_smp_br *smp;

    smp = smp_br_chan_get(conn);
    if (!smp) {
            return -ENOTCONN;
    }
    if (enable) {
        atomic_set_bit(smp->flags, SMP_FLAG_CT2);
    }else {
        atomic_clear_bit(smp->flags, SMP_FLAG_CT2);
    }
	return 0;
}
#endif /* CONFIG_BT_CLASSIC */

void bt_smp_update_io_cap(const struct bt_conn_auth_cb *auth)
{
	bt_smp_le_update_io_cap(auth);
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	bt_smp_br_update_io_cap(auth);
#endif /* CONFIG_BT_CLASSIC */
}

bool bt_smp_is_bonded(struct bt_conn *conn)
{
	SMP_AUTH_INFO le_auth_info;
	API_RESULT retval;
	LOG_DBG("Conn bound?");

	retval = BT_smp_get_device_security_info (&conn->deviceId, &le_auth_info);
	if (API_SUCCESS == retval)
	{
        if (SMP_TRUE == le_auth_info.bonding)
        {
            LOG_DBG("true");
            return true;
        }
	}
    LOG_DBG("false");
    return false;
}

#endif /* CONFIG_BT_BLE_DISABLE */
#endif /* CONFIG_BT_SMP */
