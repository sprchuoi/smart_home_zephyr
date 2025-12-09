.. _inter_thread_communication:

Inter-Thread Communication
##########################

Overview
********

This application uses Zephyr RTOS primitives for efficient inter-thread communication:

* **Message Queues**: For passing data between threads
* **Semaphores**: For thread synchronization
* **Timers**: For periodic operations

UART → BLE Communication Flow
******************************

The following diagram illustrates the data flow from UART interrupt to BLE transmission:

.. code-block:: text

    ┌─────────────┐
    │ UART RX IRQ │  (Interrupt Context)
    └──────┬──────┘
           │ k_msgq_put()
           ▼
    ┌─────────────────┐
    │  Message Queue  │  (uart_msgq)
    │  [32 messages]  │
    └──────┬──────────┘
           │ k_msgq_get()
           ▼
    ┌─────────────┐
    │  UART Task  │  (Priority 5)
    │  - Echo RX  │
    │  - Buffer   │
    └──────┬──────┘
           │ ble.notify()
           ▼
    ┌─────────────┐
    │  BLE Task   │  (Priority 6)
    │  - TX data  │
    └─────────────┘

Components
**********

Message Queue (uart_msgq)
==========================

:Purpose: Transfer UART interrupt data to UART task thread
:Size: 32 messages
:Message Structure:
    .. code-block:: c

        struct uart_msg {
            uint8_t data;       // Received byte
            uint32_t timestamp; // Reception time
        };

:Location: Defined in ``uart_task.cpp``

UART Interrupt Handler
=======================

:File: ``modules/uart/uartmodule.cpp``
:Function: ``uart_irq_handler()``

Operation:
    1. UART RX interrupt fires
    2. Read bytes from FIFO (up to 32 bytes)
    3. Put each byte into message queue (non-blocking)
    4. If queue full, drop bytes and log warning

UART Task
=========

:File: ``thread/uart_task.cpp``
:Priority: 5 (medium)
:Stack Size: 2048 bytes

Operation:
    1. Block on ``k_msgq_get()`` waiting for UART data
    2. Echo received byte back to UART
    3. Accumulate bytes in buffer (128 bytes max)
    4. Send to BLE on:
        * Newline character (``\n`` or ``\r``)
        * Buffer full
        * Timer expires (100ms)

BLE Task
========

:File: ``thread/ble_task.cpp``
:Priority: 6 (higher priority)

Operation:
    * Processes BLE connection events
    * Transmits data via BLE notifications

Usage Example
*************

Sending Data via UART
======================

.. code-block:: bash

    # Connect to ESP32 UART (typically /dev/ttyUSB0)
    screen /dev/ttyUSB0 115200

    # Type characters - they will be:
    # 1. Echoed back to UART
    # 2. Forwarded to connected BLE device
    Hello World!

Receiving on BLE Client
========================

The BLE client (phone app, computer) will receive the UART data via BLE notifications.

Semaphore Synchronization
**************************

Example: Adding a semaphore to protect shared resource between BLE and WiFi tasks

.. code-block:: c

    // In a shared header (e.g., thread/sync.h)
    K_SEM_DEFINE(ble_wifi_sem, 1, 1);  // Binary semaphore, initial=1, max=1

    // In BLE task
    void ble_task_operation() {
        k_sem_take(&ble_wifi_sem, K_FOREVER);  // Lock
        
        // Critical section - access shared resource
        // ...
        
        k_sem_give(&ble_wifi_sem);  // Unlock
    }

    // In WiFi task
    void wifi_task_operation() {
        k_sem_take(&ble_wifi_sem, K_FOREVER);  // Lock
        
        // Critical section - access shared resource
        // ...
        
        k_sem_give(&ble_wifi_sem);  // Unlock
    }

Configuration
*************

UART is already enabled in ``prj.conf`` as the console device:

.. code-block:: kconfig

    CONFIG_SERIAL=y
    CONFIG_UART_INTERRUPT_DRIVEN=y

Performance Characteristics
***************************

:Latency: < 1ms from UART RX to task processing
:Throughput: Up to 115200 baud (limited by UART speed)
:Queue Depth: 32 messages prevents data loss at normal rates
:BLE Transmission: Batched every 100ms or on newline

Extending the Architecture
**************************

Adding Another Queue (Example: Sensor → WiFi)
==============================================

.. code-block:: c

    // 1. Define message structure
    struct sensor_msg {
        int value;
        uint32_t timestamp;
    };

    // 2. Create message queue
    K_MSGQ_DEFINE(sensor_wifi_msgq, sizeof(struct sensor_msg), 16, 4);

    // 3. Producer (sensor task)
    struct sensor_msg msg;
    msg.value = sensor_reading;
    msg.timestamp = k_uptime_get_32();
    k_msgq_put(&sensor_wifi_msgq, &msg, K_NO_WAIT);

    // 4. Consumer (wifi task)
    struct sensor_msg msg;
    if (k_msgq_get(&sensor_wifi_msgq, &msg, K_MSEC(100)) == 0) {
        // Process sensor data
    }

Testing
*******

Build and Flash
===============

1. Build firmware:

   .. code-block:: bash

       ./make.sh build

2. Flash to ESP32:

   .. code-block:: bash

       ./make.sh flash

3. Open serial monitor:

   .. code-block:: bash

       ./make.sh monitor

4. Type characters - observe echo and BLE transmission

5. Connect BLE client - receive forwarded UART data

Key Zephyr APIs Used
********************

Message Queues
==============

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - API
     - Description
   * - ``k_msgq_define()``
     - Define message queue
   * - ``k_msgq_put()``
     - Send message (non-blocking in ISR)
   * - ``k_msgq_get()``
     - Receive message (blocking)

Semaphores
==========

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - API
     - Description
   * - ``k_sem_define()``
     - Define semaphore
   * - ``k_sem_take()``
     - Acquire semaphore
   * - ``k_sem_give()``
     - Release semaphore

UART
====

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - API
     - Description
   * - ``uart_irq_callback_user_data_set()``
     - Set UART ISR with user data
   * - ``uart_irq_rx_enable()``
     - Enable RX interrupt
   * - ``uart_fifo_read()``
     - Read from UART FIFO
   * - ``uart_poll_out()``
     - Polled output (blocking)

Source Code Reference
*********************

Module Files
============

* ``app/src/modules/uart/uartmodule.hpp`` - UART module interface
* ``app/src/modules/uart/uartmodule.cpp`` - UART module implementation

Task Files
==========

* ``app/src/thread/uart_task.h`` - UART task interface
* ``app/src/thread/uart_task.cpp`` - UART task implementation

See Also
********

* :ref:`architecture_overview`
* :ref:`task_management`
* :ref:`ble_service`
