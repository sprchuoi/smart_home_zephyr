#!/bin/bash
#
# Attach ESP32 USB device to WSL
# This script helps attach the CP210x USB to UART Bridge to WSL
#

set -e

BUSID="2-1"
DEVICE_NAME="CP210x"

echo "========================================="
echo "ESP32 USB Device Attachment for WSL"
echo "========================================="
echo ""

# Check if running in WSL
if ! grep -qi microsoft /proc/version; then
    echo "⚠ This script must be run in WSL"
    exit 1
fi

# Check if usbipd is installed on Windows
if ! powershell.exe -Command "Get-Command usbipd" > /dev/null 2>&1; then
    echo "✗ usbipd-win is not installed on Windows"
    echo ""
    echo "Please install it from:"
    echo "  https://github.com/dorssel/usbipd-win/releases"
    echo "Or run in PowerShell as Administrator:"
    echo "  winget install --interactive --exact dorssel.usbipd-win"
    exit 1
fi

echo "ℹ Checking USB device status..."
echo ""

# List available devices
powershell.exe -Command "usbipd list" 2>/dev/null || {
    echo "✗ Failed to list USB devices"
    exit 1
}

echo ""
echo "ℹ Attempting to attach $DEVICE_NAME (BUSID: $BUSID)..."
echo ""

# Try to attach the device
# Note: This requires administrator privileges on Windows
powershell.exe -Command "Start-Process powershell -Verb RunAs -ArgumentList '-Command', 'usbipd attach --wsl --busid $BUSID'" 2>/dev/null || {
    echo "⚠ Auto-attach failed. You may need to run manually in PowerShell as Administrator:"
    echo ""
    echo "  usbipd attach --wsl --busid $BUSID"
    echo ""
    echo "Then run this script again to verify."
    exit 1
}

# Wait for device to appear
echo "⏳ Waiting for device to appear in WSL..."
for i in {1..10}; do
    if [ -e /dev/ttyUSB0 ] || [ -e /dev/ttyACM0 ]; then
        echo "✓ Device detected!"
        break
    fi
    sleep 1
    echo -n "."
done
echo ""

# Check if device is available
if [ -e /dev/ttyUSB0 ]; then
    DEVICE="/dev/ttyUSB0"
elif [ -e /dev/ttyACM0 ]; then
    DEVICE="/dev/ttyACM0"
else
    echo "⚠ Device not found in /dev/"
    echo ""
    echo "Searching for USB serial devices..."
    ls -la /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  No serial devices found"
    echo ""
    echo "If device is attached but not accessible, you may need to:"
    echo "  1. Add yourself to dialout group: sudo usermod -a -G dialout \$USER"
    echo "  2. Log out and log back in"
    echo "  3. Or run: newgrp dialout"
    exit 1
fi

# Check permissions
echo ""
echo "ℹ Checking device permissions: $DEVICE"
ls -l $DEVICE

if [ ! -r "$DEVICE" ] || [ ! -w "$DEVICE" ]; then
    echo ""
    echo "⚠ Device exists but you don't have permission to access it"
    echo ""
    echo "Quick fix (this session only):"
    echo "  sudo chmod 666 $DEVICE"
    echo ""
    echo "Permanent fix:"
    echo "  sudo usermod -a -G dialout \$USER"
    echo "  Then log out and log back in"
    echo ""
    
    # Offer to fix permissions
    read -p "Fix permissions for this session? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo chmod 666 $DEVICE
        echo "✓ Permissions fixed for this session"
    fi
fi

echo ""
echo "========================================="
echo "✓ ESP32 attached and ready"
echo "========================================="
echo ""
echo "Device: $DEVICE"
echo ""

# Ask if user wants to flash firmware
read -p "Flash firmware now? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "========================================="
    echo "Flashing Firmware"
    echo "========================================="
    echo ""
    
    # Navigate to software directory
    SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    SOFTWARE_DIR="$(dirname "$SCRIPT_DIR")"
    
    cd "$SOFTWARE_DIR"
    
    # Check if build exists
    if [ ! -f "build/zephyr/zephyr.bin" ]; then
        echo "⚠ No firmware found at build/zephyr/zephyr.bin"
        echo ""
        read -p "Build firmware first? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo ""
            echo "Building firmware..."
            export ZEPHYR_BASE="${ZEPHYR_BASE:-$HOME/zephyrproject/zephyr}"
            ./make.sh build
        else
            echo "Skipping flash."
            exit 0
        fi
    fi
    
    # Flash firmware
    export ZEPHYR_BASE="${ZEPHYR_BASE:-$HOME/zephyrproject/zephyr}"
    ./make.sh flash --port $DEVICE
    
    echo ""
    read -p "Monitor serial output? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo ""
        echo "Starting serial monitor (Ctrl+A then K to exit)..."
        sleep 1
        ./make.sh monitor --port $DEVICE
    fi
else
    echo ""
    echo "You can flash firmware later with:"
    echo "  cd ~/sandboxes/example-application/software"
    echo "  export ZEPHYR_BASE=~/zephyrproject/zephyr"
    echo "  ./make.sh flash --port $DEVICE"
    echo ""
    echo "Or monitor output:"
    echo "  ./make.sh monitor --port $DEVICE"
    echo ""
fi
