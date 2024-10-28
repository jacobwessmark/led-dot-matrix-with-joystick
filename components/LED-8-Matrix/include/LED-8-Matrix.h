#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "joystick.h"
#include "esp32s3/rom/ets_sys.h"
#include "freertos/semphr.h"

// Pin definitions for the shift registers
#define SER_PIN GPIO_NUM_41   // Serial Data Input
#define RCLK_PIN GPIO_NUM_39  // Register Clock (Latch)
#define SRCLK_PIN GPIO_NUM_42 // Shift Register Clock


// Matrix size and boundary definitions
#define MATRIX_SIZE 8
#define TOP_BOUNDARY 1
#define BOTTOM_BOUNDARY MATRIX_SIZE
#define LEFT_BOUNDARY 1
#define RIGHT_BOUNDARY MATRIX_SIZE

// Function declarations
void shift_task(void *pvParameter);
