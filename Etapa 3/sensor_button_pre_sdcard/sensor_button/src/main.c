#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "lib/button.h"
#include "lib/joystick.h"
#include "lib/led.h"
#include "lib/buzzer.h"

#include "lib/wifi_task.h"
#include "lib/sensors_task.h"
#include "lib/thingspeak.h"

int main() {
    stdio_init_all();
    button_init();
    joystick_init();
    led_init();

    printf("Ola, Sensores!\n");

    wifi_task_init();
    sensors_task_init();
    thingspeak_task_init();  

    vTaskStartScheduler();

    while (1) {
    }
}

