/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_MODULE_H
#define BLE_MODULE_H

#include <zephyr/kernel.h>
#include <stdint.h>

/* Configuration */
#define BLE_DEVICE_NAME         "ESP32 Smart Home"
#define BLE_NOTIFY_INTERVAL_MS  2000U
#define BLE_MAX_DATA_LEN        20

/**
 * @brief Initialize BLE module
 * 
 * @return 0 on success, negative errno on failure
 */
int ble_module_init(void);

/**
 * @brief Start BLE advertising
 * 
 * @return 0 on success, negative errno on failure
 */
int ble_module_start_advertising(void);

/**
 * @brief Send notification via BLE
 * 
 * @param data Data to send
 * @param len Length of data
 * @return 0 on success, negative errno on failure
 */
int ble_module_notify(const uint8_t *data, uint16_t len);

/**
 * @brief Check if BLE is connected
 * 
 * @return true if connected, false otherwise
 */
bool ble_module_is_connected(void);

/**
 * @brief Check if notifications are enabled
 * 
 * @return true if enabled, false otherwise
 */
bool ble_module_is_notify_enabled(void);

#endif /* BLE_MODULE_H */
