/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BUTTON_MODULE_HPP
#define BUTTON_MODULE_HPP

#include "core/Module.hpp"
#include <zephyr/drivers/gpio.h>
#include <functional>

class ButtonModule : public Module {
public:
    using ButtonCallback = std::function<void()>;
    
    static ButtonModule& getInstance();
    
    int init() override;
    const char* getName() const override { return "ButtonModule"; }
    
    void setCallback(ButtonCallback callback) { callback_ = callback; }
    
private:
    ButtonModule();
    ~ButtonModule() override = default;
    
    ButtonModule(const ButtonModule&) = delete;
    ButtonModule& operator=(const ButtonModule&) = delete;
    
    static void button_pressed_handler(const struct device *dev,
                                      struct gpio_callback *cb, uint32_t pins);
    
    ButtonCallback callback_;
    struct gpio_callback button_cb_data_;
};

#endif // BUTTON_MODULE_HPP
