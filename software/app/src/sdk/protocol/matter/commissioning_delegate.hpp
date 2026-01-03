/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * Commissioning Delegate - Handles Matter commissioning flow
 */

#pragma once

#include <cstdint>
#include <zephyr/kernel.h>

namespace smarthome { namespace protocol { namespace matter {

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
     * 
     * TODO:
     *  1. Setup commissioning window state management
     *  2. Initialize BLE commissioning service
     *  3. Register commissioning callbacks with CHIP stack
     *  4. Setup commissioning timeout handler
     *  5. Load discriminator and passcode from factory data
     */
    int init();

    /**
     * Open commissioning window for Matter
     * 
     * @param timeout_sec Duration to keep commissioning open (seconds)
     * @return 0 on success
     * 
     * TODO:
     *  1. Check if already commissioned
     *  2. Start BLE advertisement with commissioning service
     *  3. Setup discriminator in BLE advertisement
     *  4. Setup commissioning timer (auto-close after timeout)
     *  5. Update device state to COMMISSIONING
     *  6. Log commissioning window opening with discriminator/PIN
     */
    int openCommissioningWindow(uint32_t timeout_sec);

    /**
     * Close commissioning window
     * 
     * @return 0 on success
     * 
     * TODO:
     *  1. Stop BLE advertisement
     *  2. Cancel commissioning timeout timer
     *  3. Update device state based on commissioned status
     *  4. Log commissioning window closure
     */
    int closeCommissioningWindow();

    /**
     * Check if commissioning window is currently open
     * 
     * @return true if open, false if closed
     */
    bool isCommissioningOpen() const;

    /**
     * Callback when fabric is added (device commissioned)
     * 
     * TODO:
     *  1. Called by CHIP stack when fabric add succeeds
     *  2. Save fabric info to NVS
     *  3. Close commissioning window if open
     *  4. Update device state to COMMISSIONED
     *  5. Start attempting network join
     */
    void onFabricAdded();

    /**
     * Callback when fabric is removed (factory reset)
     * 
     * TODO:
     *  1. Called when all fabrics are removed
     *  2. Clear stored fabric data
     *  3. Update state to not commissioned
     *  4. Close commissioning window
     *  5. Trigger device cleanup
     */
    void onFabricRemoved();

    /**
     * Callback for commissioning completion
     * 
     * TODO:
     *  1. Called when Matter commissioning succeeds
     *  2. Save operational credentials to NVS
     *  3. Setup network join attempt
     *  4. Log successful commissioning
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
};

}  // namespace matter
}  // namespace protocol
}  // namespace smarthome
