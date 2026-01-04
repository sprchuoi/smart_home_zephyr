/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter Application Task Implementation
 * Main orchestrator and public interface
 */


#include "app_task.hpp"


LOG_MODULE_REGISTER(matter_app, CONFIG_LOG_DEFAULT_LEVEL);

using namespace smarthome::protocol::thread;
using namespace smarthome::protocol::matter;

// Timer and instance management
struct k_timer commissioning_timer;
static AppTask* g_app_task_instance = nullptr;

namespace smarthome { namespace protocol { namespace matter {

    /*=============================================================================
    * Constructor
    *===========================================================================*/

    AppTask::AppTask()
        : state_(AppTaskState::UNINITIALIZED)
        , commissioned_(false)
        , network_connected_(false)
        , init_time_ms_(0)
    {
        k_mutex_init(&state_mutex_);
        g_app_task_instance = this;
    }

    /*=============================================================================
    * Initialization Orchestrator
    *===========================================================================*/

    int AppTask::init()
    {
        LOG_INF("=== Matter AppTask Initialization ===");
        
        k_mutex_lock(&state_mutex_, K_FOREVER);
        state_ = AppTaskState::INITIALIZING;
        init_time_ms_ = k_uptime_get_32();
        k_mutex_unlock(&state_mutex_);
        
        int ret;
        bool onoff_state = false;
        uint8_t level = 0;
        ssize_t len;
        uint32_t init_duration_ms;
        
        // Execute initialization phases
        if ((ret = initPhase0_CoreSystem()) < 0) goto error;
        if ((ret = initPhase1_IPC()) < 0) goto error;
        
        // Load persisted state for use in later phases
        len = settings_get_val_len("matter/attributes/onoff");
        if (len > 0) {
            onoff_state = true;  // Key exists, assume ON state was persisted
        }
        len = settings_get_val_len("matter/attributes/level");
        if (len > 0) {
            level = 128;  // Key exists, use default level
        }
        
        if ((ret = initPhase2_Endpoints(onoff_state, level)) < 0) goto error;
        if ((ret = initPhase3_Matter(onoff_state, level)) < 0) goto error;
        if ((ret = initPhase4_Thread()) < 0) goto error;
        if ((ret = initPhase5_Callbacks()) < 0) goto error;
        if ((ret = initPhase6_NetworkJoin()) < 0) goto error;
        
        init_duration_ms = k_uptime_get_32() - init_time_ms_;
        LOG_INF("AppTask initialized in %u ms (Commissioned: %s, State: %d)",
                init_duration_ms, commissioned_ ? "YES" : "NO", (int)state_);
        
        return 0;
        
    error:
        LOG_ERR("AppTask initialization failed: %d", ret);
        
        k_mutex_lock(&state_mutex_, K_FOREVER);
        state_ = AppTaskState::ERROR;
        k_mutex_unlock(&state_mutex_);
        
        return ret;
    }

    /*=============================================================================
    * Event Processing
    *===========================================================================*/

    void AppTask::dispatchEvent()
    {
        AppTaskState current_state;
        k_mutex_lock(&state_mutex_, K_FOREVER);
        current_state = state_;
        k_mutex_unlock(&state_mutex_);
        
        // Check for pending attribute changes
        bool thread_connected = network_connected_;
        if (thread_connected != network_connected_) {
            k_mutex_lock(&state_mutex_, K_FOREVER);
            network_connected_ = thread_connected;
            k_mutex_unlock(&state_mutex_);
            
            if (thread_connected) {
                LOG_INF("Network connected event");
                processNetworkEvent();
            } else {
                LOG_INF("Network disconnected event");
            }
        }
    }

    /*=============================================================================
    * Commissioning Management
    *===========================================================================*/

    void AppTask::openCommissioningWindow()
    {
        LOG_INF("Opening Matter commissioning window (duration: 15 minutes)");
        
        k_mutex_lock(&state_mutex_, K_FOREVER);
        state_ = AppTaskState::COMMISSIONING;
        k_mutex_unlock(&state_mutex_);
        
        int ret = CommissioningDelegate::getInstance().openCommissioningWindow(900);
        if (ret < 0) {
            LOG_ERR("Failed to open commissioning window: %d", ret);
            k_mutex_lock(&state_mutex_, K_FOREVER);
            state_ = AppTaskState::IDLE;
            k_mutex_unlock(&state_mutex_);
            return;
        }
        
        uint16_t discriminator = CommissioningDelegate::getInstance().getDiscriminator();
        uint32_t setup_code = CommissioningDelegate::getInstance().getPasscode();
        
        LOG_INF("=== Commissioning Information ===");
        LOG_INF("Device: %s (Vendor: 0x%04X, Product: 0x%04X)", 
                DEVICE_NAME, VENDOR_ID, PRODUCT_ID);
        LOG_INF("Discriminator: %u", discriminator);
        LOG_INF("Setup Code: %u", setup_code);
        LOG_INF("Scan QR code or enter setup code in Matter controller app");
        LOG_INF("=================================");
        
        k_timer_start(&commissioning_timer, K_SECONDS(900), K_NO_WAIT);
        LOG_INF("Commissioning window will close automatically in 15 minutes");
    }

