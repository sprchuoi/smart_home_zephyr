#!/bin/bash

# nRF5340 DK Smart Matter Light - Main Build System Entry Point
# Dual-core build system for APP (Matter) and NET (OpenThread) cores

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS_LIB="$SCRIPT_DIR/scripts"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m'

# Load library functions
source "$SCRIPTS_LIB/lib.sh"

# Show usage
show_usage() {
    echo -e "${GREEN}nRF5340 DK Smart Matter Light - Build System${NC}"
    echo ""
    echo "Usage: ./make.sh <command> [options]"
    echo ""
    echo "Commands:"
    echo -e "  ${YELLOW}setup${NC}      Setup Zephyr environment (first time only)"
    echo -e "  ${YELLOW}build${NC}      Build dual-core firmware (APP + NET cores)"
    echo -e "  ${YELLOW}build-app${NC}   Build APP core only (Matter stack)"
    echo -e "  ${YELLOW}build-net${NC}   Build NET core only (OpenThread stack)"
    echo -e "  ${YELLOW}clean${NC}      Clean build artifacts"
    echo -e "  ${YELLOW}flash${NC}      Flash both cores to nRF5340 DK"
    echo -e "  ${YELLOW}monitor${NC}    Open serial monitor (debug UART)"
    echo -e "  ${YELLOW}all${NC}        Build and flash both cores"
    echo -e "  ${YELLOW}docs${NC}       Build documentation (Sphinx)"
    echo -e "  ${YELLOW}help${NC}       Show this help message"
    echo ""
    echo "Options:"
    echo "  --port <device>    Specify serial port (default: auto-detect)"
    echo "  --                 Pass additional arguments to west build"
    echo ""
    echo "Examples:"
    echo "  ./make.sh setup              # First time setup"
    echo "  ./make.sh build              # Build both APP and NET cores"
    echo "  ./make.sh build-app          # Build Matter stack only"
    echo "  ./make.sh build-net          # Build OpenThread stack only"
    echo "  ./make.sh flash              # Flash to nRF5340 DK"
    echo "  ./make.sh all                # Build and flash both cores"
    echo "  ./make.sh monitor            # Monitor serial output"
    echo "  ./make.sh docs               # Build documentation"
    echo ""
}

# Parse command
COMMAND=${1:-help}
shift || true

# Parse options
PORT=""
BUILD_ARGS=()

while [[ $# -gt 0 ]]; do
    case $1 in
        --port)
            PORT="$2"
            shift 2
            ;;
        --)
            shift
            BUILD_ARGS=("$@")
            break
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
        print_header "Building Dual-Core Firmware"
        check_environment
        build_dual_core
        ;;
    
    build-app)
        print_header "Building APP Core (Matter Stack)"
        check_environment
        build_app_core
        ;;
    
    build-net)
        print_header "Building NET Core (OpenThread Stack)"
        check_environment
        build_net_core
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
        print_header "Build and Flash Both Cores"
        check_environment
        build_dual_core
        flash_firmware "$PORT"
        echo ""
        echo -e "${GREEN}Done! Run './make.sh monitor' to see output${NC}"
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
