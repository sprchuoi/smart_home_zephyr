/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "net_core.hpp"
#include "../sdk/ipc/ipc_core.hpp"
#include "../sdk/protocol/ble/ble_manager.hpp"
#include "../sdk/protocol/radio/radio_manager.hpp"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(net_core, CONFIG_LOG_DEFAULT_LEVEL);

namespace net {

/*=============================================================================
 * Singleton Implementation
 *===========================================================================*/

NetCoreManager& NetCoreManager::getInstance() {
    static NetCoreManager instance;
    return instance;
}

/*=============================================================================
 * Constructor
 *===========================================================================*/

NetCoreManager::NetCoreManager()
    : m_state(NetCoreState::IDLE)
    , m_previous_state(NetCoreState::IDLE)
    , m_ble_enabled(false)
    , m_radio_enabled(false)
    , m_stats{}
    , m_init_time_ms(0)
{
    k_mutex_init(&m_state_mutex);
    LOG_DBG("NetCoreManager constructed");
}

/*=============================================================================
 * State Machine - State String
 *===========================================================================*/

const char* NetCoreManager::getStateString() const {
    switch (m_state) {
        case NetCoreState::IDLE: return "IDLE";
        case NetCoreState::INITIALIZING: return "INITIALIZING";
        case NetCoreState::BLE_READY: return "BLE_READY";
        case NetCoreState::RADIO_READY: return "RADIO_READY";
        case NetCoreState::OPERATING: return "OPERATING";
        case NetCoreState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void NetCoreManager::transitionTo(net::NetCoreState new_state) {
    k_mutex_lock(&m_state_mutex, K_FOREVER);
    
    if (m_state != new_state) {
        LOG_INF("State transition: %s → %s",
                getStateString(),
                (m_state = new_state, getStateString()));
        
        m_stats.state_transitions++;
    }
    
    k_mutex_unlock(&m_state_mutex);
}

int NetCoreManager::onStateEntry(net::NetCoreState state) {
    LOG_DBG("Entering state: %s", getStateString());
    
    switch (state) {
        case NetCoreState::INITIALIZING:
            LOG_INF("Initializing subsystems...");
            break;
            
        case NetCoreState::BLE_READY:
            LOG_INF("BLE subsystem ready");
            break;
            
        case NetCoreState::RADIO_READY:
            LOG_INF("Radio subsystem ready");
            break;
            
        case NetCoreState::OPERATING:
            LOG_INF("NET Core fully operational");
            break;
            
        case NetCoreState::ERROR:
            LOG_ERR("ERROR state entered");
            m_stats.errors++;
            break;
            
        default:
            break;
    }
    
    return 0;
}

void NetCoreManager::onStateExit(NetCoreState state) {
    LOG_DBG("Exiting state: %s", getStateString());
}

/*=============================================================================
 * Initialization
 *===========================================================================*/

int NetCoreManager::init() {
    LOG_INF("NET Core Manager initializing...");
    
    m_init_time_ms = k_uptime_get_32();
    transitionTo(NetCoreState::INITIALIZING);
    onStateEntry(NetCoreState::INITIALIZING);
    
    /* Initialize IPC service */
    auto& ipc = smarthome::ipc::IPCCore::getInstance();
    int ret = ipc.init();
    if (ret < 0) {
        LOG_ERR("IPC initialization failed: %d", ret);
        transitionTo(NetCoreState::ERROR);
        return ret;
    }
    LOG_INF("IPC initialized");
    
    /* Register IPC message handlers */
    ipc.registerCallback(smarthome::ipc::MessageType::STATUS_REQUEST,
                        [](const smarthome::ipc::Message& msg) {
                            NetCoreManager::getInstance().handleStatusRequest(msg);
                        });
    
    ipc.registerCallback(smarthome::ipc::MessageType::BLE_ADV_START,
                        [](const smarthome::ipc::Message& msg) {
                            NetCoreManager::getInstance().handleBLEAdvStart(msg);
                        });
    
    ipc.registerCallback(smarthome::ipc::MessageType::BLE_ADV_STOP,
                        [](const smarthome::ipc::Message& msg) {
                            NetCoreManager::getInstance().handleBLEAdvStop(msg);
                        });
    
    ipc.registerCallback(smarthome::ipc::MessageType::RADIO_ENABLE,
                        [](const smarthome::ipc::Message& msg) {
                            NetCoreManager::getInstance().handleRadioEnable(msg);
                        });
    
    ipc.registerCallback(smarthome::ipc::MessageType::RADIO_TX,
                        [](const smarthome::ipc::Message& msg) {
                            NetCoreManager::getInstance().handleRadioTx(msg);
                        });
    
    ipc.registerCallback(smarthome::ipc::MessageType::RADIO_DISABLE,
                        [](const smarthome::ipc::Message& msg) {
                            NetCoreManager::getInstance().handleRadioDisable(msg);
                        });
    
    LOG_INF("IPC callbacks registered");
    
    /* Initialize BLE module */
    auto& ble_mgr = smarthome::protocol::ble::BLEManager::getInstance();
    int ble_ret = ble_mgr.init();
    if (ble_ret < 0) {
        LOG_WRN("BLE init failed (err %d), continuing without BLE", ble_ret);
    } else {
        m_ble_enabled = true;
        LOG_INF("BLE module initialized");
        transitionTo(NetCoreState::BLE_READY);
        onStateEntry(NetCoreState::BLE_READY);
    }
    
    /* Initialize Radio module */
    auto& radio_mgr = smarthome::protocol::radio::RadioManager::getInstance();
    int radio_ret = radio_mgr.init();
    if (radio_ret < 0) {
        LOG_WRN("Radio init failed (err %d), continuing without radio", radio_ret);
    } else {
        m_radio_enabled = true;
        LOG_INF("Radio module initialized");
        transitionTo(NetCoreState::RADIO_READY);
        onStateEntry(NetCoreState::RADIO_READY);
    }
    
    /* Transition to OPERATING if both or at least one is enabled */
    if (m_ble_enabled || m_radio_enabled) {
        transitionTo(NetCoreState::OPERATING);
        onStateEntry(NetCoreState::OPERATING);
    } else {
        LOG_WRN("No radio subsystems enabled");
        transitionTo(NetCoreState::ERROR);
    }
    
    /* Start worker thread */
    k_thread_create(&m_worker_thread, m_worker_stack, K_KERNEL_STACK_SIZEOF(m_worker_stack),
                    workerThreadEntry, this, nullptr, nullptr,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);
    k_thread_name_set(&m_worker_thread, "net_core_worker");
    
    LOG_INF("NET Core Manager initialized successfully");
    return 0;
}

/*=============================================================================
 * Subsystem Control
 *===========================================================================*/

int NetCoreManager::enableBLE() {
    auto& ble_mgr = smarthome::protocol::ble::BLEManager::getInstance();
    return ble_mgr.init();
}

int NetCoreManager::disableBLE() {
    auto& ble_mgr = smarthome::protocol::ble::BLEManager::getInstance();
    if (ble_mgr.isAdvertising()) {
        return ble_mgr.stopAdvertising();
    }
    return 0;
}

int NetCoreManager::enableRadio() {
    auto& radio_mgr = smarthome::protocol::radio::RadioManager::getInstance();
    return radio_mgr.init();
}

int NetCoreManager::disableRadio() {
    auto& radio_mgr = smarthome::protocol::radio::RadioManager::getInstance();
    return radio_mgr.disable();
}

int NetCoreManager::transmitRadio(uint8_t channel, int8_t power_dbm,
                                   const uint8_t* data, size_t len) {
    auto& radio_mgr = smarthome::protocol::radio::RadioManager::getInstance();
    return radio_mgr.transmit(channel, power_dbm, data, len);
}

/*=============================================================================
 * Statistics
 *===========================================================================*/

void NetCoreManager::resetStats() {
    k_mutex_lock(&m_state_mutex, K_FOREVER);
    memset(&m_stats, 0, sizeof(m_stats));
    m_init_time_ms = k_uptime_get_32();
    k_mutex_unlock(&m_state_mutex);
    LOG_INF("Statistics reset");
}

/*=============================================================================
 * IPC Message Handlers
 *===========================================================================*/

void NetCoreManager::handleStatusRequest(const smarthome::ipc::Message& msg) {
    LOG_INF("Status request from APP core");
    
    auto response = smarthome::ipc::MessageBuilder(smarthome::ipc::MessageType::STATUS_RESPONSE)
                      .setPriority(smarthome::ipc::Priority::NORMAL)
                      .setParam(0, (uint32_t)m_state)  /* Current state */
                      .setParam(1, m_ble_enabled ? 1 : 0)
                      .setParam(2, m_radio_enabled ? 1 : 0)
                      .setParam(3, m_stats.state_transitions)
                      .build();
    
    auto& ipc = smarthome::ipc::IPCCore::getInstance();
    ipc.send(response);
}

void NetCoreManager::handleBLEAdvStart(const smarthome::ipc::Message& msg) {
    LOG_INF("BLE advertising start request");
    
    uint16_t interval_ms = msg.payload.ble.adv_interval_ms;
    LOG_INF("Advertising interval: %u ms", interval_ms);
    
    auto& ble_mgr = smarthome::protocol::ble::BLEManager::getInstance();
    int ret = ble_mgr.startAdvertising(interval_ms);
    
    auto& ipc = smarthome::ipc::IPCCore::getInstance();
    if (ret == 0) {
        auto ack = smarthome::ipc::MessageBuilder(smarthome::ipc::MessageType::ACK)
                     .setPriority(smarthome::ipc::Priority::NORMAL)
                     .build();
        ipc.send(ack);
    } else {
        auto nack = smarthome::ipc::MessageBuilder(smarthome::ipc::MessageType::NACK)
                      .setPriority(smarthome::ipc::Priority::NORMAL)
                      .build();
        ipc.send(nack);
    }
}

void NetCoreManager::handleBLEAdvStop(const smarthome::ipc::Message& msg) {
    LOG_INF("BLE advertising stop request");
    
    auto& ble_mgr = smarthome::protocol::ble::BLEManager::getInstance();
    ble_mgr.stopAdvertising();
    
    auto ack = smarthome::ipc::MessageBuilder(smarthome::ipc::MessageType::ACK)
                 .setPriority(smarthome::ipc::Priority::NORMAL)
                 .build();
    
    auto& ipc = smarthome::ipc::IPCCore::getInstance();
    ipc.send(ack);
}

void NetCoreManager::handleRadioEnable(const smarthome::ipc::Message& msg) {
    LOG_INF("Radio enable request");
    
    auto& radio_mgr = smarthome::protocol::radio::RadioManager::getInstance();
    int ret = radio_mgr.enable();
    
    auto& ipc = smarthome::ipc::IPCCore::getInstance();
    if (ret == 0) {
        auto ack = smarthome::ipc::MessageBuilder(smarthome::ipc::MessageType::ACK)
                     .setPriority(smarthome::ipc::Priority::NORMAL)
                     .build();
        ipc.send(ack);
    } else {
        auto nack = smarthome::ipc::MessageBuilder(smarthome::ipc::MessageType::NACK)
                      .setPriority(smarthome::ipc::Priority::NORMAL)
                      .build();
        ipc.send(nack);
    }
}

void NetCoreManager::handleRadioTx(const smarthome::ipc::Message& msg) {
    uint8_t channel = msg.payload.radio.channel;
    int8_t power = msg.payload.radio.power_dbm;
    
    LOG_DBG("Radio TX: channel=%u, power=%d dBm", channel, power);
    
    auto& radio_mgr = smarthome::protocol::radio::RadioManager::getInstance();
    radio_mgr.transmit(channel, power, msg.payload.radio.data, 20);
    
    auto ack = smarthome::ipc::MessageBuilder(smarthome::ipc::MessageType::ACK)
                 .setPriority(smarthome::ipc::Priority::NORMAL)
                 .build();
    
    auto& ipc = smarthome::ipc::IPCCore::getInstance();
    ipc.send(ack);
}

void NetCoreManager::handleRadioDisable(const smarthome::ipc::Message& msg) {
    LOG_INF("Radio disable request");
    
    auto& radio_mgr = smarthome::protocol::radio::RadioManager::getInstance();
    radio_mgr.disable();
    
    auto ack = smarthome::ipc::MessageBuilder(smarthome::ipc::MessageType::ACK)
                 .setPriority(smarthome::ipc::Priority::NORMAL)
                 .build();
    
    auto& ipc = smarthome::ipc::IPCCore::getInstance();
    ipc.send(ack);
}

/*=============================================================================
 * Worker Thread
 *===========================================================================*/

void NetCoreManager::workerThreadEntry(void *p1, void *p2, void *p3) {
    NetCoreManager *mgr = static_cast<NetCoreManager*>(p1);
    mgr->workerThreadLoop();
}

void NetCoreManager::workerThreadLoop() {
    LOG_INF("NET Core worker thread started");
    
    uint32_t last_stats_ms = 0;
    
    while (1) {
        k_sleep(K_MSEC(5000));
        
        /* Print stats periodically */
        uint32_t now_ms = k_uptime_get_32();
        if (now_ms - last_stats_ms >= 30000) {
            k_mutex_lock(&m_state_mutex, K_FOREVER);
            
            LOG_INF("=== NET Core Stats (uptime: %u ms) ===",
                    now_ms - m_init_time_ms);
            LOG_INF("State: %s", getStateString());
            LOG_INF("BLE: %s, Radio: %s",
                    m_ble_enabled ? "enabled" : "disabled",
                    m_radio_enabled ? "enabled" : "disabled");
            LOG_INF("Transitions: %u, Errors: %u",
                    m_stats.state_transitions, m_stats.errors);
            LOG_INF("BLE ops: %u, Radio ops: %u",
                    m_stats.ble_operations, m_stats.radio_operations);
            
            k_mutex_unlock(&m_state_mutex);
            last_stats_ms = now_ms;
        }
    }
}

} // namespace net

/*=============================================================================
 * C++ Entry Point
 *===========================================================================*/

extern "C" int main(void) {
    LOG_INF("\n\n*** nRF5340 NET Core Starting ***\n");
    LOG_INF("CPU: Cortex-M33 @ 64 MHz");
    LOG_INF("=======================================");
    
    auto& mgr = net::NetCoreManager::getInstance();
    
    int ret = mgr.init();
    if (ret < 0) {
        LOG_ERR("NET Core Manager initialization failed: %d", ret);
        return ret;
    }
    
    LOG_INF("NET Core main loop started");
    LOG_INF("Waiting for IPC commands from APP core...");
    
    /* Main loop - keep running */
    while (1) {
        k_sleep(K_MSEC(1000));
    }
    
    return 0;
}
