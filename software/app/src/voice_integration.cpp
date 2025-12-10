/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file voice_integration.cpp
 * @brief Voice control system integration example
 * 
 * This file demonstrates how to integrate all voice control modules:
 * - I2S microphone for audio capture
 * - Wake-word detection for voice activation
 * - MQTT for communication with Raspberry Pi
 * - OTA for firmware updates
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_APP_I2S_MIC
#include "modules/i2s_mic/i2s_mic_module.hpp"
#endif

#ifdef CONFIG_APP_WAKEWORD
#include "modules/wakeword/wakeword_module.hpp"
#endif

#ifdef CONFIG_APP_MQTT
#include "modules/mqtt/mqttmodule.hpp"
#endif

#ifdef CONFIG_APP_OTA
#include "modules/ota/otamodule.hpp"
#endif

#ifdef CONFIG_WIFI
#include "modules/wifi/wifiservice.hpp"
#endif

LOG_MODULE_REGISTER(voice_integration, CONFIG_APP_LOG_LEVEL);

// Audio capture callback
static void audio_callback(const int16_t* samples, size_t count);

// Wake-word detection callback
static void wakeword_callback(const WakeWordModule::DetectionInfo& info);

// MQTT message callback
static void mqtt_message_callback(const char* topic, const uint8_t* payload, size_t len);

// OTA progress callback
static void ota_progress_callback(size_t downloaded, size_t total);

// State machine
enum class VoiceState {
    INIT,
    CONNECTING,
    LISTENING,
    RECORDING,
    PROCESSING,
    ERROR
};

static VoiceState current_state = VoiceState::INIT;
static bool recording_audio = false;
static int16_t recorded_audio[16000]; // 1 second at 16kHz
static size_t recorded_samples = 0;

/**
 * @brief Initialize voice control system
 */
int voice_control_init() {
    LOG_INF("Initializing voice control system");
    
#ifdef CONFIG_WIFI
    // Initialize WiFi first
    WiFiService& wifi = WiFiService::getInstance();
    int ret = wifi.init();
    if (ret < 0) {
        LOG_ERR("WiFi init failed: %d", ret);
        return ret;
    }
    
    // Connect to WiFi
    ret = wifi.start();
    if (ret < 0) {
        LOG_ERR("WiFi start failed: %d", ret);
        return ret;
    }
    
    LOG_INF("Waiting for WiFi connection...");
    int timeout = 30; // 30 seconds
    while (!wifi.isConnected() && timeout > 0) {
        k_sleep(K_SECONDS(1));
        timeout--;
    }
    
    if (!wifi.isConnected()) {
        LOG_ERR("WiFi connection timeout");
        return -ETIMEDOUT;
    }
    
    LOG_INF("WiFi connected");
#endif

#ifdef CONFIG_APP_MQTT
    // Initialize MQTT
    MQTTModule::Config mqtt_config = {
        .broker_host = "192.168.1.100",
        .broker_port = 1883,
        .client_id = "esp32_001",
        .username = "esp32_user",
        .password = "password",
        .device_id = "esp32_001"
    };
    
    MQTTModule& mqtt = MQTTModule::getInstance();
    ret = mqtt.init(mqtt_config);
    if (ret < 0) {
        LOG_ERR("MQTT init failed: %d", ret);
        return ret;
    }
    
    // Connect to broker
    ret = mqtt.connect();
    if (ret < 0) {
        LOG_ERR("MQTT connect failed: %d", ret);
        return ret;
    }
    
    // Subscribe to control topics
    mqtt.subscribe("control/command", mqtt_message_callback);
    mqtt.subscribe("control/ota", mqtt_message_callback);
    mqtt.subscribe("control/config", mqtt_message_callback);
    
    LOG_INF("MQTT connected and subscribed");
#endif

#ifdef CONFIG_APP_OTA
    // Initialize OTA
    OTAModule& ota = OTAModule::getInstance();
    ret = ota.init();
    if (ret < 0) {
        LOG_ERR("OTA init failed: %d", ret);
        return ret;
    }
    
    ota.setProgressCallback(ota_progress_callback);
    LOG_INF("OTA module ready");
#endif

#ifdef CONFIG_APP_WAKEWORD
    // Initialize wake-word detection
    WakeWordModule& wakeword = WakeWordModule::getInstance();
    ret = wakeword.init();
    if (ret < 0) {
        LOG_ERR("Wake-word init failed: %d", ret);
        return ret;
    }
    
    wakeword.setDetectionCallback(wakeword_callback);
    wakeword.setThreshold(0.7f);
    
    ret = wakeword.start();
    if (ret < 0) {
        LOG_ERR("Wake-word start failed: %d", ret);
        return ret;
    }
    
    LOG_INF("Wake-word detection started");
#endif

#ifdef CONFIG_APP_I2S_MIC
    // Initialize I2S microphone
    I2SMicModule& mic = I2SMicModule::getInstance();
    ret = mic.init();
    if (ret < 0) {
        LOG_ERR("I2S mic init failed: %d", ret);
        return ret;
    }
    
    mic.setAudioCallback(audio_callback);
    
    ret = mic.start();
    if (ret < 0) {
        LOG_ERR("I2S mic start failed: %d", ret);
        return ret;
    }
    
    LOG_INF("I2S microphone started");
#endif
    
    current_state = VoiceState::LISTENING;
    LOG_INF("Voice control system ready");
    
    return 0;
}

