/** @file
 *  @brief Bluetooth Call Control Profile (CCP) Call Controller role.
 *
 *  Copyright (c) 2020 Nordic Semiconductor ASA
 *  Copyright (c) 2022 Codecoup
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <string.h>

#include <bluetooth/conn.h>
#include <bluetooth/audio/tbs.h>

#include "fsl_debug_console.h"

#ifndef printk
#define printk PRINTF
#endif

#define URI_SEPARATOR ":"
#define CALLER_ID "friend"

static uint8_t new_call_index;
static char remote_uri[CONFIG_BT_TBS_MAX_URI_LENGTH];

static OSA_SEMAPHORE_HANDLE_DEFINE(sem_discovery_done);

static struct bt_conn *default_conn;

static void discover_cb(struct bt_conn *conn, int err, uint8_t tbs_count, bool gtbs_found)
{
	if (!gtbs_found) {
		printk("CCP: Failed to discover GTBS\n");
		return;
	}

	printk("CCP: Discovered GTBS\n");

	if (err) {
		printk("%s (err %d)\n", __func__, err);
		return;
	}

	/* Read Bearer URI Schemes Supported List Characteristic */
	bt_tbs_client_read_uri_list(conn, BT_TBS_GTBS_INDEX);
}

static void originate_call_cb(struct bt_conn *conn, int err, uint8_t inst_index, uint8_t call_index)
{
	if (inst_index != BT_TBS_GTBS_INDEX) {
		printk("Unexpected %s for instance %u\n", __func__, inst_index);
		return;
	}

	if (err) {
		printk("%s (err %d)\n", __func__, err);
		return;
	}
	printk("CCP: Call originate successful\n");
	new_call_index = call_index;
}

static void terminate_call_cb(struct bt_conn *conn, int err,
			      uint8_t inst_index, uint8_t call_index)
{
	if (inst_index != BT_TBS_GTBS_INDEX) {
		printk("Unexpected %s for instance %u\n", __func__, inst_index);
		return;
	}

	if (err) {
		printk("%s (err %d)\n", __func__, err);
		return;
	}
	printk("CCP: Call with id %d terminated\n", call_index);
}

static void read_uri_schemes_string_cb(struct bt_conn *conn, int err,
				       uint8_t inst_index, const char *value)
{
	size_t i;

	if (inst_index != BT_TBS_GTBS_INDEX) {
		printk("Unexpected %s for instance %u\n", __func__, inst_index);
		return;
	}

	if (err) {
		printk("%s (err %d)\n", __func__, err);
		return;
	}

	/* Save first remote URI
	 *
	 * First search for the first comma (separator), and use that to determine the end of the
	 * first (or only) URI. Then use that length to copy the URI to `remote_uri` for later use.
	 */
	for (i = 0U; i < strlen(value); i++) {
		if (value[i] == ',') {
			break;
		}
	}

	if (i >= sizeof(remote_uri)) {
		printk("Cannot store URI of length %zu: %s\n", i, value);
		return;
	}

	strncpy(remote_uri, value, i);
	remote_uri[i] = '\0';

	printk("CCP: Discovered remote URI: %s\n", remote_uri);
	OSA_SemaphorePost(sem_discovery_done);
}

struct bt_tbs_client_cb tbs_client_cb = {
	.discover = discover_cb,
	.uri_list = read_uri_schemes_string_cb,
	.originate_call = originate_call_cb,
	.terminate_call = terminate_call_cb,
};

int ccp_call_ctrl_init(struct bt_conn *conn)
{
	int err;

    (void)OSA_SemaphoreCreate(sem_discovery_done, 0);

	default_conn = bt_conn_ref(conn);
	bt_tbs_client_register_cb(&tbs_client_cb);
	err = bt_tbs_client_discover(conn);
	if (err != 0) {
		return err;
	}
	OSA_SemaphoreWait(sem_discovery_done, osaWaitForever_c);

	return err;
}

int ccp_originate_call(void)
{
	int err;
	char uri[CONFIG_BT_TBS_MAX_URI_LENGTH];

	strcpy(uri, remote_uri);
	strcat(uri, URI_SEPARATOR);
	strcat(uri, CALLER_ID);

	err = bt_tbs_client_originate_call(default_conn, BT_TBS_GTBS_INDEX, uri);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		printk("TBS originate call failed: %d\n", err);
	}

	return err;
}

int ccp_terminate_call(void)
{
	int err;

	err = bt_tbs_client_terminate_call(default_conn, BT_TBS_GTBS_INDEX, new_call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		printk("TBS terminate call failed: %d\n", err);
	}

	return err;
}
