.. _ble_service:

BLE Service
###########

Overview
********

The BLE (Bluetooth Low Energy) Service provides a complete implementation of Bluetooth communication using the Zephyr Bluetooth stack. It follows the singleton pattern and provides a high-level interface for BLE advertising, GATT services, and data notifications.

Architecture
************

Class Design
============

.. code-block:: cpp

    class BleService {
    public:
        static BleService& getInstance();
        
        int init();
        int start();
        int stop();
        
        bool isConnected() const;
        bool isNotifyEnabled() const;
        
        int notify(const uint8_t* data, uint16_t len);
        int notify(const char* message);
        
        void setConnectionCallback(ConnectionCallback cb);
        void setDataReceivedCallback(DataReceivedCallback cb);
        
    private:
        BleService();
        ~BleService();
    };

Callback Types
==============

The BLE service uses function pointers for asynchronous event notification:

.. code-block:: cpp

    // Connection state callback
    using ConnectionCallback = void (*)(bool connected);
    
    // Data received callback
    using DataReceivedCallback = void (*)(const uint8_t* data, uint16_t len);

GATT Service Structure
**********************

Custom GATT Service
===================

The BLE service implements a custom GATT service with the following characteristics:

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Characteristic
     - UUID
     - Properties
   * - Data TX
     - Custom UUID
     - Notify, Read
   * - Data RX
     - Custom UUID
     - Write, Write Without Response

Service UUID
------------

The service uses a custom 128-bit UUID for identification.

Advertising
***********

Advertising Configuration
=========================

.. code-block:: cpp

    // Advertising parameters
    static constexpr uint16_t ADV_INTERVAL_MIN = 0x0020;  // 20ms
    static constexpr uint16_t ADV_INTERVAL_MAX = 0x0040;  // 40ms
    
    // Device name
    CONFIG_BT_DEVICE_NAME = "ESP32 Smart Home"

Advertising Data
================

The advertising packet includes:

* **Flags**: General discoverable, BR/EDR not supported
* **Device Name**: "ESP32 Smart Home"
* **Service UUID**: Custom GATT service UUID

Connection Management
*********************

Connection Parameters
=====================

.. code-block:: cpp

    // Connection parameters
    static constexpr uint16_t CONN_INTERVAL_MIN = 0x0018;  // 30ms
    static constexpr uint16_t CONN_INTERVAL_MAX = 0x0028;  // 50ms
    static constexpr uint16_t CONN_LATENCY = 0;
    static constexpr uint16_t CONN_TIMEOUT = 400;          // 4s

Connection Callbacks
====================

Register a callback to receive connection state changes:

.. code-block:: cpp

    static void connection_callback(bool connected) {
        if (connected) {
            LOG_INF("BLE device connected");
        } else {
            LOG_INF("BLE device disconnected");
        }
    }
    
    BleService::getInstance().setConnectionCallback(connection_callback);

Data Communication
******************

Sending Data (Notifications)
=============================

Send data to the connected BLE client:

.. code-block:: cpp

    BleService& ble = BleService::getInstance();
    
    // Send raw bytes
    uint8_t data[] = {0x01, 0x02, 0x03};
    ble.notify(data, sizeof(data));
    
    // Send string
    ble.notify("Hello BLE!");

Receiving Data
==============

Register a callback to receive data from the BLE client:

.. code-block:: cpp

    static void data_received_callback(const uint8_t* data, uint16_t len) {
        LOG_INF("Received %d bytes", len);
        // Process received data
    }
    
    BleService::getInstance().setDataReceivedCallback(data_received_callback);

API Reference
*************

Initialization
==============

.. cpp:function:: int BleService::init()

   Initialize the BLE service and Bluetooth stack.
   
   :return: 0 on success, negative error code on failure
   :retval -EALREADY: BLE already initialized
   :retval -EIO: Bluetooth enable failed

.. cpp:function:: int BleService::start()

   Start BLE advertising.
   
   :return: 0 on success, negative error code on failure
   :retval -EINVAL: Invalid advertising parameters
   :retval -EALREADY: Already advertising

.. cpp:function:: int BleService::stop()

   Stop BLE advertising.
   
   :return: 0 on success, negative error code on failure

Status Query
============

.. cpp:function:: bool BleService::isConnected() const

   Check if a BLE client is connected.
   
   :return: true if connected, false otherwise

.. cpp:function:: bool BleService::isNotifyEnabled() const

   Check if notifications are enabled by the client.
   
   :return: true if notifications enabled, false otherwise

Data Transmission
=================

.. cpp:function:: int BleService::notify(const uint8_t* data, uint16_t len)

   Send data to the connected BLE client via notification.
   
   :param data: Pointer to data buffer
   :param len: Length of data in bytes
   :return: 0 on success, negative error code on failure
   :retval -ENOTCONN: No device connected
   :retval -EINVAL: Invalid parameters

.. cpp:function:: int BleService::notify(const char* message)

   Send a text message to the connected BLE client.
   
   :param message: Null-terminated string
   :return: 0 on success, negative error code on failure

