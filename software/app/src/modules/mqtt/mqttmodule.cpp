/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mqttmodule.hpp"
#include <zephyr/logging/log.h>
#include <zephyr/net/dns_resolve.h>
#include <string.h>

LOG_MODULE_REGISTER(mqtt_module, CONFIG_APP_LOG_LEVEL);

MQTTModule& MQTTModule::getInstance() {
    static MQTTModule instance;
    return instance;
}

MQTTModule::MQTTModule()
    : connected_(false)
    , message_callback_(nullptr)
{
    k_mutex_init(&mutex_);
    k_sem_init(&connected_sem_, 0, 1);
    memset(&config_, 0, sizeof(config_));
    memset(&client_, 0, sizeof(client_));
}

int MQTTModule::init() {
    // Default configuration
    Config default_config = {
        .broker_host = "192.168.1.100",
        .broker_port = DEFAULT_PORT,
        .client_id = "esp32_001",
        .username = "esp32_user",
        .password = "password",
        .device_id = "esp32_001"
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
    if (config_.username) {
        client_.user_name = (struct mqtt_utf8*)&config_.username;
        client_.password = (struct mqtt_utf8*)&config_.password;
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
    
    // Resolve broker address
    struct zsock_addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };
    
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
    
    // Create socket
    client_.transport.type = MQTT_TRANSPORT_NON_SECURE;
    
    ret = mqtt_connect(&client_);
    if (ret != 0) {
        LOG_ERR("MQTT connect failed: %d", ret);
        return ret;
    }
    
    // Wait for connection with timeout
    ret = k_sem_take(&connected_sem_, K_SECONDS(10));
    if (ret != 0) {
        LOG_ERR("MQTT connection timeout");
        return -ETIMEDOUT;
    }
    
    LOG_INF("Connected to MQTT broker");
    return 0;
}

int MQTTModule::disconnect() {
    if (!connected_) {
        return 0;
    }
    
    int ret = mqtt_disconnect(&client_);
    if (ret != 0) {
        LOG_ERR("MQTT disconnect failed: %d", ret);
        return ret;
    }
    
    connected_ = false;
    LOG_INF("Disconnected from MQTT broker");
    return 0;
}

int MQTTModule::publish(const char* topic, const uint8_t* payload, size_t len, uint8_t qos) {
    if (!connected_ || !topic || !payload) {
        return -EINVAL;
    }
    
    struct mqtt_publish_param param = {
        .message.topic.qos = qos,
        .message.topic.topic.utf8 = (uint8_t*)topic,
        .message.topic.topic.size = strlen(topic),
        .message.payload.data = (uint8_t*)payload,
        .message.payload.len = len,
        .message_id = 1,
        .dup_flag = 0,
        .retain_flag = 0,
    };
    
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
    
    struct mqtt_topic subscribe_topic = {
        .topic.utf8 = (uint8_t*)topic,
        .topic.size = strlen(topic),
        .qos = MQTT_QOS_1_AT_LEAST_ONCE
    };
    
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
