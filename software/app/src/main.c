/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app_version.h>

#include "modules/blink/blink_module.h"
#include "modules/sensor/sensor_module.h"
#include "modules/ble/ble_module.h"
#include "modules/wifi/wifi_module.h"
#include "modules/display/display_module.h"
#include "modules/button/button_module.h"

#include "thread/blink_task.h"
#include "thread/sensor_task.h"
#include "thread/ble_task.h"
#include "thread/wifi_task.h"
#include "thread/display_task.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

/// Updated for ESP32-WROOM-32 DevKit

int main(void)
{
	int ret;

	printk("Zephyr Example Application %s\n", APP_VERSION_STRING);
	printk("ESP32-WROOM-32 with Modular Architecture\n");

	/* Initialize modules */
	ret = blink_module_init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize blink module (%d)", ret);
		return 0;
	}

	ret = sensor_module_init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize sensor module (%d)", ret);
		return 0;
	}

	ret = ble_module_init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize BLE module (%d)", ret);
		return 0;
	}

	/* Initialize WiFi in AP+STA mode (default) */
	ret = wifi_module_init(WIFI_MODE_AP_STA);
	if (ret < 0) {
		LOG_ERR("Failed to initialize WiFi module (%d)", ret);
		return 0;
	}

	ret = display_module_init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize display module (%d)", ret);
		return 0;
	}

	ret = button_module_init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize button module (%d)", ret);
		return 0;
	}

	LOG_INF("All modules initialized");
	printk("Use the sensor to change LED blinking period\n");

	/* Start task threads */
	ret = blink_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start blink task (%d)", ret);
		return 0;
	}

	ret = sensor_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start sensor task (%d)", ret);
		return 0;
	}

	ret = ble_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start BLE task (%d)", ret);
		return 0;
	}

	ret = wifi_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start WiFi task (%d)", ret);
		return 0;
	}

	ret = display_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start display task (%d)", ret);
		return 0;
	}

	LOG_INF("All tasks started successfully");

	/* Main thread sleeps */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}

