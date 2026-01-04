/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ipc_core.hpp"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(ipc_core, CONFIG_LOG_DEFAULT_LEVEL);

namespace smarthome { namespace ipc {

/*=============================================================================
 * Singleton Implementation
 *===========================================================================*/

IPCCore& IPCCore::getInstance() {
    static IPCCore instance;
    return instance;
}

/*=============================================================================
 * Constructor - Initialize all static members
 *===========================================================================*/

IPCCore::IPCCore() 
    : m_ready(false)
    , m_sequence_counter(0)
    , m_endpoint{}
    , m_endpoint_cfg{}
    , m_stats{}
{
    /* Initialize message queues with static buffers */
    k_msgq_init(&m_tx_queue, m_tx_queue_buffer, sizeof(Message), MAX_MESSAGE_QUEUE);
    k_msgq_init(&m_rx_queue, m_rx_queue_buffer, sizeof(Message), MAX_MESSAGE_QUEUE);
    
    /* Initialize synchronization primitives */
    k_mutex_init(&m_tx_mutex);
    k_sem_init(&m_ack_sem, 0, 1);
    k_sem_init(&m_ready_sem, 0, 1);
    
    /* Clear callback registry */
    memset(m_callbacks, 0, sizeof(m_callbacks));
    
    LOG_DBG("IPCCore constructed");
}

/*=============================================================================
 * Initialization
 *===========================================================================*/

int IPCCore::init() {
    if (m_ready) {
        LOG_WRN("IPC already initialized");
        return 0;
    }
    
    LOG_INF("Initializing IPC service...");
    
    /* Get IPC service device */
    const struct device *ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
    if (!device_is_ready(ipc_instance)) {
        LOG_ERR("IPC device not ready");
        return -ENODEV;
    }
    
    /* Configure endpoint */
    m_endpoint_cfg.name = "ipc_core";
    m_endpoint_cfg.cb = {
        .bound = onEndpointBound,
        .received = onMessageReceived,
        .error = onError
    };
    m_endpoint_cfg.priv = this;
    
    /* Open endpoint */
    int ret = ipc_service_open_instance(ipc_instance);
    if (ret < 0 && ret != -EALREADY) {
        LOG_ERR("Failed to open IPC instance: %d", ret);
        return ret;
    }
    
    /* Register endpoint */
    ret = ipc_service_register_endpoint(ipc_instance, &m_endpoint, &m_endpoint_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to register endpoint: %d", ret);
        return ret;
    }
    
    /* Start RX thread */
    k_thread_create(&m_rx_thread, m_rx_stack, K_KERNEL_STACK_SIZEOF(m_rx_stack),
                    rxThreadEntry, this, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);
    k_thread_name_set(&m_rx_thread, "ipc_rx");
    
    /* Wait for endpoint to be bound (with timeout) */
    ret = k_sem_take(&m_ready_sem, K_MSEC(5000));
    if (ret < 0) {
        LOG_ERR("Timeout waiting for endpoint binding");
        return -ETIMEDOUT;
    }
    
    LOG_INF("IPC initialized successfully");
    return 0;
}

/*=============================================================================
 * Send Operations
 *===========================================================================*/

int IPCCore::send(const Message& msg, uint32_t timeout_ms) {
    if (!m_ready) {
        LOG_ERR("IPC not ready");
        return -ENOTCONN;
    }
    
    if (!validateMessage(msg)) {
        LOG_ERR("Invalid message");
        return -EINVAL;
    }
    
    /* Lock TX path */
    k_mutex_lock(&m_tx_mutex, K_FOREVER);
    
    /* Add sequence number */
    Message tx_msg = msg;
    tx_msg.sequence_id = m_sequence_counter++;
    tx_msg.timestamp = k_uptime_get_32();
    
    /* Send via IPC service */
    int ret = ipc_service_send(&m_endpoint, &tx_msg, sizeof(Message));
    
    k_mutex_unlock(&m_tx_mutex);
    
    if (ret < 0) {
        LOG_ERR("IPC send failed: %d", ret);
        updateStats(true, true);
        return ret;
    }
    
    updateStats(true, false);
    LOG_DBG("Sent message type=0x%02x seq=%d", (uint8_t)tx_msg.type, tx_msg.sequence_id);
    
    return 0;
}

int IPCCore::sendSync(const Message& msg, uint32_t timeout_ms) {
    /* Reset ACK semaphore */
    k_sem_reset(&m_ack_sem);
    
    /* Send message */
    int ret = send(msg, timeout_ms);
    if (ret < 0) {
        return ret;
    }
    
    /* Wait for ACK */
    ret = k_sem_take(&m_ack_sem, K_MSEC(timeout_ms));
    if (ret < 0) {
        LOG_WRN("Timeout waiting for ACK");
        return -ETIMEDOUT;
    }
    
    return 0;
}

/*=============================================================================
 * Callback Management
 *===========================================================================*/

void IPCCore::registerCallback(MessageType type, MessageCallback callback) {
    /* Find free slot */
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!m_callbacks[i].active) {
            m_callbacks[i].type = type;
            m_callbacks[i].callback = callback;
            m_callbacks[i].active = true;
            LOG_DBG("Registered callback for message type 0x%02x", (uint8_t)type);
            return;
        }
    }
    
