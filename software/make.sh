#!/bin/bash

# ESP32 Smart Home - Main Build System Entry Point
# Single script to handle all build operations

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS_LIB="$SCRIPT_DIR/scripts"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Load library functions
source "$SCRIPTS_LIB/lib.sh"

# Show usage
show_usage() {
    echo -e "${GREEN}ESP32 Smart Home - Build System${NC}"
    echo ""
    echo "Usage: ./make.sh <command> [options]"
    echo ""
    echo "Commands:"
    echo -e "  ${YELLOW}setup${NC}      Setup Zephyr environment (first time only)"
    echo -e "  ${YELLOW}build${NC}      Build the application"
    echo -e "  ${YELLOW}clean${NC}      Clean build artifacts"
    echo -e "  ${YELLOW}flash${NC}      Flash firmware to ESP32"
    echo -e "  ${YELLOW}monitor${NC}    Open serial monitor"
    echo -e "  ${YELLOW}all${NC}        Build and flash"
    echo -e "  ${YELLOW}qemu${NC}       Build and run in QEMU (ARM smoke test)"
    echo -e "  ${YELLOW}qemu-esp32${NC} Build and run in ESP32 QEMU (blink test)"
    echo -e "  ${YELLOW}docs${NC}       Build documentation (Sphinx)"
    echo -e "  ${YELLOW}config${NC}     Edit WiFi configuration"
    echo -e "  ${YELLOW}help${NC}       Show this help message"
    echo ""
    echo "Options:"
    echo "  --board <name>     Specify board (default: esp32_devkitc_wroom)"
    echo "  --port <device>    Specify serial port (default: auto-detect)"
    echo ""
    echo "Examples:"
    echo "  ./make.sh setup              # First time setup"
    echo "  ./make.sh build              # Build firmware"
    echo "  ./make.sh flash              # Flash to ESP32"
    echo "  ./make.sh all                # Build and flash"
    echo "  ./make.sh qemu               # Run ARM smoke test in QEMU"
    echo "  ./make.sh qemu-esp32         # Run ESP32 blink test in QEMU"
    echo "  ./make.sh docs               # Build documentation"
    echo "  ./make.sh monitor            # Monitor serial output"
    echo "  ./make.sh build --board esp32_devkitc"
    echo ""
}

# Parse command
COMMAND=${1:-help}
shift || true

# Parse options
BOARD="esp32_devkitc"
PORT=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --board)
            BOARD="$2"
            shift 2
            ;;
        --port)
            PORT="$2"
            shift 2
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_usage
            exit 1
            ;;
    esac
done

# Execute command
case $COMMAND in
    setup)
        print_header "Running Setup"
        "$SCRIPTS_LIB/setup.sh"
        ;;
    
    build)
        print_header "Building Application"
        check_environment
        build_project "$BOARD"
        ;;
    
    clean)
        print_header "Cleaning Build"
        clean_build
        ;;
    
    flash)
        print_header "Flashing Firmware"
        check_environment
        flash_firmware "$PORT"
        ;;
    
    monitor)
        print_header "Serial Monitor"
        open_monitor "$PORT"
        ;;
    
    all)
        print_header "Build and Flash"
        check_environment
        build_project "$BOARD"
        flash_firmware "$PORT"
        echo ""
        echo -e "${GREEN}Done! Run './make.sh monitor' to see output${NC}"
        ;;
    
    qemu)
        print_header "QEMU Smoke Test (ARM)"
        check_environment
        run_qemu_test
        ;;
    
    qemu-esp32)
        print_header "ESP32 QEMU Blink LED Test"
        check_environment
        run_esp32_qemu_test
        ;;
    
    config)
        print_header "Edit Configuration"
        edit_config
        ;;
    
    docs)
        print_header "Building Documentation"
        build_docs
        ;;
    
    help|--help|-h)
        show_usage
        ;;
    
    *)
        echo -e "${RED}Unknown command: $COMMAND${NC}"
        echo ""
        show_usage
        exit 1
        ;;
esac

exit 0
