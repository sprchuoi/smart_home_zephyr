/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

/* Configuration */
#define SENSOR_READ_INTERVAL_MS 100U
#define BLINK_PERIOD_STEP_MS    100U

/**
 * @brief Sensor callback function type
 * 
 * @param proximity_detected True if proximity detected
 */
typedef void (*sensor_callback_t)(bool proximity_detected);

/**
 * @brief Initialize sensor module
 * 
 * @return 0 on success, negative errno on failure
 */
int sensor_module_init(void);

/**
 * @brief Register callback for sensor events
 * 
 * @param callback Callback function to call on proximity detection
 * @return 0 on success, negative errno on failure
 */
int sensor_module_register_callback(sensor_callback_t callback);

/**
 * @brief Read sensor and process data
 * 
 * @return 0 on success, negative errno on failure
 */
int sensor_module_read(void);

#endif /* SENSOR_MODULE_H */
