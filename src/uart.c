#include "uart.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/printk.h>

#define RX_BUF_SIZE   64
#define RING_BUF_SIZE 1024
#define RECEIVE_TIMEOUT 0


// static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(arduino_serial));

RING_BUF_DECLARE(ring_buf, RING_BUF_SIZE);
static uint8_t rx_buffer[RX_BUF_SIZE];

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    ARG_UNUSED(user_data);

    switch (evt->type) {
        case UART_TX_DONE:
            // TX complete, we could send next packet if needed
            break;

        case UART_TX_ABORTED:
            printk("UART TX aborted\n");
            break;

        case UART_RX_RDY:
            // Push received bytes into ring buffer
            ring_buf_put(&ring_buf, evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);
            break;

        case UART_RX_DISABLED:
            // Re-enable RX after buffer full
            uart_rx_enable(uart_dev, rx_buffer, sizeof(rx_buffer), RECEIVE_TIMEOUT);
            break;

        default:
            break;
    }
}

bool uart_drv_init(void)
{
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return false;
    }

    int ret = uart_callback_set(uart_dev, uart_cb, NULL);
    if (ret) {
        printk("uart_callback_set failed (%d)\n", ret);
        return false;
    }

    ret = uart_rx_enable(uart_dev, rx_buffer, sizeof(rx_buffer), RECEIVE_TIMEOUT);
    if (ret) {
        printk("uart_rx_enable failed (%d)\n", ret);
        return false;
    }

    printk("UART async RX enabled\n");
    return true;
}

void uart_send(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uart_poll_out(uart_dev, data[i]);
    }
    // int ret = uart_tx(uart_dev, data, len, SYS_FOREVER_MS);
    // if (ret) {
    //     printk("uart_tx failed (%d)\n", ret);
    // }
}

bool uart_read_byte(uint8_t *byte)
{
    size_t len = 1;
    return ring_buf_get(&ring_buf, byte, len);
}

// size_t uart_read_bytes(uint8_t *buf, size_t max_len)
// {
//     size_t len = max_len;
//     return ring_buf_get(&ring_buf, buf, &len) == 0 ? 0 : len;
// }

void uart_drv_flush(void)
{
    uint8_t dummy;
    while (uart_read_byte(&dummy)) 
    {
        ;
    }
}