    LOG_ERR("No free callback slots (max %d)", MAX_CALLBACKS);
}

void IPCCore::unregisterCallback(MessageType type) {
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (m_callbacks[i].active && m_callbacks[i].type == type) {
            m_callbacks[i].active = false;
            LOG_DBG("Unregistered callback for message type 0x%02x", (uint8_t)type);
            return;
        }
    }
}

/*=============================================================================
 * Statistics
 *===========================================================================*/

void IPCCore::resetStats() {
    memset(&m_stats, 0, sizeof(m_stats));
    LOG_INF("Statistics reset");
}

void IPCCore::updateStats(bool tx, bool error) {
    if (tx) {
        if (error) {
            m_stats.tx_errors++;
        } else {
            m_stats.tx_count++;
        }
    } else {
        if (error) {
            m_stats.rx_errors++;
        } else {
            m_stats.rx_count++;
        }
    }
}

/*=============================================================================
 * IPC Service Callbacks
 *===========================================================================*/

void IPCCore::onEndpointBound(void *priv) {
    IPCCore *ipc = static_cast<IPCCore*>(priv);
    LOG_INF("IPC endpoint bound");
    
    ipc->m_ready = true;
    k_sem_give(&ipc->m_ready_sem);
}

void IPCCore::onMessageReceived(const void *data, size_t len, void *priv) {
    IPCCore *ipc = static_cast<IPCCore*>(priv);
    
    if (len != sizeof(Message)) {
        LOG_ERR("Received invalid message size: %u (expected %u)", 
                len, sizeof(Message));
        ipc->updateStats(false, true);
        return;
    }
    
    const Message *msg = static_cast<const Message*>(data);
    
    /* Queue message for processing in RX thread */
    int ret = k_msgq_put(&ipc->m_rx_queue, msg, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("RX queue full, dropping message");
        ipc->m_stats.dropped_messages++;
        ipc->updateStats(false, true);
    }
}

void IPCCore::onError(const char *message, void *priv) {
    IPCCore *ipc = static_cast<IPCCore*>(priv);
    LOG_ERR("IPC error: %s", message);
    ipc->updateStats(false, true);
}

/*=============================================================================
 * Message Processing
 *===========================================================================*/

void IPCCore::processReceivedMessage(const Message& msg) {
    LOG_DBG("Processing message type=0x%02x seq=%d", 
            (uint8_t)msg.type, msg.sequence_id);
    
    updateStats(false, false);
    
    /* Handle special messages */
    switch (msg.type) {
        case MessageType::ACK:
            k_sem_give(&m_ack_sem);
            return;
            
        case MessageType::NACK:
            LOG_WRN("Received NACK from remote core");
            k_sem_give(&m_ack_sem);
            return;
            
        default:
            /* Dispatch to registered callbacks */
            dispatchMessage(msg);
            break;
    }
}

void IPCCore::dispatchMessage(const Message& msg) {
    bool handled = false;
    
    /* Call all registered callbacks for this message type */
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (m_callbacks[i].active && m_callbacks[i].type == msg.type) {
            if (m_callbacks[i].callback) {
                m_callbacks[i].callback(msg);
                handled = true;
            }
        }
    }
    
    if (!handled) {
        LOG_DBG("No handler for message type 0x%02x", (uint8_t)msg.type);
    }
}

/*=============================================================================
 * RX Thread
 *===========================================================================*/

void IPCCore::rxThreadEntry(void *p1, void *p2, void *p3) {
    IPCCore *ipc = static_cast<IPCCore*>(p1);
    ipc->rxThreadLoop();
}

void IPCCore::rxThreadLoop() {
    LOG_INF("IPC RX thread started");
    
    Message msg;
    
    while (1) {
        /* Block waiting for messages */
        int ret = k_msgq_get(&m_rx_queue, &msg, K_FOREVER);
        if (ret == 0) {
            processReceivedMessage(msg);
        }
    }
}

/*=============================================================================
 * Validation
 *===========================================================================*/

bool IPCCore::validateMessage(const Message& msg) const {
    /* Basic sanity checks */
    if ((uint8_t)msg.type == 0 || (uint8_t)msg.type == 0xFF) {
        return false;
    }
    
    if ((uint8_t)msg.priority > (uint8_t)Priority::CRITICAL) {
        return false;
    }
    
    return true;
}

}  // namespace ipc
}  // namespace smarthome
