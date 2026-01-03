.. _architecture_overview:

Architecture Overview
#####################

Introduction
************

The nRF5340 DK Matter Smart Light is built using a **dual-core architecture** with **C++ Object-Oriented Programming (OOP)** on top of the Zephyr RTOS. The system leverages the nRF5340's dual ARM Cortex-M33 processors to separate application logic from radio protocol handling, following **namespace-based modularization** and **inter-processor communication** patterns.

Dual-Core System Architecture
******************************

The application utilizes both cores of the nRF5340:

* **APP Core (Cortex-M33 @ 128 MHz)**: Matter protocol, Thread networking, LED/button control
* **NET Core (Cortex-M33 @ 64 MHz)**: BLE radio, 802.15.4 radio, OpenThread MAC/PHY

Architecture Diagram
====================

.. uml:: diagrams/dual_core_architecture.puml
   :align: center
   :width: 600px
   :caption: nRF5340 Dual-Core System Architecture

Hardware Resources
==================

.. list-table::
   :header-rows: 1
   :widths: 20 25 25 30

   * - Core
     - Flash Usage
     - RAM Usage
     - Primary Responsibilities
   * - APP Core
     - 75 KB (7.16%)
     - 34 KB (7.40%)
     - Matter, Thread, UI
   * - NET Core
     - 142 KB (54.26%)
     - 31 KB (47.25%)
     - BLE, 802.15.4 radio

Design Patterns
***************

Namespace-Based Modularization
===============================

All SDK modules use hierarchical C++ namespaces for clear organization:

.. code-block:: cpp

    namespace smarthome {
        namespace protocol {
            namespace matter {
                class AppTask { /* ... */ };
                class LightEndpoint { /* ... */ };
            }
            
            namespace thread {
                class ThreadNetworkManager { /* ... */ };
            }
            
            namespace ble {
                class BLEManager { /* ... */ };
            }
        }
        
        namespace hw {
            class ButtonManager { /* ... */ };
            class UartManager { /* ... */ };
        }
        
        namespace ipc {
            class IPCCore { /* ... */ };
        }
    }
    
    // NET core uses separate namespace
    namespace net {
        class NetCoreManager { /* ... */ };
    }

Namespace Hierarchy
===================

.. uml:: diagrams/namespace_hierarchy.puml
   :align: center
   :width: 600px
   :caption: C++ Namespace Structure

.. code-block:: text

    smarthome::
    ├── hw::
    │   ├── ButtonManager
    │   └── UartManager
    │
    ├── protocol::
    │   ├── matter::
    │   │   ├── AppTask
    │   │   ├── LightEndpoint
    │   │   └── CommissioningDelegate
    │   │
    │   ├── thread::
    │   │   ├── ThreadNetworkManager
    │   │   └── NetworkResilienceManager
    │   │
    │   ├── ble::
    │   │   └── BLEManager
    │   │
    │   └── radio::
    │       └── RadioManager
    │
    ├── ipc::
    │   ├── IPCCore
    │   └── MessageType (enum)
    │
    └── services::
        └── wakeword::
            └── ModelLoader
    
    net::                    (NET core only)
    └── NetCoreManager

.. note::
   PlantUML diagram available in CI/CD builds at ``doc/diagrams/namespace_hierarchy.puml``

Function Pointer Callbacks
===========================

The application uses function pointers instead of ``std::function`` due to Zephyr's minimal C++ support:

.. code-block:: cpp

    // Define callback type
    using ButtonCallback = void (*)(bool pressed);
    
    // Static callback function
    static void button_callback(bool pressed) {
        if (pressed) {
            // Handle button press
        }
    }
    
    // Register callback
    ButtonManager::getInstance().setCallback(button_callback);

Inter-Processor Communication
==============================

The two cores communicate via **RPMsg/OpenAMP** protocol.

.. uml:: diagrams/ipc_communication.puml
   :align: center
   :width: 600px
   :caption: Inter-Processor Communication Sequence

Hardware interrupts for low latency
* Asynchronous bidirectional communication
* Message buffering in OpenAMP

C++ Features Used
*****************

Minimal C++ Standard Library
=============================

The application uses **C++17** with **minimal standard library support**:

* No STL containers (std::vector, std::map, etc.)
* No ``std::function`` (use function pointers)
* No exceptions (``-fno-exceptions``)
* No RTTI (``-fno-rtti``)
* Use C headers (``<string.h>`` not ``<cstring>``)
* Manual memory management

Example patterns:

