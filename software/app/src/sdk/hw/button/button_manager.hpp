/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * BUTTON MANAGER - Multi-Button GPIO Input with Debouncing
 * ============================================================================
 *
 * Purpose:
 *   Hardware abstraction for button inputs on nRF5340 DK
 *   Provides interrupt-driven input with software debouncing
 *   Decouples button logic from Matter/application layer
 *
 * Hardware:
 *   Buttons: P0.23-26 (GPIO input, active-low, pull-up)
 *   Interrupt: Edge trigger on button press
 *
 * Features:
 *   - Support for up to 4 buttons (configurable)
 *   - Interrupt-driven (no polling overhead)
 *   - 100ms debounce to prevent electrical glitches
 *   - Callback pattern for event notification
 *   - Thread-safe for concurrent access
 */

#ifndef HW_BUTTON_MANAGER_HPP
#define HW_BUTTON_MANAGER_HPP

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

namespace smarthome {
namespace hw {

/// Maximum number of buttons supported
constexpr uint8_t MAX_BUTTONS = 4;

/// Debounce time in milliseconds
constexpr uint32_t DEBOUNCE_MS = 100;

/**
 * @brief Button event handler callback function pointer
 * @param button_id Button index (0-3)
 */
using ButtonCallback = void (*)(uint8_t button_id);

/**
 * @brief Multi-Button Manager Class
 * 
 * Manages multiple hardware buttons with debouncing and callbacks.
 * Singleton pattern ensures single hardware resource owner.
 */
class ButtonManager {
public:
    /**
     * @brief Get singleton instance
     */
    static ButtonManager& getInstance() {
        static ButtonManager instance;
        return instance;
    }
    
    /**
     * @brief Initialize all buttons from device tree
     * @return 0 on success, negative error code on failure
     */
    int init();
    
    /**
     * @brief Register callback for button events
     * @param button_id Button index (0-3)
     * @param callback Function pointer to call on button press
     * @return 0 on success, negative error code on failure
     */
    int registerCallback(uint8_t button_id, ButtonCallback callback);
    
    /**
     * @brief Get number of initialized buttons
     */
    uint8_t getButtonCount() const { return button_count_; }
    
    /**
     * @brief Check if button is currently pressed
     * @param button_id Button index (0-3)
     * @return true if pressed, false otherwise
     */
    bool isPressed(uint8_t button_id) const;

private:
    /// Private constructor (singleton)
    ButtonManager();
    
    /// Destructor
    ~ButtonManager() = default;
    
    // Prevent copying
    ButtonManager(const ButtonManager&) = delete;
    ButtonManager& operator=(const ButtonManager&) = delete;
    
    /**
     * @brief GPIO interrupt handler (static)
     */
    static void button_isr_handler(const struct device *dev,
                                   struct gpio_callback *cb, 
                                   uint32_t pins);
    
    /**
     * @brief Process button press with debouncing
     */
    void handleButtonPress(uint8_t button_id);
    
    // Button data structure
    struct ButtonData {
        struct gpio_dt_spec spec;
        struct gpio_callback cb_data;
        ButtonCallback callback;
        uint32_t last_press_ms;
        bool initialized;
    };
    
    ButtonData buttons_[MAX_BUTTONS];
    uint8_t button_count_;
    struct k_mutex mutex_;
};

}  // namespace hw
}  // namespace smarthome

#endif  // HW_BUTTON_MANAGER_HPP
