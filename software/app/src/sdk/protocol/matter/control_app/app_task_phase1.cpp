/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter AppTask - Phase 1: IPC & Communication Layer
 */

#include "app_task.hpp"

LOG_MODULE_DECLARE(matter_app);

using namespace smarthome::protocol::matter;
using namespace smarthome::protocol;

int AppTask::initPhase1_IPC()
{
    int ret = 0;
    LOG_INF("PHASE 1: IPC & Communication Layer");
    
    // 1.1: Initialize IPC Core for APP-NET communication
    LOG_INF("IPC Core initialized - APP<->NET communication ready");
    
    // 1.2: Wait for IPC to be ready (max 5 seconds)
    int wait_count = 0;
    while (!ipc::IPCCore::getInstance().isReady() && wait_count < DEFAULT_WAIT_IPC_READY_MS/100) //  50 attempts
    {
        k_sleep(K_MSEC(100));
        wait_count++;   
    }
    
    if (!ipc::IPCCore::getInstance().isReady()) {
        LOG_ERR("IPC Core failed to become ready");
        return -ETIMEDOUT;
    }
    LOG_INF("IPC handshake complete");
    
    // 1.3: Send initialization status to NET core
    ipc::Message init_msg;
    init_msg.type = ipc::MessageType::STATUS_REQUEST;
    init_msg.priority = ipc::Priority::NORMAL;
    init_msg.flags = 0;
    init_msg.sequence_id = 0;
    init_msg.timestamp = k_uptime_get_32();
    init_msg.payload.status.status_code = 0;
    
    ret = ipc::IPCCore::getInstance().send(init_msg);
    if (ret < 0) {
        LOG_WRN("Failed to send init status to NET core: %d", ret);
    }
    
    return 0;
}
