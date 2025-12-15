/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mqttmodule.hpp"
#include <zephyr/logging/log.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/unistd.h>
#include <string.h>

LOG_MODULE_REGISTER(mqtt_module, CONFIG_APP_LOG_LEVEL);

MQTTModule& MQTTModule::getInstance() {
    static MQTTModule instance;
    return instance;
}

MQTTModule::MQTTModule()
    : connected_(false)
    , message_callback_(nullptr)
    , sock_fd_(-1)
{
    k_mutex_init(&mutex_);
    k_sem_init(&connected_sem_, 0, 1);
    memset(&config_, 0, sizeof(config_));
    memset(&client_, 0, sizeof(client_));
}

int MQTTModule::init() {
    // Default configuration
    // MQTT Broker: 192.168.2.1:1883
    Config default_config = {
        .broker_host = CONFIG_MQTT_BROKER_HOSTNAME,
        .broker_port = CONFIG_MQTT_BROKER_PORT,
        .client_id = CONFIG_MQTT_CLIENT_ID,
        .username = CONFIG_MQTT_USERNAME,
        .password = CONFIG_MQTT_PASSWORD,
        .device_id = CONFIG_MQTT_CLIENT_ID
    };
    
    return init(default_config);
}

int MQTTModule::init(const Config& config) {
    LOG_INF("Initializing MQTT module");
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    config_ = config;
    
    // Initialize MQTT client
    mqtt_client_init(&client_);
    
    // Set buffers
    client_.rx_buf = rx_buffer_;
    client_.rx_buf_size = sizeof(rx_buffer_);
    client_.tx_buf = tx_buffer_;
    client_.tx_buf_size = sizeof(tx_buffer_);
    
    // Set client ID
    client_.client_id.utf8 = (uint8_t*)config_.client_id;
    client_.client_id.size = strlen(config_.client_id);
    
    // Set credentials
    if (config_.username && config_.password) {
        username_utf8_.utf8 = (uint8_t*)config_.username;
        username_utf8_.size = strlen(config_.username);
        password_utf8_.utf8 = (uint8_t*)config_.password;
        password_utf8_.size = strlen(config_.password);
        
        client_.user_name = &username_utf8_;
        client_.password = &password_utf8_;
    }
    
    // Set event handler
    client_.evt_cb = mqtt_event_handler;
    client_.protocol_version = MQTT_VERSION_3_1_1;
    
    k_mutex_unlock(&mutex_);
    
    LOG_INF("MQTT module initialized: broker=%s:%d, client=%s", 
            config_.broker_host, config_.broker_port, config_.client_id);
    
    return 0;
}

int MQTTModule::start() {
    return connect();
}

int MQTTModule::stop() {
    return disconnect();
}

