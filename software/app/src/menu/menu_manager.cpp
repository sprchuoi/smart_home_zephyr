/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "menu_manager.hpp"
#if defined(CONFIG_WIFI)
#include "../modules/wifi/wifiservice.hpp"
#endif
#if defined(CONFIG_BT)
#include "../modules/ble/bleservice.hpp"
#endif
#include "../modules/display/displaymodule.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(menu_manager, CONFIG_APP_LOG_LEVEL);

/* Callback functions for menu actions */

#if defined(CONFIG_WIFI)
/* WiFi menu actions */
static void wifi_connect_action() {
    LOG_INF("WiFi Connect selected");
    // Implement WiFi connection dialog
}

static void wifi_disconnect_action() {
    LOG_INF("WiFi Disconnect selected");
    WiFiService::getInstance().disconnect();
}

static void wifi_scan_result_cb(struct wifi_scan_result *entry) {
    if (entry) {
        LOG_INF("WiFi: %s (RSSI: %d)", entry->ssid, entry->rssi);
    }
}

static void wifi_scan_action() {
    LOG_INF("WiFi Scan selected");
    WiFiService::getInstance().scan(wifi_scan_result_cb);
}

static void wifi_ap_start_action() {
    LOG_INF("WiFi AP Start selected");
    WiFiService::getInstance().startAP(
        WiFiService::DEFAULT_SSID,
        WiFiService::DEFAULT_PASSWORD
    );
}

static void wifi_ap_stop_action() {
    LOG_INF("WiFi AP Stop selected");
    WiFiService::getInstance().stopAP();
}

/* Value getters for status display */
static const char* wifi_status_getter() {
    return WiFiService::getInstance().isConnected() ? "Connected" : "Disconnected";
}
#endif /* CONFIG_WIFI */

#if defined(CONFIG_BT)
/* BLE menu actions */
static void ble_start_adv_action() {
    LOG_INF("BLE Start Advertising selected");
    BleService::getInstance().startAdvertising();
}

static void ble_stop_adv_action() {
    LOG_INF("BLE Stop Advertising selected");
    BleService::getInstance().stopAdvertising();
}

static const char* ble_status_getter() {
    return BleService::getInstance().isConnected() ? "Connected" : "Idle";
}
#endif /* CONFIG_BT */

/* Display menu actions */
static void display_wake_action() {
    LOG_INF("Display Wake selected");
    DisplayModule::getInstance().wake();
}

static void display_sleep_action() {
    LOG_INF("Display Sleep selected");
    DisplayModule::getInstance().sleep();
}

/* System menu actions */
static void system_info_action() {
    LOG_INF("System Info selected");
    // Display system information
}

static void system_reset_action() {
    LOG_INF("System Reset selected");
    // Implement system reset
}

/* Display status getter */
static const char* display_status_getter() {
    return DisplayModule::getInstance().isSleeping() ? "Sleeping" : "Active";
}

/* MenuManager implementation */
MenuManager::MenuManager()
    : main_menu_(nullptr)
    , app_menu_(nullptr)
    , services_menu_(nullptr)
    , modules_menu_(nullptr)
{
}

MenuManager& MenuManager::getInstance()
{
    static MenuManager instance;
    return instance;
}

int MenuManager::init()
{
    LOG_INF("Initializing menu manager");
    
    createMainMenu();
    createAppMenu();
    createServicesMenu();
    createModulesMenu();
    
    LOG_INF("Menu manager initialized");
    return 0;
}

