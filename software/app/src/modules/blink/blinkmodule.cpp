/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "BlinkModule.hpp"
#include <app/drivers/blink.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(blink_module_cpp, CONFIG_APP_LOG_LEVEL);

#if DT_NODE_EXISTS(DT_NODELABEL(blink_led))
#define BLINK_DEV DEVICE_DT_GET(DT_NODELABEL(blink_led))
#endif

BlinkModule& BlinkModule::getInstance() {
    static BlinkModule instance;
    return instance;
}

BlinkModule::BlinkModule() : period_ms_(DEFAULT_PERIOD_MS) {
    k_mutex_init(&mutex_);
}

int BlinkModule::init() {
#if DT_NODE_EXISTS(DT_NODELABEL(blink_led))
    const struct device *blink_dev = BLINK_DEV;
    
    if (!device_is_ready(blink_dev)) {
        LOG_ERR("Blink device not ready");
        return -ENODEV;
    }
    
    LOG_INF("Blink module initialized (period: %u ms)", period_ms_);
    return 0;
#else
    LOG_WRN("Blink LED not configured in device tree");
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
#if DT_NODE_EXISTS(DT_NODELABEL(blink_led))
    const struct device *blink_dev = BLINK_DEV;
    
    if (device_is_ready(blink_dev)) {
        blink_led_on(blink_dev);
        k_sleep(K_MSEC(50));
        blink_led_off(blink_dev);
    }
#endif
}
