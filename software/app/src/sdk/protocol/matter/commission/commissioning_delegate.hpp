/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Commissioning Delegate - Handles Matter commissioning flow
 */

#pragma once

#include <cstdint>
#include <zephyr/kernel.h>

namespace smarthome { namespace protocol { namespace matter {

/// Callback type for commissioning completion
using CommissioningCompleteCallback = void (*)(void);

/**
 * Commissioning Delegate - BLE Commissioning Management
 * 
 * Handles:
 *  - Commissioning window open/close
 *  - BLE advertisement during commissioning
 *  - Passcode and discriminator validation
 *  - Commissioning state callbacks
 */
class CommissioningDelegate {
public:
    /// Singleton instance getter
    static CommissioningDelegate& getInstance();

    /// Delete copy/move constructors
    CommissioningDelegate(const CommissioningDelegate&) = delete;
    CommissioningDelegate& operator=(const CommissioningDelegate&) = delete;
    CommissioningDelegate(CommissioningDelegate&&) = delete;
    CommissioningDelegate& operator=(CommissioningDelegate&&) = delete;

    /**
     * Initialize commissioning delegate
     */
    int init();

    /**
     * Open commissioning window for Matter
     * 
     * @param timeout_sec Duration to keep commissioning open (seconds)
     * @return 0 on success
     */
    int openCommissioningWindow(uint32_t timeout_sec);

    /**
     * Close commissioning window
     * 
     * @return 0 on success
     */
    int closeCommissioningWindow();

    /**
     * Check if commissioning window is currently open
     * 
     * @return true if open, false if closed
     */
    bool isCommissioningOpen() const;

    /**
     * Check if device is commissioned (has fabric)
     * 
     * @return true if commissioned, false otherwise
     */
    bool isCommissioned() const;

    /**
     * Callback when fabric is added (device commissioned)
     */
    void onFabricAdded();

    /**
     * Callback when fabric is removed (factory reset)
     */
    void onFabricRemoved();

    /**
     * Callback for commissioning completion
     */
    void onCommissioningComplete();

    /**
     * Get current passcode for display/logging
     * 
     * @return Passcode (8-digit number)
     */
    uint32_t getPasscode() const { return commissioning_passcode_; }

    /**
     * Get current discriminator for QR code
     * 
     * @return Discriminator (12-bit value: 0-4095)
     */
    uint16_t getDiscriminator() const { return commissioning_discriminator_; }

    /**
     * Get commissioning window remaining time
     * 
     * @return Seconds remaining, 0 if window closed
     */
    uint32_t getTimeRemaining() const;

    /**
     * Set callback for commissioning completion
     * 
     * @param callback Function to call when commissioning completes
     */
    void setOnCommissioningComplete(CommissioningCompleteCallback callback) {
        completion_callback_ = callback;
    }

private:
    CommissioningDelegate();
    ~CommissioningDelegate();

    // Commissioning state
    bool commissioning_open_ = false;
    uint32_t commissioning_start_time_ = 0;
    uint32_t commissioning_timeout_sec_ = 0;

    // Commissioning parameters
    uint32_t commissioning_passcode_ = 0;
    uint16_t commissioning_discriminator_ = 0;

    // Timeout timer
    struct k_timer commissioning_timer_;
    
    // Completion callback
    CommissioningCompleteCallback completion_callback_;
};

}  // namespace matter
}  // namespace protocol
}  // namespace smarthome
