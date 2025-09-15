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
#include "lib/sd_card_log_task.h"
#include "credentials.h"
#include "lib/wifi_manager.h"
#include "lib/logger.h"
#include "lib/rtc_ntp.h"

int main() {
    stdio_init_all();
    button_init();
    joystick_init();
    led_init();

    printf("Ola, Sensores!\n");

    //wifi_task_init();
    wifi_manager_init_t();
    sensors_task_init();
    thingspeak_task_init();

    sd_card_log_task_init();

    vTaskStartScheduler();

    while (1) {
    }
}

