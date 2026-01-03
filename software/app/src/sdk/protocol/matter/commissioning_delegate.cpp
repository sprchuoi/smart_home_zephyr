/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * Commissioning Delegate Implementation - Phase 1
 */

#include "commissioning_delegate.hpp"
#include "chip_config.hpp"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(commish_delegate, CONFIG_LOG_DEFAULT_LEVEL);

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
    
    // IPC to NET core for BLE advertisement
    // IPCCore::getInstance().sendMessage(...);
    
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
    
    // IPC to NET core to stop BLE advertisement
    // IPCCore::getInstance().sendMessage(...);
    
    return 0;
}

bool CommissioningDelegate::isCommissioningOpen() const {
    return commissioning_open_;
}

void CommissioningDelegate::onFabricAdded() {
    LOG_INF("=== FABRIC ADDED - Device Commissioned ===");
    
    closeCommissioningWindow();
    
    // Save to NVS
    // nvs_handle_t nvs;
    // nvs_open(MATTER_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    // nvs_set_u8(nvs, NVSKeys::COMMISSIONED, 1);
    // nvs_commit(nvs);
    // nvs_close(nvs);
    
    LOG_INF("Commissioning complete, starting network join");
}

void CommissioningDelegate::onFabricRemoved() {
    LOG_INF("=== FABRIC REMOVED - Factory Reset ===");
    
    // Clear NVS
    // nvs_handle_t nvs;
    // nvs_open(MATTER_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    // nvs_erase_all(nvs);
    // nvs_commit(nvs);
    // nvs_close(nvs);
    
    LOG_INF("All commissioning data cleared");
}

void CommissioningDelegate::onCommissioningComplete() {
    /*
     * TODO: Implement commissioning complete callback (2-3 hours)
     * 
     * Called when all commissioning steps finish.
     * 
     * 1. Stop BLE advertisement:
     *    - Signal NET core to stop BLE
     * 
     * 2. Log completion:
     *    LOG_INF("Commissioning complete");
     * 
     * 3. Save operational credentials:
     *    - Write to NVS with wear leveling
     * 
     * 4. Prepare for network:
     *    - Initialize Thread network join
     *    - Setup network monitoring
     */
    
    LOG_INF("Commissioning completed");
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
