/* att.c - Attribute protocol handling */

/*
 * Copyright 2021 NXP
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "porting.h"
#include <string.h>
#include <errno/errno.h>
#include <stdbool.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include "BT_att_api.h"
#include "BT_gatt_db_api.h"

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/l2cap.h>

#define LOG_ENABLE IS_ENABLED(CONFIG_BT_DEBUG_ATT)
#define LOG_MODULE_NAME bt_att
#include "fsl_component_log.h"
LOG_MODULE_DEFINE(LOG_MODULE_NAME, kLOG_LevelTrace);

#include "bt_pal_hci_core.h"
#include "bt_pal_conn_internal.h"
#include "bt_pal_l2cap_internal.h"
#include "bt_pal_smp.h"
#include "bt_pal_att_internal.h"
#include "bt_pal_gatt_internal.h"

#define ATT_CHAN(_ch) CONTAINER_OF((_ch), struct bt_att_chan, chan.chan)
#define ATT_REQ(_node) CONTAINER_OF((_node), struct bt_att_req, node)

#define ATT_CMD_MASK				0x40


#ifndef BT_ATT_MAX_GROUP_COUNT
#define BT_ATT_MAX_GROUP_COUNT 10u
#endif /* BT_ATT_MAX_RSP_LENGTH */

#ifndef BT_ATT_MAX_LIST_COUNT
#define BT_ATT_MAX_LIST_COUNT 4u
#endif /* BT_ATT_MAX_RSP_LENGTH */

#ifndef BT_ATT_MAX_MULTIPLE_READ_COUNT
#define BT_ATT_MAX_MULTIPLE_READ_COUNT 4u
#endif /* BT_ATT_MAX_MULTIPLE_READ_COUNT */

#ifndef BT_ATT_MAX_MULTIPLE_VARIABLE_LENGTH_READ_COUNT
#define BT_ATT_MAX_MULTIPLE_VARIABLE_LENGTH_READ_COUNT 4u
#endif /* BT_ATT_MAX_MULTIPLE_VARIABLE_LENGTH_READ_COUNT */

#if (defined(CONFIG_BT_EATT) && ((CONFIG_BT_EATT) > 0U))
#define ATT_CHAN_MAX				(CONFIG_BT_EATT_MAX + 1)
#else
#define ATT_CHAN_MAX				1
#endif /* CONFIG_BT_EATT */

typedef enum __packed {
		ATT_COMMAND,
		ATT_REQUEST,
		ATT_RESPONSE,
		ATT_NOTIFICATION,
		ATT_CONFIRMATION,
		ATT_INDICATION,
		ATT_UNKNOWN,
} att_type_t;

static att_type_t att_op_get_type(uint8_t op);

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
struct bt_attr_data {
	uint16_t handle;
	uint16_t offset;
};

/* Pool for incoming ATT packets */
NET_BUF_POOL_DEFINE(prep_pool, CONFIG_BT_ATT_PREPARE_COUNT, BT_ATT_BUF_SIZE,
			sizeof(struct bt_attr_data), NULL);
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */

K_MEM_SLAB_DEFINE(req_slab, sizeof(struct bt_att_req),
		  CONFIG_BT_ATT_TX_COUNT, __alignof__(struct bt_att_req));

enum {
	ATT_PENDING_RSP,
	ATT_PENDING_CFM,
	ATT_CONNECTED,
	ATT_DISCONNECTED,
	ATT_ENHANCED,
	ATT_PENDING_SENT,
	ATT_OUT_OF_SYNC_SENT,

	/* Total number of flags - must be at the end of the enum */
	ATT_NUM_FLAGS,
};

struct bt_att_tx_meta_data;
typedef void (*bt_att_tx_cb_t)(struct bt_conn *conn,
				   struct bt_att_tx_meta_data *user_data);

struct bt_att_tx_meta_data {
	int err;
	uint8_t opcode;
	uint16_t attr_count;
	struct bt_att_chan *att_chan;
	bt_gatt_complete_func_t func;
	void *user_data;
	enum bt_att_chan_opt chan_opt;
};

struct bt_att_tx_meta {
	struct bt_att_tx_meta_data *data;
};

/* ATT channel specific data */
struct bt_att_chan {
	/* Connection this channel is associated with */
	struct bt_att		*att;
	struct bt_l2cap_le_chan	chan;
	ATOMIC_DEFINE(flags, ATT_NUM_FLAGS);
	struct bt_att_req	*req;
	struct k_fifo		tx_queue;
	struct k_fifo		rx_queue;
	ATT_HANDLE		handle;
	struct k_work		rx_work;
	struct k_work_delayable	timeout_work;
	sys_snode_t		node;
};

static bool bt_att_is_enhanced(struct bt_att_chan *chan)
{
	/* Optimization. */
	if (!IS_ENABLED(CONFIG_BT_EATT)) {
		return false;
	}
	/*
	 * EDEGFAST Bluetooth: Added checking condition here.
	 * This change used to avoid MISRA issue.
	 */
	else {
		return atomic_test_bit(chan->flags, ATT_ENHANCED);
	}
}

static uint16_t bt_att_mtu(struct bt_att_chan *chan)
{
	/* Core v5.3 Vol 3 Part F 3.4.2:
	 *
	 *		 The server and client shall set ATT_MTU to the minimum of the
	 *		 Client Rx MTU and the Server Rx MTU.
	 */
	return MIN(chan->chan.rx.mtu, chan->chan.tx.mtu);
}

/* Descriptor of application-specific authorization callbacks that are used
 * with the CONFIG_BT_GATT_AUTHORIZATION_CUSTOM Kconfig enabled.
 */
#if (defined (CONFIG_BT_GATT_AUTHORIZATION_CUSTOM) && (CONFIG_BT_GATT_AUTHORIZATION_CUSTOM))
const static struct bt_gatt_authorization_cb *authorization_cb;
#endif
/* ATT connection specific data */
struct bt_att {
	struct bt_conn		*conn;
	/* Shared request queue */
	sys_slist_t		reqs;
	struct k_fifo		tx_queue;
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
	sys_slist_t		prep_queue;
#endif
	/* Contains bt_att_chan instance(s) */
	sys_slist_t		chans;
#if (defined(CONFIG_BT_EATT) && ((CONFIG_BT_EATT) > 0U))
	struct {
		struct k_work_delayable connection_work;
		uint8_t chans_to_connect;

		uint16_t prev_conn_rsp_result;
		uint16_t prev_conn_req_result;
		uint8_t prev_conn_req_missing_chans;
	} eatt;
#endif /* CONFIG_BT_EATT */
	struct k_sem		lock;
};

/* ATT handle */
struct bt_att_handle {
	ATT_HANDLE  handle;
	struct bt_att_chan *chan;
};

K_MEM_SLAB_DEFINE(att_slab, sizeof(struct bt_att),
		  CONFIG_BT_MAX_CONN, __alignof__(struct bt_att));
K_MEM_SLAB_DEFINE(chan_slab, sizeof(struct bt_att_chan),
		  CONFIG_BT_MAX_CONN * ATT_CHAN_MAX,
		  __alignof__(struct bt_att_chan));

static struct bt_att_handle att_handle_node[CONFIG_BT_MAX_CONN];

static struct bt_att_req cancel;

/** The thread ATT response handlers likely run on.
 *
 *  Blocking this thread while waiting for an ATT request to resolve can cause a
 *  deadlock.
 *
 *  This can happen if the application queues ATT requests in the context of a
 *  callback from the Bluetooth stack. This is because queuing an ATT request
 *  will block until a request-resource is available, and the callbacks run on
 *  the same thread as the ATT response handler that frees request-resources.
 *
 *  The intended use of this value is to detect the above situation.
 */
static k_tid_t att_handle_rsp_thread;

static struct bt_att_tx_meta_data tx_meta_data_storage[CONFIG_BT_ATT_TX_COUNT];

struct bt_att_tx_meta_data *bt_att_get_tx_meta_data(const struct net_buf *buf);
static void att_on_sent_cb(struct bt_att_tx_meta_data *meta);

#define att_lock(att) (void)k_sem_take(&att->lock, K_FOREVER)
#define att_unlock(att) k_sem_give(&att->lock)

#if defined(CONFIG_BT_ATT_ERR_TO_STR)
const char *bt_att_err_to_str(uint8_t att_err)
{
	/* To mapping tables are used to avoid a big gap with NULL-entries. */
	#define ATT_ERR(err) [err] = #err
	#define ATT_ERR_SECOND(err) [err - BT_ATT_ERR_WRITE_REQ_REJECTED] = #err

	const char * const first_mapping_table[] = {
		ATT_ERR(BT_ATT_ERR_SUCCESS),
		ATT_ERR(BT_ATT_ERR_INVALID_HANDLE),
		ATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED),
		ATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED),
		ATT_ERR(BT_ATT_ERR_INVALID_PDU),
		ATT_ERR(BT_ATT_ERR_AUTHENTICATION),
		ATT_ERR(BT_ATT_ERR_NOT_SUPPORTED),
		ATT_ERR(BT_ATT_ERR_INVALID_OFFSET),
		ATT_ERR(BT_ATT_ERR_AUTHORIZATION),
		ATT_ERR(BT_ATT_ERR_PREPARE_QUEUE_FULL),
		ATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_FOUND),
		ATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_LONG),
		ATT_ERR(BT_ATT_ERR_ENCRYPTION_KEY_SIZE),
		ATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN),
		ATT_ERR(BT_ATT_ERR_UNLIKELY),
		ATT_ERR(BT_ATT_ERR_INSUFFICIENT_ENCRYPTION),
		ATT_ERR(BT_ATT_ERR_UNSUPPORTED_GROUP_TYPE),
		ATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES),
		ATT_ERR(BT_ATT_ERR_DB_OUT_OF_SYNC),
		ATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED),
	};

	const char * const second_mapping_table[] = {
		ATT_ERR_SECOND(BT_ATT_ERR_WRITE_REQ_REJECTED),
		ATT_ERR_SECOND(BT_ATT_ERR_CCC_IMPROPER_CONF),
		ATT_ERR_SECOND(BT_ATT_ERR_PROCEDURE_IN_PROGRESS),
		ATT_ERR_SECOND(BT_ATT_ERR_OUT_OF_RANGE),
	};


	if (att_err < ARRAY_SIZE(first_mapping_table) && first_mapping_table[att_err]) {
		return first_mapping_table[att_err];
	} else if (att_err >= BT_ATT_ERR_WRITE_REQ_REJECTED) {
		const uint8_t second_index = att_err - BT_ATT_ERR_WRITE_REQ_REJECTED;

		if (second_index < ARRAY_SIZE(second_mapping_table) &&
			second_mapping_table[second_index]) {
			return second_mapping_table[second_index];
		}
	}

	return "(unknown)";

	#undef ATT_ERR
	#undef ATT_ERR_SECOND
}
#endif /* CONFIG_BT_ATT_ERR_TO_STR */

static void att_tx_destroy(struct net_buf *buf)
{
	struct bt_att_tx_meta_data *p_meta = bt_att_get_tx_meta_data(buf);
	struct bt_att_tx_meta_data meta;

	LOG_DBG("%p", buf);

	/* Destroy the buffer first, as the callback may attempt to allocate a
	 * new one for another operation.
	 */
	meta = *p_meta;

	/* Clear the meta storage. This might help catch illegal
	 * "use-after-free"s. An initial memset is not necessary, as the
	 * metadata storage array is `static`.
	 */
	memset(p_meta, 0x00, sizeof(*p_meta));

	/* After this point, p_meta doesn't belong to us.
	 * The user data will be memset to 0 on allocation.
	 */
	net_buf_destroy(buf);

	/* ATT opcode 0 is invalid. If we get here, that means the buffer got
	 * destroyed before it was ready to be sent. Hopefully nobody sets the
	 * opcode and then destroys the buffer without sending it. :'(
	 */
	if (meta.opcode != 0) {
		att_on_sent_cb(&meta);
	}
}

NET_BUF_POOL_DEFINE(att_pool, CONFIG_BT_ATT_TX_COUNT,
			BT_L2CAP_SDU_BUF_SIZE(BT_ATT_BUF_SIZE),
			CONFIG_BT_CONN_TX_USER_DATA_SIZE, att_tx_destroy);

struct bt_att_tx_meta_data *bt_att_get_tx_meta_data(const struct net_buf *buf)
{
	__ASSERT_NO_MSG(net_buf_pool_get(buf->pool_id) == &att_pool);

	/* Metadata lifetime is implicitly tied to the buffer lifetime.
	 * Treat it as part of the buffer itself.
	 */
	return &tx_meta_data_storage[net_buf_id((struct net_buf *)buf)];
}

static int bt_att_chan_send(struct bt_att_chan *chan, struct net_buf *buf);

static void att_chan_mtu_updated(struct bt_att_chan *updated_chan);
static void bt_att_disconnected(struct bt_l2cap_chan *chan);

struct net_buf *bt_att_create_rsp_pdu(struct bt_att_chan *chan, uint8_t op);

static void bt_att_sent(struct bt_l2cap_chan *ch);

static void att_sent(void *user_data)
{
	struct bt_att_tx_meta_data *data = user_data;
	struct bt_att_chan *att_chan = data->att_chan;
	struct bt_conn *conn = att_chan->att->conn;
	struct bt_l2cap_chan *chan = &att_chan->chan.chan;

	__ASSERT_NO_MSG(!bt_att_is_enhanced(att_chan));

	LOG_DBG("conn %p chan %p", conn, chan);

	/* For EATT, `bt_att_sent` is assigned to the `.sent` L2 callback.
	 * L2CAP will then call it once the SDU has finished sending.
	 *
	 * For UATT, this won't happen, as static LE l2cap channels don't have
	 * SDUs. Call it manually instead.
	 */
	bt_att_sent(chan);

	(void)conn;
}

static int bt_att_chan_send_req(struct bt_att_chan *chan, struct net_buf *buf, bt_conn_tx_cb_t cb, void *user_data);

/* In case of success the ownership of the buffer is transferred to the stack
 * which takes care of releasing it when it completes transmitting to the
 * controller.
 *
 * In case bt_l2cap_send_cb fails the buffer state and ownership are retained
 * so the buffer can be safely pushed back to the queue to be processed later.
 */
static int chan_send(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_hdr *hdr;
	struct net_buf_simple_state state;
	int err;
	struct bt_att_tx_meta_data *data = bt_att_get_tx_meta_data(buf);
	struct bt_att_chan *prev_chan = data->att_chan;

	hdr = (struct bt_att_hdr *)buf->data;

	LOG_DBG("code 0x%02x", hdr->code);

	if (!atomic_test_bit(chan->flags, ATT_CONNECTED)) {
		LOG_ERR("ATT channel not connected");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_EATT) && hdr->code == BT_ATT_OP_MTU_REQ &&
	    chan->chan.tx.cid != BT_L2CAP_CID_ATT) {
		/* The Exchange MTU sub-procedure shall only be supported on
		 * the LE Fixed Channel Unenhanced ATT bearer
		 */
		return -ENOTSUP;
	}

	__ASSERT_NO_MSG(buf->len >= sizeof(struct bt_att_hdr));
	data->opcode = buf->data[0];
	data->err = 0;

	if (IS_ENABLED(CONFIG_BT_EATT) && bt_att_is_enhanced(chan)) {
		/* Check if sent is pending already, if it does it cannot be
		 * modified so the operation will need to be queued.
		 */
		if (atomic_test_bit(chan->flags, ATT_PENDING_SENT)) {
			return -EAGAIN;
		}

		if (hdr->code == BT_ATT_OP_SIGNED_WRITE_CMD) {
			return -ENOTSUP;
		}

		/* Check if the channel is ready to send in case of a request */
		if (att_op_get_type(hdr->code) == ATT_REQUEST &&
		    !atomic_test_bit(chan->chan.chan.status,
				     BT_L2CAP_STATUS_OUT)) {
			return -EAGAIN;
		}

		atomic_set_bit(chan->flags, ATT_PENDING_SENT);
		data->att_chan = chan;

		/* bt_l2cap_chan_send does actually return the number of bytes
		 * that could be sent immediately.
		 */
		err = bt_l2cap_chan_send(&chan->chan.chan, buf);
		if (err < 0) {
			data->att_chan = prev_chan;
			atomic_clear_bit(chan->flags, ATT_PENDING_SENT);
			data->err = err;

			return err;
		} else {
			/* On success, the almighty scheduler might already have
			 * run the destroy cb on the buffer. In that case, buf
			 * and its metadata are dangling pointers.
			 */
			buf = NULL;
			data = NULL;
		}

		return 0;
	}

	if (hdr->code == BT_ATT_OP_SIGNED_WRITE_CMD) {
		err = bt_smp_sign(chan->att->conn, buf);
		if (err) {
			LOG_ERR("Error signing data");
			net_buf_unref(buf);
			return err;
		}
	}

	net_buf_simple_save(&buf->b, &state);

	data->att_chan = chan;

#if 0 /* EDGEFATS: Send ATT data via Ethermind */
	err = bt_l2cap_send_pdu(&chan->chan, buf, NULL, NULL);
#else
	err = bt_att_chan_send_req(chan, buf, NULL, NULL);
#endif
	if (err) {
		if (err == -ENOBUFS) {
			LOG_ERR("Ran out of TX buffers or contexts.");
		}

		if (hdr->code == BT_ATT_OP_SIGNED_WRITE_CMD) {
			buf->len -= 12;
		}
		/* In case of an error has occurred restore the buffer state */
		net_buf_simple_restore(&buf->b, &state);
		data->att_chan = prev_chan;
		data->err = err;
	}

	return err;
}

