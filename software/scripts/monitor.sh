#!/bin/bash

# ESP32 Smart Home - Monitor Script
# Monitors serial output from ESP32

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}ESP32 Smart Home - Serial Monitor${NC}"
echo ""

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
    read -p "Enter serial port: " SERIAL_PORT
fi

BAUDRATE=${1:-115200}

echo -e "${YELLOW}Port:${NC} $SERIAL_PORT"
echo -e "${YELLOW}Baudrate:${NC} $BAUDRATE"
echo ""
echo -e "${YELLOW}Press Ctrl+A then K to exit screen${NC}"
echo -e "${YELLOW}Press RESET button on ESP32 to restart${NC}"
echo ""
sleep 2

# Use screen if available, otherwise try minicom
if command -v screen &> /dev/null; then
    screen $SERIAL_PORT $BAUDRATE
elif command -v minicom &> /dev/null; then
    minicom -D $SERIAL_PORT -b $BAUDRATE
else
    echo "Please install screen or minicom:"
    echo "  sudo apt install screen"
fi
