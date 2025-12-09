.. _wifi_service:

WiFi Service
############

Overview
********

The WiFi Service provides comprehensive WiFi connectivity for the ESP32, supporting three operation modes: Station (STA), Access Point (AP), and simultaneous AP+STA. It uses the Zephyr network management API and ESP32 WiFi driver.

Architecture
************

Class Design
============

.. code-block:: cpp

    class WiFiService {
    public:
        enum class Mode {
            STA,      // Station mode - connect to existing WiFi
            AP,       // Access Point mode - create WiFi hotspot
            AP_STA    // Both modes simultaneously
        };
        
        static WiFiService& getInstance();
        
        int init(Mode mode = Mode::STA);
        int start();
        int stop();
        
        // Station mode operations
        int connect(const char* ssid, const char* password);
        int disconnect();
        int scan(ScanResultCallback callback);
        
        // Access Point mode operations
        int startAP(const char* ssid, const char* password);
        int stopAP();
        
        bool isConnected() const;
        Mode getMode() const;
        
        void setConnectionCallback(ConnectionCallback cb);
        void setScanResultCallback(ScanResultCallback cb);
        
    private:
        WiFiService();
        ~WiFiService();
    };

Operation Modes
***************

Station Mode (STA)
==================

Connect to an existing WiFi network as a client.

**Use Cases**:

* Internet connectivity
* Cloud communication
* OTA updates
* Network time synchronization

**Configuration**:

.. code-block:: cpp

    WiFiService& wifi = WiFiService::getInstance();
    wifi.init(WiFiService::Mode::STA);
    wifi.connect("MyWiFi", "password123");

Access Point Mode (AP)
======================

Create a WiFi hotspot for other devices to connect.

**Use Cases**:

* Device provisioning
* Local configuration
* Direct device-to-device communication
* Captive portal for setup

**Configuration**:

.. code-block:: cpp

    WiFiService& wifi = WiFiService::getInstance();
    wifi.init(WiFiService::Mode::AP);
    wifi.startAP("ESP32_SmartHome_AP", "12345678");

AP+STA Mode
===========

Operate as both access point and station simultaneously.

**Use Cases**:

* WiFi range extender
* Gateway applications
* Configuration while maintaining internet
* Mesh networking

**Configuration**:

.. code-block:: cpp

    WiFiService& wifi = WiFiService::getInstance();
    wifi.init(WiFiService::Mode::AP_STA);
    
    // Start AP for local connections
    wifi.startAP("ESP32_Config", "setup123");
    
    // Connect to internet
    wifi.connect("HomeWiFi", "password");

Station Mode Operations
************************

Network Scanning
================

Scan for available WiFi networks:

.. code-block:: cpp

    static void scan_result_callback(wifi_scan_result* result) {
        LOG_INF("SSID: %s, RSSI: %d, Channel: %d",
                result->ssid, result->rssi, result->channel);
    }
    
    WiFiService& wifi = WiFiService::getInstance();
    wifi.setScanResultCallback(scan_result_callback);
    wifi.scan(scan_result_callback);

Connecting to Network
=====================

.. code-block:: cpp

    static void connection_callback(bool connected) {
        if (connected) {
            LOG_INF("WiFi connected");
            // Get IP address, start services, etc.
        } else {
            LOG_WRN("WiFi disconnected");
            // Attempt reconnection
        }
    }
    
    WiFiService& wifi = WiFiService::getInstance();
    wifi.setConnectionCallback(connection_callback);
    
    int ret = wifi.connect("MySSID", "MyPassword");
    if (ret != 0) {
        LOG_ERR("Connection failed: %d", ret);
    }

Disconnecting
=============

.. code-block:: cpp

    WiFiService& wifi = WiFiService::getInstance();
    wifi.disconnect();

Access Point Operations
***********************

Starting AP
===========

.. code-block:: cpp

    WiFiService& wifi = WiFiService::getInstance();
    
    // Start AP with custom SSID and password
    int ret = wifi.startAP("ESP32_IoT_Device", "SecurePass123");
    if (ret == 0) {
        LOG_INF("AP started successfully");
        // AP is now active on channel 6
    }

Default AP Configuration
========================

.. code-block:: cpp

    static constexpr const char* DEFAULT_SSID = "ESP32_SmartHome_AP";
    static constexpr const char* DEFAULT_PASSWORD = "12345678";
    static constexpr uint8_t DEFAULT_CHANNEL = 6;

AP Parameters:

* **Security**: WPA2-PSK (if password provided), Open (no password)
* **Channel**: 6 (2.4GHz)
* **Band**: 2.4 GHz
* **Max Clients**: Depends on CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM

Stopping AP
===========

.. code-block:: cpp

    WiFiService& wifi = WiFiService::getInstance();
    wifi.stopAP();

Network Management
******************

Event Handling
==============