static bool att_chan_matches_chan_opt(struct bt_att_chan *chan, enum bt_att_chan_opt chan_opt)
{
	__ASSERT_NO_MSG(chan_opt <= BT_ATT_CHAN_OPT_ENHANCED_ONLY);

	if (chan_opt == BT_ATT_CHAN_OPT_NONE) {
		return true;
	}

	if (bt_att_is_enhanced(chan)) {
		return (chan_opt & BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	} else {
		return (chan_opt & BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	}
}

static struct net_buf *get_first_buf_matching_chan(struct k_fifo *fifo, struct bt_att_chan *chan)
{
	if (IS_ENABLED(CONFIG_BT_EATT)) {
		struct k_fifo skipped;
		struct net_buf *buf;
		struct net_buf *ret = NULL;
		struct bt_att_tx_meta_data *meta;

		memset(&skipped, 0, sizeof(skipped));
		k_fifo_init(&skipped);

		while ((buf = net_buf_get(fifo, K_NO_WAIT))) {
			meta = bt_att_get_tx_meta_data(buf);
			if (!ret &&
				att_chan_matches_chan_opt(chan, meta->chan_opt)) {
				ret = buf;
			} else {
				net_buf_put(&skipped, buf);
			}
		}

		__ASSERT_NO_MSG(k_fifo_is_empty(fifo));

		while ((buf = net_buf_get(&skipped, K_NO_WAIT))) {
			net_buf_put(fifo, buf);
		}

		__ASSERT_NO_MSG(k_fifo_is_empty(&skipped));

		return ret;
	} else {
		return net_buf_get(fifo, K_NO_WAIT);
	}
}

static struct bt_att_req *get_first_req_matching_chan(sys_slist_t *reqs, struct bt_att_chan *chan)
{
#if (defined(CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0))
	if (IS_ENABLED(CONFIG_BT_EATT)) {
		sys_snode_t *curr, *prev = NULL;
		struct bt_att_tx_meta_data *meta = NULL;

		SYS_SLIST_FOR_EACH_NODE(reqs, curr) {
			meta = bt_att_get_tx_meta_data(ATT_REQ(curr)->buf);
			if (att_chan_matches_chan_opt(chan, meta->chan_opt)) {
				break;
			}

			prev = curr;
		}

		if (curr) {
			sys_slist_remove(reqs, prev, curr);

			return ATT_REQ(curr);
		}

		return NULL;
	}
#else
	sys_snode_t *node = sys_slist_get(reqs);

	if (node) {
		return ATT_REQ(node);
	} else {
		return NULL;
	}
#endif
}

static int process_queue(struct bt_att_chan *chan, struct k_fifo *queue)
{
	struct net_buf *buf;
	int err;

	buf = get_first_buf_matching_chan(queue, chan);
	if (buf) {
		err = bt_att_chan_send(chan, buf);
		if (err) {
			/* Push it back if it could not be send */
			k_queue_prepend(&queue->_queue, buf);
			return err;
		}

		return 0;
	}

	return -ENOENT;
}

/* Send requests without taking tx_sem */
static int chan_req_send(struct bt_att_chan *chan, struct bt_att_req *req)
{
	struct net_buf *buf;
	int err;

	if (bt_att_mtu(chan) < net_buf_frags_len(req->buf)) {
		return -EMSGSIZE;
	}

	LOG_DBG("chan %p req %p len %zu", chan, req, net_buf_frags_len(req->buf));

	chan->req = req;

	/* Release since bt_l2cap_send_cb takes ownership of the buffer */
	buf = req->buf;
	req->buf = NULL;

	/* This lock makes sure the value of `bt_att_mtu(chan)` does not
	 * change.
	 */
	err = bt_att_chan_send(chan, buf);
	if (err) {
		/* We still have the ownership of the buffer */
		req->buf = buf;
		LOG_DBG("req %p is cleared from chan %p", chan->req, chan);
		chan->req = NULL;
	} else {
		bt_gatt_req_set_mtu(req, bt_att_mtu(chan));
	}

	return err;
}

static void bt_att_sent(struct bt_l2cap_chan *ch)
{
	struct bt_att_chan *chan = ATT_CHAN(ch);
	struct bt_att *att = chan->att;
	int err;

	LOG_DBG("chan %p", chan);

	atomic_clear_bit(chan->flags, ATT_PENDING_SENT);

	if (!att) {
		LOG_DBG("Ignore sent on detached ATT chan");
		return;
	}

	/* Process pending requests first since they require a response they
	 * can only be processed one at time while if other queues were
	 * processed before they may always contain a buffer starving the
	 * request queue.
	 */
	if (!chan->req && !sys_slist_is_empty(&att->reqs)) {
		sys_snode_t *node = sys_slist_get(&att->reqs);

		if (chan_req_send(chan, ATT_REQ(node)) >= 0) {
			return;
		}

		/* Prepend back to the list as it could not be sent */
		sys_slist_prepend(&att->reqs, node);
	}

	/* Process channel queue */
	err = process_queue(chan, &chan->tx_queue);
	if (!err) {
		return;
	}

	/* Process global queue */
	(void)process_queue(chan, &att->tx_queue);
}

static void chan_rebegin_att_timeout(struct bt_att_tx_meta_data *user_data)
{
	struct bt_att_tx_meta_data *data = user_data;
	struct bt_att_chan *chan = data->att_chan;

	LOG_DBG("chan %p chan->req %p", chan, chan->req);

	if (!atomic_test_bit(chan->flags, ATT_CONNECTED)) {
		LOG_ERR("ATT channel not connected");
		return;
	}

	/* Start timeout work. Only if we are sure that the request is really
	 * in-flight.
	 */
	if (chan->req) {
		k_work_reschedule(&chan->timeout_work, BT_ATT_TIMEOUT);
	}
}

static void chan_req_notif_sent(struct bt_att_tx_meta_data *user_data)
{
	struct bt_att_tx_meta_data *data = user_data;
	struct bt_att_chan *chan = data->att_chan;
	struct bt_conn *conn = chan->att->conn;
	bt_gatt_complete_func_t func = data->func;
	uint16_t attr_count = data->attr_count;
	void *ud = data->user_data;

	LOG_DBG("chan %p CID 0x%04X", chan, chan->chan.tx.cid);

	if (!atomic_test_bit(chan->flags, ATT_CONNECTED)) {
		LOG_ERR("ATT channel not connected");
		return;
	}

	if (func) {
		for (uint16_t i = 0; i < attr_count; i++) {
			func(conn, ud);
		}
	}
}

static void att_on_sent_cb(struct bt_att_tx_meta_data *meta)
{
	const att_type_t op_type = att_op_get_type(meta->opcode);

	LOG_DBG("opcode 0x%x", meta->opcode);

	if (!meta->att_chan ||
		!meta->att_chan->att ||
		!meta->att_chan->att->conn) {
		LOG_DBG("Bearer not connected, dropping ATT cb");
		return;
	}

	if (meta->err) {
		LOG_ERR("Got err %d, not calling ATT cb", meta->err);
		return;
	}

	if (!bt_att_is_enhanced(meta->att_chan)) {
		/* For EATT, L2CAP will call it after the SDU is fully sent. */
		LOG_DBG("UATT bearer, calling att_sent");
		att_sent(meta);
	}

	switch (op_type) {
	case ATT_RESPONSE:
		return;
	case ATT_CONFIRMATION:
		return;
	case ATT_REQUEST:
	case ATT_INDICATION:
		chan_rebegin_att_timeout(meta);
		return;
	case ATT_COMMAND:
	case ATT_NOTIFICATION:
		chan_req_notif_sent(meta);
		return;
	default:
		__ASSERT(false, "Unknown op type 0x%02X", op_type);
		return;
	}
}

static struct net_buf *bt_att_chan_create_pdu(struct bt_att_chan *chan, uint8_t op, size_t len)
{
	struct bt_att_hdr *hdr;
	struct net_buf *buf;
	struct bt_att_tx_meta_data *data;
	k_timeout_t timeout;

	if (len + sizeof(op) > bt_att_mtu(chan)) {
		LOG_WRN("ATT MTU exceeded, max %u, wanted %zu", bt_att_mtu(chan),
			len + sizeof(op));
		return NULL;
	}

	switch (att_op_get_type(op)) {
	case ATT_RESPONSE:
	case ATT_CONFIRMATION:
		/* Use a timeout only when responding/confirming */
		timeout = BT_ATT_TIMEOUT;
		break;
	default:
		timeout = K_FOREVER;
	}

	/* This will reserve headspace for lower layers */
	buf = bt_l2cap_create_pdu_timeout(&att_pool, 0, timeout);
	if (!buf) {
		LOG_ERR("Unable to allocate buffer for op 0x%02x", op);
		return NULL;
	}

	/* If we got a buf from `att_pool`, then the metadata slot at its index
	 * is officially ours to use.
	 */
	data = bt_att_get_tx_meta_data(buf);

	if (IS_ENABLED(CONFIG_BT_EATT)) {
		net_buf_reserve(buf, BT_L2CAP_SDU_BUF_SIZE(0));
	}

	data->att_chan = chan;

	hdr = (struct bt_att_hdr *)net_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}

static int bt_att_chan_send(struct bt_att_chan *chan, struct net_buf *buf)
{
	LOG_DBG("chan %p flags %lu code 0x%02x", chan, atomic_get(chan->flags),
		((struct bt_att_hdr *)buf->data)->code);

	if (IS_ENABLED(CONFIG_BT_EATT) &&
		!att_chan_matches_chan_opt(chan, bt_att_get_tx_meta_data(buf)->chan_opt)) {
		return -EINVAL;
	}

	return chan_send(chan, buf);
}

static void att_send_process(struct bt_att *att)
{
	struct bt_att_chan *chan, *tmp, *prev = NULL;
	int err = 0;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		if (err == -ENOENT && prev &&
			(bt_att_is_enhanced(chan) == bt_att_is_enhanced(prev))) {
			/* If there was nothing to send for the previous channel and the current
			 * channel has the same "enhancedness", there will be nothing to send for
			 * this channel either.
			 */
			continue;
		}

		err = process_queue(chan, &att->tx_queue);
		if (!err) {
			/* Success */
			return;
		}

		prev = chan;
	}
}

#if 0
static void bt_att_chan_send_rsp(struct bt_att_chan *chan, struct net_buf *buf)
{
	int err;

	err = chan_send(chan, buf);
	if (err) {
		/* Responses need to be sent back using the same channel */
		net_buf_put(&chan->tx_queue, buf);
	}
}
#endif

static void send_err_rsp(struct bt_att_chan *chan, uint8_t req, uint16_t handle,
			 uint8_t err)
{
	ATT_ERROR_RSP_PARAM		err_param;

	/* Ignore opcode 0x00 */
	if (!req) {
		return;
	}

	err_param.handle = sys_cpu_to_le16(handle);
	err_param.op_code = req;
	err_param.error_code = err;

	(void)BT_att_send_error_rsp(&chan->handle, &err_param);
}

static uint8_t att_mtu_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_exchange_mtu_req *req;
	uint16_t mtu_client, mtu_server;
	ATT_XCHG_MTU_RSP_PARAM xcnhgRspParam;
	uint8_t ret = 0;

	/* Exchange MTU sub-procedure shall only be supported on the
	 * LE Fixed Channel Unenhanced ATT bearer.
	 */
	if (bt_att_is_enhanced(chan)) {
		return BT_ATT_ERR_NOT_SUPPORTED;
	}

	req = (struct bt_att_exchange_mtu_req *)buf->data;

	mtu_client = sys_le16_to_cpu(req->mtu);

	LOG_DBG("Client MTU %u", mtu_client);

	/* Check if MTU is valid */
	if (mtu_client < BT_ATT_DEFAULT_LE_MTU) {
		return BT_ATT_ERR_INVALID_PDU;
	}

	mtu_server = BT_LOCAL_ATT_MTU_UATT;

	LOG_DBG("Server MTU %u", mtu_server);

	xcnhgRspParam.mtu = sys_cpu_to_le16(mtu_server);

	if(API_SUCCESS != BT_att_send_mtu_xcnhg_rsp(&chan->handle, &xcnhgRspParam))
	{
		ret = BT_ATT_ERR_UNLIKELY;
	}

	if (0 == ret)
	{
		/* The ATT_EXCHANGE_MTU_REQ/RSP is just an alternative way of
		 * communicating the L2CAP MTU.
		 */
		chan->chan.rx.mtu = mtu_server;
		chan->chan.tx.mtu = mtu_client;

		LOG_DBG("Negotiated MTU %u", bt_att_mtu(chan));
	}

#if (defined(CONFIG_BT_GATT_CLIENT) && (CONFIG_BT_GATT_CLIENT > 0))
	/* Mark the MTU Exchange as complete.
	 * This will skip sending ATT Exchange MTU from our side.
	 *
	 * Core 5.3 | Vol 3, Part F 3.4.2.2:
	 * If MTU is exchanged in one direction, that is sufficient for both directions.
	 */
	atomic_set_bit(chan->att->conn->flags, BT_CONN_ATT_MTU_EXCHANGED);
#endif /* CONFIG_BT_GATT_CLIENT */

	att_chan_mtu_updated(chan);

	return 0;
}

