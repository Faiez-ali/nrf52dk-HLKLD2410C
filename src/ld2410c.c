/*
 * ld2410c.c
 * Zephyr driver for LD2410C
 * Implements enter/exit config and enable engineering mode
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include "uart.h"
#include "ld2410c.h"

#define CONFIG_DELAY_MS 100 // Delay after entering config mode
#define ACK_TIMEOUT_MS 300  // Timeout for waiting ACK (ms)

#define LD2410_NODE DT_NODELABEL(ld2410_out)

static const struct gpio_dt_spec ld2410_out = GPIO_DT_SPEC_GET(DT_CHILD(LD2410_NODE, ld2410_motion), gpios);

static const uint8_t headConfig[4] = {0xFD, 0xFC, 0xFB, 0xFA};
static const uint8_t tailConfig[4] = {0x04, 0x03, 0x02, 0x01};

/* ---------------- Constants ---------------- */

#define LD2410_MAX_PAYLOAD 256

#define HDR0 0xF4
#define HDR1 0xF3
#define HDR2 0xF2
#define HDR3 0xF1

#define TAIL0 0xF8
#define TAIL1 0xF7
#define TAIL2 0xF6
#define TAIL3 0xF5

/* ---------------- State ---------------- */


static uint8_t  header_win[4];
static uint8_t  frame_buf[LD2410_MAX_PAYLOAD];
static uint16_t expected_len;
static uint16_t collected;
static stream_state_t state = STREAM_IDLE;


static struct k_mutex data_mutex;
static ld2410_data_t latest;
// -----------------------------
// Internal helpers
// -----------------------------

static void parse_data_frame(const uint8_t *payload, uint16_t len)
{
    if (len < 11) {
        return;
    }

    if (!((payload[0] == 0x01 || payload[0] == 0x02) && payload[1] == 0xAA)) {
        return;
    }

    ld2410_data_t tmp;

    tmp.timestamp_ms        = k_uptime_get();
    tmp.status              = payload[2];
    tmp.moving_dist_cm      = payload[3] | (payload[4] << 8);
    tmp.moving_signal       = payload[5];
    tmp.static_dist_cm      = payload[6] | (payload[7] << 8);
    tmp.static_signal       = payload[8];
    tmp.distance_cm         = payload[9] | (payload[10] << 8);
    tmp.presence            = ld2410_motion_detected();

    k_mutex_lock(&data_mutex, K_MSEC(50));
    latest = tmp;
    k_mutex_unlock(&data_mutex);

        // --- Print parsed data ---
    printk("\n--- LD2410C Frame ---\n");
    printk("Timestamp(ms)      : %u\n", tmp.timestamp_ms);
    printk("Status             : 0x%02X\n", tmp.status);
    printk("Moving Distance(cm): %u\n", tmp.moving_dist_cm);
    printk("Moving Signal      : %u\n", tmp.moving_signal);
    printk("Static Distance(cm): %u\n", tmp.static_dist_cm);
    printk("Static Signal      : %u\n", tmp.static_signal);
    printk("Distance(cm)       : %u\n", tmp.distance_cm);
    printk("Presence Detected  : %s\n", tmp.presence ? "Yes" : "No");

}

/* ---------------- Byte-wise stream handler ---------------- */

static void handle_rx_byte(uint8_t b)
{
    /* shift header window */
    header_win[0] = header_win[1];
    header_win[1] = header_win[2];
    header_win[2] = header_win[3];
    header_win[3] = b;

    if (header_win[0] == HDR0 &&
        header_win[1] == HDR1 &&
        header_win[2] == HDR2 &&
        header_win[3] == HDR3) {

        state        = STREAM_LEN_L;
        expected_len = 0;
        collected    = 0;
        return;
    }

    switch (state) {
    case STREAM_IDLE:
        break;

    case STREAM_LEN_L:
        expected_len = b;
        state = STREAM_LEN_H;
        break;

    case STREAM_LEN_H:
        expected_len |= ((uint16_t)b << 8);
        if (expected_len == 0 || expected_len > LD2410_MAX_PAYLOAD) {
            state = STREAM_IDLE;
        } else {
            collected = 0;
            state = STREAM_PAYLOAD;
        }
        break;

    case STREAM_PAYLOAD:
        frame_buf[collected++] = b;
        if (collected >= expected_len) {
            state = STREAM_TAIL_0;
        }
        break;

    case STREAM_TAIL_0:
        state = (b == TAIL0) ? STREAM_TAIL_1 : STREAM_IDLE;
        break;

    case STREAM_TAIL_1:
        state = (b == TAIL1) ? STREAM_TAIL_2 : STREAM_IDLE;
        break;

    case STREAM_TAIL_2:
        state = (b == TAIL2) ? STREAM_TAIL_3 : STREAM_IDLE;
        break;

    case STREAM_TAIL_3:
        if (b == TAIL3) {
            parse_data_frame(frame_buf, expected_len);
        }
        state = STREAM_IDLE;
        break;
    }
}

