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
/* Configuration */
#define SENSOR_PERIOD_MS_MAX  1000U
#define SENSOR_PERIOD_MS_MIN  0U
#define SENSOR_READ_INTERVAL_MS 100U
#define SENSOR_PERIOD_STEP_MS    100U
#define SENSOR_SAMPLE_PERIOD_MS 100U

/**
 * @brief Initialize and start the sensor task thread
 * 
 * @return 0 on success, negative errno on failure
 */
int sensor_task_start(void);

#endif /* SENSOR_TASK_H */
