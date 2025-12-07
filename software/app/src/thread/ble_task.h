/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_TASK_H
#define BLE_TASK_H

#include <zephyr/kernel.h>

#define BLE_TASK_STACK_SIZE 2048
#define BLE_TASK_PRIORITY 5

/**
 * @brief Initialize and start the BLE task thread
 * 
 * @return 0 on success, negative errno on failure
 */
int ble_task_start(void);

#endif /* BLE_TASK_H */
