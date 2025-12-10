/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "i2s_mic_module.hpp"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(i2s_mic, CONFIG_APP_LOG_LEVEL);

// ESP32 I2S pins for INMP441
#define I2S_BCK_PIN  26  // BCLK
#define I2S_WS_PIN   25  // LRCLK/WS
#define I2S_DIN_PIN  33  // SD/Data In

#if DT_NODE_EXISTS(DT_NODELABEL(i2s0))
#define I2S_DEV_NODE DT_NODELABEL(i2s0)
#else
#define I2S_DEV_NODE DT_INVALID_NODE
#endif

I2SMicModule& I2SMicModule::getInstance() {
    static I2SMicModule instance;
    return instance;
}

I2SMicModule::I2SMicModule()
    : i2s_dev_(nullptr)
    , audio_callback_(nullptr)
    , running_(false)
    , write_idx_(0)
    , read_idx_(0)
{
    k_mutex_init(&mutex_);
    memset(ring_buffer_, 0, sizeof(ring_buffer_));
}

int I2SMicModule::init() {
#if DT_NODE_EXISTS(I2S_DEV_NODE)
    LOG_INF("Initializing I2S microphone");
    
    i2s_dev_ = DEVICE_DT_GET(I2S_DEV_NODE);
    if (!device_is_ready(i2s_dev_)) {
        LOG_ERR("I2S device not ready");
        return -ENODEV;
    }
    
    // Configure I2S
    struct i2s_config config = {
        .word_size = BITS_PER_SAMPLE,
        .channels = CHANNELS,
        .format = I2S_FMT_DATA_FORMAT_I2S,
        .options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER,
        .frame_clk_freq = SAMPLE_RATE,
        .mem_slab = nullptr,
        .block_size = BLOCK_SIZE * sizeof(int16_t),
        .timeout = 1000,
    };
    
    int ret = i2s_configure(i2s_dev_, I2S_DIR_RX, &config);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S: %d", ret);
        return ret;
    }
    
    LOG_INF("I2S microphone initialized: %dHz, %d-bit", SAMPLE_RATE, BITS_PER_SAMPLE);
    return 0;
#else
    LOG_WRN("I2S device not configured in devicetree");
    return -ENOTSUP;
#endif
}

int I2SMicModule::start() {
#if DT_NODE_EXISTS(I2S_DEV_NODE)
    if (!i2s_dev_ || running_) {
        return -EINVAL;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    int ret = i2s_trigger(i2s_dev_, I2S_DIR_RX, I2S_TRIGGER_START);
    if (ret < 0) {
        LOG_ERR("Failed to start I2S: %d", ret);
        k_mutex_unlock(&mutex_);
        return ret;
    }
    
    running_ = true;
    k_mutex_unlock(&mutex_);
    
    LOG_INF("I2S microphone started");
    return 0;
#else
    return -ENOTSUP;
#endif
}

int I2SMicModule::stop() {
#if DT_NODE_EXISTS(I2S_DEV_NODE)
    if (!i2s_dev_ || !running_) {
        return -EINVAL;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    int ret = i2s_trigger(i2s_dev_, I2S_DIR_RX, I2S_TRIGGER_STOP);
    if (ret < 0) {
        LOG_ERR("Failed to stop I2S: %d", ret);
        k_mutex_unlock(&mutex_);
        return ret;
    }
    
    running_ = false;
    k_mutex_unlock(&mutex_);
    
    LOG_INF("I2S microphone stopped");
    return 0;
#else
    return -ENOTSUP;
#endif
}

int I2SMicModule::read(int16_t* buffer, size_t samples) {
#if DT_NODE_EXISTS(I2S_DEV_NODE)
    if (!i2s_dev_ || !buffer || samples == 0) {
        return -EINVAL;
    }
    
    void* mem_block;
    size_t block_size;
    
    int ret = i2s_read(i2s_dev_, &mem_block, &block_size);
    if (ret < 0) {
        return ret;
    }
    
    // Copy to output buffer
    size_t samples_to_copy = block_size / sizeof(int16_t);
    if (samples_to_copy > samples) {
        samples_to_copy = samples;
    }
    
    memcpy(buffer, mem_block, samples_to_copy * sizeof(int16_t));
    
    // Call callback if set
    if (audio_callback_) {
        audio_callback_(buffer, samples_to_copy);
    }
    
    // Store in ring buffer
    k_mutex_lock(&mutex_, K_FOREVER);
    for (size_t i = 0; i < samples_to_copy; i++) {
        ring_buffer_[write_idx_] = buffer[i];
        write_idx_ = (write_idx_ + 1) % RING_BUFFER_SIZE;
    }
    k_mutex_unlock(&mutex_);
    
    return samples_to_copy;
#else
    return -ENOTSUP;
#endif
}

void I2SMicModule::setAudioCallback(AudioCallback callback) {
    k_mutex_lock(&mutex_, K_FOREVER);
    audio_callback_ = callback;
    k_mutex_unlock(&mutex_);
}
