/* conn.c - Bluetooth connection handling */

/*
 * Copyright 2021 NXP
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <porting.h>
#include <string.h>
#include <errno/errno.h>
#include <stdbool.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/check.h>
#include <sys/util.h>
#include <sys/slist.h>


#include "BT_att_api.h"
#include "BT_gatt_db_api.h"
#include "sco_audio_pl.h"

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/direction.h>
#include <bluetooth/conn.h>
#include <bluetooth/att.h>

#define LOG_ENABLE IS_ENABLED(CONFIG_BT_DEBUG_CONN)
#define LOG_MODULE_NAME bt_conn
#include "fsl_component_log.h"
LOG_MODULE_DEFINE(LOG_MODULE_NAME, kLOG_LevelTrace);

#include "bt_pal_buf_view.h"
#include "bt_pal_hci_core.h"
#include "bt_pal_id.h"
#include "bt_pal_adv.h"
#include "bt_pal_conn_internal.h"
#include "bt_pal_l2cap_internal.h"
#include "bt_pal_keys.h"
#include "bt_pal_smp.h"
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
#include "bt_pal_ssp.h"
#include "bt_pal_l2cap_br_interface.h"
#include "bt_pal_sco_internal.h"
#endif /* CONFIG_BT_CLASSIC > 0U */
#include "bt_pal_att_internal.h"
#include "bt_pal_gatt_internal.h"
#include "bt_pal_direction_internal.h"
#if (defined(CONFIG_BT_ISO) && ((CONFIG_BT_ISO) > 0))
#include "bt_pal_iso_internal.h"
#endif
#include "bt_pal_addr_internal.h"

/* Peripheral timeout to initialize Connection Parameter Update procedure */
#define CONN_UPDATE_TIMEOUT K_MSEC(CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT)

/* CONN */
STRUCT_SECTION_DEFINE(bt_conn_cb);

K_FIFO_DEFINE(free_tx);

static void tx_free(struct bt_conn_tx *tx);

static void conn_tx_destroy(struct bt_conn *conn, struct bt_conn_tx *tx)
{
	__ASSERT_NO_MSG(tx);

	bt_conn_tx_cb_t cb = tx->cb;
	void *user_data = tx->user_data;

	LOG_DBG("conn %p tx %p cb %p ud %p", conn, tx, cb, user_data);

	/* Free up TX metadata before calling callback in case the callback
	 * tries to allocate metadata
	 */
	tx_free(tx);

	if (cb) {
		cb(conn, user_data, -ESHUTDOWN);
	}
}

#if (defined(CONFIG_BT_CONN_TX) && ((CONFIG_BT_CONN_TX) > 0))
static void tx_complete_work(struct k_work *work);
#endif /* CONFIG_BT_CONN_TX */

static void notify_recycled_conn_slot(void);

void bt_tx_irq_raise(void);

/* Group Connected BT_CONN only in this */
#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0))
/* Peripheral timeout to initialize Connection Parameter Update procedure */
#define CONN_UPDATE_TIMEOUT  K_MSEC(CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT)

static void deferred_work(struct k_work *work);
static void notify_connected(struct bt_conn *conn);

static struct bt_conn acl_conns[CONFIG_BT_MAX_CONN];
NET_BUF_POOL_DEFINE(acl_tx_pool, CONFIG_BT_L2CAP_TX_BUF_COUNT,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#if ((defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U)) || (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U)))
const struct bt_conn_auth_cb *bt_auth;
sys_slist_t bt_auth_info_cbs = SYS_SLIST_STATIC_INIT(&bt_auth_info_cbs);
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC */


static sys_slist_t conn_cbs = SYS_SLIST_STATIC_INIT(&conn_cbs);

static struct bt_conn_tx conn_tx[CONFIG_BT_CONN_TX_MAX];

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
static int bt_hci_connect_br_cancel(struct bt_conn *conn);

static struct bt_conn sco_conns[CONFIG_BT_MAX_SCO_CONN];
#endif /* CONFIG_BT_CLASSIC */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CONN_TX)
void frag_destroy(struct net_buf *buf);

/* Storage for fragments (views) into the upper layers' PDUs. */
/* TODO: remove user-data requirements */
NET_BUF_POOL_FIXED_DEFINE(fragments, CONFIG_BT_CONN_FRAG_COUNT, CONFIG_BT_CONN_TX_USER_DATA_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, frag_destroy);

struct frag_md {
	struct bt_buf_view_meta view_meta;
};
struct frag_md frag_md_pool[CONFIG_BT_CONN_FRAG_COUNT];

struct frag_md *get_frag_md(struct net_buf *fragment)
{
	return &frag_md_pool[net_buf_id(fragment)];
}

void frag_destroy(struct net_buf *frag)
{
	/* allow next view to be allocated (and unlock the parent buf) */
	bt_buf_destroy_view(frag, &get_frag_md(frag)->view_meta);

	LOG_DBG("");

	/* Kick the TX processor to send the rest of the frags. */
	bt_tx_irq_raise();
}

static struct net_buf *get_data_frag(struct net_buf *outside, size_t winsize)
{
	struct net_buf *window;

	__ASSERT_NO_MSG(!bt_buf_has_view(outside));

	/* Keeping a ref is the caller's responsibility */
	window = net_buf_alloc_len(&fragments, 0, K_NO_WAIT);
	if (!window) {
		return window;
	}

	window = bt_buf_make_view(window, outside,
				  winsize, &get_frag_md(window)->view_meta);

	LOG_DBG("get-acl-frag: outside %p window %p size %d", outside, window, winsize);

	return window;
}
#else /* !CONFIG_BT_CONN_TX */
static struct net_buf *get_data_frag(struct net_buf *outside, size_t winsize)
{
	ARG_UNUSED(outside);
	ARG_UNUSED(winsize);

	/* This will never get called. It's only to allow compilation to take
	 * place and the later linker stage to remove this implementation.
	 */

	return NULL;
}
#endif /* CONFIG_BT_CONN_TX */

#if 0
STRUCT_SECTION_START_EXTERN(bt_conn_cb);
STRUCT_SECTION_END_EXTERN(bt_conn_cb);
#endif

#if (defined(CONFIG_BT_ISO) && ((CONFIG_BT_ISO) > 0))
extern struct bt_conn iso_conns[CONFIG_BT_ISO_MAX_CHAN];

/* Callback TX buffers for ISO */
static struct bt_conn_tx iso_tx[CONFIG_BT_ISO_TX_BUF_COUNT];

int bt_conn_iso_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(iso_tx); i++) {
		k_fifo_put(&free_tx, &iso_tx[i]);
	}

	return 0;
}
#endif /* CONFIG_BT_ISO */

struct k_sem *bt_conn_get_pkts(struct bt_conn *conn)
{
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.acl_mtu) {
		return &bt_dev.br.pkts;
	}
#endif /* CONFIG_BT_CLASSIC */

#if (defined(CONFIG_BT_ISO) && ((CONFIG_BT_ISO) > 0U))
	/* Use ISO pkts semaphore if LE Read Buffer Size command returned
	 * dedicated ISO buffers.
	 */
	if (conn->type == BT_CONN_TYPE_ISO) {
		if (bt_dev.le.iso_mtu && bt_dev.le.iso_limit != 0) {
			return &bt_dev.le.iso_pkts;
		}

		return NULL;
	}
#endif /* CONFIG_BT_ISO */

#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0U))
	if (bt_dev.le.acl_mtu) {
		return &bt_dev.le.acl_pkts;
	}
#endif /* CONFIG_BT_CONN */

	return NULL;
}

static inline const char *state2str(bt_conn_state_t state)
{
	switch (state) {
	case BT_CONN_DISCONNECTED:
		return "disconnected";
	case BT_CONN_DISCONNECT_COMPLETE:
		return "disconnect-complete";
	case BT_CONN_INITIATING:
		return "initiating";
	case BT_CONN_SCAN_BEFORE_INITIATING:
		return "scan-before-initiating";
	case BT_CONN_INITIATING_FILTER_LIST:
		return "initiating-filter-list";
	case BT_CONN_ADV_CONNECTABLE:
		return "adv-connectable";
	case BT_CONN_ADV_DIR_CONNECTABLE:
		return "adv-dir-connectable";
	case BT_CONN_CONNECTED:
		return "connected";
	case BT_CONN_DISCONNECTING:
		return "disconnecting";
	default:
		return "(unknown)";
	}
}

static void tx_free(struct bt_conn_tx *tx)
{
	LOG_DBG("%p", tx);
	tx->cb = NULL;
	tx->user_data = NULL;
	k_fifo_put(&free_tx, tx);
}

#if defined(CONFIG_BT_CONN_TX)
static void tx_notify(struct bt_conn *conn)
{
	__ASSERT_NO_MSG(k_current_get() ==
			k_work_queue_thread_get(&k_sys_work_q));

	LOG_DBG("conn %p", conn);

	while (1) {
		struct bt_conn_tx *tx = NULL;
		unsigned int key;
		bt_conn_tx_cb_t cb;
		void *user_data;

		key = irq_lock();
		if (!sys_slist_is_empty(&conn->tx_complete)) {
			const sys_snode_t *node = sys_slist_get_not_empty(&conn->tx_complete);

			tx = CONTAINER_OF(node, struct bt_conn_tx, node);
		}
		irq_unlock(key);

		if (!tx) {
			return;
		}

		LOG_DBG("tx %p cb %p user_data %p", tx, tx->cb, tx->user_data);

		/* Copy over the params */
		cb = tx->cb;
		user_data = tx->user_data;

		/* Free up TX notify since there may be user waiting */
		tx_free(tx);

		/* Run the callback, at this point it should be safe to
		 * allocate new buffers since the TX should have been
		 * unblocked by tx_free.
		 */
		if (cb) {
			cb(conn, user_data, 0);
		}

		LOG_DBG("raise TX IRQ");
		bt_tx_irq_raise();
	}
}
#endif	/* CONFIG_BT_CONN_TX */

struct bt_conn *bt_conn_new(struct bt_conn *conns, size_t size)
{
	struct bt_conn *conn = NULL;
	int i;

	for (i = 0; i < size; i++) {
		if (atomic_cas(&conns[i].ref, 0, 1)) {
			conn = &conns[i];
			break;
		}
	}

	if (!conn) {
		return NULL;
	}

	(void)memset(conn, 0, offsetof(struct bt_conn, ref));

	/* Init security level semaphore resource. */
	k_sem_init(&conn->sec_lvl_updated, 1, 1);

#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0))
	k_work_init_delayable(&conn->deferred_work, deferred_work);
#endif /* CONFIG_BT_CONN */
#if (defined(CONFIG_BT_CONN_TX) && (CONFIG_BT_CONN_TX > 0))
	k_work_init(&conn->tx_complete_work, tx_complete_work);
#endif /* CONFIG_BT_CONN_TX */

	return conn;
}

void bt_conn_reset_rx_state(struct bt_conn *conn)
{
	if (!conn->rx) {
		return;
	}

	net_buf_unref(conn->rx);
	conn->rx = NULL;
}

static void bt_acl_recv(struct bt_conn *conn, struct net_buf *buf,
			uint8_t flags)
{
	uint16_t acl_total_len;

	/* Check packet boundary flags */
	switch (flags) {
	case BT_ACL_START:
		if (conn->rx) {
			LOG_ERR("Unexpected first L2CAP frame");
			bt_conn_reset_rx_state(conn);
		}

		LOG_DBG("First, len %u final %u", buf->len,
			(buf->len < sizeof(uint16_t)) ? 0 : sys_get_le16(buf->data));

		conn->rx = buf;
		break;
	case BT_ACL_CONT:
		if (!conn->rx) {
			LOG_ERR("Unexpected L2CAP continuation");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		if (!buf->len) {
			LOG_DBG("Empty ACL_CONT");
			net_buf_unref(buf);
			return;
		}

		if (buf->len > net_buf_tailroom(conn->rx)) {
			LOG_ERR("Not enough buffer space for L2CAP data");

			/* Frame is not complete but we still pass it to L2CAP
			 * so that it may handle error on protocol level
			 * eg disconnect channel.
			 */
			bt_l2cap_recv(conn, conn->rx, false);
			conn->rx = NULL;
			net_buf_unref(buf);
			return;
		}

		net_buf_add_mem(conn->rx, buf->data, buf->len);
		net_buf_unref(buf);
		break;
	default:
		/* BT_ACL_START_NO_FLUSH and BT_ACL_COMPLETE are not allowed on
		 * LE-U from Controller to Host.
		 * Only BT_ACL_POINT_TO_POINT is supported.
		 */
		LOG_ERR("Unexpected ACL flags (0x%02x)", flags);
		bt_conn_reset_rx_state(conn);
		net_buf_unref(buf);
		return;
	}

	if (conn->rx->len < sizeof(uint16_t)) {
		/* Still not enough data received to retrieve the L2CAP header
		 * length field.
		 */
		return;
	}

	acl_total_len = sys_get_le16(conn->rx->data) + sizeof(struct bt_l2cap_hdr);

	if (conn->rx->len < acl_total_len) {
		/* L2CAP frame not complete. */
		return;
	}

	if (conn->rx->len > acl_total_len) {
		LOG_ERR("ACL len mismatch (%u > %u)", conn->rx->len, acl_total_len);
		bt_conn_reset_rx_state(conn);
		return;
	}

	/* L2CAP frame complete. */
	buf = conn->rx;
	conn->rx = NULL;

	LOG_DBG("Successfully parsed %u byte L2CAP packet", buf->len);
	bt_l2cap_recv(conn, buf, true);
}

static void wait_for_tx_work(struct bt_conn *conn)
{
#if defined(CONFIG_BT_CONN_TX)
	LOG_DBG("conn %p", conn);

	if (IS_ENABLED(CONFIG_BT_RECV_WORKQ_SYS) ||
	    k_current_get() == k_work_queue_thread_get(&k_sys_work_q)) {
		tx_notify(conn);
	} else {
		struct k_work_sync sync;
		int err;

		err = k_work_submit(&conn->tx_complete_work);
		__ASSERT(err >= 0, "couldn't submit (err %d)", err);

		k_work_flush(&conn->tx_complete_work, &sync);
		(void)err;
	}
	LOG_DBG("done");
#else
	ARG_UNUSED(conn);
#endif	/* CONFIG_BT_CONN_TX */
}

void bt_conn_recv(struct bt_conn *conn, struct net_buf *buf, uint8_t flags)
{
	/* Make sure we notify any pending TX callbacks before processing
	 * new data for this connection.
	 *
	 * Always do so from the same context for sanity. In this case that will
	 * be the system workqueue.
	 */
	wait_for_tx_work(conn);

	LOG_DBG("handle %u len %u flags %02x", conn->handle, buf->len, flags);

	if (IS_ENABLED(CONFIG_BT_ISO_RX) && conn->type == BT_CONN_TYPE_ISO) {
#if (defined(CONFIG_BT_ISO_RX) && (CONFIG_BT_ISO_RX > 0))
		bt_iso_recv(conn, buf, flags);
#endif /* CONFIG_BT_ISO_RX */
		return;
	} else if (IS_ENABLED(CONFIG_BT_CONN)) {
		bt_acl_recv(conn, buf, flags);
	} else {
		__ASSERT(false, "Invalid connection type %u", conn->type);
	}
}

static bool dont_have_tx_context(struct bt_conn *conn)
{
	return k_fifo_is_empty(&free_tx);
}

static struct bt_conn_tx *conn_tx_alloc(void)
{
	struct bt_conn_tx *ret = k_fifo_get(&free_tx, K_NO_WAIT);

	LOG_DBG("%p", ret);

	return ret;
}

enum {
	FRAG_START,
	FRAG_CONT,
	FRAG_SINGLE,
	FRAG_END
};

static int send_acl(struct bt_conn *conn, struct net_buf *buf, uint8_t flags)
{
	struct bt_hci_acl_hdr *hdr;

	switch (flags) {
	case FRAG_START:
	case FRAG_SINGLE:
		flags = BT_ACL_START_NO_FLUSH;
		break;
	case FRAG_CONT:
	case FRAG_END:
		flags = BT_ACL_CONT;
		break;
	default:
		return -EINVAL;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_acl_handle_pack(conn->handle, flags));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));

	bt_buf_set_type(buf, BT_BUF_ACL_OUT);

	return bt_send(buf);
}

