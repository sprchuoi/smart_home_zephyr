#!/bin/bash

# ESP32 Smart Home - Library Functions
# Common functions used by build scripts

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Print formatted header
print_header() {
    local title="$1"
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}$title${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
}

# Print success message
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Print error message
print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Print warning message
print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Print info message
print_info() {
    echo -e "${CYAN}ℹ $1${NC}"
}

# Check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Check Zephyr environment
check_environment() {
    if [ -z "$ZEPHYR_BASE" ]; then
        print_error "ZEPHYR_BASE not set"
        echo ""
        echo "Please run: ./make.sh setup"
        echo "Or manually: export ZEPHYR_BASE=~/zephyrproject/zephyr"
        exit 1
    fi
    
    print_success "ZEPHYR_BASE: $ZEPHYR_BASE"
    
    if ! command_exists west; then
        print_error "west not found"
        echo "Install: pip3 install --user west"
        exit 1
    fi
    
    if ! command_exists xtensa-esp32-elf-gcc; then
        print_warning "ESP32 toolchain not found in PATH"
        echo "The build may fail without the toolchain"
    fi
}

# Build project
build_project() {
    local board="$1"
    
    if [ ! -f "app/CMakeLists.txt" ]; then
        print_error "Must run from software/ directory"
        exit 1
    fi
    
    # Fix board name for ESP32
    if [ "$board" = "esp32_devkitc_wroom" ] || [ "$board" = "esp32_devkitc" ]; then
        board="esp32_devkitc/esp32/procpu"
        print_info "Using board: $board"
    fi
    
    print_info "Application: app/"
    echo ""
    
    print_info "Building application..."
    
    # Try west build
    if west build -b "$board" app; then
        echo ""
        print_header "Build Complete"
        print_success "Binary: build/zephyr/zephyr.bin"
        ls -lh build/zephyr/zephyr.bin 2>/dev/null || true
        echo ""
        print_info "Next: ./make.sh flash"
        return 0
    else
        print_error "Build failed"
        exit 1
    fi
}

# Clean build
clean_build() {
    print_info "Removing build directory..."
    rm -rf build
    print_success "Build cleaned"
}

# Detect serial port
detect_serial_port() {
    local port=""
    
    if [ -e "/dev/ttyUSB0" ]; then
        port="/dev/ttyUSB0"
    elif [ -e "/dev/ttyUSB1" ]; then
        port="/dev/ttyUSB1"
    elif [ -e "/dev/ttyACM0" ]; then
        port="/dev/ttyACM0"
    else
        echo ""
        print_warning "No serial port auto-detected"
        echo "Available ports:"
        ls /dev/tty* | grep -E "USB|ACM" || echo "  None found"
        echo ""
        read -p "Enter serial port: " port
    fi
    
    echo "$port"
}

# Flash firmware
flash_firmware() {
    local port="$1"
    
    if [ ! -f "build/zephyr/zephyr.bin" ]; then
        print_error "Binary not found. Build first: ./make.sh build"
        exit 1
    fi
    
    if [ -z "$port" ]; then
        port=$(detect_serial_port)
    fi
    
    print_info "Serial port: $port"
    print_info "Binary: build/zephyr/zephyr.bin"
    echo ""
    
    print_info "Flashing firmware..."
    
    if command_exists west; then
        west flash --esp-device "$port"
    elif command_exists esptool.py; then
        esptool.py --chip esp32 --port "$port" --baud 460800 \
            --before default_reset --after hard_reset write_flash \
            -z --flash_mode dio --flash_freq 40m --flash_size detect \
            0x1000 build/zephyr/zephyr.bin
    else
        print_error "Neither west nor esptool.py found"
        echo "Install esptool: pip3 install --user esptool"
        exit 1
    fi
    
    if [ $? -eq 0 ]; then
        echo ""
        print_header "Flash Complete"
        print_success "Firmware flashed successfully"
        echo ""
        print_info "Monitor output: ./make.sh monitor"
    else
        echo ""
        print_error "Flash failed"
        echo ""
        print_warning "Troubleshooting:"
        echo "  1. Press and hold BOOT button during flash"
        echo "  2. Check USB cable"
        echo "  3. Verify port: ls /dev/ttyUSB*"
        echo "  4. Check permissions: sudo chmod 666 $port"
        exit 1
    fi
}

# Open serial monitor
open_monitor() {
    local port="$1"
    local baudrate="${2:-115200}"
    
    if [ -z "$port" ]; then
        port=$(detect_serial_port)
    fi
    
    print_info "Port: $port"
    print_info "Baudrate: $baudrate"
    echo ""
    print_info "Press Ctrl+A then K to exit screen"
    print_info "Press RESET button on ESP32 to restart"
    echo ""
    sleep 2
    
    if command_exists screen; then
        screen "$port" "$baudrate"
    elif command_exists minicom; then
        minicom -D "$port" -b "$baudrate"
    else
        print_error "No serial monitor found"
        echo "Install: sudo apt install screen"
        exit 1
    fi
}

