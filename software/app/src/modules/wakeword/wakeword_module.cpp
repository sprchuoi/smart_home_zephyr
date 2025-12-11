/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wakeword_module.hpp"
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>

// Include ML backend headers based on configuration
#ifdef CONFIG_APP_WAKEWORD_EDGE_IMPULSE
// #include "edge_impulse.h"
// Uncomment above and link Edge Impulse SDK to enable
#endif

#ifdef CONFIG_APP_WAKEWORD_TFLITE
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/schema/schema_generated.h"
// Uncomment above and link TFLite Micro to enable
#endif

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
#ifdef CONFIG_APP_WAKEWORD_TFLITE
    , tflite_interpreter_(nullptr)
    , tflite_tensor_arena_(nullptr)
#endif
#ifdef CONFIG_APP_WAKEWORD_EDGE_IMPULSE
    , ei_impulse_(nullptr)
#endif
{
    k_mutex_init(&mutex_);
    memset(feature_buffer_, 0, sizeof(feature_buffer_));
    memset(audio_buffer_, 0, sizeof(audio_buffer_));
}

int WakeWordModule::init() {
    LOG_INF("Initializing wake word detection module");
    LOG_INF("Backend: %s", getBackendType());
    
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
    LOG_INF("Loading wake word detection model...");
    
#ifdef CONFIG_APP_WAKEWORD_EDGE_IMPULSE
    return loadEdgeImpulseModel();
#elif defined(CONFIG_APP_WAKEWORD_TFLITE)
    return loadTFLiteModel();
#else
    return loadPlaceholderModel();
#endif
}

int WakeWordModule::loadEdgeImpulseModel() {
#ifdef CONFIG_APP_WAKEWORD_EDGE_IMPULSE
    LOG_INF("Loading Edge Impulse model: %s", CONFIG_APP_WAKEWORD_MODEL_PATH);
    
    // Edge Impulse SDK integration would go here
    // The actual implementation depends on the Edge Impulse SDK being linked
    // and the model being exported from Edge Impulse Studio
    
    // Example Edge Impulse integration pattern:
    // 1. Include the Edge Impulse SDK headers
    // 2. Initialize the impulse with ei_impulse_init()
    // 3. Verify the model signature
    // 4. Store the impulse context
    
    // For now, this is a stub that would be filled in with actual Edge Impulse code
    LOG_WRN("Edge Impulse model loading not yet implemented");
    LOG_INF("Please link Edge Impulse SDK and implement ei_impulse_init()");
    LOG_INF("Model path/symbol: %s", CONFIG_APP_WAKEWORD_MODEL_PATH);
    
    model_loaded_ = false;
    return -ENOSYS;  // Function not implemented
#else
    return -ENOTSUP;
#endif
}

int WakeWordModule::loadTFLiteModel() {
#ifdef CONFIG_APP_WAKEWORD_TFLITE
    LOG_INF("Loading TensorFlow Lite model: %s", CONFIG_APP_WAKEWORD_MODEL_PATH);
    
    // TensorFlow Lite Micro integration would go here
    // The actual implementation depends on TFLite Micro being available
    
    // Example TFLite Micro integration pattern:
    // 1. Allocate tensor arena: tflite_tensor_arena_ = k_malloc(CONFIG_APP_WAKEWORD_ARENA_SIZE)
    // 2. Load model data (either from filesystem or embedded C array)
    // 3. Create interpreter with: tflite::MicroInterpreter
    // 4. Allocate tensors
    // 5. Verify input/output tensor shapes
    
    // For now, this is a stub that would be filled in with actual TFLite code
    LOG_WRN("TensorFlow Lite model loading not yet implemented");
    LOG_INF("Please link TFLite Micro library and implement model loading");
    LOG_INF("Model path/symbol: %s", CONFIG_APP_WAKEWORD_MODEL_PATH);
    LOG_INF("Arena size: %d bytes", CONFIG_APP_WAKEWORD_ARENA_SIZE);
    
    model_loaded_ = false;
    return -ENOSYS;  // Function not implemented
#else
    return -ENOTSUP;
#endif
}

int WakeWordModule::loadPlaceholderModel() {
    LOG_WRN("Using placeholder model (energy-based detection)");
    LOG_INF("This is a simple energy detector, not a trained wake-word model");
    LOG_INF("To use real wake-word detection:");
    LOG_INF("  1. Configure CONFIG_APP_WAKEWORD_EDGE_IMPULSE or CONFIG_APP_WAKEWORD_TFLITE");
    LOG_INF("  2. Link the appropriate ML library");
    LOG_INF("  3. Provide a trained model");
    
    model_loaded_ = true;
    return 0;
}

float WakeWordModule::runInference(const int16_t* samples, size_t count) {
    // Preprocess audio to features
    preprocessAudio(samples, count, feature_buffer_);
    
#ifdef CONFIG_APP_WAKEWORD_EDGE_IMPULSE
    return runEdgeImpulseInference(samples, count);
#elif defined(CONFIG_APP_WAKEWORD_TFLITE)
    return runTFLiteInference(samples, count);
#else
    return runPlaceholderInference(samples, count);
#endif
}

float WakeWordModule::runEdgeImpulseInference(const int16_t* samples, size_t count) {
#ifdef CONFIG_APP_WAKEWORD_EDGE_IMPULSE
    // Edge Impulse inference would go here
    // Example pattern:
    // 1. Copy preprocessed features to impulse input buffer
    // 2. Call ei_run_classifier() or equivalent
    // 3. Extract confidence score from result
    // 4. Return confidence for the wake word class
    
    LOG_DBG("Running Edge Impulse inference (stub)");
    return 0.0f;
#else
    return 0.0f;
#endif
}

float WakeWordModule::runTFLiteInference(const int16_t* samples, size_t count) {
#ifdef CONFIG_APP_WAKEWORD_TFLITE
    // TensorFlow Lite inference would go here
    // Example pattern:
    // 1. Get input tensor: input = interpreter->input(0)
    // 2. Copy preprocessed features to input tensor
    // 3. Call interpreter->Invoke()
    // 4. Get output tensor: output = interpreter->output(0)
    // 5. Extract and return confidence score
    
    LOG_DBG("Running TensorFlow Lite inference (stub)");
    return 0.0f;
#else
    return 0.0f;
#endif
}

float WakeWordModule::runPlaceholderInference(const int16_t* samples, size_t count) {
    // Simple energy-based detection as placeholder
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