#if (defined(CONFIG_BT_ISO_TX) && (CONFIG_BT_ISO_TX > 0))
static int send_iso(struct bt_conn *conn, struct net_buf *buf, uint8_t flags)
{
	struct bt_hci_iso_hdr *hdr;
	enum bt_iso_timestamp ts;

	switch (flags) {
	case FRAG_START:
		flags = BT_ISO_START;
		break;
	case FRAG_CONT:
		flags = BT_ISO_CONT;
		break;
	case FRAG_SINGLE:
		flags = BT_ISO_SINGLE;
		break;
	case FRAG_END:
		flags = BT_ISO_END;
		break;
	default:
		return -EINVAL;
	}

	/* The TS bit is set by `iso.c:conn_iso_send`. This special byte
	 * prepends the whole SDU, and won't be there for individual fragments.
	 *
	 * Conveniently, it is only legal to set the TS bit on the first HCI
	 * fragment, so we don't have to pass this extra metadata around for
	 * every fragment, only the first one.
	 */
	if (flags == BT_ISO_SINGLE || flags == BT_ISO_START) {
		ts = (enum bt_iso_timestamp)net_buf_pull_u8(buf);
	} else {
		ts = BT_ISO_TS_ABSENT;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_iso_handle_pack(conn->handle, flags, ts));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));

	bt_buf_set_type(buf, BT_BUF_ISO_OUT);

	return bt_send(buf);
}
#endif /* CONFIG_BT_ISO_TX */

static inline uint16_t conn_mtu(struct bt_conn *conn)
{
#if (defined(CONFIG_BT_CLASSIC) && (CONFIG_BT_CLASSIC > 0U))
	if (conn->type == BT_CONN_TYPE_BR ||
	    (conn->type != BT_CONN_TYPE_ISO && !bt_dev.le.acl_mtu)) {
		return bt_dev.br.mtu;
	}
#endif /* CONFIG_BT_CLASSIC */
#if (defined(CONFIG_BT_ISO) && (CONFIG_BT_ISO > 0U))
	if (conn->type == BT_CONN_TYPE_ISO) {
		return bt_dev.le.iso_mtu;
	}
#endif /* CONFIG_BT_ISO */
#if (defined(CONFIG_BT_CONN) && (CONFIG_BT_CONN > 0U))
	return bt_dev.le.acl_mtu;
#else
	return 0;
#endif /* CONFIG_BT_CONN */
}

static bool is_classic_conn(struct bt_conn *conn)
{
	return (IS_ENABLED(CONFIG_BT_CLASSIC) &&
		conn->type == BT_CONN_TYPE_BR);
}

static bool is_iso_tx_conn(struct bt_conn *conn)
{
	return IS_ENABLED(CONFIG_BT_ISO_TX) &&
		conn->type == BT_CONN_TYPE_ISO;
}

static bool is_le_conn(struct bt_conn *conn)
{
	return IS_ENABLED(CONFIG_BT_CONN) && conn->type == BT_CONN_TYPE_LE;
}

static bool is_acl_conn(struct bt_conn *conn)
{
	return is_le_conn(conn) || is_classic_conn(conn);
}

static int send_buf(struct bt_conn *conn, struct net_buf *buf,
		    size_t len, void *cb, void *ud)
{
	struct net_buf *frag = NULL;
	struct bt_conn_tx *tx = NULL;
	uint8_t flags;
	int err;

	if (buf->len == 0) {
		__ASSERT_NO_MSG(0);

		return -EMSGSIZE;
	}

	if (bt_buf_has_view(buf)) {
		__ASSERT_NO_MSG(0);

		return -EIO;
	}

	LOG_DBG("conn %p buf %p len %u buf->len %u cb %p ud %p",
		conn, buf, len, buf->len, cb, ud);

	/* Acquire the right to send 1 packet to the controller */
	if (k_sem_take(bt_conn_get_pkts(conn), K_NO_WAIT)) {
		/* This shouldn't happen now that we acquire the resources
		 * before calling `send_buf` (in `get_conn_ready`). We say
		 * "acquire" as `tx_processor()` is not re-entrant and the
		 * thread is non-preemptible. So the sem value shouldn't change.
		 */
		__ASSERT(0, "No controller bufs");

		return -ENOMEM;
	}

	/* Allocate and set the TX context */
	tx = conn_tx_alloc();

	/* See big comment above */
	if (!tx) {
		__ASSERT(0, "No TX context");

		return -ENOMEM;
	}

	tx->cb = (bt_conn_tx_cb_t)cb;
	tx->user_data = ud;

	uint16_t frag_len = MIN(conn_mtu(conn), len);

	__ASSERT_NO_MSG(buf->ref == 1);

	if (buf->len > frag_len) {
		LOG_DBG("keep %p around", buf);
		frag = get_data_frag(net_buf_ref(buf), frag_len);
	} else {
		LOG_DBG("move %p ref in", buf);
		/* Move the ref into `frag` for the last TX. That way `buf` will
		 * get destroyed when `frag` is destroyed.
		 */
		frag = get_data_frag(buf, frag_len);
	}

	/* Caller is supposed to check we have all resources to send */
	__ASSERT_NO_MSG(frag != NULL);

	/* If the current buffer doesn't fit a controller buffer */
	if (len > conn_mtu(conn)) {
		flags = conn->next_is_frag ? FRAG_CONT : FRAG_START;
		conn->next_is_frag = true;
	} else {
		flags = conn->next_is_frag ? FRAG_END : FRAG_SINGLE;
		conn->next_is_frag = false;
	}

	LOG_DBG("send frag: buf %p len %d", buf, frag_len);

	/* At this point, the buffer is either a fragment or a full HCI packet.
	 * The flags are also valid.
	 */
	LOG_DBG("conn %p buf %p len %u flags 0x%02x",
		conn, frag, frag->len, flags);

	/* Keep track of sent buffers. We have to append _before_
	 * sending, as we might get pre-empted if the HCI driver calls
	 * k_yield() before returning.
	 *
	 * In that case, the controller could also send a num-complete-packets
	 * event and our handler will be confused that there is no corresponding
	 * callback node in the `tx_pending` list.
	 */
	atomic_inc(&conn->in_ll);
	sys_slist_append(&conn->tx_pending, &tx->node);

#if (defined(CONFIG_BT_ISO_TX) && (CONFIG_BT_ISO_TX > 0))
	if (is_iso_tx_conn(conn)) {
		err = send_iso(conn, frag, flags);
	} else
#endif /* CONFIG_BT_ISO_TX */
	if (is_acl_conn(conn)) {
		err = send_acl(conn, frag, flags);
	} else {
		err = -EINVAL;	/* Some animals disable asserts */
		__ASSERT(false, "Invalid connection type %u", conn->type);
	}

	if (!err) {
		return 0;
	}

	/* Remove buf from pending list */
	atomic_dec(&conn->in_ll);
	(void)sys_slist_find_and_remove(&conn->tx_pending, &tx->node);

	LOG_ERR("Unable to send to driver (err %d)", err);

	/* If we get here, something has seriously gone wrong: the `parent` buf
	 * (of which the current fragment belongs) should also be destroyed.
	 */
	net_buf_unref(frag);

	/* `buf` might not get destroyed right away, and its `tx`
	 * pointer will still be reachable. Make sure that we don't try
	 * to use the destroyed context later.
	 */
	conn_tx_destroy(conn, tx);
	k_sem_give(bt_conn_get_pkts(conn));

	/* Merge HCI driver errors */
	return -EIO;
}

static void conn_destroy(struct bt_conn *conn, void *data)
{
	if (conn->state == BT_CONN_CONNECTED ||
	    conn->state == BT_CONN_DISCONNECTING) {
		bt_conn_set_state(conn, BT_CONN_DISCONNECT_COMPLETE);
	}

	if (conn->state != BT_CONN_DISCONNECTED) {
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
	}
}

void bt_conn_cleanup_all(void)
{
	bt_conn_foreach(BT_CONN_TYPE_ALL, conn_destroy, NULL);
}

#if defined(CONFIG_BT_CONN)
/* Returns true if L2CAP has data to send on this conn */
static bool acl_has_data(struct bt_conn *conn)
{
	return sys_slist_peek_head(&conn->l2cap_data_ready) != NULL;
}
#endif	/* defined(CONFIG_BT_CONN) */

/* Connection "Scheduler" of sorts:
 *
 * Will try to get the optimal number of queued buffers for the connection.
 *
 * Partitions the controller's buffers to each connection according to some
 * heuristic. This is made to be tunable, fairness, simplicity, throughput etc.
 *
 * In the future, this will be a hook exposed to the application.
 */
static bool should_stop_tx(struct bt_conn *conn)
{
	LOG_DBG("%p", conn);

	if (conn->state != BT_CONN_CONNECTED) {
		return true;
	}

	/* TODO: This function should be overridable by the application: they
	 * should be able to provide their own heuristic.
	 */
	if (!conn->has_data(conn)) {
		LOG_DBG("No more data for %p", conn);
		return true;
	}

	/* Queue only 3 buffers per-conn for now */
	if (atomic_get(&conn->in_ll) < 3) {
		/* The goal of this heuristic is to allow the link-layer to
		 * extend an ACL connection event as long as the application
		 * layer can provide data.
		 *
		 * Here we chose three buffers, as some LLs need two enqueued
		 * packets to be able to set the more-data bit, and one more
		 * buffer to allow refilling by the app while one of them is
		 * being sent over-the-air.
		 */
		return false;
	}

	return true;
}

void bt_conn_data_ready(struct bt_conn *conn)
{
	LOG_DBG("DR");

	/* The TX processor will call the `pull_cb` to get the buf */
	if (!atomic_set(&conn->_conn_ready_lock, 1)) {
		/* Attach a reference to the `bt_dev.le.conn_ready` list.
		 *
		 * This reference will be consumed when the conn is popped off
		 * the list (in `get_conn_ready`).
		 */
		bt_conn_ref(conn);
		sys_slist_append(&bt_dev.le.conn_ready,
				 &conn->_conn_ready);
		LOG_DBG("raised");
	} else {
		LOG_DBG("already in list");
	}

	/* Kick the TX processor */
	bt_tx_irq_raise();
}

static bool cannot_send_to_controller(struct bt_conn *conn)
{
	return k_sem_count_get(bt_conn_get_pkts(conn)) == 0;
}

static bool dont_have_viewbufs(void)
{
#if defined(CONFIG_BT_CONN_TX)
	/* The LIFO only tracks buffers that have been destroyed at least once,
	 * hence the uninit check beforehand.
	 */
	if (fragments.uninit_count > 0) {
		/* If there are uninitialized bufs, we are guaranteed allocation. */
		return false;
	}

	/* In practice k_fifo == k_lifo ABI. */
	return k_fifo_is_empty(&fragments.free);

#else  /* !CONFIG_BT_CONN_TX */
	return false;
#endif	/* CONFIG_BT_CONN_TX */
}

__maybe_unused static bool dont_have_methods(struct bt_conn *conn)
{
	return (conn->tx_data_pull == NULL) ||
		(conn->get_and_clear_cb == NULL) ||
		(conn->has_data == NULL);
}

struct bt_conn *get_conn_ready(void)
{
	/* Here we only peek: we pop the conn (and insert it at the back if it
	 * still has data) after the QoS function returns false.
	 */
	sys_snode_t *node  = sys_slist_peek_head(&bt_dev.le.conn_ready);

	if (node == NULL) {
		return NULL;
	}

	/* `conn` borrows from the list node. That node is _not_ popped yet.
	 *
	 * If we end up not popping that conn off the list, we have to make sure
	 * to increase the refcount before returning a pointer to that
	 * connection out of this function.
	 */
	struct bt_conn *conn = CONTAINER_OF(node, struct bt_conn, _conn_ready);

	if (dont_have_viewbufs()) {
		/* We will get scheduled again when the (view) buffers are freed. If you
		 * hit this a lot, try increasing `CONFIG_BT_CONN_FRAG_COUNT`
		 */
		LOG_DBG("no view bufs");
		return NULL;
	}

	if (cannot_send_to_controller(conn)) {
		/* We will get scheduled again when the buffers are freed. */
		LOG_DBG("no LL bufs for %p", conn);
		return NULL;
	}

	if (dont_have_tx_context(conn)) {
		/* We will get scheduled again when TX contexts are available. */
		LOG_DBG("no TX contexts");
		return NULL;
	}

	CHECKIF(dont_have_methods(conn)) {
		LOG_DBG("conn %p (type %d) is missing mandatory methods",
			conn, conn->type);

		return NULL;
	}

	if (should_stop_tx(conn)) {
		/* Move reference off the list and into the `conn` variable. */
		__maybe_unused sys_snode_t *s = sys_slist_get(&bt_dev.le.conn_ready);

		__ASSERT_NO_MSG(s == node);

		(void)atomic_set(&conn->_conn_ready_lock, 0);
		/* Note: we can't assert `old` is non-NULL here, as the
		 * connection might have been marked ready by an l2cap channel
		 * that cancelled its request to send.
		 */

		/* Append connection to list if it still has data */
		if (conn->has_data(conn)) {
			LOG_DBG("appending %p to back of TX queue", conn);
			bt_conn_data_ready(conn);
		}

		return conn;
	}

