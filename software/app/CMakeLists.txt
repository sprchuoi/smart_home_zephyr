#===============================================================================
# nRF5340 DK Matter Smart Light - Dual-Core Build System
#===============================================================================
#
# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# Purpose:
#   Production-grade build configuration for Matter smart light on nRF5340 DK
#   Builds separate firmware images for APP core (Matter) and NET core (Thread)
#
# Architecture:
#   ┌──────────────────────────────────────────┐
#   │         nRF5340 DK Dual Core             │\n#   ├──────────────────────────────────────────┤
#   │ APP Core (Cortex-M33 @ 128MHz)           │\n#   │ - Matter protocol + Light control         │\n#   │ - LED/Button handling                    │\n#   │ - User-facing commissioning               │\n#   │ - Cluster attributes & reporting         │\n#   ├──────────────────────────────────────────┤
#   │ NET Core (Cortex-M33 @ 64MHz)            │\n#   │ - OpenThread 802.15.4 stack               │\n#   │ - BLE commissioning radio                │\n#   │ - Network join & routing                 │\n#   │ - Radio scheduler                       │\n#   └──────────────────────────────────────────┘
#
# Build Targets:
#   - nrf5340dk_nrf5340_cpuapp: Matter application\n#   - nrf5340dk_nrf5340_cpunet: Thread networking
#
# Usage:\n#   west build -b nrf5340dk_nrf5340_cpuapp -d build_app .\n#   west build -b nrf5340dk_nrf5340_cpunet -d build_net .
#
#===============================================================================

cmake_minimum_required(VERSION 3.13.1)

# Select appropriate device tree overlay based on target core
if(CONFIG_SOC_NRF5340_CPUAPP)
    set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/boards/nrf5340dk_nrf5340_cpuapp.overlay")
elseif(CONFIG_SOC_NRF5340_CPUNET)
    set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/boards/nrf5340dk_nrf5340_cpunet.overlay")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nrf5340_matter_light LANGUAGES C CXX)

# C++ Standard Library
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===== APP CORE SOURCES - Matter Protocol + LED/Button Control =====
# Target: nrf5340dk_nrf5340_cpuapp (Cortex-M33 @ 128 MHz)
if(CONFIG_SOC_NRF5340_CPUAPP)
    target_sources(app PRIVATE
        # Core application - main entry point and event loop
        src/app_core.cpp
        src/cpp_support.cpp
        
        # Matter protocol stack integration
        src/modules/matter/app_task.cpp
        src/modules/matter/light_endpoint.cpp
        
        # Hardware abstraction layer - GPIO control and user input
        src/modules/button/buttonmodule.cpp
        src/modules/uart/uartmodule.cpp
    )
endif()

# ===== NET CORE SOURCES - OpenThread + BLE Radio =====
# Target: nrf5340dk_nrf5340_cpunet (Cortex-M33 @ 64 MHz)
if(CONFIG_SOC_NRF5340_CPUNET)
    target_sources(app PRIVATE 
        # Network stack - Thread and BLE radio management
        src/net_core.c
    )
endif()

#===============================================================================
# INCLUDE DIRECTORIES - Source Tree Organization
#===============================================================================
# Make headers available throughout the project
target_include_directories(app PRIVATE 
    src                              # Core source files
    src/modules                      # Module headers (matter, button, uart)
    ${CMAKE_CURRENT_SOURCE_DIR}/../include  # Public API headers
)

#===============================================================================
# LINK LIBRARIES - C++ Standard Library
#===============================================================================
# Link against C++ standard library for C++ features
target_link_libraries(app PRIVATE stdc++)

