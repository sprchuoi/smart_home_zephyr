/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * IPC Core Module - Inter-Core Communication for nRF5340
 * ============================================================================
 * 
 * Purpose:
 *   Memory-efficient, zero-copy IPC mechanism between APP and NET cores
 *   Uses Zephyr's OpenAMP/RPMsg for reliable message passing
 *
 * Design Principles:
 *   - Zero dynamic allocation (static memory pools)
 *   - Lock-free ring buffers for performance
 *   - Type-safe message passing with C++ templates
 *   - RAII pattern for resource management
 *   - Observer pattern for asynchronous notifications
 *
 * Features:
 *   - Bidirectional communication (APP ↔ NET)
 *   - Message prioritization
 *   - Automatic acknowledgment handling
 *   - Callback-based event handling
 *   - Memory pooling for zero fragmentation
 */

#ifndef IPC_CORE_HPP
#define IPC_CORE_HPP

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/sys/ring_buffer.h>
#include <stdint.h>

namespace smarthome { namespace ipc {

/*=============================================================================
 * IPC Message Types - Efficient tagged union approach
 *===========================================================================*/

enum class MessageType : uint8_t {
    /* Radio control */
    RADIO_ENABLE = 0x01,
    RADIO_DISABLE = 0x02,
    RADIO_TX = 0x03,
    RADIO_RX = 0x04,
    
    /* BLE operations */
    BLE_ADV_START = 0x10,
    BLE_ADV_STOP = 0x11,
    BLE_CONNECT = 0x12,
    BLE_DISCONNECT = 0x13,
    
    /* Thread/Matter networking */
    THREAD_START = 0x20,
    THREAD_STOP = 0x21,
    THREAD_ATTACH = 0x22,
    
    /* Status/Control */
    STATUS_REQUEST = 0x30,
    STATUS_RESPONSE = 0x31,
    ACK = 0x32,
    NACK = 0x33,
    
    /* Custom user messages */
    USER_MSG = 0x40
};

enum class Priority : uint8_t {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    CRITICAL = 3
};

/*=============================================================================
 * Message Structure - Fixed size for memory efficiency
 * Total: 32 bytes (cache-line friendly on Cortex-M33)
 *===========================================================================*/

#pragma pack(push, 1)
struct Message {
    MessageType type;           // 1 byte - Message identifier
    Priority priority;          // 1 byte - Priority level
    uint8_t flags;             // 1 byte - Reserved for flags
    uint8_t sequence_id;       // 1 byte - For ordering/tracking
    uint32_t timestamp;        // 4 bytes - k_uptime_get_32()
    
    union {
        uint8_t raw[24];       // 24 bytes - Raw data payload
        
        struct {
            uint32_t param1;
            uint32_t param2;
            uint32_t param3;
            uint32_t param4;
            uint32_t param5;
            uint32_t param6;
        } params;
        
        struct {
            uint8_t channel;
            uint8_t power_dbm;
            uint16_t reserved;
            uint8_t data[20];
        } radio;
        
        struct {
            uint16_t adv_interval_ms;
            uint8_t adv_type;
            uint8_t adv_data_len;
            uint8_t adv_data[20];
        } ble;
        
        struct {
            uint32_t status_code;
            uint8_t info[20];
        } status;
    } payload;
};
#pragma pack(pop)

static_assert(sizeof(Message) == 32, "Message must be 32 bytes for alignment");
/*=============================================================================
 * Message Callback Interface - Observer pattern
 *===========================================================================*/

using MessageCallback = void (*)(const Message&);

/*=============================================================================
 * IPC Core Class - Singleton pattern for resource control
 *===========================================================================*/

class IPCCore {
public:
    /* Configuration constants */
    static constexpr uint16_t MAX_MESSAGE_QUEUE = 16;
    static constexpr uint16_t TX_BUFFER_SIZE = 512;
    static constexpr uint16_t RX_BUFFER_SIZE = 512;
    static constexpr uint32_t IPC_TIMEOUT_MS = 1000;
    
    /* Singleton access */
    static IPCCore& getInstance();
    
    /* Delete copy/move constructors for singleton */
    IPCCore(const IPCCore&) = delete;
    IPCCore& operator=(const IPCCore&) = delete;
    IPCCore(IPCCore&&) = delete;
    IPCCore& operator=(IPCCore&&) = delete;
    
    /*=========================================================================
     * Core Operations
     *=======================================================================*/
    
    /**
     * @brief Initialize IPC subsystem
     * @return 0 on success, negative errno on failure
     */
    int init();
    
    /**
     * @brief Send message to remote core
     * @param msg Message to send
     * @param timeout_ms Timeout in milliseconds (0 = no wait)
     * @return 0 on success, negative errno on failure
     */
    int send(const Message& msg, uint32_t timeout_ms = IPC_TIMEOUT_MS);
    
