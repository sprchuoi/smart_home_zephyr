/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include "displaymodule.h"

LOG_MODULE_REGISTER(display_module, CONFIG_APP_LOG_LEVEL);

static const struct device *display_dev;
static enum display_state current_state = DISPLAY_STATE_OFF;
static int64_t last_activity_time;
static K_MUTEX_DEFINE(display_mutex);

int display_module_init(void)
{
	int ret;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready");
		return -ENODEV;
	}

	/* Initialize character framebuffer */
	ret = cfb_framebuffer_init(display_dev);
	if (ret) {
		LOG_ERR("Failed to initialize framebuffer (%d)", ret);
		return ret;
	}

	/* Clear display */
	cfb_framebuffer_clear(display_dev, true);

	/* Set font */
	cfb_framebuffer_set_font(display_dev, 0);

	/* Turn on display */
	display_blanking_off(display_dev);
	current_state = DISPLAY_STATE_ON;
	last_activity_time = k_uptime_get();

	LOG_INF("Display module initialized");
	return 0;
}

int display_module_on(void)
{
	int ret;

	k_mutex_lock(&display_mutex, K_FOREVER);

	if (current_state == DISPLAY_STATE_ON) {
		k_mutex_unlock(&display_mutex);
		return 0;
	}

	ret = display_blanking_off(display_dev);
	if (ret) {
		LOG_ERR("Failed to turn on display (%d)", ret);
		k_mutex_unlock(&display_mutex);
		return ret;
	}

	current_state = DISPLAY_STATE_ON;
	last_activity_time = k_uptime_get();
	k_mutex_unlock(&display_mutex);

	LOG_INF("Display turned ON");
	return 0;
}

int display_module_off(void)
{
	int ret;

	k_mutex_lock(&display_mutex, K_FOREVER);

	if (current_state == DISPLAY_STATE_OFF || current_state == DISPLAY_STATE_SLEEP) {
		k_mutex_unlock(&display_mutex);
		return 0;
	}

	ret = display_blanking_on(display_dev);
	if (ret) {
		LOG_ERR("Failed to turn off display (%d)", ret);
		k_mutex_unlock(&display_mutex);
		return ret;
	}

	current_state = DISPLAY_STATE_SLEEP;
	k_mutex_unlock(&display_mutex);

	LOG_INF("Display went to SLEEP");
	return 0;
}

int display_module_clear(void)
{
	int ret;

	k_mutex_lock(&display_mutex, K_FOREVER);
	ret = cfb_framebuffer_clear(display_dev, true);
	k_mutex_unlock(&display_mutex);

	if (ret) {
		LOG_ERR("Failed to clear display (%d)", ret);
		return ret;
	}

	return 0;
}

int display_module_print(uint8_t line, const char *text)
{
	int ret;

	if (!text) {
		return -EINVAL;
	}

	k_mutex_lock(&display_mutex, K_FOREVER);

	/* Print text at specified line */
	ret = cfb_print(display_dev, text, 0, line * 8);
	if (ret) {
		LOG_ERR("Failed to print text (%d)", ret);
		k_mutex_unlock(&display_mutex);
		return ret;
	}

	/* Finalize framebuffer */
	ret = cfb_framebuffer_finalize(display_dev);
	if (ret) {
		LOG_ERR("Failed to finalize framebuffer (%d)", ret);
		k_mutex_unlock(&display_mutex);
		return ret;
	}

	k_mutex_unlock(&display_mutex);
	return 0;
}

int display_module_wake(void)
{
	int ret;

	k_mutex_lock(&display_mutex, K_FOREVER);
	
	/* Update activity time */
	last_activity_time = k_uptime_get();

	/* Turn on display if it was sleeping */
	if (current_state == DISPLAY_STATE_SLEEP) {
		ret = display_blanking_off(display_dev);
		if (ret) {
			LOG_ERR("Failed to wake display (%d)", ret);
			k_mutex_unlock(&display_mutex);
			return ret;
		}
		current_state = DISPLAY_STATE_ON;
		LOG_INF("Display woke up");
	}

	k_mutex_unlock(&display_mutex);
	return 0;
}

enum display_state display_module_get_state(void)
{
	enum display_state state;

	k_mutex_lock(&display_mutex, K_FOREVER);
	state = current_state;
	k_mutex_unlock(&display_mutex);

	return state;
}

int display_module_update_status(const char *wifi_status, const char *ble_status,
                                  const char *ip_addr)
{
	char line_buf[32];

	/* Check for sleep timeout */
	k_mutex_lock(&display_mutex, K_FOREVER);
	int64_t idle_time = k_uptime_get() - last_activity_time;
	
	if (idle_time > DISPLAY_SLEEP_TIMEOUT_MS && current_state == DISPLAY_STATE_ON) {
		k_mutex_unlock(&display_mutex);
		display_module_off();
		return 0;
	}
	k_mutex_unlock(&display_mutex);

	/* Don't update if display is sleeping */
	if (current_state != DISPLAY_STATE_ON) {
		return 0;
	}

	/* Clear and update display */
	display_module_clear();

	/* Line 0: Title */
	display_module_print(0, "ESP32 Smart Home");

	/* Line 2: WiFi Status */
	snprintf(line_buf, sizeof(line_buf), "WiFi: %s", wifi_status ? wifi_status : "N/A");
	display_module_print(2, line_buf);

	/* Line 3: IP Address */
	if (ip_addr && strlen(ip_addr) > 0) {
		snprintf(line_buf, sizeof(line_buf), "IP: %s", ip_addr);
		display_module_print(3, line_buf);
	}

	/* Line 5: BLE Status */
	snprintf(line_buf, sizeof(line_buf), "BLE: %s", ble_status ? ble_status : "N/A");
	display_module_print(5, line_buf);

	/* Line 7: Uptime */
	uint32_t uptime_sec = k_uptime_get() / 1000;
	snprintf(line_buf, sizeof(line_buf), "Up: %um %us", uptime_sec / 60, uptime_sec % 60);
	display_module_print(7, line_buf);

	return 0;
}
