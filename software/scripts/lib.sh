#!/bin/bash

# nRF5340 DK Smart Matter Light - Library Functions
# Dual-core build system for APP (Matter) and NET (OpenThread) cores

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

# Install Espressif QEMU automatically
install_espressif_qemu() {
    local qemu_bin="qemu-system-xtensa"
    local install_dir="$HOME/.local/espressif-qemu"
    local qemu_version="esp_develop_9.0.0_20240606"
    
    print_info "Installing Espressif QEMU..."
    
    # Detect OS
    local os_type=""
    local qemu_url=""
    
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        os_type="linux"
        qemu_url="https://github.com/espressif/qemu/releases/download/esp-develop-9.0.0-20240606/qemu-xtensa-softmmu-${qemu_version}-x86_64-linux-gnu.tar.xz"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        os_type="macos"
        # Check if Homebrew is available
        if command_exists brew; then
            print_info "Installing via Homebrew..."
            if brew tap espressif/espressif && brew install qemu; then
                print_success "Espressif QEMU installed via Homebrew"
                return 0
            else
                print_warning "Homebrew installation failed, trying manual installation"
            fi
        fi
        # Fallback to manual download for macOS
        if [[ $(uname -m) == "arm64" ]]; then
            qemu_url="https://github.com/espressif/qemu/releases/download/esp-develop-9.0.0-20240606/qemu-xtensa-softmmu-${qemu_version}-aarch64-apple-darwin.tar.xz"
        else
            qemu_url="https://github.com/espressif/qemu/releases/download/esp-develop-9.0.0-20240606/qemu-xtensa-softmmu-${qemu_version}-x86_64-apple-darwin.tar.xz"
        fi
    else
        print_error "Unsupported OS: $OSTYPE"
        return 1
    fi
    
    # Create install directory
    mkdir -p "$install_dir"
    
    print_info "Downloading Espressif QEMU from GitHub..."
    print_info "URL: $qemu_url"
    
    local download_file="$install_dir/qemu-espressif.tar.xz"
    
    # Download QEMU
    if command_exists wget; then
        wget -q --show-progress -O "$download_file" "$qemu_url" || {
            print_error "Download failed"
            return 1
        }
    elif command_exists curl; then
        curl -L -o "$download_file" "$qemu_url" || {
            print_error "Download failed"
            return 1
        }
    else
        print_error "Neither wget nor curl found. Please install one of them."
        return 1
    fi
    
    print_info "Extracting QEMU..."
    tar xf "$download_file" -C "$install_dir" --strip-components=1 || {
        print_error "Extraction failed"
        rm -f "$download_file"
        return 1
    }
    
    rm -f "$download_file"
    
    # Add to PATH for current session
    export PATH="$install_dir/bin:$PATH"
    
    if command_exists "$qemu_bin"; then
        print_success "Espressif QEMU installed successfully"
        print_info "Installation directory: $install_dir"
        print_info "QEMU Binary: $(which $qemu_bin)"
        echo ""
        print_warning "Add to your shell profile for permanent access:"
        echo "export PATH=\"$install_dir/bin:\$PATH\""
        echo ""
        return 0
    else
        print_error "Installation completed but QEMU binary not found"
        return 1
    fi
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
    
    if ! command_exists arm-none-eabi-gcc; then
        print_warning "ARM toolchain not found in PATH"
        echo "The build may fail without the ARM toolchain for nRF5340"
    fi
}

# Build dual-core firmware (APP + NET)
build_dual_core() {
    local app_board="nrf5340dk/nrf5340/cpuapp"
    local net_board="nrf5340dk/nrf5340/cpunet"
    
    if [ ! -f "app/CMakeLists.txt" ]; then
        print_error "Must run from software/ directory"
        exit 1
    fi
    
    print_info "Building APP Core (Matter stack)..."
    if build_app_core; then
        echo ""
        print_success "APP Core built successfully"
    else
        print_error "APP Core build failed"
        exit 1
    fi
    
    echo ""
    print_info "Building NET Core (OpenThread stack)..."
    if build_net_core; then
        echo ""
        print_success "NET Core built successfully"
    else
        print_error "NET Core build failed"
        exit 1
    fi
    
    echo ""
    print_header "Dual-Core Build Complete"
    print_success "APP Core: app/build_app/zephyr/zephyr.hex"
    print_success "NET Core: app/build_net/zephyr/zephyr.hex"
    echo ""
    print_info "Next: ./make.sh flash"
}

