#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# Apply board overlay if using ESP32
if(BOARD MATCHES "esp32")
    set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/boards/esp32_devkitc.overlay")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C CXX)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_sources(app PRIVATE 
    src/main.cpp
    src/cpp_support.cpp
    # Menu system
    src/menu/menu.cpp
    src/menu/menu_manager.cpp
    # C++ OOP Modules
    src/modules/blink/blinkmodule.cpp
    src/modules/sensor/sensormodule.cpp
    src/modules/display/displaymodule.cpp
    src/modules/button/buttonmodule.cpp
    src/modules/uart/uartmodule.cpp
    # Threads (C++ task orchestration)
    src/thread/blink_task.cpp
    src/thread/sensor_task.cpp
    src/thread/display_task.cpp
    src/thread/uart_task.cpp
)

# Conditionally compile BLE module
if(CONFIG_BT)
    target_sources(app PRIVATE
        src/modules/ble/bleservice.cpp
        src/thread/ble_task.cpp
    )
endif()

# Conditionally compile WiFi module
if(CONFIG_WIFI)
    target_sources(app PRIVATE
        src/modules/wifi/wifiservice.cpp
        src/thread/wifi_task.cpp
    )
endif()

# Conditionally compile MQTT module
if(CONFIG_MQTT_LIB)
    target_sources(app PRIVATE
        src/modules/mqtt/mqttmodule.cpp
        src/thread/mqtt_task.cpp
    )
endif()

# Add voice control modules subdirectories
add_subdirectory_ifdef(CONFIG_I2S src/modules/i2s_mic)
add_subdirectory_ifdef(CONFIG_BOOTLOADER_MCUBOOT src/modules/ota)
add_subdirectory(src/modules/wakeword)

target_include_directories(app PRIVATE 
    src
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link C++ standard library and support libraries
target_link_libraries(app PRIVATE stdc++ gcc)
