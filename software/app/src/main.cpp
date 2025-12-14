/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <app_version.h>

// C++ Module includes
#include "modules/blink/blinkmodule.hpp"
#include "modules/sensor/sensormodule.hpp"
#if defined(CONFIG_BT)
#include "modules/ble/bleservice.hpp"
#endif
#if defined(CONFIG_WIFI)
#include "modules/wifi/wifiservice.hpp"
#endif
#include "modules/display/displaymodule.hpp"
#include "modules/button/buttonmodule.hpp"
#include "modules/uart/uartmodule.hpp"

// Core infrastructure
#include "menu/menu.hpp"
#include "menu/menu_manager.hpp"

// Task headers
#include "thread/blink_task.h"
#include "thread/sensor_task.h"
#if defined(CONFIG_BT)
#include "thread/ble_task.h"
#endif
#if defined(CONFIG_WIFI)
#include "thread/wifi_task.h"
#endif
#include "thread/display_task.h"
#include "thread/uart_task.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

/// ESP32-WROOM-32 with C++ OOP Architecture

// Forward declarations
static void Os_Init(void);
static void Os_Start(void);
static void menu_display_callback(MenuItem* menu, MenuItem* selected);

// Menu display callback wrapper
static void menu_display_callback(MenuItem* menu, MenuItem* selected)
{
	DisplayModule::getInstance().renderMenu(menu, selected);
}

extern "C" int main(void)
{
	printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

	Os_Init(); // Initialize OS and modules
	
	Os_Start(); // Start OS threads

	/* Main thread sleeps */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}

static void Os_Init(void)
{
	int ret;
	
	// Zephyr OS initializes itself automatically.
	printk("Init OS and modules...\n");
	// Initialize all modules using C++ singletons
	ret = BlinkModule::getInstance().init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize blink module (%d)", ret);
		return;
	}

	ret = SensorModule::getInstance().init();
	if (ret < 0) {
		LOG_WRN("Sensor module not available (%d) - continuing without sensor", ret);
	}

#if defined(CONFIG_BT)
	ret = BleService::getInstance().init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize BLE Service (%d)", ret);
		return;
	}
#endif

#if defined(CONFIG_WIFI)
	ret = WiFiService::getInstance().init(WiFiService::Mode::AP_STA);
	if (ret < 0) {
		LOG_ERR("Failed to initialize WiFi service (%d)", ret);
		return;
	}
#endif

	ret = DisplayModule::getInstance().init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize display module (%d)", ret);
		return;
	}

	ret = ButtonModule::getInstance().init();
	if (ret < 0) {
		LOG_WRN("Button module not available (%d) - continuing without button", ret);
	}

	// Initialize menu system
	ret = MenuManager::getInstance().init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize menu manager (%d)", ret);
		return;
	}
	
	// Set display callback for menu system
	MenuSystem::getInstance().setDisplayCallback(menu_display_callback);

	LOG_INF("All modules initialized");
	printk("Use the sensor to change LED blinking period\n");
} 


static void Os_Start(void)
{
	int ret;
	
	printk("\n*** Starting all tasks ***\n");
	LOG_INF("Starting all tasks");
	
	// Zephyr OS starts its own scheduler automatically.
	// Start task threads
	ret = blink_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start blink task (%d)", ret);
		printk("FAILED to start blink task: %d\n", ret);
		return;
	}
	printk("Blink task created\n");

	ret = sensor_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start sensor task (%d)", ret);
		printk("FAILED to start sensor task: %d\n", ret);
		return;
	}
	printk("Sensor task created\n");

#if defined(CONFIG_BT)
	ret = ble_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start BLE task (%d)", ret);
		printk("FAILED to start BLE task: %d\n", ret);
		return;
	}
	printk("BLE task created\n");
#endif

#if defined(CONFIG_WIFI)
	ret = wifi_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start WiFi task (%d)", ret);
		printk("FAILED to start WiFi task: %d\n", ret);
		return;
	}
	printk("WiFi task created\n");
#endif

	ret = display_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start display task (%d)", ret);
		printk("FAILED to start display task: %d\n", ret);
		return;
	}
	printk("Display task created\n");

	ret = uart_task_start();
	if (ret < 0) {
		LOG_ERR("Failed to start UART task (%d)", ret);
		printk("FAILED to start UART task: %d\n", ret);
		return;
	}
	printk("UART task created\n");
	

	LOG_INF("All tasks started successfully");
	printk("*** All tasks started successfully ***\n\n");
}


