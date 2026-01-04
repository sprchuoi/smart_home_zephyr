#!/bin/bash
#
# Copyright (c) 2025 Sprchuoi
# SPDX-License-Identifier: Apache-2.0
#
# Monitor script for nRF5340 DK serial output
#

DEVICE="${1:-/dev/ttyACM0}"
BAUD="${2:-115200}"

echo "=========================================="
echo "nRF5340 DK Serial Monitor"
echo "=========================================="
echo "Device: $DEVICE"
echo "Baud: $BAUD"
echo ""
echo "Press Ctrl+C to exit"
echo ""

# Try different serial monitoring tools
if command -v picocom &> /dev/null; then
    picocom "$DEVICE" -b "$BAUD"
elif command -v minicom &> /dev/null; then
    minicom -D "$DEVICE" -b "$BAUD"
elif command -v screen &> /dev/null; then
    screen "$DEVICE" "$BAUD"
else
    echo "No serial monitor found. Install: picocom, minicom, or screen"
    exit 1
fi
