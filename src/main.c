/*
 * main.c
 * Example usage of LD2410C with nRF52832 DK (UART0 console)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "uart.h"
#include "ld2410c.h"

void main(void)
{
    printk("LD2410C Zephyr example start\n");

    // Initialize UART (console UART0 already configured by Zephyr)
    if (!uart_drv_init()) {
        printk("UART init failed!\n");
        return;
    }

    printk("UART ready\n");

    if (!init_ld2410_gpio()) {
        printk("LD2410C GPIO init failed!\n");
        return;
    }
    printk("LD2410C GPIO initialized\n");

    // Enter configuration mode
    // bool ret = ld2410c_enter_config();
    // printk("ld2410c_enter_config returned %d\n", ret);
    // if (!ld2410c_enter_config()) {
    //     printk("Failed to enter config mode\n");
    //     return;
    // }
    // printk("Entered config mode\n");

    // Enable engineering / continuous reporting
    // ret = ld2410c_enable_engineering_mode();
    // printk("ld2410c_enable_engineering_mode returned %d\n", ret);

    if (!ld2410c_enable_engineering_mode()) {
        printk("Failed to enable engineering mode\n");
        // Continue anyway
    } else {
        printk("Engineering mode enabled\n");
    }
    // ld2410c_exit_config();
    // printk("ld2410c_exit_config returned %d\n", ret);
    // Exit configuration mode
    // if (!ld2410c_exit_config()) {
    //     printk("Failed to exit config mode\n");
    //     return;
    // }
    // printk("Exited config mode\n");

    // printk("LD2410C setup complete!\n");

    // // Optionally, enter loop to read sensor data
    while (1) {
        ld2410c_process_uart();
        k_msleep(5);
    }
}
