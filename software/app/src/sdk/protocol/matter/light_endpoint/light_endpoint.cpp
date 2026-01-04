/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter Light Endpoint Implementation
 */

#include <zephyr/logging/log.h>
#include "light_endpoint.hpp"

LOG_MODULE_REGISTER(matter_light, CONFIG_APP_LOG_LEVEL);

namespace smarthome { namespace protocol { namespace matter {

int LightEndpoint::init()
{
    LOG_INF("Initializing Matter Light Endpoint");
    
    mLightOn = false;
    mBrightness = 254;
    
    // Load persisted state from NVS
    // nvs_handle_t nvs;
    // if (nvs_open("matter", NVS_READONLY, &nvs) == 0) {
    //     uint8_t state;
    //     if (nvs_get_u8(nvs, "light_on", &state) == 0) {
    //         mLightOn = (state != 0);
    //     }
    //     if (nvs_get_u8(nvs, "light_level", &mBrightness) != 0) {
    //         mBrightness = 254;
    //     }
    //     nvs_close(nvs);
    // }
    
    LOG_INF("Matter Light Endpoint initialized");
    LOG_INF("Initial state - On: %d, Brightness: %d", mLightOn, mBrightness);
    
    return 0;
}

void LightEndpoint::setLightState(bool on)
{
    if (mLightOn == on) {
        return;  // No change
    }
    
    mLightOn = on;
    LOG_INF("Light state changed: %s", on ? "ON" : "OFF");
    
    // Save to NVS
    // nvs_handle_t nvs;
    // if (nvs_open("matter", NVS_READWRITE, &nvs) == 0) {
    //     nvs_set_u8(nvs, "light_on", on ? 1 : 0);
    //     nvs_commit(nvs);
    //     nvs_close(nvs);
    // }
    
    updateAttributes();
}

void LightEndpoint::setBrightness(uint8_t brightness)
{
    if (brightness > 254) {
        brightness = 254;
    }
    
    if (mBrightness == brightness) {
        return;  // No change
    }
    
    mBrightness = brightness;
    LOG_INF("Brightness set to: %d", brightness);
    
    // Save to NVS
    // nvs_handle_t nvs;
    // if (nvs_open("matter", NVS_READWRITE, &nvs) == 0) {
    //     nvs_set_u8(nvs, "light_level", brightness);
    //     nvs_commit(nvs);
    //     nvs_close(nvs);
    // }
    
    updateAttributes();
}

void LightEndpoint::updateAttributes()
{
    /* TODO: Send attribute reports to Matter controller */
    LOG_DBG("Updating Matter attributes - On: %d, Brightness: %d", mLightOn, mBrightness);
}

}  // namespace matter
}  // namespace protocol
}  // namespace smarthome
