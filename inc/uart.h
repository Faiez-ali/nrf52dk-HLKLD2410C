#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

bool uart_drv_init(void);              // Initialize UART driver
void uart_send(const uint8_t *data, size_t len); // Send data over UART
bool uart_read_byte(uint8_t *byte);             // Read a single byte from ring buffer
// size_t uart_read_bytes(uint8_t *buf, size_t max_len);
void uart_drv_flush(void);

#endif // UART_H
