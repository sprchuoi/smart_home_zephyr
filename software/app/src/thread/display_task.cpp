/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "display_task.h"
#include "../modules/display/displaymodule.hpp"
#include "../modules/button/buttonmodule.hpp"
#include "../modules/wifi/wifiservice.hpp"
#include "../modules/ble/bleservice.hpp"

LOG_MODULE_REGISTER(display_task, CONFIG_APP_LOG_LEVEL);

/* Thread stack and control block */
K_THREAD_STACK_DEFINE(display_task_stack, DISPLAY_TASK_STACK_SIZE);
static struct k_thread display_task_data;

/* Button event handler */
static void button_event_handler(bool pressed)
{
	if (pressed) {
		/* Wake up display on button press */
		DisplayModule::getInstance().wake();
		LOG_INF("Display woken by button");
	}
}

/* Display task thread entry */
static void display_task_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	char wifi_status_str[16];
	char ble_status_str[16];
	char ip_str[16];

	LOG_INF("Display task started");

	/* Register button callback */
	ButtonModule::getInstance().setCallback([]() {
		button_event_handler(true);
	});

	/* Display startup message */
	DisplayModule::getInstance().updateStatus("ESP32 Smart Home", "Initializing...", nullptr);

	k_sleep(K_SECONDS(2));

	/* Main display update loop */
	while (1) {
		/* Get WiFi status */
		if (WiFiService::getInstance().isConnected()) {
			strcpy(wifi_status_str, "Connected");
			// TODO: Get IP address from network interface
			strcpy(ip_str, "0.0.0.0");
		} else {
			strcpy(wifi_status_str, "Disconnected");
			ip_str[0] = '\0';
		}

		/* Get BLE status */
		if (BleService::getInstance().isConnected()) {
			strcpy(ble_status_str, "Connected");
		} else {
			strcpy(ble_status_str, "Advertising");
		}

		/* Update display with status */
		DisplayModule::getInstance().updateStatus(wifi_status_str, ble_status_str, ip_str);

		/* Update every 2 seconds */
		k_sleep(K_SECONDS(2));
	}
}

int display_task_start(void)
{
	/* Create display task thread */
	k_thread_create(&display_task_data, display_task_stack,
			K_THREAD_STACK_SIZEOF(display_task_stack),
			display_task_entry,
			NULL, NULL, NULL,
			DISPLAY_TASK_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&display_task_data, "display_task");

	LOG_INF("Display task thread created");
	return 0;
}
