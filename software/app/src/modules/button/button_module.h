/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BUTTON_MODULE_H
#define BUTTON_MODULE_H

#include <zephyr/kernel.h>
#include <stdbool.h>

/* Configuration */
#define BUTTON_DEBOUNCE_MS 50

/**
 * @brief Button callback function type
 * 
 * @param pressed True if button pressed, false if released
 */
typedef void (*button_callback_t)(bool pressed);

/**
 * @brief Initialize button module
 * 
 * @return 0 on success, negative errno on failure
 */
int button_module_init(void);

/**
 * @brief Register callback for button events
 * 
 * @param callback Callback function
 * @return 0 on success, negative errno on failure
 */
int button_module_register_callback(button_callback_t callback);

/**
 * @brief Get button state
 * 
 * @return true if pressed, false otherwise
 */
bool button_module_is_pressed(void);

#endif /* BUTTON_MODULE_H */
