Voice Control System
====================

Overview
--------

The voice control system enables wake-word detection and audio processing using an I2S microphone, with MQTT communication to a Raspberry Pi broker for command processing and OTA firmware updates.

.. contents::
   :local:
   :depth: 2

Architecture
------------

System Components
~~~~~~~~~~~~~~~~~

.. code-block:: text

    ┌─────────────────────────────────────────────────────────┐
    │                    ESP32 Device                          │
    │                                                          │
    │  ┌──────────────┐      ┌───────────────┐               │
    │  │  I2S Mic     │─────▶│  Audio Buffer │               │
    │  │  (INMP441)   │      │  Ring Buffer  │               │
    │  └──────────────┘      └───────┬───────┘               │
    │                                 │                        │
    │                                 ▼                        │
    │                        ┌────────────────┐               │
    │                        │  Wake Word     │               │
    │                        │  Detection     │               │
    │                        │  (Local)       │               │
    │                        └────────┬───────┘               │
    │                                 │                        │
    │                                 ▼                        │
    │                        ┌────────────────┐               │
    │                        │  Audio/Text    │               │
    │                        │  Encoding      │               │
    │                        └────────┬───────┘               │
    │                                 │                        │
    │  ┌─────────────────────────────▼────────────────────┐  │
    │  │            MQTT Client Module                     │  │
    │  │  • Publish: audio/text/telemetry                 │  │
    │  │  • Subscribe: control commands                   │  │
    │  └─────────────────────┬────────────────────────────┘  │
    │                        │                                │
    │  ┌─────────────────────▼────────────────────────────┐  │
    │  │            OTA Update Module                      │  │
    │  │  • HTTP GET from Pi                              │  │
    │  │  • Firmware validation                           │  │
    │  └──────────────────────────────────────────────────┘  │
    └───────────────────────┬──────────────────────────────┘
                            │ WiFi/LAN
                            │
    ┌───────────────────────▼──────────────────────────────┐
    │               Raspberry Pi Broker                     │
    │                                                       │
    │  ┌────────────────┐    ┌──────────────────┐         │
    │  │  MQTT Broker   │    │  Voice Processing │         │
    │  │  (Mosquitto)   │◀──▶│  (Whisper/Piper) │         │
    │  └────────────────┘    └──────────────────┘         │
    │                                                       │
    │  ┌────────────────┐    ┌──────────────────┐         │
    │  │  HTTP Server   │    │  Home Assistant  │         │
    │  │  (OTA updates) │    │  Integration     │         │
    │  └────────────────┘    └──────────────────┘         │
    └───────────────────────────────────────────────────────┘

Data Flow
~~~~~~~~~

1. **Audio Capture**: I2S microphone continuously captures audio
2. **Wake Word**: Local detection (e.g., "Hey ESP")
3. **Audio Encoding**: Convert to compressed format (OPUS/PCM)
4. **MQTT Publish**: Send audio chunk or transcribed text to ``voice/audio`` or ``voice/text``
5. **Command Reception**: Subscribe to ``control/command`` for responses
6. **Telemetry**: Publish sensor data to ``telemetry/sensors``
7. **OTA**: Receive update notification via ``control/ota``, download from Pi

I2S Microphone Module
---------------------

Hardware Setup
~~~~~~~~~~~~~~

INMP441 I2S Microphone connections:

.. list-table::
   :header-rows: 1
   :widths: 20 20 60

   * - Pin
     - ESP32
     - Description
   * - SCK
     - GPIO26
     - Serial Clock
   * - WS
     - GPIO25
     - Word Select (L/R)
   * - SD
     - GPIO33
     - Serial Data
   * - VDD
     - 3.3V
     - Power
   * - GND
     - GND
     - Ground

Configuration
~~~~~~~~~~~~~

Enable I2S in ``prj.conf``:

.. code-block:: kconfig

   CONFIG_I2S=y
   CONFIG_I2S_ESP32=y
   CONFIG_AUDIO_CODEC=y

API Reference
~~~~~~~~~~~~~

.. code-block:: cpp

   class I2SMicModule : public Module {
   public:
       static I2SMicModule& getInstance();
       
       int init() override;
       int start() override;
       int stop() override;
       
       // Read audio samples
       int read(int16_t* buffer, size_t samples);
       
       // Set callback for audio data
       void setAudioCallback(AudioCallback callback);
       
   private:
       static constexpr uint32_t SAMPLE_RATE = 16000;
       static constexpr uint8_t BITS_PER_SAMPLE = 16;
   };

Wake Word Detection
-------------------

Local Processing
~~~~~~~~~~~~~~~~

Lightweight wake word detection runs on ESP32:

.. code-block:: cpp

   class WakeWordDetector {
   public:
       static WakeWordDetector& getInstance();
       
       // Initialize with model
       int init(const char* model_path);
       
       // Process audio frame
       bool detect(const int16_t* audio, size_t samples);
       
       // Set detection callback
       void setCallback(void (*callback)(const char* word));
       
   private:
       static constexpr float CONFIDENCE_THRESHOLD = 0.8f;
   };

