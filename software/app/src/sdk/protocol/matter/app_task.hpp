/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * MATTER APPLICATION TASK - Full Phase 1+2 Implementation
 * ============================================================================
 *
 * Phase 1: CHIP Stack Integration (25-30h)
 *   - CHIP stack initialization and configuration
 *   - Root and Light endpoint registration
 *   - OnOff and Level Control cluster implementation
 *   - BLE commissioning flow and delegates
 *   - NVS-based persistence for fabric data
 *   - Factory reset with state cleanup
 *
 * Phase 2: OpenThread Integration (30-40h)
 *   - OpenThread stack setup and configuration
 *   - Thread network join/leave with credentials
 *   - CHIP-Thread connectivity integration
 *   - Network resilience and automatic reconnection
 *   - Link quality monitoring with exponential backoff
 *   - Comprehensive network diagnostics
 *
 * Purpose:
 *   Central controller for Matter protocol stack lifecycle and events
 *   Manages initialization, commissioning, and attribute changes
 *   Coordinates with Light Endpoint for device behavior
 *   Manages Thread network connectivity and resilience
 *
 * Responsibilities:
 *   - Matter stack initialization and configuration
 *   - Commissioning window management (BLE advertisement)
 *   - Event queue processing and dispatching
 *   - Factory reset and persistent storage
 *   - Thread network join/leave event handling
 *   - Network resilience and failover
 *   - Device diagnostics and monitoring
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
 *   - All state changes protected by mutex
 */

#pragma once

#include <zephyr/kernel.h>
#include <cstdint>
#include "../thread/thread_network_manager.hpp"
#include "../thread/network_resilience_manager.hpp"

namespace smarthome { namespace protocol { namespace matter {

/**
 * AppTask state enumeration - Full state machine
     */
    enum class AppTaskState : uint8_t {
        UNINITIALIZED = 0,      // Initial state, not yet initialized
        INITIALIZING = 1,       // CHIP stack initializing
        IDLE = 2,               // Ready but not commissioned
        COMMISSIONING = 3,      // BLE commissioning window open
        COMMISSIONED = 4,       // Fabric added, awaiting network
        NETWORK_JOINING = 5,    // Attempting Thread network join
        NETWORK_CONNECTED = 6,  // Full connectivity (Thread + Matter)
        ERROR = 7               // Error state, recovery in progress
    };

    /**
     * Matter event types for queue
     */
    enum class EventType : uint8_t {
        CHIP_EVENT = 0,
        COMMISSIONING_EVENT = 1,
        ATTRIBUTE_CHANGE = 2,
        THREAD_STATE_CHANGE = 3,
        NETWORK_HEALTH_CHANGE = 4,
        FACTORY_RESET = 5,
        OTA_AVAILABLE = 6
    };

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
         * PHASE 1: Initialize Matter/CHIP stack
         * 
         * @return 0 on success, negative error code on failure
         * @note Must be called once during initialization before dispatchEvent()
         * 
         * TODO: Implement (10-12 hours)
         *  1. Initialize CHIP DeviceLayer:
         *     - Call chip::DeviceLayer::PlatformMgr().InitChipStack()
         *     - Setup platform logging
         *     - Initialize HAL (hardware abstraction layer)
         *  2. Create and register Root Endpoint (ID 0):
         *     - Basic cluster (0x0028)
         *     - Identify cluster (0x0003)
         *     - Descriptor cluster (0x001D)
         *  3. Create and register Light Endpoint (ID 1):
         *     - OnOff cluster (0x0006)
         *     - Level Control cluster (0x0008)
         *     - Color Control cluster (0x0300) - optional
         *  4. Setup attribute callbacks:
         *     - OnOff attribute changed → setLight()
         *     - Level attribute changed → setBrightness()
         *  5. Initialize event loop:
         *     - Create event queue (k_msgq)
         *     - Start event dispatch thread
         *  6. Load persisted data from NVS:
         *     - Fabric information (if commissioned)
         *     - Light on/off state
         *     - Light brightness level
         *  7. Register commissioning delegate callbacks
         *  8. Start device as Matter server
         */
        int init();
        
