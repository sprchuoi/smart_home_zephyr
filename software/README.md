# ESP32 Smart Home - Voice Control Application

<a href="https://github.com/zephyrproject-rtos/example-application/actions/workflows/build.yml?query=branch%3Amain">
  <img src="https://github.com/zephyrproject-rtos/example-application/actions/workflows/build.yml/badge.svg?event=push">
</a>
<a href="https://github.com/zephyrproject-rtos/example-application/actions/workflows/docs.yml?query=branch%3Amain">
  <img src="https://github.com/zephyrproject-rtos/example-application/actions/workflows/docs.yml/badge.svg?event=push">
</a>
<a href="https://zephyrproject-rtos.github.io/example-application">
  <img alt="Documentation" src="https://img.shields.io/badge/documentation-3D578C?logo=sphinx&logoColor=white">
</a>
<a href="https://zephyrproject-rtos.github.io/example-application/doxygen">
  <img alt="API Documentation" src="https://img.shields.io/badge/API-documentation-3D578C?logo=c&logoColor=white">
</a>

Voice-controlled smart home system built on Zephyr RTOS for ESP32, featuring local wake-word detection, MQTT communication with Raspberry Pi broker, and OTA firmware updates.

## Features

### Core Capabilities
- **Voice Control**: I2S microphone (INMP441) with local wake-word detection
  - Supports Edge Impulse models (TensorFlow Lite for Microcontrollers)
  - Custom model format support
  - Placeholder mode for testing
- **MQTT Communication**: Publish/subscribe to Raspberry Pi broker (Mosquitto)
- **OTA Updates**: HTTP-based firmware updates from Pi
- **WiFi Connectivity**: STA and AP+STA modes with auto-reconnect
- **BLE**: Bluetooth Low Energy for backup communication
- **Telemetry**: Sensor data publishing and device health monitoring
- **Menu System**: Linked-list based UI navigation

### Architecture Highlights
- **C++ OOP Design**: Singleton pattern modules with clear separation
- **RTOS Tasks**: Dedicated threads for each service (WiFi, BLE, MQTT, Audio)
- **Message Queues**: Inter-thread communication (e.g., UART→BLE)
- **Conditional Compilation**: Minimal builds with CONFIG flags
- **MCUboot**: Secure boot and OTA with image verification

## System Architecture

```
ESP32 Device                    Raspberry Pi
┌─────────────────────┐        ┌──────────────────────┐
│  I2S Microphone     │        │  MQTT Broker         │
│  (INMP441)          │        │  (Mosquitto)         │
│         │           │        │                      │
│         ▼           │        │  Voice Processing    │
│  Wake Word Detect   │        │  (Whisper/Piper)     │
│         │           │        │                      │
│         ▼           │        │  HTTP Server         │
│  Audio Encoding     │        │  (OTA Firmware)      │
│         │           │        │                      │
│         ▼           │        │  Home Assistant      │
│  MQTT Client ◄─────────WiFi──► Integration         │
│         │           │        │                      │
│         ▼           │        └──────────────────────┘
│  OTA Manager        │
│         │           │
│         ▼           │
│  Control/Telemetry  │
└─────────────────────┘
```

### MQTT Topics

**Device → Broker (Publish)**
- `voice/audio/<device_id>` - Audio data (compressed)
- `voice/text/<device_id>` - Transcribed text
- `telemetry/sensors/<device_id>` - Sensor readings
- `telemetry/status/<device_id>` - Device health

**Broker → Device (Subscribe)**
- `control/command/<device_id>` - Control commands
- `control/ota/<device_id>` - OTA notifications
- `control/config/<device_id>` - Configuration updates

## Hardware Requirements

- **ESP32-DevKitC** (ESP32-WROOM-32)
- **INMP441** I2S MEMS Microphone
- **Raspberry Pi 3/4** (MQTT broker + voice processing)

### Wiring

| Component | Pin | ESP32 GPIO |
|-----------|-----|------------|
| INMP441 SCK | Serial Clock | GPIO26 |
| INMP441 WS | Word Select | GPIO25 |
| INMP441 SD | Serial Data | GPIO33 |
| INMP441 VDD | Power | 3.3V |
| INMP441 GND | Ground | GND |

## Getting Started

### Prerequisites

### Prerequisites

- Zephyr SDK 0.17.4+
- Python 3.12+
- West build tool
- Raspberry Pi with Mosquitto MQTT broker

Follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Quick Start

#### 1. Clone and Setup

```shell
git clone <repository-url> smart-home-zephyr
cd smart-home-zephyr/software
./make.sh setup
```

#### 2. Build Firmware

**Full Build (all features)**
```shell
./make.sh build
```

**Voice Control Build**
```shell
./make.sh build -- -DCONF_FILE=voice.conf
```

