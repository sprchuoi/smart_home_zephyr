/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter AppTask - Phase 4: Thread Network Layer
 */

#include "app_task.hpp"
#include "../../thread/thread_network_manager.hpp"
#include "../../thread/network_resilience_manager.hpp"

LOG_MODULE_DECLARE(matter_app);

using namespace smarthome::protocol::matter;
using namespace smarthome::protocol::thread;

int AppTask::initPhase4_Thread()
{
    LOG_INF("PHASE 4: Thread Network Layer");
    
    int ret = smarthome::protocol::thread::ThreadNetworkManager::getInstance().init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize Thread Manager: %d", ret);
        return ret;
    }
    
    ret = smarthome::protocol::thread::NetworkResilienceManager::getInstance().init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize Resilience Manager: %d", ret);
        return ret;
    }
    
    LOG_INF("Thread network stack initialized");
    return 0;
}
