/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "wifi_task.h"
#include "../modules/wifi/wifi_module.h"

LOG_MODULE_REGISTER(wifi_task, CONFIG_APP_LOG_LEVEL);

/* Thread stack and control block */
K_THREAD_STACK_DEFINE(wifi_task_stack, WIFI_TASK_STACK_SIZE);
static struct k_thread wifi_task_data;

/* WiFi event handler */
static void wifi_event_handler(enum wifi_status status)
{
	switch (status) {
	case WIFI_STATUS_DISCONNECTED:
		LOG_WRN("WiFi disconnected");
		break;
	case WIFI_STATUS_CONNECTING:
		LOG_INF("WiFi connecting...");
		break;
	case WIFI_STATUS_CONNECTED:
		{
			char ip_str[16];
			if (wifi_module_get_ip(ip_str, sizeof(ip_str)) == 0) {
				printk("WiFi connected! IP: %s\n", ip_str);
			} else {
				printk("WiFi connected!\n");
			}
		}
		break;
	case WIFI_STATUS_ERROR:
		LOG_ERR("WiFi error");
		break;
	default:
		break;
	}
}

/* WiFi task thread entry */
static void wifi_task_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	enum wifi_status status;
	enum wifi_mode mode;

	LOG_INF("WiFi task started");

	/* Register callback */
	wifi_module_register_callback(wifi_event_handler);

	/* Get current mode */
	mode = wifi_module_get_mode();
	
	/* Start based on mode */
	switch (mode) {
	case WIFI_MODE_STA:
		/* Connect to WiFi in STA mode */
		LOG_INF("Starting WiFi in STA mode");
		ret = wifi_module_connect(NULL, NULL);
		if (ret) {
			LOG_ERR("Failed to start WiFi connection");
		}
		break;

	case WIFI_MODE_AP:
		/* Start AP mode only */
		LOG_INF("Starting WiFi in AP mode");
		ret = wifi_module_start_ap(NULL, NULL, 0);
		if (ret) {
			LOG_ERR("Failed to start AP mode");
		}
		break;

	case WIFI_MODE_AP_STA:
		/* Start AP mode first */
		LOG_INF("Starting WiFi in AP+STA mode");
		ret = wifi_module_start_ap(NULL, NULL, 0);
		if (ret) {
			LOG_ERR("Failed to start AP mode");
		}
		
		/* Then connect to WiFi as station */
		k_sleep(K_SECONDS(2));  /* Give AP time to start */
		ret = wifi_module_connect(NULL, NULL);
		if (ret) {
			LOG_ERR("Failed to start WiFi connection");
		}
		break;

	default:
		LOG_ERR("Unknown WiFi mode");
		return;
	}

	/* Monitor WiFi status and reconnect if needed (only for STA/AP+STA modes) */
	while (1) {
		mode = wifi_module_get_mode();
		
		if (mode == WIFI_MODE_STA || mode == WIFI_MODE_AP_STA) {
			status = wifi_module_get_status();

			if (status == WIFI_STATUS_DISCONNECTED || status == WIFI_STATUS_ERROR) {
				LOG_INF("Attempting to reconnect STA...");
				ret = wifi_module_connect(NULL, NULL);
				if (ret) {
					LOG_ERR("Reconnection failed");
				}
				k_sleep(K_MSEC(WIFI_RECONNECT_INTERVAL_MS));
			} else {
				/* Check status periodically */
				k_sleep(K_SECONDS(10));
			}
		} else {
			/* AP only mode - just sleep */
			k_sleep(K_SECONDS(30));
		}
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
