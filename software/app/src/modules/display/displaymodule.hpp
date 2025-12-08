/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DISPLAY_MODULE_HPP
#define DISPLAY_MODULE_HPP

#include "core/Module.hpp"
#include <zephyr/kernel.h>

class DisplayModule : public Module {
public:
    static constexpr uint32_t SLEEP_TIMEOUT_MS = 30000;
    
    static DisplayModule& getInstance();
    
    int init() override;
    const char* getName() const override { return "DisplayModule"; }
    
    void wake();
    void sleep();
    void updateStatus(const char* line1, const char* line2, const char* line3);
    bool isSleeping() const { return sleeping_; }
    
private:
    DisplayModule();
    ~DisplayModule() override = default;
    
    DisplayModule(const DisplayModule&) = delete;
    DisplayModule& operator=(const DisplayModule&) = delete;
    
    bool sleeping_;
    int64_t last_activity_;
    struct k_mutex mutex_;
};

#endif // DISPLAY_MODULE_HPP
