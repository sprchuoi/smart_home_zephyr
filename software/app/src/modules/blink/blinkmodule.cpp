/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blinkmodule.hpp"
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(blink_module_cpp, CONFIG_APP_LOG_LEVEL);

// Define the LED GPIO spec from device tree
#if DT_NODE_EXISTS(DT_ALIAS(led0))
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#endif

BlinkModule& BlinkModule::getInstance() {
    static BlinkModule instance;
    return instance;
}

BlinkModule::BlinkModule() : period_ms_(DEFAULT_PERIOD_MS) {
    k_mutex_init(&mutex_);
}

int BlinkModule::init() {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED GPIO device not ready");
        return -ENODEV;
    }
    
    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED GPIO (%d)", ret);
        return ret;
    }
    
    LOG_INF("Blink module initialized (period: %u ms)", period_ms_);
    return 0;
#else
    LOG_WRN("LED not configured in device tree");
    return -ENOTSUP;
#endif
}

void BlinkModule::setPeriod(uint32_t period_ms) {
    k_mutex_lock(&mutex_, K_FOREVER);
    period_ms_ = period_ms;
    k_mutex_unlock(&mutex_);
    LOG_INF("Blink period updated: %u ms", period_ms);
}

void BlinkModule::tick() {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
    static bool led_state = false;
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    // Toggle LED
    led_state = !led_state;
    gpio_pin_set_dt(&led, led_state ? 1 : 0);
    
    k_mutex_unlock(&mutex_);
    
    // Sleep for the configured period
    k_sleep(K_MSEC(period_ms_));
#else
    // If no LED, just sleep
    k_sleep(K_MSEC(period_ms_));
#endif
}
