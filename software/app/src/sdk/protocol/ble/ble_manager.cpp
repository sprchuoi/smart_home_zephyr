/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble_manager.hpp"
#include <zephyr/logging/log.h>
#include <string.h>

#ifdef CONFIG_BT
#include <zephyr/bluetooth/bluetooth.h>
#endif

LOG_MODULE_REGISTER(ble_manager, CONFIG_LOG_DEFAULT_LEVEL);

namespace smarthome { namespace protocol { namespace ble {

BLEManager& BLEManager::getInstance() {
    static BLEManager instance;
    return instance;
}

BLEManager::BLEManager()
    : m_state(BLEState::DISABLED)
    , m_enabled(false)
    , m_advertising(false)
    , m_adv_interval_ms(0)
{
    k_mutex_init(&m_mutex);
}

const char* BLEManager::getStateString() const {
    switch (m_state) {
        case BLEState::DISABLED: return "DISABLED";
        case BLEState::INITIALIZING: return "INITIALIZING";
        case BLEState::IDLE: return "IDLE";
        case BLEState::ADVERTISING: return "ADVERTISING";
        case BLEState::CONNECTED: return "CONNECTED";
        case BLEState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

int BLEManager::init() {
    k_mutex_lock(&m_mutex, K_FOREVER);
    
    if (m_enabled) {
        k_mutex_unlock(&m_mutex);
        LOG_DBG("BLE already initialized");
        return 0;
    }
    
    LOG_INF("BLE Manager: Initializing...");
    m_state = BLEState::INITIALIZING;
    
#ifdef CONFIG_BT
    int ret = bt_enable(NULL);
    if (ret < 0) {
        LOG_ERR("BLE enable failed: %d", ret);
        m_state = BLEState::ERROR;
        k_mutex_unlock(&m_mutex);
        return ret;
    }
    
    m_enabled = true;
    m_state = BLEState::IDLE;
    LOG_INF("BLE Manager: Initialized successfully");
#else
    LOG_WRN("BLE not configured in this build");
    m_state = BLEState::ERROR;
    k_mutex_unlock(&m_mutex);
    return -ENOTSUP;
#endif
    
    k_mutex_unlock(&m_mutex);
    return 0;
}

int BLEManager::startAdvertising(uint16_t interval_ms) {
    k_mutex_lock(&m_mutex, K_FOREVER);
    
    if (!m_enabled) {
        LOG_WRN("BLE not enabled");
        k_mutex_unlock(&m_mutex);
        return -ENOTSUP;
    }
    
    if (m_advertising) {
        LOG_DBG("BLE already advertising");
        k_mutex_unlock(&m_mutex);
        return 0;
    }
    
    LOG_INF("BLE Manager: Starting advertising (interval: %u ms)", interval_ms);
    m_adv_interval_ms = interval_ms;
    m_advertising = true;
    m_state = BLEState::ADVERTISING;
    
    /* TODO: Implement actual BLE advertising */
    
    k_mutex_unlock(&m_mutex);
    return 0;
}

int BLEManager::stopAdvertising() {
    k_mutex_lock(&m_mutex, K_FOREVER);
    
    if (!m_advertising) {
        k_mutex_unlock(&m_mutex);
        return 0;
    }
    
    LOG_INF("BLE Manager: Stopping advertising");
    m_advertising = false;
    m_state = BLEState::IDLE;
    
    /* TODO: Implement actual BLE advertising stop */
    
    k_mutex_unlock(&m_mutex);
    return 0;
}

} // namespace ble
} // namespace protocol
} // namespace smarthome