**Minimal Build (MQTT + OTA only)**
```shell
./make.sh build -- -DCONF_FILE=minimal.conf
```

#### 3. Flash to ESP32

```shell
./make.sh flash --port /dev/ttyUSB0
```

#### 4. Monitor Output

```shell
./make.sh monitor
```

### Raspberry Pi Setup

#### Install MQTT Broker

```shell
sudo apt-get update
sudo apt-get install mosquitto mosquitto-clients

# Configure authentication
sudo mosquitto_passwd -c /etc/mosquitto/passwd esp32_user
sudo systemctl restart mosquitto
```

#### HTTP OTA Server

```shell
# Create firmware directory
mkdir -p ~/firmware

# Start simple HTTP server
python3 -m http.server 8000 --directory ~/firmware
```

#### Test MQTT Connection

```shell
# Subscribe to telemetry
mosquitto_sub -h localhost -t "telemetry/#" -u esp32_user -P password

# Publish test command
mosquitto_pub -h localhost -t "control/command/esp32_001" \
  -m '{"command":"test"}' -u esp32_user -P password
```

## Build Commands

```shell
./make.sh setup       # First-time Zephyr setup
./make.sh build       # Build firmware
./make.sh clean       # Clean build artifacts
./make.sh flash       # Flash to device
./make.sh monitor     # Open serial monitor
./make.sh all         # Build and flash
./make.sh qemu        # Run smoke test in QEMU
./make.sh docs        # Build documentation
./make.sh config      # Edit WiFi configuration
```

## Configuration

Edit `app/prj.conf` for feature flags:

```kconfig
# WiFi
CONFIG_WIFI=y
CONFIG_ESP32_WIFI_AP_STA_MODE=y

# BLE
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y

# MQTT
CONFIG_MQTT_LIB=y

# I2S Microphone
CONFIG_I2S=y
CONFIG_I2S_ESP32=y

# OTA
CONFIG_BOOTLOADER_MCUBOOT=y
CONFIG_IMG_MANAGER=y
```

## Memory Footprint

| Configuration | Flash | RAM |
|---------------|-------|-----|
| Full (WiFi+BLE+MQTT+I2S) | ~720 KB | ~180 KB |
| Minimal (MQTT+OTA) | ~290 KB | ~138 KB |

## Testing

### Unit Tests

```shell
west twister -T tests --integration
```

### QEMU Smoke Test

```shell
./make.sh qemu
```

### OTA Test

```shell
# Build new firmware
./make.sh build

# Copy to Pi
scp build/zephyr/zephyr.bin pi@192.168.1.100:~/firmware/firmware.bin

# Trigger OTA via MQTT
mosquitto_pub -h 192.168.1.100 -t "control/ota/esp32_001" \
  -m '{"version":"1.2.0","url":"http://192.168.1.100:8000/firmware.bin"}' \
  -u esp32_user -P password
```

## Documentation

### Build Documentation

```shell
./make.sh docs
```

Documentation includes:
- **Architecture** - System design and patterns
- **BLE Service** - Bluetooth Low Energy API
- **WiFi Service** - WiFi connectivity and AP mode
- **Voice System** - I2S microphone, wake-word (with Edge Impulse integration), MQTT, OTA
- **Inter-thread Communication** - Message queues and semaphores

### Manual Build

```shell
cd doc
pip install -r requirements.txt
doxygen              # API documentation
make html            # Sphinx documentation
```

## Project Structure

```
software/
├── app/
│   ├── src/
│   │   ├── main.cpp              # Application entry point
│   │   ├── menu/                 # Menu system (linked list)
│   │   ├── modules/              # Hardware modules
│   │   │   ├── ble/              # BLE service
│   │   │   ├── wifi/             # WiFi service
│   │   │   ├── uart/             # UART module
│   │   │   └── display/          # Display module
│   │   └── thread/               # RTOS tasks
│   ├── boards/                   # Board overlays
│   ├── prj.conf                  # Zephyr configuration
│   └── CMakeLists.txt
├── doc/                          # Sphinx documentation
├── tests/                        # Unit tests
├── scripts/                      # Build scripts
├── make.sh                       # Main build script
└── west.yml                      # West manifest
```

## Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/my-feature`
3. Commit changes: `git commit -am 'Add feature'`
4. Push to branch: `git push origin feature/my-feature`
5. Submit pull request

## License

Apache-2.0 License - See LICENSE file for details

## References

- [Zephyr Project Documentation](https://docs.zephyrproject.org/)
- [ESP32 Technical Reference](https://www.espressif.com/en/products/socs/esp32)
- [MQTT Protocol](https://mqtt.org/)
- [Mosquitto MQTT Broker](https://mosquitto.org/)
- [MCUboot Bootloader](https://mcuboot.com/)
