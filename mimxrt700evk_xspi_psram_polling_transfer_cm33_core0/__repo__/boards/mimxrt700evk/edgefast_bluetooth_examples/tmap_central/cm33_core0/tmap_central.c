/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>

#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/bap.h>
#include <bluetooth/audio/bap_lc3_preset.h>
#include <bluetooth/audio/tmap.h>
#include <bluetooth/audio/cap.h>

#include "tmap_central.h"

#include "fsl_debug_console.h"

#ifndef printk
#define printk PRINTF
#endif

static struct bt_conn *default_conn;

static OSA_SEMAPHORE_HANDLE_DEFINE(sem_connected);
static OSA_SEMAPHORE_HANDLE_DEFINE(sem_security_updated);
static OSA_SEMAPHORE_HANDLE_DEFINE(sem_disconnected);
static OSA_SEMAPHORE_HANDLE_DEFINE(sem_mtu_exchanged);
static OSA_SEMAPHORE_HANDLE_DEFINE(sem_discovery_done);

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged: %u/%u\n", tx, rx);
	OSA_SemaphorePost(sem_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated
};

void tmap_discovery_complete(enum bt_tmap_role role, struct bt_conn *conn, int err)
{
	if (conn != default_conn) {
		return;
	}

	if (err) {
		printk("TMAS discovery failed! (err %d)\n", err);
		return;
	}
	printk("TMAS discovery done\n");
	OSA_SemaphorePost(sem_discovery_done);
}

static struct bt_tmap_cb tmap_callbacks = {
	.discovery_complete = tmap_discovery_complete
};

static void start_scan(void);

static struct bt_conn_cb conn_callbacks;

static int init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");
	bt_gatt_cb_register(&gatt_callbacks);

	bt_conn_cb_register(&conn_callbacks);

	return 0;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);
	OSA_SemaphorePost(sem_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	OSA_SemaphorePost(sem_disconnected);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
				enum bt_security_err err)
{
	if (err == 0) {
		printk("Security changed: %u, level %d\n", err, level);
		OSA_SemaphorePost(sem_security_updated);
	} else {
		printk("Failed to set security level: %u\n", err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

static bool check_audio_support_and_connect(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	struct net_buf_simple tmas_svc_data;
	const struct bt_uuid *uuid;
	uint16_t uuid_val;
	uint16_t peer_tmap_role = 0;
	int err;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	if (data->type != BT_DATA_SVC_DATA16) {
		return true; /* Continue parsing to next AD data type */
	}

	if (data->data_len < sizeof(uuid_val)) {
		printk("AD invalid size %u\n", data->data_len);
		return true; /* Continue parsing to next AD data type */
	}

	net_buf_simple_init_with_data(&tmas_svc_data, (void *)data->data, data->data_len);
	uuid_val = net_buf_simple_pull_le16(&tmas_svc_data);
	uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(uuid_val));
	if (bt_uuid_cmp(uuid, BT_UUID_TMAS) != 0) {
		/* We are looking for the TMAS service data */
		return true; /* Continue parsing to next AD data type */
	}

	printk("Found TMAS in peer adv data!\n");
	if (tmas_svc_data.len < sizeof(peer_tmap_role)) {
		printk("AD invalid size %u\n", data->data_len);
		return false; /* Stop parsing */
	}

	peer_tmap_role = net_buf_simple_pull_le16(&tmas_svc_data);
	if (!(sys_le16_to_cpu(peer_tmap_role) & BT_TMAP_ROLE_UMR)) {
		printk("No TMAS UMR support!\n");
		return false; /* Stop parsing */
	}

	printk("Attempt to connect!\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		printk("Failed to stop scan: %d\n", err);
		return false;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err != 0) {
		printk("Create conn to failed (%u)\n", err);
		start_scan();
	}

	return false; /* Stop parsing */
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
					struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	if (default_conn != NULL) {
		/* Already connected */
		return;
	}

	/* Check for connectable, extended advertising */
	if (((info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0) ||
		((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE)) != 0) {
		bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
		printk("[DEVICE]: %s, ", le_addr);
		/* Check for TMAS support in advertising data */
		bt_data_parse(buf, check_audio_support_and_connect, (void *)info->addr);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static int scan_and_connect(void)
{
	int err;

	start_scan();

	err = OSA_SemaphoreWait(sem_connected, osaWaitForever_c);
	if (err != 0) {
		printk("failed to take sem_connected (err %d)\n", err);
		return err;
	}

	err = OSA_SemaphoreWait(sem_mtu_exchanged, osaWaitForever_c);
	if (err != 0) {
		printk("failed to take sem_mtu_exchanged (err %d)\n", err);
		return err;
	}

	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err != 0) {
		printk("failed to set security (err %d)\n", err);
		return err;
	}

	err = OSA_SemaphoreWait(sem_security_updated, osaWaitForever_c);
	if (err != 0) {
		printk("failed to take sem_security_updated (err %d)\n", err);
		return err;
	}

	return 0;
}

void tmap_central_task(void *param)
{
	int err;

	(void)OSA_SemaphoreCreate(sem_connected, 0);
	(void)OSA_SemaphoreCreate(sem_security_updated, 0);
	(void)OSA_SemaphoreCreate(sem_disconnected, 0);
	(void)OSA_SemaphoreCreate(sem_mtu_exchanged, 0);
	(void)OSA_SemaphoreCreate(sem_discovery_done, 0);

	err = init();
	if (err != 0) {
		while(1);
	}

	printk("Initializing TMAP and setting role\n");
	/* Initialize TMAP */
	err = bt_tmap_register((enum bt_tmap_role)(BT_TMAP_ROLE_CG | BT_TMAP_ROLE_UMS));
	if (err != 0) {
		while(1);
	}

	/* Initialize CAP Initiator */
	err = cap_initiator_init();
	if (err != 0) {
		while(1);
	}
	printk("CAP initialized\n");

	/* Initialize VCP Volume Controller */
	err = vcp_vol_ctlr_init();
	if (err != 0) {
		while(1);
	}
	printk("VCP initialized\n");

	/* Initialize MCP Server */
	err = mcp_server_init();
	if (err != 0) {
		while(1);
	}
	printk("MCP initialized\n");

	/* Initialize CCP Server */
	err = ccp_server_init();
	if (err != 0) {
		while(1);
	}
	printk("CCP initialized\n");

	/* Register scan callback and start scanning */
	bt_le_scan_cb_register(&scan_callbacks);
	err = scan_and_connect();
	if (err != 0) {
		while(1);
	}

	err = bt_tmap_discover(default_conn, &tmap_callbacks);
	if (err != 0) {
		while(1);
	}

	OSA_SemaphoreWait(sem_discovery_done, osaWaitForever_c);

	/* Send a VCP command */
	err = vcp_vol_ctlr_mute();
	if (err != 0) {
		printk("Error sending mute command!\n");
	}

	/* Discover and configure unicast streams */
	err = cap_initiator_setup(default_conn);
	if (err != 0) {
		while(1);
	}

	while(1);
}