.. code-block:: cpp

    // Use function pointers, not std::function
    using Callback = void (*)(int value);
    
    // Use C headers
    #include <string.h>  // NOT <cstring>
    #include <zephyr/kernel.h>
    
    // Use Zephyr kernel types
    K_KERNEL_STACK_MEMBER(stack, 1024);  // NOT K_THREAD_STACK_MEMBER
    
    // Include kernel.h BEFORE namespace declarations
    // to avoid scoping issues with k_timer, k_mutex, etc.

Directory Structure
~~~~~~~~~~~~~~~~~~~

.. uml:: diagrams/directory_structure.puml
   :align: center
   :width: 600px
   :caption: Source Code Organization

.. code-block:: text

      src/
      ├── app_core/                     # APP Core entry point
      │   ├── app_core.cpp              # Main application logic
      │   └── cpp_support.cpp           # C++ runtime support
      │
      ├── net_core/                     # NET Core entry point
      │   ├── net_core.hpp              # NET core manager
      │   └── net_core.cpp              # Network processor state machine
      │
      ├── app_core/                     # APP Core entry point
    │   ├── app_core.cpp              # Main application logic
      └── sdk/                          # Shared SDK library (both cores)
          ├── protocol/                 # Protocol implementations
          │   ├── matter/               # Matter (CHIP) protocol [APP]
          │   │   ├── app_task.cpp
          │   │   ├── light_endpoint.cpp
          │   │   └── commissioning_delegate.cpp
          │   │
          │   ├── thread/               # Thread networking [APP]
    └── sdk/                          # Shared SDK library (both cores)
        ├── protocol/                 # Protocol implementations
        │   ├── matter/               # Matter (CHIP) protocol [APP]
        │   │   ├── app_task.cpp
        │   │   ├── light_endpoint.cpp
        │   │   └── commissioning_delegate.cpp
        │   │
        │   ├── thread/               # Thread networking [APP]
        │   │   ├── thread_network_manager.cpp
        │   │   └── network_resilience_manager.cpp
        │   │
        │   ├── ble/                  # Bluetooth Low Energy [NET]
        │   │   └── ble_manager.cpp
        │   │
        │   └── radio/                # IEEE 802.15.4 radio [NET]
        │       └── radio_manager.cpp
        │
        ├── hw/                       # Hardware abstraction [APP]
        │   ├── button/
        │   │   └── button_manager.cpp
        │   └── uart/
        │       └── uart_manager.cpp
        │
        ├── ipc/                      # Inter-processor comm [BOTH]
        │   └── ipc_core.cpp
        │
        └── services/                 # Higher-level services
            └── wakeword/             # ML model loading
                └── model_loader.cpp

Legend:

* ``[APP]`` - Used only by APP core
* ``[NET]`` - Used only by NET core
* ``[BOTH]`` - Shared by both cores

.. note::
   PlantUML diagram available in CI/CD builds at ``doc/diagrams/directory_structure.puml``ucture.puml`` for PlantUML diagram with color coding.

Module Responsibilities
=======================

APP Core Modules
~~~~~~~~~~~~~~~~

**AppTask** (``sdk/protocol/matter/``)
   Matter application state machine and event handling

**LightEndpoint** (``sdk/protocol/matter/``)
   Matter endpoint for LED control (on/off, brightness)

**ThreadNetworkManager** (``sdk/protocol/thread/``)
   Thread network initialization and management

**NetworkResilienceManager** (``sdk/protocol/thread/``)
   Network health monitoring and recovery

**ButtonManager** (``sdk/hw/button/``)
   GPIO button input with debouncing

**UartManager** (``sdk/hw/uart/``)
   Serial communication interface

NET Core Modules
~~~~~~~~~~~~~~~~

**NetCoreManager** (``net_core/``)
   Network processor state machine and coordinator

**BLEManager** (``sdk/protocol/ble/``)
   Bluetooth Low Energy advertising and connections

**RadioManager** (``sdk/protocol/radio/``)
   IEEE 802.15.4 radio control and MAC layer

Shared Modules
~~~~~~~~~~~~~~

**IPCCore** (``sdk/ipc/``)
   Inter-processor communication using RPMsg/OpenAMP
   Used by both APP and NET cores

**ModelLoader** (``sdk/services/wakeword/``)
   Machine learning model loading service

Inter-Core Communication
************************

The APP and NET cores communicate via the **IPC Core** module using Zephyr's OpenAMP/RPMsg:

Message Types
=============

* **Radio Control**: Commands from APP to NET for radio configuration
* **BLE Status**: Connection state updates from NET to APP
* **Network Statistics**: Link quality and performance data
* **Power Management**: Sleep/wake coordination

