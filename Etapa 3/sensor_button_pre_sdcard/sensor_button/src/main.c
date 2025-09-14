#include <stdio.h>
#include "pico/stdlib.h"

#include "lib/button.h"
#include "lib/joystick.h"
#include "lib/led.h"
#include "lib/buzzer.h"
#include "lib/wifi.h"
#include "lib/sensor.h"
#include "lib/display.h"

int main() {
    stdio_init_all();
    button_init();
    joystick_init();
    led_init();
    pwm_buzzer_init();
    display_init();

    // Inicializa Wi-Fi
    if (!wifi_init()) {
        return -1;
    }

    // Inicializa sensores
    sensor_init();

    while (1) {
        wifi_task();    // Atualiza status Wi-Fi
        sensor_task();  // Executa lógica dos sensores
        sleep_ms(10);
    }
}