static int bt_att_chan_send_mtu_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_exchange_mtu_req *mtuReq;
    uint16_t mtu;
	API_RESULT retval;

	mtuReq = (struct bt_att_exchange_mtu_req *)&buf->data[sizeof(struct bt_att_hdr)];
    mtu = mtuReq->mtu;

	retval = BT_att_send_mtu_xcnhg_req
				(
					&chan->handle,
					&mtu
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_find_info_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_find_info_req *findIfoReq;
	ATT_FIND_INFO_REQ_PARAM findInfoReqParam;
	API_RESULT retval;

	findIfoReq = (struct bt_att_find_info_req *)&buf->data[sizeof(struct bt_att_hdr)];

	findInfoReqParam.start_handle = findIfoReq->start_handle;
	findInfoReqParam.end_handle = findIfoReq->end_handle;

	retval = BT_att_send_find_info_req
				(
					&chan->handle,
					&findInfoReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_find_type_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_find_type_req *findTypeReq;
	ATT_FIND_BY_TYPE_VAL_REQ_PARAM findTypeValReqParam;
	API_RESULT retval;

	findTypeReq = (struct bt_att_find_type_req *)&buf->data[sizeof(struct bt_att_hdr)];

	findTypeValReqParam.range.start_handle = findTypeReq->start_handle;
	findTypeValReqParam.range.end_handle = findTypeReq->end_handle;
	findTypeValReqParam.uuid = findTypeReq->type;
	findTypeValReqParam.value.len = buf->len - sizeof(struct bt_att_find_type_req) - sizeof(struct bt_att_hdr);
	findTypeValReqParam.value.val = &findTypeReq->value[0];

	retval = BT_att_send_find_by_type_val_req
				(
					&chan->handle,
					&findTypeValReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_read_type_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_type_req *readTypeReq;
	ATT_READ_BY_TYPE_REQ_PARAM readByTypeReq;
	API_RESULT retval;

	readTypeReq = (struct bt_att_read_type_req *)&buf->data[sizeof(struct bt_att_hdr)];

	readByTypeReq.range.start_handle = readTypeReq->start_handle;
	readByTypeReq.range.end_handle = readTypeReq->end_handle;
	if (2 == (buf->len - sizeof(struct bt_att_read_type_req) - sizeof(struct bt_att_hdr)))
	{
		readByTypeReq.uuid_format = ATT_16_BIT_UUID_FORMAT;
		memcpy(&readByTypeReq.uuid.uuid_16, readTypeReq->uuid, sizeof(readByTypeReq.uuid.uuid_16));
	}
	else
	{
		readByTypeReq.uuid_format = ATT_128_BIT_UUID_FORMAT;
		memcpy(&readByTypeReq.uuid.uuid_128.value[0], readTypeReq->uuid, sizeof(readByTypeReq.uuid.uuid_128.value));
	}

	retval = BT_att_send_read_by_type_req
				(
					&chan->handle,
					&readByTypeReq
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_read_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_req *readReq;
	ATT_READ_REQ_PARAM readReqParam;
	API_RESULT retval;

	readReq = (struct bt_att_read_req *)&buf->data[sizeof(struct bt_att_hdr)];

	readReqParam = (ATT_ATTR_HANDLE) readReq->handle;

	retval = BT_att_send_read_req
				(
					&chan->handle,
					&readReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_read_blob_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_blob_req *readBlobReq;
	ATT_READ_BLOB_REQ_PARAM readBlobReqParam;
	API_RESULT retval;

	readBlobReq = (struct bt_att_read_blob_req *)&buf->data[sizeof(struct bt_att_hdr)];

	readBlobReqParam.handle = (ATT_ATTR_HANDLE) readBlobReq->handle;
	readBlobReqParam.offset = (ATT_ATTR_HANDLE) readBlobReq->offset;

	retval = BT_att_send_read_blob_req
				(
					&chan->handle,
					&readBlobReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_read_mult_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	ATT_READ_MULTIPLE_REQ_PARAM readMultipleReqParam;
	API_RESULT retval;

	readMultipleReqParam.list_count = (buf->len - sizeof(struct bt_att_hdr))/sizeof(uint16_t);
	readMultipleReqParam.handle_list = (uint16_t *)&buf->data[sizeof(struct bt_att_hdr)];

	retval = BT_att_send_read_multiple_req
				(
					&chan->handle,
					&readMultipleReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_read_group_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_group_req *readGroupReq;
	ATT_READ_BY_GROUP_TYPE_REQ_PARAM readByGroupParam;
	API_RESULT retval;

	readGroupReq = (struct bt_att_read_group_req *)&buf->data[sizeof(struct bt_att_hdr)];

	readByGroupParam.range.start_handle = (ATT_ATTR_HANDLE) readGroupReq->start_handle;
	readByGroupParam.range.end_handle = (ATT_ATTR_HANDLE) readGroupReq->end_handle;
	readByGroupParam.uuid_format = (UCHAR) ((2 == ((buf->len - sizeof(struct bt_att_hdr)) - sizeof(*readGroupReq))) ? ATT_16_BIT_UUID_FORMAT : ATT_128_BIT_UUID_FORMAT);

	if (ATT_16_BIT_UUID_FORMAT == readByGroupParam.uuid_format)
	{
		memcpy(&readByGroupParam.group_type.uuid_16, readGroupReq->uuid, sizeof(readByGroupParam.group_type.uuid_16));
	}
	else /* if (ATT_128_BIT_UUID_FORMAT == readByGroupParam.uuid_format) */
	{
		memcpy(&readByGroupParam.group_type.uuid_128.value[0], readGroupReq->uuid, sizeof(readByGroupParam.group_type.uuid_128.value));
	}

	retval = BT_att_send_read_by_group_req
				(
					&chan->handle,
					&readByGroupParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

#if (defined(CONFIG_BT_EATT) && ((CONFIG_BT_EATT) > 0U))
static int bt_att_chan_send_read_mult_vl_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	ATT_READ_MULTIPLE_VARIABLE_LENGTH_REQ_PARAM readMultipleVlReqParam;
	API_RESULT retval;

	readMultipleVlReqParam.list_count = (buf->len - sizeof(struct bt_att_hdr))/sizeof(uint16_t);
	readMultipleVlReqParam.handle_list = (uint16_t *)&buf->data[sizeof(struct bt_att_hdr)];

	retval = BT_att_send_read_multiple_variable_length_req
				(
					&chan->handle,
					&readMultipleVlReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}
#endif /* CONFIG_BT_EATT */

static int bt_att_chan_send_write_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_write_req *writeReq;
	ATT_WRITE_REQ_PARAM writeReqParam;
	API_RESULT retval;

	writeReq = (struct bt_att_write_req *)&buf->data[sizeof(struct bt_att_hdr)];

	writeReqParam.handle = (ATT_ATTR_HANDLE) writeReq->handle;
	writeReqParam.value.len = (UINT16) ((buf->len - sizeof(struct bt_att_hdr)) - sizeof(*writeReq));
	writeReqParam.value.val = writeReq->value;

	retval = BT_att_send_write_req
				(
					&chan->handle,
					&writeReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_prepare_write_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_prepare_write_req *prepareWriteReq;
	ATT_PREPARE_WRITE_REQ_PARAM prepareWriteReqParam;
	API_RESULT retval;

	prepareWriteReq = (struct bt_att_prepare_write_req *)&buf->data[sizeof(struct bt_att_hdr)];

	prepareWriteReqParam.handle_value.handle = (ATT_ATTR_HANDLE) prepareWriteReq->handle;
	prepareWriteReqParam.offset = (UINT16) prepareWriteReq->offset;
	prepareWriteReqParam.handle_value.value.len = (UINT16) ((buf->len - sizeof(struct bt_att_hdr)) - sizeof(*prepareWriteReq));
	prepareWriteReqParam.handle_value.value.val = prepareWriteReq->value;

	retval = BT_att_send_prepare_write_req
				(
					&chan->handle,
					&prepareWriteReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_exec_write_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_exec_write_req *execWriteReq;
	ATT_EXECUTE_WRITE_REQ_PARAM executeWriteReqParam;
	API_RESULT retval;

	execWriteReq = (struct bt_att_exec_write_req *)&buf->data[sizeof(struct bt_att_hdr)];

	executeWriteReqParam.flags = (UCHAR)execWriteReq->flags;

	retval = BT_att_send_execute_write_req
				(
					&chan->handle,
					&executeWriteReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_write_cmd(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_write_cmd *writeCmd;
	ATT_WRITE_CMD_PARAM writeCmdParam;
	API_RESULT retval;

	writeCmd = (struct bt_att_write_cmd *)&buf->data[sizeof(struct bt_att_hdr)];

	writeCmdParam.handle = (ATT_ATTR_HANDLE) writeCmd->handle;
	writeCmdParam.value.len = (UINT16) ((buf->len - sizeof(struct bt_att_hdr)) - sizeof(*writeCmd));
	writeCmdParam.value.val = writeCmd->value;

	retval = BT_att_send_write_cmd
				(
					&chan->handle,
					&writeCmdParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
static int bt_att_chan_send_signed_write_cmd(struct bt_att_chan *chan, struct net_buf *buf)
{
#if (defined(CONFIG_BT_SIGNING) && (CONFIG_BT_SIGNING > 0U))
	struct bt_att_signed_write_cmd *signedWriteReq;
	ATT_SIGNED_WRITE_CMD_PARAM signWriteReqParam;
	API_RESULT retval;

	signedWriteReq = (struct bt_att_signed_write_cmd *)&buf->data[sizeof(struct bt_att_hdr)];

	signWriteReqParam.handle_value.handle = signedWriteReq->handle;
	signWriteReqParam.handle_value.value.len = buf->len - sizeof(struct bt_att_hdr) - sizeof(struct bt_att_signed_write_cmd) - sizeof(signWriteReqParam.auth_signature);
	signWriteReqParam.handle_value.value.val = signedWriteReq->value;

	memcpy(&signWriteReqParam.auth_signature[0], &signedWriteReq->value[signWriteReqParam.handle_value.value.len], sizeof(signWriteReqParam.auth_signature));

	retval = BT_att_send_signed_write_cmd
				(
					&chan->handle,
					&signWriteReqParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
#else
	return -ENOTSUP;
#endif
}
#endif /* CONFIG_BT_SIGNING */

static int bt_att_chan_send_notify(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_notify *notify;
	ATT_HANDLE_VALUE_PAIR hndlValParam;
	API_RESULT retval;

	notify = (struct bt_att_notify *)&buf->data[sizeof(struct bt_att_hdr)];

	hndlValParam.handle	= sys_cpu_to_le16(notify->handle);
	hndlValParam.value.val = (UCHAR *)notify->value;
	hndlValParam.value.len = buf->len - sizeof(struct bt_att_hdr) - sizeof(struct bt_att_notify);

	retval = BT_att_send_hndl_val_ntf
				(
					&chan->handle,
					&hndlValParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_indicate(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_indicate *indicate;
	ATT_HANDLE_VALUE_PAIR hndlValParam;
	API_RESULT retval;

	indicate = (struct bt_att_indicate *)&buf->data[sizeof(struct bt_att_hdr)];

	hndlValParam.handle	= sys_cpu_to_le16(indicate->handle);
	hndlValParam.value.val = (UCHAR *)indicate->value;
	hndlValParam.value.len = buf->len - sizeof(struct bt_att_hdr) - sizeof(struct bt_att_indicate);

	retval = BT_att_send_hndl_val_ind
				(
					&chan->handle,
					&hndlValParam
				);
	if (API_SUCCESS != retval)
	{
		return -EIO;
	}
	else
	{
		return 0;
	}
}

static int bt_att_chan_send_req(struct bt_att_chan *chan, struct net_buf *buf, bt_conn_tx_cb_t cb, void *user_data)
{
	struct bt_att_hdr *hdr;
	int err = -EINVAL;

	hdr = (struct bt_att_hdr *)buf->data;

	LOG_DBG("chan %p buf %p (code %d) cb %p user_data %p", chan, buf, hdr->code, cb, user_data);

	switch (hdr->code)
	{
	case BT_ATT_OP_MTU_REQ:
		err = bt_att_chan_send_mtu_req(chan, buf);
	break;
	case BT_ATT_OP_FIND_INFO_REQ:
		err = bt_att_chan_send_find_info_req(chan, buf);
	break;
	case BT_ATT_OP_FIND_TYPE_REQ:
		err = bt_att_chan_send_find_type_req(chan, buf);
	break;
	case BT_ATT_OP_READ_TYPE_REQ:
		err = bt_att_chan_send_read_type_req(chan, buf);
	break;
	case BT_ATT_OP_READ_REQ:
		err = bt_att_chan_send_read_req(chan, buf);
	break;
	case BT_ATT_OP_READ_BLOB_REQ:
		err = bt_att_chan_send_read_blob_req(chan, buf);
	break;
	case BT_ATT_OP_READ_MULT_REQ:
		err = bt_att_chan_send_read_mult_req(chan, buf);
	break;
	case BT_ATT_OP_READ_GROUP_REQ:
		err = bt_att_chan_send_read_group_req(chan, buf);
	break;
#if (defined(CONFIG_BT_EATT) && ((CONFIG_BT_EATT) > 0U))
	case BT_ATT_OP_READ_MULT_VL_REQ:
		err = bt_att_chan_send_read_mult_vl_req(chan, buf);
	break;
#endif /* CONFIG_BT_EATT */
	case BT_ATT_OP_WRITE_REQ:
		err = bt_att_chan_send_write_req(chan, buf);
	break;
	case BT_ATT_OP_PREPARE_WRITE_REQ:
		err = bt_att_chan_send_prepare_write_req(chan, buf);
	break;
	case BT_ATT_OP_EXEC_WRITE_REQ:
		err = bt_att_chan_send_exec_write_req(chan, buf);
	break;
	case BT_ATT_OP_WRITE_CMD:
		err = bt_att_chan_send_write_cmd(chan, buf);
	break;
#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
	case BT_ATT_OP_SIGNED_WRITE_CMD:
		err = bt_att_chan_send_signed_write_cmd(chan, buf);
	break;
#endif
	case BT_ATT_OP_NOTIFY:
		err = bt_att_chan_send_notify(chan, buf);
	break;
	case BT_ATT_OP_INDICATE:
		err = bt_att_chan_send_indicate(chan, buf);
	break;
	default:
		err = -ENOTSUP;
	break;
	}

	LOG_DBG("err %d", err);

	if (err >= 0)
	{
		net_buf_unref(buf);
		if (NULL != cb)
		{
			cb(chan->att->conn, user_data, err);
		}
	}

	return err;
}

static int bt_att_chan_req_send(struct bt_att_chan *chan,
				struct bt_att_req *req)
{
	__ASSERT_NO_MSG(chan);
	__ASSERT_NO_MSG(req);
	__ASSERT_NO_MSG(req->func);
	__ASSERT_NO_MSG(!chan->req);

	LOG_DBG("req %p", req);

	return chan_req_send(chan, req);
}

static void att_req_send_process(struct bt_att *att)
{
	struct bt_att_req *req = NULL;
	struct bt_att_chan *chan, *tmp, *prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		/* If there is an ongoing transaction, do not use the channel */
		if (chan->req) {
			continue;
		}

		if (!req && prev && (bt_att_is_enhanced(chan) == bt_att_is_enhanced(prev))) {
			/* If there was nothing to send for the previous channel and the current
			 * channel has the same "enhancedness", there will be nothing to send for
			 * this channel either.
			 */
			continue;
		}

		prev = chan;

		/* Pull next request from the list */
		req = get_first_req_matching_chan(&att->reqs, chan);
		if (!req) {
			continue;
		}

		if (bt_att_chan_req_send(chan, req) >= 0) {
			return;
		}

		/* Prepend back to the list as it could not be sent */
		sys_slist_prepend(&att->reqs, &req->node);
	}
}

static uint8_t att_handle_rsp(struct bt_att_chan *chan, void *pdu, uint16_t len,
			      int err)
{
	bt_att_func_t func = NULL;
	void *params;

	LOG_DBG("chan %p err %d len %u: %s", chan, err, len, bt_hex(pdu, len));

	/* Cancel timeout if ongoing */
	k_work_cancel_delayable(&chan->timeout_work);

	if (!chan->req) {
		LOG_WRN("No pending ATT request");
		goto process;
	}

	/* Check if request has been cancelled */
	if (chan->req == &cancel) {
		LOG_DBG("req %p is cleared from chan %p", chan->req, chan);
		chan->req = NULL;
		goto process;
	}

	/* Reset func so it can be reused by the callback */
	func = chan->req->func;
	chan->req->func = NULL;
	params = chan->req->user_data;

	/* free allocated request so its memory can be reused */
	bt_att_req_free(chan->req);
	chan->req = NULL;

process:
	/* Process pending requests */
	att_req_send_process(chan->att);
	if (func) {
		func(chan->att->conn, err, pdu, len, params);
	}

	return 0;
}

#if (defined(CONFIG_BT_GATT_CLIENT) && ((CONFIG_BT_GATT_CLIENT) > 0U))
static uint8_t att_mtu_rsp(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_exchange_mtu_rsp *rsp;
	uint16_t mtu;

	rsp = (struct bt_att_exchange_mtu_rsp *)buf->data;

	mtu = sys_le16_to_cpu(rsp->mtu);

	LOG_DBG("Server MTU %u", mtu);

	/* Check if MTU is valid */
	if (mtu < BT_ATT_DEFAULT_LE_MTU) {
		return att_handle_rsp(chan, NULL, 0, BT_ATT_ERR_INVALID_PDU);
	}

	/* The following must equal the value we sent in the req. We assume this
	 * is a rsp to `gatt_exchange_mtu_encode`.
	 */
	chan->chan.rx.mtu = BT_LOCAL_ATT_MTU_UATT;
	/* The ATT_EXCHANGE_MTU_REQ/RSP is just an alternative way of
	 * communicating the L2CAP MTU.
	 */

	chan->chan.tx.mtu = mtu;

	LOG_DBG("Negotiated MTU %u", bt_att_mtu(chan));

	att_chan_mtu_updated(chan);

	return att_handle_rsp(chan, rsp, buf->len, 0);
}
#endif /* CONFIG_BT_GATT_CLIENT */

static bool range_is_valid(uint16_t start, uint16_t end, uint16_t *err)
{
	/* Handle 0 is invalid */
	if (!start || !end) {
		if (err) {
			*err = 0U;
		}
		return false;
	}

	/* Check if range is valid */
	if (start > end) {
		if (err) {
			*err = start;
		}
		return false;
	}

	return true;
}

struct find_info_data {
	struct bt_att_chan *chan;
	size_t sofar;
	ATT_FIND_INFO_RSP_PARAM	param;
	ATT_HANDLE_UUID_PAIR	   handle_uuid_list[BT_ATT_MAX_LIST_COUNT];
};

static uint8_t find_info_cb(const struct bt_gatt_attr *attr, uint16_t handle,
				void *user_data)
{
	struct find_info_data *data = (struct find_info_data *)user_data;
	struct bt_att_chan *chan = data->chan;
	size_t len;

	LOG_DBG("handle 0x%04x", handle);

	/* Initialize rsp at first entry */
	if (NULL == data->param.handle_value_list.list)
	{
		data->param.handle_value_list.list = data->handle_uuid_list;
		data->param.uuid_format = (BT_UUID_TYPE_16 == attr->uuid->type) ?
					ATT_16_BIT_UUID_FORMAT : ATT_128_BIT_UUID_FORMAT;
		data->sofar += 1;
	}


	switch (data->param.uuid_format) {
	case ATT_16_BIT_UUID_FORMAT:
		if (attr->uuid->type != BT_UUID_TYPE_16) {
			return BT_GATT_ITER_STOP;
		}

		/* Check the sending length */
		len = sizeof(handle) + sizeof(data->param.handle_value_list.list[data->param.handle_value_list.list_count].uuid.uuid_16);
		if (bt_att_mtu(chan) < (data->sofar + len))
		{
			return BT_GATT_ITER_STOP;
		}
		/* Fast forward to next item position */
		data->param.handle_value_list.list[data->param.handle_value_list.list_count].handle = sys_cpu_to_le16(handle);
		data->param.handle_value_list.list[data->param.handle_value_list.list_count].uuid.uuid_16 = sys_cpu_to_le16(BT_UUID_16(attr->uuid)->val);
		data->param.handle_value_list.list_count++;
		data->sofar += len;

		if (data->param.handle_value_list.list_count < BT_ATT_MAX_LIST_COUNT)
		{
			return BT_GATT_ITER_CONTINUE;
		}

		break;
	case ATT_128_BIT_UUID_FORMAT:
		if (attr->uuid->type != BT_UUID_TYPE_128) {
			return BT_GATT_ITER_STOP;
		}

		/* Check the sending length */
		len = sizeof(handle) + sizeof(data->param.handle_value_list.list[data->param.handle_value_list.list_count].uuid.uuid_128.value);
		if (bt_att_mtu(chan) < (data->sofar + len))
		{
			return BT_GATT_ITER_STOP;
		}
		/* Fast forward to next item position */
		data->param.handle_value_list.list[data->param.handle_value_list.list_count].handle = sys_cpu_to_le16(handle);
		(void)memcpy(data->param.handle_value_list.list[data->param.handle_value_list.list_count].uuid.uuid_128.value, BT_UUID_128(attr->uuid)->val,
			   sizeof(data->param.handle_value_list.list[data->param.handle_value_list.list_count].uuid.uuid_128.value));
		data->param.handle_value_list.list_count++;
		data->sofar += len;

		if (data->param.handle_value_list.list_count < BT_ATT_MAX_LIST_COUNT)
		{
			return BT_GATT_ITER_CONTINUE;
		}
		break;
	default:
	  /* MISRA 16.4 : The switch statement does not have a non-empty default clause. */
		break;
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t att_find_info_rsp(struct bt_att_chan *chan, uint16_t start_handle,
				  uint16_t end_handle)
{
	struct find_info_data data;
	uint8_t ret = 0;

	(void)memset(&data, 0, sizeof(data));

	data.chan = chan;
	bt_gatt_foreach_attr(start_handle, end_handle, find_info_cb, &data);

	if (NULL != data.param.handle_value_list.list)
	{
		if (API_SUCCESS != BT_att_send_find_info_rsp (&chan->handle,&data.param))
		{
			ret = BT_ATT_ERR_UNLIKELY;
		}
	}
	else
	{
		send_err_rsp(chan, BT_ATT_OP_FIND_INFO_REQ, start_handle, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	}

	return ret;
}

static uint8_t att_find_info_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_find_info_req *req;
	uint16_t start_handle, end_handle, err_handle;

	req = (struct bt_att_find_info_req *)buf->data;

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);

	LOG_DBG("start_handle 0x%04x end_handle 0x%04x", start_handle, end_handle);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(chan, BT_ATT_OP_FIND_INFO_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	return att_find_info_rsp(chan, start_handle, end_handle);
}

struct find_type_data {
	struct bt_att_chan *chan;
	uint8_t buffer[BT_ATT_MTU];
	size_t len;
	size_t sofar;
	struct bt_att_handle_group *group;
	const void *value;
	ATT_FIND_BY_TYPE_VAL_RSP_PARAM	list;
	ATT_HANDLE_RANGE				  group_list[BT_ATT_MAX_GROUP_COUNT];
	uint8_t value_len;
	uint8_t err;
};

static uint8_t find_type_cb(const struct bt_gatt_attr *attr, uint16_t handle,
				void *user_data)
{
	struct find_type_data *data = (struct find_type_data *)user_data;
	struct bt_att_chan *chan = data->chan;
	struct bt_conn *conn = chan->chan.chan.conn;
	int read;
	uint8_t uuid[16];

	/* Skip secondary services */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
		goto skip;
	}

	/* Update group end_handle if not a primary service */
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY)) {
		if (data->group &&
			handle > sys_le16_to_cpu(data->group->end_handle)) {
			data->group->end_handle = sys_cpu_to_le16(handle);
			data->list.range[data->list.count-1].end_handle = sys_cpu_to_le16(handle);
		}
		return BT_GATT_ITER_CONTINUE;
	}

	LOG_DBG("handle 0x%04x", handle);

	/* stop if there is no space left */
	if ((data->len - data->sofar) < sizeof(*data->group)) {
		return BT_GATT_ITER_STOP;
	}

	/* Read attribute value and store in the buffer */
	read = attr->read(conn, attr, uuid, sizeof(uuid), 0);
	if (read < 0) {
		/*
		 * Since we don't know if it is the service with requested UUID,
		 * we cannot respond with an error to this request.
		 */
		goto skip;
	}

	/* Check if data matches */
	if (read != data->value_len) {
		/* Use bt_uuid_cmp() to compare UUIDs of different form. */
		struct bt_uuid_128 ref_uuid;
		struct bt_uuid_128 recvd_uuid;

		if (!bt_uuid_create(&recvd_uuid.uuid, (const uint8_t *)data->value, data->value_len)) {
			LOG_WRN("Unable to create UUID: size %u", data->value_len);
			goto skip;
		}
		if (!bt_uuid_create(&ref_uuid.uuid, uuid, read)) {
			LOG_WRN("Unable to create UUID: size %d", read);
			goto skip;
		}
		if (bt_uuid_cmp(&recvd_uuid.uuid, &ref_uuid.uuid)) {
			goto skip;
		}
	} else if (memcmp(data->value, uuid, read)) {
		goto skip;
	}

	/* If service has been found, error should be cleared */
	data->err = 0x00;

	/* Fast forward to next item position */
	data->group = (struct bt_att_handle_group *)&data->buffer[data->sofar];
	data->group->start_handle = sys_cpu_to_le16(handle);
	data->group->end_handle = sys_cpu_to_le16(handle);
	data->sofar += sizeof(*data->group);

	data->list.range[data->list.count].start_handle = sys_cpu_to_le16(handle);
	data->list.range[data->list.count].end_handle = sys_cpu_to_le16(handle);
	data->list.count++;

	/* continue to find the end_handle */
	return BT_GATT_ITER_CONTINUE;

skip:
	data->group = NULL;
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_find_type_rsp(struct bt_att_chan *chan, uint16_t start_handle,
				  uint16_t end_handle, const void *value,
				  uint8_t value_len)
{
	struct find_type_data data;
	uint8_t ret = 0;

	(void)memset(&data, 0, sizeof(data));

	data.chan = chan;
	data.group = NULL;
	data.value = value;
	data.value_len = value_len;
	data.list.range = data.group_list;
	data.len = MIN(sizeof(data.buffer), bt_att_mtu(chan));

	/* Pre-set error in case no service will be found */
	data.err = BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;

	bt_gatt_foreach_attr(start_handle, end_handle, find_type_cb, &data);

	/* If error has not been cleared, no service has been found */
	if (data.err) {
		/* Respond since handle is set */
		send_err_rsp(chan, BT_ATT_OP_FIND_TYPE_REQ, start_handle,
				 data.err);
		return 0;
	}
	else
	{
		if (API_SUCCESS != BT_att_send_find_by_type_val_rsp(&chan->handle, &data.list))
		{
			ret = BT_ATT_ERR_UNLIKELY;
		}
	}

	return ret;
}

static uint8_t att_find_type_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_find_type_req *req;
	uint16_t start_handle, end_handle, err_handle, type;
	uint8_t *value;

	req = (struct bt_att_find_type_req *)net_buf_pull_mem(buf, sizeof(*req));

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);
	type = sys_le16_to_cpu(req->type);
	value = buf->data;

	LOG_DBG("start_handle 0x%04x end_handle 0x%04x type %u", start_handle, end_handle, type);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(chan, BT_ATT_OP_FIND_TYPE_REQ, err_handle,
				 BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	/* The Attribute Protocol Find By Type Value Request shall be used with
	 * the Attribute Type parameter set to the UUID for "Primary Service"
	 * and the Attribute Value set to the 16-bit Bluetooth UUID or 128-bit
	 * UUID for the specific primary service.
	 */
	if (bt_uuid_cmp(BT_UUID_DECLARE_16(type), BT_UUID_GATT_PRIMARY)) {
		send_err_rsp(chan, BT_ATT_OP_FIND_TYPE_REQ, start_handle,
				 BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return 0;
	}

	return att_find_type_rsp(chan, start_handle, end_handle, value,
				 buf->len);
}

static uint8_t err_to_att(int err)
{
	LOG_DBG("%d", err);

	if (err < 0 && err >= -0xff) {
		return -err;
	}

	return BT_ATT_ERR_UNLIKELY;
}

struct read_type_data {
	struct bt_att_chan *chan;
	struct bt_uuid *uuid;
	uint8_t buffer[BT_ATT_MTU];
	size_t len;
	size_t sofar;
	struct bt_att_read_type_rsp *rsp;
	struct bt_att_data *item;
	ATT_READ_BY_TYPE_RSP_PARAM	 param;
	ATT_HANDLE_VALUE_PAIR		  handle_value_list[BT_ATT_MAX_GROUP_COUNT];
	uint8_t err;
};

typedef bool (*attr_read_cb)(struct net_buf *buf, ssize_t read,
				 void *user_data);
static bool attr_read_authorize(struct bt_conn *conn,
				const struct bt_gatt_attr *attr)
{
	if (!IS_ENABLED(CONFIG_BT_GATT_AUTHORIZATION_CUSTOM)) {
		return true;
	}
#if (defined (CONFIG_BT_GATT_AUTHORIZATION_CUSTOM) && (CONFIG_BT_GATT_AUTHORIZATION_CUSTOM))
	if (!authorization_cb || !authorization_cb->read_authorize) {
		return true;
	}

	return authorization_cb->read_authorize(conn, attr);
#endif
}

#if 0

static bool attr_read_type_cb(struct net_buf *frag, ssize_t read,
				  void *user_data)
{
	struct read_type_data *data = user_data;

	if (!data->rsp->len) {
		/* Set len to be the first item found */
		data->rsp->len = read + sizeof(*data->item);
	} else if (data->rsp->len != read + sizeof(*data->item)) {
		/* All items should have the same size */
		frag->len -= sizeof(*data->item);
		data->item = NULL;
		return false;
	}

	return true;
}

static ssize_t att_chan_read(struct bt_att_chan *chan,
				 const struct bt_gatt_attr *attr,
				 struct net_buf *buf, uint16_t offset,
				 attr_read_cb cb, void *user_data)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	ssize_t read;
	struct net_buf *frag;
	size_t len, total = 0;

	if (bt_att_mtu(chan) <= net_buf_frags_len(buf)) {
		return 0;
	}

	frag = net_buf_frag_last(buf);

	/* Create necessary fragments if MTU is bigger than what a buffer can
	 * hold.
	 */
	do {
		len = MIN(bt_att_mtu(chan) - net_buf_frags_len(buf),
			  net_buf_tailroom(frag));
		if (!len) {
			frag = net_buf_alloc(net_buf_pool_get(buf->pool_id),
						 osaWaitNone_c);
			/* If not buffer can be allocated immediately return */
			if (!frag) {
				return total;
			}

			net_buf_frag_add(buf, frag);

			len = MIN(bt_att_mtu(chan) - net_buf_frags_len(buf),
				  net_buf_tailroom(frag));
		}

		read = attr->read(conn, attr, frag->data + frag->len, len,
				  offset);
		if (read < 0) {
			if (total) {
				return total;
			}

			return read;
		}

		if (cb && !cb(frag, read, user_data)) {
			break;
		}

		net_buf_add(frag, read);
		total += read;
		offset += read;
	} while (bt_att_mtu(chan) > net_buf_frags_len(buf) && read == len);

	return total;
}
#endif
static uint8_t read_type_cb(const struct bt_gatt_attr *attr, uint16_t handle,
				void *user_data)
{
	struct read_type_data *data = (struct read_type_data *)user_data;
	struct bt_att_chan *chan = data->chan;
	struct bt_conn *conn = chan->chan.chan.conn;
	ssize_t read;

	/* Skip if doesn't match */
	if (bt_uuid_cmp(attr->uuid, data->uuid)) {
		return BT_GATT_ITER_CONTINUE;
	}

	LOG_DBG("handle 0x%04x", handle);

	/*
	 * If an attribute in the set of requested attributes would cause an
	 * Error Response then this attribute cannot be included in a
	 * Read By Type Response and the attributes before this attribute
	 * shall be returned
	 *
	 * If the first attribute in the set of requested attributes would
	 * cause an Error Response then no other attributes in the requested
	 * attributes can be considered.
	 */
	data->err = bt_gatt_check_perm(conn, attr, BT_GATT_PERM_READ_MASK);
	if (data->err) {
		if (data->rsp->len) {
			data->err = 0x00;
		}
		return BT_GATT_ITER_STOP;
	}

	/* Check the attribute authorization logic */
	if (!attr_read_authorize(conn, attr)) {
		data->err = BT_ATT_ERR_AUTHORIZATION;
		return BT_GATT_ITER_STOP;
	}
	/*
	 * If any attribute is founded in handle range it means that error
	 * should be changed from pre-set: attr not found error to no error.
	 */
	data->err = 0x00;

	/* Fast forward to next item position */
	data->item = (struct bt_att_data *)&data->buffer[data->sofar];
	data->item->handle = sys_cpu_to_le16(handle);
	data->sofar += sizeof(*data->item);

#if 0
	read = att_chan_read(chan, attr, data->buf, 0, attr_read_type_cb, data);
#else
	read = attr->read(conn, attr, &data->buffer[data->sofar],
			  data->len - data->sofar, 0);
#endif
	if (read < 0) {
		data->err = err_to_att(read);
		return BT_GATT_ITER_STOP;
	}

	if (!data->rsp->len) {
		/* Set len to be the first item found */
		data->rsp->len = read + sizeof(*data->item);
	} else if (data->rsp->len != (read + sizeof(*data->item))) {
		/* All items should have the same size */
		data->sofar -= sizeof(*data->item);
		return BT_GATT_ITER_STOP;
	}

	data->param.handle_value_list[data->param.count].handle = data->item->handle;
	data->param.handle_value_list[data->param.count].value.len = read;
	data->param.handle_value_list[data->param.count].value.val = &data->buffer[data->sofar];
	data->param.count++;

	data->sofar += read;

	if (data->param.count >= ARRAY_SIZE(data->handle_value_list))
	{
		return BT_GATT_ITER_STOP;
	}

	/* continue only if there are still space for more items */
	return ((data->len - data->sofar) > data->rsp->len) ?
		   BT_GATT_ITER_CONTINUE : BT_GATT_ITER_STOP;
}

static uint8_t att_read_type_rsp(struct bt_att_chan *chan, struct bt_uuid *uuid,
				  uint16_t start_handle, uint16_t end_handle)
{
	struct read_type_data data;
	uint8_t ret = 0;

	(void)memset(&data, 0, sizeof(data));

	data.len = MIN(sizeof(data.buffer), bt_att_mtu(chan));
	data.chan = chan;
	data.uuid = uuid;
	data.rsp = (struct bt_att_read_type_rsp *)data.buffer;
	data.sofar = sizeof(*data.rsp);
	data.param.handle_value_list = data.handle_value_list;

	/* Pre-set error if no attr will be found in handle */
	data.err = BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;

	bt_gatt_foreach_attr(start_handle, end_handle, read_type_cb, &data);

	if (data.err) {
		/* Response here since handle is set */
		send_err_rsp(chan, BT_ATT_OP_READ_TYPE_REQ, start_handle,
				 data.err);
	}
	else
	{
		if (API_SUCCESS != BT_att_read_by_type_rsp(&chan->handle, &data.param))
		{
			ret = BT_ATT_ERR_UNLIKELY;
		}
	}

	return ret;
}

static uint8_t att_read_type_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_type_req *req;
	uint16_t start_handle, end_handle, err_handle;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;
	uint8_t uuid_len = buf->len - sizeof(*req);

	/* Type can only be UUID16 or UUID128 */
	if (uuid_len != 2 && uuid_len != 16) {
		return BT_ATT_ERR_INVALID_PDU;
	}

	req = (struct bt_att_read_type_req *)net_buf_pull_mem(buf, sizeof(*req));

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);
	if (!bt_uuid_create(&u.uuid, req->uuid, uuid_len)) {
		return BT_ATT_ERR_UNLIKELY;
	}

	LOG_DBG("start_handle 0x%04x end_handle 0x%04x type %s", start_handle, end_handle,
		bt_uuid_str(&u.uuid));

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(chan, BT_ATT_OP_READ_TYPE_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}
 
	/* Reading Database Hash is special as it may be used to make client change aware
	* (Core Specification 5.4 Vol 3. Part G. 2.5.2.1 Robust Caching).
	*
	* GATT client shall always use GATT Read Using Characteristic UUID sub-procedure for
	* reading Database Hash
	* (Core Specification 5.4 Vol 3. Part G. 7.3 Databse Hash)
	*/
	if (bt_uuid_cmp(&u.uuid, BT_UUID_GATT_DB_HASH) != 0) {
		if (!bt_gatt_change_aware(chan->att->conn, true)) {
			if (!atomic_test_and_set_bit(chan->flags, ATT_OUT_OF_SYNC_SENT)) {
				return BT_ATT_ERR_DB_OUT_OF_SYNC;
			} else {
				return 0;
			}
		}
	}

	return att_read_type_rsp(chan, &u.uuid, start_handle, end_handle);
}

struct read_data {
	struct bt_att_chan *chan;
	uint16_t offset;
	uint8_t buffer[BT_ATT_MTU];
	size_t len;
	size_t sofar;
	ATT_READ_RSP_PARAM rspParam;
	uint8_t err;
};

static uint8_t read_cb(const struct bt_gatt_attr *attr, uint16_t handle,
		       void *user_data)
{
	struct read_data *data = (struct read_data *)user_data;
	struct bt_att_chan *chan = data->chan;
	struct bt_conn *conn = chan->chan.chan.conn;
	int ret;

	LOG_DBG("handle 0x%04x", handle);

	data->rspParam.val = &data->buffer[data->sofar];
	/*
	 * If any attribute is founded in handle range it means that error
	 * should be changed from pre-set: invalid handle error to no error.
	 */
	data->err = 0x00;

	/* Check attribute permissions */
	data->err = bt_gatt_check_perm(conn, attr, BT_GATT_PERM_READ_MASK);
	if (data->err) {
		return BT_GATT_ITER_STOP;
	}

	/* Check the attribute authorization logic */
	if (!attr_read_authorize(conn, attr)) {
		data->err = BT_ATT_ERR_AUTHORIZATION;
		return BT_GATT_ITER_STOP;
	}

	/* Read attribute value and store in the buffer */
#if 0
	ret = att_chan_read(chan, attr, data->buf, data->offset, NULL, NULL);
#else
	ret = attr->read(conn, attr, &data->buffer[data->sofar],
			  data->len - data->sofar, data->offset);
#endif
	if (ret < 0) {
		data->err = err_to_att(ret);
		return BT_GATT_ITER_STOP;
	}

	data->sofar += ret;
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_read_rsp(struct bt_att_chan *chan, uint8_t op, uint8_t rsp,
			 uint16_t handle, uint16_t offset)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct read_data data;
	uint8_t ret = 0;

	if (!bt_gatt_change_aware(conn, true)) {
		if (!atomic_test_and_set_bit(chan->flags, ATT_OUT_OF_SYNC_SENT)) {
			return BT_ATT_ERR_DB_OUT_OF_SYNC;
		} else {
			return 0;
		}
	}

	if (!handle) {
		return BT_ATT_ERR_INVALID_HANDLE;
	}

	(void)memset(&data, 0, sizeof(data));

	data.len = MIN(bt_att_mtu(chan), sizeof(data.buffer));
	data.offset = offset;
	data.chan = chan;

	/* Pre-set error if no attr will be found in handle */
	data.err = BT_ATT_ERR_INVALID_HANDLE;

	bt_gatt_foreach_attr(handle, handle, read_cb, &data);

	/* In case of error discard data and respond with an error */
	if (data.err) {

		/* Respond here since handle is set */
		send_err_rsp(chan, op, handle, data.err);
	}
	else
	{
		data.rspParam.len = data.sofar;
		if (API_SUCCESS != BT_att_send_pdu(&chan->handle, rsp, &data.rspParam))
		{
			ret = BT_ATT_ERR_UNLIKELY;
		}
	}
	return ret;
}

static uint8_t att_read_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_req *req;
	uint16_t handle;

	req = (struct bt_att_read_req *)buf->data;

	handle = sys_le16_to_cpu(req->handle);

	LOG_DBG("handle 0x%04x", handle);

	return att_read_rsp(chan, BT_ATT_OP_READ_REQ, BT_ATT_OP_READ_RSP,
				handle, 0);
}

static uint8_t att_read_blob_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_blob_req *req;
	uint16_t handle, offset;

	req = (struct bt_att_read_blob_req *)buf->data;

	handle = sys_le16_to_cpu(req->handle);
	offset = sys_le16_to_cpu(req->offset);

	LOG_DBG("handle 0x%04x offset %u", handle, offset);

	return att_read_rsp(chan, BT_ATT_OP_READ_BLOB_REQ,
				BT_ATT_OP_READ_BLOB_RSP, handle, offset);
}

#if (defined(CONFIG_BT_GATT_READ_MULTIPLE) && ((CONFIG_BT_GATT_READ_MULTIPLE) > 0U))
static uint8_t att_read_mult_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct read_data data;
	ATT_READ_MULTIPLE_RSP_PARAM	rsp_param;
	ATT_VALUE					  value[BT_ATT_MAX_MULTIPLE_READ_COUNT];
	uint16_t handle;
	uint8_t ret = 0;
	size_t last_len = 0;

	if (!bt_gatt_change_aware(conn, true)) {
		if (!atomic_test_and_set_bit(chan->flags, ATT_OUT_OF_SYNC_SENT)) {
			return BT_ATT_ERR_DB_OUT_OF_SYNC;
		} else {
			return 0;
		}
	}

	(void)memset(&data, 0, sizeof(data));
	data.len = MIN(bt_att_mtu(chan), sizeof(data.buffer));
	data.chan = chan;

	rsp_param.actual_count = 0;
	rsp_param.count = 0;
	rsp_param.value = value;

	while (buf->len >= sizeof(uint16_t)) {
		handle = net_buf_pull_le16(buf);

		LOG_DBG("handle 0x%04x ", handle);

		/* An Error Response shall be sent by the server in response to
		 * the Read Multiple Request [....] if a read operation is not
		 * permitted on any of the Characteristic Values.
		 *
		 * If handle is not valid then return invalid handle error.
		 * If handle is found error will be cleared by read_cb.
		 */
		data.err = BT_ATT_ERR_INVALID_HANDLE;

		bt_gatt_foreach_attr(handle, handle, read_cb, &data);

		/* Stop reading in case of error */
		if (data.err) {
			/* Respond here since handle is set */
			send_err_rsp(chan, BT_ATT_OP_READ_MULT_REQ, handle,
					 data.err);
			return 0;
		}
		data.rspParam.len = data.sofar - last_len;
		last_len = data.sofar;
		value[rsp_param.count] = data.rspParam;
		rsp_param.count++;

		if (rsp_param.count >= BT_ATT_MAX_MULTIPLE_READ_COUNT)
		{
			break;
		}
	}

	if (API_SUCCESS != BT_att_read_multiple_rsp(&chan->handle, &rsp_param))
	{
		ret = BT_ATT_ERR_UNLIKELY;
	}

	return ret;
}
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */

#if (defined(CONFIG_BT_GATT_READ_MULT_VAR_LEN) && (CONFIG_BT_GATT_READ_MULT_VAR_LEN > 0))
static uint8_t read_vl_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	struct read_data *data = user_data;
	struct bt_att_chan *chan = data->chan;
	struct bt_conn *conn = chan->chan.chan.conn;
	struct bt_att_read_mult_vl_rsp *rsp;
	int read;

	LOG_DBG("handle 0x%04x", handle);

	data->rspParam.val = &data->buffer[data->sofar + sizeof(*rsp)];

	/*
	 * If any attribute is founded in handle range it means that error
	 * should be changed from pre-set: invalid handle error to no error.
	 */
	data->err = 0x00;

	/* Check attribute permissions */
	data->err = bt_gatt_check_perm(conn, attr, BT_GATT_PERM_READ_MASK);
	if (data->err) {
		return BT_GATT_ITER_STOP;
	}

	/* Check the attribute authorization logic */
	if (!attr_read_authorize(conn, attr)) {
		data->err = BT_ATT_ERR_AUTHORIZATION;
		return BT_GATT_ITER_STOP;
	}
	/* The Length Value Tuple List may be truncated within the first two
	 * octets of a tuple due to the size limits of the current ATT_MTU.
	 */
	if (bt_att_mtu(chan) - data->sofar < 2) {
		return BT_GATT_ITER_STOP;
	}

	read = attr->read(conn, attr, &data->buffer[data->sofar + sizeof(*rsp)], data->len - data->sofar - sizeof(*rsp), 0);
	if (read < 0) {
		data->err = err_to_att(read);
		return BT_GATT_ITER_STOP;
	}

	data->sofar += sizeof(*rsp) + read;

		data->rspParam.len = read;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_read_mult_vl_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct read_data data;
	uint16_t handle;

	ATT_READ_MULTIPLE_VARIABLE_LENGTH_RSP_PARAM rsp_param;
	ATT_VALUE value[BT_ATT_MAX_MULTIPLE_VARIABLE_LENGTH_READ_COUNT];
	uint8_t ret = 0;

	if (!bt_gatt_change_aware(conn, true)) {
		if (!atomic_test_and_set_bit(chan->flags, ATT_OUT_OF_SYNC_SENT)) {
			return BT_ATT_ERR_DB_OUT_OF_SYNC;
		} else {
			return 0;
		}
	}

	(void)memset(&data, 0, sizeof(data));

	data.len = MIN(bt_att_mtu(chan), sizeof(data.buffer));
	data.chan = chan;

	rsp_param.actual_count = 0;
	rsp_param.count = 0;
	rsp_param.value = value;

	while (buf->len >= sizeof(uint16_t)) {
		handle = net_buf_pull_le16(buf);

		LOG_DBG("handle 0x%04x ", handle);

		/* If handle is not valid then return invalid handle error.
		 * If handle is found error will be cleared by read_cb.
		 */
		data.err = BT_ATT_ERR_INVALID_HANDLE;

		bt_gatt_foreach_attr(handle, handle, read_vl_cb, &data);

		/* Stop reading in case of error */
		if (data.err) {
			/* Respond here since handle is set */
			send_err_rsp(chan, BT_ATT_OP_READ_MULT_VL_REQ, handle,
					 data.err);
			return 0;
		}

		value[rsp_param.count] = data.rspParam;
		rsp_param.count++;

		if (rsp_param.count >= BT_ATT_MAX_MULTIPLE_VARIABLE_LENGTH_READ_COUNT)
		{
			break;
		}
	}

	if (API_SUCCESS != BT_att_send_read_multiple_variable_length_rsp(&chan->handle, &rsp_param))
	{
		ret = BT_ATT_ERR_UNLIKELY;
	}

	return ret;
}
#endif /* CONFIG_BT_GATT_READ_MULT_VAR_LEN */

struct read_group_data {
	struct bt_att_chan *chan;
	struct bt_uuid *uuid;
	uint8_t buffer[BT_ATT_MTU];
	size_t len;
	size_t sofar;
	struct bt_att_read_group_rsp *rsp;
	struct bt_att_group_data *group;
	ATT_GROUP_ATTR_DATA_LIST		   list;
	ATT_GROUP_ATTR_DATA_ELEMENT		group_list[BT_ATT_MAX_GROUP_COUNT];
};

#if 0
static bool attr_read_group_cb(struct net_buf *frag, ssize_t read,
				   void *user_data)
{
	struct read_group_data *data = user_data;

	if (!data->rsp->len) {
		/* Set len to be the first group found */
		data->rsp->len = read + sizeof(*data->group);
	} else if (data->rsp->len != read + sizeof(*data->group)) {
		/* All groups entries should have the same size */
		data->buf->len -= sizeof(*data->group);
		data->group = NULL;
		return false;
	}

	return true;
}
#endif

static uint8_t read_group_cb(const struct bt_gatt_attr *attr, uint16_t handle,
				 void *user_data)
{
	struct read_group_data *data = (struct read_group_data *)user_data;
	struct bt_att_chan *chan = data->chan;
	int read;

	/* Update group end_handle if attribute is not a service */
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) &&
		bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
		if (data->group &&
			handle > sys_le16_to_cpu(data->group->end_handle)) {
			data->group->end_handle = sys_cpu_to_le16(handle);
			data->list.list[data->list.count-1].range.end_handle = sys_cpu_to_le16(handle);
		}
		return BT_GATT_ITER_CONTINUE;
	}

	/* If Group Type don't match skip */
	if (bt_uuid_cmp(attr->uuid, data->uuid)) {
		data->group = NULL;
		return BT_GATT_ITER_CONTINUE;
	}

	LOG_DBG("handle 0x%04x", handle);

	/* Stop if there is no space left */
	if (data->rsp->len &&
	   (data->len - data->sofar) < (data->rsp->len))
	{
		return BT_GATT_ITER_STOP;
	}

	/* Fast forward to next group position */
	data->group = (struct bt_att_group_data *)&data->buffer[data->sofar];

	/* Initialize group handle range */
	data->group->start_handle = sys_cpu_to_le16(handle);
	data->group->end_handle = sys_cpu_to_le16(handle);
	data->sofar += sizeof(*data->group);

	/* Read attribute value and store in the buffer */
#if 0
	read = att_chan_read(chan, attr, data->buf, 0, attr_read_group_cb,
				 data);
#else
	read = attr->read(chan->chan.chan.conn, attr, &data->buffer[data->sofar],
			  data->len - data->sofar, 0);
#endif
	if (read < 0) {
		/* TODO: Handle read errors */
		return BT_GATT_ITER_STOP;
	}

	if (!data->rsp->len) {
		/* Set len to be the first group found */
		data->rsp->len = read + sizeof(*data->group);
		data->list.length = read;
	} else if (data->rsp->len != (read + sizeof(*data->group))) {
		/* All groups entries should have the same size */
		data->sofar -= sizeof(*data->group);
		return BT_GATT_ITER_STOP;
	}

	data->list.count++;
	data->list.list[data->list.count-1].attr_value = &data->buffer[data->sofar];
	data->list.list[data->list.count-1].range.start_handle = sys_cpu_to_le16(handle);
	data->list.list[data->list.count-1].range.end_handle = sys_cpu_to_le16(handle);

	data->sofar += read;

	/* continue only if there are still space for more items */
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_read_group_rsp(struct bt_att_chan *chan, struct bt_uuid *uuid,
				   uint16_t start_handle, uint16_t end_handle)
{
	struct read_group_data data;
	uint16_t ret = 0;

	(void)memset(&data, 0, sizeof(data));

	data.len = MIN(sizeof(data.buffer), bt_att_mtu(chan));
	data.chan = chan;
	data.uuid = uuid;
	data.rsp = (struct bt_att_read_group_rsp *)data.buffer;
	data.group = NULL;
	data.sofar = sizeof(*data.rsp);
	data.list.list = data.group_list;

	bt_gatt_foreach_attr(start_handle, end_handle, read_group_cb, &data);

	if (!data.list.count) {
		/* Respond here since handle is set */
		send_err_rsp(chan, BT_ATT_OP_READ_GROUP_REQ, start_handle,
				 BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return 0;
	}
	else
	{
		if (API_SUCCESS != BT_att_read_by_group_rsp(&chan->handle, &data.list))
		{
			ret = BT_ATT_ERR_UNLIKELY;
		}
	}

	return ret;
}

static uint8_t att_read_group_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_group_req *req;
	uint16_t start_handle, end_handle, err_handle;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;
	uint8_t uuid_len = buf->len - sizeof(*req);

	/* Type can only be UUID16 or UUID128 */
	if (uuid_len != 2 && uuid_len != 16) {
		return BT_ATT_ERR_INVALID_PDU;
	}

	req = (struct bt_att_read_group_req *)net_buf_pull_mem(buf, sizeof(*req));

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);

	if (!bt_uuid_create(&u.uuid, req->uuid, uuid_len)) {
		return BT_ATT_ERR_UNLIKELY;
	}

	LOG_DBG("start_handle 0x%04x end_handle 0x%04x type %s",
		   start_handle, end_handle, bt_uuid_str(&u.uuid));

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(chan, BT_ATT_OP_READ_GROUP_REQ, err_handle,
				 BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	/* Core v4.2, Vol 3, sec 2.5.3 Attribute Grouping:
	 * Not all of the grouping attributes can be used in the ATT
	 * Read By Group Type Request. The "Primary Service" and "Secondary
	 * Service" grouping types may be used in the Read By Group Type
	 * Request. The "Characteristic" grouping type shall not be used in
	 * the ATT Read By Group Type Request.
	 */
	if (bt_uuid_cmp(&u.uuid, BT_UUID_GATT_PRIMARY) &&
		bt_uuid_cmp(&u.uuid, BT_UUID_GATT_SECONDARY)) {
		send_err_rsp(chan, BT_ATT_OP_READ_GROUP_REQ, start_handle,
				 BT_ATT_ERR_UNSUPPORTED_GROUP_TYPE);
		return 0;
	}

	return att_read_group_rsp(chan, &u.uuid, start_handle, end_handle);
}

struct write_data {
	struct bt_conn *conn;
	uint8_t req;
	const void *value;
	uint16_t len;
	uint16_t offset;
	uint8_t err;
};

static bool attr_write_authorize(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr)
{
	if (!IS_ENABLED(CONFIG_BT_GATT_AUTHORIZATION_CUSTOM)) {
		return true;
	}
#if (defined (CONFIG_BT_GATT_AUTHORIZATION_CUSTOM) && (CONFIG_BT_GATT_AUTHORIZATION_CUSTOM))
	if (!authorization_cb || !authorization_cb->write_authorize) {
		return true;
	}

	return authorization_cb->write_authorize(conn, attr);
#endif
}

static uint8_t write_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			void *user_data)
{
	struct write_data *data = (struct write_data *)user_data;
	int write;
	uint8_t flags = 0U;

	LOG_DBG("handle 0x%04x offset %u", handle, data->offset);

	/* Check attribute permissions */
	data->err = bt_gatt_check_perm(data->conn, attr,
				       BT_GATT_PERM_WRITE_MASK);
	if (data->err) {
		return BT_GATT_ITER_STOP;
	}

	/* Check the attribute authorization logic */
	if (!attr_write_authorize(data->conn, attr)) {
		data->err = BT_ATT_ERR_AUTHORIZATION;
		return BT_GATT_ITER_STOP;
	}

	/* Set command flag if not a request */
	if (!data->req) {
		flags |= BT_GATT_WRITE_FLAG_CMD;
	} else if (data->req == BT_ATT_OP_EXEC_WRITE_REQ) {
		flags |= BT_GATT_WRITE_FLAG_EXECUTE;
	}

	/* Write attribute value */
	write = attr->write(data->conn, attr, data->value, data->len,
			    data->offset, flags);
	if (write < 0 || write != data->len) {
		data->err = err_to_att(write);
		return BT_GATT_ITER_STOP;
	}

	data->err = 0U;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_write_rsp(struct bt_att_chan *chan, uint8_t req, uint8_t rsp,
			  uint16_t handle, uint16_t offset, const void *value,
			  uint16_t len)
{
	struct write_data data;
	uint8_t ret = 0;

	if (!bt_gatt_change_aware(chan->att->conn, req ? true : false)) {
		if (!atomic_test_and_set_bit(chan->flags, ATT_OUT_OF_SYNC_SENT)) {
			return BT_ATT_ERR_DB_OUT_OF_SYNC;
		} else {
			return 0;
		}
	}

	if (!handle) {
		return BT_ATT_ERR_INVALID_HANDLE;
	}

	(void)memset(&data, 0, sizeof(data));

	data.conn = chan->att->conn;
	data.req = req;
	data.offset = offset;
	data.value = value;
	data.len = len;
	data.err = BT_ATT_ERR_INVALID_HANDLE;

	bt_gatt_foreach_attr(handle, handle, write_cb, &data);

	if (data.err) {
		/* In case of error discard data and respond with an error */
		if (rsp) {
			/* Respond here since handle is set */
			send_err_rsp(chan, req, handle, data.err);
		}
		return req == BT_ATT_OP_EXEC_WRITE_REQ ? data.err : 0;
	}
	else
	{
		if (rsp) {
			API_RESULT retVal = BT_att_write_rsp(&chan->handle);
			if (API_SUCCESS != retVal)
			{
				ret = BT_ATT_ERR_UNLIKELY;
			}
		}
	}

	return ret;
}

static uint8_t att_write_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	uint16_t handle;

	handle = net_buf_pull_le16(buf);

	LOG_DBG("handle 0x%04x", handle);

	return att_write_rsp(chan, BT_ATT_OP_WRITE_REQ, BT_ATT_OP_WRITE_RSP,
			     handle, 0, buf->data, buf->len);
}

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
struct prep_data {
	struct bt_conn *conn;
	struct net_buf *buf;
	const void *value;
	uint16_t len;
	uint16_t offset;
	uint8_t err;
};

static uint8_t prep_write_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			     void *user_data)
{
	struct prep_data *data = user_data;
	struct bt_attr_data *attr_data;
	int write;

	LOG_DBG("handle 0x%04x offset %u", handle, data->offset);

	/* Check attribute permissions */
	data->err = bt_gatt_check_perm(data->conn, attr,
				       BT_GATT_PERM_WRITE_MASK);
	if (data->err) {
		return BT_GATT_ITER_STOP;
	}

	/* Check the attribute authorization logic */
	if (!attr_write_authorize(data->conn, attr)) {
		data->err = BT_ATT_ERR_AUTHORIZATION;
		return BT_GATT_ITER_STOP;
	}

	/* Check if attribute requires handler to accept the data */
	if (!(attr->perm & BT_GATT_PERM_PREPARE_WRITE)) {
		goto append;
	}

	/* Write attribute value to check if device is authorized */
	write = attr->write(data->conn, attr, data->value, data->len,
			    data->offset, BT_GATT_WRITE_FLAG_PREPARE);
	if (write != 0) {
		data->err = err_to_att(write);
		return BT_GATT_ITER_STOP;
	}

append:
	/* Copy data into the outstanding queue */
	data->buf = net_buf_alloc(&prep_pool, K_NO_WAIT);
	if (!data->buf) {
		data->err = BT_ATT_ERR_PREPARE_QUEUE_FULL;
		return BT_GATT_ITER_STOP;
	}

	attr_data = net_buf_user_data(data->buf);
	attr_data->handle = handle;
	attr_data->offset = data->offset;

	net_buf_add_mem(data->buf, data->value, data->len);

	data->err = 0U;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_prep_write_rsp(struct bt_att_chan *chan, uint16_t handle,
				   uint16_t offset, const void *value, uint8_t len)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct prep_data data;
	ATT_PREPARE_WRITE_RSP_PARAM	prepare_write_rsp;
	API_RESULT retval;
	uint8_t ret = 0;

	if (!bt_gatt_change_aware(conn, true)) {
		if (!atomic_test_and_set_bit(chan->flags, ATT_OUT_OF_SYNC_SENT)) {
			return BT_ATT_ERR_DB_OUT_OF_SYNC;
		} else {
			return 0;
		}
	}

	if (!handle) {
		return BT_ATT_ERR_INVALID_HANDLE;
	}

	(void)memset(&data, 0, sizeof(data));

	data.conn = conn;
	data.offset = offset;
	data.value = value;
	data.len = len;
	data.err = BT_ATT_ERR_INVALID_HANDLE;

	bt_gatt_foreach_attr(handle, handle, prep_write_cb, &data);

	if (data.err) {
		/* Respond here since handle is set */
		send_err_rsp(chan, BT_ATT_OP_PREPARE_WRITE_REQ, handle,
				 data.err);
		return 0;
	}

	LOG_DBG("buf %p handle 0x%04x offset %u", data.buf, handle, offset);

	/* Store buffer in the outstanding queue */
	net_buf_slist_put(&chan->att->prep_queue, data.buf);

	/* Generate response */
	prepare_write_rsp.offset = offset;
	prepare_write_rsp.handle_value.handle = handle;
	prepare_write_rsp.handle_value.value.val = (uint8_t*)value;
	prepare_write_rsp.handle_value.value.len = len;

	retval = BT_att_send_prepare_write_rsp
				(
					&chan->handle,
					&prepare_write_rsp
				);
	if (API_SUCCESS != retval)
	{
		ret = BT_ATT_ERR_UNLIKELY;
	}
	return ret;
}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */

static uint8_t att_prepare_write_req(struct bt_att_chan *chan, struct net_buf *buf)
{
#if CONFIG_BT_ATT_PREPARE_COUNT == 0
	return BT_ATT_ERR_NOT_SUPPORTED;
#else
	struct bt_att_prepare_write_req *req;
	uint16_t handle, offset;

	req = net_buf_pull_mem(buf, sizeof(*req));

	handle = sys_le16_to_cpu(req->handle);
	offset = sys_le16_to_cpu(req->offset);

	LOG_DBG("handle 0x%04x offset %u", handle, offset);

	return att_prep_write_rsp(chan, handle, offset, buf->data, buf->len);
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */
}

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
static uint8_t exec_write_reassemble(uint16_t handle, uint16_t offset,
				     sys_slist_t *list,
				     struct net_buf_simple *buf)
{
	struct net_buf *entry, *next;
	sys_snode_t *prev;

	prev = NULL;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, entry, next, node) {
		struct bt_attr_data *tmp_data = net_buf_user_data(entry);

		LOG_DBG("entry %p handle 0x%04x, offset %u", entry, tmp_data->handle,
			tmp_data->offset);

		if (tmp_data->handle == handle) {
			if (tmp_data->offset == 0) {
				/* Multiple writes to the same handle can occur
				 * in a prepare write queue. If the offset is 0,
				 * that should mean that it's a new write to the
				 * same handle, and we break to process the
				 * first write.
				 */

				LOG_DBG("tmp_data->offset == 0");
				break;
			}

			if (tmp_data->offset != buf->len + offset) {
				/* We require that the offset is increasing
				 * properly to avoid badly reassembled buffers
				 */

				LOG_DBG("Bad offset %u (%u, %u)", tmp_data->offset, buf->len,
					offset);

				return BT_ATT_ERR_INVALID_OFFSET;
			}

			if (buf->len + entry->len > buf->size) {
				return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}

			net_buf_simple_add_mem(buf, entry->data, entry->len);
			sys_slist_remove(list, prev, &entry->node);
			net_buf_unref(entry);
		} else {
			prev = &entry->node;
		}
	}

	return BT_ATT_ERR_SUCCESS;
}

static uint8_t att_exec_write_rsp(struct bt_att_chan *chan, uint8_t flags)
{
#if 0
	struct bt_conn *conn = chan->chan.chan.conn;
#endif
	struct net_buf *buf;
	API_RESULT retval;
	uint8_t err = 0U;
	uint8_t ret = 0;

	/* The following code will iterate on all prepare writes in the
	 * prep_queue, and reassemble those that share the same handle.
	 * Once a handle has been reassembled, it is sent to the upper layers,
	 * and the next handle is processed
	 */
	while (!sys_slist_is_empty(&chan->att->prep_queue)) {
		struct bt_attr_data *data;
		uint16_t handle;

		NET_BUF_SIMPLE_DEFINE_STATIC(reassembled_data,
					     MIN(BT_ATT_MAX_ATTRIBUTE_LEN,
						 CONFIG_BT_ATT_PREPARE_COUNT * BT_ATT_BUF_SIZE));

		buf = net_buf_slist_get(&chan->att->prep_queue);
		data = net_buf_user_data(buf);
		handle = data->handle;

		LOG_DBG("buf %p handle 0x%04x offset %u", buf, handle, data->offset);

		net_buf_simple_reset(&reassembled_data);
		net_buf_simple_add_mem(&reassembled_data, buf->data, buf->len);

		err = exec_write_reassemble(handle, data->offset,
					    &chan->att->prep_queue,
					    &reassembled_data);
		if (err != BT_ATT_ERR_SUCCESS) {
			send_err_rsp(chan, BT_ATT_OP_EXEC_WRITE_REQ,
				     handle, err);
			return 0;
		}

		/* Just discard the data if an error was set */
		if (!err && flags == BT_ATT_FLAG_EXEC) {
			err = att_write_rsp(chan, BT_ATT_OP_EXEC_WRITE_REQ, 0,
					    handle, data->offset,
					    reassembled_data.data,
					    reassembled_data.len);
			if (err) {
				/* Respond here since handle is set */
				send_err_rsp(chan, BT_ATT_OP_EXEC_WRITE_REQ,
					     data->handle, err);
			}
		}

		net_buf_unref(buf);
	}

	if (err) {
		return 0;
	}

	/* Generate response */
	retval = BT_att_send_execute_write_rsp
			(
				&chan->handle
			);
	if (API_SUCCESS != retval)
	{
		ret = BT_ATT_ERR_UNLIKELY;
	}
	return ret;
}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */


static uint8_t att_exec_write_req(struct bt_att_chan *chan, struct net_buf *buf)
{
#if CONFIG_BT_ATT_PREPARE_COUNT == 0
	return BT_ATT_ERR_NOT_SUPPORTED;
#else
	struct bt_att_exec_write_req *req;

	req = (void *)buf->data;

	LOG_DBG("flags 0x%02x", req->flags);

	return att_exec_write_rsp(chan, req->flags);
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */
}

static uint8_t att_write_cmd(struct bt_att_chan *chan, struct net_buf *buf)
{
	uint16_t handle;

	handle = net_buf_pull_le16(buf);

	LOG_DBG("handle 0x%04x", handle);

	return att_write_rsp(chan, 0, 0, handle, 0, buf->data, buf->len);
}

#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
static uint8_t att_signed_write_cmd(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct bt_att_signed_write_cmd *req;
	uint16_t handle;
	int err;

	/* The Signed Write Without Response sub-procedure shall only be supported
	 * on the LE Fixed Channel Unenhanced ATT bearer.
	 */
	if (bt_att_is_enhanced(chan)) {
		/* No response for this command */
		return 0;
	}

	req = (struct bt_att_signed_write_cmd *)buf->data;

	handle = sys_le16_to_cpu(req->handle);

	LOG_DBG("handle 0x%04x", handle);

	/* Verifying data requires full buffer including attribute header */
	net_buf_push(buf, sizeof(struct bt_att_hdr));
	err = bt_smp_sign_verify(conn, buf);
	if (err) {
		LOG_ERR("Error verifying data");
		/* No response for this command */
		return 0;
	}

	net_buf_pull(buf, sizeof(struct bt_att_hdr));
	net_buf_pull(buf, sizeof(*req));

	return att_write_rsp(chan, 0, 0, handle, 0, buf->data,
				 buf->len - sizeof(struct bt_att_signature));
}
#endif /* CONFIG_BT_SIGNING */

#if (defined(CONFIG_BT_GATT_CLIENT) && ((CONFIG_BT_GATT_CLIENT) > 0U))
#if (defined(CONFIG_BT_ATT_RETRY_ON_SEC_ERR) && (CONFIG_BT_ATT_RETRY_ON_SEC_ERR > 0))
static int att_change_security(struct bt_conn *conn, uint8_t err)
{
	bt_security_t sec;

	switch (err) {
	case BT_ATT_ERR_INSUFFICIENT_ENCRYPTION:
		if (conn->sec_level >= BT_SECURITY_L2)
			return -EALREADY;
		sec = BT_SECURITY_L2;
		break;
	case BT_ATT_ERR_AUTHENTICATION:
		if (conn->sec_level < BT_SECURITY_L2) {
			/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part C]
			 * page 375:
			 *
			 * If an LTK is not available, the service request
			 * shall be rejected with the error code 'Insufficient
			 * Authentication'.
			 * Note: When the link is not encrypted, the error code
			 * "Insufficient Authentication" does not indicate that
			 * MITM protection is required.
			 */
			sec = BT_SECURITY_L2;
		} else if (conn->sec_level < BT_SECURITY_L3) {
			/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part C]
			 * page 375:
			 *
			 * If an authenticated pairing is required but only an
			 * unauthenticated pairing has occurred and the link is
			 * currently encrypted, the service request shall be
			 * rejected with the error code 'Insufficient
			 * Authentication'.
			 * Note: When unauthenticated pairing has occurred and
			 * the link is currently encrypted, the error code
			 * 'Insufficient Authentication' indicates that MITM
			 * protection is required.
			 */
			sec = BT_SECURITY_L3;
		} else if (conn->sec_level < BT_SECURITY_L4) {
			/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part C]
			 * page 375:
			 *
			 * If LE Secure Connections authenticated pairing is
			 * required but LE legacy pairing has occurred and the
			 * link is currently encrypted, the service request
			 * shall be rejected with the error code ''Insufficient
			 * Authentication'.
			 */
			sec = BT_SECURITY_L4;
		} else {
			return -EALREADY;
		}
		break;
	default:
		return -EINVAL;
	}

	return bt_conn_set_security(conn, sec);
}
#endif /* CONFIG_BT_ATT_RETRY_ON_SEC_ERR */

static uint8_t att_error_rsp(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_error_rsp *rsp;
	uint8_t err;

	rsp = (struct bt_att_error_rsp *)buf->data;

	LOG_DBG("request 0x%02x handle 0x%04x error 0x%02x", rsp->request,
		   sys_le16_to_cpu(rsp->handle), rsp->error);

	/* Don't retry if there is no req pending or it has been cancelled.
	 *
	 * BLUETOOTH SPECIFICATION Version 5.2 [Vol 3, Part F]
	 * page 1423:
	 *
	 * If an error code is received in the ATT_ERROR_RSP PDU that is not
	 * understood by the client, for example an error code that was reserved
	 * for future use that is now being used in a future version of the
	 * specification, then the ATT_ERROR_RSP PDU shall still be considered to
	 * state that the given request cannot be performed for an unknown reason.
	 */
	if (!chan->req || chan->req == &cancel || !rsp->error) {
		err = BT_ATT_ERR_UNLIKELY;
		goto done;
	}

	err = rsp->error;
#if (defined(CONFIG_BT_ATT_RETRY_ON_SEC_ERR) && (CONFIG_BT_ATT_RETRY_ON_SEC_ERR > 0))
	int ret;

	/* Check if error can be handled by elevating security. */
	ret = att_change_security(chan->chan.chan.conn, err);
	if (ret == 0 || ret == -EBUSY) {
		/* ATT timeout work is normally cancelled in att_handle_rsp.
		 * However retrying is special case, so the timeout shall
		 * be cancelled here.
		 */
		k_work_cancel_delayable(&chan->timeout_work);

		chan->req->retrying = true;
		return 0;
	}
#endif /* CONFIG_BT_ATT_RETRY_ON_SEC_ERR */

done:
	return att_handle_rsp(chan, NULL, 0, err);
}

static uint8_t att_handle_find_info_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_find_type_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_read_type_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_read_rsp(struct bt_att_chan *chan,
				struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_read_blob_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

#if (defined(CONFIG_BT_GATT_READ_MULTIPLE) && ((CONFIG_BT_GATT_READ_MULTIPLE) > 0U))
static uint8_t att_handle_read_mult_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

#endif /* CONFIG_BT_GATT_READ_MULTIPLE */

#if (defined(CONFIG_BT_GATT_READ_MULT_VAR_LEN) && (CONFIG_BT_GATT_READ_MULT_VAR_LEN > 0))
static uint8_t att_handle_read_mult_vl_rsp(struct bt_att_chan *chan,
					struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}
#endif /* CONFIG_BT_GATT_READ_MULT_VAR_LEN */

static uint8_t att_handle_read_group_rsp(struct bt_att_chan *chan,
				      struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_write_rsp(struct bt_att_chan *chan,
				 struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_prepare_write_rsp(struct bt_att_chan *chan,
					 struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_exec_write_rsp(struct bt_att_chan *chan,
				      struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_notify(struct bt_att_chan *chan, struct net_buf *buf)
{
	uint16_t handle;

	handle = net_buf_pull_le16(buf);

	LOG_DBG("chan %p handle 0x%04x", chan, handle);

	bt_gatt_notification(chan->att->conn, handle, buf->data, buf->len);

	return 0;
}

static uint8_t att_indicate(struct bt_att_chan *chan, struct net_buf *buf)
{
	uint16_t handle;
	API_RESULT retval;

	handle = net_buf_pull_le16(buf);

	LOG_DBG("chan %p handle 0x%04x", chan, handle);

	bt_gatt_notification(chan->att->conn, handle, buf->data, buf->len);

	retval = BT_att_send_hndl_val_cnf(&chan->handle);
	assert(API_SUCCESS == retval);
	(void)retval;

	return 0;
}

static uint8_t att_notify_mult(struct bt_att_chan *chan, struct net_buf *buf)
{
	LOG_DBG("chan %p", chan);

	bt_gatt_mult_notification(chan->att->conn, buf->data, buf->len);

	return 0;
}
#endif /* CONFIG_BT_GATT_CLIENT */

static uint8_t att_confirm(struct bt_att_chan *chan, struct net_buf *buf)
{
	LOG_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_notify_cfm(struct bt_att_chan *chan, struct net_buf *buf)
{
	uint16_t handle;

	handle = net_buf_pull_le16(buf);

	LOG_DBG("chan %p handle 0x%04x", chan, handle);

	(void)handle;

	return 0;
}

static const struct att_handler {
	uint8_t	   op;
	uint8_t	   expect_len;
	att_type_t type;
	uint8_t	   (*func)(struct bt_att_chan *chan, struct net_buf *buf);
} handlers[] = {
	{ BT_ATT_OP_NOTIFY + 1,
		0,
		ATT_UNKNOWN,
		att_notify_cfm },
	{ BT_ATT_OP_MTU_REQ,
		sizeof(struct bt_att_exchange_mtu_req),
		ATT_REQUEST,
		att_mtu_req },
	{ BT_ATT_OP_FIND_INFO_REQ,
		sizeof(struct bt_att_find_info_req),
		ATT_REQUEST,
		att_find_info_req },
	{ BT_ATT_OP_FIND_TYPE_REQ,
		sizeof(struct bt_att_find_type_req),
		ATT_REQUEST,
		att_find_type_req },
	{ BT_ATT_OP_READ_TYPE_REQ,
		sizeof(struct bt_att_read_type_req),
		ATT_REQUEST,
		att_read_type_req },
	{ BT_ATT_OP_READ_REQ,
		sizeof(struct bt_att_read_req),
		ATT_REQUEST,
		att_read_req },
	{ BT_ATT_OP_READ_BLOB_REQ,
		sizeof(struct bt_att_read_blob_req),
		ATT_REQUEST,
		att_read_blob_req },
#if (defined(CONFIG_BT_GATT_READ_MULTIPLE) && ((CONFIG_BT_GATT_READ_MULTIPLE) > 0U))
	{ BT_ATT_OP_READ_MULT_REQ,
		BT_ATT_READ_MULT_MIN_LEN_REQ,
		ATT_REQUEST,
		att_read_mult_req },
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */
#if (defined(CONFIG_BT_GATT_READ_MULT_VAR_LEN) && (CONFIG_BT_GATT_READ_MULT_VAR_LEN > 0))
	{ BT_ATT_OP_READ_MULT_VL_REQ,
		BT_ATT_READ_MULT_MIN_LEN_REQ,
		ATT_REQUEST,
		att_read_mult_vl_req },
#endif /* CONFIG_BT_GATT_READ_MULT_VAR_LEN */
	{ BT_ATT_OP_READ_GROUP_REQ,
		sizeof(struct bt_att_read_group_req),
		ATT_REQUEST,
		att_read_group_req },
	{ BT_ATT_OP_WRITE_REQ,
		sizeof(struct bt_att_write_req),
		ATT_REQUEST,
		att_write_req },
	{ BT_ATT_OP_PREPARE_WRITE_REQ,
		sizeof(struct bt_att_prepare_write_req),
		ATT_REQUEST,
		att_prepare_write_req },
	{ BT_ATT_OP_EXEC_WRITE_REQ,
		sizeof(struct bt_att_exec_write_req),
		ATT_REQUEST,
		att_exec_write_req },
	{ BT_ATT_OP_CONFIRM,
		0,
		ATT_CONFIRMATION,
		att_confirm },
	{ BT_ATT_OP_WRITE_CMD,
		sizeof(struct bt_att_write_cmd),
		ATT_COMMAND,
		att_write_cmd },
#if (defined(CONFIG_BT_SIGNING) && ((CONFIG_BT_SIGNING) > 0U))
	{ BT_ATT_OP_SIGNED_WRITE_CMD,
		(sizeof(struct bt_att_write_cmd) +
		 sizeof(struct bt_att_signature)),
		ATT_COMMAND,
		att_signed_write_cmd },
#endif /* CONFIG_BT_SIGNING */
#if (defined(CONFIG_BT_GATT_CLIENT) && ((CONFIG_BT_GATT_CLIENT) > 0U))
	{ BT_ATT_OP_ERROR_RSP,
		sizeof(struct bt_att_error_rsp),
		ATT_RESPONSE,
		att_error_rsp },
	{ BT_ATT_OP_MTU_RSP,
		sizeof(struct bt_att_exchange_mtu_rsp),
		ATT_RESPONSE,
		att_mtu_rsp },
	{ BT_ATT_OP_FIND_INFO_RSP,
		sizeof(struct bt_att_find_info_rsp),
		ATT_RESPONSE,
		att_handle_find_info_rsp },
	{ BT_ATT_OP_FIND_TYPE_RSP,
		sizeof(struct bt_att_handle_group),
		ATT_RESPONSE,
		att_handle_find_type_rsp },
	{ BT_ATT_OP_READ_TYPE_RSP,
		sizeof(struct bt_att_read_type_rsp),
		ATT_RESPONSE,
		att_handle_read_type_rsp },
	{ BT_ATT_OP_READ_RSP,
		0,
		ATT_RESPONSE,
		att_handle_read_rsp },
	{ BT_ATT_OP_READ_BLOB_RSP,
		0,
		ATT_RESPONSE,
		att_handle_read_blob_rsp },
#if (defined(CONFIG_BT_GATT_READ_MULTIPLE) && ((CONFIG_BT_GATT_READ_MULTIPLE) > 0U))
	{ BT_ATT_OP_READ_MULT_RSP,
		0,
		ATT_RESPONSE,
		att_handle_read_mult_rsp },
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */
#if (defined(CONFIG_BT_GATT_READ_MULT_VAR_LEN) && (CONFIG_BT_GATT_READ_MULT_VAR_LEN > 0))
	{ BT_ATT_OP_READ_MULT_VL_RSP,
		sizeof(struct bt_att_read_mult_vl_rsp),
		ATT_RESPONSE,
		att_handle_read_mult_vl_rsp },
#endif /* CONFIG_BT_GATT_READ_MULT_VAR_LEN */
	{ BT_ATT_OP_READ_GROUP_RSP,
		sizeof(struct bt_att_read_group_rsp),
		ATT_RESPONSE,
		att_handle_read_group_rsp },
	{ BT_ATT_OP_WRITE_RSP,
		0,
		ATT_RESPONSE,
		att_handle_write_rsp },
	{ BT_ATT_OP_PREPARE_WRITE_RSP,
		sizeof(struct bt_att_prepare_write_rsp),
		ATT_RESPONSE,
		att_handle_prepare_write_rsp },
	{ BT_ATT_OP_EXEC_WRITE_RSP,
		0,
		ATT_RESPONSE,
		att_handle_exec_write_rsp },
	{ BT_ATT_OP_NOTIFY,
		sizeof(struct bt_att_notify),
		ATT_NOTIFICATION,
		att_notify },
	{ BT_ATT_OP_INDICATE,
		sizeof(struct bt_att_indicate),
		ATT_INDICATION,
		att_indicate },
	{ BT_ATT_OP_NOTIFY_MULT,
		sizeof(struct bt_att_notify_mult),
		ATT_NOTIFICATION,
		att_notify_mult },
#endif /* CONFIG_BT_GATT_CLIENT */
};

static att_type_t att_op_get_type(uint8_t op)
{
	switch (op) {
	case BT_ATT_OP_MTU_REQ:
	case BT_ATT_OP_FIND_INFO_REQ:
	case BT_ATT_OP_FIND_TYPE_REQ:
	case BT_ATT_OP_READ_TYPE_REQ:
	case BT_ATT_OP_READ_REQ:
	case BT_ATT_OP_READ_BLOB_REQ:
	case BT_ATT_OP_READ_MULT_REQ:
	case BT_ATT_OP_READ_MULT_VL_REQ:
	case BT_ATT_OP_READ_GROUP_REQ:
	case BT_ATT_OP_WRITE_REQ:
	case BT_ATT_OP_PREPARE_WRITE_REQ:
	case BT_ATT_OP_EXEC_WRITE_REQ:
		return ATT_REQUEST;
	case BT_ATT_OP_CONFIRM:
		return ATT_CONFIRMATION;
	case BT_ATT_OP_WRITE_CMD:
	case BT_ATT_OP_SIGNED_WRITE_CMD:
		return ATT_COMMAND;
	case BT_ATT_OP_ERROR_RSP:
	case BT_ATT_OP_MTU_RSP:
	case BT_ATT_OP_FIND_INFO_RSP:
	case BT_ATT_OP_FIND_TYPE_RSP:
	case BT_ATT_OP_READ_TYPE_RSP:
	case BT_ATT_OP_READ_RSP:
	case BT_ATT_OP_READ_BLOB_RSP:
	case BT_ATT_OP_READ_MULT_RSP:
	case BT_ATT_OP_READ_MULT_VL_RSP:
	case BT_ATT_OP_READ_GROUP_RSP:
	case BT_ATT_OP_WRITE_RSP:
	case BT_ATT_OP_PREPARE_WRITE_RSP:
	case BT_ATT_OP_EXEC_WRITE_RSP:
		return ATT_RESPONSE;
	case BT_ATT_OP_NOTIFY:
	case BT_ATT_OP_NOTIFY_MULT:
		return ATT_NOTIFICATION;
	case BT_ATT_OP_INDICATE:
		return ATT_INDICATION;
	}

	if (op & ATT_CMD_MASK) {
		return ATT_COMMAND;
	}

	return ATT_UNKNOWN;
}

static struct bt_conn *get_conn(struct bt_att_chan *att_chan)
{
	return att_chan->chan.chan.conn;
}

static int bt_att_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_att_chan *att_chan = ATT_CHAN(chan);
	struct bt_conn *conn = get_conn(att_chan);
	struct bt_att_hdr *hdr;
	const struct att_handler *handler;
	uint8_t err;
	size_t i;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small ATT PDU received");
		return 0;
	}

	hdr = (struct bt_att_hdr *)net_buf_pull_mem(buf, sizeof(*hdr));
	LOG_DBG("Received ATT chan %p code 0x%02x len %zu", att_chan, hdr->code,
		net_buf_frags_len(buf));

	if (conn->state != BT_CONN_CONNECTED) {
		LOG_DBG("not connected: conn %p state %u", conn, conn->state);
		return 0;
	}

	if (!att_chan->att) {
		LOG_DBG("Ignore recv on detached ATT chan");
		return 0;
	}

	for (i = 0, handler = NULL; i < ARRAY_SIZE(handlers); i++) {
		if (hdr->code == handlers[i].op) {
			handler = &handlers[i];
			break;
		}
	}

	if (!handler) {
		LOG_WRN("Unhandled ATT code 0x%02x", hdr->code);
		if (att_op_get_type(hdr->code) != ATT_COMMAND &&
		    att_op_get_type(hdr->code) != ATT_INDICATION) {
			send_err_rsp(att_chan, hdr->code, 0,
				     BT_ATT_ERR_NOT_SUPPORTED);
		}
		return 0;
	}

	if (buf->len < handler->expect_len) {
		LOG_ERR("Invalid len %u for code 0x%02x", buf->len, hdr->code);
		err = BT_ATT_ERR_INVALID_PDU;
	} else {
		err = handler->func(att_chan, buf);
	}

	if (handler->type == ATT_REQUEST && err) {
		LOG_DBG("ATT error 0x%02x", err);
		send_err_rsp(att_chan, hdr->code, 0, err);
	}

	return 0;
}

static struct bt_att *att_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;
	struct bt_att_chan *att_chan;

	if (conn->state != BT_CONN_CONNECTED) {
		LOG_WRN("Not connected");
		return NULL;
	}

	chan = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_ATT);
	if (!chan) {
		LOG_ERR("Unable to find ATT channel");
		return NULL;
	}

	att_chan = ATT_CHAN(chan);
	if (!atomic_test_bit(att_chan->flags, ATT_CONNECTED)) {
		LOG_ERR("ATT channel not connected");
		return NULL;
	}

	return att_chan->att;
}

static struct bt_att_chan *att_chan_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;
	struct bt_att_chan *att_chan;

	if (conn->state != BT_CONN_CONNECTED) {
		LOG_WRN("Not connected");
		return NULL;
	}

	chan = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_ATT);
	if (!chan) {
		LOG_ERR("Unable to find ATT channel");
		return NULL;
	}

	att_chan = ATT_CHAN(chan);
	if (atomic_test_bit(att_chan->flags, ATT_DISCONNECTED)) {
		LOG_WRN("ATT channel flagged as disconnected");
		return NULL;
	}

	return att_chan;
}

struct net_buf *bt_att_create_pdu(struct bt_conn *conn, uint8_t op, size_t len)
{
	struct bt_att *att;
	struct bt_att_chan *chan, *tmp;

	att = att_get(conn);
	if (!att) {
		return NULL;
	}

	/* This allocator should _not_ be used for RSPs. */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		if (len + sizeof(op) > bt_att_mtu(chan)) {
			continue;
		}

		return bt_att_chan_create_pdu(chan, op, len);
	}

	LOG_WRN("No ATT channel for MTU %zu", len + sizeof(op));

	return NULL;
}

