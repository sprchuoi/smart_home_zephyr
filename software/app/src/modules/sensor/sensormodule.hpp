/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSOR_MODULE_HPP
#define SENSOR_MODULE_HPP

#include "core/Module.hpp"
#include <zephyr/kernel.h>
#include <functional>

class SensorModule : public Module {
public:
    using SensorCallback = std::function<void(int value)>;
    
    static constexpr uint32_t DEFAULT_SAMPLE_PERIOD_MS = 1000;
    
    static SensorModule& getInstance();
    
    int init() override;
    const char* getName() const override { return "SensorModule"; }
    
    int read();
    void setCallback(SensorCallback callback) { callback_ = callback; }
    
private:
    SensorModule();
    ~SensorModule() override = default;
    
    SensorModule(const SensorModule&) = delete;
    SensorModule& operator=(const SensorModule&) = delete;
    
    SensorCallback callback_;
    struct k_mutex mutex_;
};

#endif // SENSOR_MODULE_HPP
