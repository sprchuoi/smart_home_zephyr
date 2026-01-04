/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter AppTask - Phase 5: Event System & Callbacks
 */

#include "app_task.hpp"

LOG_MODULE_DECLARE(matter_app);

using namespace smarthome::protocol::matter;

int AppTask::initPhase5_Callbacks()
{
    LOG_INF("PHASE 5: Event System & Callbacks");
    
    // Register Matter commissioning completion callback
    CommissioningDelegate::getInstance().setOnCommissioningComplete(
        commissioning_complete_callback);
    
    LOG_INF("Event callbacks registered");
    return 0;
}
