/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "radio_manager.hpp"
#include <zephyr/logging/log.h>
#include <string.h>

#ifdef CONFIG_IEEE802154
#include <zephyr/net/ieee802154_radio.h>
#endif

LOG_MODULE_REGISTER(radio_manager, CONFIG_LOG_DEFAULT_LEVEL);

namespace smarthome { namespace protocol { namespace radio {

RadioManager& RadioManager::getInstance() {
    static RadioManager instance;
    return instance;
}

RadioManager::RadioManager()
    : m_state(RadioState::DISABLED)
    , m_enabled(false)
    , m_current_channel(15)
    , m_current_power(0)
    , m_tx_count(0)
    , m_rx_count(0)
{
    k_mutex_init(&m_mutex);
}

const char* RadioManager::getStateString() const {
    switch (m_state) {
        case RadioState::DISABLED: return "DISABLED";
        case RadioState::INITIALIZING: return "INITIALIZING";
        case RadioState::IDLE: return "IDLE";
        case RadioState::TRANSMITTING: return "TRANSMITTING";
        case RadioState::RECEIVING: return "RECEIVING";
        case RadioState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

int RadioManager::init() {
    k_mutex_lock(&m_mutex, K_FOREVER);
    
    if (m_enabled) {
        k_mutex_unlock(&m_mutex);
        LOG_DBG("Radio already initialized");
        return 0;
    }
    
    LOG_INF("Radio Manager: Initializing 802.15.4...");
    m_state = RadioState::INITIALIZING;
    
#ifdef CONFIG_IEEE802154
    m_enabled = true;
    m_state = RadioState::IDLE;
    LOG_INF("Radio Manager: Initialized successfully");
    LOG_INF("Radio state: IDLE, Default channel: %u, Power: %d dBm", 
            m_current_channel, m_current_power);
#else
    LOG_WRN("IEEE 802.15.4 not configured in this build");
    m_state = RadioState::ERROR;
    k_mutex_unlock(&m_mutex);
    return -ENOTSUP;
#endif
    
    k_mutex_unlock(&m_mutex);
    return 0;
}

int RadioManager::enable() {
    k_mutex_lock(&m_mutex, K_FOREVER);
    
    if (m_enabled) {
        k_mutex_unlock(&m_mutex);
        return 0;
    }
    
    LOG_INF("Radio Manager: Enabling radio");
    
#ifdef CONFIG_IEEE802154
    m_enabled = true;
    m_state = RadioState::IDLE;
    LOG_INF("Radio Manager: Radio enabled");
#else
    LOG_WRN("IEEE 802.15.4 not available");
    k_mutex_unlock(&m_mutex);
    return -ENOTSUP;
#endif
    
    k_mutex_unlock(&m_mutex);
    return 0;
}

int RadioManager::disable() {
    k_mutex_lock(&m_mutex, K_FOREVER);
    
    if (!m_enabled) {
        k_mutex_unlock(&m_mutex);
        return 0;
    }
    
    LOG_INF("Radio Manager: Disabling radio");
    m_enabled = false;
    m_state = RadioState::DISABLED;
    
    k_mutex_unlock(&m_mutex);
    return 0;
}

int RadioManager::transmit(uint8_t channel, int8_t power_dbm,
                          const uint8_t* data, size_t len) {
    if (!data || len == 0 || len > 127) {
        return -EINVAL;
    }
    
    k_mutex_lock(&m_mutex, K_FOREVER);
    
    if (!m_enabled) {
        LOG_WRN("Radio not enabled");
        k_mutex_unlock(&m_mutex);
        return -ENOTSUP;
    }
    
    LOG_DBG("Radio Manager: TX request - channel=%u, power=%d dBm, len=%u",
            channel, power_dbm, len);
    
    m_current_channel = channel;
    m_current_power = power_dbm;
    m_state = RadioState::TRANSMITTING;
    
    /* TODO: Implement actual radio transmission */
    
    m_tx_count++;
    m_state = RadioState::IDLE;
    
    LOG_DBG("Radio Manager: TX complete (total: %u)", m_tx_count);
    
    k_mutex_unlock(&m_mutex);
    return 0;
}

} // namespace radio
} // namespace protocol
} // namespace smarthome