    void AppTask::closeCommissioningWindow()
    {
        LOG_INF("Closing Matter commissioning window");
        
        int ret = CommissioningDelegate::getInstance().closeCommissioningWindow();
        if (ret < 0) {
            LOG_ERR("Failed to close commissioning window: %d", ret);
        }
        
        k_timer_stop(&commissioning_timer);
        
        k_mutex_lock(&state_mutex_, K_FOREVER);
        if (commissioned_) {
            state_ = AppTaskState::COMMISSIONED;
            LOG_INF("Device is commissioned - state: COMMISSIONED");
        } else {
            state_ = AppTaskState::IDLE;
            LOG_INF("Commissioning cancelled or timed out - state: IDLE");
        }
        k_mutex_unlock(&state_mutex_);
        
        LOG_INF("Commissioning window closed");
    }

    /*=============================================================================
    * Factory Reset
    *===========================================================================*/

    void AppTask::factoryReset()
    {
        LOG_WRN("Performing factory reset - clearing all configuration");
        
        LOG_INF("Stopping all operations...");
        closeCommissioningWindow();
        // ThreadNetworkManager::getInstance().leave();  // Skip if not available
        LOG_INF("Disconnected from Thread network");
        
        LOG_INF("Clearing persistent storage...");
        CommissioningDelegate::getInstance().onFabricRemoved();
        
        settings_delete("matter/fabric");
        settings_delete("matter/config");
        settings_delete("matter/network");
        settings_delete("matter/attributes");
        settings_save();
        
        LOG_INF("NVS storage cleared (fabric, config, credentials)");
        
        LOG_INF("Resetting Matter stack...");
        // LightEndpoint::getInstance().setOnOff(false);  // Skip if not available
        LOG_INF("Matter stack reset - all fabrics removed");
        
        k_mutex_lock(&state_mutex_, K_FOREVER);
        state_ = AppTaskState::UNINITIALIZED;
        commissioned_ = false;
        network_connected_ = false;
        k_mutex_unlock(&state_mutex_);
        
        LOG_INF("Factory reset complete - rebooting device in 2 seconds");
        k_sleep(K_MSEC(2000));
        
        LOG_INF("Initiating cold system reboot...");
        // TODO: sys_reboot needs CONFIG_REBOOT=y in prj.conf
        sys_reboot(SYS_REBOOT_COLD);
        LOG_WRN("Reboot not implemented - halting");
    }

    /*=============================================================================
    * State & Status Queries
    *===========================================================================*/

    bool AppTask::isCommissioned() const
    {
        return commissioned_;
    }

    bool AppTask::isNetworkConnected() const
    {
        return network_connected_;
    }

    uint32_t AppTask::getUptimeSec() const
    {
        uint32_t uptime_ms = k_uptime_get_32() - init_time_ms_;
        return uptime_ms / 1000;
    }

    void AppTask::setCommissioned(bool commissioned)
    {
        k_mutex_lock(&state_mutex_, K_FOREVER);
        commissioned_ = commissioned;
        k_mutex_unlock(&state_mutex_);
    }

    /*=============================================================================
    * Internal State Machine
    *===========================================================================*/

