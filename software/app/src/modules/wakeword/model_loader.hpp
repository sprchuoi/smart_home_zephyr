/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODEL_LOADER_HPP
#define MODEL_LOADER_HPP

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Model loader interface for wake-word detection models
 * 
 * Supports loading Edge Impulse models (TensorFlow Lite) and custom formats
 */
class ModelLoader {
public:
    enum class ModelType {
        EDGE_IMPULSE,
        CUSTOM,
        PLACEHOLDER
    };

    struct ModelInfo {
        ModelType type;
        const uint8_t* model_data;
        size_t model_size;
        size_t input_size;
        size_t output_size;
        const char* version;
    };

    virtual ~ModelLoader() = default;

    /**
     * @brief Load and initialize the model
     * @return 0 on success, negative errno on failure
     */
    virtual int load() = 0;

    /**
     * @brief Run inference on input data
     * @param input Input feature array
     * @param input_size Size of input array
     * @param output Output prediction array
     * @param output_size Size of output array
     * @return 0 on success, negative errno on failure
     */
    virtual int infer(const float* input, size_t input_size,
                      float* output, size_t output_size) = 0;

    /**
     * @brief Unload the model and free resources
     */
    virtual void unload() = 0;

    /**
     * @brief Check if model is loaded
     * @return true if loaded
     */
    virtual bool isLoaded() const = 0;

    /**
     * @brief Get model information
     * @return Model info structure
     */
    virtual ModelInfo getInfo() const = 0;

protected:
    ModelLoader() = default;
};

/**
 * @brief Create model loader based on configuration
 * @return Pointer to model loader instance, or nullptr on failure
 */
ModelLoader* createModelLoader();

#endif // MODEL_LOADER_HPP
