/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "sensor_module.h"

LOG_MODULE_REGISTER(sensor_module, CONFIG_APP_LOG_LEVEL);

static const struct device *sensor_dev;
static struct sensor_value last_val = { 0 };
static sensor_callback_t event_callback = NULL;

int sensor_module_init(void)
{
	sensor_dev = DEVICE_DT_GET(DT_NODELABEL(example_sensor));
	if (!device_is_ready(sensor_dev)) {
		LOG_ERR("Sensor device not ready");
		return -ENODEV;
	}

	LOG_INF("Sensor module initialized");
	return 0;
}

int sensor_module_register_callback(sensor_callback_t callback)
{
	if (!callback) {
		return -EINVAL;
	}
	
	event_callback = callback;
	LOG_INF("Sensor callback registered");
	return 0;
}

int sensor_module_read(void)
{
	int ret;
	struct sensor_value val;

	ret = sensor_sample_fetch(sensor_dev);
	if (ret < 0) {
		LOG_ERR("Could not fetch sample (%d)", ret);
		return ret;
	}

	ret = sensor_channel_get(sensor_dev, SENSOR_CHAN_PROX, &val);
	if (ret < 0) {
		LOG_ERR("Could not get sample (%d)", ret);
		return ret;
	}

	/* Detect rising edge (proximity detected) */
	if ((last_val.val1 == 0) && (val.val1 == 1)) {
		LOG_INF("Proximity detected");
		
		if (event_callback) {
			event_callback(true);
		}
	}

	last_val = val;
	return 0;
}
