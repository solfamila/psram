/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file
 * @brief Wireless Uart Service Profile
 *
 */

#ifndef __WIRELESS_UART_H__
#define __WIRELESS_UART_H__

typedef int (*bt_gatt_wu_read_response_t)(void *param, uint8_t* buffer, ssize_t length);
typedef int (*bt_gatt_wu_read_request_t)(struct bt_conn *conn, bt_gatt_wu_read_response_t response, void *param);
typedef int (*bt_gatt_wu_data_received_t)(struct bt_conn *conn, uint8_t* buffer, ssize_t length);

typedef struct _bt_gatt_wu_config
{
	bt_gatt_wu_read_request_t read;
	bt_gatt_wu_data_received_t data_received;
} bt_gatt_wu_config_t;

#define WIRELESS_UART_SERIVCE_UUID 0xE0, 0x1C, 0x4B, 0x5E, 0x1E, 0xEB, 0xA1, 0x5C, 0xEE, 0xF4, 0x5E, 0xBA, 0x00, 0x01, 0xFF, 0x01

#define WIRELESS_UART_SERIVCE BT_UUID_DECLARE_128(WIRELESS_UART_SERIVCE_UUID)
#define WIRELESS_UART_RAED_STREAM BT_UUID_DECLARE_128(0xE0, 0x1C, 0x4B, 0x5E, 0x1E, 0xEB, 0xA1, 0x5C, 0xEE, 0xF4, 0x5E, 0xBA, 0x02, 0x01, 0xFF, 0x01)
#define WIRELESS_UART_WRITE_STREAM BT_UUID_DECLARE_128(0xE0, 0x1C, 0x4B, 0x5E, 0x1E, 0xEB, 0xA1, 0x5C, 0xEE, 0xF4, 0x5E, 0xBA, 0x01, 0x01, 0xFF, 0x01)
#define WIRELESS_UART_INFO BT_UUID_DECLARE_128(0xE0, 0x1C, 0x4B, 0x5E, 0x1E, 0xEB, 0xA1, 0x5C, 0xEE, 0xF4, 0x5E, 0xBA, 0x03, 0x01, 0xFF, 0x01)
#define WIRELESS_UART_SERIAL_NO BT_UUID_DECLARE_128(0xE0, 0x1C, 0x4B, 0x5E, 0x1E, 0xEB, 0xA1, 0x5C, 0xEE, 0xF4, 0x5E, 0xBA, 0x04, 0x01, 0xFF, 0x01)

void bt_gatt_wu_init(char* name, char* serial_no, bt_gatt_wu_config_t* config);
void bt_gatt_wu_notify(struct bt_conn *conn);
void bt_gatt_wu_connected(struct bt_conn *conn);
void bt_gatt_wu_disconnected(struct bt_conn *conn);

#endif
