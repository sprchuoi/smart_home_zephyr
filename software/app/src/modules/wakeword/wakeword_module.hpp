/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WAKEWORD_MODULE_HPP
#define WAKEWORD_MODULE_HPP

#include "core/Module.hpp"
#include <zephyr/kernel.h>

/**
 * @brief Wake Word Detection Module
 * 
 * Detects wake words in audio stream using lightweight models
 * Supports Edge Impulse or custom models
 */
class WakeWordModule : public Module {
public:
    enum class DetectionResult {
        NO_DETECTION,
        WAKE_WORD_DETECTED,
        ERROR
    };
    
    struct DetectionInfo {
        const char* keyword;
        float confidence;
        uint32_t timestamp_ms;
    };
    
    using DetectionCallback = void (*)(const DetectionInfo& info);
    
    static constexpr float DEFAULT_THRESHOLD = 0.7f;
    static constexpr size_t WINDOW_SIZE = 512;  // Samples per detection window
    
    static WakeWordModule& getInstance();
    
    int init() override;
    int start() override;
    int stop() override;
    const char* getName() const override { return "WakeWordModule"; }
    
    /**
     * @brief Process audio samples for wake word detection
     * @param samples Audio samples (16-bit PCM)
     * @param count Number of samples
     * @return Detection result
     */
    DetectionResult process(const int16_t* samples, size_t count);
    
    /**
     * @brief Set detection callback
     * @param callback Function called when wake word detected
     */
    void setDetectionCallback(DetectionCallback callback);
    
    /**
     * @brief Set detection threshold (0.0 - 1.0)
     * @param threshold Confidence threshold
     */
    void setThreshold(float threshold);
    
    /**
     * @brief Get current threshold
     * @return Threshold value
     */
    float getThreshold() const { return threshold_; }
    
    /**
     * @brief Check if model is loaded
     * @return true if model loaded
     */
    bool isModelLoaded() const { return model_loaded_; }
    
private:
    WakeWordModule();
    ~WakeWordModule() override = default;
    
    WakeWordModule(const WakeWordModule&) = delete;
    WakeWordModule& operator=(const WakeWordModule&) = delete;
    
    int loadModel();
    float runInference(const int16_t* samples, size_t count);
    void preprocessAudio(const int16_t* samples, size_t count, float* features);
    
    DetectionCallback detection_callback_;
    float threshold_;
    bool model_loaded_;
    bool running_;
    
    // Feature extraction buffers
    float feature_buffer_[WINDOW_SIZE];
    int16_t audio_buffer_[WINDOW_SIZE];
    size_t buffer_idx_;
    
    struct k_mutex mutex_;
    uint32_t detection_count_;
};

#endif // WAKEWORD_MODULE_HPP
