/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <zephyr/kernel.h>
#include <stdint.h>

/* Configuration */
#define DISPLAY_SLEEP_TIMEOUT_MS    30000  /* 30 seconds of inactivity */
#define DISPLAY_BRIGHTNESS_ON       0xFF
#define DISPLAY_BRIGHTNESS_OFF      0x00

/* Display state */
enum display_state {
	DISPLAY_STATE_OFF,
	DISPLAY_STATE_ON,
	DISPLAY_STATE_SLEEP
};

/**
 * @brief Initialize display module
 * 
 * @return 0 on success, negative errno on failure
 */
int display_module_init(void);

/**
 * @brief Turn display on
 * 
 * @return 0 on success, negative errno on failure
 */
int display_module_on(void);

/**
 * @brief Turn display off (sleep mode)
 * 
 * @return 0 on success, negative errno on failure
 */
int display_module_off(void);

/**
 * @brief Clear display
 * 
 * @return 0 on success, negative errno on failure
 */
int display_module_clear(void);

/**
 * @brief Print text on display
 * 
 * @param line Line number (0-7 for 128x64 display)
 * @param text Text to display
 * @return 0 on success, negative errno on failure
 */
int display_module_print(uint8_t line, const char *text);

/**
 * @brief Wake up display (reset sleep timer)
 * 
 * @return 0 on success, negative errno on failure
 */
int display_module_wake(void);

/**
 * @brief Get current display state
 * 
 * @return Current display state
 */
enum display_state display_module_get_state(void);

/**
 * @brief Update display with system info
 * 
 * @param wifi_status WiFi connection status
 * @param ble_status BLE connection status
 * @param ip_addr IP address string
 * @return 0 on success, negative errno on failure
 */
int display_module_update_status(const char *wifi_status, const char *ble_status, 
                                  const char *ip_addr);

#endif /* DISPLAY_MODULE_H */