	return bt_conn_ref(conn);
}

/* Crazy that this file is compiled even if this is not true, but here we are. */
#if defined(CONFIG_BT_CONN)
static void acl_get_and_clear_cb(struct bt_conn *conn, struct net_buf *buf,
				 bt_conn_tx_cb_t *cb, void **ud)
{
	__ASSERT_NO_MSG(is_acl_conn(conn));

	*cb = (bt_conn_tx_cb_t)closure_cb(buf->user_data);
	*ud = closure_data(buf->user_data);
	memset(buf->user_data, 0, buf->user_data_size);
}
#endif	/* defined(CONFIG_BT_CONN) */

/* Acts as a "null-routed" bt_send(). This fn will decrease the refcount of
 * `buf` and call the user callback with an error code.
 */
static void destroy_and_callback(struct bt_conn *conn,
				 struct net_buf *buf,
				 bt_conn_tx_cb_t cb,
				 void *ud)
{
	if (!cb) {
		conn->get_and_clear_cb(conn, buf, &cb, &ud);
	}

	LOG_DBG("pop: cb %p userdata %p", cb, ud);

	/* bt_send() would've done an unref. Do it here also, so the buffer is
	 * hopefully destroyed and the user callback can allocate a new one.
	 */
	net_buf_unref(buf);

	if (cb) {
		cb(conn, ud, -ESHUTDOWN);
	}
}

static volatile bool _suspend_tx;

#if defined(CONFIG_BT_TESTING)
void bt_conn_suspend_tx(bool suspend)
{
	_suspend_tx = suspend;

	LOG_DBG("%sing all data TX", suspend ? "suspend" : "resum");

	bt_tx_irq_raise();
}
#endif	/* CONFIG_BT_TESTING */

void bt_conn_tx_processor(void)
{
	LOG_DBG("start");
	struct bt_conn *conn;
	struct net_buf *buf;
	bt_conn_tx_cb_t cb = NULL;
	size_t buf_len;
	void *ud = NULL;

	if (!IS_ENABLED(CONFIG_BT_CONN_TX)) {
		/* Mom, can we have a real compiler? */
		return;
	}

	if (IS_ENABLED(CONFIG_BT_TESTING) && _suspend_tx) {
		return;
	}

	conn = get_conn_ready();

	if (!conn) {
		LOG_DBG("no connection wants to do stuff");
		return;
	}

	LOG_DBG("processing conn %p", conn);

	if (conn->state != BT_CONN_CONNECTED) {
		LOG_WRN("conn %p: not connected", conn);

		/* Call the user callbacks & destroy (final-unref) the buffers
		 * we were supposed to send.
		 */
		buf = conn->tx_data_pull(conn, SIZE_MAX, &buf_len);
		while (buf) {
			destroy_and_callback(conn, buf, cb, ud);
			buf = conn->tx_data_pull(conn, SIZE_MAX, &buf_len);
		}

		goto exit;
	}

	/* now that we are guaranteed resources, we can pull data from the upper
	 * layer (L2CAP or ISO).
	 */
	buf = conn->tx_data_pull(conn, conn_mtu(conn), &buf_len);
	if (!buf) {
		/* Either there is no more data, or the buffer is already in-use
		 * by a view on it. In both cases, the TX processor will be
		 * triggered again, either by the view's destroy callback, or by
		 * the upper layer when it has more data.
		 */
		LOG_DBG("no buf returned");

		goto exit;
	}

	bool last_buf = conn_mtu(conn) >= buf_len;

	if (last_buf) {
		/* Only pull the callback info from the last buffer.
		 * We still allocate one TX context per-fragment though.
		 */
		conn->get_and_clear_cb(conn, buf, &cb, &ud);
		LOG_DBG("pop: cb %p userdata %p", cb, ud);
	}

	LOG_DBG("TX process: conn %p buf %p (%s)",
		conn, buf, last_buf ? "last" : "frag");

	int err = send_buf(conn, buf, buf_len, (void *)cb, ud);

	if (err) {
		/* -EIO means `unrecoverable error`. It can be an assertion that
		 *  failed or an error from the HCI driver.
		 *
		 * -ENOMEM means we thought we had all the resources to send the
		 *  buf (ie. TX context + controller buffer) but one of them was
		 *  not available. This is likely due to a failure of
		 *  assumption, likely that we have been pre-empted somehow and
		 *  that `tx_processor()` has been re-entered.
		 *
		 *  In both cases, we destroy the buffer and mark the connection
		 *  as dead.
		 */
		LOG_ERR("Fatal error (%d). Disconnecting %p", err, conn);
		destroy_and_callback(conn, buf, cb, ud);
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

		goto exit;
	}

	/* Always kick the TX work. It will self-suspend if it doesn't get
	 * resources or there is nothing left to send.
	 */
	bt_tx_irq_raise();

exit:
	/* Give back the ref that `get_conn_ready()` gave us */
	bt_conn_unref(conn);
}

static void process_unack_tx(struct bt_conn *conn)
{
	LOG_DBG("%p", conn);

	/* Return any unacknowledged packets */
	while (1) {
		struct bt_conn_tx *tx;
		sys_snode_t *node;

		node = sys_slist_get(&conn->tx_pending);

		if (!node) {
			bt_tx_irq_raise();
			return;
		}

		tx = CONTAINER_OF(node, struct bt_conn_tx, node);

		conn_tx_destroy(conn, tx);
		k_sem_give(bt_conn_get_pkts(conn));
	}
}

struct bt_conn *conn_lookup_handle(struct bt_conn *conns, size_t size,
				   uint16_t handle)
{
	int i;

	for (i = 0; i < size; i++) {
		struct bt_conn *conn = bt_conn_ref(&conns[i]);

		if (!conn) {
			continue;
		}

		/* We only care about connections with a valid handle */
		if (!bt_conn_is_handle_valid(conn)) {
			bt_conn_unref(conn);
			continue;
		}

		if (conn->handle != handle) {
			bt_conn_unref(conn);
			continue;
		}

		return conn;
	}

	return NULL;
}

void bt_conn_set_state(struct bt_conn *conn, bt_conn_state_t state)
{
	bt_conn_state_t old_state;

	LOG_DBG("%s -> %s", state2str(conn->state), state2str(state));

	if (conn->state == state) {
		LOG_WRN("no transition %s", state2str(state));
		return;
	}

	old_state = conn->state;
	conn->state = state;

	/* Actions needed for exiting the old state */
	switch (old_state) {
	case BT_CONN_DISCONNECTED:
		/* Take a reference for the first state transition after
		 * bt_conn_add_le() and keep it until reaching DISCONNECTED
		 * again.
		 */
		if (conn->type != BT_CONN_TYPE_ISO) {
			bt_conn_ref(conn);
		}
		break;
	case BT_CONN_INITIATING:
		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    conn->type == BT_CONN_TYPE_LE) {
			k_work_cancel_delayable(&conn->deferred_work);
		}
		break;
	default:
		/* MISRA 16.4 : The switch statement does not have a non-empty default clause. */
		break;
	}

	/* Actions needed for entering the new state */
	switch (conn->state) {
	case BT_CONN_CONNECTED:
		if (conn->type == BT_CONN_TYPE_SCO) {
			/* WARNING: TODO: Notify sco connected */
#if (defined (CONFIG_BT_HFP_HF) && (CONFIG_BT_HFP_HF > 0U)) || (defined (CONFIG_BT_HFP_AG) && (CONFIG_BT_HFP_AG > 0U))
			sco_audio_start_pl ();
#endif
#if (defined(CONFIG_BT_CLASSIC) && (CONFIG_BT_CLASSIC > 0))
			if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
				bt_sco_connected(conn);
			}
#endif /* CONFIG_BT_CLASSIC */
			break;
		}

#if (defined(CONFIG_BT_ISO) && (CONFIG_BT_ISO > 0))
		if (IS_ENABLED(CONFIG_BT_ISO) &&
		    conn->type == BT_CONN_TYPE_ISO) {
			bt_iso_connected(conn);
			break;
		}
#endif /* CONFIG_BT_ISO */

		/* EDGEFAST Bluetooth: Get device ID */
		if (BT_CONN_TYPE_LE == conn->type)
		{
			BT_DEVICE_ADDR peerAddr;
			DEVICE_HANDLE deviceHandle;

			if (BT_HCI_ROLE_CENTRAL == conn->role)
			{
				peerAddr.type = conn->le.resp_addr.type;
				memcpy(&peerAddr.addr[0], &conn->le.resp_addr.a.val[0], sizeof(peerAddr.addr));
			}
			else
			{
				peerAddr.type = conn->le.init_addr.type;
				memcpy(&peerAddr.addr[0], &conn->le.init_addr.a.val[0], sizeof(peerAddr.addr));
			}
			if (API_SUCCESS == device_queue_search_le_remote_addr(&deviceHandle, &peerAddr))
			{
				conn->deviceId = deviceHandle;
			}
		}
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
		else if (BT_CONN_TYPE_BR == conn->type)
		{
			BT_DEVICE_ADDR peerAddr;
			DEVICE_HANDLE deviceHandle;

			peerAddr.type = BT_ADDR_LE_PUBLIC;
			memcpy(&peerAddr.addr[0], &conn->br.dst.val[0], sizeof(peerAddr.addr));
			if (API_SUCCESS == device_queue_search_br_edr_remote_addr(&deviceHandle, &peerAddr))
			{
				conn->deviceId = deviceHandle;
			}
		}
#endif
#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0U))
		sys_slist_init(&conn->channels);

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
		    conn->role == BT_CONN_ROLE_PERIPHERAL) {

#if (defined(CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS) && (CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS > 0U))
			if (conn->type == BT_CONN_TYPE_LE) {
				conn->le.conn_param_retry_countdown =
					CONFIG_BT_CONN_PARAM_RETRY_COUNT;
			}
#endif /* CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS */

			k_work_schedule(&conn->deferred_work,
					CONN_UPDATE_TIMEOUT);
		}
#endif /* CONFIG_BT_CONN */

		break;
	case BT_CONN_DISCONNECTED:
#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0U))
		if (conn->type == BT_CONN_TYPE_SCO) {
			/* WARNING: TODO: Notify sco disconnected */
#if (defined (CONFIG_BT_HFP_HF) && (CONFIG_BT_HFP_HF > 0U)) || (defined (CONFIG_BT_HFP_AG) && (CONFIG_BT_HFP_AG > 0U))
			sco_audio_stop_pl();
#endif
#if (defined(CONFIG_BT_CLASSIC) && (CONFIG_BT_CLASSIC > 0))
			if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
				bt_sco_disconnected(conn);
			}
#endif /* CONFIG_BT_CLASSIC */
			bt_conn_unref(conn);
			break;
		}

		/* Notify disconnection and queue a dummy buffer to wake
		 * up and stop the tx thread for states where it was
		 * running.
		 */
		switch (old_state) {
		case BT_CONN_DISCONNECT_COMPLETE:
			wait_for_tx_work(conn);

			/* Cancel Connection Update if it is pending */
			if ((conn->type == BT_CONN_TYPE_LE) &&
			    (k_work_delayable_busy_get(&conn->deferred_work) &
			     (K_WORK_QUEUED | K_WORK_DELAYED))) {
				k_work_cancel_delayable(&conn->deferred_work);
			}

			bt_conn_reset_rx_state(conn);

			LOG_DBG("trigger disconnect work");
			k_work_reschedule(&conn->deferred_work, K_NO_WAIT);

			/* The last ref will be dropped during cleanup */
			break;
		case BT_CONN_INITIATING:
			/* LE Create Connection command failed. This might be
			 * directly from the API, don't notify application in
			 * this case.
			 */
			if (conn->err) {
				notify_connected(conn);
			}

			bt_conn_unref(conn);
			break;
		case BT_CONN_SCAN_BEFORE_INITIATING:
			/* This indicates that connection establishment
			 * has been stopped. This could either be triggered by
			 * the application through bt_conn_disconnect or by
			 * timeout set by bt_conn_le_create_param.timeout.
			 */
			if (conn->err) {
				notify_connected(conn);
			}

			bt_conn_unref(conn);
			break;
		case BT_CONN_ADV_DIR_CONNECTABLE:
			/* this indicate Directed advertising stopped */
			if (conn->err) {
				notify_connected(conn);
			}

			bt_conn_unref(conn);
			break;
		case BT_CONN_INITIATING_FILTER_LIST:
			/* this indicates LE Create Connection with filter
			 * policy has been stopped. This can only be triggered
			 * by the application, so don't notify.
			 */
			bt_conn_unref(conn);
			break;
		case BT_CONN_ADV_CONNECTABLE:
			/* This can only happen when application stops the
			 * advertiser, conn->err is never set in this case.
			 */
			bt_conn_unref(conn);
			break;
		case BT_CONN_CONNECTED:
		case BT_CONN_DISCONNECTING:
		case BT_CONN_DISCONNECTED:
			/* Cannot happen. */
			LOG_WRN("Invalid (%u) old state", state);
			break;
		}
		break;
	case BT_CONN_INITIATING_FILTER_LIST:
		break;
	case BT_CONN_ADV_CONNECTABLE:
		break;
	case BT_CONN_SCAN_BEFORE_INITIATING:
		break;
	case BT_CONN_ADV_DIR_CONNECTABLE:
		break;
	case BT_CONN_INITIATING:
		if (conn->type == BT_CONN_TYPE_SCO) {
			break;
		}
		/*
		 * Timer is needed only for LE. For other link types controller
		 * will handle connection timeout.
		 */
		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    conn->type == BT_CONN_TYPE_LE &&
		    bt_dev.create_param.timeout != 0) {
			k_work_schedule(&conn->deferred_work,
					K_MSEC(10 * bt_dev.create_param.timeout));
		}

		break;
	case BT_CONN_DISCONNECTING:
		break;
#endif /* CONFIG_BT_CONN */
	case BT_CONN_DISCONNECT_COMPLETE:
		if (conn->err == BT_HCI_ERR_CONN_FAIL_TO_ESTAB) {
			/* No ACK or data was ever received. The peripheral may be
			 * unaware of the connection attempt.
			 *
			 * Beware of confusing higher layer errors. Anything that looks
			 * like it's from the remote is synthetic.
			 */
			LOG_WRN("conn %p failed to establish. RF noise?", conn);
		}

		process_unack_tx(conn);
		break;
	default:
		LOG_WRN("no valid (%u) state was set", state);

		break;
	}
}

