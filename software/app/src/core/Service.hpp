/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "Module.hpp"
#include <zephyr/kernel.h>

/**
 * @brief Base class for service modules
 * 
 * Services are modules that provide functionality to other modules
 * They typically run in background and respond to events/requests
 */
class Service : public Module {
public:
    virtual ~Service() = default;
    
    /**
     * @brief Check if service is running
     * @return true if service is running
     */ 
    virtual bool isRunning() const { return running_; }
    
    /**
     * @brief Process service (called periodically)
     * @return 0 on success, negative errno on failure
     */
    virtual int process() { return 0; }
    
protected:
    Service() : running_(false) {}
    
    bool running_;
};

#endif // SERVICE_HPP
