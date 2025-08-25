#include "joystick.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include "pico/time.h"

static bool setting_mode_active = false;

void joystick_init(void) {
    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);

    gpio_init(JOY_SW);
    gpio_set_dir(JOY_SW, GPIO_IN);
    gpio_pull_up(JOY_SW);
}

void joystick_update_limit(int *temp_limit, bool active) {
    setting_mode_active = active;
    
    if (!active) return;  // só altera se estivermos no "modo config"

    static absolute_time_t last_update = {0};
    absolute_time_t now = get_absolute_time();
    
    // Limitar a taxa de atualização para 5Hz (200ms)
    if (absolute_time_diff_us(last_update, now) < 200000) {
        return;
    }

    adc_select_input(0); // eixo X
    uint16_t joy_x_raw = adc_read();

    if (joy_x_raw > 3500) {   // direita → aumenta
        (*temp_limit)++;
        printf("Limite aumentado para: %dC\n", *temp_limit);
        last_update = now;
    } else if (joy_x_raw < 1000) { // esquerda → diminui
        (*temp_limit)--;
        printf("Limite diminuido para: %dC\n", *temp_limit);
        last_update = now;
    }

    if (*temp_limit < 0) *temp_limit = 0;
    if (*temp_limit > 100) *temp_limit = 100;
}

bool is_setting_mode_active(void) {
    return setting_mode_active;
}