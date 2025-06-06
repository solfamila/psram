/** @file
 *  @brief GATT Battery Service
 */

/*
 * Copyright 2021-2024 NXP
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno/errno.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <sys/atomic.h>

#include <porting.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>
#include <bluetooth/services/bas.h>

#define LOG_ENABLE IS_ENABLED(CONFIG_BT_DEBUG_SERVICE)
#define LOG_MODULE_NAME bt_bas
#include "fsl_component_log.h"
LOG_MODULE_DEFINE(LOG_MODULE_NAME, kLOG_LevelTrace);

static uint8_t battery_level = 100U;

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				       uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("BAS Notifications %s", notif_enabled ? "enabled" : "disabled");

    (void)notif_enabled;
}

static ssize_t read_blvl(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint8_t lvl8 = battery_level;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &lvl8,
				 sizeof(lvl8));
}

/* Constant values from the Assigned Numbers specification:
 * https://www.bluetooth.com/wp-content/uploads/Files/Specification/Assigned_Numbers.pdf?id=89
 */
static const struct bt_gatt_cpf level_cpf = {
	.format = 0x04,        /* uint8 */
	.exponent = 0x0,
	.unit = 0x27AD,        /* Percentage */
	.name_space = 0x01,    /* Bluetooth SIG */
	.description = 0x0106, /* "main" */
};
static BT_GATT_SERVICE_DEFINE(bas,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_blvl, NULL,
			       &battery_level),
	BT_GATT_CCC(blvl_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CPF(&level_cpf),
);

static int bas_init(void)
{

	return 0;
}

uint8_t bt_bas_get_battery_level(void)
{
	return battery_level;
}

int bt_bas_set_battery_level(uint8_t level)
{
	int rc;

	if (level > 100U) {
		return -EINVAL;
	}

	battery_level = level;

	rc = bt_gatt_notify(NULL, &bas.attrs[1], &level, sizeof(level));

	return rc == -ENOTCONN ? 0 : rc;
}

#if 0
SYS_INIT(bas_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif