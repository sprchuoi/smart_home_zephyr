/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Network Resilience Manager - Link monitoring and failover
 */

#pragma once

#include <cstdint>
#include <zephyr/kernel.h>

namespace smarthome { namespace protocol { namespace thread {

/**
 * Network Health Status Enum
 */
enum class NetworkHealth : uint8_t {
    UNKNOWN = 0,        ///< Network health not yet determined
    POOR = 1,          ///< Weak link, packet loss detected
    FAIR = 2,          ///< Acceptable connectivity
    GOOD = 3,          ///< Strong link, low latency
    EXCELLENT = 4      ///< Optimal network conditions
};

/**
 * Network Resilience Manager
 * 
 * Responsibilities:
 *  - Monitor Thread network link health
 *  - Detect network disconnections
 *  - Implement exponential backoff for reconnection
 *  - Track network statistics (uptime, disconnects, etc.)
 *  - Provide health diagnostics
 */
class NetworkResilienceManager {
public:
    /// Singleton instance getter
    static NetworkResilienceManager& getInstance();

    /// Delete copy/move constructors
    NetworkResilienceManager(const NetworkResilienceManager&) = delete;
    NetworkResilienceManager& operator=(const NetworkResilienceManager&) = delete;
    NetworkResilienceManager(NetworkResilienceManager&&) = delete;
    NetworkResilienceManager& operator=(NetworkResilienceManager&&) = delete;

    /**
     * Initialize resilience manager
     * 
     * TODO:
     *  1. Initialize health monitoring state
     *  2. Setup periodic health check timer
     *  3. Initialize statistics counters
     *  4. Register with Thread network manager
     *  5. Load previous statistics from NVS (uptime, disconnects, etc.)
     */
    int init();

    /**
     * Register callback for network health changes
     * 
     * @param callback Function to call when health status changes
     *                 Parameter: new NetworkHealth status
     */
    using HealthChangeCallback = void (*)(NetworkHealth);
    void setHealthCallback(HealthChangeCallback callback);

    /**
     * Register callback for disconnection events
     * 
     * @param callback Function to call when network disconnects
     */
    using DisconnectCallback = void (*)(void);
    void setDisconnectCallback(DisconnectCallback callback);

    /**
     * Check current network health
     * 
     * TODO:
     *  1. Query Thread network for link quality (RSSI)
     *  2. Measure packet loss rate
     *  3. Check neighbor count and routing tables
     *  4. Determine health level:
     *     - RSSI < -95dBm → POOR
     *     - RSSI -95 to -80dBm → FAIR
     *     - RSSI -80 to -65dBm → GOOD
     *     - RSSI > -65dBm → EXCELLENT
     *  5. Return calculated health status
     */
    NetworkHealth updateHealth();

    /**
     * Get current health status
     */
    NetworkHealth getHealth() const { return current_health_; }

    /**
     * Get health status name (for logging)
     */
    const char* getHealthName() const;

    /**
     * Handle network link down event
     * 
     * Called by Thread manager when link goes down
     * 
     * TODO:
     *  1. Record timestamp of link down
     *  2. Start link down timeout timer
     *  3. Update health to POOR
     *  4. Trigger disconnect callbacks
     */
    void onLinkDown();

    /**
     * Handle network link up event
     * 
     * Called by Thread manager when link recovers
     * 
     * TODO:
     *  1. Record recovery timestamp
     *  2. Calculate downtime duration
     *  3. Update disconnect statistics
     *  4. Clear link down timer
     *  5. Restart health monitoring
     */
    void onLinkUp();

    /**
     * Get uptime in seconds
     * 
     * @return Time device has been running
     * 
     * TODO:
     *  1. Query system uptime
     *  2. Return total uptime since boot
     */
    uint32_t getUptimeSec() const;

    /**
     * Get network connected time in seconds
     * 
     * @return Time device has been connected to Thread
     * 
     * TODO:
     *  1. Calculate time since last network connection
     *  2. Return duration
     */
    uint32_t getNetworkConnectedTimeSec() const;

    /**
     * Get network disconnect count
     * 
     * @return Number of network disconnections since boot
     */
    uint16_t getDisconnectCount() const { return disconnect_count_; }

    /**
     * Get total reconnection attempts
     */
    uint16_t getReconnectAttempts() const { return reconnect_attempts_; }

    /**
     * Save statistics to NVS for persistence
     * 
     * TODO:
     *  1. Write disconnect count to NVS
     *  2. Write reconnect attempts to NVS
     *  3. Write last disconnection time to NVS
     */
    int saveStatistics();

    /**
     * Reset all statistics
     * 
     * TODO:
     *  1. Clear disconnect counter
     *  2. Clear reconnection attempts
     *  3. Reset uptime tracking
     */
    void resetStatistics();

    /**
     * Get detailed health report
     * 
     * Returns formatted string with:
     *  - Current health status
     *  - RSSI value
     *  - Uptime
     *  - Disconnect count
     *  - Last disconnect duration
     *  - Neighbor count
     * 
     * TODO:
     *  1. Query all health metrics
     *  2. Format as human-readable string
     *  3. Return for logging/display
     */
    const char* getHealthReport();

private:
    NetworkResilienceManager();
    ~NetworkResilienceManager();

    // Health tracking
    NetworkHealth current_health_ = NetworkHealth::UNKNOWN;
    int8_t current_rssi_ = 0;
    float current_packet_loss_ = 0.0f;

    // Disconnect tracking
    uint16_t disconnect_count_ = 0;
    uint16_t reconnect_attempts_ = 0;
    uint32_t last_link_down_time_ = 0;
    uint32_t total_downtime_ms_ = 0;

    // Network timing
    uint32_t network_connect_time_ = 0;
    uint32_t boot_time_ = 0;

    // Callbacks
    HealthChangeCallback health_callback_ = nullptr;
    DisconnectCallback disconnect_callback_ = nullptr;

    // Monitoring
    struct k_timer health_check_timer_;
    struct k_timer link_down_timeout_timer_;

    // Synchronization
    struct k_mutex stats_mutex_;
};

}  // namespace thread
}  // namespace protocol
}  // namespace smarthome
