#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C CXX)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_sources(app PRIVATE 
    src/main.cpp
    src/cpp_support.cpp
    # C++ OOP Modules
    src/modules/blink/blinkmodule.cpp
    src/modules/sensor/sensormodule.cpp
    src/modules/ble/bleservice.cpp
    src/modules/wifi/wifiservice.cpp
    src/modules/display/displaymodule.cpp
    src/modules/button/buttonmodule.cpp
    # Threads (C++ task orchestration)
    src/thread/blink_task.cpp
    src/thread/sensor_task.cpp
    src/thread/ble_task.cpp
    src/thread/wifi_task.cpp
    src/thread/display_task.cpp
)

target_include_directories(app PRIVATE 
    src
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link C++ standard library and support libraries
target_link_libraries(app PRIVATE stdc++ gcc)
