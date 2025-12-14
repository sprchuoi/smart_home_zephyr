# WiFi Configuration

## Setup

1. **Copy the template file:**
   ```bash
   cp wifi_config.conf.template wifi_config.conf
   ```

2. **Edit `wifi_config.conf` with your credentials:**
   ```conf
   CONFIG_WIFI_SSID="YourNetworkName"
   CONFIG_WIFI_PASSWORD="YourPassword"
   CONFIG_WIFI_AP_SSID="ESP32_SmartHome"
   CONFIG_WIFI_AP_PASSWORD="12345678"
   
   # Optional: Specify WiFi channel for faster connection (0 = scan all)
   CONFIG_WIFI_CHANNEL=6
   ```

3. **Build with your configuration:**
   ```bash
   cd ~/sandboxes/example-application/software
   ./make.sh build -- -DEXTRA_CONF_FILE=app/wifi_config.conf
   ```

## Speed Optimization

### Channel Hint (Recommended)
Setting the WiFi channel can reduce connection time from 8-12 seconds to 3-5 seconds by skipping channel scanning.

**Find your router's channel:**
- **Windows:** `netsh wlan show networks mode=bssid` (look for "Channel")
- **Linux/Mac:** `iwlist wlan0 scan | grep Channel` or use WiFi Analyzer app
- **Router:** Check admin page → Wireless Settings → Channel

**Add to wifi_config.conf:**
```conf
CONFIG_WIFI_CHANNEL=6  # Replace 6 with your router's channel
```

Common 2.4GHz channels: 1, 6, 11 (non-overlapping)

### Other Optimizations
The firmware includes:
- **Reduced initialization delay**: 500ms instead of 2s
- **Connection timeout**: 15s (was infinite)
- **Delayed AP mode**: Starts AP after station connects (faster boot)
- **Smart reconnection**: 60s backoff with state tracking

## Security Notes

- ✅ `wifi_config.conf` is **gitignored** and won't be committed
- ✅ Your credentials stay local to your machine
- ✅ Template file shows the format without real credentials
- ⚠️ Never commit `wifi_config.conf` with real passwords

## Alternative: Use Kconfig

You can also set WiFi credentials using Kconfig:

```bash
cd software
west build -t menuconfig app
# Navigate to: WiFi Configuration
# Set SSID and Password
```

## Default Values

If `wifi_config.conf` is not found, these defaults are used:

- **Station SSID:** `ESP32_Network`
- **Station Password:** `password123`
- **AP SSID:** `ESP32_SmartHome`
- **AP Password:** `12345678`

## Build Without Config File

If you don't create `wifi_config.conf`, the build will use defaults from `prj.conf`:

```bash
./make.sh build
```

## Example: Full Build Process

```bash
# 1. Setup credentials
cp app/wifi_config.conf.template app/wifi_config.conf
nano app/wifi_config.conf  # Edit with your WiFi credentials

# 2. Build with credentials
./make.sh build -- -DEXTRA_CONF_FILE=app/wifi_config.conf

# 3. Flash to ESP32
./make.sh flash --port /dev/ttyUSB0

# 4. Monitor
./make.sh monitor --port /dev/ttyUSB0
```

## Troubleshooting

**Q: Build fails with "file not found"**
- A: Create `wifi_config.conf` from the template, or build without it (uses defaults)

**Q: WiFi doesn't connect**
- A: Check SSID and password in `wifi_config.conf`
- A: Verify your router is on 2.4GHz (ESP32 doesn't support 5GHz)
- A: Check logs: `./make.sh monitor`

**Q: I accidentally committed my password!**
- A: Change your WiFi password immediately
- A: Remove from git history: `git filter-branch` or use BFG Repo-Cleaner
- A: Force push: `git push --force`
