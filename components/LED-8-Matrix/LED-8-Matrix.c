#include "LED-8-matrix.h"

static const char *TAG = "[LED-8-MATRIX]";

// Track LED states
volatile bool led_states[MATRIX_SIZE][MATRIX_SIZE] = {false};
volatile bool persistent_led_states[MATRIX_SIZE][MATRIX_SIZE] = {false};

// Synchronize LED access
volatile SemaphoreHandle_t led_state_semaphore;

int current_row = 1; // Starting row
int current_col = 1; // Starting column

uint16_t led_patterns[MATRIX_SIZE][MATRIX_SIZE] = {
    {0b1111111000000001, 0b1111111000000010, 0b1111111000000100, 0b1111111000001000,
     0b1111111000010000, 0b1111111000100000, 0b1111111001000000, 0b1111111010000000},
    {0b1111110100000001, 0b1111110100000010, 0b1111110100000100, 0b1111110100001000,
     0b1111110100010000, 0b1111110100100000, 0b1111110101000000, 0b1111110110000000},
    {0b1111101100000001, 0b1111101100000010, 0b1111101100000100, 0b1111101100001000,
     0b1111101100010000, 0b1111101100100000, 0b1111101101000000, 0b1111101110000000},
    {0b1111011100000001, 0b1111011100000010, 0b1111011100000100, 0b1111011100001000,
     0b1111011100010000, 0b1111011100100000, 0b1111011101000000, 0b1111011110000000},
    {0b1110111100000001, 0b1110111100000010, 0b1110111100000100, 0b1110111100001000,
     0b1110111100010000, 0b1110111100100000, 0b1110111101000000, 0b1110111110000000},
    {0b1101111100000001, 0b1101111100000010, 0b1101111100000100, 0b1101111100001000,
     0b1101111100010000, 0b1101111100100000, 0b1101111101000000, 0b1101111110000000},
    {0b1011111100000001, 0b1011111100000010, 0b1011111100000100, 0b1011111100001000,
     0b1011111100010000, 0b1011111100100000, 0b1011111101000000, 0b1011111110000000},
    {0b0111111100000001, 0b0111111100000010, 0b0111111100000100, 0b0111111100001000,
     0b0111111100010000, 0b0111111100100000, 0b0111111101000000, 0b0111111110000000}};

void IRAM_ATTR shiftOut16(uint16_t data)
{
    for (int i = 15; i >= 0; i--)
    {
        gpio_set_level(SER_PIN, (data >> i) & 1);
        gpio_set_level(SRCLK_PIN, 1);
        ets_delay_us(10);
        gpio_set_level(SRCLK_PIN, 0);
    }
    gpio_set_level(RCLK_PIN, 1);
    ets_delay_us(10);
    gpio_set_level(RCLK_PIN, 0);
}

void IRAM_ATTR toggle_current_led()
{
    xSemaphoreTake(led_state_semaphore, portMAX_DELAY);
    persistent_led_states[current_row - 1][current_col - 1] ^= true;
    xSemaphoreGive(led_state_semaphore);
}

void IRAM_ATTR turn_on_led(uint8_t row, uint8_t col)
{
    if (row > 0 && row <= MATRIX_SIZE && col > 0 && col <= MATRIX_SIZE)
    {
        xSemaphoreTake(led_state_semaphore, portMAX_DELAY);
        led_states[row - 1][col - 1] = true;
        xSemaphoreGive(led_state_semaphore);
    }
}

void IRAM_ATTR turn_off_led(uint8_t row, uint8_t col)
{
    if (row > 0 && row <= MATRIX_SIZE && col > 0 && col <= MATRIX_SIZE)
    {
        xSemaphoreTake(led_state_semaphore, portMAX_DELAY);
        if (!persistent_led_states[row - 1][col - 1])
            led_states[row - 1][col - 1] = false;
        xSemaphoreGive(led_state_semaphore);
    }
}

void IRAM_ATTR clear_leds()
{
    xSemaphoreTake(led_state_semaphore, portMAX_DELAY);
    for (uint8_t row = 0; row < MATRIX_SIZE; row++)
    {
        for (uint8_t col = 0; col < MATRIX_SIZE; col++)
        {
            led_states[row][col] = false;
        }
    }
    xSemaphoreGive(led_state_semaphore);
}

void button_press_task(void *pvParameters)
{
    uint8_t btn_press;
    while (1)
    {
        if (xQueueReceive(joystick_button_queue, &btn_press, portMAX_DELAY) == pdTRUE)
        {
            toggle_current_led();
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

void init_starting_led()
{
    clear_leds();
    turn_on_led(current_row, current_col);
}

void update_matrix_display_task(void *pvParameters)
{
    while (1)
    {
        for (uint8_t row = 0; row < MATRIX_SIZE; row++)
        {
            uint16_t combined_pattern = 0;

            xSemaphoreTake(led_state_semaphore, portMAX_DELAY);
            for (uint8_t col = 0; col < MATRIX_SIZE; col++)
            {
                if (led_states[row][col] || persistent_led_states[row][col])
                {
                    combined_pattern |= led_patterns[row][col];
                }
            }
            xSemaphoreGive(led_state_semaphore);

            shiftOut16(combined_pattern);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void move_led(int row_change, int col_change)
{
    turn_off_led(current_row, current_col);

    int new_row = current_row + row_change;
    int new_col = current_col + col_change;

    if (new_row >= TOP_BOUNDARY && new_row <= BOTTOM_BOUNDARY)
        current_row = new_row;
    if (new_col >= LEFT_BOUNDARY && new_col <= RIGHT_BOUNDARY)
        current_col = new_col;

    turn_on_led(current_row, current_col);
}

void get_directions_task(void *pvParameters)
{
    int movement_id = 0;
    while (1)
    {
        if (xQueueReceive(joystick_output_queue, &movement_id, pdMS_TO_TICKS(20)) == pdTRUE)
        {
            ESP_LOGI(TAG, "Joystick moved: %d", movement_id);
            switch (movement_id)
            {
            case 5:
            case 6:
                move_led(1, 0);
                break; // Right (E)
            case 7:
            case 8:
                move_led(-1, 0);
                break; // Left (W)
            case 3:
            case 4:
                move_led(0, -1);
                break; // Up (N)
            case 1:
            case 2:
                move_led(0, 1);
                break; // Down (S)
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void shift_task(void *pvParameter)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << SER_PIN) | (1ULL << RCLK_PIN) | (1ULL << SRCLK_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE};
    gpio_config(&io_conf);

    led_state_semaphore = xSemaphoreCreateMutex();

    xTaskCreate(get_directions_task, "get_directions_task", 4096, NULL, 5, NULL);
    xTaskCreate(button_press_task, "button_press_task", 2048, NULL, 5, NULL);
    xTaskCreate(update_matrix_display_task, "update_matrix_display_task", 4096, NULL, 10, NULL);

    clear_leds();
    init_starting_led();

    vTaskDelete(NULL);
}
