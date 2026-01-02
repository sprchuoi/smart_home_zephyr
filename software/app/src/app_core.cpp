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

/* Matter includes */
#include "modules/matter/app_task.hpp"
#include "modules/matter/light_endpoint.hpp"

/* Hardware control */
#include "modules/button/buttonmodule.hpp"
#include "modules/uart/uartmodule.hpp"

LOG_MODULE_REGISTER(app_core, CONFIG_LOG_DEFAULT_LEVEL);

/* Device tree references - 4 LEDs */
/* All 4 LEDs on the board */
static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),  /* P0.28 */
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),  /* P0.29 */
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),  /* P0.30 */
	GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),  /* P0.31 */
};
#define NUM_LEDS 4

/* All 4 buttons on the board - each controls its corresponding LED */
static const struct gpio_dt_spec buttons[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios),  /* P0.23 */
	GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios),  /* P0.24 */
	GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios),  /* P0.25 */
	GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios),  /* P0.26 */
};
#define NUM_BUTTONS 4

/* LED state tracking - each button toggles its LED */
static bool led_state[NUM_LEDS] = {false, false, false, false};

/* Button debounce tracking */
static uint32_t last_button_press_ms[NUM_BUTTONS] = {0};
#define DEBOUNCE_MS 100

/* Threads for button handling */
static struct k_thread button_thread_data[NUM_BUTTONS];
static k_tid_t button_thread_id[NUM_BUTTONS];
static K_THREAD_STACK_DEFINE(button_stack_0, 512);
static K_THREAD_STACK_DEFINE(button_stack_1, 512);
static K_THREAD_STACK_DEFINE(button_stack_2, 512);
static K_THREAD_STACK_DEFINE(button_stack_3, 512);
static k_thread_stack_t *button_stacks[] = {
	button_stack_0, button_stack_1, button_stack_2, button_stack_3
};

/*
 * Button callback - toggle corresponding LED
 * cb->pin_mask tells us which button was pressed
 */
static void button_pressed_callback(const struct device *dev, 
                                   struct gpio_callback *cb, 
                                   uint32_t pins)
{
	/* Find which button (0-3) was pressed */
	int button_idx = -1;
	for (int i = 0; i < NUM_BUTTONS; i++) {
		if (buttons[i].port == dev) {
			button_idx = i;
			break;
		}
	}
	
	if (button_idx < 0) {
		return;
	}
	
	uint32_t now = k_uptime_get_32();
	
	/* Debounce: ignore presses within DEBOUNCE_MS */
	if (now - last_button_press_ms[button_idx] < DEBOUNCE_MS) {
		return;
	}
	last_button_press_ms[button_idx] = now;
	
	/* Toggle the LED for this button */
	led_state[button_idx] = !led_state[button_idx];
	gpio_pin_set_dt(&leds[button_idx], led_state[button_idx] ? 1 : 0);
	
	LOG_INF("Button %d pressed - LED%d toggled to %s", 
	        button_idx, button_idx, led_state[button_idx] ? "ON" : "OFF");
}

/*
 * Button monitoring thread - one per button
 */
static void button_thread_func(void *arg1, void *arg2, void *arg3)
{
	int button_idx = (int)(intptr_t)arg1;
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	
	LOG_INF("Button %d thread started", button_idx);
	
	const struct gpio_dt_spec *btn = &buttons[button_idx];
	
	if (!device_is_ready(btn->port)) {
		LOG_ERR("Button %d device not ready", button_idx);
		return;
	}
	
	int ret = gpio_pin_configure_dt(btn, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure button %d: %d", button_idx, ret);
		return;
	}
	
	static struct gpio_callback button_cb_data[NUM_BUTTONS];
	gpio_init_callback(&button_cb_data[button_idx], button_pressed_callback, BIT(btn->pin));
	
	ret = gpio_add_callback(btn->port, &button_cb_data[button_idx]);
	if (ret < 0) {
		LOG_ERR("Failed to add button %d callback: %d", button_idx, ret);
		return;
	}
	
	ret = gpio_pin_interrupt_configure_dt(btn, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure button %d interrupt: %d", button_idx, ret);
		return;
	}
	
	LOG_INF("Button %d interrupt configured on P0.%d", button_idx, btn->pin);
	
	/* Thread keeps running for interrupt handling */
	while (1) {
		k_sleep(K_FOREVER);
	}
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
	ret = Matter::AppTask::getInstance().init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize Matter AppTask: %d", ret);
		return ret;
	}
	LOG_INF("Matter stack initialized");
	
	/* Initialize Light Endpoint */
	LOG_INF("Initializing Light Endpoint...");
	ret = Matter::LightEndpoint::getInstance().init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize Light Endpoint: %d", ret);
		return ret;
	}
	LOG_INF("Light Endpoint initialized");
	
	/* Create button monitoring threads - one for each button */
	LOG_INF("Starting 4 button monitoring threads...");
	for (int i = 0; i < NUM_BUTTONS; i++) {
		button_thread_id[i] = k_thread_create(
			&button_thread_data[i],
			button_stacks[i],
			K_THREAD_STACK_SIZEOF(button_stack_0),
			button_thread_func,
			(void *)(intptr_t)i, NULL, NULL,
			K_PRIO_COOP(7),
			0,
			K_NO_WAIT
		);
		
		if (!button_thread_id[i]) {
			LOG_ERR("Failed to create button %d thread", i);
			return -ENOMEM;
		}
		
		char thread_name[16];
		snprintf(thread_name, sizeof(thread_name), "button_%d", i);
		k_thread_name_set(button_thread_id[i], thread_name);
		LOG_INF("Button %d thread created", i);
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
		Matter::AppTask::getInstance().dispatchEvent();
		
		/* Yield to other threads */
		k_sleep(K_MSEC(10));
	}
	
	return 0;
}

/* Initialize after POST_KERNEL phase */
SYS_INIT(app_core_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
