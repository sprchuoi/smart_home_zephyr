/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter AppTask - Phase 6: Post-Initialization & Network Join
 */

#include "app_task.hpp"
#include "../../thread/thread_network_manager.hpp"

LOG_MODULE_DECLARE(matter_app);

using namespace smarthome::protocol::matter;
using namespace smarthome::protocol::thread;

int AppTask::initPhase6_NetworkJoin()
{
    LOG_INF("PHASE 6: Post-Initialization & Network Join");
    
    // Attempt Thread network join if commissioned
    if (commissioned_) {
        int ret = smarthome::protocol::thread::ThreadNetworkManager::getInstance().startNetworkJoin();
        if (ret < 0) {
            LOG_WRN("Failed to start Thread network join: %d (will retry)", ret);
        }
    }
    
    // Update final state
    k_mutex_lock(&state_mutex_, K_FOREVER);
    if (commissioned_ && network_connected_) {
        state_ = AppTaskState::NETWORK_CONNECTED;
    } else if (commissioned_) {
        state_ = AppTaskState::COMMISSIONED;
    } else {
        state_ = AppTaskState::IDLE;
    }
    k_mutex_unlock(&state_mutex_);
    
    return 0;
}
