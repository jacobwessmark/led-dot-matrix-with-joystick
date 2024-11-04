#include "joystick.h"

static const char *TAG = "[JOYSTICK]";
static volatile bool button_pressed = false;
static uint32_t last_button_time = 0;

QueueHandle_t joystick_output_queue;
QueueHandle_t joystick_button_queue;

joystick_movement_t joystick_movements[] = {
    {0, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, "Idle"},
    {1, JOYSTICK_FAST_E_MIN, JOYSTICK_FAST_E_MAX, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, "Fast E"},
    {2, JOYSTICK_SLOW_E_MIN, JOYSTICK_SLOW_E_MAX, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, "Slow E"},
    {3, JOYSTICK_FAST_W_MIN, JOYSTICK_FAST_W_MAX, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, "Fast W"},
    {4, JOYSTICK_SLOW_W_MIN, JOYSTICK_SLOW_W_MAX, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, "Slow W"},
    {5, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, JOYSTICK_FAST_N_MIN, JOYSTICK_FAST_N_MAX, "Fast N"},
    {6, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, JOYSTICK_SLOW_N_MIN, JOYSTICK_SLOW_N_MAX, "Slow N"},
    {7, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, JOYSTICK_FAST_S_MIN, JOYSTICK_FAST_S_MAX, "Fast S"},
    {8, JOYSTICK_IDLE_MIN, JOYSTICK_IDLE_MAX, JOYSTICK_SLOW_S_MIN, JOYSTICK_SLOW_S_MAX, "Slow S"},
};

bool is_joystick_idle(int32_t x_data, int32_t y_data)
{
    return (x_data >= JOYSTICK_IDLE_MIN && x_data <= JOYSTICK_IDLE_MAX) &&
           (y_data >= JOYSTICK_IDLE_MIN && y_data <= JOYSTICK_IDLE_MAX);
}

static bool on_conv_done(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    uint8_t adc_read_buf[ADC_CONV_FRAME_SIZE];
    uint32_t out_size = 0;

    if (adc_continuous_read(handle, adc_read_buf, sizeof(adc_read_buf), &out_size, 0) == ESP_OK)
    {
        int32_t adc_1_data = 0, adc_2_data = 0;
        static int movement_id = 0;
        const int FAST_INTERVAL_MS = 200, SLOW_INTERVAL_MS = 800;
        static int64_t last_event_time_fast = 0, last_event_time_slow = 0;
        int64_t current_time = esp_timer_get_time() / 1000;

        for (int i = 0; i < out_size; i += SOC_ADC_DIGI_RESULT_BYTES)
        {
            adc_digi_output_data_t *data = (adc_digi_output_data_t *)&adc_read_buf[i];
            if (data->type2.channel == ADC_1)
            {
                adc_1_data = data->type2.data;
            }
            else if (data->type2.channel == ADC_2)
            {
                adc_2_data = data->type2.data;
            }
        }

        // Check for joystick movements
        bool movement_detected = false;
        int movement_interval = SLOW_INTERVAL_MS;

        for (int i = 0; i < sizeof(joystick_movements) / sizeof(joystick_movements[0]); i++)
        {
            joystick_movement_t movement = joystick_movements[i];
            if ((adc_1_data >= movement.x_min_threshold && adc_1_data <= movement.x_max_threshold) &&
                (adc_2_data >= movement.y_min_threshold && adc_2_data <= movement.y_max_threshold))
            {
                movement_detected = true;
                movement_id = movement.movement_id;
                movement_interval = (movement_id % 2 == 1) ? FAST_INTERVAL_MS : SLOW_INTERVAL_MS;
                break;
            }
        }

        if (movement_detected)
        {
            if ((movement_interval == FAST_INTERVAL_MS && current_time - last_event_time_fast >= FAST_INTERVAL_MS) ||
                (movement_interval == SLOW_INTERVAL_MS && current_time - last_event_time_slow >= SLOW_INTERVAL_MS))
            {
                if (movement_id != 0)
                {
                    xQueueSendFromISR(joystick_output_queue, &movement_id, NULL);
                }
                if (movement_interval == FAST_INTERVAL_MS)
                {
                    last_event_time_fast = current_time;
                }
                else
                {
                    last_event_time_slow = current_time;
                }
            }
        }

        // Handle button event if applicable
        if (button_pressed && current_time - last_event_time_slow >= DEBOUNCE_DELAY)
        {
            uint8_t btn_press = 1;
            ESP_LOGI(TAG, "Button Pressed");
            xQueueSendFromISR(joystick_button_queue, &btn_press, NULL);
            last_event_time_slow = current_time;
        }

        return pdFALSE; // ISR already handles context switch if needed
    }
    return false;
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    int level = gpio_get_level((int)arg);
    uint32_t current_time = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;

    if (current_time - last_button_time >= DEBOUNCE_DELAY)
    {
        button_pressed = (level == 0);
        last_button_time = current_time;
    }
}

void init_button_interrupt()
{
    gpio_install_isr_service(0);
    gpio_isr_handler_add(SW_PIN, gpio_isr_handler, (void *)SW_PIN);
}

void joystick_init(void *pvParameter)
{
    joystick_output_queue = xQueueCreate(1, sizeof(int));
    joystick_button_queue = xQueueCreate(1, sizeof(uint8_t));
    if (joystick_output_queue == NULL || joystick_button_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE};
    gpio_config(&io_conf);
    init_button_interrupt();

    adc_continuous_handle_t adc_handle = NULL;
    adc_continuous_handle_cfg_t adc_handle_config = {
        .max_store_buf_size = ADC_READ_BUF_SIZE,
        .conv_frame_size = ADC_CONV_FRAME_SIZE,
    };

    if (adc_continuous_new_handle(&adc_handle_config, &adc_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create ADC handle");
        return;
    }

    adc_digi_pattern_config_t adc_patterns[2] = {
        {.atten = ADC_ATTEN, .bit_width = ADC_WIDTH, .channel = ADC_1, .unit = ADC_UNIT},
        {.atten = ADC_ATTEN, .bit_width = ADC_WIDTH, .channel = ADC_2, .unit = ADC_UNIT}};

    adc_continuous_config_t continuous_config = {
        .pattern_num = 2,
        .adc_pattern = adc_patterns,
        .sample_freq_hz = 1000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    if (adc_continuous_config(adc_handle, &continuous_config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure ADC");
        return;
    }

    adc_continuous_evt_cbs_t adc_callbacks = {.on_conv_done = on_conv_done};
    if (adc_continuous_register_event_callbacks(adc_handle, &adc_callbacks, NULL) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register ADC callbacks");
        return;
    }

    adc_continuous_start(adc_handle);
    vTaskDelete(NULL);
}