struct net_buf *bt_att_create_rsp_pdu(struct bt_att_chan *chan, uint8_t op)
{
	size_t headroom;
	struct bt_att_hdr *hdr;
	struct bt_att_tx_meta_data *data;
	struct net_buf *buf;

	buf = net_buf_alloc(&att_pool, BT_ATT_TIMEOUT);
	if (!buf) {
		LOG_ERR("Unable to allocate buffer for op 0x%02x", op);
		return NULL;
	}

	headroom = BT_L2CAP_BUF_SIZE(0);

	if (bt_att_is_enhanced(chan)) {
		headroom += BT_L2CAP_SDU_HDR_SIZE;
	}

	net_buf_reserve(buf, headroom);

	data = bt_att_get_tx_meta_data(buf);
	data->att_chan = chan;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}

static void att_reset(struct bt_att *att)
{
	struct net_buf *buf;

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
	/* Discard queued buffers */
	while ((buf = net_buf_slist_get(&att->prep_queue))) {
		net_buf_unref(buf);
	}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT > 0 */

#if (defined(CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0))
	struct k_work_sync sync;

	(void)k_work_cancel_delayable_sync(&att->eatt.connection_work, &sync);
#endif /* CONFIG_BT_EATT */

	while ((buf = net_buf_get(&att->tx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	/* Notify pending requests */
	while (!sys_slist_is_empty(&att->reqs)) {
		struct bt_att_req *req;
		sys_snode_t *node;

		node = sys_slist_get_not_empty(&att->reqs);
		req = CONTAINER_OF(node, struct bt_att_req, node);
		if (req->func) {
			req->func(att->conn, -ECONNRESET, NULL, 0,
				  req->user_data);
		}

		bt_att_req_free(req);
	}

	/* FIXME: `att->conn` is not reference counted. Consider using `bt_conn_ref`
	 * and `bt_conn_unref` to follow convention.
	 */
	att->conn = NULL;
	k_mem_slab_free(&att_slab, (void *)att);
}

static void att_chan_detach(struct bt_att_chan *chan)
{
	struct net_buf *buf;

	LOG_DBG("chan %p", chan);

	sys_slist_find_and_remove(&chan->att->chans, &chan->node);

	/* Release pending buffers */
	while ((buf = net_buf_get(&chan->tx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	/* Release pending buffers */
	while ((buf = net_buf_get(&chan->rx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	if (chan->req) {
		/* Notify outstanding request */
		att_handle_rsp(chan, NULL, 0, -ECONNRESET);
	}

	chan->att = NULL;
	atomic_clear_bit(chan->flags, ATT_CONNECTED);
}

static void rx_async_work(struct k_work *work)
{
	struct bt_att_chan *chan = CONTAINER_OF(work, struct bt_att_chan,
						rx_work);
	struct bt_l2cap_le_chan *ch = &chan->chan;
	struct net_buf *buf;
	int err = -ENOENT;

	LOG_DBG("RX workqueue");

	if (!atomic_test_bit(chan->flags, ATT_CONNECTED)) {
		LOG_WRN("Waiting for ATT channel ready");
		return;
	}

	do
	{
		buf = net_buf_get(&chan->rx_queue, K_NO_WAIT);
		if (buf != NULL)
		{
			err = bt_att_recv(&ch->chan, buf);
			net_buf_unref(buf);
		}
	} while (NULL != buf);
	(void)err;
}

static void att_timeout(struct k_work *work)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_att_chan *chan = CONTAINER_OF(dwork, struct bt_att_chan,
						timeout_work);

	bt_addr_le_to_str(bt_conn_get_dst(chan->att->conn), addr, sizeof(addr));
	LOG_ERR("ATT Timeout for device %s", addr);

	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part F] page 480:
	 *
	 * A transaction not completed within 30 seconds shall time out. Such a
	 * transaction shall be considered to have failed and the local higher
	 * layers shall be informed of this failure. No more attribute protocol
	 * requests, commands, indications or notifications shall be sent to the
	 * target device on this ATT Bearer.
	 */
	bt_att_disconnected(&chan->chan.chan);
}

#if (defined(CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0))
static struct bt_att_chan *att_get_fixed_chan(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_ATT);
	__ASSERT(chan, "No ATT channel found");

	return ATT_CHAN(chan);
}
#endif /* CONFIG_BT_EATT */

static void att_chan_attach(struct bt_att *att, struct bt_att_chan *chan)
{
	LOG_DBG("att %p chan %p flags %lu", att, chan, atomic_get(chan->flags));

	if (sys_slist_is_empty(&att->chans)) {
		/* Init general queues when attaching the first channel */
		k_fifo_init(&att->tx_queue);
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
		sys_slist_init(&att->prep_queue);
#endif
	}

	sys_slist_prepend(&att->chans, &chan->node);
}

static void bt_att_connected(struct bt_l2cap_chan *chan)
{
	struct bt_att_chan *att_chan = ATT_CHAN(chan);
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

	LOG_DBG("chan %p cid 0x%04x", le_chan, le_chan->tx.cid);

	atomic_set_bit(att_chan->flags, ATT_CONNECTED);

	att_chan_mtu_updated(att_chan);

	k_work_init_delayable(&att_chan->timeout_work, att_timeout);

	bt_gatt_connected(le_chan->chan.conn);

	k_work_submit(&att_chan->rx_work);
}

static void bt_att_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_att_chan *att_chan = ATT_CHAN(chan);
	struct bt_att *att = att_chan->att;
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);

	LOG_DBG("chan %p cid 0x%04x", le_chan, le_chan->tx.cid);

	if (!att_chan->att) {
		LOG_DBG("Ignore disconnect on detached ATT chan");
		return;
	}

	att_chan_detach(att_chan);

	/* Don't reset if there are still channels to be used */
	if (!sys_slist_is_empty(&att->chans)) {
		return;
	}

	att_reset(att);

	bt_gatt_disconnected(le_chan->chan.conn);
}

#if (defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U))
static uint8_t att_req_retry(struct bt_att_chan *att_chan)
{
	struct bt_att_req *req = att_chan->req;
	struct net_buf *buf;

	/* Resend buffer */
	if (!req->encode) {
		/* This request does not support resending */
		return BT_ATT_ERR_AUTHENTICATION;
	}


	buf = bt_att_chan_create_pdu(att_chan, req->att_op, req->len);
	if (!buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	if (req->encode(buf, req->len, req->user_data)) {
		net_buf_unref(buf);
		return BT_ATT_ERR_UNLIKELY;
	}

	if (chan_send(att_chan, buf)) {
		net_buf_unref(buf);
		return BT_ATT_ERR_UNLIKELY;
	}

	return BT_ATT_ERR_SUCCESS;
}

static void bt_att_encrypt_change(struct bt_l2cap_chan *chan,
				  uint8_t hci_status)
{
	struct bt_att_chan *att_chan = ATT_CHAN(chan);
	struct bt_l2cap_le_chan *le_chan = BT_L2CAP_LE_CHAN(chan);
	struct bt_conn *conn = le_chan->chan.conn;
	uint8_t err;

	LOG_DBG("chan %p conn %p handle %u sec_level 0x%02x status 0x%02x", le_chan, conn,
		conn->handle, conn->sec_level, hci_status);

	if (!att_chan->att) {
		LOG_DBG("Ignore encrypt change on detached ATT chan");
		return;
	}

	/*
	 * If status (HCI status of security procedure) is non-zero, notify
	 * outstanding request about security failure.
	 */
	if (hci_status) {
		if (att_chan->req && att_chan->req->retrying) {
			att_handle_rsp(att_chan, NULL, 0,
					   BT_ATT_ERR_AUTHENTICATION);
		}

		return;
	}

	bt_gatt_encrypt_change(conn);

	if (conn->sec_level == BT_SECURITY_L1) {
		return;
	}

	if (!(att_chan->req && att_chan->req->retrying)) {
		return;
	}

	LOG_DBG("Retrying");

	err = att_req_retry(att_chan);
	if (err) {
		LOG_DBG("Retry failed (%d)", err);
		att_handle_rsp(att_chan, NULL, 0, err);
	}
}
#endif /* CONFIG_BT_SMP */

static void bt_att_status(struct bt_l2cap_chan *ch, atomic_t *status)
{
	struct bt_att_chan *chan = ATT_CHAN(ch);
	sys_snode_t *node;

	LOG_DBG("chan %p status %p", ch, status);

	if (!atomic_test_bit(status, BT_L2CAP_STATUS_OUT)) {
		return;
	}

	if (!chan->att) {
		LOG_DBG("Ignore status on detached ATT chan");
		return;
	}

	/* If there is a request pending don't attempt to send */
	if (chan->req) {
		return;
	}

	/* Pull next request from the list */
	node = sys_slist_get(&chan->att->reqs);
	if (!node) {
		return;
	}

	if (bt_att_chan_req_send(chan, ATT_REQ(node)) >= 0) {
		return;
	}

	/* Prepend back to the list as it could not be sent */
	sys_slist_prepend(&chan->att->reqs, node);
}

static void bt_att_released(struct bt_l2cap_chan *ch)
{
	struct bt_att_chan *chan = ATT_CHAN(ch);

	LOG_DBG("chan %p", chan);

	k_mem_slab_free(&chan_slab, (void *)chan);
}

#if (defined(CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0))
static void bt_att_reconfigured(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_att_chan *att_chan = ATT_CHAN(l2cap_chan);

	LOG_DBG("chan %p", att_chan);

	att_chan_mtu_updated(att_chan);
}
#endif /* CONFIG_BT_EATT */

static struct bt_att_chan *att_chan_new(struct bt_att *att, atomic_val_t flags)
{
	int quota = 0;
	static struct bt_l2cap_chan_ops ops = {
		.connected = bt_att_connected,
		.disconnected = bt_att_disconnected,
		.recv = bt_att_recv,
		.sent = bt_att_sent,
		.status = bt_att_status,
#if (defined(CONFIG_BT_SMP) && ((CONFIG_BT_SMP) > 0U))
		.encrypt_change = bt_att_encrypt_change,
#endif /* CONFIG_BT_SMP */
		.released = bt_att_released,
	#if (defined(CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0))
		.reconfigured = bt_att_reconfigured,
	#endif /* CONFIG_BT_EATT */
	};
	struct bt_att_chan *chan;

	SYS_SLIST_FOR_EACH_CONTAINER(&att->chans, chan, node) {
		if (chan->att == att) {
			quota++;
		}

		if (quota == ATT_CHAN_MAX) {
			LOG_DBG("Maximum number of channels reached: %d", quota);
			return NULL;
		}
	}

	if (k_mem_slab_alloc(&chan_slab, (void **)&chan, K_NO_WAIT)) {
		LOG_WRN("No available ATT channel for conn %p", att->conn);
		return NULL;
	}

	(void)memset(chan, 0, sizeof(*chan));
	chan->chan.chan.ops = &ops;

	k_work_init(&chan->rx_work, rx_async_work);

	k_fifo_init(&chan->tx_queue);
	k_fifo_init(&chan->rx_queue);
	atomic_set(chan->flags, flags);
	chan->att = att;
	att_chan_attach(att, chan);

	if (bt_att_is_enhanced(chan)) {
		/* EATT: The MTU will be sent in the ECRED conn req/rsp PDU. The
		 * TX MTU is received on L2CAP-level.
		 */
		chan->chan.rx.mtu = BT_LOCAL_ATT_MTU_EATT;
	} else {
		/* UATT: L2CAP Basic is not able to communicate the L2CAP MTU
		 * without help. ATT has to manage the MTU. The initial MTU is
		 * defined by spec.
		 */
		chan->chan.tx.mtu = BT_ATT_DEFAULT_LE_MTU;
		chan->chan.rx.mtu = BT_ATT_DEFAULT_LE_MTU;
	}

	return chan;
}

#if 0//(defined(CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0))
size_t bt_eatt_count(struct bt_conn *conn)
{
	struct bt_att *att;
	struct bt_att_chan *chan;
	size_t eatt_count = 0;

	if (!conn) {
		return 0;
	}

	att = att_get(conn);
	if (!att) {
		return 0;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&att->chans, chan, node) {
		if (bt_att_is_enhanced(chan) &&
		    atomic_test_bit(chan->flags, ATT_CONNECTED)) {
			eatt_count++;
		}
	}

	return eatt_count;
}
#if 0
static void att_enhanced_connection_work_handler(struct k_work *work)
{
	const struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	const struct bt_att *att = CONTAINER_OF(dwork, struct bt_att, eatt.connection_work);
	const int err = bt_eatt_connect(att->conn, att->eatt.chans_to_connect);

	if (err == -ENOMEM) {
		LOG_DBG("Failed to connect %d EATT channels, central has probably "
			   "already established some.",
			   att->eatt.chans_to_connect);
	} else if (err < 0) {
		LOG_WRN("Failed to connect %d EATT channels (err: %d)", att->eatt.chans_to_connect,
			err);
	}

}
#endif
#endif /* CONFIG_BT_EATT */

#if ((defined CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0u))
static void eatt_enhanced_connection_work_handler(struct k_work *work);
#endif

static int bt_att_accept_internal(struct bt_att_handle *handle)
{
	struct bt_att *att;
	struct bt_att_chan *chan;

	LOG_DBG("ATT handle %p", handle);

	if (k_mem_slab_alloc(&att_slab, (void **)&att, K_NO_WAIT)) {
		LOG_ERR("No available ATT context for att %p", handle);
		return -ENOMEM;
	}

	(void)memset(att, 0, sizeof(*att));
	sys_slist_init(&att->reqs);
	sys_slist_init(&att->chans);

	k_sem_init(&att->lock, 1, 1);

#if (defined(CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0U))
	k_work_init_delayable(&att->eatt.connection_work,
				  eatt_enhanced_connection_work_handler);
#endif /* CONFIG_BT_EATT */

	chan = att_chan_new(att, 0);
	if (!chan) {
		return -ENOMEM;
	}

	chan->handle = handle->handle;

	handle->chan = chan;

	return 0;
}

static struct bt_att_handle *ethermind_att_handle_lookup_by_device_id(uint8_t device_id);

static int bt_att_accept(struct bt_conn *conn, struct bt_l2cap_chan **ch)
{
	struct bt_att_handle *handle;
	struct bt_att_chan *chan;

	LOG_DBG("conn %p handle %u", conn, conn->handle);

	handle = ethermind_att_handle_lookup_by_device_id(conn->deviceId);
	if (handle == NULL) {
		LOG_ERR("ATT Handle is not found");
		return -ENOMEM;
	}

	att_handle_rsp_thread = k_current_get();

	chan = handle->chan;
	chan->att->conn = conn;

	*ch = &chan->chan.chan;

	return 0;
}

/* The L2CAP channel section is sorted lexicographically. Make sure that ATT fixed channel will be
 * placed as the last one to ensure that SMP channel is properly initialized before bt_att_connected
 * tries to send security request.
 */
BT_L2CAP_CHANNEL_DEFINE(z_att_fixed_chan, BT_L2CAP_CID_ATT, bt_att_accept, NULL);

#if (defined(CONFIG_BT_EATT) && ((CONFIG_BT_EATT) > 0U))
#if 0
static k_timeout_t credit_based_connection_delay(struct bt_conn *conn)
{
	/*
	 * 5.3 Vol 3, Part G, Section 5.4 L2CAP COLLISION MITIGATION
	 * ... In this situation, the Central may retry
	 * immediately but the Peripheral shall wait a minimum of 100 ms before retrying;
	 * on LE connections, the Peripheral shall wait at least 2 *
	 * (connPeripheralLatency + 1) * connInterval if that is longer.
	 */

	if (IS_ENABLED(CONFIG_BT_CENTRAL) && conn->role == BT_CONN_ROLE_CENTRAL) {
		return K_NO_WAIT;
	} else if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		uint8_t random;
		int err;

		err = bt_rand(&random, sizeof(random));
		if (err) {
			random = 0;
		}

		const uint8_t rand_delay = random & 0x7; /* Small random delay for IOP */
		/* The maximum value of (latency + 1) * 2 multipled with the
		 * maximum connection interval has a maximum value of
		 * 4000000000 which can be stored in 32-bits, so this won't
		 * result in an overflow
		 */
		const uint32_t calculated_delay_us =
			2 * (conn->le.latency + 1) * BT_CONN_INTERVAL_TO_US(conn->le.interval);
		const uint32_t calculated_delay_ms = calculated_delay_us / USEC_PER_MSEC;

		return K_MSEC(MAX(100, calculated_delay_ms + rand_delay));
	}

	/* Must be either central or peripheral */
	__ASSERT_NO_MSG(false);
}
#endif

#if 0
static int att_schedule_eatt_connect(struct bt_conn *conn, uint8_t chans_to_connect)
{
	struct bt_att *att = att_get(conn);

	if (!att) {
		return -ENOTCONN;
	}

	att->eatt.chans_to_connect = chans_to_connect;

	return k_work_reschedule(&att->eatt.connection_work,
				 credit_based_connection_delay(conn));
}
#endif

#if 0
static void handle_potential_collision(struct bt_att *att)
{
	__ASSERT_NO_MSG(att);

	int err;
	size_t to_connect = att->eatt.prev_conn_req_missing_chans;

	if (att->eatt.prev_conn_rsp_result == BT_L2CAP_LE_ERR_NO_RESOURCES &&
		att->eatt.prev_conn_req_result == BT_L2CAP_LE_ERR_NO_RESOURCES) {
		LOG_DBG("Credit based connection request collision detected");

		/* Reset to not keep retrying on repeated failures */
		att->eatt.prev_conn_rsp_result = 0;
		att->eatt.prev_conn_req_result = 0;
		att->eatt.prev_conn_req_missing_chans = 0;

		if (to_connect == 0) {
			return;
		}

		err = att_schedule_eatt_connect(att->conn, to_connect);
		if (err < 0) {
			LOG_ERR("Failed to schedule EATT connection retry (err: %d)", err);
		}
	}
}
#endif

#if 0
static void ecred_connect_req_cb(struct bt_conn *conn, uint16_t result, uint16_t psm)
{
	struct bt_att *att = att_get(conn);

	if (!att) {
		return;
	}

	if (psm != BT_EATT_PSM) {
		/* Collision mitigation is only a requirement on the EATT PSM */
		return;
	}

	att->eatt.prev_conn_rsp_result = result;

	handle_potential_collision(att);
}
#endif

#if 0
static void ecred_connect_rsp_cb(struct bt_conn *conn, uint16_t result,
				 uint8_t attempted_to_connect, uint8_t succeeded_to_connect,
				 uint16_t psm)
{
	struct bt_att *att = att_get(conn);

	if (!att) {
		return;
	}

	if (psm != BT_EATT_PSM) {
		/* Collision mitigation is only a requirement on the EATT PSM */
		return;
	}

	att->eatt.prev_conn_req_result = result;
	att->eatt.prev_conn_req_missing_chans =
		attempted_to_connect - succeeded_to_connect;

	handle_potential_collision(att);
}
#endif

#if 0
int bt_eatt_connect(struct bt_conn *conn, size_t num_channels)
{
#if (defined(CONFIG_BT_L2CAP_ECRED) && ((CONFIG_BT_L2CAP_ECRED) > 0))
	struct bt_att_chan *att_chan;
	struct bt_att *att;
	struct bt_l2cap_chan *chan[CONFIG_BT_EATT_MAX + 1] = {NULL};
	size_t offset = 0;
	size_t i = 0;
	int err;

	if (!conn) {
		return -EINVAL;
	}

	/* Check the encryption level for EATT */
	if (bt_conn_get_security(conn) < BT_SECURITY_L2) {
		/* Vol 3, Part G, Section 5.3.2 Channel Requirements states:
		 * The channel shall be encrypted.
		 */
		return -EPERM;
	}

	if (num_channels > CONFIG_BT_EATT_MAX || num_channels == 0) {
		return -EINVAL;
	}

	att_chan = eatt_att_get_fixed_chan(conn);
	att = att_chan->att;

	memset(chan, 0, sizeof(struct bt_l2cap_chan *) * (CONFIG_BT_EATT_MAX + 1));

	while (num_channels--) {
		att_chan = eatt_att_chan_new(att, BIT(ATT_ENHANCED));
		if (!att_chan) {
			break;
		}

		chan[i] = &att_chan->chan.chan;
		i++;
	}

	if (!i) {
		return -ENOMEM;
	}

	while (offset < i) {
		/* bt_l2cap_ecred_chan_connect() uses the first L2CAP_ECRED_CHAN_MAX_PER_REQ
		 * elements of the array or until a null-terminator is reached.
		 */
		err = bt_l2cap_ecred_chan_connect(conn, &chan[offset], BT_EATT_PSM);
		if (err < 0) {
			return err;
		}

		offset += L2CAP_ECRED_CHAN_MAX_PER_REQ;
	}
#endif

	return 0;
}
#endif

#if (defined(CONFIG_BT_EATT_AUTO_CONNECT) && (CONFIG_BT_EATT_AUTO_CONNECT > 0))
#if 0
static void eatt_auto_connect(struct bt_conn *conn, bt_security_t level,
				  enum bt_security_err err)
{
	int eatt_err;

	if (err || level < BT_SECURITY_L2 || !bt_att_fixed_chan_only(conn)) {
		return;
	}

	eatt_err = att_schedule_eatt_connect(conn, CONFIG_BT_EATT_MAX);
	if (eatt_err < 0) {
		LOG_WRN("Automatic creation of EATT bearers failed on "
			"connection %s with error %d",
			bt_addr_le_str_real(bt_conn_get_dst(conn)), eatt_err);
	}
}
#endif
#endif /* CONFIG_BT_EATT_AUTO_CONNECT */

int bt_eatt_disconnect(struct bt_conn *conn)
{
	struct bt_att_chan *chan;
	struct bt_att *att;
	int err = -ENOTCONN;

	if (!conn) {
		return -EINVAL;
	}

	chan = att_get_fixed_chan(conn);
	att = chan->att;

	SYS_SLIST_FOR_EACH_CONTAINER(&att->chans, chan, node) {
		if (bt_att_is_enhanced(chan)) {
			err = bt_l2cap_chan_disconnect(&chan->chan.chan);
		}
	}

	return err;
}

#if (defined(CONFIG_BT_TESTING) && (CONFIG_BT_TESTING > 0))
#if 0
int bt_eatt_disconnect_one(struct bt_conn *conn)
{
	struct bt_att_chan *chan = att_get_fixed_chan(conn);
	struct bt_att *att = chan->att;
	int err = -ENOTCONN;

	if (!conn) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&att->chans, chan, node) {
		if (bt_att_is_enhanced(chan)) {
			err = bt_l2cap_chan_disconnect(&chan->chan.chan);
			return err;
		}
	}

	return err;
}

int bt_eatt_reconfigure(struct bt_conn *conn, uint16_t mtu)
{
	struct bt_att_chan *att_chan = att_get_fixed_chan(conn);
	struct bt_att *att = att_chan->att;
	struct bt_l2cap_chan *chans[CONFIG_BT_EATT_MAX + 1] = { NULL };
	size_t offset = 0;
	size_t i = 0;
	int err;

	SYS_SLIST_FOR_EACH_CONTAINER(&att->chans, att_chan, node) {
		if (bt_att_is_enhanced(att_chan)) {
			chans[i] = &att_chan->chan.chan;
			i++;
		}
	}

	while (offset < i) {
		/* bt_l2cap_ecred_chan_reconfigure() uses the first L2CAP_ECRED_CHAN_MAX_PER_REQ
		 * elements of the array or until a null-terminator is reached.
		 */
		err = bt_l2cap_ecred_chan_reconfigure(&chans[offset], mtu, 0);
		if (err < 0) {
			return err;
		}

		offset += L2CAP_ECRED_CHAN_MAX_PER_REQ;
	}

	return 0;
}
#endif
#endif /* CONFIG_BT_TESTING */
#endif /* CONFIG_BT_EATT */
#if 0
static int bt_eatt_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	struct bt_att_chan *att_chan = att_get_fixed_chan(conn);
	struct bt_att *att = att_chan->att;

	LOG_DBG("conn %p handle %u", conn, conn->handle);

	att_chan = att_chan_new(att, BIT(ATT_ENHANCED));
	if (att_chan) {
		*chan = &att_chan->chan.chan;
		return 0;
	}

	return -ENOMEM;
}

static void bt_eatt_init(void)
{
	int err;
	static struct bt_l2cap_server eatt_l2cap = {
		.psm = BT_EATT_PSM,
		.sec_level = BT_SECURITY_L2,
		.accept = bt_eatt_accept,
	};
	struct bt_l2cap_server *registered_server;

	LOG_DBG("");

	/* Check if eatt_l2cap server has already been registered. */
	registered_server = bt_l2cap_server_lookup_psm(eatt_l2cap.psm);
	if (registered_server != &eatt_l2cap) {
		err = bt_l2cap_server_register(&eatt_l2cap);
		if (err < 0) {
			LOG_ERR("EATT Server registration failed %d", err);
		}
	}

#if (defined(CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0))
	static const struct bt_l2cap_ecred_cb cb = {
		.ecred_conn_rsp = ecred_connect_rsp_cb,
		.ecred_conn_req = ecred_connect_req_cb,
	};

	bt_l2cap_register_ecred_cb(&cb);
#endif /* CONFIG_BT_EATT */
}
#endif

/*
 * EDEGFAST Bluetooth: Added checking condition here.
 * This change used to recevied incoming ATT packets from Ethermind.
 */
NET_BUF_POOL_DEFINE(att_rx_pool, CONFIG_BT_ATT_RX_MAX, BT_ATT_MTU,
			CONFIG_NET_BUF_USER_DATA_SIZE, NULL);

static int bt_att_accept_internal(struct bt_att_handle *handle);

static int ethermind_att_handle_alloc(ATT_HANDLE *handle)
{
	size_t index;
	unsigned int reg;

	reg = DisableGlobalIRQ();
	for (index = 0;index < CONFIG_BT_MAX_CONN;index++)
	{
		if (att_handle_node[index].chan == NULL)
		{
			int err;
			att_handle_node[index].handle = *handle;
			err = bt_att_accept_internal(&att_handle_node[index]);
			EnableGlobalIRQ(reg);
			return err;
		}
	}
	EnableGlobalIRQ(reg);
	return -ENOSR;
}

static int ethermind_att_handle_free(ATT_HANDLE *handle)
{
	size_t index;
	unsigned int reg;

	reg = DisableGlobalIRQ();
	for (index = 0;index < CONFIG_BT_MAX_CONN;index++)
	{
		if (att_handle_node[index].chan != NULL)
		{
			if (att_handle_node[index].handle.att_id == handle->att_id)
			{
				att_handle_node[index].chan = NULL;
				att_handle_node[index].handle.att_id = 0;
				att_handle_node[index].handle.device_id = 0;
				EnableGlobalIRQ(reg);
				return 0;
			}
		}
	}
	EnableGlobalIRQ(reg);
	return -ENXIO;
}

static struct bt_att_handle *ethermind_att_handle_lookup_by_device_id(uint8_t device_id)
{
	size_t index;
	unsigned int reg;

	reg = DisableGlobalIRQ();
	for (index = 0;index < CONFIG_BT_MAX_CONN;index++)
	{
		if (att_handle_node[index].chan != NULL)
		{
			if (att_handle_node[index].handle.device_id == device_id)
			{
				EnableGlobalIRQ(reg);
				return &att_handle_node[index];
			}
		}
	}
	EnableGlobalIRQ(reg);
	return NULL;
}

static API_RESULT ethermind_bt_att_cb
		   (
			   ATT_HANDLE	* handle,
			   UCHAR		 att_event,
			   API_RESULT	event_result,
			   UCHAR		 * eventdata,
			   UINT16		event_datalen
		   )
{
	struct bt_att_chan *attChan = NULL;
	struct bt_att_handle *att_handle;
	struct bt_conn *conn = NULL;
	int err = -ENOENT;

	conn = bt_conn_lookup_device_id(handle->device_id);

	if (NULL != conn) {
		attChan = att_chan_get(conn);
	}

	if (attChan == NULL) {
		LOG_WRN("Cannot find ATT channel on conn %p, Get ATT handle", conn);
		att_handle = ethermind_att_handle_lookup_by_device_id(handle->device_id);
		if (att_handle != NULL) {
			LOG_DBG("Find ATT handle %p", att_handle);
			attChan = att_handle->chan;
		} else {
			LOG_ERR("Cannot find ATT handle");
		}
	}

	LOG_DBG("att rx chan %p event 0x%02X len %d", attChan, att_event, event_datalen);

	if (ATT_CONNECTION_IND == att_event)
	{
		err = ethermind_att_handle_alloc(handle);
		if (err != 0) {
			LOG_ERR("Cannot alloc att handle");
		}
	}
	else if (ATT_DISCONNECTION_IND == att_event)
	{
		err = ethermind_att_handle_free(handle);
		if (err != 0) {
			LOG_ERR("Cannot find att handle");
		}
	}
	else
	{
		if (NULL != attChan)
		{
			struct net_buf *buf;

			attChan->handle = *handle;
			buf = net_buf_alloc(&att_rx_pool, 0);
			if (NULL != buf)
			{
				struct bt_att_hdr hdr;

				if (ATT_UNKNOWN_PDU_IND != att_event)
				{
					hdr.code = att_event;
					(void)net_buf_add_mem(buf, &hdr, sizeof(hdr));
				}
				else
				{
					/* the hdr->code is in the eventdata */
				}
				(void)net_buf_add_mem(buf, eventdata, event_datalen);
				LOG_DBG("RX queue put buf %p", buf);
				net_buf_put(&attChan->rx_queue, buf);
				k_work_submit(&attChan->rx_work);
			}
			else
			{
				LOG_ERR("Get RX buf failure!");
			}
		}
	}
	if (NULL != conn)
	{
		bt_conn_unref(conn);
	}
	return (0 == err) ? API_SUCCESS : API_FAILURE;
}

static void bt_eatt_init(void);

void bt_att_init(void)
{
	ATT_APPLICATION attInterface;
	static bool initialized = false;

	if (!initialized) {
		attInterface.cb = ethermind_bt_att_cb;
		(void)BT_att_register (&attInterface);
	}

	bt_gatt_init();

	(void)memset(att_handle_node, 0, sizeof(att_handle_node));

#if ((defined CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0U))
	if (IS_ENABLED(CONFIG_BT_EATT)) {
		bt_eatt_init();
	}
#endif
}

ATT_HANDLE *bt_att_lookup_conn(struct bt_conn *conn)
{
	struct bt_att_chan *attChan = att_chan_get(conn);
	if (NULL != attChan)
	{
		return &attChan->handle;
	}
	else
	{
		return NULL;
	}
}

uint16_t bt_att_get_mtu(struct bt_conn *conn)
{
	struct bt_att_chan *chan, *tmp;
	struct bt_att *att;
	uint16_t mtu = 0;

	att = att_get(conn);
	if (!att) {
		return 0;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		if (bt_att_mtu(chan) > mtu) {
			mtu = bt_att_mtu(chan);
		}
	}

	return mtu;
}

static void att_chan_mtu_updated(struct bt_att_chan *updated_chan)
{
	struct bt_att *att = updated_chan->att;
	struct bt_att_chan *chan, *tmp;
	uint16_t max_tx = 0, max_rx = 0;

	/* Get maximum MTU's of other channels */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		if (chan == updated_chan) {
			continue;
		}
		max_tx = MAX(max_tx, chan->chan.tx.mtu);
		max_rx = MAX(max_rx, chan->chan.rx.mtu);
	}

	/* If either maximum MTU has changed */
	if ((updated_chan->chan.tx.mtu > max_tx) ||
	    (updated_chan->chan.rx.mtu > max_rx)) {
		max_tx = MAX(max_tx, updated_chan->chan.tx.mtu);
		max_rx = MAX(max_rx, updated_chan->chan.rx.mtu);
		bt_gatt_att_max_mtu_changed(att->conn, max_tx, max_rx);
	}
}

struct bt_att_req *bt_att_req_alloc(k_timeout_t timeout)
{
	struct bt_att_req *req = NULL;

	if (k_current_get() == att_handle_rsp_thread) {
		/* No req will be fulfilled while blocking on the bt_recv thread.
		 * Blocking would cause deadlock.
		 */
		LOG_DBG("Timeout discarded. No blocking on bt_recv thread.");
		timeout = K_NO_WAIT;
	}

	/* Reserve space for request */
	if (k_mem_slab_alloc(&req_slab, (void **)&req, timeout)) {
		LOG_DBG("No space for req");
		return NULL;
	}

	LOG_DBG("req %p", req);

	memset(req, 0, sizeof(*req));

	return req;
}

void bt_att_req_free(struct bt_att_req *req)
{
	LOG_DBG("req %p", req);

	if (req->buf) {
		net_buf_unref(req->buf);
		req->buf = NULL;
	}

	k_mem_slab_free(&req_slab, (void *)req);
}

int bt_att_send(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_att *att;

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(buf);

	att = att_get(conn);
	if (!att) {
		net_buf_unref(buf);
		return -ENOTCONN;
	}

	net_buf_put(&att->tx_queue, buf);
	att_send_process(att);

	return 0;
}

int bt_att_req_send(struct bt_conn *conn, struct bt_att_req *req)
{
	struct bt_att *att;

	LOG_DBG("conn %p req %p", conn, req);

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(req);

	att = att_get(conn);
	if (!att) {
		return -ENOTCONN;
	}

	att_lock(att);

	sys_slist_append(&att->reqs, &req->node);
	att_req_send_process(att);

	att_unlock(att);

	return 0;
}

static bool bt_att_chan_req_cancel(struct bt_att_chan *chan,
				   struct bt_att_req *req)
{
	if (chan->req != req) {
		return false;
	}

	chan->req = &cancel;

	bt_att_req_free(req);

	return true;
}

void bt_att_req_cancel(struct bt_conn *conn, struct bt_att_req *req)
{
	struct bt_att *att;
	struct bt_att_chan *chan, *tmp;

	LOG_DBG("req %p", req);

	if (!conn || !req) {
		return;
	}

	att = att_get(conn);
	if (!att) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		/* Check if request is outstanding */
		if (bt_att_chan_req_cancel(chan, req)) {
			return;
		}
	}

	/* Remove request from the list */
	sys_slist_find_and_remove(&att->reqs, &req->node);

	bt_att_req_free(req);
}

