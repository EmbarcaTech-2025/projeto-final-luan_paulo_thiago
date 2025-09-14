#include "led.h"
#include "pico/stdlib.h"

void led_init(){
    gpio_init(LED_R); gpio_set_dir(LED_R, GPIO_OUT); gpio_put(LED_R, 0);
    gpio_init(LED_G); gpio_set_dir(LED_G, GPIO_OUT); gpio_put(LED_G, 0);
    gpio_init(LED_B); gpio_set_dir(LED_B, GPIO_OUT); gpio_put(LED_B, 0);
}

void led_set(int r, int g, int b) {
    gpio_put(LED_R, r);
    gpio_put(LED_G, g);
    gpio_put(LED_B, b);
}

void led_off() {
    led_set(0,0,0);
}

// Atualiza LED conforme temperatura e retorna se deve enviar dados
bool led_update_status(float temp, int limit) {
    if (temp > limit) {
        led_set(1,0,0);   // Vermelho
        return true;
    } else if (temp > limit - 5) {
        led_set(1,1,0);   // Amarelo
        return true;
    } else {
        led_set(0,1,0);   // Verde
        return false;
    }
}
