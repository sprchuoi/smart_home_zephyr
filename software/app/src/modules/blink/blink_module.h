/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLINK_MODULE_H
#define BLINK_MODULE_H

#include <zephyr/kernel.h>

/* Configuration */
#define BLINK_PERIOD_MS_MAX  1000U
#define BLINK_PERIOD_MS_MIN  0U

/**
 * @brief Initialize blink module
 * 
 * @return 0 on success, negative errno on failure
 */
int blink_module_init(void);

/**
 * @brief Set blink period
 * 
 * @param period_ms Period in milliseconds
 * @return 0 on success, negative errno on failure
 */
int blink_module_set_period(unsigned int period_ms);

/**
 * @brief Get current blink period
 * 
 * @return Current period in milliseconds
 */
unsigned int blink_module_get_period(void);

/**
 * @brief Blink LED once (on/off cycle)
 * 
 * @return 0 on success, negative errno on failure
 */
int blink_module_tick(void);

#endif /* BLINK_MODULE_H */
