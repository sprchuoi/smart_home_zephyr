/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include <zephyr/kernel.h>

#define DISPLAY_TASK_STACK_SIZE 2048
#define DISPLAY_TASK_PRIORITY 5

/**
 * @brief Initialize and start the display task thread
 * 
 * @return 0 on success, negative errno on failure
 */
int display_task_start(void);

#endif /* DISPLAY_TASK_H */
