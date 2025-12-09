/*
 * UART Task Implementation
 * Receives data from UART interrupt via message queue
 * Forwards data to BLE task
 */

#include "uart_task.h"
#include "modules/uart/uartmodule.hpp"
#include "modules/ble/bleservice.hpp"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(uart_task, CONFIG_APP_LOG_LEVEL);

// Message queue for UART data
K_MSGQ_DEFINE(uart_msgq, sizeof(struct uart_msg), UART_MSGQ_SIZE, 4);

// Thread data
K_THREAD_STACK_DEFINE(uart_task_stack, UART_TASK_STACK_SIZE);
struct k_thread uart_task_thread;

// Buffer for accumulating data before sending to BLE
#define BLE_TX_BUFFER_SIZE 128
static uint8_t ble_tx_buffer[BLE_TX_BUFFER_SIZE];
static size_t ble_tx_len = 0;

// Timer for periodic BLE transmission
static struct k_timer ble_tx_timer;

static void ble_tx_timer_handler(struct k_timer *timer) {
    // Send accumulated data to BLE if any
    if (ble_tx_len > 0) {
        BleService& ble = BleService::getInstance();
        
        if (ble.isConnected()) {
            int ret = ble.notify(ble_tx_buffer, ble_tx_len);
            if (ret == 0) {
                LOG_INF("Sent %d bytes to BLE", ble_tx_len);
            }
        }
        
        ble_tx_len = 0;  // Reset buffer
    }
}

void uart_task_entry(void *p1, void *p2, void *p3) {
    LOG_INF("UART task started");
    
    // Initialize UART module with message queue
    UartModule& uart = UartModule::getInstance();
    int ret = uart.init(&uart_msgq);
    if (ret < 0) {
        LOG_ERR("Failed to initialize UART: %d", ret);
        return;
    }
    
    // Initialize timer for periodic BLE transmission (100ms)
    k_timer_init(&ble_tx_timer, ble_tx_timer_handler, NULL);
    k_timer_start(&ble_tx_timer, K_MSEC(100), K_MSEC(100));
    
    struct uart_msg msg;
    
    while (1) {
        // Wait for message from UART interrupt
        ret = k_msgq_get(&uart_msgq, &msg, K_FOREVER);
        if (ret == 0) {
            // Echo received character to UART
            uart.send(&msg.data, 1);
            
            // Add to BLE transmission buffer
            if (ble_tx_len < BLE_TX_BUFFER_SIZE) {
                ble_tx_buffer[ble_tx_len++] = msg.data;
                
                // If newline or buffer full, send immediately
                if (msg.data == '\n' || msg.data == '\r' || 
                    ble_tx_len >= BLE_TX_BUFFER_SIZE) {
                    
                    BleService& ble = BleService::getInstance();
                    if (ble.isConnected()) {
                        int tx_ret = ble.notify(ble_tx_buffer, ble_tx_len);
                        if (tx_ret == 0) {
                            LOG_INF("UART->BLE: %d bytes", ble_tx_len);
                        }
                    }
                    
                    ble_tx_len = 0;
                }
            }
        }
    }
}
