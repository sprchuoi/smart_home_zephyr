/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_LOG_H
#define APP_LOG_H

/*
 * Common log level for application modules
 * Use this instead of CONFIG_APP_LOG_LEVEL which is auto-generated
 */

#ifndef APP_LOG_LEVEL
#ifdef CONFIG_APP_LOG_LEVEL
#define APP_LOG_LEVEL CONFIG_APP_LOG_LEVEL
#else
#define APP_LOG_LEVEL LOG_LEVEL_INF
#endif
#endif

#endif /* APP_LOG_H */
