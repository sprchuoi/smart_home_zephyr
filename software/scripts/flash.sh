#!/bin/bash

# ESP32 Smart Home - Flash Script
# This script flashes the compiled firmware to ESP32

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}ESP32 Smart Home - Flash Script${NC}"
echo -e "${GREEN}========================================${NC}"

# Check if binary exists
if [ ! -f "build/zephyr/zephyr.bin" ]; then
    echo -e "${RED}Error: Binary not found!${NC}"
    echo "Please build first: ./build.sh"
    exit 1
fi

# Auto-detect serial port
SERIAL_PORT=""
if [ -e "/dev/ttyUSB0" ]; then
    SERIAL_PORT="/dev/ttyUSB0"
elif [ -e "/dev/ttyUSB1" ]; then
    SERIAL_PORT="/dev/ttyUSB1"
elif [ -e "/dev/ttyACM0" ]; then
    SERIAL_PORT="/dev/ttyACM0"
else
    echo -e "${YELLOW}Available serial ports:${NC}"
    ls /dev/tty* | grep -E "USB|ACM" || echo "No serial ports found"
    echo ""
    read -p "Enter serial port (e.g., /dev/ttyUSB0): " SERIAL_PORT
fi

echo -e "${YELLOW}Serial port:${NC} $SERIAL_PORT"
echo -e "${YELLOW}Binary:${NC} build/zephyr/zephyr.bin"
echo ""

# Check for west
if command -v west &> /dev/null; then
    echo -e "${GREEN}Flashing with west...${NC}"
    west flash --esp-device $SERIAL_PORT
else
    # Check for esptool
    if ! command -v esptool.py &> /dev/null; then
        echo -e "${RED}Error: Neither west nor esptool.py found${NC}"
        echo "Install esptool: pip3 install --user esptool"
        exit 1
    fi
    
    echo -e "${GREEN}Flashing with esptool...${NC}"
    esptool.py --chip esp32 --port $SERIAL_PORT --baud 460800 \
        --before default_reset --after hard_reset write_flash \
        -z --flash_mode dio --flash_freq 40m --flash_size detect \
        0x1000 build/zephyr/zephyr.bin
fi

# Check result
if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Flash completed successfully!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "${YELLOW}Monitor serial output:${NC}"
    echo "  screen $SERIAL_PORT 115200"
    echo "  or: west attach"
else
    echo ""
    echo -e "${RED}Flash failed!${NC}"
    echo ""
    echo -e "${YELLOW}Troubleshooting:${NC}"
    echo "1. Press and hold BOOT button while connecting"
    echo "2. Check USB cable connection"
    echo "3. Verify serial port: ls /dev/ttyUSB*"
    echo "4. Check permissions: sudo chmod 666 $SERIAL_PORT"
    exit 1
fi