/**
 * @brief Audio capture callback - processes incoming audio
 */
static void audio_callback(const int16_t* samples, size_t count) {
    if (current_state != VoiceState::LISTENING && current_state != VoiceState::RECORDING) {
        return;
    }
    
#ifdef CONFIG_APP_WAKEWORD
    // Always run wake-word detection
    WakeWordModule& wakeword = WakeWordModule::getInstance();
    auto result = wakeword.process(samples, count);
    
    // If recording after wake-word, capture audio
    if (recording_audio && recorded_samples < sizeof(recorded_audio)/sizeof(recorded_audio[0])) {
        size_t to_copy = count;
        size_t remaining = sizeof(recorded_audio)/sizeof(recorded_audio[0]) - recorded_samples;
        if (to_copy > remaining) {
            to_copy = remaining;
        }
        
        memcpy(&recorded_audio[recorded_samples], samples, to_copy * sizeof(int16_t));
        recorded_samples += to_copy;
        
        // Stop recording after 3 seconds
        if (recorded_samples >= 48000) { // 3 seconds at 16kHz
            recording_audio = false;
            current_state = VoiceState::PROCESSING;
            
            LOG_INF("Recording complete: %d samples", recorded_samples);
            
            // Process and send audio
#ifdef CONFIG_APP_MQTT
            MQTTModule& mqtt = MQTTModule::getInstance();
            
            // In production, encode audio (OPUS/PCM) before sending
            // For now, send as raw PCM
            char topic[64];
            snprintf(topic, sizeof(topic), "voice/audio/%s", mqtt.getDeviceId());
            
            mqtt.publish(topic, (const uint8_t*)recorded_audio, 
                        recorded_samples * sizeof(int16_t), 1);
            
            LOG_INF("Audio sent via MQTT");
#endif
            
            recorded_samples = 0;
            current_state = VoiceState::LISTENING;
        }
    }
#endif
}

/**
 * @brief Wake-word detection callback
 */
static void wakeword_callback(const WakeWordModule::DetectionInfo& info) {
    LOG_INF("Wake-word detected: %s (%.2f confidence)", info.keyword, info.confidence);
    
    // Start recording audio for 3 seconds
    current_state = VoiceState::RECORDING;
    recording_audio = true;
    recorded_samples = 0;
    
    // Notify via MQTT
#ifdef CONFIG_APP_MQTT
    MQTTModule& mqtt = MQTTModule::getInstance();
    char topic[64];
    snprintf(topic, sizeof(topic), "voice/wakeword/%s", mqtt.getDeviceId());
    
    char payload[128];
    snprintf(payload, sizeof(payload), 
             "{\"keyword\":\"%s\",\"confidence\":%.2f,\"timestamp\":%u}",
             info.keyword, info.confidence, info.timestamp_ms);
    
    mqtt.publish(topic, (const uint8_t*)payload, strlen(payload), 0);
#endif
}

