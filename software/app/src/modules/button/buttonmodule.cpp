/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "buttonmodule.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(button_module_cpp, CONFIG_APP_LOG_LEVEL);

#if DT_NODE_HAS_STATUS(DT_ALIAS(sw0), okay)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
#endif

ButtonModule& ButtonModule::getInstance() {
    static ButtonModule instance;
    return instance;
}

ButtonModule::ButtonModule() : callback_(nullptr) {
}

int ButtonModule::init() {
#if DT_NODE_HAS_STATUS(DT_ALIAS(sw0), okay)
    if (!device_is_ready(button.port)) {
        LOG_ERR("Button device not ready");
        return -ENODEV;
    }
    
    int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret) {
        LOG_ERR("Failed to configure button (%d)", ret);
        return ret;
    }
    
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret) {
        LOG_ERR("Failed to configure button interrupt (%d)", ret);
        return ret;
    }
    
    gpio_init_callback(&button_cb_data_, button_pressed_handler, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data_);
    
    LOG_INF("Button module initialized");
    return 0;
#else
    LOG_WRN("Button not configured in device tree");
    return -ENOTSUP;
#endif
}

void ButtonModule::button_pressed_handler(const struct device *dev,
                                         struct gpio_callback *cb, uint32_t pins) {
#if DT_NODE_HAS_STATUS(DT_ALIAS(sw0), okay)
    ButtonModule& instance = getInstance();
    
    LOG_INF("Button pressed");
    
    if (instance.callback_) {
        instance.callback_();
    }
#endif
}
