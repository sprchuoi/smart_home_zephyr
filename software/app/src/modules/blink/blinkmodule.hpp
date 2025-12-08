/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLINK_MODULE_HPP
#define BLINK_MODULE_HPP

#include "core/Module.hpp"
#include <zephyr/kernel.h>

class BlinkModule : public Module {
public:
    static constexpr uint32_t DEFAULT_PERIOD_MS = 1000;
    
    static BlinkModule& getInstance();
    
    int init() override;
    const char* getName() const override { return "BlinkModule"; }
    
    void setPeriod(uint32_t period_ms);
    uint32_t getPeriod() const { return period_ms_; }
    void tick();
    
private:
    BlinkModule();
    ~BlinkModule() override = default;
    
    BlinkModule(const BlinkModule&) = delete;
    BlinkModule& operator=(const BlinkModule&) = delete;
    
    uint32_t period_ms_;
    struct k_mutex mutex_;
};

#endif // BLINK_MODULE_HPP
