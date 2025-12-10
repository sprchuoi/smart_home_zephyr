/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef I2S_MIC_MODULE_HPP
#define I2S_MIC_MODULE_HPP

#include "core/Module.hpp"
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>

/**
 * @brief I2S Microphone Module
 * 
 * Captures audio from I2S MEMS microphone (e.g., INMP441)
 * Provides audio samples via callback or blocking read
 */
class I2SMicModule : public Module {
public:
    static constexpr uint32_t SAMPLE_RATE = 16000;      // 16kHz
    static constexpr uint8_t BITS_PER_SAMPLE = 16;      // 16-bit
    static constexpr uint8_t CHANNELS = 1;              // Mono
    static constexpr size_t BLOCK_SIZE = 512;           // Samples per block
    
    using AudioCallback = void (*)(const int16_t* samples, size_t count);
    
    static I2SMicModule& getInstance();
    
    int init() override;
    int start() override;
    int stop() override;
    const char* getName() const override { return "I2SMicModule"; }
    
    /**
     * @brief Read audio samples (blocking)
     * @param buffer Output buffer for samples
     * @param samples Number of samples to read
     * @return 0 on success, negative errno on failure
     */
    int read(int16_t* buffer, size_t samples);
    
    /**
     * @brief Set callback for audio data
     * @param callback Function called with audio samples
     */
    void setAudioCallback(AudioCallback callback);
    
    /**
     * @brief Get current sample rate
     * @return Sample rate in Hz
     */
    uint32_t getSampleRate() const { return SAMPLE_RATE; }
    
private:
    I2SMicModule();
    ~I2SMicModule() override = default;
    
    I2SMicModule(const I2SMicModule&) = delete;
    I2SMicModule& operator=(const I2SMicModule&) = delete;
    
    const struct device* i2s_dev_;
    AudioCallback audio_callback_;
    bool running_;
    
    struct k_mutex mutex_;
    
    // Ring buffer for audio samples
    static constexpr size_t RING_BUFFER_SIZE = SAMPLE_RATE * 2; // 2 seconds
    int16_t ring_buffer_[RING_BUFFER_SIZE];
    size_t write_idx_;
    size_t read_idx_;
};

#endif // I2S_MIC_MODULE_HPP
