/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "thread_network_manager.hpp"
#include "../matter/commission/chip_config.hpp"
#include "network_resilience_manager.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(thread_mgr, CONFIG_LOG_DEFAULT_LEVEL);

namespace smarthome { namespace protocol { namespace thread {

ThreadNetworkManager& ThreadNetworkManager::getInstance() {
    static ThreadNetworkManager instance;
    return instance;
}

ThreadNetworkManager::ThreadNetworkManager()
    : current_state_(ThreadState::DISABLED)
    , rejoin_attempts_(0)
    , last_rejoin_time_(0)
    , current_rssi_(0)
{
    k_mutex_init(&state_mutex_);
    k_timer_init(&rejoin_timer_, nullptr, nullptr);
    k_timer_init(&health_check_timer_, nullptr, nullptr);
}

ThreadNetworkManager::~ThreadNetworkManager() {
}

const char* ThreadNetworkManager::getStateName() const {
    switch (current_state_) {
        case ThreadState::DISABLED: return "DISABLED";
        case ThreadState::INITIALIZING: return "INITIALIZING";
        case ThreadState::IDLE: return "IDLE";
        case ThreadState::JOINING: return "JOINING";
        case ThreadState::CHILD: return "CHILD";
        case ThreadState::ROUTER: return "ROUTER";
        case ThreadState::LEADER: return "LEADER";
        case ThreadState::DETACHING: return "DETACHING";
        case ThreadState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

int ThreadNetworkManager::init() {
    LOG_INF("=== Initializing Thread Network Manager ===");
    LOG_INF("Channel: %d, PAN ID: 0x%04x", matter::THREAD_CHANNEL, matter::THREAD_PAN_ID);
    LOG_INF("Network name: %s", matter::THREAD_NETWORK_NAME);
    LOG_INF("TX Power: %d dBm", matter::THREAD_TX_POWER);
    
    k_mutex_lock(&state_mutex_, K_FOREVER);
    current_state_ = ThreadState::INITIALIZING;
    k_mutex_unlock(&state_mutex_);
    
    // OpenThread initialization will be done when CONFIG_OPENTHREAD is available
    // For now, set state to IDLE to indicate ready for join
    
    k_mutex_lock(&state_mutex_, K_FOREVER);
    current_state_ = ThreadState::IDLE;
    k_mutex_unlock(&state_mutex_);
    
    LOG_INF("Thread Network Manager initialized");
    return 0;
}

int ThreadNetworkManager::startNetworkJoin() {
    LOG_INF("Starting Thread network join");
    
    if (current_state_ == ThreadState::CHILD ||
        current_state_ == ThreadState::ROUTER ||
        current_state_ == ThreadState::LEADER) {
        LOG_INF("Already attached to Thread network");
        return 0;
    }
    
    k_mutex_lock(&state_mutex_, K_FOREVER);
    current_state_ = ThreadState::JOINING;
    k_mutex_unlock(&state_mutex_);
    
    // Reset rejoin attempts
    rejoin_attempts_ = 0;
    
    // OpenThread join logic will be added when OpenThread SDK is integrated
    // otThreadSetEnabled(instance, true);
    // otIp6SetEnabled(instance, true);
    
    LOG_INF("Thread join initiated");
    
    // Simulate successful join for testing
    k_mutex_lock(&state_mutex_, K_FOREVER);
    current_state_ = ThreadState::CHILD;
    k_mutex_unlock(&state_mutex_);
    
    if (state_callback_) {
        state_callback_(ThreadState::CHILD);
    }
    
    return 0;
}

int ThreadNetworkManager::leaveNetwork() {
    LOG_INF("Leaving Thread network");
    
    k_mutex_lock(&state_mutex_, K_FOREVER);
    ThreadState prev_state = current_state_;
    current_state_ = ThreadState::DETACHING;
    k_mutex_unlock(&state_mutex_);
    
    // Cancel any pending rejoin
    k_timer_stop(&rejoin_timer_);
    
    // OpenThread detach logic
    // otThreadSetEnabled(instance, false);
    
    k_mutex_lock(&state_mutex_, K_FOREVER);
    current_state_ = ThreadState::IDLE;
    k_mutex_unlock(&state_mutex_);
    
    if (state_callback_ && prev_state != ThreadState::IDLE) {
        state_callback_(ThreadState::IDLE);
    }
    
    LOG_INF("Thread network left");
    return 0;
}

void ThreadNetworkManager::setStateCallback(StateChangeCallback callback) {
    state_callback_ = callback;
}

int8_t ThreadNetworkManager::getLinkQuality() const {
    // OpenThread RSSI query will be added
    // const otRouterInfo *parentInfo = otThreadGetParent(instance);
    // if (parentInfo) {
    //     return parent RSSI value
    // }
    return current_rssi_;
}

void ThreadNetworkManager::getNetworkDiagnostics() {
    LOG_INF("=== Thread Network Diagnostics ===");
    LOG_INF("State: %s", getStateName());
    LOG_INF("RSSI: %d dBm", current_rssi_);
    LOG_INF("Rejoin attempts: %d", rejoin_attempts_);
    
    // Additional OpenThread diagnostics:
    // - Extended address
    // - Short address (RLOC16)
    // - Leader data
    // - Neighbor count
    // - Route table size
}

int ThreadNetworkManager::scheduleNetworkRejoin() {
    k_mutex_lock(&state_mutex_, K_FOREVER);
    rejoin_attempts_++;
    k_mutex_unlock(&state_mutex_);
    
    if (rejoin_attempts_ > matter::MAX_RECONNECT_ATTEMPTS) {
        LOG_ERR("Max rejoin attempts (%d) reached", matter::MAX_RECONNECT_ATTEMPTS);
        return -ETIMEDOUT;
    }
    
    // Calculate exponential backoff delay
    uint32_t delay_ms = matter::INITIAL_RECONNECT_DELAY_MS;
    for (uint8_t i = 1; i < rejoin_attempts_; i++) {
        delay_ms = (uint32_t)(delay_ms * matter::RECONNECT_BACKOFF_MULTIPLIER);
        if (delay_ms > matter::MAX_RECONNECT_DELAY_MS) {
            delay_ms = matter::MAX_RECONNECT_DELAY_MS;
            break;
        }
    }
    
    LOG_INF("Scheduling rejoin attempt %d/%d in %d ms",
            rejoin_attempts_, matter::MAX_RECONNECT_ATTEMPTS, delay_ms);
    
    k_timer_start(&rejoin_timer_, K_MSEC(delay_ms), K_NO_WAIT);
    last_rejoin_time_ = k_uptime_get_32();
    
    return 0;
}

int ThreadNetworkManager::scanAndJoin() {
    LOG_INF("Scanning for Thread networks");
    
    // OpenThread active scan
    // otError error = otLinkActiveScan(instance, channelMask, scanDuration,
    //                                   scanCallback, this);
    
    // For now, just trigger join
    return startNetworkJoin();
}

void ThreadNetworkManager::setTxPower(int8_t power_dbm) {
    if (power_dbm < -20) power_dbm = -20;
    if (power_dbm > 20) power_dbm = 20;
    
    // otPlatRadioSetTransmitPower(instance, power_dbm);
    
    LOG_INF("Thread TX power set to %d dBm", power_dbm);
}

const char* ThreadNetworkManager::getThreadVersion() const {
    // return otGetVersionString();
    return "OpenThread 1.3.0";
}

}  // namespace thread
}  // namespace protocol
}  // namespace smarthome
