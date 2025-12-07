#!/bin/bash

# ESP32 Smart Home - Build Script
# This script builds the Zephyr application for ESP32-WROOM-32

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}ESP32 Smart Home - Build Script${NC}"
echo -e "${GREEN}========================================${NC}"

# Check if ZEPHYR_BASE is set
if [ -z "$ZEPHYR_BASE" ]; then
    echo -e "${RED}Error: ZEPHYR_BASE not set${NC}"
    echo ""
    echo "Please run setup first: ./setup.sh"
    echo ""
    echo "Or manually set ZEPHYR_BASE:"
    echo "  export ZEPHYR_BASE=~/zephyrproject/zephyr"
    echo ""
    exit 1
fi

echo -e "${YELLOW}Zephyr Base:${NC} $ZEPHYR_BASE"

# Check if west is installed
if ! command -v west &> /dev/null; then
    echo -e "${RED}Error: west command not found${NC}"
    echo "Please install west: pip3 install --user west"
    exit 1
fi

# Check if we're in the correct directory
if [ ! -f "app/CMakeLists.txt" ]; then
    echo -e "${RED}Error: Must run from software/ directory${NC}"
    exit 1
fi

# Board selection
BOARD=${1:-esp32_devkitc_wroom}

echo -e "${YELLOW}Board:${NC} $BOARD"
echo -e "${YELLOW}Application:${NC} app/"
echo ""

# Clean build option
if [ "$2" == "clean" ]; then
    echo -e "${YELLOW}Performing clean build...${NC}"
    rm -rf build
fi

# Build the application
echo -e "${GREEN}Building application...${NC}"

# Use cmake directly if not in a west workspace
if [ -f "$ZEPHYR_BASE/cmake/app/boilerplate.cmake" ] || [ -f "$ZEPHYR_BASE/../zephyr/cmake/app/boilerplate.cmake" ]; then
    # Try west build first
    if west build -b $BOARD app 2>/dev/null; then
        BUILD_SUCCESS=1
    else
        # Fall back to cmake if west build fails
        echo -e "${YELLOW}West workspace not found, using cmake directly...${NC}"
        mkdir -p build
        cd build
        cmake -DBOARD=$BOARD ../app -GNinja
        ninja
        cd ..
        BUILD_SUCCESS=1
    fi
else
    echo -e "${RED}Cannot find Zephyr installation${NC}"
    echo "Please run: ./setup.sh"
    exit 1
fi

# Check build result
if [ $BUILD_SUCCESS ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "${YELLOW}Binary location:${NC} build/zephyr/zephyr.bin"
    echo -e "${YELLOW}Binary size:${NC}"
    ls -lh build/zephyr/zephyr.bin
    echo ""
    echo -e "${YELLOW}To flash:${NC} ./flash.sh"
    echo -e "${YELLOW}Or:${NC} west flash"
else
    echo ""
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
