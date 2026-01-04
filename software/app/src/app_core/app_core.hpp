#ifndef _APP_CORE_HPP_
#define _APP_CORE_HPP_
/*
 * ============================================================================
 * APP CORE - Matter Smart Light Application
 * ============================================================================
 * 
 * Purpose:
 *   Cortex-M33 @ 128 MHz - Matter Light Device with 4 LEDs
 *   Implements Matter protocol + hardware
 * control layer
 *  Manages LED state machine and user input (button)
 * Hardware:
 *  LED0-3: P0.28-31 (4 green LEDs on DK)
 *  Button: P0.23-26 (4 buttons on DK)
 *  UART:   P0.20/22 (debug @ 115200 baud)
 * Features:
 *  - Matter OnOff + Level Control clusters
 *  - 4 LED brightness control
 *  - Button with 100ms debounce
 *  - LED state synchronization 
 * - BLE commissioning (shared radio)
 */

 /*
 * Copyright (c) 2025 Sprchuoi, Nordic Semiconductor ASA
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
#include "sdk/protocol/matter/control_app/app_task.hpp"
#include "sdk/protocol/matter/light_endpoint/light_endpoint.hpp"

/* Hardware abstraction layer */
#include "sdk/hw/button/button_manager.hpp"
#include "sdk/hw/uart/uart_manager.hpp"

/* IPC for inter-core communication */
#include "sdk/ipc/ipc_core.hpp"

typedef enum {
    APP_OK = 1,
    APP_ERROR = 0,
} app_status_t; 


#endif /* _APP_CORE_HPP_ */