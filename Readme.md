# NRF52DK HLKLD2410C

A Zephyr OS project demonstrating UART communication with circular DMA (Direct Memory Access) and LD2410C human presence detection sensor integration on the nRF52 Development Kit.

## Overview

This project showcases:
- **UART Circular DMA Mode**: Efficient serial communication using DMA in circular mode to handle UART data transfers without blocking CPU operations
- **LD2410C Integration**: Integration with the LD2410C millimeter-wave human presence detection sensor
- **Asynchronous UART API**: Non-blocking UART operations using Zephyr's async API
- **Ring Buffer Management**: Efficient circular buffer implementation for data handling

The application reads data from the console and echoes it back, demonstrating reliable UART communication with the nRF52 microcontroller.

## Features

- ✅ Non-blocking UART communication using async API
- ✅ DMA-based circular mode data transfer
- ✅ LD2410C sensor driver integration
- ✅ Ring buffer for efficient data queuing
- ✅ Serial console support
- ✅ Logging and debug output capabilities

## Prerequisites

### Hardware Requirements
- **nRF52 Development Kit** (nRF52DK)
- **LD2410C Sensor Module** (optional, for human presence detection)
- USB cable for programming and serial communication

### Software Requirements
- **Zephyr RTOS**: Latest LTS or development branch
- **CMake**: Version 3.20.0 or higher
- **Nordic nRF5 SDK**: Compatible with your nRF52 board
- **Serial Terminal**: For console interaction (e.g., PuTTY, minicom, or screen)

## Getting Started

### 1. Set Up Zephyr Environment

Follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html) to set up your development environment.

### 2. Clone the Repository

```bash
git clone https://github.com/Faiez-ali/nrf52dk-HLKLD2410C.git
cd nrf52dk-HLKLD2410C