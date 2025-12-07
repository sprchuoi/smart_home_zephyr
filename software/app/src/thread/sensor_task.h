/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <zephyr/kernel.h>

#define SENSOR_TASK_STACK_SIZE 1024
#define SENSOR_TASK_PRIORITY 5

/**
 * @brief Initialize and start the sensor task thread
 * 
 * @return 0 on success, negative errno on failure
 */
int sensor_task_start(void);

#endif /* SENSOR_TASK_H */
