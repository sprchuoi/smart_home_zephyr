/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * NET CORE State Machine - Radio Management for nRF5340 DK
 * ============================================================================
 * 
 * Purpose:
 *   Cortex-M33 @ 64 MHz - Network processor with state machine
 *   Manages IEEE 802.15.4 radio and BLE for Matter/Thread
 *   Implements proper state transitions for radio operations
 *
 * State Machine:
 *   IDLE → INIT → BLE_READY/RADIO_READY → OPERATING → ERROR
 */

#ifndef NET_CORE_HPP
#define NET_CORE_HPP

#include <zephyr/kernel.h>
#include <cstdint>

// Forward declarations
namespace smarthome { namespace ipc { struct Message; } }

namespace net {

/*=============================================================================
 * NET Core State Definition
 *===========================================================================*/

enum class NetCoreState : uint8_t {
    IDLE = 0,           /* Initial state, no subsystems initialized */
    INITIALIZING = 1,   /* Initializing IPC and radio subsystems */
    BLE_READY = 2,      /* BLE subsystem ready */
    RADIO_READY = 3,    /* Radio (802.15.4) subsystem ready */
    OPERATING = 4,      /* Both subsystems operational */
    ERROR = 5           /* Error state - recovery needed */
};

/*=============================================================================
 * NET Core Manager Class - State Machine Pattern
 *===========================================================================*/

class NetCoreManager {
public:
    /* Singleton access */
    static NetCoreManager& getInstance();
    
    /* Delete copy/move */
    NetCoreManager(const NetCoreManager&) = delete;
    NetCoreManager& operator=(const NetCoreManager&) = delete;
    
    /**
     * @brief Initialize NET core
     * @return 0 on success, negative errno on failure
     */
    int init();
    
    /**
     * @brief Get current state
     */
    NetCoreState getState() const { return m_state; }
    
    /**
     * @brief Get state as string
     */
    const char* getStateString() const;
    
    /**
     * @brief Enable BLE subsystem
     */
    int enableBLE();
    
    /**
     * @brief Disable BLE subsystem
     */
    int disableBLE();
    
    /**
     * @brief Enable radio (802.15.4)
     */
    int enableRadio();
    
    /**
     * @brief Disable radio
     */
    int disableRadio();
    
    /**
     * @brief Transmit radio packet
     */
    int transmitRadio(uint8_t channel, int8_t power_dbm,
                     const uint8_t* data, size_t len);
    
    /**
     * @brief Check if BLE is enabled
     */
    bool isBLEEnabled() const { return m_ble_enabled; }
    
    /**
     * @brief Check if radio is enabled
     */
    bool isRadioEnabled() const { return m_radio_enabled; }
    
    /**
     * @brief Get uptime in milliseconds
     */
    uint32_t getUptime() const { return k_uptime_get_32(); }
    
    /**
     * @brief Statistics structure
     */
    struct Statistics {
        uint32_t uptime_ms;
        uint32_t state_transitions;
        uint32_t ble_operations;
        uint32_t radio_operations;
        uint32_t errors;
    };
    
    /**
     * @brief Get statistics
     */
    const Statistics& getStats() const { return m_stats; }
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
private:
    /* Private constructor for singleton */
    NetCoreManager();
    ~NetCoreManager() = default;
    
    /*=========================================================================
     * State Management
     *=======================================================================*/
    
    /**
     * @brief Transition to new state
     */
    void transitionTo(NetCoreState new_state);
    
    /**
     * @brief Handle entry actions for each state
     */
    int onStateEntry(NetCoreState state);
    
    /**
     * @brief Handle exit actions for each state
     */
    void onStateExit(NetCoreState state);
    
    /*=========================================================================
     * IPC Message Handlers
     *=======================================================================*/
    
    void handleStatusRequest(const smarthome::ipc::Message& msg);
    void handleBLEAdvStart(const smarthome::ipc::Message& msg);
    void handleBLEAdvStop(const smarthome::ipc::Message& msg);
    void handleRadioEnable(const smarthome::ipc::Message& msg);
    void handleRadioTx(const smarthome::ipc::Message& msg);
    void handleRadioDisable(const smarthome::ipc::Message& msg);
    
    /*=========================================================================
     * Internal State
     *=======================================================================*/
    
    NetCoreState m_state;
    NetCoreState m_previous_state;
    bool m_ble_enabled;
    bool m_radio_enabled;
    
    struct k_mutex m_state_mutex;
    Statistics m_stats;
    
    uint32_t m_init_time_ms;
    
    /* Worker thread for async operations */
    struct k_thread m_worker_thread;
    K_KERNEL_STACK_MEMBER(m_worker_stack, 1024);
    
    static void workerThreadEntry(void *p1, void *p2, void *p3);
    void workerThreadLoop();
};

} // namespace net

#endif // NET_CORE_HPP