struct bt_conn *bt_conn_lookup_handle(uint16_t handle, enum bt_conn_type type)
{
	struct bt_conn *conn;

#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0U))
	conn = conn_lookup_handle(acl_conns, ARRAY_SIZE(acl_conns), handle);
	if (conn) {
		goto found;
	}
#endif /* CONFIG_BT_CONN */

#if (defined(CONFIG_BT_ISO) && ((CONFIG_BT_ISO) > 0U))
	conn = conn_lookup_handle(iso_conns, ARRAY_SIZE(iso_conns), handle);
	if (conn) {
		goto found;
	}
#endif

 #if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	conn = conn_lookup_handle(sco_conns, ARRAY_SIZE(sco_conns), handle);
	if (conn) {
		goto found;
	}
#endif

found:
	if (conn) {
		if (type & conn->type) {
			return conn;
		}
		LOG_WRN("incompatible handle %u", handle);
		bt_conn_unref(conn);
	}
	return NULL;
}

void bt_conn_foreach(enum bt_conn_type type,
		     void (*func)(struct bt_conn *conn, void *data),
		     void *data)
{
	int i;

#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0U))
	for (i = 0; i < ARRAY_SIZE(acl_conns); i++) {
		struct bt_conn *conn = bt_conn_ref(&acl_conns[i]);

		if (!conn) {
			continue;
		}

		if (!(conn->type & type)) {
			bt_conn_unref(conn);
			continue;
		}

		func(conn, data);
		bt_conn_unref(conn);
	}
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	if (type & BT_CONN_TYPE_SCO) {
		for (i = 0; i < ARRAY_SIZE(sco_conns); i++) {
			struct bt_conn *conn = bt_conn_ref(&sco_conns[i]);

			if (!conn) {
				continue;
			}

			func(conn, data);
			bt_conn_unref(conn);
		}
	}
#endif /* defined(CONFIG_BT_CLASSIC) */
#endif /* CONFIG_BT_CONN */

#if (defined(CONFIG_BT_ISO) && ((CONFIG_BT_ISO) > 0U))
	if (type & BT_CONN_TYPE_ISO) {
		for (i = 0; i < ARRAY_SIZE(iso_conns); i++) {
			struct bt_conn *conn = bt_conn_ref(&iso_conns[i]);

			if (!conn) {
				continue;
			}

			func(conn, data);
			bt_conn_unref(conn);
		}
	}
#endif /* defined(CONFIG_BT_ISO) */
}

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	atomic_val_t old;

	__ASSERT_NO_MSG(conn);

	/* Reference counter must be checked to avoid incrementing ref from
	 * zero, then we should return NULL instead.
	 * Loop on clear-and-set in case someone has modified the reference
	 * count since the read, and start over again when that happens.
	 */
	do {
		old = atomic_get(&conn->ref);

		if (!old) {
			return NULL;
		}
	} while (!atomic_cas(&conn->ref, old, old + 1));

	LOG_DBG("handle %u ref %ld -> %ld", conn->handle, old, old + 1);

	return conn;
}

void bt_conn_unref(struct bt_conn *conn)
{
	atomic_val_t old;
	bool deallocated;
	enum bt_conn_type conn_type;
	uint8_t conn_role;
	uint16_t conn_handle;

	__ASSERT(conn, "Invalid connection reference");

	/* Storing parameters of interest so we don't access the object
	 * after decrementing its ref-count
	 */
	conn_type = conn->type;
	conn_role = conn->role;
	conn_handle = conn->handle;

	old = atomic_dec(&conn->ref);

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
    if ((atomic_get(&conn->ref) == 0) && (NULL != conn->br.pending_l2cap_ecbfc_req))
    {
        net_buf_unref(conn->br.pending_l2cap_ecbfc_req);
        conn->br.pending_l2cap_ecbfc_req = NULL;
    }
#endif

	/* Prevent from accessing connection object */
	conn = NULL;
	deallocated = (atomic_get(&old) == 1);

	LOG_DBG("handle %u ref %ld -> %ld", conn_handle, old, (old - 1));

	__ASSERT(old > 0, "Conn reference counter is 0");

	/* Slot has been freed and can be taken. No guarantees are made on requests
	 * to claim connection object as only the first claim will be served.
	 */
	if (deallocated) {
		notify_recycled_conn_slot();
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn_type == BT_CONN_TYPE_LE &&
	    conn_role == BT_CONN_ROLE_PERIPHERAL && deallocated) {
		bt_le_adv_resume();
	}
	(void)conn_handle;
}

uint8_t bt_conn_index(const struct bt_conn *conn)
{
	ptrdiff_t index = 0;

	switch (conn->type) {
#if (defined(CONFIG_BT_ISO) && ((CONFIG_BT_ISO) > 0U))
	case BT_CONN_TYPE_ISO:
		index = conn - iso_conns;
		__ASSERT(index >= 0 && index < ARRAY_SIZE(iso_conns),
			"Invalid bt_conn pointer");
		break;
#endif
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	case BT_CONN_TYPE_SCO:
		index = conn - sco_conns;
		__ASSERT(index >= 0 && index < ARRAY_SIZE(sco_conns),
			"Invalid bt_conn pointer");
		break;
#endif
	default:
#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0U))
		index = conn - acl_conns;
		__ASSERT(index >= 0 && index < ARRAY_SIZE(acl_conns),
			 "Invalid bt_conn pointer");
#else
		__ASSERT(false, "Invalid connection type %u", conn->type);
#endif /* CONFIG_BT_CONN */
		break;
	}

	return (uint8_t)index;
}


#if (defined(CONFIG_NET_BUF_LOG) && ((CONFIG_NET_BUF_LOG) > 0U))
struct net_buf *bt_conn_create_pdu_timeout_debug(struct net_buf_pool *pool,
						 size_t reserve,
						 k_timeout_t timeout,
						 const char *func, int line)
#else
struct net_buf *bt_conn_create_pdu_timeout(struct net_buf_pool *pool,
					   size_t reserve, k_timeout_t timeout)
#endif
{
	struct net_buf *buf;

	/*
	 * PDU must not be allocated from ISR as we block with 'K_FOREVER'
	 * during the allocation
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
	    k_current_get() == k_work_queue_thread_get(&k_sys_work_q)) {
		LOG_DBG("Timeout discarded. No blocking in syswq.");
		timeout = K_NO_WAIT;
	}

	if (!pool) {
#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0U))
		pool = &acl_tx_pool;
#else
		return NULL;
#endif /* CONFIG_BT_CONN */
	}

	if (IS_ENABLED(CONFIG_BT_DEBUG_CONN)) {
#if (defined(CONFIG_NET_BUF_LOG) && ((CONFIG_NET_BUF_LOG) > 0U))
		buf = net_buf_alloc_fixed_debug(pool, osaWaitNone_c, func, line);
#else
		buf = net_buf_alloc(pool, K_NO_WAIT);
#endif
		if (!buf) {
			LOG_WRN("Unable to allocate buffer with osaWaitNone_c");
#if (defined(CONFIG_NET_BUF_LOG) && ((CONFIG_NET_BUF_LOG) > 0U))
			buf = net_buf_alloc_fixed_debug(pool, timeout, func,
							line);
#else
			buf = net_buf_alloc(pool, timeout);
#endif
		}
	} else {
#if (defined(CONFIG_NET_BUF_LOG) && ((CONFIG_NET_BUF_LOG) > 0U))
		buf = net_buf_alloc_fixed_debug(pool, timeout, func,
							line);
#else
		buf = net_buf_alloc(pool, timeout);
#endif
	}

	if (!buf) {
		LOG_WRN("Unable to allocate buffer within timeout");
		return NULL;
	}

	reserve += sizeof(struct bt_hci_acl_hdr) + BT_BUF_RESERVE;
	net_buf_reserve(buf, reserve);

	return buf;
}

#if (defined(CONFIG_BT_CONN_TX) && ((CONFIG_BT_CONN_TX) > 0U))
static void tx_complete_work(struct k_work *work)
{
	struct bt_conn *conn = CONTAINER_OF(work, struct bt_conn,
					    tx_complete_work);

	LOG_DBG("conn %p", conn);

	tx_notify(conn);
}
#endif /* CONFIG_BT_CONN_TX */

static void notify_recycled_conn_slot(void)
{
#if (defined(CONFIG_BT_CONN) && (CONFIG_BT_CONN > 0))
	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->recycled) {
			callback->recycled();
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->recycled) {
			cb->recycled();
		}
	}
#endif
}

#if !defined(CONFIG_BT_CONN)
int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	/* Dummy implementation to satisfy the compiler */

	return 0;
}
#endif	/* !CONFIG_BT_CONN */

/* Group Connected BT_CONN only in this */
#if (defined(CONFIG_BT_CONN) && ((CONFIG_BT_CONN) > 0U))

void bt_conn_connected(struct bt_conn *conn)
{
	bt_l2cap_connected(conn);
	notify_connected(conn);
}

static int conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	int err;

	err = bt_hci_disconnect(conn->handle, reason);
	if (err) {
		return err;
	}

	if (conn->state == BT_CONN_CONNECTED) {
		bt_conn_set_state(conn, BT_CONN_DISCONNECTING);
	}

	return 0;
}

int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	/* Disconnection is initiated by us, so auto connection shall
	 * be disabled. Otherwise the passive scan would be enabled
	 * and we could send LE Create Connection as soon as the remote
	 * starts advertising.
	 */
#if !(defined(CONFIG_BT_FILTER_ACCEPT_LIST) && (CONFIG_BT_FILTER_ACCEPT_LIST > 0))
	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->type == BT_CONN_TYPE_LE) {
		bt_le_set_auto_conn(&conn->le.dst, NULL);
	}
#endif /* !defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

	switch (conn->state) {
	case BT_CONN_SCAN_BEFORE_INITIATING:
		conn->err = reason;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			bt_le_scan_update(false);
		}
		return 0;
	case BT_CONN_INITIATING:
		if (conn->type == BT_CONN_TYPE_LE) {
			if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
				k_work_cancel_delayable(&conn->deferred_work);
				return bt_le_create_conn_cancel();
			}
		}

#if (defined(CONFIG_BT_ISO) && ((CONFIG_BT_ISO) > 0U))
		else if (conn->type == BT_CONN_TYPE_ISO) {
			return conn_disconnect(conn, reason);
		}
#endif /* CONFIG_BT_ISO */
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
		else if (conn->type == BT_CONN_TYPE_BR) {
			return bt_hci_connect_br_cancel(conn);
		}
#endif /* CONFIG_BT_CLASSIC */
		else {
			__ASSERT(false, "Invalid conn type %u", conn->type);
		}

		return 0;
	case BT_CONN_CONNECTED:
		return conn_disconnect(conn, reason);
	case BT_CONN_DISCONNECTING:
		return 0;
	case BT_CONN_DISCONNECTED:
	default:
		return -ENOTCONN;
	}
}

static void notify_connected(struct bt_conn *conn)
{
	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {

		if (callback->connected) {
			callback->connected(conn, conn->err);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->connected) {
			cb->connected(conn, conn->err);
		}
	}

#if 0 /* WARNING: TODO: Need to be checked */
#if !(defined (CONFIG_BT_BLE_DISABLE) && (CONFIG_BT_BLE_DISABLE > 0U))
	if (!conn->err) {
		bt_gatt_connected(conn);
	}
#endif
#endif
}

static void notify_disconnected(struct bt_conn *conn)
{
	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->disconnected) {
			callback->disconnected(conn, conn->err);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->disconnected) {
			cb->disconnected(conn, conn->err);
		}
	}
}

#if (defined(CONFIG_BT_REMOTE_INFO) && ((CONFIG_BT_REMOTE_INFO) > 0U))
void notify_remote_info(struct bt_conn *conn)
{
	struct bt_conn_remote_info remote_info;
	int err;

	err = bt_conn_get_remote_info(conn, &remote_info);
	if (err) {
		LOG_DBG("Notify remote info failed %d", err);
		return;
	}

	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->remote_info_available) {
			callback->remote_info_available(conn, &remote_info);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->remote_info_available) {
			cb->remote_info_available(conn, &remote_info);
		}
	}
}
#endif /* defined(CONFIG_BT_REMOTE_INFO) */

void notify_le_param_updated(struct bt_conn *conn)
{
	/* If new connection parameters meet requirement of pending
	 * parameters don't send peripheral conn param request anymore on timeout
	 */
	if (atomic_test_bit(conn->flags, BT_CONN_PERIPHERAL_PARAM_SET) &&
	    conn->le.interval >= conn->le.interval_min &&
	    conn->le.interval <= conn->le.interval_max &&
	    conn->le.latency == conn->le.pending_latency &&
	    conn->le.timeout == conn->le.pending_timeout) {
		atomic_clear_bit(conn->flags, BT_CONN_PERIPHERAL_PARAM_SET);
	}

	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->le_param_updated) {
			callback->le_param_updated(conn, conn->le.interval,
						   conn->le.latency,
						   conn->le.timeout);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->le_param_updated) {
			cb->le_param_updated(conn, conn->le.interval,
					     conn->le.latency,
					     conn->le.timeout);
		}
	}
}

#if (defined(CONFIG_BT_USER_DATA_LEN_UPDATE) && ((CONFIG_BT_USER_DATA_LEN_UPDATE) > 0U))
void notify_le_data_len_updated(struct bt_conn *conn)
{
	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->le_data_len_updated) {
			callback->le_data_len_updated(conn, &conn->le.data_len);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->le_data_len_updated) {
			cb->le_data_len_updated(conn, &conn->le.data_len);
		}
	}
}
#endif

#if (defined(CONFIG_BT_USER_PHY_UPDATE) && ((CONFIG_BT_USER_PHY_UPDATE) > 0U))
void notify_le_phy_updated(struct bt_conn *conn)
{
	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->le_phy_updated) {
			callback->le_phy_updated(conn, &conn->le.phy);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->le_phy_updated) {
			cb->le_phy_updated(conn, &conn->le.phy);
		}
	}
}
#endif

bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	if (!bt_le_conn_params_valid(param)) {
		return false;
	}

	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (!callback->le_param_req) {
			continue;
		}

		if (!callback->le_param_req(conn, param)) {
			return false;
		}

		/* The callback may modify the parameters so we need to
		 * double-check that it returned valid parameters.
		 */
		if (!bt_le_conn_params_valid(param)) {
			return false;
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (!cb->le_param_req) {
			continue;
		}

		if (!cb->le_param_req(conn, param)) {
			return false;
		}

		/* The callback may modify the parameters so we need to
		 * double-check that it returned valid parameters.
		 */
		if (!bt_le_conn_params_valid(param)) {
			return false;
		}
	}

	/* Default to accepting if there's no app callback */
	return true;
}