/**
 * @brief MQTT message callback - handles commands from Raspberry Pi
 */
static void mqtt_message_callback(const char* topic, const uint8_t* payload, size_t len) {
    LOG_INF("MQTT message: %s (%d bytes)", topic, len);
    
    // Parse JSON payload (simplified)
    char payload_str[256];
    size_t copy_len = len < sizeof(payload_str) - 1 ? len : sizeof(payload_str) - 1;
    memcpy(payload_str, payload, copy_len);
    payload_str[copy_len] = '\0';
    
    // Handle OTA updates
    if (strstr(topic, "control/ota")) {
#ifdef CONFIG_APP_OTA
        // Expected format: {"version":"1.1.0","url":"http://192.168.1.100/firmware.bin","checksum":"abc123..."}
        // Simple parsing (in production, use JSON library)
        
        char* version_start = strstr(payload_str, "\"version\":\"");
        char* url_start = strstr(payload_str, "\"url\":\"");
        char* checksum_start = strstr(payload_str, "\"checksum\":\"");
        
        if (version_start && url_start && checksum_start) {
            version_start += 11;
            url_start += 7;
            checksum_start += 12;
            
            char* version_end = strchr(version_start, '"');
            char* url_end = strchr(url_start, '"');
            char* checksum_end = strchr(checksum_start, '"');
            
            if (version_end && url_end && checksum_end) {
                *version_end = '\0';
                *url_end = '\0';
                *checksum_end = '\0';
                
                LOG_INF("OTA update: %s -> %s", 
                       OTAModule::getInstance().getCurrentVersion(), version_start);
                
                OTAModule::UpdateInfo update_info = {
                    .version = version_start,
                    .url = url_start,
                    .checksum = checksum_start,
                    .size = 512000 // 500KB estimate
                };
                
                int ret = OTAModule::getInstance().startUpdate(update_info);
                if (ret == 0) {
                    LOG_INF("Update downloaded, applying...");
                    OTAModule::getInstance().applyUpdate();
                } else {
                    LOG_ERR("Update failed: %d", ret);
                }
            }
        }
#endif
    }
    // Handle other commands
    else if (strstr(topic, "control/command")) {
        LOG_INF("Command: %s", payload_str);
        // TODO: Parse and execute commands
    }
}

/**
 * @brief OTA progress callback
 */
static void ota_progress_callback(size_t downloaded, size_t total) {
    static size_t last_reported = 0;
    
    // Report every 10%
    size_t percent = (downloaded * 100) / total;
    size_t last_percent = (last_reported * 100) / total;
    
    if (percent / 10 > last_percent / 10) {
        LOG_INF("OTA progress: %d%%", percent);
        last_reported = downloaded;
    }
}

/**
 * @brief Publish telemetry data
 */
void voice_control_publish_telemetry() {
#ifdef CONFIG_APP_MQTT
    MQTTModule& mqtt = MQTTModule::getInstance();
    
    if (!mqtt.isConnected()) {
        return;
    }
    
    char topic[64];
    snprintf(topic, sizeof(topic), "telemetry/sensors/%s", mqtt.getDeviceId());
    
    // Example telemetry (replace with actual sensor data)
    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"uptime\":%u,\"heap_free\":%u,\"version\":\"%s\"}",
             k_uptime_get_32(),
             k_heap_free_get(NULL),
             CONFIG_APP_VERSION);
    
    mqtt.publish(topic, (const uint8_t*)payload, strlen(payload), 0);
#endif
}

/**
 * @brief Voice control main loop
 */
void voice_control_loop() {
    static uint32_t last_telemetry = 0;
    
    while (1) {
        // Publish telemetry every 30 seconds
        uint32_t now = k_uptime_get_32();
        if (now - last_telemetry > 30000) {
            voice_control_publish_telemetry();
            last_telemetry = now;
        }
        
        k_sleep(K_SECONDS(1));
    }
}
