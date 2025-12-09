/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "ble_task.h"
#include "../modules/ble/bleservice.hpp"

LOG_MODULE_REGISTER(ble_task, CONFIG_APP_LOG_LEVEL);

/* Thread stack and control block */
K_THREAD_STACK_DEFINE(ble_task_stack, BLE_TASK_STACK_SIZE);
static struct k_thread ble_task_data;

/* BLE task thread entry */
static void ble_task_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int err;
	uint32_t count = 0;
	BleService& ble = BleService::getInstance();

	LOG_INF("BLE task started");

	/* Start advertising */
	err = ble.startAdvertising();
	if (err) {
		LOG_ERR("Failed to start advertising");
		return;
	}

	/* Periodically send "Hello World" notifications */
	while (1) {
		if (ble.isConnected() && ble.isNotifyEnabled()) {
			char msg[20];
			snprintf(msg, sizeof(msg), "Hello World %u", count++);
			
			err = ble.notify(msg);
			if (err == 0) {
				LOG_INF("Sent: %s", msg);
			}
		}

		k_sleep(K_MSEC(BLE_NOTIFY_INTERVAL_MS));
	}
}

int ble_task_start(void)
{
	/* Create BLE task thread */
	k_thread_create(&ble_task_data, ble_task_stack,
			K_THREAD_STACK_SIZEOF(ble_task_stack),
			ble_task_entry,
			NULL, NULL, NULL,
			BLE_TASK_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&ble_task_data, "ble_task");

	LOG_INF("BLE task thread created");
	return 0;
}