static int send_conn_le_param_update(struct bt_conn *conn,
				const struct bt_le_conn_param *param)
{
	LOG_DBG("conn %p features 0x%02x params (%d-%d %d %d)", conn, conn->le.features[0],
		param->interval_min, param->interval_max, param->latency, param->timeout);

	/* Proceed only if connection parameters contains valid values*/
	if (!bt_le_conn_params_valid(param)) {
		return -EINVAL;
	}

	/* Use LE connection parameter request if both local and remote support
	 * it; or if local role is central then use LE connection update.
	 */
	if ((BT_FEAT_LE_CONN_PARAM_REQ_PROC(bt_dev.le.features) &&
	     BT_FEAT_LE_CONN_PARAM_REQ_PROC(conn->le.features) &&
	     !atomic_test_bit(conn->flags, BT_CONN_PERIPHERAL_PARAM_L2CAP)) ||
	     (conn->role == BT_HCI_ROLE_CENTRAL)) {
		int rc;

		rc = bt_conn_le_conn_update(conn, param);

		/* store those in case of fallback to L2CAP */
		if (rc == 0) {
			conn->le.interval_min = param->interval_min;
			conn->le.interval_max = param->interval_max;
			conn->le.pending_latency = param->latency;
			conn->le.pending_timeout = param->timeout;
		}

		return rc;
	}

	/* If remote central does not support LL Connection Parameters Request
	 * Procedure
	 */
	return bt_l2cap_update_conn_param(conn, param);
}

#if (defined(CONFIG_BT_ISO_UNICAST) && ((CONFIG_BT_ISO_UNICAST) > 0U))
static struct bt_conn *conn_lookup_iso(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(iso_conns); i++) {
		struct bt_conn *iso = bt_conn_ref(&iso_conns[i]);

		if (iso == NULL) {
			continue;
		}

		if (iso->iso.acl == conn) {
			return iso;
		}

		bt_conn_unref(iso);
	}

	return NULL;
}
#endif /* CONFIG_BT_ISO */

#if (defined(CONFIG_BT_CLASSIC) && (CONFIG_BT_CLASSIC > 0U))
static struct bt_conn *conn_lookup_sco(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sco_conns); i++) {
		struct bt_conn *sco = bt_conn_ref(&sco_conns[i]);

		if (sco == NULL) {
			continue;
		}

		if (sco->sco.acl == conn) {
			return sco;
		}

		bt_conn_unref(sco);
	}

	return NULL;
}
#endif /* CONFIG_BT_CLASSIC */

static void deferred_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_conn *conn = CONTAINER_OF(dwork, struct bt_conn, deferred_work);
	const struct bt_le_conn_param *param;

	LOG_DBG("conn %p", conn);

	if (conn->state == BT_CONN_DISCONNECTED) {
#if (defined(CONFIG_BT_ISO_UNICAST) && (CONFIG_BT_ISO_UNICAST > 0))
		struct bt_conn *iso;

		if (conn->type == BT_CONN_TYPE_ISO) {
			/* bt_iso_disconnected is responsible for unref'ing the
			 * connection pointer, as it is conditional on whether
			 * the connection is a central or peripheral.
			 */
			bt_iso_disconnected(conn);
			return;
		}

		/* Mark all ISO channels associated
		 * with ACL conn as not connected, and
		 * remove ACL reference
		 */
		iso = conn_lookup_iso(conn);
		while (iso != NULL) {
			struct bt_iso_chan *chan = iso->iso.chan;

			if (chan != NULL) {
				bt_iso_chan_set_state(chan,
						      BT_ISO_STATE_DISCONNECTING);
			}

			bt_iso_cleanup_acl(iso);

			bt_conn_unref(iso);
			iso = conn_lookup_iso(conn);
		}
#endif
#if (defined(CONFIG_BT_CLASSIC) && (CONFIG_BT_CLASSIC > 0U))
		struct bt_conn *sco;

		/* Mark all SCO channels associated
		 * with ACL conn as not connected, and
		 * remove ACL reference
		 */
		sco = conn_lookup_sco(conn);
		while (sco != NULL) {
			struct bt_sco_chan *chan = sco->sco.chan;

			if (chan != NULL) {
				bt_sco_chan_set_state(chan,
						      BT_SCO_STATE_DISCONNECTING);
			}

			bt_sco_cleanup_acl(sco);

			bt_conn_unref(sco);
			sco = conn_lookup_sco(conn);
		}
#endif /* CONFIG_BT_CLASSIC */

		if (atomic_test_and_clear_bit(conn->flags, BT_CONN_UNPAIRING))
		{
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
			if (conn->type == BT_CONN_TYPE_BR)
			{
				bt_addr_le_t le_addr;

				le_addr.type = BT_ADDR_LE_PUBLIC;
				memcpy(le_addr.a.val, conn->br.dst.val, sizeof(le_addr.a.val));
				bt_conn_unpair(conn->id, &le_addr, NULL);
			}
			else
#endif /* (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U)) */
			{
				bt_addr_le_t rpa;

				if (conn->role == BT_HCI_ROLE_CENTRAL) {
					bt_addr_le_copy(&rpa, &conn->le.resp_addr);
				} else {
					bt_addr_le_copy(&rpa, &conn->le.init_addr);
				}
				bt_conn_unpair(conn->id, &conn->le.dst, &rpa);
			}
		}

		bt_l2cap_disconnected(conn);
		notify_disconnected(conn);

		/* Release the reference we took for the very first
		 * state transition.
		 */
		bt_conn_unref(conn);
		return;
	}

	if (conn->type != BT_CONN_TYPE_LE) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_CONN_ROLE_CENTRAL) {
		/* we don't call bt_conn_disconnect as it would also clear
		 * auto connect flag if it was set, instead just cancel
		 * connection directly
		 */
		bt_le_create_conn_cancel();
		return;
	}

	/* if application set own params use those, otherwise use defaults. */
	if (atomic_test_and_clear_bit(conn->flags,
				      BT_CONN_PERIPHERAL_PARAM_SET)) {
		int err;

		param = BT_LE_CONN_PARAM(conn->le.interval_min,
					 conn->le.interval_max,
					 conn->le.pending_latency,
					 conn->le.pending_timeout);

		err = send_conn_le_param_update(conn, param);
		if (!err) {
			atomic_clear_bit(conn->flags,
					 BT_CONN_PERIPHERAL_PARAM_AUTO_UPDATE);
		} else {
			LOG_WRN("Send LE param update failed (err %d)", err);
		}
	} else if (IS_ENABLED(CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS)) {
#if (defined(CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS) && (CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS > 0U))
		int err;

		param = BT_LE_CONN_PARAM(
				CONFIG_BT_PERIPHERAL_PREF_MIN_INT,
				CONFIG_BT_PERIPHERAL_PREF_MAX_INT,
				CONFIG_BT_PERIPHERAL_PREF_LATENCY,
				CONFIG_BT_PERIPHERAL_PREF_TIMEOUT);

		err = send_conn_le_param_update(conn, param);
		if (!err) {
			atomic_set_bit(conn->flags,
				       BT_CONN_PERIPHERAL_PARAM_AUTO_UPDATE);
		} else {
			LOG_WRN("Send auto LE param update failed (err %d)",
				err);
		}
#endif
	}

	atomic_set_bit(conn->flags, BT_CONN_PERIPHERAL_PARAM_UPDATE);
}

static struct bt_conn *acl_conn_new(void)
{
	return bt_conn_new(acl_conns, ARRAY_SIZE(acl_conns));
}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
void bt_sco_cleanup(struct bt_conn *sco_conn)
{
	bt_sco_cleanup_acl(sco_conn);
	bt_conn_unref(sco_conn);
}

static struct bt_conn *sco_conn_new(void)
{
	return bt_conn_new(sco_conns, ARRAY_SIZE(sco_conns));
}

struct bt_conn *bt_conn_create_br(const bt_addr_t *peer,
				  const struct bt_br_conn_param *param)
{
	struct bt_hci_cp_connect *cp;
	struct bt_conn *conn;
	struct net_buf *buf;

	conn = bt_conn_lookup_addr_br(peer);
	if (conn) {
		switch (conn->state) {
		case BT_CONN_INITIATING:
		case BT_CONN_CONNECTED:
			return conn;
		default:
			bt_conn_unref(conn);
			return NULL;
		}
	}

	conn = bt_conn_add_br(peer);
	if (!conn) {
		return NULL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_CONNECT, sizeof(*cp));
	if (!buf) {
		bt_conn_unref(conn);
		return NULL;
	}

	cp = (struct bt_hci_cp_connect *)net_buf_add(buf, sizeof(*cp));

	(void)memset(cp, 0, sizeof(*cp));

	memcpy(&cp->bdaddr, peer, sizeof(cp->bdaddr));
	cp->packet_type = sys_cpu_to_le16(0xcc18); /* DM1 DH1 DM3 DH5 DM5 DH5 */
	cp->pscan_rep_mode = 0x02; /* R2 */
	cp->allow_role_switch = param->allow_role_switch ? 0x01 : 0x00;
	cp->clock_offset = 0x0000; /* TODO used cached clock offset */

	if (bt_hci_cmd_send_sync(BT_HCI_OP_CONNECT, buf, NULL) < 0) {
		bt_conn_unref(conn);
		return NULL;
	}

	bt_conn_set_state(conn, BT_CONN_INITIATING);
	conn->role = BT_CONN_ROLE_CENTRAL;

	return conn;
}

struct bt_conn *bt_conn_lookup_addr_sco(const bt_addr_t *peer)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sco_conns); i++) {
		struct bt_conn *conn = bt_conn_ref(&sco_conns[i]);

		if (!conn) {
			continue;
		}

		if (conn->type != BT_CONN_TYPE_SCO) {
			bt_conn_unref(conn);
			continue;
		}

		if (!bt_addr_eq(peer, &conn->sco.acl->br.dst)) {
			bt_conn_unref(conn);
			continue;
		}

		return conn;
	}

	return NULL;
}

struct bt_conn *bt_conn_lookup_addr_br(const bt_addr_t *peer)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(acl_conns); i++) {
		struct bt_conn *conn = bt_conn_ref(&acl_conns[i]);

		if (!conn) {
			continue;
		}

		if (conn->type != BT_CONN_TYPE_BR) {
			bt_conn_unref(conn);
			continue;
		}

		if (!bt_addr_eq(peer, &conn->br.dst)) {
			bt_conn_unref(conn);
			continue;
		}

		return conn;
	}

	return NULL;
}

struct bt_conn *bt_conn_add_sco(const bt_addr_t *peer, int link_type)
{
	struct bt_conn *sco_conn = sco_conn_new();

	if (!sco_conn) {
		return NULL;
	}

	sco_conn->sco.acl = bt_conn_lookup_addr_br(peer);
	if (!sco_conn->sco.acl) {
		bt_conn_unref(sco_conn);
		return NULL;
	}

	sco_conn->type = BT_CONN_TYPE_SCO;

	if (link_type == BT_HCI_SCO) {
		if (BT_FEAT_LMP_ESCO_CAPABLE(bt_dev.features)) {
			sco_conn->sco.pkt_type = (bt_dev.br.esco_pkt_type &
						  ESCO_PKT_MASK);
		} else {
			sco_conn->sco.pkt_type = (bt_dev.br.esco_pkt_type &
						  SCO_PKT_MASK);
		}
	} else if (link_type == BT_HCI_ESCO) {
#if (defined(CONFIG_BT_EDR_ESCO_PKT) && ((CONFIG_BT_EDR_ESCO_PKT) > 0U))
		sco_conn->sco.pkt_type = bt_dev.br.esco_pkt_type;
#else
		sco_conn->sco.pkt_type = (bt_dev.br.esco_pkt_type &
					  ~EDR_ESCO_PKT_MASK);
#endif
	}

	return sco_conn;
}

struct bt_conn *bt_conn_add_br(const bt_addr_t *peer)
{
	struct bt_conn *conn = acl_conn_new();

	if (!conn) {
		return NULL;
	}

	bt_addr_copy(&conn->br.dst, peer);
	conn->type = BT_CONN_TYPE_BR;
	conn->tx_data_pull = l2cap_br_data_pull;
	conn->get_and_clear_cb = acl_get_and_clear_cb;
	conn->has_data = acl_has_data;

	return conn;
}

static int bt_hci_connect_br_cancel(struct bt_conn *conn)
{
	struct bt_hci_cp_connect_cancel *cp;
	struct bt_hci_rp_connect_cancel *rp;
	struct net_buf *buf, *rsp;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_CONNECT_CANCEL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = (struct bt_hci_cp_connect_cancel *)net_buf_add(buf, sizeof(*cp));
	memcpy(&cp->bdaddr, &conn->br.dst, sizeof(cp->bdaddr));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_CONNECT_CANCEL, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (struct bt_hci_rp_connect_cancel *)rsp->data;

	err = rp->status ? -EIO : 0;

	net_buf_unref(rsp);

	return err;
}

#endif /* CONFIG_BT_CLASSIC */

#if (defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U))
bool bt_conn_ltk_present(const struct bt_conn *conn)
{
	const struct bt_keys *keys = conn->le.keys;

	if (!keys) {
		keys = bt_keys_find_addr(conn->id, &conn->le.dst);
	}

	if (keys) {
		if (conn->role == BT_HCI_ROLE_CENTRAL) {
			return keys->keys & (BT_KEYS_LTK_P256 | BT_KEYS_PERIPH_LTK);
		} else {
			return keys->keys & (BT_KEYS_LTK_P256 | BT_KEYS_LTK);
		}
	}

	return false;
}

void bt_conn_identity_resolved(struct bt_conn *conn)
{
	const bt_addr_le_t *rpa;

	if (conn->role == BT_HCI_ROLE_CENTRAL) {
		rpa = &conn->le.resp_addr;
	} else {
		rpa = &conn->le.init_addr;
	}


	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->identity_resolved) {
			callback->identity_resolved(conn, rpa, &conn->le.dst);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->identity_resolved) {
			cb->identity_resolved(conn, rpa, &conn->le.dst);
		}
	}
}

int bt_conn_le_start_encryption(struct bt_conn *conn, uint8_t rand[8],
				uint8_t ediv[2], const uint8_t *ltk, size_t len)
{
	struct bt_hci_cp_le_start_encryption *cp;
	struct net_buf *buf;

	if (len > sizeof(cp->ltk)) {
		return -EINVAL;
	}
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_START_ENCRYPTION, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = (struct bt_hci_cp_le_start_encryption *)net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);

	/*
	 * EDEGFAST: Fix building warning:
	 * Warning[Pa039]: use of address of unaligned structure member
	 */
#if 0
	memcpy((uint8_t *)(&cp->rand), rand, sizeof(cp->rand));
	memcpy((uint8_t *)(&cp->ediv), ediv, sizeof(cp->ediv));
