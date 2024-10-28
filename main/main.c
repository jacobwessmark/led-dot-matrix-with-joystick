#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "joystick.h"
#include "LED-8-Matrix.h"

void app_main(void)
{
    // Initialize the joystick
    xTaskCreate(joystick_init, "joystick_init", 4096, NULL, 5, NULL);
    // Initialize the LED matrix
    xTaskCreate(shift_task, "shift_task", 4096, NULL, 5, NULL);
}