The WiFi service handles the following network events:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Event
     - Description
   * - NET_EVENT_WIFI_SCAN_RESULT
     - WiFi scan found a network
   * - NET_EVENT_WIFI_SCAN_DONE
     - WiFi scan completed
   * - NET_EVENT_WIFI_CONNECT_RESULT
     - Connection attempt completed
   * - NET_EVENT_WIFI_DISCONNECT_RESULT
     - Disconnection occurred
   * - NET_EVENT_IPV4_ADDR_ADD
     - IP address assigned
   * - NET_EVENT_IPV4_ADDR_DEL
     - IP address removed

DHCP Client
===========

Automatic IP address assignment is enabled in STA mode:

.. code-block:: kconfig

    CONFIG_ESP32_WIFI_STA_AUTO_DHCPV4=y
    CONFIG_NET_DHCPV4=y

API Reference
*************

Initialization
==============

.. cpp:function:: int WiFiService::init(Mode mode)

   Initialize WiFi service with specified operation mode.
   
   :param mode: Operation mode (STA, AP, or AP_STA)
   :return: 0 on success, negative error code on failure

.. cpp:function:: int WiFiService::start()

   Start WiFi operation based on configured mode.
   
   :return: 0 on success, negative error code on failure

.. cpp:function:: int WiFiService::stop()

   Stop WiFi operation and disconnect.
   
   :return: 0 on success, negative error code on failure

Station Mode
============

.. cpp:function:: int WiFiService::connect(const char* ssid, const char* password)

   Connect to a WiFi network.
   
   :param ssid: Network SSID (max 32 bytes)
   :param password: Network password (max 64 bytes)
   :return: 0 on success, negative error code on failure
   :retval -EINVAL: Invalid SSID or password
   :retval -ENODEV: No network interface
   :retval -EALREADY: Already connected

.. cpp:function:: int WiFiService::disconnect()

   Disconnect from current WiFi network.
   
   :return: 0 on success, negative error code on failure

.. cpp:function:: int WiFiService::scan(ScanResultCallback callback)

   Scan for available WiFi networks.
   
   :param callback: Function called for each network found
   :return: 0 on success, negative error code on failure

Access Point Mode
=================

.. cpp:function:: int WiFiService::startAP(const char* ssid, const char* password)

   Start WiFi Access Point.
   
   :param ssid: AP SSID (max 32 bytes)
   :param password: AP password (min 8 bytes for WPA2, empty for open)
   :return: 0 on success, negative error code on failure
   :retval -ENOTSUP: AP mode not supported in configuration
   :retval -ENODEV: No network interface

.. cpp:function:: int WiFiService::stopAP()

   Stop WiFi Access Point.
   
   :return: 0 on success, negative error code on failure

Status Query
============

.. cpp:function:: bool WiFiService::isConnected() const

   Check if connected to a WiFi network (STA mode).
   
   :return: true if connected, false otherwise

.. cpp:function:: Mode WiFiService::getMode() const

   Get current operation mode.
   
   :return: Current WiFi mode (STA, AP, or AP_STA)

Configuration
*************

Required Kconfig
================

Basic WiFi configuration in ``prj.conf``:

.. code-block:: kconfig

    # WiFi driver
    CONFIG_WIFI=y
    CONFIG_WIFI_ESP32=y
    CONFIG_ESP32_WIFI_STA_AUTO_DHCPV4=y
    
    # Network management
    CONFIG_NET_L2_WIFI_MGMT=y
    CONFIG_NET_MGMT=y
    CONFIG_NET_MGMT_EVENT=y
    CONFIG_NET_MGMT_EVENT_INFO=y
    CONFIG_NET_MGMT_EVENT_STACK_SIZE=2048
    
    # AP+STA mode support
    CONFIG_WIFI_NM=y
    CONFIG_ESP32_WIFI_AP_STA_MODE=y
    
    # Networking
    CONFIG_NETWORKING=y
    CONFIG_NET_IPV4=y
    CONFIG_NET_DHCPV4=y
    CONFIG_NET_SOCKETS=y
    CONFIG_NET_CONNECTION_MANAGER=y
    
    # Network buffers
    CONFIG_NET_PKT_RX_COUNT=10
    CONFIG_NET_PKT_TX_COUNT=10
    CONFIG_NET_BUF_RX_COUNT=10
    CONFIG_NET_BUF_TX_COUNT=10

Device Tree
===========

For AP+STA mode, a second WiFi device node is required:

**File**: ``boards/esp32_devkitc.overlay``

.. code-block:: devicetree

    / {
        wifi_ap: wifi_ap {
            compatible = "espressif,esp32-wifi";
            status = "okay";
        };
    };

Usage Examples
**************

Station Mode Example
====================