#else
	cp->rand = sys_cpu_to_le64(sys_get_le64(rand));
	cp->ediv = sys_cpu_to_le16(sys_get_le16(ediv));
#endif

	memcpy(cp->ltk, ltk, len);
	if (len < sizeof(cp->ltk)) {
		(void)memset(cp->ltk + len, 0, sizeof(cp->ltk) - len);
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_START_ENCRYPTION, buf, NULL);
}
#endif /* CONFIG_BT_SMP */

#if ((defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U)) || (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U)))
uint8_t bt_conn_enc_key_size(const struct bt_conn *conn)
{
	if (!conn->encrypt) {
		return 0;
	}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	if (IS_ENABLED(CONFIG_BT_CLASSIC) &&
	    conn->type == BT_CONN_TYPE_BR) {
		/* EDGEFAST: Get encryption key size from conn->br.link_key->enc_size directly*/
#if 0
		struct bt_hci_cp_read_encryption_key_size *cp;
		struct bt_hci_rp_read_encryption_key_size *rp;
		struct net_buf *buf;
		struct net_buf *rsp;
		uint8_t key_size;

		buf = bt_hci_cmd_create(BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE,
					sizeof(*cp));
		if (!buf) {
			return 0;
		}

		cp = (struct bt_hci_cp_read_encryption_key_size *)net_buf_add(buf, sizeof(*cp));
		cp->handle = sys_cpu_to_le16(conn->handle);

		if (bt_hci_cmd_send_sync(BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE,
					buf, &rsp)) {
			return 0;
		}

		rp = (struct bt_hci_rp_read_encryption_key_size *)rsp->data;

		key_size = rp->status ? 0 : rp->key_size;

		net_buf_unref(rsp);

		return key_size;
#else
		return conn->br.link_key ? conn->br.link_key->enc_size : 0;
#endif
	}
#endif /* CONFIG_BT_CLASSIC */

#if (defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U))
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		return conn->le.keys ? conn->le.keys->enc_size : 0;
	}
#else
	return 0;
#endif
}

static void reset_pairing(struct bt_conn *conn)
{
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	if (conn->type == BT_CONN_TYPE_BR) {
		atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING);
		atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING_INITIATOR);
		atomic_clear_bit(conn->flags, BT_CONN_BR_LEGACY_SECURE);
	}
#endif /* CONFIG_BT_CLASSIC */

	/* Reset required security level to current operational */
	conn->required_sec_level = conn->sec_level;
}

void bt_conn_security_changed(struct bt_conn *conn, uint8_t hci_err,
			      enum bt_security_err err)
{
	reset_pairing(conn);
	bt_l2cap_security_changed(conn, hci_err);

#if (defined(CONFIG_BT_ISO_CENTRAL) && (CONFIG_BT_ISO_CENTRAL > 0))
	if (IS_ENABLED(CONFIG_BT_ISO_CENTRAL)) {
		bt_iso_security_changed(conn, hci_err);
	}
#endif

	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->security_changed) {
			callback->security_changed(conn, conn->sec_level, err);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->security_changed) {
			cb->security_changed(conn, conn->sec_level, err);
		}
	}

#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	if (!err && conn->sec_level >= BT_SECURITY_L2) {
		if (conn->type == BT_CONN_TYPE_LE) {
			bt_keys_update_usage(conn->id, bt_conn_get_dst(conn));
		}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
#if (defined(CONFIG_BT_SETTINGS) && ((CONFIG_BT_SETTINGS) > 0U))
		if (conn->type == BT_CONN_TYPE_BR) {
			bt_keys_link_key_update_usage(&conn->br.dst);
		}
#endif /* CONFIG_BT_SETTINGS */
#endif /* CONFIG_BT_CLASSIC */

	}
#endif
}

static int start_security(struct bt_conn *conn)
{
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	if (IS_ENABLED(CONFIG_BT_CLASSIC) && conn->type == BT_CONN_TYPE_BR) {
		return bt_ssp_start_security(conn);
	}
#endif /* CONFIG_BT_CLASSIC */

#if (defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U))
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		return bt_smp_start_security(conn);
	}
#else
	return -EINVAL;
#endif
}

int bt_conn_set_security(struct bt_conn *conn, bt_security_t sec)
{
	bool force_pair;
	int err;

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	force_pair = sec & BT_SECURITY_FORCE_PAIR;
	sec &= ~BT_SECURITY_FORCE_PAIR;

	if (IS_ENABLED(CONFIG_BT_SMP_SC_ONLY)) {
		sec = BT_SECURITY_L4;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		sec = BT_SECURITY_L3;
	}

	/* nothing to do */
	if (!force_pair && (conn->sec_level >= sec || conn->required_sec_level >= sec)) {
		return 0;
	}

	atomic_set_bit_to(conn->flags, BT_CONN_FORCE_PAIR, force_pair);
	conn->required_sec_level = sec;

	err = start_security(conn);

	/* reset required security level in case of error */
	if (err) {
		conn->required_sec_level = conn->sec_level;
	}

	return err;
}

bt_security_t bt_conn_get_security(const struct bt_conn *conn)
{
	return conn->sec_level;
}
#else
bt_security_t bt_conn_get_security(const struct bt_conn *conn)
{
	return BT_SECURITY_L1;
}
#endif /* CONFIG_BT_SMP */

int bt_conn_cb_register(struct bt_conn_cb *cb)
{
	if (sys_slist_find(&conn_cbs, &cb->_node, NULL)) {
		return -EEXIST;
	}

	sys_slist_append(&conn_cbs, &cb->_node);

	return 0;
}

int bt_conn_cb_unregister(struct bt_conn_cb *cb)
{
	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&conn_cbs, &cb->_node)) {
		return -ENOENT;
	}

	return 0;
}

bool bt_conn_exists_le(uint8_t id, const bt_addr_le_t *peer)
{
	struct bt_conn *conn = bt_conn_lookup_addr_le(id, peer);

	if (conn) {
		/* Connection object already exists.
		 * If the connection state is not "disconnected",then the
		 * connection was created but has not yet been disconnected.
		 * If the connection state is "disconnected" then the connection
		 * still has valid references. The last reference of the stack
		 * is released after the disconnected callback.
		 */
		LOG_WRN("Found valid connection (%p) with address %s in %s state ", conn,
			bt_addr_le_str(peer), state2str(conn->state));
		bt_conn_unref(conn);
		return true;
	}

	return false;
}

struct bt_conn *bt_conn_add_le(uint8_t id, const bt_addr_le_t *peer)
{
	struct bt_conn *conn = acl_conn_new();

	if (!conn) {
		return NULL;
	}

	conn->id = id;
	bt_addr_le_copy(&conn->le.dst, peer);
#if (defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U))
	conn->sec_level = BT_SECURITY_L1;
	conn->required_sec_level = BT_SECURITY_L1;
#endif /* CONFIG_BT_SMP */
	conn->type = BT_CONN_TYPE_LE;
	conn->tx_data_pull = l2cap_data_pull;
	conn->get_and_clear_cb = acl_get_and_clear_cb;
	conn->has_data = acl_has_data;
	conn->le.interval_min = BT_GAP_INIT_CONN_INT_MIN;
	conn->le.interval_max = BT_GAP_INIT_CONN_INT_MAX;

	return conn;
}

static struct bt_conn *conn_lookup_device_id(struct bt_conn *conns, size_t size,
				   uint8_t device_id)
{
	int i;

	for (i = 0; i < size; i++) {
		struct bt_conn *conn = bt_conn_ref(&conns[i]);

		if (!conn) {
			continue;
		}

		/* We only care about connections with a valid handle */
		if (!bt_conn_is_handle_valid(conn)) {
			bt_conn_unref(conn);
			continue;
		}

		if (conns[i].deviceId != device_id) {
			bt_conn_unref(conn);
			continue;
		}

		return conn;
	}

	return NULL;
}

struct bt_conn *bt_conn_lookup_device_id(uint8_t device_id)
{
	struct bt_conn *conn;

	conn = conn_lookup_device_id(acl_conns, ARRAY_SIZE(acl_conns), device_id);
	if (conn) {
		return conn;
	}

#if (defined(CONFIG_BT_ISO) && ((CONFIG_BT_ISO) > 0U))
	conn = conn_lookup_device_id(iso_conns, ARRAY_SIZE(iso_conns), device_id);
	if (conn) {
		return conn;
	}
#endif

 #if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	conn = conn_lookup_device_id(sco_conns, ARRAY_SIZE(sco_conns), device_id);
	if (conn) {
		return conn;
	}
#endif

	return NULL;
}

bool bt_conn_is_peer_addr_le(const struct bt_conn *conn, uint8_t id,
			     const bt_addr_le_t *peer)
{
	if (id != conn->id) {
		return false;
	}

	/* Check against conn dst address as it may be the identity address */
	if (bt_addr_le_eq(peer, &conn->le.dst)) {
		return true;
	}

	/* Check against initial connection address */
	if (conn->role == BT_HCI_ROLE_CENTRAL) {
		return bt_addr_le_eq(peer, &conn->le.resp_addr);
	}

	return bt_addr_le_eq(peer, &conn->le.init_addr);
}

struct bt_conn *bt_conn_lookup_addr_le(uint8_t id, const bt_addr_le_t *peer)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(acl_conns); i++) {
		struct bt_conn *conn = bt_conn_ref(&acl_conns[i]);

		if (!conn) {
			continue;
		}

		if (conn->type != BT_CONN_TYPE_LE) {
			bt_conn_unref(conn);
			continue;
		}

		if (!bt_conn_is_peer_addr_le(conn, id, peer)) {
			bt_conn_unref(conn);
			continue;
		}

		return conn;
	}

	return NULL;
}

struct bt_conn *bt_conn_lookup_state_le(uint8_t id, const bt_addr_le_t *peer,
					const bt_conn_state_t state)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(acl_conns); i++) {
		struct bt_conn *conn = bt_conn_ref(&acl_conns[i]);

		if (!conn) {
			continue;
		}

		if (conn->type != BT_CONN_TYPE_LE) {
			bt_conn_unref(conn);
			continue;
		}

		if (peer && !bt_conn_is_peer_addr_le(conn, id, peer)) {
			bt_conn_unref(conn);
			continue;
		}

		if (!(conn->state == state && conn->id == id)) {
			bt_conn_unref(conn);
			continue;
		}

		return conn;
	}

	return NULL;
}

const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn)
{
	const bt_addr_le_t *dst;
	if (BT_CONN_TYPE_LE == conn->type)
	{
		dst = &conn->le.dst;
	}
	else
	{
		dst = NULL;
	}
	return dst;
}

const bt_addr_t *bt_conn_get_dst_br(const struct bt_conn *conn)
{
	const bt_addr_t *dst;
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	if (BT_CONN_TYPE_BR == conn->type)
	{
		dst = &conn->br.dst;
	}
	else
#endif
	{
		dst = NULL;
	}
	return dst;
}

static enum bt_conn_state conn_internal_to_public_state(bt_conn_state_t state)
{
	switch (state) {
	case BT_CONN_DISCONNECTED:
	case BT_CONN_DISCONNECT_COMPLETE:
		return BT_CONN_STATE_DISCONNECTED;
	case BT_CONN_SCAN_BEFORE_INITIATING:
	case BT_CONN_INITIATING_FILTER_LIST:
	case BT_CONN_ADV_CONNECTABLE:
	case BT_CONN_ADV_DIR_CONNECTABLE:
	case BT_CONN_INITIATING:
		return BT_CONN_STATE_CONNECTING;
	case BT_CONN_CONNECTED:
		return BT_CONN_STATE_CONNECTED;
	case BT_CONN_DISCONNECTING:
		return BT_CONN_STATE_DISCONNECTING;
	default:
		__ASSERT(false, "Invalid conn state %u", state);
		return (enum bt_conn_state)0;
	}
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	info->type = conn->type;
	info->role = conn->role;
	info->id = conn->id;
	info->state = conn_internal_to_public_state(conn->state);
	info->security.flags = (enum bt_security_flag)0;
	info->security.level = bt_conn_get_security(conn);
#if (defined(CONFIG_BT_SMP) && (CONFIG_BT_SMP > 0)) || \
	(defined(CONFIG_BT_CLASSIC) && (CONFIG_BT_CLASSIC > 0))
	info->security.enc_key_size = bt_conn_enc_key_size(conn);
#else
	info->security.enc_key_size = 0;
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC */

	switch (conn->type) {
	case BT_CONN_TYPE_LE:
		info->le.dst = &conn->le.dst;
		info->le.src = &bt_dev.id_addr[conn->id];
		if (conn->role == BT_HCI_ROLE_CENTRAL) {
			info->le.local = &conn->le.init_addr;
			info->le.remote = &conn->le.resp_addr;
		} else {
			info->le.local = &conn->le.resp_addr;
			info->le.remote = &conn->le.init_addr;
		}
		info->le.interval = conn->le.interval;
		info->le.latency = conn->le.latency;
		info->le.timeout = conn->le.timeout;
#if (defined(CONFIG_BT_USER_PHY_UPDATE) && ((CONFIG_BT_USER_PHY_UPDATE) > 0U))
		info->le.phy = &conn->le.phy;
#endif
#if (defined(CONFIG_BT_USER_DATA_LEN_UPDATE) && ((CONFIG_BT_USER_DATA_LEN_UPDATE) > 0U))
		info->le.data_len = &conn->le.data_len;
#endif
		if (conn->le.keys && (conn->le.keys->flags & BT_KEYS_SC)) {
			info->security.flags |= BT_SECURITY_FLAG_SC;
		}
		if (conn->le.keys && (conn->le.keys->flags & BT_KEYS_OOB)) {
			info->security.flags |= BT_SECURITY_FLAG_OOB;
		}
		return 0;
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	case BT_CONN_TYPE_BR:
		info->br.dst = &conn->br.dst;
		return 0;
#endif
#if (defined(CONFIG_BT_ISO) && (CONFIG_BT_ISO > 0U))
	case BT_CONN_TYPE_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO_UNICAST) &&
		    conn->iso.info.type == BT_ISO_CHAN_TYPE_CONNECTED && conn->iso.acl != NULL) {
			info->le.dst = &conn->iso.acl->le.dst;
			info->le.src = &bt_dev.id_addr[conn->iso.acl->id];
		} else {
			info->le.src = BT_ADDR_LE_NONE;
			info->le.dst = BT_ADDR_LE_NONE;
		}
		return 0;
#endif
	default:
		break;
	}

	return -EINVAL;
}

int bt_conn_get_remote_info(struct bt_conn *conn,
			    struct bt_conn_remote_info *remote_info)
{
	if (!atomic_test_bit(conn->flags, BT_CONN_AUTO_FEATURE_EXCH) ||
	    (IS_ENABLED(CONFIG_BT_REMOTE_VERSION) &&
	     !atomic_test_bit(conn->flags, BT_CONN_AUTO_VERSION_INFO))) {
		return -EBUSY;
	}

