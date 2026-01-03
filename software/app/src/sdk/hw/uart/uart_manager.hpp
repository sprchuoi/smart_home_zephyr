/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * ============================================================================
 * UART MANAGER - Serial Communication for Debug & Logging
 * ============================================================================
 *
 * Purpose:
 *   Hardware abstraction for UART0 debug interface
 *   Provides interrupt-driven serial transmission with message queue
 *   Enables logging without blocking the application
 *
 * Hardware:
 *   UART0: P0.20 TX, P0.22 RX @ 115200 baud
 *   Interrupt-driven with buffer management
 *
 * Features:
 *   - Non-blocking transmit via interrupt
 *   - Message queue for async operations
 *   - Ring buffer reception with callback
 *   - Suitable for high-frequency logging
 */

#ifndef HW_UART_MANAGER_HPP
#define HW_UART_MANAGER_HPP

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

namespace smarthome {
namespace hw {

class UartManager {
public:
    /// Get singleton instance
    static UartManager& getInstance();
    
    // Prevent copying
    UartManager(const UartManager&) = delete;
    UartManager& operator=(const UartManager&) = delete;
    
    /**
     * Initialize UART with message queue
     * 
     * @param msgq Pointer to Zephyr message queue for RX data
     * @return 0 on success, negative error code on failure
     */
    int init(struct k_msgq* msgq);
    
    /**
     * Send data over UART
     * 
     * @param data Pointer to data buffer
     * @param len Number of bytes to send
     * @return Number of bytes sent, or negative error
     */
    int send(const uint8_t* data, size_t len);
    
    /**
     * Get UART device pointer
     * @return Pointer to UART device structure
     */
    const struct device* getDevice() const { return uart_dev_; }

private:
    /// Private constructor (singleton)
    UartManager();
    
    /// Destructor
    ~UartManager() = default;
    
    /// UART interrupt handler (static for C API)
    static void uart_irq_handler(const struct device *dev, void *user_data);
    
    // Members
    const struct device *uart_dev_;  ///< UART device pointer
    struct k_msgq *msgq_;             ///< Message queue for RX
    uint8_t rx_buf_[1];               ///< Single byte RX buffer
};

}  // namespace hw
}  // namespace smarthome

#endif  // HW_UART_MANAGER_HPP
