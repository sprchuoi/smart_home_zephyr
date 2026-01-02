#!/bin/bash
# Quick build reference for nRF5340 DK Matter Smart Light

# Prerequisites:
# - west: pip install west
# - nrf-command-line-tools: https://www.nordicsemi.com/products/development-tools/nrf5-command-line-tools

# Build both cores (clean)
cd software/app
west build -b nrf5340dk_nrf5340_cpuapp -d build_app . -c
west build -b nrf5340dk_nrf5340_cpunet -d build_net . -c

# Flash to device
nrfjprog --eraseall
nrfjprog --program build_app/zephyr/zephyr.hex
nrfjprog --program build_net/zephyr/zephyr.hex
nrfjprog --reset

# Monitor serial output
picocom /dev/ttyACM0 -b 115200
# OR
minicom -D /dev/ttyACM0

# Expected APP Core output:
# ============================================
# nRF5340 DK Matter Smart Light (APP Core)
# Version: 0.1.0
# Cortex-M33 @ 128 MHz
# ============================================
# LED initialized on P0.28
# Initializing Matter stack...
# Matter stack initialized
# Button thread created
# APP Core initialization complete!

# Test:
# 1. Press Button SW1 (P0.23) → LED P0.28 toggles
# 2. Commission via Matter (Google Home / Apple Home)
# 3. Control LED from phone
