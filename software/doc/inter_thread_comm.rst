.. _inter_thread_communication:

Inter-Processor Communication
##############################

Overview
********

The nRF5340 DK application uses **dual-core architecture** where the APP core and NET core communicate via **RPMsg/OpenAMP** protocol. This enables efficient separation of concerns:

* **APP Core**: High-level application logic (Matter, Thread, UI)
* **NET Core**: Low-level radio protocols (BLE, 802.15.4)

Communication Mechanisms
************************

The cores communicate using:

* **RPMsg/OpenAMP**: Zephyr's inter-processor communication framework
* **Shared Memory**: Zero-copy message passing
* **Interrupts**: Fast notification between cores
* **Message Queues**: Buffered asynchronous communication

IPC Architecture
****************

.. uml:: diagrams/ipc_communication.puml
   :align: center
   :scale: 75%
   :caption: Inter-Processor Communication Flow

Inter-Processor Communication Flow:

.. code-block:: text

    APP Core                              NET Core
    ────────────────────────────          ────────────────────────────
    │                                     │
    │ smarthome::ipc::IPCCore            │ smarthome::ipc::IPCCore
    │ ┌───────────────────┐              │ ┌───────────────────┐
    │ │ send(message)     │              │ │ receive_callback()│
    │ └────────┬──────────┘              │ └────────▲──────────┘
    │          │                          │          │
    │          ▼                          │          │
    │  ┌──────────────┐                  │  ┌──────────────┐
    │  │  serialize   │                  │  │ deserialize  │
    │  └──────┬───────┘                  │  └──────▲───────┘
    │         │                           │         │
    │         ▼                           │         │
    │  ┌────────────────────────────────────────────┴─────┐
    │  │         Shared Memory (RPMsg Buffer)             │
    │  │  ┌──────────────┐  ┌──────────────┐             │
    │  │  │ TX Queue (→) │  │ RX Queue (←) │             │
    │  │  └──────────────┘  └──────────────┘             │
    │  └────────┬────────────────────────────▲───────────┘
    │           │                            │
    │           ▼                            │
    │    [Interrupt NET]              [Interrupt APP]
    │                                        │
    └────────────────────────────────────────┘

.. note::
   PlantUML sequence diagram available in CI/CD builds at ``doc/diagrams/ipc_communication.puml``

The ``smarthome::ipc::IPCCore`` module provides a C++ wrapper around Zephyr's OpenAMP API:

.. code-block:: cpp

    namespace smarthome {
    namespace ipc {
    
    class IPCCore {
    public:
        static IPCCore& getInstance();
        
        void init();
        void send(const Message& msg);
        void registerCallback(MessageCallback callback);
        
    private:
        // OpenAMP endpoint
        struct rpmsg_endpoint endpoint_;
        
        // Message queue for RX
        K_KERNEL_STACK_MEMBER(rx_stack_, 1024);
        struct k_thread rx_thread_;
    };
    
    } // namespace ipc
    } // namespace smarthome

.. only:: builder_html

   .. note::
      See ``doc/diagrams/ipc_communication.puml`` for detailed PlantUML sequence diagram.

Message Types
*************

IPC Message Structure
======================

.. code-block:: cpp

    namespace smarthome {
    namespace ipc {
    
    enum class MessageType : uint8_t {
        RADIO_CONTROL,      // APP → NET: Radio commands
        BLE_STATUS,         // NET → APP: BLE connection state
        NETWORK_STATS,      // NET → APP: Link quality data
        POWER_MANAGEMENT,   // Bidirectional: Sleep/wake
    };
    
    struct Message {
        MessageType type;
        uint32_t timestamp;
        uint16_t length;
        uint8_t data[256];
    };
    
    } // namespace ipc
    } // namespace smarthome

Communication Patterns
**********************

APP → NET Core
==============

**Use Case**: APP core requests BLE advertising

.. code-block:: cpp

    // APP Core
    smarthome::ipc::Message msg;
    msg.type = MessageType::RADIO_CONTROL;
    // ... populate data ...
    
    smarthome::ipc::IPCCore::getInstance().send(msg);
    
    // NET Core receives and processes
    void handle_radio_control(const Message& msg) {
        smarthome::protocol::ble::BLEManager::getInstance()
            .startAdvertising();
    }

NET → APP Core
==============

**Use Case**: NET core notifies BLE connection

.. code-block:: cpp

    // NET Core
    smarthome::ipc::Message msg;
    msg.type = MessageType::BLE_STATUS;
    // ... populate connection info ...
    
    smarthome::ipc::IPCCore::getInstance().send(msg);
    
    // APP Core receives and updates UI
    void handle_ble_status(const Message& msg) {
        // Update LED status or notify Matter stack
    }

Implementation Details
**********************

Initialization
==============

Both cores must initialize IPC before communication:

**APP Core** (``app_core.cpp``):

.. code-block:: cpp

    extern "C" int main() {
        // Initialize IPC first
        smarthome::ipc::IPCCore::getInstance().init();
        
        // Register callback
        smarthome::ipc::IPCCore::getInstance()
            .registerCallback(app_ipc_callback);
        
        // ... rest of initialization ...
    }

**NET Core** (``net_core.cpp``):

.. code-block:: cpp

    namespace net {
    
    void NetCoreManager::init() {
        // Initialize IPC
        smarthome::ipc::IPCCore::getInstance().init();
        
        // Register callback
        smarthome::ipc::IPCCore::getInstance()
            .registerCallback([this](const auto& msg) {
                this->handleMessage(msg);
            });
        
        // Initialize radio subsystems
        ble_manager_.init();
        radio_manager_.init();
    }
    
    } // namespace net