int MQTTModule::connect() {
    if (connected_) {
        return 0;
    }
    
    LOG_INF("Connecting to MQTT broker: %s:%d", 
            config_.broker_host, config_.broker_port);
    
    // Small delay to ensure network stack is ready
    k_sleep(K_MSEC(500));
    
    // Resolve broker address
    struct zsock_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    struct zsock_addrinfo* addr_result;
    int ret = zsock_getaddrinfo(config_.broker_host, NULL, &hints, &addr_result);
    if (ret != 0) {
        LOG_ERR("DNS resolution failed: %d", ret);
        return -EHOSTUNREACH;
    }
    
    memcpy(&broker_addr_, addr_result->ai_addr, addr_result->ai_addrlen);
    zsock_freeaddrinfo(addr_result);
    
    // Set port
    if (broker_addr_.ss_family == AF_INET) {
        ((struct sockaddr_in*)&broker_addr_)->sin_port = htons(config_.broker_port);
    }
    
    LOG_DBG("Creating TCP socket...");
    
    // Create socket
    sock_fd_ = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd_ < 0) {
        LOG_ERR("Failed to create socket: errno=%d", errno);
        return -errno;
    }
    
    LOG_DBG("Socket created: fd=%d", sock_fd_);
    
    // Set socket to non-blocking mode for faster connection attempts
    int flags = zsock_fcntl(sock_fd_, F_GETFL, 0);
    if (flags < 0) {
        LOG_WRN("Failed to get socket flags");
    } else {
        ret = zsock_fcntl(sock_fd_, F_SETFL, flags | O_NONBLOCK);
        if (ret < 0) {
            LOG_WRN("Failed to set non-blocking mode");
        }
    }
    
    // Connect socket to broker (non-blocking)
    LOG_INF("Connecting socket to %s:%d...", config_.broker_host, config_.broker_port);
    ret = zsock_connect(sock_fd_, (struct sockaddr*)&broker_addr_, sizeof(struct sockaddr_in));
    
    if (ret < 0 && errno != EINPROGRESS) {
        LOG_ERR("Socket connect failed: errno=%d", errno);
        zsock_close(sock_fd_);
        sock_fd_ = -1;
        return -errno;
    }
    
    // Wait for connection to complete with poll (1 second timeout)
    if (errno == EINPROGRESS) {
        struct zsock_pollfd fds[1];
        fds[0].fd = sock_fd_;
        fds[0].events = ZSOCK_POLLOUT;
        fds[0].revents = 0;
        
        ret = zsock_poll(fds, 1, 1000);  // 1 second timeout
        if (ret < 0) {
            LOG_ERR("Poll failed: errno=%d", errno);
            zsock_close(sock_fd_);
            sock_fd_ = -1;
            return -errno;
        } else if (ret == 0) {
            LOG_ERR("Connection timeout");
            zsock_close(sock_fd_);
            sock_fd_ = -1;
            return -ETIMEDOUT;
        }
        
        // Check if connection succeeded
        int error = 0;
        socklen_t len = sizeof(error);
        ret = zsock_getsockopt(sock_fd_, SOL_SOCKET, SO_ERROR, &error, &len);
        if (ret < 0 || error != 0) {
            LOG_ERR("Connection failed: %d", error ? error : errno);
            zsock_close(sock_fd_);
            sock_fd_ = -1;
            return error ? -error : -errno;
        }
    }
    
    // Set back to blocking mode for MQTT operations
    flags = zsock_fcntl(sock_fd_, F_GETFL, 0);
    if (flags >= 0) {
        zsock_fcntl(sock_fd_, F_SETFL, flags & ~O_NONBLOCK);
    }
    
    LOG_INF("Socket connected to broker");
    
    // Set up MQTT transport
    client_.transport.type = MQTT_TRANSPORT_NON_SECURE;
    client_.transport.tcp.sock = sock_fd_;
    
    // Set broker address in client
    client_.broker = &broker_addr_;
    
    LOG_DBG("Initiating MQTT connection...");
    
    // Connect MQTT
    ret = mqtt_connect(&client_);
    if (ret != 0) {
        LOG_ERR("MQTT connect failed: %d", ret);
        zsock_close(sock_fd_);
        sock_fd_ = -1;
        return ret;
    }
    
    LOG_DBG("MQTT connect initiated, polling for CONNACK...");
    
    // Poll for CONNACK with timeout
    int64_t timeout_time = k_uptime_get() + K_SECONDS(10).ticks;
    while (k_uptime_get() < timeout_time) {
        // Process MQTT input to get CONNACK
        ret = mqtt_input(&client_);
        if (ret != 0 && ret != -EAGAIN) {
            LOG_ERR("MQTT input failed: %d", ret);
            zsock_close(sock_fd_);
            sock_fd_ = -1;
            return ret;
        }
        
        // Check if connected
        if (k_sem_take(&connected_sem_, K_MSEC(100)) == 0) {
            LOG_INF("MQTT CONNACK received");
            break;
        }
        
        // Keep connection alive
        mqtt_live(&client_);
    }
    
    // Final check if connected
    if (!connected_) {
        LOG_ERR("MQTT connection timeout");
        zsock_close(sock_fd_);
        sock_fd_ = -1;
        return -ETIMEDOUT;
    }
    
    LOG_INF("Connected to MQTT broker");
    return 0;
}

