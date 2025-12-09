/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "wifi_task.h"
#include "../modules/wifi/wifiservice.hpp"

LOG_MODULE_REGISTER(wifi_task, CONFIG_APP_LOG_LEVEL);

/* Thread stack and control block */
K_THREAD_STACK_DEFINE(wifi_task_stack, WIFI_TASK_STACK_SIZE);
static struct k_thread wifi_task_data;

/* WiFi connection callback */
static void wifi_connection_callback(bool connected)
{
	if (connected) {
		LOG_INF("WiFi connected!");
	} else {
		LOG_WRN("WiFi disconnected");
	}
}

/* WiFi task thread entry */
static void wifi_task_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_INF("WiFi task started");

	// Get WiFi service instance
	WiFiService& wifi = WiFiService::getInstance();

	// Set connection callback
	wifi.setConnectionCallback(wifi_connection_callback);

	// WiFi service is already initialized in main, just monitor status
	while (1) {
		if (!wifi.isConnected() && wifi.isRunning()) {
			// Attempt reconnection if configured for STA mode
			LOG_INF("Checking WiFi connection status...");
		}
		
		// Check status periodically
		k_sleep(K_SECONDS(10));
	}
}

int wifi_task_start(void)
{
	/* Create WiFi task thread */
	k_thread_create(&wifi_task_data, wifi_task_stack,
			K_THREAD_STACK_SIZEOF(wifi_task_stack),
			wifi_task_entry,
			NULL, NULL, NULL,
			WIFI_TASK_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&wifi_task_data, "wifi_task");

	LOG_INF("WiFi task thread created");
	return 0;
}
