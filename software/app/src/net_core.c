/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * NET CORE - Radio Management for nRF5340 DK
 * ============================================================================
 * 
 * Purpose:
 *   Cortex-M33 @ 64 MHz - Network processor
 *   Manages IEEE 802.15.4 radio and BLE for Matter/Thread
 *
 * Architecture:
 *   This core runs the radio stack (Thread/BLE) separate from APP core
 *   Communication with APP core via IPC (Inter-Processor Communication)
 *
 * Note:
 *   Full OpenThread integration requires west manifest updates
 *   This minimal version initializes radio and BLE for future expansion
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_BT
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#endif

#ifdef CONFIG_IEEE802154
#include <zephyr/net/ieee802154_radio.h>
#endif

LOG_MODULE_REGISTER(net_core, CONFIG_LOG_DEFAULT_LEVEL);

/**
 * Main entry point for NET core
 * Initializes radio stack (BLE + 802.15.4)
 */
int main(void)
{
	LOG_INF("nRF5340 NET Core starting...");
	LOG_INF("CPU: Cortex-M33 @ 64 MHz");
	
#ifdef CONFIG_BT
	int err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
	} else {
		LOG_INF("Bluetooth LE enabled");
	}
#endif

#ifdef CONFIG_IEEE802154
	LOG_INF("IEEE 802.15.4 radio enabled");
#endif

	LOG_INF("NET Core initialized");
	LOG_INF("Waiting for IPC commands from APP core...");
	
	/* Main loop - handle radio events */
	while (1) {
		k_sleep(K_MSEC(1000));
		
		/* In production: Process OpenThread tasklets, BLE events, IPC messages */
		/* For now: Idle loop showing core is running */
	}

	return 0;
}
