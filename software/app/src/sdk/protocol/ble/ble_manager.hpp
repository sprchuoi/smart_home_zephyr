/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * BLE Manager Module - Bluetooth LE Management
 * ============================================================================
 * 
 * Manages BLE subsystem with state tracking and operations
 */

#ifndef BLE_MANAGER_HPP
#define BLE_MANAGER_HPP

#include <zephyr/kernel.h>
#include <cstdint>

namespace smarthome { namespace protocol { namespace ble {

enum class BLEState : uint8_t {
    DISABLED = 0,
    INITIALIZING = 1,
    IDLE = 2,
    ADVERTISING = 3,
    CONNECTED = 4,
    ERROR = 5
};

class BLEManager {
public:
    static BLEManager& getInstance();
    
    BLEManager(const BLEManager&) = delete;
    BLEManager& operator=(const BLEManager&) = delete;
    
    /**
     * @brief Initialize BLE subsystem
     */
    int init();
    
    /**
     * @brief Start BLE advertising
     * @param interval_ms Advertising interval in milliseconds
     */
    int startAdvertising(uint16_t interval_ms);
    
    /**
     * @brief Stop BLE advertising
     */
    int stopAdvertising();
    
    /**
     * @brief Get BLE state
     */
    BLEState getState() const { return m_state; }
    
    /**
     * @brief Check if BLE is enabled
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief Check if advertising
     */
    bool isAdvertising() const { return m_advertising; }
    
    /**
     * @brief Get state as string
     */
    const char* getStateString() const;
    
private:
    BLEManager();
    ~BLEManager() = default;
    
    BLEState m_state;
    bool m_enabled;
    bool m_advertising;
    uint16_t m_adv_interval_ms;
    struct k_mutex m_mutex;
};

} // namespace ble
} // namespace protocol
} // namespace smarthome

#endif // BLE_MANAGER_HPP
