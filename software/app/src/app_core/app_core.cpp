/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * APP CORE - Matter Smart Light Application
 * ============================================================================
 * 
 * Purpose:
 *   Cortex-M33 @ 128 MHz - Matter Light Device with 4 LEDs
 *   Implements Matter protocol + hardware control layer
 *   Manages LED state machine and user input (button)
 *
 * Hardware:
 *   LED0-3: P0.28-31 (4 green LEDs on DK)
 *   Button: P0.23-26 (4 buttons on DK)
 *   UART:   P0.20/22 (debug @ 115200 baud)
 *
 * Features:
 *   - Matter OnOff + Level Control clusters
 *   - 4 LED brightness control
 *   - Button with 100ms debounce
 *   - LED state synchronization
 *   - BLE commissioning (shared radio)
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <app_version.h>
#include <stdint.h>
#include <stdio.h>

/* Protocol layer includes */
#include "sdk/protocol/matter/app_task.hpp"
#include "sdk/protocol/matter/light_endpoint.hpp"

/* Hardware abstraction layer */
#include "sdk/hw/button/button_manager.hpp"
#include "sdk/hw/uart/uart_manager.hpp"

/* IPC for inter-core communication */
#include "sdk/ipc/ipc_core.hpp"

LOG_MODULE_REGISTER(app_core, CONFIG_LOG_DEFAULT_LEVEL);

/*=============================================================================
 * LED Management
 *===========================================================================*/

/* Device tree references - 4 LEDs */
static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),  /* P0.28 */
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),  /* P0.29 */
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),  /* P0.30 */
	GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),  /* P0.31 */
};
#define NUM_LEDS 4

/* LED state tracking */
static bool led_state[NUM_LEDS] = {false, false, false, false};

/*=============================================================================
 * Button Event Handler - Uses ButtonManager Module
 *===========================================================================*/

/**
 * @brief Button press callback - toggles corresponding LED
 * @param button_id Button index (0-3)
 */
static void on_button_pressed(uint8_t button_id)
{
	if (button_id >= NUM_LEDS) {
		return;
	}
	
	/* Toggle LED state */
	led_state[button_id] = !led_state[button_id];
	gpio_pin_set_dt(&leds[button_id], led_state[button_id] ? 1 : 0);
	
	LOG_INF("Button %d pressed - LED%d toggled to %s", 
	        button_id, button_id, led_state[button_id] ? "ON" : "OFF");
	
	/* Notify Matter stack of state change */
	smarthome::protocol::matter::LightEndpoint::getInstance().setLightState(led_state[button_id]);
}

/*
 * IPC message handlers - receive data from NET core
 */
static void handle_status_response(const smarthome::ipc::Message& msg) {
	LOG_INF("Received status from NET core: 0x%08x", 
	        msg.payload.status.status_code);
	
	/* Update local state based on NET core status */
	if (msg.payload.status.status_code == 0) {
		LOG_INF("NET core is healthy");
	}
}

static void handle_ble_event(const smarthome::ipc::Message& msg) {
	LOG_INF("BLE event from NET core: type=0x%02x", (uint8_t)msg.type);
	
	/* Process BLE events (connections, disconnections, etc.) */
}

static void handle_radio_event(const smarthome::ipc::Message& msg) {
	LOG_DBG("Radio event from NET core");
	
	/* Process radio events (TX complete, RX data, etc.) */
}

/*
 * Initialize IPC and register callbacks
 */
static int init_ipc(void) {
	LOG_INF("Initializing IPC...");
	
	auto& ipc = smarthome::ipc::IPCCore::getInstance();
	
	int ret = ipc.init();
	if (ret < 0) {
		LOG_ERR("IPC init failed: %d", ret);
		return ret;
	}
	
	/* Register callbacks for messages from NET core */
	ipc.registerCallback(smarthome::ipc::MessageType::STATUS_RESPONSE, handle_status_response);
	ipc.registerCallback(smarthome::ipc::MessageType::BLE_CONNECT, handle_ble_event);
	ipc.registerCallback(smarthome::ipc::MessageType::BLE_DISCONNECT, handle_ble_event);
	ipc.registerCallback(smarthome::ipc::MessageType::RADIO_RX, handle_radio_event);
	
	LOG_INF("IPC initialized successfully");
	
	/* Send initial status request to NET core */
	auto msg = smarthome::ipc::MessageBuilder(smarthome::ipc::MessageType::STATUS_REQUEST)
	              .setPriority(smarthome::ipc::Priority::NORMAL)
	              .build();
	
	ret = ipc.send(msg, 1000);
	if (ret < 0) {
		LOG_WRN("Failed to send initial status request: %d", ret);
	} else {
		LOG_INF("Sent status request to NET core");
	}
	
	return 0;
}