        /**
         * PHASE 1: Process pending Matter events (non-blocking)
         * 
         * Call this regularly from main event loop.
         * Processes attribute changes, network events, commissioning, etc.
         * @note Safe to call frequently, uses internal state machine
         * 
         * TODO: Implement (5-8 hours)
         *  1. Dequeue event from internal queue (k_msgq_get with K_NO_WAIT)
         *  2. Switch on EventType:
         *     - CHIP_EVENT: Process CHIP stack event
         *     - COMMISSIONING_EVENT: Update commissioning state
         *     - ATTRIBUTE_CHANGE: Sync with Matter stack
         *     - THREAD_STATE_CHANGE: Update network state
         *     - NETWORK_HEALTH_CHANGE: Log health status
         *     - FACTORY_RESET: Clear all data and reboot
         *  3. Call appropriate handler based on event type
         *  4. Return immediately if no events pending
         */
        void dispatchEvent();
        
        /**
         * PHASE 1: Open commissioning window for Matter
         * 
         * Starts BLE advertisement for Matter commissioning
         * Duration: COMMISSIONING_WINDOW_TIMEOUT_SEC (default 600s = 10 min)
         * 
         * TODO: Implement (4-6 hours)
         *  1. Check if already commissioned - if yes, allow re-commissioning
         *  2. Get commissioning parameters from chip_config.hpp:
         *     - Passcode (COMMISSIONABLE_PIN_CODE)
         *     - Discriminator (COMMISSIONING_DISCRIMINATOR)
         *  3. Update internal state to COMMISSIONING
         *  4. Send IPC message to NET core:
         *     - Message type: IPC_BLE_START_COMMISSIONING
         *     - Payload: discriminator, advertising interval
         *  5. Start commissioning timer (auto-close after timeout)
         *  6. Setup commissioning delegate callbacks:
         *     - onFabricAdded() → close window, start network join
         *     - onCommissioningComplete() → log success, update state
         *  7. Log commissioning window opened with PIN/QR for user
         */
        void openCommissioningWindow();
        
        /**
         * PHASE 1: Close commissioning window
         * 
         * Stops BLE advertisement and prevents new commissioners
         * 
         * TODO: Implement (2-3 hours)
         *  1. Check if currently commissioning
         *  2. Cancel commissioning timeout timer
         *  3. Send IPC message to NET core:
         *     - Message type: IPC_BLE_STOP_COMMISSIONING
         *  4. Update state:
         *     - If commissioned: state = COMMISSIONED
         *     - If not commissioned: state = IDLE
         *  5. Call CommissioningDelegate::closeCommissioningWindow()
         */
        void closeCommissioningWindow();
        
        /**
         * PHASE 1: Reset device to factory defaults
         * 
         * Clears all Matter attributes and network information
         * Persisted state in NVS is cleared
         * @note Device will reboot after reset
         * 
         * TODO: Implement (3-4 hours)
         *  1. Update state to ERROR (prevent other operations)
         *  2. Close commissioning window if open
         *  3. Leave Thread network (via ThreadNetworkManager)
         *  4. Clear NVS data:
         *     - Delete fabric data (MATTER_NVS_NAMESPACE)
         *     - Delete credentials
         *     - Delete attribute values
         *  5. Reset Light endpoint to defaults
         *  6. Call CommissioningDelegate::onFabricRemoved()
         *  7. Delay 500ms to ensure NVS writes complete
         *  8. Trigger system reboot via k_sys_reboot(K_REBOOT_COLD)
         */
        void factoryReset();
        
        /**
         * Get current task state
         * @return Current AppTaskState
         */
        AppTaskState getState() const { return state_; }
        
        /**
         * Check if device is commissioned
         * 
         * TODO: Implement (1 hour)
         *  1. Query CHIP fabric table
         *  2. Return true if any fabric exists
         *  3. Cache result in commissioned_ for quick access
         */
        bool isCommissioned() const;
        
        /**
         * Check if device is connected to network
         * 
         * TODO: Implement (1 hour)
         *  1. Query ThreadNetworkManager state
         *  2. Return true if attached to Thread
         */
        bool isNetworkConnected() const;
        
