#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(app PRIVATE 
    src/main.c
    # Modules (library functions)
    src/modules/blink/blink_module.c
    src/modules/sensor/sensor_module.c
    src/modules/ble/ble_module.c
    src/modules/wifi/wifi_module.c
    src/modules/display/display_module.c
    src/modules/button/button_module.c
    # Threads (task orchestration)
    src/thread/blink_task.c
    src/thread/sensor_task.c
    src/thread/ble_task.c
    src/thread/wifi_task.c
    src/thread/display_task.c
)

target_include_directories(app PRIVATE src)
