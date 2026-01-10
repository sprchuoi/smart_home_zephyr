/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * MATTER APPLICATION TASK 
 * ============================================================================
 * 
*/
#pragma once
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>

#include <cstdint>
#include <string.h>


#include "../../thread/thread_network_manager.hpp"
#include "../../thread/network_resilience_manager.hpp"
#include "../../../ipc/ipc_core.hpp"
#include "../light_endpoint/light_endpoint.hpp"
#include "../commission/chip_config.hpp"
#include "../commission/commissioning_delegate.hpp"

#define DEFAULT_WAIT_IPC_READY_MS 5000
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
         */
        int init();
        
        /**
         * PHASE 1: Process pending Matter events (non-blocking)
         * 
         * Call this regularly from main event loop.
         * Processes attribute changes, network events, commissioning, etc.
         * @note Safe to call frequently, uses internal state machine
         */
        void dispatchEvent();
        
        /**
         * PHASE 1: Open commissioning window for Matter
         * 
         * Starts BLE advertisement for Matter commissioning
         * Duration: COMMISSIONING_WINDOW_TIMEOUT_SEC (default 600s = 10 min)
         */
        void openCommissioningWindow();
        
        /**
         * PHASE 1: Close commissioning window
         * 
         * Stops BLE advertisement and prevents new commissioners
         */
        void closeCommissioningWindow();
        
        /**
         * PHASE 1: Reset device to factory defaults
         * 
         * Clears all Matter attributes and network information
         * Persisted state in NVS is cleared
         * @note Device will reboot after reset
         */
        void factoryReset();
        
        /**
         * Get current task state
         * @return Current AppTaskState
         */
        AppTaskState getState() const { return state_; }
        
        /**
         * Check if device is commissioned
         */
        bool isCommissioned() const;
        
        /**
         * Check if device is connected to network
         */
        bool isNetworkConnected() const;
        
        /**
         * Get device uptime in milliseconds
         */
        uint32_t getUptimeSec() const;

        /**
         * Set commissioned state (internal use by callbacks)
         * 
         * @param commissioned New commissioned state
         */
        void setCommissioned(bool commissioned);

        /**
         * PHASE 2: Register callback for state changes
         */
        using StateChangeCallback = void (*)(AppTaskState);
        void setStateChangeCallback(StateChangeCallback callback);

        /**
         * PHASE 2: Get Thread network manager instance
         */
        class ThreadNetworkManager& getThreadManager();

        /**
         * PHASE 2: Get network resilience manager instance
         */
        class NetworkResilienceManager& getResilienceManager();


        static void commissioning_complete_callback(void);

        static void commissioning_timeout_handler(struct k_timer *timer);

    private:
        /// Private constructor (singleton)
        AppTask();
        
        /// Destructor
        ~AppTask() = default;
        
        /**
         * Initialization phase functions (modular init breakdown)
         */
        int initPhase0_CoreSystem();
        int initPhase1_IPC();
        int initPhase2_Endpoints(bool onoff_state, uint8_t level);
        int initPhase3_Matter(bool onoff_state, uint8_t level);
        int initPhase4_Thread();
        int initPhase5_Callbacks();
        int initPhase6_NetworkJoin();
        
        /**
         * PHASE 1: Internal state machine handler
         */
        void handleStateChange(AppTaskState new_state);
        
        /**
         * PHASE 1: Process attribute change events
         */
        void processAttributeChange();
        
        /**
         * PHASE 2: Process network connectivity events
         */
        void processNetworkEvent();

        /**
         * PHASE 2: Handle Thread state transitions
         */
        void handleThreadStateChange(smarthome::protocol::thread::ThreadState state);

        /**
         * PHASE 2: Handle network health changes
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
