/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * Radio Manager Module - IEEE 802.15.4 Radio Management
 * ============================================================================
 * 
 * Manages 802.15.4 radio subsystem for Thread/Matter
 */

#ifndef RADIO_MANAGER_HPP
#define RADIO_MANAGER_HPP

#include <zephyr/kernel.h>
#include <cstdint>

namespace smarthome { namespace protocol { namespace radio {

enum class RadioState : uint8_t {
    DISABLED = 0,
    INITIALIZING = 1,
    IDLE = 2,
    TRANSMITTING = 3,
    RECEIVING = 4,
    ERROR = 5
};

class RadioManager {
public:
    static RadioManager& getInstance();
    
    RadioManager(const RadioManager&) = delete;
    RadioManager& operator=(const RadioManager&) = delete;
    
    /**
     * @brief Initialize radio subsystem
     */
    int init();
    
    /**
     * @brief Enable radio
     */
    int enable();
    
    /**
     * @brief Disable radio
     */
    int disable();
    
    /**
     * @brief Transmit radio packet
     * @param channel 802.15.4 channel (11-26)
     * @param power_dbm Transmit power in dBm
     * @param data Packet data
     * @param len Packet length (max 127)
     */
    int transmit(uint8_t channel, int8_t power_dbm, 
                const uint8_t* data, size_t len);
    
    /**
     * @brief Get radio state
     */
    RadioState getState() const { return m_state; }
    
    /**
     * @brief Check if radio is enabled
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief Get state as string
     */
    const char* getStateString() const;
    
    /**
     * @brief Get transmitted packet count
     */
    uint32_t getTxCount() const { return m_tx_count; }
    
    /**
     * @brief Get received packet count
     */
    uint32_t getRxCount() const { return m_rx_count; }
    
private:
    RadioManager();
    ~RadioManager() = default;
    
    RadioState m_state;
    bool m_enabled;
    uint8_t m_current_channel;
    int8_t m_current_power;
    uint32_t m_tx_count;
    uint32_t m_rx_count;
    struct k_mutex m_mutex;
};

} // namespace radio
} // namespace protocol
} // namespace smarthome

#endif // RADIO_MANAGER_HPP
