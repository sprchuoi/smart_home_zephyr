#!/bin/bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# Flash script for nRF5340 DK Matter Smart Light
#

set -e

BOARD="nrf5340dk_nrf5340"
APP_CORE="${BOARD}_cpuapp"
NET_CORE="${BOARD}_cpunet"
BUILD_DIR="build_nrf5340"

echo "=========================================="
echo "nRF5340 DK Flash Script"
echo "=========================================="

# Check for nrfjprog
if ! command -v nrfjprog &> /dev/null; then
    echo "[!] ERROR: nrfjprog not found"
    echo "    Install: https://www.nordicsemi.com/products/development-tools/nrf5-command-line-tools"
    exit 1
fi

TARGET="${1:-both}"

case "$TARGET" in
    app)
        echo "[*] Flashing APP Core..."
        nrfjprog --program "$BUILD_DIR/app/zephyr/zephyr.hex" --chiperase
        echo "[✓] APP Core flashed"
        ;;
    net)
        echo "[*] Flashing NET Core..."
        nrfjprog --program "$BUILD_DIR/net/zephyr/zephyr.hex"
        echo "[✓] NET Core flashed"
        ;;
    both|all)
        echo "[*] Flashing both cores (complete erase)..."
        nrfjprog --eraseall
        
        echo "[1/2] Flashing APP Core..."
        nrfjprog --program "$BUILD_DIR/app/zephyr/zephyr.hex"
        
        echo "[2/2] Flashing NET Core..."
        nrfjprog --program "$BUILD_DIR/net/zephyr/zephyr.hex"
        
        echo "[✓] Both cores flashed successfully"
        ;;
    *)
        echo "Usage: $0 [app|net|both]"
        exit 1
        ;;
esac

echo ""
echo "[*] Resetting device..."
nrfjprog --reset
sleep 1

echo "[✓] Done! Check serial output:"
echo "  picocom /dev/ttyACM0 -b 115200"
