#include "led.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>

void led_init(){

    gpio_init(LED_R); gpio_set_dir(LED_R, GPIO_OUT); gpio_put(LED_R, 0);
    gpio_init(LED_G); gpio_set_dir(LED_G, GPIO_OUT); gpio_put(LED_G, 0);
    gpio_init(LED_B); gpio_set_dir(LED_B, GPIO_OUT); gpio_put(LED_B, 0);

}