/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file cpp_support.cpp
 * @brief Minimal C++ runtime support for embedded systems
 * 
 * Provides minimal implementations of C++ runtime functions
 * needed for singletons, exceptions, and memory management.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <cstdint>

// Guard type for static initialization
typedef uint64_t __guard_t;

extern "C" {

// Thread-safe static initialization guards
static K_MUTEX_DEFINE(guard_mutex);

int __cxa_guard_acquire(__guard_t *g) {
    k_mutex_lock(&guard_mutex, K_FOREVER);
    return !*(char *)(g);
}

void __cxa_guard_release(__guard_t *g) {
    *(char *)g = 1;
    k_mutex_unlock(&guard_mutex);
}

void __cxa_guard_abort(__guard_t *g) {
    k_mutex_unlock(&guard_mutex);
}

} // extern "C"

// std::function exception stubs
namespace std {
    void __throw_bad_function_call() {
        printk("ERROR: Bad function call (nullptr std::function)\n");
        k_panic();
    }
    
    void __throw_length_error(const char* msg) {
        printk("ERROR: Length error: %s\n", msg);
        k_panic();
    }
    
    void __throw_bad_alloc() {
        printk("ERROR: Bad allocation\n");
        k_panic();
    }
} // namespace std
