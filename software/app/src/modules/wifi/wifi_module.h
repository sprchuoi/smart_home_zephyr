/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <zephyr/kernel.h>
#include <stdint.h>

/* Configuration */
#define WIFI_SSID           "YourSSID"
#define WIFI_PSK            "YourPassword"
#define WIFI_CONNECT_TIMEOUT_MS  10000
#define WIFI_RECONNECT_INTERVAL_MS 5000

/* AP Mode Configuration */
#define WIFI_AP_SSID        "ESP32_SmartHome_AP"
#define WIFI_AP_PSK         "12345678"
#define WIFI_AP_CHANNEL     6
#define WIFI_AP_MAX_CONNECTIONS 4

/* WiFi mode */
enum wifi_mode {
	WIFI_MODE_STA,        /* Station mode (client) */
	WIFI_MODE_AP,         /* Access Point mode */
	WIFI_MODE_AP_STA      /* AP + STA mode (simultaneous) */
};

/* WiFi status */
enum wifi_status {
	WIFI_STATUS_DISCONNECTED,
	WIFI_STATUS_CONNECTING,
	WIFI_STATUS_CONNECTED,
	WIFI_STATUS_ERROR
};

/**
 * @brief WiFi event callback function type
 * 
 * @param status Current WiFi status
 */
typedef void (*wifi_event_callback_t)(enum wifi_status status);

/**
 * @brief Initialize WiFi module
 * 
 * @param mode WiFi mode (STA, AP, or AP+STA)
 * @return 0 on success, negative errno on failure
 */
int wifi_module_init(enum wifi_mode mode);

/**
 * @brief Set WiFi mode
 * 
 * @param mode WiFi mode to set
 * @return 0 on success, negative errno on failure
 */
int wifi_module_set_mode(enum wifi_mode mode);

/**
 * @brief Get current WiFi mode
 * 
 * @return Current WiFi mode
 */
enum wifi_mode wifi_module_get_mode(void);

/**
 * @brief Connect to WiFi network (STA mode)
 * 
 * @param ssid WiFi SSID (NULL to use default)
 * @param psk WiFi password (NULL to use default)
 * @return 0 on success, negative errno on failure
 */
int wifi_module_connect(const char *ssid, const char *psk);

/**
 * @brief Start AP mode
 * 
 * @param ssid AP SSID (NULL to use default)
 * @param psk AP password (NULL to use default)
 * @param channel WiFi channel (0 to use default)
 * @return 0 on success, negative errno on failure
 */
int wifi_module_start_ap(const char *ssid, const char *psk, uint8_t channel);

/**
 * @brief Disconnect from WiFi network (STA mode)
 * 
 * @return 0 on success, negative errno on failure
 */
int wifi_module_disconnect(void);

/**
 * @brief Stop AP mode
 * 
 * @return 0 on success, negative errno on failure
 */
int wifi_module_stop_ap(void);

/**
 * @brief Get current WiFi status
 * 
 * @return Current WiFi status
 */
enum wifi_status wifi_module_get_status(void);

/**
 * @brief Register callback for WiFi events
 * 
 * @param callback Callback function
 * @return 0 on success, negative errno on failure
 */
int wifi_module_register_callback(wifi_event_callback_t callback);

/**
 * @brief Get IP address
 * 
 * @param ip_str Buffer to store IP address string
 * @param len Length of buffer
 * @return 0 on success, negative errno on failure
 */
int wifi_module_get_ip(char *ip_str, size_t len);

#endif /* WIFI_MODULE_H */
