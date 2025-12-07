/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLINK_TASK_H
#define BLINK_TASK_H

#include <zephyr/kernel.h>

#define BLINK_TASK_STACK_SIZE 1024
#define BLINK_TASK_PRIORITY 5

/**
 * @brief Initialize and start the blink task thread
 * 
 * @return 0 on success, negative errno on failure
 */
int blink_task_start(void);

#endif /* BLINK_TASK_H */