Supported Wake Words
~~~~~~~~~~~~~~~~~~~~

- "Hey ESP"
- "OK Home"
- Custom trained models via Edge Impulse

MQTT Module
-----------

Configuration
~~~~~~~~~~~~~

.. code-block:: kconfig

   CONFIG_MQTT_LIB=y
   CONFIG_NET_SOCKETS=y
   CONFIG_DNS_RESOLVER=y

API Reference
~~~~~~~~~~~~~

.. code-block:: cpp

   class MQTTModule : public Service {
   public:
       struct Config {
           const char* broker_host;  // Pi IP or hostname
           uint16_t broker_port;     // Default: 1883
           const char* client_id;
           const char* username;
           const char* password;
       };
       
       static MQTTModule& getInstance();
       
       int init(const Config& config) override;
       int connect();
       int disconnect();
       
       // Publish message
       int publish(const char* topic, const uint8_t* payload, 
                   size_t len, uint8_t qos = 0);
       
       // Subscribe to topic
       int subscribe(const char* topic, 
                     void (*callback)(const char* topic, 
                                     const uint8_t* payload, 
                                     size_t len));
       
       // Check connection
       bool isConnected() const;
   };

MQTT Topics
~~~~~~~~~~~

**Device → Broker (Publish)**

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Topic
     - Description
   * - ``voice/audio/<device_id>``
     - Raw/compressed audio data
   * - ``voice/text/<device_id>``
     - Transcribed text from local ASR
   * - ``telemetry/sensors/<device_id>``
     - Sensor readings (JSON)
   * - ``telemetry/status/<device_id>``
     - Device health/status
   * - ``device/online/<device_id>``
     - LWT message (online/offline)

**Broker → Device (Subscribe)**

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Topic
     - Description
   * - ``control/command/<device_id>``
     - Control commands (JSON)
   * - ``control/ota/<device_id>``
     - OTA update notification
   * - ``control/config/<device_id>``
     - Configuration updates

Message Formats
~~~~~~~~~~~~~~~

**Telemetry Message**:

.. code-block:: json

   {
       "timestamp": 1234567890,
       "device_id": "esp32_001",
       "sensors": {
           "temperature": 23.5,
           "humidity": 45.2,
           "motion": false
       }
   }

**Control Command**:

.. code-block:: json

   {
       "command": "light_on",
       "params": {
           "brightness": 80,
           "color": "warm"
       }
   }

**OTA Notification**:

.. code-block:: json

   {
       "version": "1.2.0",
       "url": "http://192.168.1.100:8000/firmware.bin",
       "checksum": "sha256:abc123..."
   }

OTA Update Module
-----------------

Configuration
~~~~~~~~~~~~~

.. code-block:: kconfig

   CONFIG_BOOTLOADER_MCUBOOT=y
   CONFIG_IMG_MANAGER=y
   CONFIG_MCUMGR=y
   CONFIG_HTTP_CLIENT=y

API Reference
~~~~~~~~~~~~~

.. code-block:: cpp

   class OTAModule : public Module {
   public:
       enum class State {
           IDLE,
           DOWNLOADING,
           VERIFYING,
           UPDATING,
           COMPLETE,
           FAILED
       };
       
       static OTAModule& getInstance();
       
       int init() override;
       
       // Start OTA update from URL
       int startUpdate(const char* url, const char* checksum);
       
       // Get current state
       State getState() const;
       
       // Get progress (0-100)
       uint8_t getProgress() const;
       
       // Set progress callback
       void setProgressCallback(void (*callback)(uint8_t progress));
       
   private:
       static constexpr size_t DOWNLOAD_BUFFER_SIZE = 4096;
   };

OTA Process Flow
~~~~~~~~~~~~~~~~

1. **Notification**: Receive OTA command via MQTT
2. **Download**: HTTP GET firmware.bin from Pi
3. **Verify**: Check SHA256 checksum
4. **Write**: Store to secondary partition
5. **Mark**: Set boot flag for new image
6. **Reboot**: Apply update
7. **Confirm**: Test and confirm or rollback

Example Usage
~~~~~~~~~~~~~

.. code-block:: cpp

   void ota_callback(const char* topic, const uint8_t* payload, size_t len) {
       // Parse OTA notification JSON
       cJSON* json = cJSON_Parse((char*)payload);
       const char* url = cJSON_GetObjectItem(json, "url")->valuestring;
       const char* checksum = cJSON_GetObjectItem(json, "checksum")->valuestring;
       
       // Start OTA update
       OTAModule& ota = OTAModule::getInstance();
       ota.setProgressCallback([](uint8_t progress) {
           LOG_INF("OTA Progress: %d%%", progress);
       });
       
       int ret = ota.startUpdate(url, checksum);
       if (ret == 0) {
           LOG_INF("OTA update started");
       }
       
       cJSON_Delete(json);
   }

