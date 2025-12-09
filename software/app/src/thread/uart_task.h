/*
 * UART Task - Processes UART messages and forwards to BLE
 */

#ifndef UART_TASK_H
#define UART_TASK_H

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

// Message structure for UART data
struct uart_msg {
    uint8_t data;
    uint32_t timestamp;
};

// Message queue for UART -> BLE communication
#define UART_MSGQ_SIZE 32
extern struct k_msgq uart_msgq;

// UART task entry point
void uart_task_entry(void *p1, void *p2, void *p3);

// Task thread definition
#define UART_TASK_STACK_SIZE 2048
#define UART_TASK_PRIORITY 5

K_THREAD_STACK_DECLARE(uart_task_stack, UART_TASK_STACK_SIZE);
extern struct k_thread uart_task_thread;

#ifdef __cplusplus
}
#endif

#endif // UART_TASK_H
