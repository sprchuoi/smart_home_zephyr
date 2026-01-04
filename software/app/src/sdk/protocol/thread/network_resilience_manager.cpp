/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Network Resilience Manager Implementation - Phase 2
 */

#include "network_resilience_manager.hpp"
#include "../matter/commission/chip_config.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>


LOG_MODULE_REGISTER(net_resilience, CONFIG_LOG_DEFAULT_LEVEL);

namespace smarthome { namespace protocol { namespace thread {

NetworkResilienceManager& NetworkResilienceManager::getInstance() {
    static NetworkResilienceManager instance;
    return instance;
}

NetworkResilienceManager::NetworkResilienceManager()
    : current_health_(NetworkHealth::UNKNOWN)
    , current_rssi_(0)
    , current_packet_loss_(0.0f)
    , disconnect_count_(0)
    , reconnect_attempts_(0)
    , last_link_down_time_(0)
    , total_downtime_ms_(0)
    , network_connect_time_(0)
    , boot_time_(0)
{
    k_mutex_init(&stats_mutex_);
    k_timer_init(&health_check_timer_, nullptr, nullptr);
    k_timer_init(&link_down_timeout_timer_, nullptr, nullptr);
}

NetworkResilienceManager::~NetworkResilienceManager() {
}

const char* NetworkResilienceManager::getHealthName() const {
    switch (current_health_) {
        case NetworkHealth::UNKNOWN: return "UNKNOWN";
        case NetworkHealth::POOR: return "POOR";
        case NetworkHealth::FAIR: return "FAIR";
        case NetworkHealth::GOOD: return "GOOD";
        case NetworkHealth::EXCELLENT: return "EXCELLENT";
        default: return "INVALID";
    }
}

int NetworkResilienceManager::init() {
    LOG_INF("=== Initializing Network Resilience Manager ===");
    
    boot_time_ = k_uptime_get_32();
    
    // Start periodic health monitoring
    k_timer_start(&health_check_timer_,
                  K_SECONDS(matter::NETWORK_HEALTH_CHECK_INTERVAL_SEC),
                  K_SECONDS(matter::NETWORK_HEALTH_CHECK_INTERVAL_SEC));
    
    LOG_INF("Resilience manager initialized");
    LOG_INF("Boot time: %d ms", boot_time_);
    LOG_INF("Health check interval: %d seconds", matter::NETWORK_HEALTH_CHECK_INTERVAL_SEC);
    
    return 0;
}

void NetworkResilienceManager::setHealthCallback(HealthChangeCallback callback) {
    health_callback_ = callback;
}

void NetworkResilienceManager::setDisconnectCallback(DisconnectCallback callback) {
    disconnect_callback_ = callback;
}

NetworkHealth NetworkResilienceManager::updateHealth() {
    k_mutex_lock(&stats_mutex_, K_FOREVER);
    
    NetworkHealth prev_health = current_health_;
    
    // Get link quality from Thread manager
    // current_rssi_ = ThreadNetworkManager::getInstance().getLinkQuality();
    
    // Assess health based on RSSI
    if (current_rssi_ < -95) {
        current_health_ = NetworkHealth::POOR;
    } else if (current_rssi_ < -80) {
        current_health_ = NetworkHealth::FAIR;
    } else if (current_rssi_ < -65) {
        current_health_ = NetworkHealth::GOOD;
    } else if (current_rssi_ > -65) {
        current_health_ = NetworkHealth::EXCELLENT;
    } else {
        current_health_ = NetworkHealth::UNKNOWN;
    }
    
    // Factor in packet loss
    if (current_packet_loss_ > 10.0f && current_health_ > NetworkHealth::POOR) {
        current_health_ = static_cast<NetworkHealth>(
            static_cast<uint8_t>(current_health_) - 1
        );
    }
    
    if (prev_health != current_health_) {
        LOG_INF("Network health: %s (RSSI: %d dBm, Loss: %.1f%%)",
                getHealthName(), current_rssi_, current_packet_loss_);
        
        if (health_callback_) {
            health_callback_(current_health_);
        }
    }
    
    k_mutex_unlock(&stats_mutex_);
    
    return current_health_;
}

void NetworkResilienceManager::onLinkDown() {
    /*
     * TODO: Implement link down handling (3-4 hours)
     * 
     * STEP 1: Record Timestamp (1 hour)
     *  - Set last_link_down_time_ = k_uptime_get_32()
     *  - Increment disconnect_count_
     * 
     * STEP 2: Update Health (1 hour)
     *  - Set current_health_ = NetworkHealth::POOR
     *  - Trigger health callback
     * 
     * STEP 3: Start Timeout Timer (1 hour)
     *  - k_timer_start(&link_down_timeout_timer_,
     *                  K_MSEC(LINK_DOWN_TIMEOUT_MS),
     *                  K_NO_WAIT);
     *  - If timer expires, trigger rejoin logic
     * 
     * STEP 4: Notify Callbacks (30 min)
     *  - Call disconnect_callback_ if set
     *  - Log disconnect event
     * 
     * STEP 5: Save Statistics (30 min)
     *  - saveStatistics() to NVS
     */
    
    LOG_WRN("=== LINK DOWN DETECTED ===");
    
    k_mutex_lock(&stats_mutex_, K_FOREVER);
    last_link_down_time_ = k_uptime_get_32();
    disconnect_count_++;
    current_health_ = NetworkHealth::POOR;
    k_mutex_unlock(&stats_mutex_);
    
    if (disconnect_callback_) {
        disconnect_callback_();
    }
    
    LOG_INF("Disconnect count: %d", disconnect_count_);
}

void NetworkResilienceManager::onLinkUp() {
    /*
     * TODO: Implement link up handling (2-3 hours)
     * 
     * STEP 1: Calculate Downtime (1 hour)
     *  - uint32_t downtime_ms = k_uptime_get_32() - last_link_down_time_
     *  - total_downtime_ms_ += downtime_ms
     *  - Log downtime duration
     * 
     * STEP 2: Update Health (1 hour)
     *  - Assess new link quality
     *  - Update current_health_
     *  - Trigger updateHealth()
     * 
     * STEP 3: Cancel Timers (30 min)
     *  - k_timer_stop(&link_down_timeout_timer_)
     *  - Reset rejoin backoff
     * 
     * STEP 4: Save Statistics (30 min)
     *  - Update NVS with latest stats
     */
    
    LOG_INF("=== LINK UP DETECTED ===");
    
    k_mutex_lock(&stats_mutex_, K_FOREVER);
    if (last_link_down_time_ > 0) {
        uint32_t downtime_ms = k_uptime_get_32() - last_link_down_time_;
        total_downtime_ms_ += downtime_ms;
        LOG_INF("Downtime duration: %d ms", downtime_ms);
        last_link_down_time_ = 0;
    }
    network_connect_time_ = k_uptime_get_32();
    k_mutex_unlock(&stats_mutex_);
    
    // Trigger health update
    updateHealth();
}

uint32_t NetworkResilienceManager::getUptimeSec() const {
    return (k_uptime_get_32() - boot_time_) / 1000;
}

uint32_t NetworkResilienceManager::getNetworkConnectedTimeSec() const {
    if (network_connect_time_ == 0) {
        return 0;
    }
    return (k_uptime_get_32() - network_connect_time_) / 1000;
}

int NetworkResilienceManager::saveStatistics() {
    /*
     * TODO: Implement NVS save (2-3 hours)
     * 
     *  1. Open NVS:
     *     nvs_handle_t nvs = nvs_open("resilience", NVS_READWRITE);
     * 
     *  2. Write statistics:
     *     nvs_set_u16(nvs, "disconnect_count", disconnect_count_);
     *     nvs_set_u16(nvs, "reconnect_attempts", reconnect_attempts_);
     *     nvs_set_u32(nvs, "total_downtime_ms", total_downtime_ms_);
     * 
     *  3. Commit and close:
     *     nvs_commit(nvs);
     *     nvs_close(nvs);
     * 
     *  4. Handle errors gracefully
     */
    
    LOG_DBG("Saving resilience statistics to NVS");
    return 0;
}

void NetworkResilienceManager::resetStatistics() {
    /*
     * TODO: Implement reset (1 hour)
     * 
     *  1. Lock mutex
     *  2. Clear all counters
     *  3. Reset health to UNKNOWN
     *  4. Clear NVS
     */
    
    LOG_INF("Resetting resilience statistics");
    
    k_mutex_lock(&stats_mutex_, K_FOREVER);
    disconnect_count_ = 0;
    reconnect_attempts_ = 0;
    total_downtime_ms_ = 0;
    k_mutex_unlock(&stats_mutex_);
}

const char* NetworkResilienceManager::getHealthReport() {
    static char report[256];
    
    k_mutex_lock(&stats_mutex_, K_FOREVER);
    
    snprintf(report, sizeof(report),
             "Health: %s | RSSI: %d dBm | Loss: %.1f%% | "
             "Uptime: %d s | Connected: %d s | "
             "Disconnects: %d | Reconnects: %d",
             getHealthName(),
             current_rssi_,
             current_packet_loss_,
             getUptimeSec(),
             getNetworkConnectedTimeSec(),
             disconnect_count_,
             reconnect_attempts_);
    
    k_mutex_unlock(&stats_mutex_);
    
    return report;
}

}  // namespace thread
}  // namespace protocol
}  // namespace smarthome
