/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <app/drivers/blink.h>
#include "blink_module.h"

LOG_MODULE_REGISTER(blink_module, CONFIG_APP_LOG_LEVEL);

static const struct device *blink_dev;
static unsigned int current_period_ms = BLINK_PERIOD_MS_MAX;
static K_MUTEX_DEFINE(period_mutex);

int blink_module_init(void)
{
	blink_dev = DEVICE_DT_GET(DT_NODELABEL(blink_led));
	if (!device_is_ready(blink_dev)) {
		LOG_ERR("Blink device not ready");
		return -ENODEV;
	}

	int ret = blink_off(blink_dev);
	if (ret < 0) {
		LOG_ERR("Could not turn off LED (%d)", ret);
		return ret;
	}

	LOG_INF("Blink module initialized");
	return 0;
}

int blink_module_set_period(unsigned int period_ms)
{
	k_mutex_lock(&period_mutex, K_FOREVER);
	current_period_ms = period_ms;
	k_mutex_unlock(&period_mutex);
	
	LOG_DBG("Blink period set to %u ms", period_ms);
	return 0;
}

unsigned int blink_module_get_period(void)
{
	unsigned int period;
	
	k_mutex_lock(&period_mutex, K_FOREVER);
	period = current_period_ms;
	k_mutex_unlock(&period_mutex);
	
	return period;
}

int blink_module_tick(void)
{
	int ret;
	unsigned int period = blink_module_get_period();
	
	if (period > 0) {
		ret = blink_on(blink_dev);
		if (ret < 0) {
			LOG_ERR("Could not turn on LED (%d)", ret);
			return ret;
		}
		k_sleep(K_MSEC(period / 2));

		ret = blink_off(blink_dev);
		if (ret < 0) {
			LOG_ERR("Could not turn off LED (%d)", ret);
			return ret;
		}
		k_sleep(K_MSEC(period / 2));
	} else {
		/* If period is 0, keep LED on */
		ret = blink_on(blink_dev);
		if (ret < 0) {
			LOG_ERR("Could not turn on LED (%d)", ret);
			return ret;
		}
		k_sleep(K_MSEC(100));
	}
	
	return 0;
}
