#include "sensors_task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "lib/aht10.h"
#include "lib/bmp280.h"
#include "lib/button.h"
#include "lib/display.h"
#include "lib/joystick.h"
#include "lib/led.h"
#include "lib/thingspeak.h"
#include "lib/buzzer.h"

#include "lib/wifi_task.h" // para acessar wifi_connected

// Variáveis globais
static int temp_limit = 25;
static bool setting_mode = false;
static absolute_time_t last_thingspeak_send;

// Task principal
static void sensors_task(void *pvParameters) {
    // Inicializa I2C + display OLED
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    display_init();

    // Inicializa BMP280
    bmp280_init();
    struct bmp280_calib_param params;
    bmp280_get_calib_params(&params);

    int32_t raw_temperature;
    int32_t raw_pressure;

    float temperature1;
    float humidity;
    float dew;

    vTaskDelay(pdMS_TO_TICKS(250));

    for (;;) {
        // --- Botão para entrar no modo config ---
        if (!is_reading_active() && gpio_get(JOY_SW) == 0) {
            vTaskDelay(pdMS_TO_TICKS(200)); // debounce
            setting_mode = !setting_mode;
            printf("Modo config: %s\n", setting_mode ? "ON" : "OFF");
        }

        // Atualiza limite via joystick
        joystick_update_limit(&temp_limit, setting_mode && !is_reading_active());

        if (is_reading_active()) {
            // Sensores
            temperature1 = GetTemperature();
            humidity = GetHumidity();
            dew = GetDewPoint();

            bmp280_read_raw(&raw_temperature, &raw_pressure);
            int32_t temperature2 = bmp280_convert_temp(raw_temperature, &params);

            float temperature = ((temperature2 / 100.f) + temperature1) / 2.0f;

            // --- Controle de LEDs e envio de dados ---
            bool should_send = false;

            if (temperature > temp_limit) {
                // Crítico - vermelho
                gpio_put(LED_R, 1);
                gpio_put(LED_G, 0);
                gpio_put(LED_B, 0);
                should_send = true;
           
            } else if (temperature > temp_limit - 5) {
                // Alerta - amarelo
                gpio_put(LED_R, 1);
                gpio_put(LED_G, 1);
                gpio_put(LED_B, 0);
                should_send = true;
               
            } else {
                // Normal - verde
                gpio_put(LED_R, 0);
                gpio_put(LED_G, 1);
                gpio_put(LED_B, 0);
                should_send = false;
            }

            // --- Envio ao ThingSpeak ---
            if (should_send && wifi_connected &&
                absolute_time_diff_us(last_thingspeak_send, get_absolute_time()) > 30000000) {
                if (thingspeak_queue) {
                xQueueSend(thingspeak_queue, &temperature, 0);
}

                last_thingspeak_send = get_absolute_time();
            }

            display_show_data(temperature, wifi_connected);

        } else {
            // Leituras desativadas → LED apagado
            gpio_put(LED_R, 0);
            gpio_put(LED_G, 0);
            gpio_put(LED_B, 0);

            if (setting_mode) {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "Set Limit: %dC", temp_limit);
                display_show_text(0, 32, buffer);
            } else {
                display_show_text(0, 32, "Leitura OFF");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // reduz uso de CPU
    }
}

// Inicialização da task
void sensors_task_init(void) {
    xTaskCreate(
        sensors_task,
        "SensorsTask",
        4096,             // stack
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
}