struct bt_att_req *bt_att_find_req_by_user_data(struct bt_conn *conn, const void *user_data)
{
	struct bt_att *att;
	struct bt_att_chan *chan;
	struct bt_att_req *req;

	att = att_get(conn);
	if (!att) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&att->chans, chan, node) {
		if (chan->req->user_data == user_data) {
			return chan->req;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&att->reqs, req, node) {
		if (req->user_data == user_data) {
			return req;
		}
	}

	return NULL;
}

bool bt_att_fixed_chan_only(struct bt_conn *conn)
{
#if (defined(CONFIG_BT_EATT) && (CONFIG_BT_EATT > 0))
	return bt_eatt_count(conn) == 0;
#else
	return true;
#endif /* CONFIG_BT_EATT */
}

void bt_att_clear_out_of_sync_sent(struct bt_conn *conn)
{
	struct bt_att *att = att_get(conn);
	struct bt_att_chan *chan;

	if (!att) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&att->chans, chan, node) {
		atomic_clear_bit(chan->flags, ATT_OUT_OF_SYNC_SENT);
	}
}

bool bt_att_out_of_sync_sent_on_fixed(struct bt_conn *conn)
{
	struct bt_l2cap_chan *l2cap_chan;
	struct bt_att_chan *att_chan;

	l2cap_chan = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_ATT);
	if (!l2cap_chan) {
		return false;
	}

	att_chan = ATT_CHAN(l2cap_chan);
	return atomic_test_bit(att_chan->flags, ATT_OUT_OF_SYNC_SENT);
}