	remote_info->type = conn->type;
#if (defined(CONFIG_BT_REMOTE_VERSION) && ((CONFIG_BT_REMOTE_VERSION) > 0U))
	/* The conn->rv values will be just zeroes if the operation failed */
	remote_info->version = conn->rv.version;
	remote_info->manufacturer = conn->rv.manufacturer;
	remote_info->subversion = conn->rv.subversion;
#else
	remote_info->version = 0;
	remote_info->manufacturer = 0;
	remote_info->subversion = 0;
#endif

	switch (conn->type) {
	case BT_CONN_TYPE_LE:
		remote_info->le.features = conn->le.features;
		return 0;
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	case BT_CONN_TYPE_BR:
		/* TODO: Make sure the HCI commands to read br features and
		*  extended features has finished. */
		return -ENOTSUP;
#endif
	default:
		return -EINVAL;
	}
}

/* Read Transmit Power Level HCI command */
static int bt_conn_get_tx_power_level(struct bt_conn *conn, uint8_t type,
				      int8_t *tx_power_level)
{
	int err;
	struct bt_hci_rp_read_tx_power_level *rp;
	struct net_buf *rsp;
	struct bt_hci_cp_read_tx_power_level *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_TX_POWER_LEVEL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = (struct bt_hci_cp_read_tx_power_level *)net_buf_add(buf, sizeof(*cp));
	cp->type = type;
	cp->handle = sys_cpu_to_le16(conn->handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_TX_POWER_LEVEL, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (struct bt_hci_rp_read_tx_power_level *) rsp->data;
	*tx_power_level = rp->tx_power_level;
	net_buf_unref(rsp);

	return 0;
}

#if (defined(CONFIG_BT_TRANSMIT_POWER_CONTROL) && ((CONFIG_BT_TRANSMIT_POWER_CONTROL) > 0U))
void notify_tx_power_report(struct bt_conn *conn,
			    struct bt_conn_le_tx_power_report report)
{
	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->tx_power_report) {
			callback->tx_power_report(conn, &report);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb)
	{
		if (cb->tx_power_report) {
			cb->tx_power_report(conn, &report);
		}
	}
}

int bt_conn_le_enhanced_get_tx_power_level(struct bt_conn *conn,
					   struct bt_conn_le_tx_power *tx_power)
{
	int err;
	struct bt_hci_rp_le_read_tx_power_level *rp;
	struct net_buf *rsp;
	struct bt_hci_cp_le_read_tx_power_level *cp;
	struct net_buf *buf;

	if (!tx_power->phy) {
		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_ENH_READ_TX_POWER_LEVEL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->phy = tx_power->phy;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_ENH_READ_TX_POWER_LEVEL, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *) rsp->data;
	tx_power->phy = rp->phy;
	tx_power->current_level = rp->current_tx_power_level;
	tx_power->max_level = rp->max_tx_power_level;
	net_buf_unref(rsp);

	return 0;
}

int bt_conn_le_get_remote_tx_power_level(struct bt_conn *conn,
					 enum bt_conn_le_tx_power_phy phy)
{
	struct bt_hci_cp_le_read_tx_power_level *cp;
	struct net_buf *buf;

	if (!phy) {
		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_READ_REMOTE_TX_POWER_LEVEL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->phy = phy;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_REMOTE_TX_POWER_LEVEL, buf, NULL);
}

int bt_conn_le_set_tx_power_report_enable(struct bt_conn *conn,
					  bool local_enable,
					  bool remote_enable)
{
	struct bt_hci_cp_le_set_tx_power_report_enable *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_TX_POWER_REPORT_ENABLE, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->local_enable = local_enable ? BT_HCI_LE_TX_POWER_REPORT_ENABLE :
		BT_HCI_LE_TX_POWER_REPORT_DISABLE;
	cp->remote_enable = remote_enable ? BT_HCI_LE_TX_POWER_REPORT_ENABLE :
		BT_HCI_LE_TX_POWER_REPORT_DISABLE;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_TX_POWER_REPORT_ENABLE, buf, NULL);
}
#endif /* CONFIG_BT_TRANSMIT_POWER_CONTROL */

int bt_conn_le_get_tx_power_level(struct bt_conn *conn,
				  struct bt_conn_le_tx_power *tx_power_level)
{
	int err;

	if (tx_power_level->phy != 0) {
		if (IS_ENABLED(CONFIG_BT_TRANSMIT_POWER_CONTROL)) {
			return bt_conn_le_enhanced_get_tx_power_level(conn, tx_power_level);
		} else {
			return -ENOTSUP;
		}
	}

	err = bt_conn_get_tx_power_level(conn, BT_TX_POWER_LEVEL_CURRENT,
					 &tx_power_level->current_level);
	if (err) {
		return err;
	}

	err = bt_conn_get_tx_power_level(conn, BT_TX_POWER_LEVEL_MAX,
					 &tx_power_level->max_level);
	return err;
}

#if defined(CONFIG_BT_PATH_LOSS_MONITORING)
void notify_path_loss_threshold_report(struct bt_conn *conn,
				       struct bt_conn_le_path_loss_threshold_report report)
{
	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->path_loss_threshold_report) {
			callback->path_loss_threshold_report(conn, &report);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb)
	{
		if (cb->path_loss_threshold_report) {
			cb->path_loss_threshold_report(conn, &report);
		}
	}
}

int bt_conn_le_set_path_loss_mon_param(struct bt_conn *conn,
				       const struct bt_conn_le_path_loss_reporting_param *params)
{
	struct bt_hci_cp_le_set_path_loss_reporting_parameters *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PATH_LOSS_REPORTING_PARAMETERS, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->high_threshold = params->high_threshold;
	cp->high_hysteresis = params->high_hysteresis;
	cp->low_threshold = params->low_threshold;
	cp->low_hysteresis = params->low_hysteresis;
	cp->min_time_spent = sys_cpu_to_le16(params->min_time_spent);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_PATH_LOSS_REPORTING_PARAMETERS, buf, NULL);
}

int bt_conn_le_set_path_loss_mon_enable(struct bt_conn *conn, bool reporting_enable)
{
	struct bt_hci_cp_le_set_path_loss_reporting_enable *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PATH_LOSS_REPORTING_ENABLE, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->enable = reporting_enable ? BT_HCI_LE_PATH_LOSS_REPORTING_ENABLE :
			BT_HCI_LE_PATH_LOSS_REPORTING_DISABLE;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_PATH_LOSS_REPORTING_ENABLE, buf, NULL);
}
#endif /* CONFIG_BT_PATH_LOSS_MONITORING */

int bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param)
{
	LOG_DBG("conn %p features 0x%02x params (%d-%d %d %d)", conn, conn->le.features[0],
		param->interval_min, param->interval_max, param->latency, param->timeout);

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_CONN_ROLE_CENTRAL) {
		return send_conn_le_param_update(conn, param);
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		/* if peripheral conn param update timer expired just send request */
		if (atomic_test_bit(conn->flags, BT_CONN_PERIPHERAL_PARAM_UPDATE)) {
			return send_conn_le_param_update(conn, param);
		}

		/* store new conn params to be used by update timer */
		conn->le.interval_min = param->interval_min;
		conn->le.interval_max = param->interval_max;
		conn->le.pending_latency = param->latency;
		conn->le.pending_timeout = param->timeout;
		atomic_set_bit(conn->flags, BT_CONN_PERIPHERAL_PARAM_SET);
	}

	return 0;
}

#if (defined(CONFIG_BT_USER_DATA_LEN_UPDATE) && ((CONFIG_BT_USER_DATA_LEN_UPDATE) > 0U))
int bt_conn_le_data_len_update(struct bt_conn *conn,
			       const struct bt_conn_le_data_len_param *param)
{
	if (conn->le.data_len.tx_max_len == param->tx_max_len &&
	    conn->le.data_len.tx_max_time == param->tx_max_time) {
		return -EALREADY;
	}

	return bt_le_set_data_len(conn, param->tx_max_len, param->tx_max_time);
}
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */

#if (defined(CONFIG_BT_USER_PHY_UPDATE) && ((CONFIG_BT_USER_PHY_UPDATE) > 0U))
int bt_conn_le_phy_update(struct bt_conn *conn,
			  const struct bt_conn_le_phy_param *param)
{
	uint8_t phy_opts, all_phys;

	if ((param->options & BT_CONN_LE_PHY_OPT_CODED_S2) &&
	    (param->options & BT_CONN_LE_PHY_OPT_CODED_S8)) {
		phy_opts = BT_HCI_LE_PHY_CODED_ANY;
	} else if (param->options & BT_CONN_LE_PHY_OPT_CODED_S2) {
		phy_opts = BT_HCI_LE_PHY_CODED_S2;
	} else if (param->options & BT_CONN_LE_PHY_OPT_CODED_S8) {
		phy_opts = BT_HCI_LE_PHY_CODED_S8;
	} else {
		phy_opts = BT_HCI_LE_PHY_CODED_ANY;
	}

	all_phys = 0U;
	if (param->pref_tx_phy == BT_GAP_LE_PHY_NONE) {
		all_phys |= BT_HCI_LE_PHY_TX_ANY;
	}

	if (param->pref_rx_phy == BT_GAP_LE_PHY_NONE) {
		all_phys |= BT_HCI_LE_PHY_RX_ANY;
	}

	return bt_le_set_phy(conn, all_phys, param->pref_tx_phy,
			     param->pref_rx_phy, phy_opts);
}
#endif

#if (defined(CONFIG_BT_CENTRAL) && ((CONFIG_BT_CENTRAL) > 0U))
static void bt_conn_set_param_le(struct bt_conn *conn,
				 const struct bt_le_conn_param *param)
{
	conn->le.interval_min = param->interval_min;
	conn->le.interval_max = param->interval_max;
	conn->le.latency = param->latency;
	conn->le.timeout = param->timeout;
}

static void create_param_setup(const struct bt_conn_le_create_param *param)
{
	bt_dev.create_param = *param;

	bt_dev.create_param.timeout =
		(bt_dev.create_param.timeout != 0) ?
		bt_dev.create_param.timeout :
		(MSEC_PER_SEC / 10) * CONFIG_BT_CREATE_CONN_TIMEOUT;

	bt_dev.create_param.interval_coded =
		(bt_dev.create_param.interval_coded != 0) ?
		bt_dev.create_param.interval_coded :
		bt_dev.create_param.interval;

	bt_dev.create_param.window_coded =
		(bt_dev.create_param.window_coded != 0) ?
		bt_dev.create_param.window_coded :
		bt_dev.create_param.window;
}

#if (defined(CONFIG_BT_FILTER_ACCEPT_LIST) && ((CONFIG_BT_FILTER_ACCEPT_LIST) > 0U))
int bt_conn_le_create_auto(const struct bt_conn_le_create_param *create_param,
			   const struct bt_le_conn_param *param)
{
	struct bt_conn *conn;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (!bt_le_conn_params_valid(param)) {
		return -EINVAL;
	}

	conn = bt_conn_lookup_state_le(BT_ID_DEFAULT, BT_ADDR_LE_NONE,
				       BT_CONN_INITIATING_FILTER_LIST);
	if (conn) {
		bt_conn_unref(conn);
		return -EALREADY;
	}

	/* Scanning either to connect or explicit scan, either case scanner was
	 * started by application and should not be stopped.
	 */
	if (!BT_LE_STATES_SCAN_INIT(bt_dev.le.states) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		return -EINVAL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING)) {
		return -EINVAL;
	}

	if (!bt_id_scan_random_addr_check()) {
		return -EINVAL;
	}

	conn = bt_conn_add_le(BT_ID_DEFAULT, BT_ADDR_LE_NONE);
	if (!conn) {
		return -ENOMEM;
	}

	bt_conn_set_param_le(conn, param);
	create_param_setup(create_param);

	atomic_set_bit(conn->flags, BT_CONN_AUTO_CONNECT);
	bt_conn_set_state(conn, BT_CONN_INITIATING_FILTER_LIST);

	err = bt_le_create_conn(conn);
	if (err) {
		LOG_ERR("Failed to start filtered scan");
		conn->err = 0;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		bt_conn_unref(conn);
		return err;
	}

	/* Since we don't give the application a reference to manage in
	 * this case, we need to release this reference here.
	 */
	bt_conn_unref(conn);
	return 0;
}

int bt_conn_create_auto_stop(void)
{
	struct bt_conn *conn;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EINVAL;
	}

	conn = bt_conn_lookup_state_le(BT_ID_DEFAULT, BT_ADDR_LE_NONE,
				       BT_CONN_INITIATING_FILTER_LIST);
	if (!conn) {
		return -EINVAL;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING)) {
		return -EINVAL;
	}

	bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
	bt_conn_unref(conn);

	err = bt_le_create_conn_cancel();
	if (err) {
		LOG_ERR("Failed to stop initiator");
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

static int conn_le_create_common_checks(const bt_addr_le_t *peer,
					const struct bt_le_conn_param *conn_param)
{

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (!bt_le_conn_params_valid(conn_param)) {
		return -EINVAL;
	}

	if (!BT_LE_STATES_SCAN_INIT(bt_dev.le.states) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return -EAGAIN;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING)) {
		return -EALREADY;
	}

	if (!bt_id_scan_random_addr_check()) {
		return -EINVAL;
	}

	if (bt_conn_exists_le(BT_ID_DEFAULT, peer)) {
		return -EINVAL;
	}

	return 0;
}

static struct bt_conn *conn_le_create_helper(const bt_addr_le_t *peer,
				     const struct bt_le_conn_param *conn_param)
{
	bt_addr_le_t dst;
	struct bt_conn *conn;

	if (bt_addr_le_is_resolved(peer)) {
		bt_addr_le_copy_resolved(&dst, peer);
	} else {
		bt_addr_le_copy(&dst, bt_lookup_id_addr(BT_ID_DEFAULT, peer));
	}

	/* Only default identity supported for now */
	conn = bt_conn_add_le(BT_ID_DEFAULT, &dst);
	if (!conn) {
		return NULL;
	}

	bt_conn_set_param_le(conn, conn_param);

	return conn;
}