Message Threading
=================

The IPC module uses Zephyr threading for message processing:

.. code-block:: cpp

    class IPCCore {
    private:
        // RX thread for processing incoming messages
        K_KERNEL_STACK_MEMBER(rx_stack_, 1024);
        struct k_thread rx_thread_;
        
        // Thread entry point
        static void rx_thread_entry(void* p1, void* p2, void* p3);
    };

The RX thread:

1. Blocks on OpenAMP receive
2. Deserializes incoming message
3. Invokes registered callback
4. Returns to blocking receive

Performance Characteristics
***************************

:Latency: < 100μs for message passing between cores
:Throughput: Up to 10,000 messages/second
:Message Size: Up to 256 bytes per message
:Queue Depth: Configurable in OpenAMP (default 16 messages)

The low latency is achieved through:

* **Shared Memory**: Zero-copy message passing
* **Hardware Interrupts**: Direct core-to-core notification
* **No Context Switching**: Dedicated IPC thread on each core

Configuration
*************

OpenAMP Configuration
=====================

Zephyr configuration for inter-processor communication:

.. code-block:: kconfig

    # Enable OpenAMP/RPMsg
    CONFIG_OPENAMP=y
    CONFIG_OPENAMP_RSC_TABLE=y
    
    # IPC configuration
    CONFIG_IPC_SERVICE=y
    CONFIG_IPC_SERVICE_BACKEND_RPMSG=y
    
    # Shared memory region
    CONFIG_OPENAMP_SHM_BASE_ADDRESS=0x20070000
    CONFIG_OPENAMP_SHM_SIZE=0x10000

Thread Stack Sizes
==================

.. code-block:: cpp

    // IPC RX thread stack
    K_KERNEL_STACK_MEMBER(rx_stack_, 1024);  // 1 KB sufficient
    
    // NET core main stack
    K_KERNEL_STACK_DEFINE(net_core_stack, 2048);  // 2 KB

Usage Examples
**************

Example 1: Request Network Statistics
======================================

APP core requests network quality data from NET core:

.. code-block:: cpp

    // APP Core - request stats
    smarthome::ipc::Message req;
    req.type = MessageType::NETWORK_STATS;
    smarthome::ipc::IPCCore::getInstance().send(req);
    
    // NET Core - respond with stats
    void handle_stats_request(const Message& req) {
        smarthome::ipc::Message resp;
        resp.type = MessageType::NETWORK_STATS;
        
        // Populate with actual stats
        int8_t rssi = get_current_rssi();
        memcpy(resp.data, &rssi, sizeof(rssi));
        
        smarthome::ipc::IPCCore::getInstance().send(resp);
    }

Example 2: BLE Connection Notification
=======================================

NET core notifies APP core of BLE connection changes:

.. code-block:: cpp

    // NET Core - BLE callback
    void ble_connected_callback(bool connected) {
        smarthome::ipc::Message msg;
        msg.type = MessageType::BLE_STATUS;
        msg.data[0] = connected ? 1 : 0;
        msg.length = 1;
        
        smarthome::ipc::IPCCore::getInstance().send(msg);
    }
    
    // APP Core - handle notification
    void app_ipc_callback(const smarthome::ipc::Message& msg) {
        if (msg.type == MessageType::BLE_STATUS) {
            bool connected = msg.data[0] != 0;
            LOG_INF("BLE connection status: %s", 
                    connected ? "connected" : "disconnected");
        }
    }

Testing
*******

Build and Flash
=========both cores:

   .. code-block:: bash

       cd software
       ./make.sh build

2. Flash to nRF5340 DK:

   .. code-block:: bash

       ./make.sh flash

3. Monitor serial output:

   .. code-block:: bash

       screen /dev/ttyACM0 115200

4. Test IPC by pressing buttons - observe messages between cores

Debugging IPC
=============

Enable IPC debug logging in ``prj.conf``:

.. code-block:: kconfig

    CONFIG_OPENAMP_LOG_LEVEL_DBG=y
    CONFIG_IPC_SERVICE_LOG_LEVEL_DBG=y

Key Zephyr APIs Used
********************

OpenAMP/RPMsg
=============

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - API
     - Description
   * - ``rpmsg_service_register_endpoint()``
     - Register IPC endpoint
   * - ``rpmsg_send()``
     - Send message to remote core
   * - ``rpmsg_service_receive()``
     - Receive message (callback)

IPC Service
===========

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - API
     - Description
   * - ``ipc_service_open_instance()``
     - Open IPC instance
   * - ``ipc_service_register_endpoint()``
     - Register endpoint
   * - ``ipc_service_send()``
     - Send data via IPC

Threading
=========

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - API
     - Description
   * - ``K_KERNEL_STACK_MEMBER()``
     - Define thread stack in class
   * - ``k_thread_create()``
     - Create new thread
   * - ``k_sleep()``
     - Yield thread execution

Source Code Reference
*********************

IPC Module Files
================

* ``app/src/sdk/ipc/ipc_core.hpp`` - IPC interface
* ``app/src/sdk/ipc/ipc_core.cpp`` - IPC implementation
* ``app/src/app_core/app_core.cpp`` - APP core usage
* ``app/src/net_core/net_core.cpp`` - NET core usage
* ``app/src/modules/uart/uartmodule.hpp`` - UART module interface
* ``app/src/modules/uart/uartmodule.cpp`` - UART module implementation

Task Files
==========

* ``app/src/thread/uart_task.h`` - UART task interface
* ``app/src/thread/uart_task.cpp`` - UART task implementation

See Also
********

* :ref:`architecture_overview`
* :ref:`ble_service`
* :ref:`wifi_service`