        /**
         * Get device uptime in milliseconds
         * 
         * TODO: Implement (1 hour)
         *  1. Call k_uptime_get_32()
         */
        uint32_t getUptimeSec() const;

        /**
         * PHASE 2: Register callback for state changes
         * 
         * TODO: Implement (1 hour)
         */
        using StateChangeCallback = void (*)(AppTaskState);
        void setStateChangeCallback(StateChangeCallback callback);

        /**
         * PHASE 2: Get Thread network manager instance
         * 
         * TODO: Implement (1 hour)
         */
        class ThreadNetworkManager& getThreadManager();

        /**
         * PHASE 2: Get network resilience manager instance
         * 
         * TODO: Implement (1 hour)
         */
        class NetworkResilienceManager& getResilienceManager();

    private:
        /// Private constructor (singleton)
        AppTask();
        
        /// Destructor
        ~AppTask() = default;
        
        /**
         * PHASE 1: Internal state machine handler
         * 
         * TODO: Implement (3-4 hours)
         *  - UNINITIALIZED → INITIALIZING: start CHIP init
         *  - INITIALIZING → IDLE: CHIP ready, not commissioned
         *  - IDLE → COMMISSIONING: open commissioning window
         *  - COMMISSIONING → COMMISSIONED: fabric added
         *  - COMMISSIONED → NETWORK_JOINING: attempt Thread join
         *  - NETWORK_JOINING → NETWORK_CONNECTED: Thread attached
         *  - Any state → ERROR: on error, recovery timer scheduled
         *  - ERROR → IDLE/COMMISSIONED: after recovery timeout
         */
        void handleStateChange(AppTaskState new_state);
        
        /**
         * PHASE 1: Process attribute change events
         * 
         * TODO: Implement (3-4 hours)
         *  - OnOff cluster attribute (0x0000) change:
         *    1. Get new state from CHIP stack
         *    2. Call LightEndpoint::setOnOff(state)
         *    3. Update physical LED
         *    4. Save to NVS
         *  - Level Control cluster attribute change:
         *    1. Get new level from CHIP stack
         *    2. Call LightEndpoint::setLevel(level)
         *    3. Update LED PWM
         *    4. Save to NVS
         *  - Trigger attribute change callbacks registered by Matter controllers
         */
        void processAttributeChange();
        
        /**
         * PHASE 2: Process network connectivity events
         * 
         * TODO: Implement (5-6 hours)
         *  - Thread attachment event:
         *    1. Update state to NETWORK_CONNECTED
         *    2. Notify connectivity to CHIP stack
         *    3. Start network diagnostics collection
         *    4. Update NetworkResilienceManager
         *  - Thread detachment event:
         *    1. Update state to COMMISSIONED (waiting for rejoin)
         *    2. Start NetworkResilienceManager rejoin logic
         *    3. Setup exponential backoff retry timer
         *  - Link quality degradation:
         *    1. Monitor RSSI via NetworkResilienceManager
         *    2. Trigger proactive scan if quality poor
         *  - Commissioning completion:
         *    1. Received fabric from commissioner
         *    2. Save fabric to NVS
         *    3. Trigger Thread network join attempt
         */
        void processNetworkEvent();

        /**
         * PHASE 2: Handle Thread state transitions
         * 
         * TODO: Implement (4-5 hours)
         */
        void handleThreadStateChange(smarthome::protocol::thread::ThreadState state);

        /**
         * PHASE 2: Handle network health changes
         * 
         * TODO: Implement (3-4 hours)
         */
        void handleNetworkHealthChange(smarthome::protocol::thread::NetworkHealth health);

        /*=== State & Configuration ===*/
        AppTaskState state_ = AppTaskState::UNINITIALIZED;
        bool commissioned_ = false;
        bool network_connected_ = false;
        uint32_t init_time_ms_ = 0;
        
        // Callbacks
        StateChangeCallback state_change_callback_ = nullptr;
        
        // Synchronization
        struct k_mutex state_mutex_;
        
        // Event queue
        struct k_msgq event_queue_;
        k_tid_t event_thread_;
    };

}  // namespace matter
}  // namespace protocol
}  // namespace smarthome
