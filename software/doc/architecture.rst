.. _architecture_overview:

Architecture Overview
#####################

Introduction
************

The ESP32 Smart Home application is built using a **C++ Object-Oriented Programming (OOP)** architecture on top of the Zephyr RTOS. The design follows the **Singleton pattern** for hardware modules and uses **task-based concurrency** for parallel operations.

System Architecture
*******************

The application is structured in three main layers:

.. code-block:: text

    ┌─────────────────────────────────────┐
    │     Application Layer (main.cpp)    │
    │  - Initialization                   │
    │  - Task orchestration               │
    └─────────────────┬───────────────────┘
                      │
    ┌─────────────────┴───────────────────┐
    │      Module Layer (C++ Singletons)  │
    │  - BleService                       │
    │  - WiFiService                      │
    │  - SensorModule                     │
    │  - UartModule                       │
    │  - DisplayModule                    │
    │  - BlinkModule                      │
    │  - ButtonModule                     │
    └─────────────────┬───────────────────┘
                      │
    ┌─────────────────┴───────────────────┐
    │    Hardware Abstraction Layer       │
    │  - Zephyr Device Drivers            │
    │  - Device Tree Bindings             │
    └─────────────────────────────────────┘

Design Patterns
***************

Singleton Pattern
=================

All hardware modules use the singleton pattern to ensure only one instance manages each hardware resource:

.. code-block:: cpp

    class BleService {
    public:
        static BleService& getInstance() {
            static BleService instance;
            return instance;
        }
        
        // Delete copy constructor and assignment
        BleService(const BleService&) = delete;
        BleService& operator=(const BleService&) = delete;
        
    private:
        BleService() = default;
    };

Task-Based Concurrency
======================

Each functional area runs in its own Zephyr thread with appropriate priority:

.. list-table::
   :header-rows: 1
   :widths: 20 15 15 50

   * - Task
     - Priority
     - Stack
     - Purpose
   * - uart_task
     - 5
     - 2048
     - UART communication
   * - ble_task
     - 6
     - 2048
     - BLE advertising & notifications
   * - wifi_task
     - 7
     - 2048
     - WiFi AP/STA management
   * - sensor_task
     - 8
     - 1024
     - Sensor reading
   * - blink_task
     - 9
     - 1024
     - LED blinking
   * - display_task
     - 10
     - 2048
     - Display updates

C++ Features Used
*****************

Standard Library Support
========================

The application uses **C++17** with **Newlib** C library:

* No STL support (std::function, std::vector, etc.)
* Uses function pointers instead of std::function
* Static callbacks instead of lambdas
* Manual memory management (no smart pointers)

Example callback pattern:

.. code-block:: cpp

    // Define callback type
    using ConnectionCallback = void (*)(bool connected);
    
    // Static callback function
    static void connection_callback(bool connected) {
        // Handle connection state
    }
    
    // Register callback
    BleService::getInstance().setConnectionCallback(connection_callback);

Module Organization
*******************

Directory Structure
===================

.. code-block:: text

    app/src/
    ├── main.cpp                    # Application entry point
    ├── cpp_support.cpp            # C++ runtime support
    ├── modules/                   # Hardware abstraction modules
    │   ├── ble/
    │   │   ├── bleservice.hpp
    │   │   └── bleservice.cpp
    │   ├── wifi/
    │   │   ├── wifiservice.hpp
    │   │   └── wifiservice.cpp
    │   ├── uart/
    │   │   ├── uartmodule.hpp
    │   │   └── uartmodule.cpp
    │   ├── sensor/
    │   ├── display/
    │   ├── blink/
    │   └── button/
    └── thread/                    # Task implementations
        ├── uart_task.cpp
        ├── ble_task.cpp
        ├── wifi_task.cpp
        ├── sensor_task.cpp
        ├── blink_task.cpp
        └── display_task.cpp

Module Responsibilities
=======================

BleService
----------

:Purpose: Bluetooth Low Energy communication
:Features:
    * GAP advertising
    * GATT service management
    * Notification support
    * Connection state management

WiFiService
-----------

:Purpose: WiFi network connectivity
:Features:
    * Station mode (connect to AP)
    * Access Point mode (create hotspot)
    * AP+STA simultaneous mode
    * Network scanning
    * DHCP client

UartModule
----------

:Purpose: Serial communication
:Features:
    * Interrupt-driven reception
    * Message queue for thread communication
    * Echo functionality
    * BLE forwarding

SensorModule
------------

:Purpose: Sensor data acquisition
:Features:
    * Device tree binding
    * Periodic reading
    * Callback notification

DisplayModule
-------------

:Purpose: Display management (SSD1306 OLED)
:Features:
    * Text rendering
    * Status display
    * Character framebuffer

BlinkModule
-----------

:Purpose: LED control
:Features:
    * Configurable blink period
    * GPIO control via custom driver

ButtonModule
------------

:Purpose: Button input handling
:Features:
    * GPIO interrupt
    * Debouncing
    * Callback notification

Inter-Module Communication
**************************

The modules communicate through several mechanisms:

1. **Direct Function Calls**: Modules call each other's public methods
2. **Callbacks**: Asynchronous event notification using function pointers
3. **Message Queues**: Thread-safe data passing (see :ref:`inter_thread_communication`)
4. **Semaphores**: Resource protection and synchronization

Initialization Sequence
***********************

The application follows a strict initialization order in ``main.cpp``:

1. **Module Initialization** (``Os_Init()``):
   
   .. code-block:: cpp

       BlinkModule::getInstance().init();
       SensorModule::getInstance().init();
       BleService::getInstance().init();
       WiFiService::getInstance().init(WiFiService::Mode::AP_STA);
       DisplayModule::getInstance().init();
       ButtonModule::getInstance().init();

2. **Task Creation** (``Os_Start()``):
   
   .. code-block:: cpp

       blink_task_start();
       sensor_task_start();
       ble_task_start();
       wifi_task_start();
       display_task_start();
       uart_task_start();

3. **Main Thread Sleep**:
   
   .. code-block:: cpp

       while (1) {
           k_sleep(K_FOREVER);
       }

Build System
************

The application uses CMake with Zephyr's build system:

* **CMakeLists.txt**: Defines source files and dependencies
* **prj.conf**: Zephyr configuration options
* **Kconfig**: Custom configuration symbols
* **Device Tree Overlays**: Board-specific hardware configuration

Memory Layout
*************

ESP32 Memory Regions:

.. list-table::
   :header-rows: 1
   :widths: 25 20 20 35

   * - Region
     - Size
     - Usage
     - Notes
   * - Flash
     - 4 MB
     - 17.7%
     - Code and constants
   * - IRAM
     - 224 KB
     - 40%
     - Interrupt handlers
   * - DRAM0
     - 140 KB
     - 89%
     - Data and BSS
   * - DRAM1
     - 96 KB
     - 38%
     - Heap and stacks

See Also
********

* :ref:`inter_thread_communication`
* :ref:`ble_service`
* :ref:`wifi_service`
