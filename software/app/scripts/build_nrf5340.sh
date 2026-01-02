#!/bin/bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# Build script for nRF5340 DK Matter Smart Light
#

set -e

BOARD="nrf5340dk_nrf5340"
APP_CORE="${BOARD}_cpuapp"
NET_CORE="${BOARD}_cpunet"
BUILD_DIR="build_nrf5340"

echo "=========================================="
echo "nRF5340 DK Matter Smart Light Build Script"
echo "=========================================="

# Parse arguments
BUILD_MODE="${1:-both}"
CLEAN="${2:-no}"

if [[ "$CLEAN" == "clean" ]]; then
    echo "[*] Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

case "$BUILD_MODE" in
    app)
        echo "[*] Building APP Core (Matter + LED/Button)..."
        west build -b "$APP_CORE" -d "$BUILD_DIR/app" . -c
        ;;
    net)
        echo "[*] Building NET Core (Thread + BLE)..."
        west build -b "$NET_CORE" -d "$BUILD_DIR/net" . -c
        ;;
    both|all)
        echo "[*] Building both APP and NET cores..."
        
        echo "[1/2] Building APP Core..."
        west build -b "$APP_CORE" -d "$BUILD_DIR/app" . -c
        
        echo "[2/2] Building NET Core..."
        west build -b "$NET_CORE" -d "$BUILD_DIR/net" . -c
        
        echo "[*] Creating combined HEX file..."
        # Merge both cores into single HEX (if using pyocd or nrfjprog)
        # nrfjprog --program "$BUILD_DIR/app/zephyr/zephyr.hex" --chiperase
        # nrfjprog --program "$BUILD_DIR/net/zephyr/zephyr.hex"
        ;;
    *)
        echo "Usage: $0 [app|net|both|all] [clean]"
        echo ""
        echo "Examples:"
        echo "  $0 app              # Build APP core only"
        echo "  $0 net              # Build NET core only"
        echo "  $0 both             # Build both cores (default)"
        echo "  $0 both clean       # Clean and build both cores"
        exit 1
        ;;
esac

echo "[✓] Build completed successfully"
echo ""
echo "Build artifacts:"
echo "  APP Core: $BUILD_DIR/app/zephyr/zephyr.hex"
echo "  NET Core: $BUILD_DIR/net/zephyr/zephyr.hex"
echo ""
echo "Next steps:"
echo "  1. Flash APP Core: nrfjprog --program $BUILD_DIR/app/zephyr/zephyr.hex --chiperase"
echo "  2. Flash NET Core: nrfjprog --program $BUILD_DIR/net/zephyr/zephyr.hex"
echo "  3. Monitor: nrfjprog --reset"
