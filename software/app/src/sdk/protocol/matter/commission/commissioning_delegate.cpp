/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Commissioning Delegate Implementation - Phase 1
 */

#include "commissioning_delegate.hpp"
#include "chip_config.hpp"
#include "../ipc/ipc_core.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(commish_delegate, CONFIG_LOG_DEFAULT_LEVEL);

using namespace smarthome::ipc;

namespace smarthome { namespace protocol { namespace matter {

CommissioningDelegate& CommissioningDelegate::getInstance() {
    static CommissioningDelegate instance;
    return instance;
}

CommissioningDelegate::CommissioningDelegate()
    : commissioning_open_(false)
    , commissioning_start_time_(0)
    , commissioning_timeout_sec_(0)
    , commissioning_passcode_(COMMISSIONABLE_PIN_CODE)
    , commissioning_discriminator_(COMMISSIONING_DISCRIMINATOR)
    , completion_callback_(nullptr)
{
    k_timer_init(&commissioning_timer_, nullptr, nullptr);
}

CommissioningDelegate::~CommissioningDelegate() {
}

int CommissioningDelegate::init() {
    LOG_INF("Initializing Commissioning Delegate");
    LOG_INF("Passcode: %08d", commissioning_passcode_);
    LOG_INF("Discriminator: %04x", commissioning_discriminator_);
    LOG_INF("Device ready for commissioning");
    
    return 0;
}

int CommissioningDelegate::openCommissioningWindow(uint32_t timeout_sec) {
    if (commissioning_open_) {
        LOG_WRN("Commissioning window already open");
        return -EALREADY;
    }
    
    LOG_INF("Opening commissioning window for %d seconds", timeout_sec);
    LOG_INF("Discriminator: %04x, Passcode: %08d", 
            commissioning_discriminator_, commissioning_passcode_);
    
    commissioning_open_ = true;
    commissioning_start_time_ = k_uptime_get_32();
    commissioning_timeout_sec_ = timeout_sec;
    
    k_timer_start(&commissioning_timer_, K_SECONDS(timeout_sec), K_NO_WAIT);
    
    // Send IPC message to NET core to start BLE advertisement
    Message ble_msg;
    ble_msg.type = MessageType::BLE_ADV_START;
    ble_msg.priority = Priority::HIGH;
    ble_msg.flags = 0x06; // Connectable + Discoverable
    ble_msg.sequence_id = 0;
    ble_msg.timestamp = k_uptime_get_32();
    ble_msg.payload.ble.adv_interval_ms = 100;
    ble_msg.payload.ble.adv_type = 0;
    ble_msg.payload.ble.adv_data_len = 0;
    
    int ret = IPCCore::getInstance().send(ble_msg);
    if (ret < 0) {
        LOG_ERR("Failed to start BLE advertising: %d", ret);
        commissioning_open_ = false;
        k_timer_stop(&commissioning_timer_);
        return ret;
    }
    
    return 0;
}

int CommissioningDelegate::closeCommissioningWindow() {
    if (!commissioning_open_) {
        LOG_DBG("Commissioning window already closed");
        return 0;
    }
    
    LOG_INF("Closing commissioning window");
    
    k_timer_stop(&commissioning_timer_);
    commissioning_open_ = false;
    
    // Send IPC message to NET core to stop BLE advertisement
    Message ble_msg;
    ble_msg.type = MessageType::BLE_ADV_STOP;
    ble_msg.priority = Priority::NORMAL;
    ble_msg.flags = 0;
    ble_msg.sequence_id = 0;
    ble_msg.timestamp = k_uptime_get_32();
    
    int ret = IPCCore::getInstance().send(ble_msg);
    if (ret < 0) {
        LOG_ERR("Failed to stop BLE advertising: %d", ret);
    }
    
    return 0;
}

bool CommissioningDelegate::isCommissioningOpen() const {
    return commissioning_open_;
}

bool CommissioningDelegate::isCommissioned() const {
    // Check if device has fabric info stored
    // Use settings_get_val_len to check if the key exists
    ssize_t len = settings_get_val_len("matter/fabric/commissioned");
    // If length > 0, the key exists and device is commissioned
    return (len > 0);
}

void CommissioningDelegate::onFabricAdded() {
    LOG_INF("=== FABRIC ADDED - Device Commissioned ===");
    
    // Close commissioning window
    closeCommissioningWindow();
    
    // Save commissioning state to persistent storage
    uint8_t commissioned = 1;
    int ret = settings_save_one("matter/fabric/commissioned", &commissioned, sizeof(commissioned));
    if (ret < 0) {
        LOG_ERR("Failed to save commissioned state: %d", ret);
    }
    
    uint8_t fabric_count = 1;
    ret = settings_save_one("matter/fabric/count", &fabric_count, sizeof(fabric_count));
    if (ret < 0) {
        LOG_ERR("Failed to save fabric count: %d", ret);
    }
    
    ret = settings_save();
    if (ret < 0) {
        LOG_ERR("Failed to commit settings: %d", ret);
    }
    
    LOG_INF("Fabric info saved, starting network join");
}

void CommissioningDelegate::onFabricRemoved() {
    LOG_INF("=== FABRIC REMOVED - Factory Reset ===");
    
    // Clear commissioning data from persistent storage
    int ret = settings_delete("matter/fabric");
    if (ret < 0 && ret != -ENOENT) {
        LOG_ERR("Failed to delete fabric settings: %d", ret);
    }
    
    ret = settings_save();
    if (ret < 0) {
        LOG_ERR("Failed to commit settings: %d", ret);
    }
    
    LOG_INF("All commissioning data cleared");
}

void CommissioningDelegate::onCommissioningComplete() {
    LOG_INF("=== Commissioning Complete ===");
    
    // Close commissioning window and stop BLE advertising
    closeCommissioningWindow();
    
    // Save operational credentials timestamp
    uint32_t completion_time = k_uptime_get_32();
    int ret = settings_save_one("matter/config/commissioned_time", 
                                &completion_time, sizeof(completion_time));
    if (ret < 0) {
        LOG_WRN("Failed to save commissioning timestamp: %d", ret);
    }
    
    ret = settings_save();
    if (ret < 0) {
        LOG_ERR("Failed to commit settings: %d", ret);
    }
    
    LOG_INF("Operational credentials saved");
    
    // Invoke completion callback if registered
    if (completion_callback_) {
        completion_callback_();
    }
}

uint32_t CommissioningDelegate::getTimeRemaining() const {
    if (!commissioning_open_) {
        return 0;
    }
    
    uint32_t elapsed_sec = (k_uptime_get_32() - commissioning_start_time_) / 1000;
    if (elapsed_sec >= commissioning_timeout_sec_) {
        return 0;
    }
    
    return commissioning_timeout_sec_ - elapsed_sec;
}

}  // namespace matter
}  // namespace protocol
}  // namespace smarthome