# Build APP core (Matter protocol stack)
build_app_core() {
    local board="nrf5340dk/nrf5340/cpuapp"
    local build_dir="app/build_app"
    
    if [ ! -f "app/CMakeLists.txt" ]; then
        print_error "Must run from software/ directory"
        exit 1
    fi
    
    print_info "Board: $board"
    print_info "Build dir: $build_dir"
    echo ""
    
    if west build -b "$board" -d "$build_dir" app -c; then
        echo ""
        print_success "APP Core build successful"
        ls -lh "$build_dir/zephyr/zephyr.hex" 2>/dev/null || true
        return 0
    else
        print_error "APP Core build failed"
        return 1
    fi
}

# Build NET core (OpenThread + BLE radio scheduler)
build_net_core() {
    local board="nrf5340dk/nrf5340/cpunet"
    local build_dir="app/build_net"
    
    if [ ! -f "app/CMakeLists.txt" ]; then
        print_error "Must run from software/ directory"
        exit 1
    fi
    
    print_info "Board: $board"
    print_info "Build dir: $build_dir"
    echo ""
    
    if west build -b "$board" -d "$build_dir" app -c -- -DCONF_FILE=prj_cpunet.conf; then
        echo ""
        print_success "NET Core build successful"
        ls -lh "$build_dir/zephyr/zephyr.hex" 2>/dev/null || true
        return 0
    else
        print_error "NET Core build failed"
        return 1
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

# Flash firmware to nRF5340 DK
flash_firmware() {
    local port="$1"
    
    # Check if both build directories exist
    if [ ! -f "app/build_app/zephyr/zephyr.hex" ]; then
        print_error "APP Core binary not found. Build first: ./make.sh build-app"
        exit 1
    fi
    
    if [ ! -f "app/build_net/zephyr/zephyr.hex" ]; then
        print_error "NET Core binary not found. Build first: ./make.sh build-net"
        exit 1
    fi
    
    if [ -z "$port" ]; then
        port=$(detect_serial_port)
    fi
    
    print_header "Flashing nRF5340 DK"
    print_info "Serial port: $port"
    print_info "APP Core: app/build_app/zephyr/zephyr.hex"
    print_info "NET Core: app/build_net/zephyr/zephyr.hex"
    echo ""
    
    # Verify west is available and can flash
    if ! command_exists west; then
        print_error "west not found"
        echo "Install: pip3 install --user west"
        exit 1
    fi
    
    # Check if nrfjprog is available (Nordic Segger J-Link flashing tool)
    if ! command_exists nrfjprog; then
        print_warning "nrfjprog not found. Attempting to flash via west..."
        print_info "For optimal performance, install nRF Command Line Tools"
        echo ""
    fi
    
    print_info "Flashing APP Core..."
    if west flash -d app/build_app -r jlink 2>/dev/null || west flash -d app/build_app -r nrfjprog 2>/dev/null || west flash -d app/build_app; then
        print_success "APP Core flashed successfully"
    else
        print_error "APP Core flash failed"
        echo ""
        print_warning "Troubleshooting:"
        echo "  1. Install nRF Command Line Tools: https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools"
        echo "  2. Connect nRF5340 DK via USB"
        echo "  3. Check USB connection: lsusb | grep SEGGER"
        exit 1
    fi
    
    echo ""
    print_info "Flashing NET Core..."
    if west flash -d app/build_net -r jlink 2>/dev/null || west flash -d app/build_net -r nrfjprog 2>/dev/null || west flash -d app/build_net; then
        print_success "NET Core flashed successfully"
    else
        print_error "NET Core flash failed"
        exit 1
    fi
    
    echo ""
    print_header "Flash Complete"
    print_success "Both cores flashed successfully to nRF5340 DK"
    echo ""
    print_info "LEDs on board:"
    echo "  LED1 (P0.28) = Matter/commissioning status"
    echo "  LED2 (P1.6)  = Thread network status"
    echo ""
    print_info "Buttons on board:"
    echo "  Button1 (P0.23) = Toggle LED, short press to commission"
    echo "  Button2 (P0.24) = Factory reset"
    echo ""
    print_info "Monitor output: ./make.sh monitor"
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
    print_info "Debug UART output from APP core"
    print_info "Press Ctrl+A then K to exit screen, or Ctrl+C to exit minicom"
    print_info "Press RESET button on nRF5340 DK to restart"
    echo ""
    sleep 2
    
    if command_exists screen; then
        screen "$port" "$baudrate"
    elif command_exists minicom; then
        minicom -D "$port" -b "$baudrate"
    else
        print_error "No serial monitor found"
        echo "Install: sudo apt install screen (or minicom)"
        exit 1
    fi
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

# Export functions
export -f print_header
export -f print_success
export -f print_error
export -f print_warning
export -f print_info
export -f command_exists
export -f check_environment
export -f build_dual_core
export -f build_app_core
export -f build_net_core
export -f clean_build
export -f detect_serial_port
export -f flash_firmware
export -f open_monitor
export -f build_docs