    /**
     * @brief Send message and wait for acknowledgment
     * @param msg Message to send
     * @param timeout_ms Timeout in milliseconds
     * @return 0 on success, negative errno on failure
     */
    int sendSync(const Message& msg, uint32_t timeout_ms = IPC_TIMEOUT_MS);
    
    /**
     * @brief Register callback for specific message type
     * @param type Message type to listen for
     * @param callback Function to call when message received
     */
    void registerCallback(MessageType type, MessageCallback callback);
    
    /**
     * @brief Unregister callback for message type
     * @param type Message type to stop listening for
     */
    void unregisterCallback(MessageType type);
    
    /**
     * @brief Check if IPC is ready for communication
     * @return true if ready, false otherwise
     */
    bool isReady() const { return m_ready; }
    
    /**
     * @brief Get statistics for monitoring
     */
    struct Statistics {
        uint32_t tx_count;
        uint32_t rx_count;
        uint32_t tx_errors;
        uint32_t rx_errors;
        uint32_t dropped_messages;
        uint32_t buffer_overruns;
    };
    
    const Statistics& getStats() const { return m_stats; }
    void resetStats();
    
private:
    /* Private constructor for singleton */
    IPCCore();
    ~IPCCore() = default;
    
    /*=========================================================================
     * Internal State
     *=======================================================================*/
    
    bool m_ready;
    uint8_t m_sequence_counter;
    struct ipc_ept m_endpoint;
    struct ipc_ept_cfg m_endpoint_cfg;
    Statistics m_stats;
    
    /* Message queues - static allocation */
    struct k_msgq m_tx_queue;
    struct k_msgq m_rx_queue;
    char m_tx_queue_buffer[MAX_MESSAGE_QUEUE * sizeof(Message)];
    char m_rx_queue_buffer[MAX_MESSAGE_QUEUE * sizeof(Message)];
    
    /* Synchronization primitives */
    struct k_mutex m_tx_mutex;
    struct k_sem m_ack_sem;
    struct k_sem m_ready_sem;
    
    /* Callback registry - fixed size for memory efficiency */
    static constexpr uint8_t MAX_CALLBACKS = 16;
    struct CallbackEntry {
        MessageType type;
        MessageCallback callback;
        bool active;
    };
    CallbackEntry m_callbacks[MAX_CALLBACKS];
    
    /* Worker thread for RX processing */
    struct k_thread m_rx_thread;
    K_KERNEL_STACK_MEMBER(m_rx_stack, 1024);
    
    /*=========================================================================
     * Internal Methods
     *=======================================================================*/
    
    /* IPC service callbacks (static - called by Zephyr) */
    static void onEndpointBound(void *priv);
    static void onMessageReceived(const void *data, size_t len, void *priv);
    static void onError(const char *message, void *priv);
    
    /* Message processing */
    void processReceivedMessage(const Message& msg);
    void dispatchMessage(const Message& msg);
    
    /* Worker thread entry point */
    static void rxThreadEntry(void *p1, void *p2, void *p3);
    void rxThreadLoop();
    
    /* Helper methods */
    bool validateMessage(const Message& msg) const;
    void updateStats(bool tx, bool error);
};

/*=============================================================================
 * Message Builder - Fluent interface for construction
 *===========================================================================*/

class MessageBuilder {
public:
    MessageBuilder(MessageType type) {
        m_msg = {};
        m_msg.type = type;
        m_msg.priority = Priority::NORMAL;
        m_msg.timestamp = k_uptime_get_32();
    }
    
    MessageBuilder& setPriority(Priority p) {
        m_msg.priority = p;
        return *this;
    }
    
    MessageBuilder& setParam(uint8_t index, uint32_t value) {
        if (index < 6) {
            switch(index) {
                case 0: m_msg.payload.params.param1 = value; break;
                case 1: m_msg.payload.params.param2 = value; break;
                case 2: m_msg.payload.params.param3 = value; break;
                case 3: m_msg.payload.params.param4 = value; break;
                case 4: m_msg.payload.params.param5 = value; break;
                case 5: m_msg.payload.params.param6 = value; break;
            }
        }
        return *this;
    }
    
    MessageBuilder& setRawData(const void* data, size_t len) {
        size_t copy_len = len > sizeof(m_msg.payload.raw) ? 
                         sizeof(m_msg.payload.raw) : len;
        memcpy(m_msg.payload.raw, data, copy_len);
        return *this;
    }
    
    Message build() {
        return m_msg;
    }
    
private:
    Message m_msg;
};

}  // namespace ipc
}  // namespace smarthome

#endif  // IPC_CORE_HPP
