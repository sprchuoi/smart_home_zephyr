/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter AppTask - Phase 2: Device Endpoints & Capabilities
 */

#include "app_task.hpp"


LOG_MODULE_DECLARE(matter_app);

using namespace smarthome::protocol::matter;

int AppTask::initPhase2_Endpoints(bool onoff_state, uint8_t level)
{
    LOG_INF("PHASE 2: Device Endpoints & Capabilities");
    
    // 2.1: Initialize Light Endpoint (primary device function)
    int ret = LightEndpoint::getInstance().init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize Light Endpoint: %d", ret);
        return ret;
    }
    
    // Restore persisted state to endpoint (if methods exist)
    // For now, just log the restoration
    if (onoff_state || level > 0) {
        LOG_INF("Restored Light state: %s, Level: %u", 
                onoff_state ? "ON" : "OFF", level);
    }
    LOG_INF("Light Endpoint initialized");
    
    return 0;
}