Minimal CLI Firmware
--------------------

The minimal firmware includes:

Core Components
~~~~~~~~~~~~~~~

1. **MQTT Client**: Communication with broker
2. **OTA Manager**: Firmware update capability
3. **Health Monitor**: Periodic ping/status
4. **Command Handler**: Process control messages

Configuration
~~~~~~~~~~~~~

Minimal build configuration:

.. code-block:: kconfig

   # Disable unused features
   CONFIG_BT=n
   CONFIG_DISPLAY=n
   CONFIG_SENSOR=n
   
   # Enable required features
   CONFIG_MQTT_LIB=y
   CONFIG_HTTP_CLIENT=y
   CONFIG_MCUBOOT=y
   CONFIG_I2S=y
   CONFIG_WIFI=y

Build Command
~~~~~~~~~~~~~

.. code-block:: bash

   west build -b esp32_devkitc/esp32/procpu app -- \
       -DCONFIG_MINIMAL_FIRMWARE=y

Memory Footprint
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 30 30

   * - Component
     - Flash (KB)
     - RAM (KB)
   * - MQTT Client
     - 45
     - 12
   * - OTA Manager
     - 30
     - 8
   * - HTTP Client
     - 25
     - 6
   * - I2S + Audio
     - 40
     - 32
   * - WiFi Stack
     - 150
     - 80
   * - **Total**
     - **~290**
     - **~138**

Raspberry Pi Setup
------------------

MQTT Broker
~~~~~~~~~~~

Install Mosquitto:

.. code-block:: bash

   sudo apt-get update
   sudo apt-get install mosquitto mosquitto-clients
   
   # Configure
   sudo nano /etc/mosquitto/mosquitto.conf

Configuration:

.. code-block:: text

   listener 1883
   allow_anonymous false
   password_file /etc/mosquitto/passwd
   
   # Persistence
   persistence true
   persistence_location /var/lib/mosquitto/

Create user:

.. code-block:: bash

   sudo mosquitto_passwd -c /etc/mosquitto/passwd esp32_user
   sudo systemctl restart mosquitto

HTTP OTA Server
~~~~~~~~~~~~~~~

Simple Python HTTP server for firmware hosting:

.. code-block:: python

   #!/usr/bin/env python3
   import http.server
   import socketserver
   
   PORT = 8000
   DIRECTORY = "/home/pi/firmware"
   
   class Handler(http.server.SimpleHTTPRequestHandler):
       def __init__(self, *args, **kwargs):
           super().__init__(*args, directory=DIRECTORY, **kwargs)
   
   with socketserver.TCPServer(("", PORT), Handler) as httpd:
       print(f"Serving at port {PORT}")
       httpd.serve_forever()

Testing
-------

MQTT Testing
~~~~~~~~~~~~

Publish test message:

.. code-block:: bash

   mosquitto_pub -h 192.168.1.100 -t "control/command/esp32_001" \
       -m '{"command":"test"}' -u esp32_user -P password

Subscribe to telemetry:

.. code-block:: bash

   mosquitto_sub -h 192.168.1.100 -t "telemetry/#" \
       -u esp32_user -P password

OTA Testing
~~~~~~~~~~~

1. Build new firmware
2. Copy to Pi: ``scp firmware.bin pi@192.168.1.100:~/firmware/``
3. Publish OTA command:

.. code-block:: bash

   mosquitto_pub -h 192.168.1.100 -t "control/ota/esp32_001" \
       -m '{"version":"1.2.0","url":"http://192.168.1.100:8000/firmware.bin"}' \
       -u esp32_user -P password

Integration Example
-------------------

Complete example integrating all components:

.. code-block:: cpp

   void main_app() {
       // Initialize WiFi
       WiFiService& wifi = WiFiService::getInstance();
       wifi.init(WiFiService::Mode::STA);
       wifi.connect("MySSID", "password");
       
       // Initialize MQTT
       MQTTModule::Config mqtt_config = {
           .broker_host = "192.168.1.100",
           .broker_port = 1883,
           .client_id = "esp32_001",
           .username = "esp32_user",
           .password = "password"
       };
       MQTTModule& mqtt = MQTTModule::getInstance();
       mqtt.init(mqtt_config);
       mqtt.connect();
       
       // Subscribe to control topics
       mqtt.subscribe("control/command/esp32_001", command_handler);
       mqtt.subscribe("control/ota/esp32_001", ota_handler);
       
       // Initialize I2S microphone
       I2SMicModule& mic = I2SMicModule::getInstance();
       mic.init();
       mic.setAudioCallback(audio_callback);
       mic.start();
       
       // Start health monitor (periodic ping)
       k_timer_start(&health_timer, K_SECONDS(30), K_SECONDS(30));
   }

See Also
--------

- :doc:`wifi_service` - WiFi configuration
- :doc:`ble_service` - BLE communication
- :doc:`inter_thread_comm` - Message queues
- :doc:`architecture` - System architecture
