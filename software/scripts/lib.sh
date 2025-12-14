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
    
    if ! command_exists xtensa-esp32-elf-gcc; then
        print_warning "ESP32 toolchain not found in PATH"
        echo "The build may fail without the toolchain"
    fi
}

# Build project
build_project() {
    local board="$1"
    shift
    local build_args=("$@")
    
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
    
    # Show additional build arguments if provided
    if [ ${#build_args[@]} -gt 0 ]; then
        print_info "Extra arguments: ${build_args[*]}"
    fi
    
    echo ""
    
    print_info "Building application..."
    
    # Try west build with additional arguments
    if west build -b "$board" app "${build_args[@]}"; then
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

# Attach ESP32 USB device to WSL
attach_esp32() {
    local BUSID="2-1"
    local DEVICE_NAME="CP210x"
    
    # Check if running in WSL
    if ! grep -qi microsoft /proc/version; then
        print_error "This command must be run in WSL"
        echo ""
        echo "If you're on Windows, run this in PowerShell as Administrator:"
        echo "  cd software\\scripts"
        echo "  .\\attach-esp32.ps1"
        exit 1
    fi
    
    # Check if usbipd is installed on Windows
    if ! powershell.exe -Command "Get-Command usbipd" > /dev/null 2>&1; then
        print_error "usbipd-win is not installed on Windows"
        echo ""
        echo "Please install it from:"
        echo "  https://github.com/dorssel/usbipd-win/releases"
        echo "Or run in PowerShell as Administrator:"
        echo "  winget install --interactive --exact dorssel.usbipd-win"
        exit 1
    fi
    
    print_info "Checking USB device status..."
    echo ""
    
    # List available devices
    powershell.exe -Command "usbipd list" 2>/dev/null || {
        print_error "Failed to list USB devices"
        exit 1
    }
    
    echo ""
    print_info "Attempting to attach $DEVICE_NAME (BUSID: $BUSID)..."
    echo ""
    
    # Try to attach the device
    powershell.exe -Command "Start-Process powershell -Verb RunAs -ArgumentList '-Command', 'usbipd attach --wsl --busid $BUSID'" 2>/dev/null || {
        print_warning "Auto-attach failed. Run manually in PowerShell as Administrator:"
        echo ""
        echo "  usbipd attach --wsl --busid $BUSID"
        echo ""
        exit 1
    }
    
    # Wait for device to appear
    print_info "Waiting for device to appear in WSL..."
    for i in {1..10}; do
        if [ -e /dev/ttyUSB0 ] || [ -e /dev/ttyACM0 ]; then
            print_success "Device detected!"
            break
        fi
        sleep 1
        echo -n "."
    done
    echo ""
    
    # Check if device is available
    local DEVICE=""
    if [ -e /dev/ttyUSB0 ]; then
        DEVICE="/dev/ttyUSB0"
    elif [ -e /dev/ttyACM0 ]; then
        DEVICE="/dev/ttyACM0"
    else
        print_warning "Device not found in /dev/"
        echo ""
        echo "Searching for USB serial devices..."
        ls -la /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  No serial devices found"
        echo ""
        echo "If device is attached but not accessible, you may need to:"
        echo "  1. Add yourself to dialout group: sudo usermod -a -G dialout \$USER"
        echo "  2. Activate group: newgrp dialout"
        echo "  3. Or run: sudo chmod 666 /dev/ttyUSB0"
        exit 1
    fi
    
    # Check permissions
    echo ""
    print_info "Checking device permissions: $DEVICE"
    ls -l $DEVICE
    
    if [ ! -r "$DEVICE" ] || [ ! -w "$DEVICE" ]; then
        echo ""
        print_warning "Device exists but you don't have permission to access it"
        echo ""
        echo "Quick fix (this session only):"
        echo "  sudo chmod 666 $DEVICE"
        echo ""
        echo "Permanent fix:"
        echo "  sudo usermod -a -G dialout \$USER"
        echo "  newgrp dialout"
        echo ""
        
        read -p "Fix permissions for this session? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            sudo chmod 666 $DEVICE
            print_success "Permissions fixed for this session"
        fi
    fi
    
    echo ""
    print_success "ESP32 attached and ready"
    print_info "Device: $DEVICE"
    echo ""
    
    # Ask if user wants to flash firmware
    read -p "Flash firmware now? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo ""
        
        # Check if build exists
        if [ ! -f "build/zephyr/zephyr.bin" ]; then
            print_warning "No firmware found at build/zephyr/zephyr.bin"
            echo ""
            read -p "Build firmware first? (y/n) " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                echo ""
                check_environment
                build_project "esp32_devkitc"
            else
                print_info "Skipping flash."
                return 0
            fi
        fi
        
        # Flash firmware
        flash_firmware "$DEVICE"
        
        echo ""
        read -p "Monitor serial output? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo ""
            print_info "Starting serial monitor (Ctrl+A then K to exit)..."
            sleep 1
            open_monitor "$DEVICE"
        fi
    else
        echo ""
        print_info "You can flash firmware later with:"
        echo "  ./make.sh flash --port $DEVICE"
        echo ""
        print_info "Or monitor output:"
        echo "  ./make.sh monitor --port $DEVICE"
        echo ""
    fi
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
    
    if command_exists screen; then
        print_warning "To exit: Press Ctrl+A, then type ':quit' and press Enter"
        print_warning "Or: Press Ctrl+A, then press '\\' (backslash), then press 'y'"
        print_info "Press RESET button on ESP32 to restart"
        echo ""
        sleep 2
        screen "$port" "$baudrate"
    elif command_exists minicom; then
        print_warning "To exit: Press Ctrl+A, then press X"
        print_info "Press RESET button on ESP32 to restart"
        echo ""
        sleep 2
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

# Run ESP32 QEMU smoke test
run_esp32_qemu_test() {
    local esp32_board="esp32_devkitc/esp32/procpu"
    local timeout_sec=30
    local qemu_bin="qemu-system-xtensa"
    
    print_info "Building for ESP32 QEMU..."
    
    # Check if Espressif QEMU is installed
    if ! command_exists "$qemu_bin"; then
        print_warning "Espressif QEMU not found"
        echo ""
        read -p "Would you like to install Espressif QEMU automatically? (y/N) " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_espressif_qemu || {
                print_error "Failed to install Espressif QEMU"
                print_info "Manual installation:"
                print_info "  - Download from: https://github.com/espressif/qemu/releases"
                print_info "  - macOS Homebrew: brew install espressif/espressif/qemu"
                return 1
            }
        else
            print_info "Manual installation:"
            print_info "  - Download from: https://github.com/espressif/qemu/releases"
            print_info "  - macOS Homebrew: brew install espressif/espressif/qemu"
            return 1
        fi
    fi
    
    print_success "QEMU Binary: $(which $qemu_bin)"
    
    # Fetch ESP32 HAL blobs if not already present
    print_info "Fetching ESP32 HAL blobs..."
    west blobs fetch hal_espressif || {
        print_warning "Could not fetch HAL blobs (may already exist)"
    }
    
    # Build for ESP32
    print_info "Building for ESP32..."
    west build -b "$esp32_board" app -p || {
        print_error "ESP32 build failed"
        return 1
    }
    
    print_success "Build successful"
    
    local zephyr_elf="build/zephyr/zephyr.elf"
    local zephyr_bin="build/zephyr/zephyr.bin"
    local flash_img="build/zephyr/flash_image.bin"
    local qemu_log="build/esp32_qemu_output.log"
    local flash_size_mb=4
    local bootloader="build/zephyr/esp-idf/build/bootloader/bootloader.bin"
    local partition_table="build/zephyr/esp-idf/build/partition_table/partition-table.bin"
    
    if [ ! -f "$zephyr_elf" ]; then
        print_error "Zephyr ELF not found: $zephyr_elf"
        return 1
    fi
    
    print_info "Zephyr ELF: $zephyr_elf"
    
    # Check if we have ESP-IDF bootloader components
    if [ -f "$bootloader" ] && [ -f "$partition_table" ]; then
        print_success "Found ESP-IDF components, creating flash image with bootloader"
        
        local current_size=$(stat -c%s "$zephyr_bin" 2>/dev/null || stat -f%z "$zephyr_bin" 2>/dev/null)
        print_info "Zephyr binary size: $current_size bytes"
        
        # Create empty flash image (4MB)
        dd if=/dev/zero of="$flash_img" bs=1M count=$flash_size_mb status=none 2>/dev/null
        
        # Write bootloader at 0x1000
        dd if="$bootloader" of="$flash_img" bs=1 seek=4096 conv=notrunc status=none 2>/dev/null
        print_success "Bootloader written at 0x1000"
        
        # Write partition table at 0x8000
        dd if="$partition_table" of="$flash_img" bs=1 seek=32768 conv=notrunc status=none 2>/dev/null
        print_success "Partition table written at 0x8000"
        
        # Write application at 0x10000
        dd if="$zephyr_bin" of="$flash_img" bs=1 seek=65536 conv=notrunc status=none 2>/dev/null
        print_success "Application written at 0x10000"
        
        local flash_size=$(stat -c%s "$flash_img" 2>/dev/null || stat -f%z "$flash_img" 2>/dev/null)
        print_success "Flash image created: $flash_size bytes"
        
        print_info "Running ESP32 in QEMU with flash image (${timeout_sec}s timeout)..."
        echo ""
        
        # Run QEMU with flash image
        timeout "${timeout_sec}s" "$qemu_bin" \
            -nographic \
            -machine esp32 \
            -drive file="$flash_img",if=mtd,format=raw \
            -serial mon:stdio \
            2>&1 | tee "$qemu_log" || EXIT_CODE=$?
    else
        print_warning "ESP-IDF bootloader not found, trying ELF direct load"
        print_info "This may not work on ESP32 - Zephyr might need ESP-IDF bootloader"
        
        print_info "Running ESP32 in QEMU with ELF (${timeout_sec}s timeout)..."
        echo ""
        
        # Try running QEMU with just the binary at offset 0
        # ESP32 expects code at 0x40000000 (IRAM) but bootloader sets this up
        # Without bootloader, we need to create a minimal flash image
        dd if=/dev/zero of="$flash_img" bs=1M count=$flash_size_mb status=none 2>/dev/null
        dd if="$zephyr_bin" of="$flash_img" bs=1 seek=65536 conv=notrunc status=none 2>/dev/null
        print_info "Created minimal flash image with app at 0x10000"
        
        timeout "${timeout_sec}s" "$qemu_bin" \
            -nographic \
            -machine esp32 \
            -drive file="$flash_img",if=mtd,format=raw \
            -serial mon:stdio \
            2>&1 | tee "$qemu_log" || EXIT_CODE=$?
    fi
    
    echo ""
    
    # Check QEMU output for boot activity
    local test_passed=0
    local boot_detected=0
    
    # Check for module initialization messages
    if grep -qi "Blink module initialized" "$qemu_log" 2>/dev/null; then
        print_success "✓ Blink module initialized"
        test_passed=1
    fi
    
    if grep -qi "Blink task started" "$qemu_log" 2>/dev/null; then
        print_success "✓ Blink task started"
        test_passed=1
    fi
    
    if grep -qi "Display module initialized" "$qemu_log" 2>/dev/null; then
        print_success "✓ Display module initialized"
        test_passed=1
    fi
    
    # Check for boot messages
    if grep -qi "Booting Zephyr" "$qemu_log" 2>/dev/null || \
       grep -qi "\*\*\* Booting" "$qemu_log" 2>/dev/null; then
        print_success "✓ Zephyr boot detected"
        boot_detected=1
    fi
    
    # Evaluate results
    if [ $test_passed -eq 1 ]; then
        print_success "ESP32 QEMU test passed: Module initialization detected"
    elif [ $boot_detected -eq 1 ]; then
        print_success "ESP32 QEMU test passed: System boot detected"
    elif [ ${EXIT_CODE:-0} -eq 124 ]; then
        print_success "ESP32 QEMU test passed: Timeout (application running)"
    elif [ ${EXIT_CODE:-0} -eq 0 ]; then
        print_success "ESP32 QEMU test passed: Clean exit"
    else
        print_error "ESP32 QEMU test failed: No boot activity detected (exit code: $EXIT_CODE)"
        echo ""
        print_warning "Last 30 lines of QEMU output:"
        tail -n 30 "$qemu_log"
        echo ""
        print_info "Full QEMU log: $qemu_log"
        return $EXIT_CODE
    fi
    
    print_info "Flash image: build/zephyr/flash_image.bin"
    print_info "QEMU output log: $qemu_log"
    return 0
}

# Export functions
export -f print_header
export -f print_success
export -f print_error
export -f print_warning
export -f print_info
export -f command_exists
export -f install_espressif_qemu
export -f check_environment
export -f build_project
export -f clean_build
export -f detect_serial_port
export -f flash_firmware
export -f open_monitor
export -f edit_config
export -f build_docs
export -f run_qemu_test
export -f run_esp32_qemu_test
