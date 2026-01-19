/*
 * ld2410c.h
 * Header for LD2410C sensor commands
 */

#ifndef LD2410C_H
#define LD2410C_H

#include <stdbool.h>
#include <stdint.h>


typedef struct {
    bool presence;
    uint8_t  status;
    uint8_t  moving_signal;
    uint8_t  static_signal;
    uint16_t moving_dist_cm;
    uint16_t static_dist_cm;
    uint16_t distance_cm;
    uint32_t timestamp_ms;
} ld2410_data_t;

typedef enum {
    STREAM_IDLE,
    STREAM_LEN_L,
    STREAM_LEN_H,
    STREAM_PAYLOAD,
    STREAM_TAIL_0,
    STREAM_TAIL_1,
    STREAM_TAIL_2,
    STREAM_TAIL_3
} stream_state_t;

// Initialize GPIO for LD2410C for motion detection output
bool init_ld2410_gpio(void);

// Check if motion is detected
bool ld2410_motion_detected(void);

// Enter configuration mode
bool ld2410c_enter_config(void);

// Enable engineering / continuous reporting
bool ld2410c_enable_engineering_mode(void);

// Exit configuration mode
bool ld2410c_exit_config(void);

void ld2410c_process_uart(void);

bool ld2410c_get_data(ld2410_data_t *out);


#endif // LD2410C_H
