/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sensor_task.h"
#include "../modules/sensor/sensor_module.h"
#include "../modules/blink/blink_module.h"

LOG_MODULE_REGISTER(sensor_task, CONFIG_APP_LOG_LEVEL);

/* Thread stack and control block */
K_THREAD_STACK_DEFINE(sensor_task_stack, SENSOR_TASK_STACK_SIZE);
static struct k_thread sensor_task_data;

/* Sensor event handler */
static void sensor_event_handler(bool proximity_detected)
{
	if (proximity_detected) {
		unsigned int current_period = blink_module_get_period();
		unsigned int new_period;
		
		if (current_period == 0U) {
			new_period = BLINK_PERIOD_MS_MAX;
		} else {
			new_period = current_period - BLINK_PERIOD_STEP_MS;
		}
		
		blink_module_set_period(new_period);
		printk("Proximity detected, setting LED period to %u ms\n", new_period);
	}
}

/* Sensor task thread entry */
static void sensor_task_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_INF("Sensor task started");

	/* Register callback */
	sensor_module_register_callback(sensor_event_handler);

	while (1) {
		sensor_module_read();
		k_sleep(K_MSEC(SENSOR_READ_INTERVAL_MS));
	}
}

int sensor_task_start(void)
{
	/* Create sensor task thread */
	k_thread_create(&sensor_task_data, sensor_task_stack,
			K_THREAD_STACK_SIZEOF(sensor_task_stack),
			sensor_task_entry,
			NULL, NULL, NULL,
			SENSOR_TASK_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&sensor_task_data, "sensor_task");

	LOG_INF("Sensor task thread created");
	return 0;
}
