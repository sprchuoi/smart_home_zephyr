# MQTT Configuration

This file explains how to configure MQTT settings for the ESP32 Smart Home device.

## Quick Start

1. **Copy the template:**
   ```bash
   cd software/app
   cp mqtt_config.conf.template mqtt_config.conf
   ```

2. **Edit `mqtt_config.conf` with your MQTT broker credentials:**
   ```properties
   CONFIG_MQTT_BROKER_HOSTNAME="192.168.2.1"
   CONFIG_MQTT_BROKER_PORT=1883
   CONFIG_MQTT_CLIENT_ID="esp32_001"
   CONFIG_MQTT_USERNAME="esp32_user"
   CONFIG_MQTT_PASSWORD="YourSecurePassword"
   CONFIG_MQTT_DEVICE_ID="esp32_001"
   ```

3. **Build with both configs:**
   ```bash
   ./make.sh build -- -DEXTRA_CONF_FILE="app/wifi_config.conf;app/mqtt_config.conf"
   ```

## Configuration Options

### MQTT Broker Settings

- **CONFIG_MQTT_BROKER_HOSTNAME**: Broker IP address or hostname
  - Example: `"192.168.2.1"` or `"mqtt.example.com"`
  - Default: `"192.168.2.1"`

- **CONFIG_MQTT_BROKER_PORT**: Broker TCP port
  - Default: `1883` (non-TLS)
  - TLS: `8883` (requires CONFIG_MQTT_LIB_TLS=y)

### MQTT Authentication

- **CONFIG_MQTT_CLIENT_ID**: Unique client identifier
  - Must be unique across all clients connecting to the broker
  - Example: `"esp32_living_room"`, `"esp32_kitchen"`
  - Default: `"esp32_001"`

- **CONFIG_MQTT_USERNAME**: Broker authentication username
  - Required if broker has authentication enabled
  - Default: `"esp32_user"`

- **CONFIG_MQTT_PASSWORD**: Broker authentication password
  - Keep this secure and never commit to git
  - Default: `"password"` (change this!)

### Device Identification

- **CONFIG_MQTT_DEVICE_ID**: Device ID used in MQTT topics
  - Used to construct topic paths
  - Example topics:
    - `smart_home/devices/{device_id}/status`
    - `smart_home/devices/{device_id}/command`
  - Default: `"esp32_001"`

## MQTT Topic Structure

The device uses the following topic hierarchy:

```
smart_home/devices/{device_id}/status    # Device status (publish)
smart_home/devices/{device_id}/command   # Device commands (subscribe)
smart_home/devices/{device_id}/telemetry # Sensor data (publish)
```

### Status Message Format

```json
{
  "device_id": "esp32_001",
  "device_type": "sensor",
  "status": "connected",
  "ip": "192.168.2.15",
  "rssi": -65,
  "timestamp": 12345
}
```

## Security Notes

- **mqtt_config.conf is gitignored** - Your credentials will not be committed
- Never commit actual credentials to version control
- Use strong passwords for MQTT authentication
- Consider using TLS for production deployments

## Testing Your Configuration

1. **Start a local MQTT broker (optional):**
   ```bash
   docker run -p 1883:1883 -p 9001:9001 eclipse-mosquitto
   ```

2. **Subscribe to status messages:**
   ```bash
   mosquitto_sub -h 192.168.2.1 -t "smart_home/devices/+/status" -v
   ```

3. **Publish a test command:**
   ```bash
   mosquitto_pub -h 192.168.2.1 -t "smart_home/devices/esp32_001/command" \
     -m '{"action":"test","value":123}'
   ```

## Multiple Device Configuration

For multiple ESP32 devices, create separate config files:

```bash
# Device 1
CONFIG_MQTT_CLIENT_ID="esp32_living_room"
CONFIG_MQTT_DEVICE_ID="esp32_living_room"

# Device 2
CONFIG_MQTT_CLIENT_ID="esp32_kitchen"
CONFIG_MQTT_DEVICE_ID="esp32_kitchen"
```

## Build Examples

```bash
# Build with WiFi and MQTT config
./make.sh build -- -DEXTRA_CONF_FILE="app/wifi_config.conf;app/mqtt_config.conf"

# Flash the device
./make.sh flash

# Monitor serial output
./make.sh monitor
```

## Troubleshooting

- **Connection fails**: Check broker IP, port, and firewall settings
- **Authentication fails**: Verify username/password are correct
- **Timeout**: Ensure broker is running and reachable on the network
- **Connection drops**: Check broker keep-alive settings and network stability
