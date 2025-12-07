/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include <zephyr/kernel.h>

#define WIFI_TASK_STACK_SIZE 2048
#define WIFI_TASK_PRIORITY 5

/**
 * @brief Initialize and start the WiFi task thread
 * 
 * @return 0 on success, negative errno on failure
 */
int wifi_task_start(void);

#endif /* WIFI_TASK_H */
