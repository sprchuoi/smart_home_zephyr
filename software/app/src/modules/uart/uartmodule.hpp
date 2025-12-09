/*
 * UART Module - Singleton Pattern
 * Handles UART communication with interrupt-driven reception
 */

#ifndef UART_MODULE_HPP
#define UART_MODULE_HPP

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

class UartModule {
public:
    // Singleton pattern
    static UartModule& getInstance();
    
    // Delete copy constructor and assignment operator
    UartModule(const UartModule&) = delete;
    UartModule& operator=(const UartModule&) = delete;
    
    // Initialize UART with message queue
    int init(struct k_msgq* msgq);
    
    // Send data
    int send(const uint8_t* data, size_t len);
    
    // Get device
    const struct device* getDevice() const { return uart_dev_; }
    
private:
    UartModule();
    ~UartModule() = default;
    
    // UART interrupt callback (must be static)
    static void uart_irq_handler(const struct device *dev, void *user_data);
    
    const struct device *uart_dev_;
    struct k_msgq *msgq_;
    uint8_t rx_buf_[1];  // Single byte reception buffer
};

#endif // UART_MODULE_HPP
    