int bt_conn_le_create(const bt_addr_le_t *peer, const struct bt_conn_le_create_param *create_param,
		      const struct bt_le_conn_param *conn_param, struct bt_conn **ret_conn)
{
	struct bt_conn *conn;
	int err;

	err = conn_le_create_common_checks(peer, conn_param);
	if (err) {
		return err;
	}

	conn = conn_le_create_helper(peer, conn_param);
	if (!conn) {
		return -ENOMEM;
	}

	create_param_setup(create_param);

#if (defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U))
	if (bt_dev.le.rl_entries > bt_dev.le.rl_size) {
		/* Use host-based identity resolving. */
		bt_conn_set_state(conn, BT_CONN_SCAN_BEFORE_INITIATING);

		err = bt_le_scan_update(true);
		if (err) {
			bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
			bt_conn_unref(conn);

			return err;
		}

		*ret_conn = conn;
		return 0;
	}
#endif

	bt_conn_set_state(conn, BT_CONN_INITIATING);

	err = bt_le_create_conn(conn);
	if (err) {
		conn->err = 0;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		bt_conn_unref(conn);

		bt_le_scan_update(false);
		return err;
	}

	*ret_conn = conn;
	return 0;
}

int bt_conn_le_create_synced(const struct bt_le_ext_adv *adv,
			     const struct bt_conn_le_create_synced_param *synced_param,
			     const struct bt_le_conn_param *conn_param, struct bt_conn **ret_conn)
{
	struct bt_conn *conn;
	int err;

	err = conn_le_create_common_checks(synced_param->peer, conn_param);
	if (err) {
		return err;
	}

	if (!atomic_test_bit(adv->flags, BT_PER_ADV_ENABLED)) {
		return -EINVAL;
	}

	if (!BT_FEAT_LE_PAWR_ADVERTISER(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (synced_param->subevent >= BT_HCI_PAWR_SUBEVENT_MAX) {
		return -EINVAL;
	}

	conn = conn_le_create_helper(synced_param->peer, conn_param);
	if (!conn) {
		return -ENOMEM;
	}

	/* The connection creation timeout is not really useful for PAwR.
	 * The controller will give a result for the connection attempt
	 * within a periodic interval. We do not know the periodic interval
	 * used, so disable the timeout.
	 */
	bt_dev.create_param.timeout = 0;
	bt_conn_set_state(conn, BT_CONN_INITIATING);

	err = bt_le_create_conn_synced(conn, adv, synced_param->subevent);
	if (err) {
		conn->err = 0;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		bt_conn_unref(conn);

		return err;
	}

	*ret_conn = conn;
	return 0;
}

#if !(defined(CONFIG_BT_FILTER_ACCEPT_LIST) && (CONFIG_BT_FILTER_ACCEPT_LIST > 0))
int bt_le_set_auto_conn(const bt_addr_le_t *addr,
			const struct bt_le_conn_param *param)
{
	struct bt_conn *conn;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (param && !bt_le_conn_params_valid(param)) {
		return -EINVAL;
	}

	if (!bt_id_scan_random_addr_check()) {
		return -EINVAL;
	}

	/* Only default identity is supported */
	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
	if (!conn) {
		conn = bt_conn_add_le(BT_ID_DEFAULT, addr);
		if (!conn) {
			return -ENOMEM;
		}
	}

	if (param) {
		bt_conn_set_param_le(conn, param);

		if (!atomic_test_and_set_bit(conn->flags,
					     BT_CONN_AUTO_CONNECT)) {
			bt_conn_ref(conn);
		}
	} else {
		if (atomic_test_and_clear_bit(conn->flags,
					      BT_CONN_AUTO_CONNECT)) {
			bt_conn_unref(conn);
			if (conn->state == BT_CONN_SCAN_BEFORE_INITIATING) {
				bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
			}
		}
	}

	if (conn->state == BT_CONN_DISCONNECTED &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		if (param) {
			bt_conn_set_state(conn, BT_CONN_SCAN_BEFORE_INITIATING);
		}
		bt_le_scan_update(false);
	}

	bt_conn_unref(conn);

	return 0;
}
#endif /* !defined(CONFIG_BT_FILTER_ACCEPT_LIST) */
#endif /* CONFIG_BT_CENTRAL */

int bt_conn_le_conn_update(struct bt_conn *conn,
			   const struct bt_le_conn_param *param)
{
	struct hci_cp_le_conn_update *conn_update;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_UPDATE,
				sizeof(*conn_update));
	if (!buf) {
		return -ENOBUFS;
	}

	conn_update = (struct hci_cp_le_conn_update *)net_buf_add(buf, sizeof(*conn_update));
	(void)memset(conn_update, 0, sizeof(*conn_update));
	conn_update->handle = sys_cpu_to_le16(conn->handle);
	conn_update->conn_interval_min = sys_cpu_to_le16(param->interval_min);
	conn_update->conn_interval_max = sys_cpu_to_le16(param->interval_max);
	conn_update->conn_latency = sys_cpu_to_le16(param->latency);
	conn_update->supervision_timeout = sys_cpu_to_le16(param->timeout);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CONN_UPDATE, buf, NULL);
}

#if ((defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U)) || (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U)))
int bt_conn_auth_cb_register(const struct bt_conn_auth_cb *cb)
{
	if (!cb) {
		bt_auth = NULL;
		return 0;
	}

	if (bt_auth) {
		return -EALREADY;
	}

	/* The cancel callback must always be provided if the app provides
	 * interactive callbacks.
	 */
	if (!cb->cancel &&
	    (cb->passkey_display || cb->passkey_entry || cb->passkey_confirm ||
#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	     cb->pincode_entry ||
#endif
	     cb->pairing_confirm)) {
		return -EINVAL;
	}

	bt_auth = cb;

	bt_smp_update_io_cap(bt_auth);

	return 0;
}

#if (defined(CONFIG_BT_SMP) && (CONFIG_BT_SMP > 0))
int bt_conn_auth_cb_overlay(struct bt_conn *conn, const struct bt_conn_auth_cb *cb)
{
	CHECKIF(conn == NULL) {
		return -EINVAL;
	}

	/* The cancel callback must always be provided if the app provides
	 * interactive callbacks.
	 */
	if (cb && !cb->cancel &&
	    (cb->passkey_display || cb->passkey_entry || cb->passkey_confirm ||
	     cb->pairing_confirm)) {
		return -EINVAL;
	}

	if (conn->type == BT_CONN_TYPE_LE) {
		return bt_smp_auth_cb_overlay(conn, cb);
	}

	return -ENOTSUP;
}
#endif

int bt_conn_auth_info_cb_register(struct bt_conn_auth_info_cb *cb)
{
	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	if (sys_slist_find(&bt_auth_info_cbs, &cb->node, NULL)) {
		return -EALREADY;
	}

	sys_slist_append(&bt_auth_info_cbs, &cb->node);

	return 0;
}

int bt_conn_auth_info_cb_unregister(struct bt_conn_auth_info_cb *cb)
{
	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&bt_auth_info_cbs, &cb->node)) {
		return -EALREADY;
	}

	return 0;
}

int bt_conn_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	if (IS_ENABLED(CONFIG_BT_SMP) && conn->type == BT_CONN_TYPE_LE) {
		return bt_smp_auth_passkey_entry(conn, passkey);
	}

#if (defined(CONFIG_BT_CLASSIC) && ((CONFIG_BT_CLASSIC) > 0U))
	if (IS_ENABLED(CONFIG_BT_CLASSIC) && conn->type == BT_CONN_TYPE_BR) {
		if (!bt_auth) {
			return -EINVAL;
		}

		return bt_ssp_auth_passkey_entry(conn, passkey);
	}
#endif /* CONFIG_BT_CLASSIC */

	return -EINVAL;
}

#if (defined(CONFIG_BT_PASSKEY_KEYPRESS) && (CONFIG_BT_PASSKEY_KEYPRESS > 0))
int bt_conn_auth_keypress_notify(struct bt_conn *conn,
				 enum bt_conn_auth_keypress type)
{
	if (IS_ENABLED(CONFIG_BT_SMP) && conn->type == BT_CONN_TYPE_LE) {
		return bt_smp_auth_keypress_notify(conn, type);
	}

	LOG_ERR("Not implemented for conn type %d", conn->type);
	return -EINVAL;
}
#endif

int bt_conn_auth_passkey_confirm(struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_SMP) && conn->type == BT_CONN_TYPE_LE) {
		return bt_smp_auth_passkey_confirm(conn);
	}

#if (defined(CONFIG_BT_CLASSIC) && (CONFIG_BT_CLASSIC > 0))
	if (IS_ENABLED(CONFIG_BT_CLASSIC) && conn->type == BT_CONN_TYPE_BR) {
		if (!bt_auth) {
			return -EINVAL;
		}

		return bt_ssp_auth_passkey_confirm(conn);
	}
#endif
	return -EINVAL;
}

int bt_conn_auth_cancel(struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_SMP) && conn->type == BT_CONN_TYPE_LE) {
		return bt_smp_auth_cancel(conn);
	}

#if (defined(CONFIG_BT_CLASSIC) && (CONFIG_BT_CLASSIC > 0))
	if (IS_ENABLED(CONFIG_BT_CLASSIC) && conn->type == BT_CONN_TYPE_BR) {
		if (!bt_auth) {
			return -EINVAL;
		}

		return bt_ssp_auth_cancel(conn);
	}
#endif

	return -EINVAL;
}

int bt_conn_auth_pairing_confirm(struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_SMP) && conn->type == BT_CONN_TYPE_LE) {
		return bt_smp_auth_pairing_confirm(conn);
	}

#if (defined(CONFIG_BT_CLASSIC) && (CONFIG_BT_CLASSIC > 0))
	if (IS_ENABLED(CONFIG_BT_CLASSIC) && conn->type == BT_CONN_TYPE_BR) {
		if (!bt_auth) {
			return -EINVAL;
		}

		return bt_ssp_auth_pairing_confirm(conn);
	}
#endif

	return -EINVAL;
}
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC */

struct bt_conn *bt_conn_lookup_index(uint8_t index)
{
	if (index >= ARRAY_SIZE(acl_conns)) {
		return NULL;
	}

	return bt_conn_ref(&acl_conns[index]);
}

int bt_conn_init(void)
{
	int err, i;

	k_fifo_init(&free_tx);
	for (i = 0; i < ARRAY_SIZE(conn_tx); i++) {
		k_fifo_put(&free_tx, &conn_tx[i]);
	}

#if !(defined (CONFIG_BT_BLE_DISABLE) && (CONFIG_BT_BLE_DISABLE > 0U))
	bt_att_init();
#endif

	err = bt_smp_init();
	if (err) {
		return err;
	}

	bt_l2cap_init();

	/* Initialize background scan */
	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		for (i = 0; i < ARRAY_SIZE(acl_conns); i++) {
			struct bt_conn *conn = bt_conn_ref(&acl_conns[i]);

			if (!conn) {
				continue;
			}

#if !(defined(CONFIG_BT_FILTER_ACCEPT_LIST) && (CONFIG_BT_FILTER_ACCEPT_LIST > 0))
			if (atomic_test_bit(conn->flags,
					    BT_CONN_AUTO_CONNECT)) {
				/* Only the default identity is supported */
				conn->id = BT_ID_DEFAULT;
				bt_conn_set_state(conn,
						  BT_CONN_SCAN_BEFORE_INITIATING);
			}
#endif /* !defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

			bt_conn_unref(conn);
		}
	}

	return 0;
}

#if (defined(CONFIG_BT_DF_CONNECTION_CTE_RX) && (CONFIG_BT_DF_CONNECTION_CTE_RX > 0))
void bt_hci_le_df_connection_iq_report_common(uint8_t event, struct net_buf *buf)
{
	struct bt_df_conn_iq_samples_report iq_report;
	struct bt_conn *conn;
	int err;

	if (event == BT_HCI_EVT_LE_CONNECTION_IQ_REPORT) {
		err = hci_df_prepare_connection_iq_report(buf, &iq_report, &conn);
		if (err) {
			LOG_ERR("Prepare CTE conn IQ report failed %d", err);
			return;
		}
	} else if (IS_ENABLED(CONFIG_BT_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES) &&
		   event == BT_HCI_EVT_VS_LE_CONNECTION_IQ_REPORT) {
		err = hci_df_vs_prepare_connection_iq_report(buf, &iq_report, &conn);
		if (err) {
			LOG_ERR("Prepare CTE conn IQ report failed %d", err);
			return;
		}
	} else {
		LOG_ERR("Unhandled VS connection IQ report");
		return;
	}

	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->cte_report_cb) {
			callback->cte_report_cb(conn, &iq_report);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb)
	{
		if (cb->cte_report_cb) {
			cb->cte_report_cb(conn, &iq_report);
		}
	}

	bt_conn_unref(conn);
}

void bt_hci_le_df_connection_iq_report(struct net_buf *buf)
{
	bt_hci_le_df_connection_iq_report_common(BT_HCI_EVT_LE_CONNECTION_IQ_REPORT, buf);
}

#if (defined(CONFIG_BT_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES) && (CONFIG_BT_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES > 0))
void bt_hci_le_vs_df_connection_iq_report(struct net_buf *buf)
{
	bt_hci_le_df_connection_iq_report_common(BT_HCI_EVT_VS_LE_CONNECTION_IQ_REPORT, buf);
}
#endif /* CONFIG_BT_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES */
#endif /* CONFIG_BT_DF_CONNECTION_CTE_RX */

#if (defined(CONFIG_BT_DF_CONNECTION_CTE_REQ) && (CONFIG_BT_DF_CONNECTION_CTE_REQ > 0))
void bt_hci_le_df_cte_req_failed(struct net_buf *buf)
{
	struct bt_df_conn_iq_samples_report iq_report;
	struct bt_conn *conn;
	int err;

	err = hci_df_prepare_conn_cte_req_failed(buf, &iq_report, &conn);
	if (err) {
		LOG_ERR("Prepare CTE REQ failed IQ report failed %d", err);
		return;
	}

	struct bt_conn_cb *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_cbs, callback, _node) {
		if (callback->cte_report_cb) {
			callback->cte_report_cb(conn, &iq_report);
		}
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb)
	{
		if (cb->cte_report_cb) {
			cb->cte_report_cb(conn, &iq_report);
		}
	}

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_DF_CONNECTION_CTE_REQ */

#endif /* CONFIG_BT_CONN */

int bt_conn_scan(bt_conn_scan_cb_t cb, void *param)
{
    size_t index;
    bool stop = false;

    if (NULL == cb)
    {
        return -EINVAL;
    }

    for (index = 0; index < ARRAY_SIZE(acl_conns); index++)
    {
        if (0 == acl_conns[index].ref) {
            continue;
        }

        /* We only care about connections with a valid handle */
        if (true != bt_conn_is_handle_valid(&acl_conns[index]))
        {
            continue;
        }

        bt_conn_ref(&acl_conns[index]);
        stop = cb(&acl_conns[index], param);
        bt_conn_unref(&acl_conns[index]);
        if (true == stop)
        {
            break;
        }
    }
    return 0;
}