/* ---------------- Public API ---------------- */

void ld2410c_process_uart(void)
{
    uint8_t byte;

    while (uart_read_byte(&byte)) {
        handle_rx_byte(byte);
    }
}

bool ld2410c_get_data(ld2410_data_t *out)
{
    if (!out) return false;

    k_mutex_lock(&data_mutex, K_MSEC(20));
    *out = latest;
    k_mutex_unlock(&data_mutex);

    return true;
}

/**
 * Wait for ACK from LD2410C
 * Returns true if ACK received within timeout
 */
static bool wait_for_ack(void)
{
    uint64_t deadline = k_uptime_get() + ACK_TIMEOUT_MS;
    uint8_t byte;

    while (k_uptime_get() < deadline)
    {
        printk("UART bytes read %d\n", uart_read_byte(&byte));
        if (uart_read_byte(&byte) > 0)
        {
            // if (byte == 0x00 || byte == 0x01 || byte == 0xAA)
            // {
            //     // LD2410 ACK patterns vary by firmware
                return true;
            // }
        }
        k_msleep(10);
    }

    return false;
}

static bool send_raw_command(const uint8_t *cmd, size_t len)
{
    // send header
    uart_send(headConfig, sizeof(headConfig));

    // send command body
    uart_send(cmd, len);

    // send tail
    uart_send(tailConfig, sizeof(tailConfig));

    // allow TX to flush
    k_msleep(10);

    return true;
}


/**
 * Send a “simple command” frame to LD2410C
 * cmd: 16-bit command
 */
static bool send_simple_command(uint16_t commandLE)
{
    uint8_t cmd[4];

    // little endian encoding
    cmd[0] = (uint8_t)(commandLE & 0xFF);
    cmd[1] = (uint8_t)((commandLE >> 8) & 0xFF);
    cmd[2] = 0x00;
    cmd[3] = 0x00;

    // uart_drv_flush(); // clear RX buffer before sending
    send_raw_command(cmd, sizeof(cmd));

    return wait_for_ack();
}

/**
 * Retry a simple command multiple times
 */
static bool retry_cmd(uint16_t cmd, int attempts, int delay_ms)
{
    for (int i = 0; i < attempts; i++)
    {
        // send_simple_command(cmd);
        
        if (send_simple_command(cmd))
        {
            return true;
        }
        k_msleep(delay_ms);

    }

    return false;
}

// -----------------------------
// Public API
// -----------------------------

/**
 * Enter configuration mode (0x01FF)
 */
bool ld2410c_enter_config(void)
{
    bool ok = retry_cmd(0x01FF, 3, 100);
    if (ok)
        k_msleep(CONFIG_DELAY_MS); // delay after config mode entry

    return ok;
}

/**
 * Enable engineering / continuous reporting
 * Tries common codes with retries
 */
bool ld2410c_enable_engineering_mode(void)
{
    if (retry_cmd(0x0061, 3, 150))
    {
        return true;
    }
    // if (retry_cmd(0x0060, 3, 150))
    //     return true;
    // if (retry_cmd(0x0062, 3, 150))
    //     return true;

    return false;
}

/**
 * Exit configuration mode (0x01FE)
 */
bool ld2410c_exit_config(void)
{
    return retry_cmd(0x00FE, 3, 100);
}


bool init_ld2410_gpio(void)
{
	if (!device_is_ready(ld2410_out.port)) {
		printk("LD2410 GPIO device not ready\n");
		return false;
	}

	int ret = gpio_pin_configure_dt(&ld2410_out, GPIO_INPUT);
	if (ret) {
		printk("Failed to configure LD2410 GPIO (err %d)\n", ret);
		return false;
	}

	printk("LD2410 GPIO initialized on %s pin %d\n",
	       ld2410_out.port->name,
	       ld2410_out.pin);

	return true;
}

bool ld2410_motion_detected(void)
{
	return gpio_pin_get_dt(&ld2410_out);
}