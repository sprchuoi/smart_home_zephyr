/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Thread Network Manager - OpenThread Integration
 */

#pragma once

#include <cstdint>
#include <zephyr/kernel.h>

namespace smarthome { namespace protocol { namespace thread {

/**
 * Thread Network Manager State Machine
 */
enum class ThreadState : uint8_t {
    DISABLED = 0,           ///< Thread disabled
    INITIALIZING = 1,       ///< OpenThread stack initializing
    IDLE = 2,               ///< Thread enabled but not attached
    JOINING = 3,            ///< Attempting to join Thread network
    CHILD = 4,              ///< Attached as Thread child
    ROUTER = 5,             ///< Attached as Thread router
    LEADER = 6,             ///< Attached as Thread leader
    DETACHING = 7,          ///< Detaching from Thread network
    ERROR = 8               ///< Error state
};

/**
 * Thread Network Manager - OpenThread Stack Management
 * 
 * Responsibilities:
 *  - OpenThread stack initialization
 *  - Thread network join/leave
 *  - Network diagnostics collection
 *  - Link quality monitoring
 *  - Resilience and reconnection
 */
class ThreadNetworkManager {
public:
    /// Singleton instance getter
    static ThreadNetworkManager& getInstance();

    /// Delete copy/move constructors
    ThreadNetworkManager(const ThreadNetworkManager&) = delete;
    ThreadNetworkManager& operator=(const ThreadNetworkManager&) = delete;
    ThreadNetworkManager(ThreadNetworkManager&&) = delete;
    ThreadNetworkManager& operator=(ThreadNetworkManager&&) = delete;

    /**
     * Initialize OpenThread stack
     * 
     * TODO:
     *  1. Configure OpenThread - set channel, pan ID, network name
     *  2. Initialize radio subsystem (via RadioManager)
     *  3. Setup OpenThread callbacks:
     *     - Link state change callback
     *     - Role change callback (child/router/leader)
     *     - Network data callback
     *  4. Create worker thread for monitoring
     *  5. Register with CHIP stack connectivity manager
     *  6. Setup network diagnostics collector
     */
    int init();

    /**
     * Start Thread network join process
     * 
     * @return 0 on success
     * 
     * TODO:
     *  1. Check if already attached to Thread
     *  2. Load Thread network credentials from NVS (if available)
     *  3. If credentials exist:
     *     - Restore network config
     *     - Attempt attach with stored credentials
     *  4. If no credentials:
     *     - Wait for CHIP stack to provide credentials (post-commissioning)
     *  5. Setup join timeout (fail if not attached in 60 seconds)
     *  6. Update state to JOINING
     *  7. Start link quality monitoring
     */
    int startNetworkJoin();

    /**
     * Leave Thread network
     * 
     * @return 0 on success
     * 
     * TODO:
     *  1. Stop any pending join attempts
     *  2. Detach from Thread network
     *  3. Clear network state
     *  4. Update state to IDLE
     *  5. Cancel monitoring timers
     */
    int leaveNetwork();

    /**
     * Register callback for state changes
     * 
     * @param callback Function to call on Thread state change
     *                 Parameter: new ThreadState
     */
    using StateChangeCallback = void (*)(ThreadState);
    void setStateCallback(StateChangeCallback callback);

    /**
     * Get current Thread state
     */
    ThreadState getState() const { return current_state_; }

    /**
     * Get Thread role name (for logging)
     */
    const char* getStateName() const;

    /**
     * Check if attached to Thread network
     */
    bool isAttached() const {
        return current_state_ == ThreadState::CHILD ||
               current_state_ == ThreadState::ROUTER ||
               current_state_ == ThreadState::LEADER;
    }

    /**
     * Get link quality indicator (RSSI)
     * 
     * @return Average RSSI in dBm
     * 
     * TODO:
     *  1. Query OpenThread for current RSSI
     *  2. Return average link quality metric
     */
    int8_t getLinkQuality() const;

    /**
     * Get network diagnostics
     * 
     * Contains:
     *  - Extended address (64-bit unique ID)
     *  - Short address
     *  - Leader data (leader ID, partition ID, version)
     *  - Neighbor table
     *  - Route table
     *  - Security material version
     * 
     * TODO:
     *  1. Query OpenThread diagnostics
     *  2. Format and return diagnostic info
     *  3. Include link quality, neighbor count, leader info
     */
    void getNetworkDiagnostics();

    /**
     * Attempt network rejoin with exponential backoff
     * 
     * TODO:
     *  1. Check if link is down
     *  2. Calculate backoff delay based on attempt count
     *  3. Schedule rejoin attempt
     *  4. Cap attempts at MAX_RECONNECT_ATTEMPTS
     */
    int scheduleNetworkRejoin();

    /**
     * Force Thread network scan
     * 
     * Scans for active Thread networks and attempts join if credentials available
     * 
     * TODO:
     *  1. Trigger OpenThread network scan
     *  2. Analyze scan results
     *  3. Filter networks by stored PAN ID
     *  4. Rank by signal strength
     *  5. Attempt join on best match
     */
    int scanAndJoin();

    /**
     * Set Thread TX power
     * 
     * @param power_dbm TX power in dBm (-20 to +20)
     */
    void setTxPower(int8_t power_dbm);

    /**
     * Get OpenThread version string
     */
    const char* getThreadVersion() const;

private:
    ThreadNetworkManager();
    ~ThreadNetworkManager();

    ThreadState current_state_ = ThreadState::DISABLED;
    StateChangeCallback state_callback_ = nullptr;

    // Network joining state
    uint8_t rejoin_attempts_ = 0;
    uint32_t last_rejoin_time_ = 0;
    struct k_timer rejoin_timer_;

    // Link quality monitoring
    int8_t current_rssi_ = 0;
    struct k_timer health_check_timer_;

    // Synchronization
    struct k_mutex state_mutex_;
};

}  // namespace thread
}  // namespace protocol
}  // namespace smarthome