/*
 * APP Core initialization
 */
static int app_core_init(void)
{
	int ret;
	
	/* Early UART test - this should always print if UART is working */
	printk("\n\n*** nRF5340 DK Booting ***\n");
	printk("*** APP Core Starting ***\n\n");
	
	LOG_INF("============================================");
	LOG_INF("  nRF5340 DK Matter Smart Light (APP Core)  ");
	LOG_INF("  Version: %s", APP_VERSION_STRING);
	LOG_INF("  Cortex-M33 @ 128 MHz");
	LOG_INF("  4 LEDs: P0.28-31");
	LOG_INF("============================================");
	
	/* Initialize IPC first for inter-core communication */
	ret = init_ipc();
	if (ret < 0) {
		LOG_ERR("IPC initialization failed: %d", ret);
		/* Continue anyway - IPC is optional */
	}
	
	/* Initialize all 4 LEDs and turn them ON to verify GPIO works */
	for (int i = 0; i < NUM_LEDS; i++) {
		if (!device_is_ready(leds[i].port)) {
			LOG_ERR("LED%d device not ready", i);
			return -ENODEV;
		}
		
		ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure LED%d: %d", i, ret);
			return ret;
		}
		
		/* Turn LED ON */
		led_state[i] = true;
		gpio_pin_set_dt(&leds[i], 1);
		LOG_INF("LED%d initialized and turned ON (P0.%d)", i, 28 + i);
	}
	LOG_INF("All 4 LEDs turned ON - verify all 4 green lights are visible");
	
	/* Initialize Matter application task */
	LOG_INF("Initializing Matter stack...");
	ret = smarthome::protocol::matter::AppTask::getInstance().init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize Matter AppTask: %d", ret);
		return ret;
	}
	LOG_INF("Matter stack initialized");
	
	/* Initialize Light Endpoint */
	LOG_INF("Initializing Light Endpoint...");
	ret = smarthome::protocol::matter::LightEndpoint::getInstance().init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize Light Endpoint: %d", ret);
		return ret;
	}
	LOG_INF("Light Endpoint initialized");
	
	/* Initialize Button Manager module */
	LOG_INF("Initializing button manager...");
	auto& btnMgr = smarthome::hw::ButtonManager::getInstance();
	ret = btnMgr.init();
	if (ret < 0) {
		LOG_WRN("Button manager initialization failed: %d", ret);
	} else {
		/* Register callbacks for each button */
		for (uint8_t i = 0; i < btnMgr.getButtonCount() && i < NUM_LEDS; i++) {
			btnMgr.registerCallback(i, on_button_pressed);
			LOG_INF("Button %d callback registered", i);
		}
		LOG_INF("Button manager initialized with %d button(s)", btnMgr.getButtonCount());
	}
	
	LOG_INF("APP Core initialization complete!");
	LOG_INF("Waiting for Matter commissioning...");
	
	return 0;
}

/*
 * APP Core main loop
 */
extern "C" int main(void)
{
	int ret = app_core_init();
	if (ret < 0) {
		LOG_ERR("APP Core initialization failed: %d", ret);
		return ret;
	}
	
	LOG_INF("APP Core main loop started");
	LOG_INF("Listening for Matter events and button input...");
	
	/* Main loop - process Matter events */
	while (1) {
		/* Matter event processing */
		smarthome::protocol::matter::AppTask::getInstance().dispatchEvent();
		
		/* Yield to other threads */
		k_sleep(K_MSEC(10));
	}
	
	return 0;
}

/* Initialize after POST_KERNEL phase */
SYS_INIT(app_core_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