.. code-block:: cpp

    #include "modules/wifi/wifiservice.hpp"
    
    static void wifi_connected(bool connected) {
        if (connected) {
            LOG_INF("Connected to WiFi");
            // Start cloud services
        } else {
            LOG_WRN("WiFi disconnected");
            // Reconnect logic
        }
    }
    
    void wifi_task_entry(void) {
        WiFiService& wifi = WiFiService::getInstance();
        
        // Initialize in STA mode
        wifi.init(WiFiService::Mode::STA);
        wifi.setConnectionCallback(wifi_connected);
        
        // Connect to network
        wifi.connect("HomeNetwork", "password123");
        
        while (1) {
            k_sleep(K_FOREVER);
        }
    }

AP Mode Example
===============

.. code-block:: cpp

    void setup_ap_mode(void) {
        WiFiService& wifi = WiFiService::getInstance();
        
        // Initialize in AP mode
        wifi.init(WiFiService::Mode::AP);
        
        // Start AP for device configuration
        int ret = wifi.startAP("ESP32_Setup", "Config123");
        if (ret == 0) {
            LOG_INF("Configuration AP started");
            LOG_INF("Connect to 'ESP32_Setup' with password 'Config123'");
            
            // Start web server for configuration
            // start_config_server();
        }
    }

AP+STA Mode Example
===================

.. code-block:: cpp

    void dual_mode_setup(void) {
        WiFiService& wifi = WiFiService::getInstance();
        
        // Initialize in AP+STA mode
        wifi.init(WiFiService::Mode::AP_STA);
        
        // Start AP for local access
        wifi.startAP("ESP32_Local", "local123");
        
        // Connect to internet
        wifi.connect("InternetRouter", "router_pass");
        
        // Now device is accessible locally via AP
        // and has internet via STA connection
    }

Network Scanning Example
=========================

.. code-block:: cpp

    static void handle_scan_result(wifi_scan_result* result) {
        printf("Found: %-32s | RSSI: %4d | Ch: %2d | Sec: %d\n",
               result->ssid,
               result->rssi,
               result->channel,
               result->security);
    }
    
    void scan_networks(void) {
        WiFiService& wifi = WiFiService::getInstance();
        wifi.init(WiFiService::Mode::STA);
        
        LOG_INF("Scanning for WiFi networks...");
        wifi.scan(handle_scan_result);
    }

Troubleshooting
***************

Connection Issues
=================

**Cannot connect to WiFi**:

* Verify SSID and password are correct
* Check WiFi signal strength (RSSI)
* Ensure security type matches (WPA2/WPA3)
* Check if MAC address is filtered
* Verify channel is supported (1-13 for 2.4GHz)

**AP not visible**:

* Ensure AP mode is enabled in Kconfig
* Check devicetree overlay is applied
* Verify channel 6 is not congested
* Check if other devices can create APs

**DHCP timeout**:

* Increase DHCP timeout: ``CONFIG_NET_DHCPV4_INITIAL_DELAY_MAX``
* Check router DHCP server is working
* Verify IP pool is not exhausted

Performance Issues
==================

**Slow throughput**:

* Check WiFi signal strength
* Reduce distance to router
* Change WiFi channel (less congestion)
* Increase buffer counts in Kconfig
* Use 5GHz if available (ESP32-S3/C3)

**Frequent disconnections**:

* Improve signal strength
* Increase connection timeout
* Enable WiFi power saving mode
* Check for interference sources

Memory Issues
=============

**Stack overflow in WiFi task**:

* Increase ``CONFIG_NET_MGMT_EVENT_STACK_SIZE``
* Reduce buffer counts if memory constrained
* Check for memory leaks

Performance Optimization
************************

Throughput
==========

To maximize WiFi throughput:

1. **Increase Buffers**:

   .. code-block:: kconfig

       CONFIG_NET_PKT_RX_COUNT=20
       CONFIG_NET_PKT_TX_COUNT=20
       CONFIG_NET_BUF_RX_COUNT=20
       CONFIG_NET_BUF_TX_COUNT=20

2. **Use TCP Window Scaling**
3. **Disable WiFi power saving in high-performance mode**
4. **Use QoS priorities for time-sensitive data**

Power Saving
============

To minimize power consumption:

1. **Enable WiFi Power Saving**:

   .. code-block:: kconfig

       CONFIG_ESP32_WIFI_STA_PS_ENABLED=y

2. **Use longer beacon intervals**
3. **Implement sleep modes between transmissions**
4. **Use AP mode only when needed**

Source Code Reference
*********************

* ``app/src/modules/wifi/wifiservice.hpp`` - WiFi service interface
* ``app/src/modules/wifi/wifiservice.cpp`` - WiFi service implementation  
* ``app/src/thread/wifi_task.cpp`` - WiFi task orchestration
* ``app/boards/esp32_devkitc.overlay`` - Device tree overlay for AP mode

See Also
********

* :ref:`architecture_overview`
* :ref:`inter_thread_communication`
* :ref:`ble_service`
