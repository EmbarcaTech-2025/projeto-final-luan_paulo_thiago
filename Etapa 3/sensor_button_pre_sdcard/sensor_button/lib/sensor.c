#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "sensor.h"
#include "aht10.h"
#include "bmp280.h"
#include "button.h"
#include "joystick.h"
#include "led.h"
#include "buzzer.h"
#include "display.h"
#include "thingspeak.h"
#include "wifi.h"

// Variáveis internas
static int temp_limit = 25;
static bool setting_mode = false;
static absolute_time_t last_thingspeak_send;
static struct bmp280_calib_param bmp_params;

// Inicialização dos sensores
void sensor_init(void) {
    // Inicializa I2C
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    bmp280_init();
    bmp280_get_calib_params(&bmp_params);

    last_thingspeak_send = get_absolute_time();
    sleep_ms(250);
}

// Loop dos sensores
void sensor_task(void) {
    // Botão SW → ativa modo config
    if (!is_reading_active() && gpio_get(JOY_SW) == 0) {
        sleep_ms(200); // debounce
        setting_mode = !setting_mode;
        printf("Modo config: %s\n", setting_mode ? "ON" : "OFF");
    }

    // Joystick altera limite só em modo config
    joystick_update_limit(&temp_limit, setting_mode && !is_reading_active());

    if (is_reading_active()) {
        // Leituras
        float t1 = GetTemperature();
        float h = GetHumidity();
        float d = GetDewPoint();

        int32_t raw_t, raw_p;
        bmp280_read_raw(&raw_t, &raw_p);
        int32_t t2 = bmp280_convert_temp(raw_t, &bmp_params);

        float temperature = ((t2 / 100.f) + t1) / 2.0f;

        // Atualiza LED + Buzzer
        bool alert = led_update_status(temperature, temp_limit);
        buzzer_update_status(temperature, temp_limit);

        // Envio ao ThingSpeak
        if (alert && wifi_is_connected() &&
            absolute_time_diff_us(last_thingspeak_send, get_absolute_time()) > 30000000) {
            thingspeak_send(temperature);
            last_thingspeak_send = get_absolute_time();
        }

        // Atualiza Display
        display_show_data(temperature, wifi_is_connected());

    } else {
        led_off();

        if (setting_mode) {
            display_show_limit(temp_limit);
        } else {
            display_show_off();
        }
    }
}

// Getters e setters
int sensor_get_temp_limit(void) {
    return temp_limit;
}

void sensor_set_temp_limit(int limit) {
    temp_limit = limit;
}