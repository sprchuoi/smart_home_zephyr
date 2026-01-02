/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * BUTTON MODULE - GPIO Input Handling with Debouncing
 * ============================================================================
 *
 * Purpose:
 *   Hardware abstraction for button input on nRF5340 DK
 *   Provides interrupt-driven input with software debouncing
 *   Decouples button logic from Matter/application layer
 *
 * Hardware:
 *   Pin: P0.23 (GPIO input, active-low, pull-up)
 *   Interrupt: Edge trigger on button press
 *
 * Features:
 *   - Interrupt-driven (no polling overhead)
 *   - 100ms debounce to prevent electrical glitches
 *   - Callback pattern for event notification
 *   - Thread-safe for concurrent access
 */

#ifndef BUTTON_MODULE_HPP
#define BUTTON_MODULE_HPP

#include <zephyr/drivers/gpio.h>

class ButtonModule {
public:
    /// Callback function type for button press events
    using ButtonCallback = void (*)();
    
    /// Get singleton instance
    static ButtonModule& getInstance() {
        static ButtonModule instance;
        return instance;
    }
    
    /**
     * Initialize GPIO for button input
     * @return 0 on success, negative error code on failure
     */
    int init();
    
    /**
     * Set callback function for button press events
     * @param callback Function to call on button press
     */
    void setCallback(ButtonCallback callback) { callback_ = callback; }

private:
    /// Private constructor (singleton)
    ButtonModule();
    
    /// Destructor
    ~ButtonModule() = default;
    
    // Prevent copying
    ButtonModule(const ButtonModule&) = delete;
    ButtonModule& operator=(const ButtonModule&) = delete;
    
    /// GPIO interrupt handler
    static void button_pressed_handler(const struct device *dev,
                                      struct gpio_callback *cb, uint32_t pins);
    
    // Members
    ButtonCallback callback_;              ///< Callback on press
    struct gpio_callback button_cb_data_;  ///< GPIO callback struct
};

#endif // BUTTON_MODULE_HPP
