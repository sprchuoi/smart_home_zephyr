/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MQTT_MODULE_HPP
#define MQTT_MODULE_HPP

#include "core/Service.hpp"
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>

/**
 * @brief MQTT Client Module
 * 
 * Handles MQTT communication with Raspberry Pi broker
 * Supports publish/subscribe for voice, telemetry, and control
 */
class MQTTModule : public Service {
public:
    struct Config {
        const char* broker_host;
        uint16_t broker_port;
        const char* client_id;
        const char* username;
        const char* password;
        const char* device_id;
    };
    
    using MessageCallback = void (*)(const char* topic, const uint8_t* payload, size_t len);
    
    static constexpr uint16_t DEFAULT_PORT = 1883;
    static constexpr size_t MAX_PAYLOAD_SIZE = 1024;
    
    static MQTTModule& getInstance();
    
    int init() override;
    int init(const Config& config);
    int start() override;
    int stop() override;
    const char* getName() const override { return "MQTTModule"; }
    
    /**
     * @brief Connect to MQTT broker
     * @return 0 on success, negative errno on failure
     */
    int connect();
    
    /**
     * @brief Disconnect from broker
     * @return 0 on success, negative errno on failure
     */
    int disconnect();
    
    /**
     * @brief Publish message to topic
     * @param topic MQTT topic
     * @param payload Message payload
     * @param len Payload length
     * @param qos QoS level (0, 1, or 2)
     * @return 0 on success, negative errno on failure
     */
    int publish(const char* topic, const uint8_t* payload, size_t len, uint8_t qos = 0);
    
    /**
     * @brief Subscribe to topic
     * @param topic MQTT topic (supports wildcards)
     * @param callback Function called when message received
     * @return 0 on success, negative errno on failure
     */
    int subscribe(const char* topic, MessageCallback callback);
    
    /**
     * @brief Process MQTT messages and maintain connection
     * Must be called periodically to handle keep-alive
     */
    void live();
    
    /**
     * @brief Check if connected to broker
     * @return true if connected
     */
    bool isConnected() const { return connected_; }
    
    /**
     * @brief Get device ID
     * @return Device ID string
     */
    const char* getDeviceId() const { return config_.device_id; }
    
private:
    MQTTModule();
    ~MQTTModule() override = default;
    
    MQTTModule(const MQTTModule&) = delete;
    MQTTModule& operator=(const MQTTModule&) = delete;
    
    static void mqtt_event_handler(struct mqtt_client* client, 
                                   const struct mqtt_evt* evt);
    
    Config config_;
    struct mqtt_client client_;
    struct sockaddr_storage broker_addr_;
    int sock_fd_;
    
    bool connected_;
    MessageCallback message_callback_;
    
    uint8_t rx_buffer_[MAX_PAYLOAD_SIZE];
    uint8_t tx_buffer_[MAX_PAYLOAD_SIZE];
    
    struct mqtt_utf8 username_utf8_;
    struct mqtt_utf8 password_utf8_;
    
    struct k_mutex mutex_;
    struct k_sem connected_sem_;
};

#endif // MQTT_MODULE_HPP