Communication Flow
==================

1. APP core calls ``IPCCore::send(message)``
2. Message serialized and placed in shared memory
3. NET core interrupted via IPC mechanism
4. NET core ``IPCCore::receive()`` callback invoked
5. Message deserialized and processed

See :ref:`inter_thread_communication` for detailed IPC documentation.
    * Callback notification

Inter-Module Communication
**************************

The modules communicate through several mechanisms:

1. **Direct Function Calls**: Modules call each other's public methods
2. **Callbacks**: Asynchronous event notification using function pointers
3. **Message Queues**: Thread-safe data passing (see :ref:`inter_thread_communication`)
4. **Semaphores**: Resource protection and synchronization

Initialization Sequence
APP Core Initialization
========================

The APP core follows this initialization sequence in ``app_core.cpp``:

.. code-block:: cpp

    extern "C" int main() {
        // 1. Initialize hardware managers
        smarthome::hw::ButtonManager::getInstance().init();
        smarthome::hw::UartManager::getInstance().init();
        
        // 2. Initialize IPC for inter-core communication
        smarthome::ipc::IPCCore::getInstance().init();
        
        // 3. Initialize Matter protocol stack
        smarthome::protocol::matter::AppTask::getInstance().init();
        smarthome::protocol::matter::LightEndpoint::getInstance().init();
        
        // 4. Initialize Thread networking
        smarthome::protocol::thread::ThreadNetworkManager::getInstance().init();
        
        // 5. Start Matter application
        smarthome::protocol::matter::AppTask::getInstance().startApp();
        
        // 6. Main thread sleep
        while (1) {
            k_sleep(K_FOREVER);
        }
    }

NET Core Initialization
========================

The NET core follows this initialization sequence in ``net_core.cpp``:

.. code-block:: cpp

    extern "C" int main() {
        // 1. Create NET core manager
        net::NetCoreManager net_core;
        
        // 2. Initialize (sets up BLE, Radio, IPC)
        net_core.init();
        
        // 3. Run state machine
        net_core.run();  // Never returns
    }

The NET core manager internally initializes:

1. ``smarthome::ipc::IPCCore`` - Inter-core communication
2. ``smarthome::protocol::ble::BLEManager`` - Bluetooth radio
3. ``smarthome::protocol::radio::RadioManager`` - 802.15.4 radio

Build System
************

The application uses CMake with Zephyr's build system:

* **CMakeLists.txt**: Defines source files for both cores
* **prj_nrf5340_app.conf**: APP core configuration
* **prj_nrf5340_net.conf**: NET core configuration (requires ``CONFIG_STATIC_INIT_GNU=y`` for C++)
* **Kconfig**: Custom configuration symbols
* **Device Tree Overlays**: Board-specific hardware configuration

Build Configuration
===================

.. code-block:: cmake

    # APP Core sources
    if(CONFIG_SOC_NRF5340_CPUAPP)
        target_sources(app PRIVATE
            src/app_core/app_core.cpp
            src/sdk/protocol/matter/app_task.cpp
            src/sdk/protocol/thread/thread_network_manager.cpp
            src/sdk/hw/button/button_manager.cpp
            src/sdk/ipc/ipc_core.cpp
        )
    endif()
    
    # NET Core sources
    if(CONFIG_SOC_NRF5340_CPUNET)
        target_sources(app PRIVATE
            src/net_core/net_core.cpp
            src/sdk/protocol/ble/ble_manager.cpp
            src/sdk/protocol/radio/radio_manager.cpp
            src/sdk/ipc/ipc_core.cpp
        )
    endif()

Memory Layout
*************

nRF5340 Memory Regions:

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 40

   * - Core
     - Region
     - Size
     - Usage
   * - APP
     - Flash
     - 1 MB
     - 7.16% (75 KB)
   * - APP
     - RAM
     - 448 KB
     - 7.40% (34 KB)
   * - NET
     - Flash
     - 256 KB
     - 54.26% (142 KB)
   * - NET
     - RAM
     - 64 KB
     - 47.25% (31 KB)

The NET core uses more flash due to the OpenThread and BLE protocol stacks.

See Also
********

* :ref:`inter_thread_communication` - IPC details
* :ref:`ble_service` - BLE implementation  
* :ref:`wifi_service` - WiFi/Thread networking
   * - DRAM1
     - 96 KB
     - 38%
     - Heap and stacks

See Also
********

* :ref:`inter_thread_communication`
* :ref:`ble_service`
* :ref:`wifi_service`
