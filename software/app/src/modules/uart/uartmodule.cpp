/*
 * UART Module Implementation
 */

#include "uartmodule.hpp"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(uart_module, CONFIG_APP_LOG_LEVEL);

// Message structure for UART -> BLE communication
struct uart_msg {
    uint8_t data;
    uint32_t timestamp;
};

UartModule& UartModule::getInstance() {
    static UartModule instance;
    return instance;
}

UartModule::UartModule() 
    : uart_dev_(nullptr)
    , msgq_(nullptr) {
}

int UartModule::init(struct k_msgq* msgq) {
    if (!msgq) {
        LOG_ERR("Message queue is NULL");
        return -EINVAL;
    }
    
    msgq_ = msgq;
    
    // Get UART device (using default console UART)
    uart_dev_ = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    
    if (!device_is_ready(uart_dev_)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }
    
    // Configure UART for interrupt-driven operation
    int ret = uart_irq_callback_user_data_set(uart_dev_, uart_irq_handler, this);
    if (ret < 0) {
        LOG_ERR("Failed to set UART callback: %d", ret);
        return ret;
    }
    
    // Enable RX interrupt
    uart_irq_rx_enable(uart_dev_);
    
    LOG_INF("UART module initialized (interrupt-driven)");
    return 0;
}

void UartModule::uart_irq_handler(const struct device *dev, void *user_data) {
    UartModule *self = static_cast<UartModule*>(user_data);
    
    if (!uart_irq_update(dev)) {
        return;
    }
    
    if (uart_irq_rx_ready(dev)) {
        uint8_t buf[32];
        int recv_len = uart_fifo_read(dev, buf, sizeof(buf));
        
        if (recv_len > 0) {
            // Send each byte to message queue
            for (int i = 0; i < recv_len; i++) {
                struct uart_msg msg;
                msg.data = buf[i];
                msg.timestamp = k_uptime_get_32();
                
                // Non-blocking send to message queue
                if (k_msgq_put(self->msgq_, &msg, K_NO_WAIT) != 0) {
                    // Queue full, drop message
                    LOG_WRN("UART message queue full, dropped byte: 0x%02x", buf[i]);
                }
            }
        }
    }
}

int UartModule::send(const uint8_t* data, size_t len) {
    if (!uart_dev_ || !data || len == 0) {
        return -EINVAL;
    }
    
    for (size_t i = 0; i < len; i++) {
        uart_poll_out(uart_dev_, data[i]);
    }
    
    return 0;
}
