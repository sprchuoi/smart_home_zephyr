/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wakeword_module.hpp"
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>

LOG_MODULE_REGISTER(wakeword, CONFIG_APP_LOG_LEVEL);

WakeWordModule& WakeWordModule::getInstance() {
    static WakeWordModule instance;
    return instance;
}

WakeWordModule::WakeWordModule()
    : detection_callback_(nullptr)
    , threshold_(DEFAULT_THRESHOLD)
    , model_loaded_(false)
    , running_(false)
    , buffer_idx_(0)
    , detection_count_(0)
{
    k_mutex_init(&mutex_);
    memset(feature_buffer_, 0, sizeof(feature_buffer_));
    memset(audio_buffer_, 0, sizeof(audio_buffer_));
}

int WakeWordModule::init() {
    LOG_INF("Initializing wake word detection module");
    
    int ret = loadModel();
    if (ret < 0) {
        LOG_ERR("Failed to load model: %d", ret);
        return ret;
    }
    
    LOG_INF("Wake word module initialized, threshold: %.2f", threshold_);
    return 0;
}

int WakeWordModule::start() {
    k_mutex_lock(&mutex_, K_FOREVER);
    running_ = true;
    buffer_idx_ = 0;
    detection_count_ = 0;
    k_mutex_unlock(&mutex_);
    
    LOG_INF("Wake word detection started");
    return 0;
}

int WakeWordModule::stop() {
    k_mutex_lock(&mutex_, K_FOREVER);
    running_ = false;
    k_mutex_unlock(&mutex_);
    
    LOG_INF("Wake word detection stopped");
    return 0;
}

WakeWordModule::DetectionResult WakeWordModule::process(const int16_t* samples, size_t count) {
    if (!running_ || !model_loaded_ || !samples || count == 0) {
        return DetectionResult::ERROR;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    // Fill audio buffer
    for (size_t i = 0; i < count; i++) {
        audio_buffer_[buffer_idx_] = samples[i];
        buffer_idx_ = (buffer_idx_ + 1) % WINDOW_SIZE;
        
        // Run inference when buffer is full
        if (buffer_idx_ == 0) {
            float confidence = runInference(audio_buffer_, WINDOW_SIZE);
            
            if (confidence >= threshold_) {
                LOG_INF("Wake word detected! Confidence: %.2f", confidence);
                
                detection_count_++;
                
                if (detection_callback_) {
                    DetectionInfo info = {
                        .keyword = "hey_device",
                        .confidence = confidence,
                        .timestamp_ms = k_uptime_get_32()
                    };
                    detection_callback_(info);
                }
                
                k_mutex_unlock(&mutex_);
                return DetectionResult::WAKE_WORD_DETECTED;
            }
        }
    }
    
    k_mutex_unlock(&mutex_);
    return DetectionResult::NO_DETECTION;
}

void WakeWordModule::setDetectionCallback(DetectionCallback callback) {
    k_mutex_lock(&mutex_, K_FOREVER);
    detection_callback_ = callback;
    k_mutex_unlock(&mutex_);
}

void WakeWordModule::setThreshold(float threshold) {
    if (threshold < 0.0f || threshold > 1.0f) {
        LOG_WRN("Invalid threshold: %.2f, using default", threshold);
        threshold = DEFAULT_THRESHOLD;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    threshold_ = threshold;
    k_mutex_unlock(&mutex_);
    
    LOG_INF("Detection threshold set to: %.2f", threshold);
}

int WakeWordModule::loadModel() {
    // TODO: Load Edge Impulse model or custom model
    // For now, use placeholder
    LOG_WRN("Using placeholder model (no actual detection)");
    model_loaded_ = true;
    return 0;
}

float WakeWordModule::runInference(const int16_t* samples, size_t count) {
    // Preprocess audio to features
    preprocessAudio(samples, count, feature_buffer_);
    
    // TODO: Run neural network inference
    // For now, use simple energy-based detection as placeholder
    float energy = 0.0f;
    for (size_t i = 0; i < count; i++) {
        float normalized = samples[i] / 32768.0f;
        energy += normalized * normalized;
    }
    energy = sqrtf(energy / count);
    
    // Simulate confidence score
    float confidence = energy * 2.0f; // Scale energy to 0-1 range
    if (confidence > 1.0f) {
        confidence = 1.0f;
    }
    
    return confidence;
}

void WakeWordModule::preprocessAudio(const int16_t* samples, size_t count, float* features) {
    // Simple normalization to [-1, 1]
    for (size_t i = 0; i < count && i < WINDOW_SIZE; i++) {
        features[i] = samples[i] / 32768.0f;
    }
    
    // TODO: Apply FFT, MFCC, or other feature extraction
}
