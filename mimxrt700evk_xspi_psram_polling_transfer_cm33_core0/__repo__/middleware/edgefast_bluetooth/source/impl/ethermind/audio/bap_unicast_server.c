/*  Bluetooth Audio Unicast Server */

/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if (defined(CONFIG_BT_BAP_UNICAST_SERVER) && (CONFIG_BT_BAP_UNICAST_SERVER > 0))

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <porting.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/bap.h>
#include <bluetooth/conn.h>
#include <bluetooth/iso.h>

#include "ascs_internal.h"
#include "bap_iso.h"
#include "bap_endpoint.h"
#include "pacs_internal.h"

#define LOG_ENABLE IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_UNICAST_SERVER)
#define LOG_MODULE_NAME bt_unicast_server
#include "fsl_component_log.h"
LOG_MODULE_DEFINE(LOG_MODULE_NAME, kLOG_LevelTrace);

static const struct bt_bap_unicast_server_cb *unicast_server_cb;

int bt_bap_unicast_server_register_cb(const struct bt_bap_unicast_server_cb *cb)
{
	int err;

	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	if (unicast_server_cb != NULL) {
		LOG_DBG("callback structure already registered");
		return -EALREADY;
	}

	err = bt_ascs_init(cb);
	if (err != 0) {
		return err;
	}

	unicast_server_cb = cb;

	return 0;
}

int bt_bap_unicast_server_unregister_cb(const struct bt_bap_unicast_server_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	if (unicast_server_cb != cb) {
		LOG_DBG("callback structure not registered");
		return -EINVAL;
	}

	bt_ascs_cleanup();
	unicast_server_cb = NULL;

	return 0;
}

int bt_bap_unicast_server_reconfig(struct bt_bap_stream *stream,
				   const struct bt_audio_codec_cfg *codec_cfg)
{
	struct bt_bap_ep *ep;
	struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS,
						     BT_BAP_ASCS_REASON_NONE);
	int err;

	ep = stream->ep;

	if (unicast_server_cb != NULL &&
		unicast_server_cb->reconfig != NULL) {
		err = unicast_server_cb->reconfig(stream, (enum bt_audio_dir)ep->dir, codec_cfg, &ep->qos_pref, &rsp);
	} else {
		err = -ENOTSUP;
	}

	if (err != 0) {
		return err;
	}

	(void)memcpy(&ep->codec_cfg, codec_cfg, sizeof(*codec_cfg));

	return ascs_ep_set_state(ep, BT_BAP_EP_STATE_CODEC_CONFIGURED);
}

int bt_bap_unicast_server_start(struct bt_bap_stream *stream)
{
	struct bt_bap_ep *ep = stream->ep;

	if (ep->dir != BT_AUDIO_DIR_SINK) {
		LOG_DBG("Invalid operation for stream %p with dir %u",
			stream, ep->dir);

		return -EINVAL;
	}

	/* If ISO is connected to go streaming state,
	 * else wait for ISO to be connected
	 */
	if (ep->iso->chan.state == BT_ISO_STATE_CONNECTED) {
		return ascs_ep_set_state(ep, BT_BAP_EP_STATE_STREAMING);
	}

	ep->receiver_ready = true;

	return 0;
}

int bt_bap_unicast_server_metadata(struct bt_bap_stream *stream, const uint8_t meta[],
				   size_t meta_len)
{
	struct bt_bap_ep *ep;
	struct bt_bap_ascs_rsp rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_SUCCESS,
						     BT_BAP_ASCS_REASON_NONE);
	int err;

	if (meta_len > sizeof(ep->codec_cfg.meta)) {
		return -ENOMEM;
	}

	if (unicast_server_cb != NULL && unicast_server_cb->metadata != NULL) {
		err = unicast_server_cb->metadata(stream, meta, meta_len, &rsp);
	} else {
		err = -ENOTSUP;
	}


	if (err) {
		LOG_ERR("Metadata failed: err %d, code %u, reason %u", err, rsp.code, rsp.reason);
		return err;
	}

	ep = stream->ep;
	(void)memcpy(ep->codec_cfg.meta, meta, meta_len);

	/* Set the state to the same state to trigger the notifications */
	return ascs_ep_set_state(ep, ep->status.state);
}

int bt_bap_unicast_server_disable(struct bt_bap_stream *stream)
{
	return bt_ascs_disable_ase(stream->ep);
}

int bt_bap_unicast_server_release(struct bt_bap_stream *stream)
{
	return bt_ascs_release_ase(stream->ep);
}

int bt_bap_unicast_server_config_ase(struct bt_conn *conn, struct bt_bap_stream *stream,
				     struct bt_audio_codec_cfg *codec_cfg,
				     const struct bt_audio_codec_qos_pref *qos_pref)
{
	return bt_ascs_config_ase(conn, stream, codec_cfg, qos_pref);
}

void bt_bap_unicast_server_foreach_ep(struct bt_conn *conn, bt_bap_ep_func_t func, void *user_data)
{
	bt_ascs_foreach_ep(conn, func, user_data);
}

#endif /* CONFIG_BT_BAP_UNICAST_SERVER */