void MenuManager::createMainMenu()
{
    MenuSystem& menu_sys = MenuSystem::getInstance();
    
    // Create main menu (root)
    main_menu_ = menu_sys.createMenuItem("Main Menu", MenuItemType::SUBMENU);
    if (!main_menu_) {
        LOG_ERR("Failed to create main menu");
        return;
    }
    
    // Set as root menu
    menu_sys.setRootMenu(main_menu_);
    
    // Create main menu items
    MenuItem* app_item = menu_sys.createMenuItem("Application", MenuItemType::SUBMENU);
    MenuItem* services_item = menu_sys.createMenuItem("Services", MenuItemType::SUBMENU);
    MenuItem* modules_item = menu_sys.createMenuItem("Modules", MenuItemType::SUBMENU);
    MenuItem* system_item = menu_sys.createMenuItem("System", MenuItemType::SUBMENU);
    
    if (!app_item || !services_item || !modules_item || !system_item) {
        LOG_ERR("Failed to create main menu items");
        return;
    }
    
    // Add to main menu
    menu_sys.addMenuItem(main_menu_, app_item);
    menu_sys.addMenuItem(main_menu_, services_item);
    menu_sys.addMenuItem(main_menu_, modules_item);
    menu_sys.addMenuItem(main_menu_, system_item);
    
    // Store references
    app_menu_ = app_item;
    services_menu_ = services_item;
    modules_menu_ = modules_item;
    
    // Create system submenu
    MenuItem* sys_info = menu_sys.createMenuItem("System Info", MenuItemType::ACTION);
    MenuItem* sys_reset = menu_sys.createMenuItem("Reset", MenuItemType::ACTION);
    MenuItem* sys_back = menu_sys.createMenuItem("< Back", MenuItemType::BACK);
    
    if (sys_info && sys_reset && sys_back) {
        sys_info->setCallback(system_info_action);
        sys_reset->setCallback(system_reset_action);
        
        menu_sys.addMenuItem(system_item, sys_info);
        menu_sys.addMenuItem(system_item, sys_reset);
        menu_sys.addMenuItem(system_item, sys_back);
    }
}

void MenuManager::createAppMenu()
{
    if (!app_menu_) {
        return;
    }
    
    MenuSystem& menu_sys = MenuSystem::getInstance();
    
    // Create application menu items
    MenuItem* app_status = menu_sys.createMenuItem("Status", MenuItemType::ACTION);
    MenuItem* app_settings = menu_sys.createMenuItem("Settings", MenuItemType::SUBMENU);
    MenuItem* app_back = menu_sys.createMenuItem("< Back", MenuItemType::BACK);
    
    if (app_status && app_settings && app_back) {
        app_status->setCallback(system_info_action);
        
        menu_sys.addMenuItem(app_menu_, app_status);
        menu_sys.addMenuItem(app_menu_, app_settings);
        menu_sys.addMenuItem(app_menu_, app_back);
    }
}

void MenuManager::createServicesMenu()
{
    if (!services_menu_) {
        return;
    }
    
    MenuSystem& menu_sys = MenuSystem::getInstance();
    
#if defined(CONFIG_WIFI)
    // Create WiFi submenu
    MenuItem* wifi_menu = menu_sys.createMenuItem("WiFi", MenuItemType::SUBMENU);
    menu_sys.addMenuItem(services_menu_, wifi_menu);
    
    if (wifi_menu) {
        MenuItem* wifi_status = menu_sys.createMenuItem("Status", MenuItemType::VALUE);
        MenuItem* wifi_connect = menu_sys.createMenuItem("Connect", MenuItemType::ACTION);
        MenuItem* wifi_disconnect = menu_sys.createMenuItem("Disconnect", MenuItemType::ACTION);
        MenuItem* wifi_scan = menu_sys.createMenuItem("Scan", MenuItemType::ACTION);
        MenuItem* wifi_ap_start = menu_sys.createMenuItem("Start AP", MenuItemType::ACTION);
        MenuItem* wifi_ap_stop = menu_sys.createMenuItem("Stop AP", MenuItemType::ACTION);
        MenuItem* wifi_back = menu_sys.createMenuItem("< Back", MenuItemType::BACK);
        
        if (wifi_status) {
            wifi_status->setValueGetter(wifi_status_getter);
            menu_sys.addMenuItem(wifi_menu, wifi_status);
        }
        if (wifi_connect) {
            wifi_connect->setCallback(wifi_connect_action);
            menu_sys.addMenuItem(wifi_menu, wifi_connect);
        }
        if (wifi_disconnect) {
            wifi_disconnect->setCallback(wifi_disconnect_action);
            menu_sys.addMenuItem(wifi_menu, wifi_disconnect);
        }
        if (wifi_scan) {
            wifi_scan->setCallback(wifi_scan_action);
            menu_sys.addMenuItem(wifi_menu, wifi_scan);
        }
        if (wifi_ap_start) {
            wifi_ap_start->setCallback(wifi_ap_start_action);
            menu_sys.addMenuItem(wifi_menu, wifi_ap_start);
        }
        if (wifi_ap_stop) {
            wifi_ap_stop->setCallback(wifi_ap_stop_action);
            menu_sys.addMenuItem(wifi_menu, wifi_ap_stop);
        }
        if (wifi_back) {
            menu_sys.addMenuItem(wifi_menu, wifi_back);
        }
    }
#endif /* CONFIG_WIFI */
    
#if defined(CONFIG_BT)
    // Create BLE submenu
    MenuItem* ble_menu = menu_sys.createMenuItem("BLE", MenuItemType::SUBMENU);
    menu_sys.addMenuItem(services_menu_, ble_menu);
    
    if (ble_menu) {
        MenuItem* ble_status = menu_sys.createMenuItem("Status", MenuItemType::VALUE);
        MenuItem* ble_start_adv = menu_sys.createMenuItem("Start Advertising", MenuItemType::ACTION);
        MenuItem* ble_stop_adv = menu_sys.createMenuItem("Stop Advertising", MenuItemType::ACTION);
        MenuItem* ble_back = menu_sys.createMenuItem("< Back", MenuItemType::BACK);
        
        if (ble_status) {
            ble_status->setValueGetter(ble_status_getter);
            menu_sys.addMenuItem(ble_menu, ble_status);
        }
        if (ble_start_adv) {
            ble_start_adv->setCallback(ble_start_adv_action);
            menu_sys.addMenuItem(ble_menu, ble_start_adv);
        }
        if (ble_stop_adv) {
            ble_stop_adv->setCallback(ble_stop_adv_action);
            menu_sys.addMenuItem(ble_menu, ble_stop_adv);
        }
        if (ble_back) {
            menu_sys.addMenuItem(ble_menu, ble_back);
        }
    }
#endif /* CONFIG_BT */
    
    // Back to main menu
    MenuItem* services_back = menu_sys.createMenuItem("< Back", MenuItemType::BACK);
    if (services_back) {
        menu_sys.addMenuItem(services_menu_, services_back);
    }
}

