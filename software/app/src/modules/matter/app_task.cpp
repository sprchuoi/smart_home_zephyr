/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter Application Task Implementation
 */

#include <zephyr/logging/log.h>
#include "app_task.hpp"
#include "light_endpoint.hpp"

LOG_MODULE_REGISTER(matter_app, CONFIG_APP_LOG_LEVEL);

namespace Matter {

int AppTask::init()
{
    int ret = 0;
    
    LOG_INF("Initializing Matter AppTask");
    
    /* Initialize Light Endpoint */
    ret = LightEndpoint::getInstance().init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize Light Endpoint: %d", ret);
        return ret;
    }
    
    /* TODO: Initialize Matter stack
     * - Initialize CHIP stack
     * - Register clusters
     * - Setup commissioning
     */
    
    LOG_INF("Matter AppTask initialized");
    return 0;
}

void AppTask::dispatchEvent()
{
    /* TODO: Process Matter events from event queue */
}

void AppTask::openCommissioningWindow()
{
    LOG_INF("Opening Matter commissioning window");
    
    /* TODO: Start BLE advertising for Matter commissioning
     * Duration: 15 minutes or configurable
     */
}

void AppTask::factoryReset()
{
    LOG_WRN("Performing factory reset");
    
    /* TODO: Clear NVS, restart Matter stack */
}

}  // namespace Matter