Callbacks
=========

.. cpp:function:: void BleService::setConnectionCallback(ConnectionCallback cb)

   Register a callback for connection state changes.
   
   :param cb: Function pointer to callback

.. cpp:function:: void BleService::setDataReceivedCallback(DataReceivedCallback cb)

   Register a callback for received data.
   
   :param cb: Function pointer to callback

Configuration
*************

Kconfig Options
===============

Required Kconfig symbols in ``prj.conf``:

.. code-block:: kconfig

    # Bluetooth configuration
    CONFIG_BT=y
    CONFIG_BT_PERIPHERAL=y
    CONFIG_BT_DEVICE_NAME="ESP32 Smart Home"
    CONFIG_BT_DEVICE_APPEARANCE=0
    CONFIG_BT_MAX_CONN=1
    CONFIG_BT_MAX_PAIRED=1
    
    # GATT configuration
    CONFIG_BT_GATT_DYNAMIC_DB=y
    CONFIG_BT_ATT_PREPARE_COUNT=2
    
    # Stack sizes
    CONFIG_MAIN_STACK_SIZE=2048
    CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048

Usage Example
*************

Complete Example
================

.. code-block:: cpp

    #include "modules/ble/bleservice.hpp"
    
    // Connection callback
    static void on_ble_connected(bool connected) {
        if (connected) {
            LOG_INF("BLE connected");
        } else {
            LOG_INF("BLE disconnected");
        }   
    }
    
    // Data received callback
    static void on_ble_data(const uint8_t* data, uint16_t len) {
        LOG_INF("Received %d bytes", len);
        // Echo data back
        BleService::getInstance().notify(data, len);
    }
    
    void ble_task_entry(void *p1, void *p2, void *p3) {
        BleService& ble = BleService::getInstance();
        
        // Register callbacks
        ble.setConnectionCallback(on_ble_connected);
        ble.setDataReceivedCallback(on_ble_data);
        
        // Start advertising
        int ret = ble.start();
        if (ret < 0) {
            LOG_ERR("Failed to start BLE: %d", ret);
            return;
        }
        
        // Periodic data transmission
        while (1) {
            k_sleep(K_SECONDS(5));
            
            if (ble.isConnected()) {
                char msg[] = "Heartbeat";
                ble.notify(msg);
            }
        }
    }

Testing
*******

Mobile App Testing
==================

1. **Install a BLE Scanner App**:
   
   * Android: nRF Connect, BLE Scanner
   * iOS: LightBlue, nRF Connect

2. **Scan for Device**:
   
   * Look for "ESP32 Smart Home"
   * Connect to the device

3. **Enable Notifications**:
   
   * Find the TX characteristic
   * Enable notifications

4. **Send Data**:
   
   * Find the RX characteristic
   * Write data to test receiving

Command Line Testing
====================

Using ``bluetoothctl`` on Linux:

.. code-block:: bash

    # Start bluetoothctl
    bluetoothctl
    
    # Scan for devices
    scan on
    
    # Connect (replace XX:XX:XX:XX:XX:XX with device address)
    connect XX:XX:XX:XX:XX:XX
    
    # List services
    menu gatt
    list-attributes

Troubleshooting
***************

Common Issues
=============

Device Not Advertising
-----------------------

**Symptoms**: Device not visible in BLE scanner

**Solutions**:

* Check if Bluetooth is enabled: ``CONFIG_BT=y``
* Verify advertising started successfully
* Check system logs for errors
* Ensure no other BLE connections are active

Connection Fails
----------------

**Symptoms**: Connection attempt fails immediately

**Solutions**:

* Check connection parameters
* Verify GATT database is properly initialized
* Ensure stack sizes are sufficient
* Check for memory issues

Notifications Not Working
--------------------------

**Symptoms**: Data sent but not received by client

**Solutions**:

* Verify client enabled notifications (CCCD)
* Check ``isNotifyEnabled()`` returns true
* Ensure MTU is sufficient for data size
* Verify connection is active

Performance Optimization
************************

Throughput Optimization
=======================

To maximize BLE throughput:

1. **Increase MTU**: Negotiate larger MTU size
2. **Connection Interval**: Use shorter intervals (20-50ms)
3. **Queue Data**: Buffer data to reduce overhead
4. **Use Write Without Response**: For unidirectional data

Power Optimization
==================

To minimize power consumption:

1. **Increase Advertising Interval**: 500ms - 2000ms
2. **Use Longer Connection Intervals**: 100ms - 500ms
3. **Enable Connection Latency**: Allow peripheral to skip events
4. **Reduce Notification Frequency**: Batch data transmissions

Source Code Reference
*********************

* ``app/src/modules/ble/bleservice.hpp`` - BLE service interface
* ``app/src/modules/ble/bleservice.cpp`` - BLE service implementation
* ``app/src/thread/ble_task.cpp`` - BLE task orchestration

See Also
********

* :ref:`architecture_overview`
* :ref:`inter_thread_communication`
* :ref:`wifi_service`
