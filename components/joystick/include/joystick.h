#pragma once

#include <stdio.h>
#include "driver/adc.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_timer.h"

// Pin definitions
#define SW_PIN GPIO_NUM_15

// ADC settings for joystick input
#define ADC_1 ADC1_CHANNEL_1       // GPIO2
#define ADC_2 ADC1_CHANNEL_2       // GPIO3
#define ADC_ATTEN ADC_ATTEN_DB_12  // 12 dB attenuation
#define ADC_WIDTH ADC_WIDTH_BIT_12 // 12-bit resolution
#define ADC_UNIT ADC_UNIT_1        // ADC unit 1

// Buffer settings
#define ADC_READ_BUF_SIZE 1024
#define ADC_CONV_FRAME_SIZE 128

// Debounce delay for button
#define DEBOUNCE_DELAY 50 // Milliseconds

// Joystick calibration
#define JOYSTICK_MAX 4095
#define JOYSTICK_MIN 0
#define JOYSTICK_IDLE_MIN 1820
#define JOYSTICK_IDLE_MAX 2120
#define JOYSTICK_IDLE_CENTER ((JOYSTICK_IDLE_MIN + JOYSTICK_IDLE_MAX) / 2)

// Speed percentages for slow and fast zones
#define SLOW_PERCENTAGE 0.6f
#define FAST_PERCENTAGE 0.99f

// Direction thresholds for joystick movements
#define JOYSTICK_SLOW_S_MIN (JOYSTICK_IDLE_MAX)
#define JOYSTICK_SLOW_S_MAX (JOYSTICK_IDLE_CENTER + SLOW_PERCENTAGE * (JOYSTICK_MAX - JOYSTICK_IDLE_CENTER))
#define JOYSTICK_FAST_S_MIN (JOYSTICK_SLOW_S_MAX)
#define JOYSTICK_FAST_S_MAX (JOYSTICK_MAX)

#define JOYSTICK_SLOW_N_MIN (JOYSTICK_IDLE_CENTER - SLOW_PERCENTAGE * (JOYSTICK_IDLE_CENTER - JOYSTICK_MIN))
#define JOYSTICK_SLOW_N_MAX (JOYSTICK_IDLE_MIN)
#define JOYSTICK_FAST_N_MIN (JOYSTICK_MIN)
#define JOYSTICK_FAST_N_MAX (JOYSTICK_SLOW_N_MIN)

#define JOYSTICK_SLOW_E_MIN (JOYSTICK_IDLE_MAX)
#define JOYSTICK_SLOW_E_MAX (JOYSTICK_IDLE_CENTER + SLOW_PERCENTAGE * (JOYSTICK_MAX - JOYSTICK_IDLE_CENTER))
#define JOYSTICK_FAST_E_MIN (JOYSTICK_SLOW_E_MAX)
#define JOYSTICK_FAST_E_MAX (JOYSTICK_MAX)

#define JOYSTICK_SLOW_W_MIN (JOYSTICK_IDLE_CENTER - SLOW_PERCENTAGE * (JOYSTICK_IDLE_CENTER - JOYSTICK_MIN))
#define JOYSTICK_SLOW_W_MAX (JOYSTICK_IDLE_MIN)
#define JOYSTICK_FAST_W_MIN (JOYSTICK_MIN)
#define JOYSTICK_FAST_W_MAX (JOYSTICK_SLOW_W_MIN)

// Event types and data structure
typedef enum
{
    JOYSTICK_EVENT,
    BUTTON_EVENT
} event_type_t;

typedef struct
{
    event_type_t type;
    int32_t x_data;
    int32_t y_data;
    bool button_state;
} input_event_t;

typedef struct
{
    int movement_id;
    int x_min_threshold;
    int x_max_threshold;
    int y_min_threshold;
    int y_max_threshold;
    const char *movement_name;
} joystick_movement_t;

// Queue handles for movement and button press events
extern QueueHandle_t joystick_output_queue;
extern QueueHandle_t joystick_button_queue;

// Function declarations
void joystick_init(void *pvParameter);