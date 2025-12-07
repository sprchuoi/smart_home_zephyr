# Smart Home ESP32 - Zephyr RTOS Project

[![Documentation](https://github.com/sprchuoi/smart_home_zephyr/workflows/Documentation/badge.svg)](https://sprchuoi.github.io/smart_home_zephyr/)
[![Build](https://github.com/sprchuoi/smart_home_zephyr/workflows/Build/badge.svg)](https://github.com/sprchuoi/smart_home_zephyr/actions)

ESP32-WROOM-32 smart home system built on Zephyr RTOS with modular architecture for BLE, WiFi, sensor monitoring, and OLED display.

## Features

- **Multi-threaded Architecture**: Separate tasks for LED blink, sensor reading, BLE, WiFi, and display
- **BLE Support**: GATT service with "Hello World" notifications
- **WiFi Connectivity**: AP+STA mode (access point and station simultaneously)
- **SSD1306 OLED Display**: 128x64 I2C display with 30-second auto-sleep
- **Power Management**: Button wake from display sleep
- **Modular Design**: Clean separation between library code and task orchestration

## Hardware Requirements

- ESP32-WROOM-32 DevKit board
- SSD1306 OLED display (128x64, I2C)
- LED on GPIO2 (built-in on most DevKits)
- Button on GPIO0 (BOOT button)

## Project Structure

```
software/
├── app/
│   ├── src/
│   │   ├── main.c                      # Application entry point
│   │   ├── modules/                    # Core functionality libraries
│   │   │   ├── blink/                  # LED control
│   │   │   ├── sensor/                 # Sensor reading
│   │   │   ├── ble/                    # Bluetooth communication
│   │   │   ├── wifi/                   # WiFi AP/STA mode
│   │   │   ├── display/                # SSD1306 OLED control
│   │   │   └── button/                 # Button input handling
│   │   └── thread/                     # Task orchestration layer
│   │       ├── blink_task.c/h
│   │       ├── sensor_task.c/h
│   │       ├── ble_task.c/h
│   │       ├── wifi_task.c/h
│   │       └── display_task.c/h
│   ├── boards/
│   │   └── esp32_devkitc.overlay      # Device tree configuration
│   ├── prj.conf                        # Kconfig settings
│   └── CMakeLists.txt
├── drivers/                            # Custom Zephyr drivers
├── doc/                                # Sphinx + Doxygen documentation
├── scripts/                            # Build system scripts
│   └── lib.sh                         # Build helper functions
├── make.sh                            # Main build entry point
└── west.yml                           # West manifest

## Quick Start

### Prerequisites

- Ubuntu 20.04+ or compatible Linux distribution
- Python 3.10+
- Git

### Setup and Build

```bash
# Navigate to software directory
cd software

# One-time setup (installs Zephyr SDK, dependencies)
./make.sh setup

# Fetch ESP32 HAL blobs (WiFi/BLE firmware)
west blobs fetch hal_espressif

# Configure WiFi credentials (optional)
./make.sh config

# Build and flash to ESP32
./make.sh all

# Monitor serial output
./make.sh monitor
```

### Build Commands

| Command | Description |
|---------|-------------|
| `./make.sh setup` | Initialize Zephyr environment (first time only) |
| `./make.sh build` | Build the application |
| `./make.sh clean` | Clean build artifacts |
| `./make.sh flash` | Flash firmware to ESP32 |
| `./make.sh monitor` | Open serial monitor |
| `./make.sh all` | Build and flash in one step |
| `./make.sh config` | Edit WiFi configuration |

### Build Options

```bash
# Specify board (default: esp32_devkitc/esp32/procpu)
./make.sh build --board esp32_devkitc/esp32/procpu

# Specify serial port (default: auto-detect)
./make.sh flash --port /dev/ttyUSB0
```

## Configuration

### WiFi Settings

WiFi credentials can be configured in `app/prj.conf` or via the config command:

```bash
./make.sh config
```

Default AP mode settings:
- SSID: `ESP32_SmartHome_AP`
- Password: (configured in prj.conf)

### Display Settings

The SSD1306 display is configured via device tree overlay:
- I2C Address: `0x3C`
- Resolution: 128x64
- Auto-sleep: 30 seconds
- Wake: Press GPIO0 button

## Troubleshooting

### Python externally-managed-environment Error

On Ubuntu 24.04+, use the fix script:

```bash
cd software/scripts
./fix-python.sh
```

### ESP32 Toolchain Not Found

The Zephyr SDK includes the ESP32 toolchain. Ensure setup completed successfully:

```bash
./make.sh setup
source ~/zephyrproject/.venv/bin/activate
```

### Missing HAL Espressif Blobs

Required for WiFi and BLE functionality:

```bash
west blobs fetch hal_espressif
```

### Build Fails with "Board qualifiers not found"

Ensure you're using the correct board target format for Zephyr 3.x+:

```bash
./make.sh build --board esp32_devkitc/esp32/procpu
```

## Documentation

Full documentation is available at: https://sprchuoi.github.io/smart_home_zephyr/

Build documentation locally:

```bash
cd software/doc
pip install -r requirements.txt
make html
```

## Architecture

### Module Layer (`modules/`)
Core functionality libraries with clean APIs:
- **blink_module**: LED control with configurable period
- **sensor_module**: Sensor reading with callbacks
- **ble_module**: GATT service and notifications
- **wifi_module**: AP/STA mode management
- **display_module**: SSD1306 control with auto-sleep
- **button_module**: Interrupt-driven input

### Thread Layer (`thread/`)
Task wrappers that orchestrate modules:
- Each task runs in its own thread with appropriate stack size
- Tasks call module functions in loops with timing control
- Thread-safe synchronization via mutexes

### Benefits of this Architecture
1. **Testability**: Modules can be unit tested independently
2. **Reusability**: Libraries work across different applications
3. **Maintainability**: Clear separation of concerns
4. **Scalability**: Easy to add new modules and tasks

## Contributing

This project follows Zephyr RTOS coding standards and conventions.

## License

Copyright (c) Sprchuoi 
SPDX-License-Identifier: Apache-2.0

## Resources

- [Zephyr Documentation](https://docs.zephyrproject.org/)
- [ESP32 DevKit Pinout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html)
- [SSD1306 Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)