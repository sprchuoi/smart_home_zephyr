/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "button_module.h"

LOG_MODULE_REGISTER(button_module, CONFIG_APP_LOG_LEVEL);

/* Button GPIO - GPIO0 on ESP32 (BOOT button) */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback button_cb_data;
static button_callback_t event_callback = NULL;
static bool button_pressed = false;

static void button_isr_handler(const struct device *dev, struct gpio_callback *cb,
			       uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	/* Read button state */
	int val = gpio_pin_get_dt(&button);
	button_pressed = (val == 0);  /* Active low */

	/* Call registered callback */
	if (event_callback) {
		event_callback(button_pressed);
	}

	if (button_pressed) {
		LOG_INF("Button pressed");
	} else {
		LOG_DBG("Button released");
	}
}

int button_module_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("Button GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Failed to configure button pin (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	if (ret) {
		LOG_ERR("Failed to configure button interrupt (%d)", ret);
		return ret;
	}

	gpio_init_callback(&button_cb_data, button_isr_handler, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	LOG_INF("Button module initialized");
	return 0;
}

int button_module_register_callback(button_callback_t callback)
{
	if (!callback) {
		return -EINVAL;
	}

	event_callback = callback;
	LOG_INF("Button callback registered");
	return 0;
}

bool button_module_is_pressed(void)
{
	return button_pressed;
}
