/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * MATTER LIGHT ENDPOINT - Cluster Attributes & State Machine
 * ============================================================================
 *
 * Purpose:
 *   Implements Matter Light Device clusters (OnOff, Level Control)
 *   Manages attribute state and reflects changes to physical hardware
 *   Handles both local (button) and remote (Matter) control
 *
 * Matter Clusters:
 *   - OnOff (0x0006):       on/off attribute, on/off command
 *   - Level Control (0x0008): currentLevel attribute (0-254)
 *
 * State Synchronization:
 *   Button/App -> Matter Attributes -> Matter Controllers
 *   (GPIO change) (onOff, level)    (Google Home, etc)
 *
 * Thread Safety:
 *   - All state changes are atomic
 *   - Safe for concurrent button + Matter commands
 */

#pragma once

#include <stdint.h>
#include <string.h>

namespace smarthome { namespace protocol { namespace matter {

class LightEndpoint {
public:
    /// Get singleton instance
    static LightEndpoint& getInstance() {
        static LightEndpoint instance;
        return instance;
    }
    
    // Prevent copying
    LightEndpoint(const LightEndpoint&) = delete;
    LightEndpoint& operator=(const LightEndpoint&) = delete;
    
    /**
     * Initialize Matter light endpoint
     * 
     * Registers clusters and sets initial attribute values
     * @return 0 on success, negative error code on failure
     */
    int init();
    
    /**
     * Set light power state
     * 
     * @param on true = on, false = off
     * Synchronizes Matter OnOff cluster attribute
     * Updates physical LED immediately
     */
    void setLightState(bool on);
    
    /**
     * Get current light power state
     * @return true if light is on, false if off
     */
    bool getLightState() const { return mLightOn; }
    
    /**
     * Set brightness level (0-254)
     * 
     * @param brightness 0 = off, 254 = full brightness
     * Maps to Matter Level Control cluster currentLevel
     * Triggers PWM update on LED pin
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * Get current brightness level
     * @return brightness 0-254
     */
    uint8_t getBrightness() const { return mBrightness; }
    
    /**
     * Update Matter attributes in fabric
     * 
     * Reports current state to all connected Matter controllers
     * Called after local changes or remote commands
     */
    void updateAttributes();

private:
    /// Private constructor (singleton)
    LightEndpoint() = default;
    
    /// Destructor
    ~LightEndpoint() = default;
    
    // State variables
    bool mLightOn = false;           ///< OnOff cluster state
    uint8_t mBrightness = 254;       ///< Level Control (0-254)
};

}  // namespace matter
}  // namespace protocol
}  // namespace smarthome