void MenuManager::createModulesMenu()
{
    if (!modules_menu_) {
        return;
    }
    
    MenuSystem& menu_sys = MenuSystem::getInstance();
    
    // Create Display submenu
    MenuItem* display_menu = menu_sys.createMenuItem("Display", MenuItemType::SUBMENU);
    menu_sys.addMenuItem(modules_menu_, display_menu);
    
    if (display_menu) {
        MenuItem* display_status = menu_sys.createMenuItem("Status", MenuItemType::VALUE);
        MenuItem* display_wake = menu_sys.createMenuItem("Wake", MenuItemType::ACTION);
        MenuItem* display_sleep = menu_sys.createMenuItem("Sleep", MenuItemType::ACTION);
        MenuItem* display_back = menu_sys.createMenuItem("< Back", MenuItemType::BACK);
        
        if (display_status) {
            display_status->setValueGetter(display_status_getter);
            menu_sys.addMenuItem(display_menu, display_status);
        }
        if (display_wake) {
            display_wake->setCallback(display_wake_action);
            menu_sys.addMenuItem(display_menu, display_wake);
        }
        if (display_sleep) {
            display_sleep->setCallback(display_sleep_action);
            menu_sys.addMenuItem(display_menu, display_sleep);
        }
        if (display_back) {
            menu_sys.addMenuItem(display_menu, display_back);
        }
    }
    
    // Back to main menu
    MenuItem* modules_back = menu_sys.createMenuItem("< Back", MenuItemType::BACK);
    if (modules_back) {
        menu_sys.addMenuItem(modules_menu_, modules_back);
    }
}

void MenuManager::addAppMenuItem(const char* label, MenuCallback callback)
{
    if (!app_menu_) {
        return;
    }
    
    MenuSystem& menu_sys = MenuSystem::getInstance();
    MenuItem* item = menu_sys.createMenuItem(label, MenuItemType::ACTION);
    
    if (item) {
        item->setCallback(callback);
        menu_sys.addMenuItem(app_menu_, item);
    }
}

void MenuManager::addServiceMenuItem(const char* label, MenuCallback callback)
{
    if (!services_menu_) {
        return;
    }
    
    MenuSystem& menu_sys = MenuSystem::getInstance();
    MenuItem* item = menu_sys.createMenuItem(label, MenuItemType::ACTION);
    
    if (item) {
        item->setCallback(callback);
        menu_sys.addMenuItem(services_menu_, item);
    }
}

void MenuManager::addModuleMenuItem(const char* label, MenuCallback callback)
{
    if (!modules_menu_) {
        return;
    }
    
    MenuSystem& menu_sys = MenuSystem::getInstance();
    MenuItem* item = menu_sys.createMenuItem(label, MenuItemType::ACTION);
    
    if (item) {
        item->setCallback(callback);
        menu_sys.addMenuItem(modules_menu_, item);
    }
}
