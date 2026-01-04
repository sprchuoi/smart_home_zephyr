/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "button_manager.hpp"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(button_module, CONFIG_LOG_DEFAULT_LEVEL);

namespace smarthome {
namespace hw {

/*=============================================================================
 * Device Tree Button Definitions
 *===========================================================================*/

#if DT_NODE_HAS_STATUS(DT_ALIAS(sw0), okay)
#define BUTTON0_EXISTS 1
#else
#define BUTTON0_EXISTS 0
#endif

#if DT_NODE_HAS_STATUS(DT_ALIAS(sw1), okay)
#define BUTTON1_EXISTS 1
#else
#define BUTTON1_EXISTS 0
#endif

#if DT_NODE_HAS_STATUS(DT_ALIAS(sw2), okay)
#define BUTTON2_EXISTS 1
#else
#define BUTTON2_EXISTS 0
#endif

#if DT_NODE_HAS_STATUS(DT_ALIAS(sw3), okay)
#define BUTTON3_EXISTS 1
#else
#define BUTTON3_EXISTS 0
#endif

/*=============================================================================
 * Constructor
 *===========================================================================*/

ButtonManager::ButtonManager() : button_count_(0) {
    memset(buttons_, 0, sizeof(buttons_));
    k_mutex_init(&mutex_);
}

/*=============================================================================
 * Initialization
 *===========================================================================*/

int ButtonManager::init() {
    LOG_INF("Initializing button manager...");
    
    int total_buttons = 0;
    
    // Initialize button 0
#if BUTTON0_EXISTS
    buttons_[0].spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
    if (device_is_ready(buttons_[0].spec.port)) {
        int ret = gpio_pin_configure_dt(&buttons_[0].spec, GPIO_INPUT);
        if (ret == 0) {
            gpio_init_callback(&buttons_[0].cb_data, button_isr_handler, 
                             BIT(buttons_[0].spec.pin));
            gpio_add_callback(buttons_[0].spec.port, &buttons_[0].cb_data);
            gpio_pin_interrupt_configure_dt(&buttons_[0].spec, GPIO_INT_EDGE_TO_ACTIVE);
            
            buttons_[0].initialized = true;
            buttons_[0].last_press_ms = 0;
            total_buttons++;
            LOG_INF("Button 0 initialized on P0.%d", buttons_[0].spec.pin);
        }
    }
#endif

    // Initialize button 1
#if BUTTON1_EXISTS
    buttons_[1].spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
    if (device_is_ready(buttons_[1].spec.port)) {
        int ret = gpio_pin_configure_dt(&buttons_[1].spec, GPIO_INPUT);
        if (ret == 0) {
            gpio_init_callback(&buttons_[1].cb_data, button_isr_handler, 
                             BIT(buttons_[1].spec.pin));
            gpio_add_callback(buttons_[1].spec.port, &buttons_[1].cb_data);
            gpio_pin_interrupt_configure_dt(&buttons_[1].spec, GPIO_INT_EDGE_TO_ACTIVE);
            
            buttons_[1].initialized = true;
            buttons_[1].last_press_ms = 0;
            total_buttons++;
            LOG_INF("Button 1 initialized on P0.%d", buttons_[1].spec.pin);
        }
    }
#endif

    // Initialize button 2
#if BUTTON2_EXISTS
    buttons_[2].spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios);
    if (device_is_ready(buttons_[2].spec.port)) {
        int ret = gpio_pin_configure_dt(&buttons_[2].spec, GPIO_INPUT);
        if (ret == 0) {
            gpio_init_callback(&buttons_[2].cb_data, button_isr_handler, 
                             BIT(buttons_[2].spec.pin));
            gpio_add_callback(buttons_[2].spec.port, &buttons_[2].cb_data);
            gpio_pin_interrupt_configure_dt(&buttons_[2].spec, GPIO_INT_EDGE_TO_ACTIVE);
            
            buttons_[2].initialized = true;
            buttons_[2].last_press_ms = 0;
            total_buttons++;
            LOG_INF("Button 2 initialized on P0.%d", buttons_[2].spec.pin);
        }
    }
#endif

    // Initialize button 3
#if BUTTON3_EXISTS
    buttons_[3].spec = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios);
    if (device_is_ready(buttons_[3].spec.port)) {
        int ret = gpio_pin_configure_dt(&buttons_[3].spec, GPIO_INPUT);
        if (ret == 0) {
            gpio_init_callback(&buttons_[3].cb_data, button_isr_handler, 
                             BIT(buttons_[3].spec.pin));
            gpio_add_callback(buttons_[3].spec.port, &buttons_[3].cb_data);
            gpio_pin_interrupt_configure_dt(&buttons_[3].spec, GPIO_INT_EDGE_TO_ACTIVE);
            
            buttons_[3].initialized = true;
            buttons_[3].last_press_ms = 0;
            total_buttons++;
            LOG_INF("Button 3 initialized on P0.%d", buttons_[3].spec.pin);
        }
    }
#endif

    button_count_ = total_buttons;
    
    if (total_buttons == 0) {
        LOG_WRN("No buttons configured in device tree");
        return -ENODEV;
    }
    
    LOG_INF("Button manager initialized with %d button(s)", total_buttons);
    return 0;
}

/*=============================================================================
 * Callback Registration
 *===========================================================================*/

int ButtonManager::registerCallback(uint8_t button_id, ButtonCallback callback) {
    if (button_id >= MAX_BUTTONS) {
        LOG_ERR("Invalid button ID: %d", button_id);
        return -EINVAL;
    }
    
    if (!buttons_[button_id].initialized) {
        LOG_ERR("Button %d not initialized", button_id);
        return -ENODEV;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    buttons_[button_id].callback = callback;
    k_mutex_unlock(&mutex_);
    
    LOG_DBG("Callback registered for button %d", button_id);
    return 0;
}

/*=============================================================================
 * Button State Query
 *===========================================================================*/

bool ButtonManager::isPressed(uint8_t button_id) const {
    if (button_id >= MAX_BUTTONS || !buttons_[button_id].initialized) {
        return false;
    }
    
    return gpio_pin_get_dt(&buttons_[button_id].spec) != 0;
}

/*=============================================================================
 * Interrupt Handler
 *===========================================================================*/

void ButtonManager::button_isr_handler(const struct device *dev,
                                       struct gpio_callback *cb, 
                                       uint32_t pins) {
    ButtonManager& mgr = getInstance();
    
    // Find which button triggered the interrupt
    for (uint8_t i = 0; i < MAX_BUTTONS; i++) {
        if (mgr.buttons_[i].initialized && 
            mgr.buttons_[i].spec.port == dev &&
            (pins & BIT(mgr.buttons_[i].spec.pin))) {
            mgr.handleButtonPress(i);
            break;
        }
    }
}

void ButtonManager::handleButtonPress(uint8_t button_id) {
    uint32_t now = k_uptime_get_32();
    
    // Debounce check
    if (now - buttons_[button_id].last_press_ms < DEBOUNCE_MS) {
        return;
    }
    
    buttons_[button_id].last_press_ms = now;
    
    LOG_INF("Button %d pressed", button_id);
    
    // Call registered callback
    if (buttons_[button_id].callback) {
        buttons_[button_id].callback(button_id);
    }
}

}  // namespace hw
}  // namespace smarthome