void bt_att_set_tx_meta_data(struct net_buf *buf, bt_gatt_complete_func_t func, void *user_data,
			     enum bt_att_chan_opt chan_opt)
{
	struct bt_att_tx_meta_data *data = bt_att_get_tx_meta_data(buf);

	data->func = func;
	data->user_data = user_data;
	data->attr_count = 1;
	data->chan_opt = chan_opt;
}

void bt_att_increment_tx_meta_data_attr_count(struct net_buf *buf, uint16_t attr_count)
{
	struct bt_att_tx_meta_data *data = bt_att_get_tx_meta_data(buf);

	data->attr_count += attr_count;
}

bool bt_att_tx_meta_data_match(const struct net_buf *buf, bt_gatt_complete_func_t func,
			       const void *user_data, enum bt_att_chan_opt chan_opt)
{
	const struct bt_att_tx_meta_data *meta = bt_att_get_tx_meta_data(buf);

	return ((meta->func == func) &&
		(meta->user_data == user_data) &&
		(meta->chan_opt == chan_opt));
}

bool bt_att_chan_opt_valid(struct bt_conn *conn, enum bt_att_chan_opt chan_opt)
{
	if ((chan_opt & (BT_ATT_CHAN_OPT_ENHANCED_ONLY | BT_ATT_CHAN_OPT_UNENHANCED_ONLY)) ==
	    (BT_ATT_CHAN_OPT_ENHANCED_ONLY | BT_ATT_CHAN_OPT_UNENHANCED_ONLY)) {
		/* Enhanced and Unenhanced are mutually exclusive */
		return false;
	}

	/* Choosing EATT requires EATT channels connected and encryption enabled */
	if (chan_opt & BT_ATT_CHAN_OPT_ENHANCED_ONLY) {
		return (bt_conn_get_security(conn) > BT_SECURITY_L1) &&
		       !bt_att_fixed_chan_only(conn);
	}

	return true;
}

int bt_gatt_authorization_cb_register(const struct bt_gatt_authorization_cb *cb)
{
	if (!IS_ENABLED(CONFIG_BT_GATT_AUTHORIZATION_CUSTOM)) {
		return -ENOSYS;
	}
#if (defined (CONFIG_BT_GATT_AUTHORIZATION_CUSTOM) && (CONFIG_BT_GATT_AUTHORIZATION_CUSTOM))
	if (!cb) {
		authorization_cb = NULL;
		return 0;
	}

	if (authorization_cb) {
		return -EALREADY;
	}

	authorization_cb = cb;

	return 0;
#endif
}

#define EGDEFAST_BT_PAL_EXT_EATT 1
#include "bt_pal_eatt.c"
#undef EGDEFAST_BT_PAL_EXT_EATT