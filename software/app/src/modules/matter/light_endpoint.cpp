/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter Light Endpoint Implementation
 */

#include <zephyr/logging/log.h>
#include "light_endpoint.hpp"

LOG_MODULE_REGISTER(matter_light, CONFIG_APP_LOG_LEVEL);

namespace Matter {

int LightEndpoint::init()
{
    LOG_INF("Initializing Matter Light Endpoint");
    
    mLightOn = false;
    mBrightness = 254;
    
    /* TODO: Initialize Matter cluster attributes
     * - OnOff cluster (0x0006)
     * - Level Control cluster (0x0008)
     * - Basic Info cluster (0x0028)
     */
    
    LOG_INF("Matter Light Endpoint initialized");
    return 0;
}

void LightEndpoint::setLightState(bool on)
{
    mLightOn = on;
    LOG_INF("Light state changed: %s", on ? "ON" : "OFF");
    
    /* TODO: Update Matter OnOff attribute */
    // chip::MatterReportingAttributeChangeCallback(...)
    
    updateAttributes();
}

void LightEndpoint::setBrightness(uint8_t brightness)
{
    mBrightness = brightness;
    LOG_INF("Brightness set to: %d", brightness);
    
    /* TODO: Update Matter Level Control attribute */
    
    updateAttributes();
}

void LightEndpoint::updateAttributes()
{
    /* TODO: Send attribute reports to Matter controller */
    LOG_DBG("Updating Matter attributes - On: %d, Brightness: %d", mLightOn, mBrightness);
}

}  // namespace Matter
