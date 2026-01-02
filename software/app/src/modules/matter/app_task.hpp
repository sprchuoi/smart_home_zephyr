/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * MATTER APPLICATION TASK - Stack Management & Event Processing
 * ============================================================================
 *
 * Purpose:
 *   Central controller for Matter protocol stack lifecycle and events
 *   Manages initialization, commissioning, and attribute changes
 *   Coordinates with Light Endpoint for device behavior
 *
 * Responsibilities:
 *   - Matter stack initialization and configuration
 *   - Commissioning window management (BLE advertisement)
 *   - Event queue processing and dispatching
 *   - Factory reset and persistent storage
 *   - Network join/leave event handling
 *
 * Usage Example:
 *   AppTask& task = Matter::AppTask::getInstance();
 *   task.init();
 *   while (1) {
 *     task.dispatchEvent();
 *     k_sleep(K_MSEC(10));
 *   }
 *
 * Thread Safety:
 *   - Singleton pattern ensures single instance
 *   - Non-blocking dispatch safe for event loops
 */

#pragma once

namespace Matter {

class AppTask {
public:
    /// Get singleton instance
    static AppTask& getInstance() {
        static AppTask instance;
        return instance;
    }
    
    // Prevent copying
    AppTask(const AppTask&) = delete;
    AppTask& operator=(const AppTask&) = delete;
    
    /**
     * Initialize Matter application task
     * 
     * @return 0 on success, negative error code on failure
     * @note Must be called once during initialization before dispatchEvent()
     */
    int init();
    
    /**
     * Process pending Matter events (non-blocking)
     * 
     * Call this regularly from main event loop.
     * Processes attribute changes, network events, commissioning, etc.
     * @note Safe to call frequently, uses internal state machine
     */
    void dispatchEvent();
    
    /**
     * Open commissioning window for new devices
     * 
     * Starts BLE advertisement for Matter commissioning
     * Duration: 15 minutes (configurable)
     */
    void openCommissioningWindow();
    
    /**
     * Reset device to factory defaults
     * 
     * Clears all Matter attributes and network information
     * Persisted state in NVS is cleared
     * @note Device will reboot after reset
     */
    void factoryReset();

private:
    /// Private constructor (singleton)
    AppTask() = default;
    
    /// Destructor
    ~AppTask() = default;
};

}  // namespace Matter
