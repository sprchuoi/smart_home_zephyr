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
        src/app_core/app_core.cpp
        src/app_core/cpp_support.cpp
        
        # Shared SDK - Protocol layer (Matter + Thread)
        src/sdk/protocol/matter/app_task.cpp
        src/sdk/protocol/matter/light_endpoint.cpp
        src/sdk/protocol/matter/commissioning_delegate.cpp
        src/sdk/protocol/thread/thread_network_manager.cpp
        src/sdk/protocol/thread/network_resilience_manager.cpp
        
        # Shared SDK - Hardware abstraction layer
        src/sdk/hw/button/button_manager.cpp
        src/sdk/hw/uart/uart_manager.cpp
        
        # Shared SDK - IPC module for inter-core communication
        src/sdk/ipc/ipc_core.cpp
    )
endif()

# ===== NET CORE SOURCES - OpenThread + BLE Radio =====
# Target: nrf5340dk_nrf5340_cpunet (Cortex-M33 @ 64 MHz)
if(CONFIG_SOC_NRF5340_CPUNET)
    target_sources(app PRIVATE 
        # Network core with state machine
        src/net_core/net_core.cpp
        
        # Shared SDK - IPC module for inter-core communication
        src/sdk/ipc/ipc_core.cpp
        
        # Shared SDK - Protocol layer (BLE and Radio subsystems)
        src/sdk/protocol/ble/ble_manager.cpp
        src/sdk/protocol/radio/radio_manager.cpp
    )
endif()

#===============================================================================
# INCLUDE DIRECTORIES - Source Tree Organization
#===============================================================================
# Make headers available throughout the project
target_include_directories(app PRIVATE 
    src                                    # Core source root
    src/app_core                           # APP core headers
    src/net_core                           # NET core headers
    src/sdk                                # Shared SDK headers root
    src/sdk/hw                             # Hardware abstraction layer
    src/sdk/protocol                       # Protocol layer
    src/sdk/services                       # Services layer
    src/sdk/ipc                            # IPC headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../include # Public API headers
)

#===============================================================================
# LINK LIBRARIES - C++ Standard Library
#===============================================================================
# Link against C++ standard library for C++ features
target_link_libraries(app PRIVATE stdc++)