# Edit WiFi configuration
edit_config() {
    local config_file="app/src/modules/wifi/wifi_module.h"
    
    if [ ! -f "$config_file" ]; then
        print_error "Config file not found: $config_file"
        exit 1
    fi
    
    print_info "Opening WiFi configuration..."
    echo ""
    print_info "Edit these values:"
    echo "  - WIFI_SSID (Station mode)"
    echo "  - WIFI_PSK (Station password)"
    echo "  - WIFI_AP_SSID (AP mode)"
    echo "  - WIFI_AP_PSK (AP password)"
    echo ""
    
    # Use default editor or nano
    ${EDITOR:-nano} "$config_file"
    
    echo ""
    print_success "Configuration updated"
    print_info "Rebuild to apply: ./make.sh build"
}

# Build documentation
build_docs() {
    local doc_dir="$SCRIPT_DIR/doc"
    local build_dir="$doc_dir/_build"
    
    if [ ! -d "$doc_dir" ]; then
        print_error "Documentation directory not found: $doc_dir"
        return 1
    fi
    
    # Check if Sphinx is installed
    if ! command_exists sphinx-build; then
        print_warning "Sphinx not found. Installing requirements..."
        
        if [ -f "$doc_dir/requirements.txt" ]; then
            pip3 install --user -r "$doc_dir/requirements.txt" || {
                print_error "Failed to install documentation requirements"
                print_info "Install manually: pip3 install -r doc/requirements.txt"
                return 1
            }
        else
            print_error "requirements.txt not found in doc/"
            print_info "Install Sphinx: pip3 install sphinx sphinx-rtd-theme"
            return 1
        fi
    fi
    
    # Clean previous build
    if [ -d "$build_dir" ]; then
        print_info "Cleaning previous build..."
        rm -rf "$build_dir"
    fi
    
    # Build documentation
    print_info "Building HTML documentation..."
    cd "$doc_dir"
    
    if make html; then
        print_success "Documentation built successfully"
        print_info "Output: $build_dir/html/index.html"
        
        # Try to open in browser
        if command_exists xdg-open; then
            print_info "Opening in browser..."
            xdg-open "$build_dir/html/index.html" 2>/dev/null &
        elif command_exists open; then
            print_info "Opening in browser..."
            open "$build_dir/html/index.html" 2>/dev/null &
        else
            echo ""
            print_info "View documentation:"
            echo "  file://$build_dir/html/index.html"
        fi
    else
        print_error "Documentation build failed"
        return 1
    fi
    
    cd - > /dev/null
}

# Run QEMU smoke test
run_qemu_test() {
    local qemu_board="qemu_cortex_m3"
    local timeout_sec=30
    
    print_info "Building for QEMU (ARM Cortex-M3)..."
    
    # Check if QEMU is installed
    if ! command_exists qemu-system-arm; then
        print_warning "QEMU ARM not found"
        print_info "Install: sudo apt-get install qemu-system-arm (Ubuntu/Debian)"
        print_info "         brew install qemu (macOS)"
        return 1
    fi
    
    # Build with minimal configuration for QEMU
    print_info "Building minimal configuration..."
    west build -b "$qemu_board" app -p -- \
        -DCONFIG_BT=n \
        -DCONFIG_WIFI=n \
        -DCONFIG_NETWORKING=n \
        -DCONFIG_DISPLAY=n \
        -DCONFIG_I2C=n || {
        print_error "QEMU build failed"
        return 1
    }
    
    print_success "Build successful"
    print_info "Running in QEMU (${timeout_sec}s timeout)..."
    echo ""
    
    # Run in QEMU with timeout
    timeout "${timeout_sec}s" west build -t run || EXIT_CODE=$?
    
    echo ""
    
    # Check exit code
    if [ ${EXIT_CODE:-0} -eq 124 ]; then
        print_success "QEMU smoke test passed (timeout = application running)"
    elif [ ${EXIT_CODE:-0} -eq 0 ]; then
        print_success "QEMU smoke test passed (clean exit)"
    else
        print_error "QEMU smoke test failed (exit code: $EXIT_CODE)"
        return $EXIT_CODE
    fi
    
    print_info "Build artifacts: build/zephyr/zephyr.elf"
    return 0
}

# Export functions
export -f print_header
export -f print_success
export -f print_error
export -f print_warning
export -f print_info
export -f command_exists
export -f check_environment
export -f build_project
export -f clean_build
export -f detect_serial_port
export -f flash_firmware
export -f open_monitor
export -f edit_config
export -f build_docs
export -f run_qemu_test