int MQTTModule::disconnect() {
    if (!connected_) {
        return 0;
    }
    
    struct mqtt_disconnect_param param;
    memset(&param, 0, sizeof(param));
    int ret = mqtt_disconnect(&client_, &param);
    if (ret != 0) {
        LOG_ERR("MQTT disconnect failed: %d", ret);
    }
    
    if (sock_fd_ >= 0) {
        zsock_close(sock_fd_);
        sock_fd_ = -1;
    }
    
    connected_ = false;
    LOG_INF("Disconnected from MQTT broker");
    return 0;
}

int MQTTModule::publish(const char* topic, const uint8_t* payload, size_t len, uint8_t qos) {
    if (!connected_ || !topic || !payload) {
        return -EINVAL;
    }
    
    struct mqtt_publish_param param;
    memset(&param, 0, sizeof(param));
    param.message.topic.qos = qos;
    param.message.topic.topic.utf8 = (uint8_t*)topic;
    param.message.topic.topic.size = strlen(topic);
    param.message.payload.data = (uint8_t*)payload;
    param.message.payload.len = len;
    param.message_id = 1;
    param.dup_flag = 0;
    param.retain_flag = 0;
    
    int ret = mqtt_publish(&client_, &param);
    if (ret != 0) {
        LOG_ERR("MQTT publish failed: %d", ret);
        return ret;
    }
    
    LOG_DBG("Published to %s: %d bytes", topic, len);
    return 0;
}

int MQTTModule::subscribe(const char* topic, MessageCallback callback) {
    if (!connected_ || !topic) {
        return -EINVAL;
    }
    
    message_callback_ = callback;
    
    struct mqtt_topic subscribe_topic;
    memset(&subscribe_topic, 0, sizeof(subscribe_topic));
    subscribe_topic.topic.utf8 = (uint8_t*)topic;
    subscribe_topic.topic.size = strlen(topic);
    subscribe_topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
    
    struct mqtt_subscription_list sub_list = {
        .list = &subscribe_topic,
        .list_count = 1,
        .message_id = 1
    };
    
    int ret = mqtt_subscribe(&client_, &sub_list);
    if (ret != 0) {
        LOG_ERR("MQTT subscribe failed: %d", ret);
        return ret;
    }
    
    LOG_INF("Subscribed to: %s", topic);
    return 0;
}

void MQTTModule::live() {
    if (!connected_) {
        return;
    }
    
    // Process incoming messages
    int ret = mqtt_input(&client_);
    if (ret != 0 && ret != -EAGAIN) {
        LOG_WRN("MQTT input error: %d", ret);
    }
    
    // Send keep-alive if needed
    ret = mqtt_live(&client_);
    if (ret != 0 && ret != -EAGAIN) {
        LOG_WRN("MQTT live error: %d", ret);
        if (ret == -ENOTCONN) {
            connected_ = false;
            LOG_ERR("MQTT connection lost");
        }
    }
}

void MQTTModule::mqtt_event_handler(struct mqtt_client* client, const struct mqtt_evt* evt) {
    MQTTModule& mqtt = getInstance();
    
    switch (evt->type) {
    case MQTT_EVT_CONNACK:
        if (evt->result == 0) {
            mqtt.connected_ = true;
            k_sem_give(&mqtt.connected_sem_);
            LOG_INF("MQTT connected");
        } else {
            LOG_ERR("MQTT connection failed: %d", evt->result);
        }
        break;
        
    case MQTT_EVT_DISCONNECT:
        mqtt.connected_ = false;
        LOG_INF("MQTT disconnected");
        break;
        
    case MQTT_EVT_PUBLISH:
        if (mqtt.message_callback_) {
            const struct mqtt_publish_param* pub = &evt->param.publish;
            mqtt.message_callback_(
                (const char*)pub->message.topic.topic.utf8,
                (const uint8_t*)pub->message.payload.data,
                pub->message.payload.len
            );
        }
        break;
        
    default:
        break;
    }
}
