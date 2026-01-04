/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter AppTask - Phase 0: Core System Initialization
 */


#include "app_task.hpp"

LOG_MODULE_DECLARE(matter_app);

using namespace smarthome::protocol::matter;

extern struct k_timer commissioning_timer;

int AppTask::initPhase0_CoreSystem()
{
    LOG_INF("PHASE 0: Core System Initialization");
    
    // 0.1: Initialize commissioning window timer
    k_timer_init(&commissioning_timer, commissioning_timeout_handler, nullptr);
    LOG_DBG("Commissioning timer initialized");
    
    // 0.2: Initialize settings subsystem for persistent storage
    int ret = settings_subsys_init();
    if (ret < 0 && ret != -EALREADY) {
        LOG_ERR("Failed to initialize settings subsystem: %d", ret);
        return ret;
    }
    LOG_INF("Settings subsystem ready");
    
    // 0.3: Load persisted state from storage
    ssize_t len = settings_get_val_len("matter/fabric/commissioned");
    if (len > 0) {
        commissioned_ = true;
        LOG_INF("Loaded commissioning state: device is commissioned");
    } else {
        LOG_INF("Device not commissioned (fresh start)");
    }
    
    // 0.4: Load persisted attributes
    bool onoff_state = false;
    len = settings_get_val_len("matter/attributes/onoff");
    if (len > 0) {
        LOG_DBG("Loaded OnOff attribute: ON (persisted)");
        onoff_state = true;
    }
    
    uint8_t level = 0;
    len = settings_get_val_len("matter/attributes/level");
    if (len > 0) {
        LOG_DBG("Loaded Level attribute: (persisted)");
    }
    
    return 0;
}
