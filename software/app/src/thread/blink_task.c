/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "blink_task.h"
#include "../modules/blink/blink_module.h"

LOG_MODULE_REGISTER(blink_task, CONFIG_APP_LOG_LEVEL);

/* Thread stack and control block */
K_THREAD_STACK_DEFINE(blink_task_stack, BLINK_TASK_STACK_SIZE);
static struct k_thread blink_task_data;

/* Blink task thread entry */
static void blink_task_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_INF("Blink task started");

	while (1) {
		blink_module_tick();
	}
}

int blink_task_start(void)
{
	/* Create blink task thread */
	k_thread_create(&blink_task_data, blink_task_stack,
			K_THREAD_STACK_SIZEOF(blink_task_stack),
			blink_task_entry,
			NULL, NULL, NULL,
			BLINK_TASK_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&blink_task_data, "blink_task");

	LOG_INF("Blink task thread created");
	return 0;
}