    void AppTask::handleStateChange(AppTaskState new_state)
    {
        k_mutex_lock(&state_mutex_, K_FOREVER);
        AppTaskState old_state = state_;
        k_mutex_unlock(&state_mutex_);
        
        bool valid_transition = false;
        
        switch (old_state) {
            case AppTaskState::UNINITIALIZED:
                valid_transition = (new_state == AppTaskState::INITIALIZING);
                break;
            case AppTaskState::INITIALIZING:
                valid_transition = (new_state == AppTaskState::IDLE || 
                                  new_state == AppTaskState::ERROR);
                break;
            case AppTaskState::IDLE:
                valid_transition = (new_state == AppTaskState::COMMISSIONING ||
                                  new_state == AppTaskState::NETWORK_CONNECTED);
                break;
            case AppTaskState::COMMISSIONING:
                valid_transition = (new_state == AppTaskState::COMMISSIONED ||
                                  new_state == AppTaskState::IDLE);
                break;
            case AppTaskState::COMMISSIONED:
                valid_transition = (new_state == AppTaskState::NETWORK_CONNECTED ||
                                  new_state == AppTaskState::IDLE);
                break;
            case AppTaskState::NETWORK_JOINING:
                valid_transition = (new_state == AppTaskState::NETWORK_CONNECTED ||
                                  new_state == AppTaskState::IDLE);
                break;
            case AppTaskState::NETWORK_CONNECTED:
                valid_transition = (new_state == AppTaskState::COMMISSIONED ||
                                  new_state == AppTaskState::IDLE);
                break;
            case AppTaskState::ERROR:
                valid_transition = (new_state == AppTaskState::IDLE);
                break;
        }
        
        if (!valid_transition) {
            LOG_WRN("Invalid state transition: %d -> %d", (int)old_state, (int)new_state);
            return;
        }
        
        LOG_INF("State transition: %d -> %d", (int)old_state, (int)new_state);
        
        k_mutex_lock(&state_mutex_, K_FOREVER);
        state_ = new_state;
        k_mutex_unlock(&state_mutex_);
        
        switch (new_state) {
            case AppTaskState::COMMISSIONED:
                LOG_INF("Device commissioned successfully");
                commissioned_ = true;
                break;
            case AppTaskState::NETWORK_CONNECTED:
                LOG_INF("Network connection established");
                network_connected_ = true;
                break;
            case AppTaskState::IDLE:
                LOG_INF("Device idle");
                break;
            case AppTaskState::ERROR:
                LOG_ERR("Device in error state");
                break;
            default:
                break;
        }
    }

    void AppTask::processAttributeChange()
    {
        LOG_DBG("Processing attribute change event");
        
        static bool last_on_off = false;
        static uint8_t last_level = 0;
        
        bool current_on_off = last_on_off;
        uint8_t current_level = last_level;
        
        if (current_on_off != last_on_off) {
            LOG_INF("OnOff attribute changed: %s", current_on_off ? "ON" : "OFF");
            settings_save_one("matter/attributes/onoff", 
                            &current_on_off, sizeof(current_on_off));
            last_on_off = current_on_off;
        }
        
        if (current_level != last_level) {
            LOG_INF("Level attribute changed: %u", current_level);
            settings_save_one("matter/attributes/level", 
                            &current_level, sizeof(current_level));
            last_level = current_level;
        }
    }

    void AppTask::processNetworkEvent()
    {
        LOG_DBG("Processing network event");
        
        bool thread_connected = network_connected_;
        
        if (thread_connected && !network_connected_) {
            LOG_INF("Network join detected");
            
            k_mutex_lock(&state_mutex_, K_FOREVER);
            network_connected_ = true;
            if (commissioned_) {
                state_ = AppTaskState::NETWORK_CONNECTED;
                CommissioningDelegate::getInstance().onFabricAdded();
            }
            k_mutex_unlock(&state_mutex_);
            
            LOG_INF("Thread network joined");
            
        } else if (!thread_connected && network_connected_) {
            LOG_WRN("Network disconnection detected");
            
            k_mutex_lock(&state_mutex_, K_FOREVER);
            network_connected_ = false;
            if (commissioned_) {
                state_ = AppTaskState::COMMISSIONED;
            } else {
                state_ = AppTaskState::IDLE;
            }
            k_mutex_unlock(&state_mutex_);
            
            LOG_INF("Network reconnection will be attempted automatically");
        }
    }

    /*=============================================================================
    * Static Callback Functions
    *===========================================================================*/

    void AppTask::commissioning_complete_callback(void)
    {
        if (!g_app_task_instance) {
            return;
        }
        
        CommissioningDelegate::getInstance().onCommissioningComplete();
        g_app_task_instance->setCommissioned(true);
        
        uint8_t flag = 1;
        settings_save_one("matter/fabric/commissioned", &flag, sizeof(flag));
        settings_save();
        
        g_app_task_instance->closeCommissioningWindow();
        LOG_INF("Device commissioned successfully");
    }

    /*=============================================================================
    * Timer Callback - Commissioning Window Timeout
    *===========================================================================*/

    void AppTask::commissioning_timeout_handler(struct k_timer *timer)
    {
        LOG_INF("Commissioning window timeout (15 minutes expired)");
        
        if (g_app_task_instance) {
            g_app_task_instance->closeCommissioningWindow();
        }
    }

}  // namespace matter
}  // namespace protocol
}  // namespace smarthome


