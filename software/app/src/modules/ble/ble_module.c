/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include "ble_module.h"

LOG_MODULE_REGISTER(ble_module, CONFIG_APP_LOG_LEVEL);

/* BLE connection handle */
static struct bt_conn *default_conn = NULL;
static bool notify_enabled = false;

/* Custom Service UUID: 12345678-1234-5678-1234-56789abcdef0 */
#define BT_UUID_CUSTOM_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

/* Custom Characteristic UUID: 12345678-1234-5678-1234-56789abcdef1 */
#define BT_UUID_CUSTOM_CHAR_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

#define BT_UUID_CUSTOM_SERVICE   BT_UUID_DECLARE_128(BT_UUID_CUSTOM_SERVICE_VAL)
#define BT_UUID_CUSTOM_CHAR      BT_UUID_DECLARE_128(BT_UUID_CUSTOM_CHAR_VAL)

/* Custom characteristic value */
static uint8_t char_value[BLE_MAX_DATA_LEN] = "Hello World";

/* CCC changed callback */
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Notifications %s", notify_enabled ? "enabled" : "disabled");
}

/* Characteristic read callback */
static ssize_t read_char(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

/* GATT Service Definition */
BT_GATT_SERVICE_DEFINE(custom_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CUSTOM_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_CUSTOM_CHAR,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       read_char, NULL, char_value),
	BT_GATT_CCC(ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* Advertising data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

/* Scan response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, BLE_DEVICE_NAME, sizeof(BLE_DEVICE_NAME) - 1),
};

/* Connection callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	default_conn = bt_conn_ref(conn);
	LOG_INF("BLE Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("BLE Disconnected (reason %u)", reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
	notify_enabled = false;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int ble_module_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_INF("BLE module initialized");
	return 0;
}

int ble_module_start_advertising(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return err;
	}

	LOG_INF("BLE Advertising started");
	return 0;
}

int ble_module_notify(const uint8_t *data, uint16_t len)
{
	if (!default_conn || !notify_enabled) {
		return -ENOTCONN;
	}

	if (len > BLE_MAX_DATA_LEN) {
		len = BLE_MAX_DATA_LEN;
	}

	int err = bt_gatt_notify(default_conn, &custom_svc.attrs[1], data, len);
	if (err) {
		LOG_ERR("Notify failed (err %d)", err);
		return err;
	}

	return 0;
}

bool ble_module_is_connected(void)
{
	return (default_conn != NULL);
}

bool ble_module_is_notify_enabled(void)
{
	return notify_enabled;
}
