/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "displaymodule.hpp"
#include "../../menu/menu.hpp"
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(display_module_cpp, CONFIG_APP_LOG_LEVEL);

#if DT_HAS_CHOSEN(zephyr_display)
#define DISPLAY_DEV DEVICE_DT_GET(DT_CHOSEN(zephyr_display))
#endif

DisplayModule& DisplayModule::getInstance() {
    static DisplayModule instance;
    return instance;
}

DisplayModule::DisplayModule()
    : sleeping_(false)
    , last_activity_(0)
{
    k_mutex_init(&mutex_);
}

int DisplayModule::init() {
#if DT_HAS_CHOSEN(zephyr_display)
    const struct device *display_dev = DISPLAY_DEV;
    
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return -ENODEV;
    }
    
    if (display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10) != 0) {
        LOG_ERR("Failed to set pixel format");
        return -EIO;
    }
    
    if (cfb_framebuffer_init(display_dev)) {
        LOG_ERR("Framebuffer init failed");
        return -EIO;
    }
    
    cfb_framebuffer_clear(display_dev, true);
    
    LOG_INF("Display module initialized");
    last_activity_ = k_uptime_get();
    return 0;
#else
    LOG_WRN("Display not configured in device tree");
    return -ENOTSUP;
#endif
}

void DisplayModule::wake() {
#if DT_HAS_CHOSEN(zephyr_display)
    const struct device *display_dev = DISPLAY_DEV;
    
    if (!device_is_ready(display_dev)) {    
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    if (sleeping_) {
        display_blanking_off(display_dev);
        sleeping_ = false;
        LOG_INF("Display woke up");
    }
    
    last_activity_ = k_uptime_get();
    k_mutex_unlock(&mutex_);
#endif
}

void DisplayModule::sleep() {
#if DT_HAS_CHOSEN(zephyr_display)
    const struct device *display_dev = DISPLAY_DEV;
    
    if (!device_is_ready(display_dev)) {
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    if (!sleeping_) {
        display_blanking_on(display_dev);
        sleeping_ = true;
        LOG_INF("Display went to sleep");
    }
    
    k_mutex_unlock(&mutex_);
#endif
}

void DisplayModule::updateStatus(const char* line1, const char* line2, const char* line3) {
#if DT_HAS_CHOSEN(zephyr_display)
    const struct device *display_dev = DISPLAY_DEV;
    
    if (!device_is_ready(display_dev) || sleeping_) {
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    cfb_framebuffer_clear(display_dev, false);
    
    if (line1) cfb_print(display_dev, line1, 0, 0);
    if (line2) cfb_print(display_dev, line2, 0, 16);
    if (line3) cfb_print(display_dev, line3, 0, 32);
    
    cfb_framebuffer_finalize(display_dev);
    
    last_activity_ = k_uptime_get();
    k_mutex_unlock(&mutex_);
#endif
}

void DisplayModule::renderMenu(MenuItem* menu, MenuItem* selected) {
#if DT_HAS_CHOSEN(zephyr_display)
    const struct device *display_dev = DISPLAY_DEV;
    
    if (!device_is_ready(display_dev) || sleeping_) {
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    cfb_framebuffer_clear(display_dev, false);
    
    // Display menu title
    if (menu) {
        char title[32];
        snprintf(title, sizeof(title), "=== %s ===", menu->getLabel());
        cfb_print(display_dev, title, 0, 0);
    }
    
    // Display menu items (up to 3 visible)
    if (menu && menu->getSubmenu()) {
        MenuItem* item = menu->getSubmenu();
        int y_pos = 16;
        int line_count = 0;
        
        // Find the first item to display (centered around selected)
        MenuItem* display_start = item;
        if (selected) {
            // Try to show selected in the middle
            MenuItem* temp = selected;
            for (int i = 0; i < 1 && temp->getPrev(); i++) {
                temp = temp->getPrev();
            }
            display_start = temp;
        }
        
        // Display up to 3 items
        item = display_start;
        while (item && line_count < 3) {
            char line[32];
            const char* prefix = (item == selected) ? "> " : "  ";
            
            // Format based on item type
            if (item->getType() == MenuItemType::VALUE) {
                const char* value = item->getValue();
                snprintf(line, sizeof(line), "%s%s: %s", 
                        prefix, item->getLabel(), value ? value : "");
            } else {
                snprintf(line, sizeof(line), "%s%s", prefix, item->getLabel());
            }
            
            cfb_print(display_dev, line, 0, y_pos);
            y_pos += 16;
            line_count++;
            item = item->getNext();
        }
    }
    
    cfb_framebuffer_finalize(display_dev);
    
    last_activity_ = k_uptime_get();
    k_mutex_unlock(&mutex_);
#endif
}
