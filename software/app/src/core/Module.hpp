/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULE_HPP
#define MODULE_HPP

#include <zephyr/kernel.h>
#include <stdint.h>

/**
 * @brief Base class for all modules
 * 
 * Provides common interface for module lifecycle management
 */
class Module {
public:
    virtual ~Module() = default;
    
    /**
     * @brief Initialize the module
     * @return 0 on success, negative errno on failure
     */
    virtual int init() = 0;
    
    /**
     * @brief Start the module (optional)
     * @return 0 on success, negative errno on failure
     */
    virtual int start() { return 0; }
    
    /**
     * @brief Stop the module (optional)
     * @return 0 on success, negative errno on failure
     */
    virtual int stop() { return 0; }
    
    /**
     * @brief Get module name
     * @return Module name string
     */
    virtual const char* getName() const = 0;
    
protected:
    Module() = default;
    
    // Prevent copying
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
};

#endif // MODULE_HPP
