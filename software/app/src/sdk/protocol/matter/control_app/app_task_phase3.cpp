/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * Matter AppTask - Phase 3: Matter Commissioning Layer
 */


#include "app_task.hpp"

LOG_MODULE_DECLARE(matter_app);

using namespace smarthome::protocol::matter;

int AppTask::initPhase3_Matter(bool onoff_state, uint8_t level)
{
    LOG_INF("PHASE 3: Matter Commissioning Layer");
    
    // Initialize Commissioning Delegate
    int ret = CommissioningDelegate::getInstance().init();
    if (ret < 0) {
        LOG_ERR("Commissioning Delegate init failed: %d", ret);
        return ret;
    }
    
    // Store device configuration
    uint16_t vendor_id = VENDOR_ID;
    uint16_t product_id = PRODUCT_ID;
    settings_save_one("matter/config/vendor_id", &vendor_id, sizeof(vendor_id));
    settings_save_one("matter/config/product_id", &product_id, sizeof(product_id));
    
    // Configure Endpoint 0 (Root Node) - 9 clusters
    uint16_t device_type_root = 0x0016;
    settings_save_one("matter/ep0/descriptor/device_type", &device_type_root, sizeof(device_type_root));
    
    const char* node_label = "Smart Home Light";
    settings_save_one("matter/ep0/basic/node_label", node_label, strlen(node_label));
    
    uint64_t breadcrumb = 0;
    settings_save_one("matter/ep0/commissioning/breadcrumb", &breadcrumb, sizeof(breadcrumb));
    
    uint8_t network_feature_map = 0x04;
    settings_save_one("matter/ep0/network/features", &network_feature_map, sizeof(network_feature_map));
    
    uint32_t boot_reasons = 1;
    settings_save_one("matter/ep0/diagnostics/boot_reason", &boot_reasons, sizeof(boot_reasons));
    
    uint8_t window_status = 0;
    settings_save_one("matter/ep0/admin_comm/window_status", &window_status, sizeof(window_status));
    
    // Configure Endpoint 1 (On/Off Light) - 7 clusters
    uint16_t device_type_light = 0x0100;
    settings_save_one("matter/ep1/descriptor/device_type", &device_type_light, sizeof(device_type_light));
    
    uint16_t identify_time = 0;
    settings_save_one("matter/ep1/identify/time", &identify_time, sizeof(identify_time));
    
    uint8_t name_support = 0x80;
    settings_save_one("matter/ep1/groups/name_support", &name_support, sizeof(name_support));
    
    uint8_t scene_count = 0, current_scene = 0;
    uint16_t current_group = 0;
    bool scene_valid = false;
    settings_save_one("matter/ep1/scenes/count", &scene_count, sizeof(scene_count));
    settings_save_one("matter/ep1/scenes/current", &current_scene, sizeof(current_scene));
    settings_save_one("matter/ep1/scenes/group", &current_group, sizeof(current_group));
    settings_save_one("matter/ep1/scenes/valid", &scene_valid, sizeof(scene_valid));
    
    // OnOff Cluster
    bool onoff_value = onoff_state;
    settings_save_one("matter/ep1/onoff/state", &onoff_value, sizeof(onoff_value));
    uint32_t onoff_features = 0x01;
    settings_save_one("matter/ep1/onoff/features", &onoff_features, sizeof(onoff_features));
    
    // Level Control Cluster
    uint8_t current_level_value = level > 0 ? level : 128;
    uint8_t min_level = 1, max_level = 254;
    uint16_t on_level = 254;
    settings_save_one("matter/ep1/level/current", &current_level_value, sizeof(current_level_value));
    settings_save_one("matter/ep1/level/min", &min_level, sizeof(min_level));
    settings_save_one("matter/ep1/level/max", &max_level, sizeof(max_level));
    settings_save_one("matter/ep1/level/on_level", &on_level, sizeof(on_level));
    
    // Color Control Cluster
    uint8_t color_mode = 0;
    uint16_t color_temp = 250, color_temp_min = 153, color_temp_max = 500;
    settings_save_one("matter/ep1/color/mode", &color_mode, sizeof(color_mode));
    settings_save_one("matter/ep1/color/temp", &color_temp, sizeof(color_temp));
    settings_save_one("matter/ep1/color/temp_min", &color_temp_min, sizeof(color_temp_min));
    settings_save_one("matter/ep1/color/temp_max", &color_temp_max, sizeof(color_temp_max));
    
    // Load fabric table
    ssize_t len = settings_get_val_len("matter/fabric/count");
    if (len > 0) {
        commissioned_ = true;
    }
    
    // Configure commissioning parameters
    uint16_t discriminator = 3840;
    uint32_t setup_pin = 20202021;
    settings_save_one("matter/config/discriminator", &discriminator, sizeof(discriminator));
    settings_save_one("matter/config/setup_pin", &setup_pin, sizeof(setup_pin));
    
    // Commit to storage
    ret = settings_save();
    if (ret < 0) {
        LOG_ERR("Failed to save Matter configuration: %d", ret);
        return ret;
    }
    
    LOG_INF("Matter stack initialized (2 endpoints, 16 clusters)");
    return 0;
}
