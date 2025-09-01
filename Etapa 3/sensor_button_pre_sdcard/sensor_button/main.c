#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"

#include "aht10.h"
#include "bmp280.h"
#include "button.h"
#include "display.h"
#include "joystick.h"
#include "led.h"

int temp_limit = 25;        // limite inicial
bool setting_mode = false;  // se está no modo ajuste

int main() {
    stdio_init_all();
    button_init();
    joystick_init();
    led_init();

    // Inicializa Wi-Fi/LED (BitDogLab usa o LED da Pico W)
    if (cyw43_arch_init()) {
        printf("Inicializacao do Wi-Fi falhou\n");
        return -1;
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    printf("Ola, Sensores!\n");
    printf("Pressione o botao no pino GPIO %d para iniciar/parar as leituras.\n", BUTTON_PIN);

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

    sleep_ms(250);

    while (1) {
        // --- Verificar botão C (JOYSTICK SW) apenas se as leituras não estiverem ativas ---
        if (!is_reading_active() && gpio_get(JOY_SW) == 0) {
            sleep_ms(200); // debounce
            setting_mode = !setting_mode;
            printf("Modo config: %s\n", setting_mode ? "ON" : "OFF");
        }

        // --- Joystick ativo apenas no modo config e se as leituras não estiverem ativas ---
        joystick_update_limit(&temp_limit, setting_mode && !is_reading_active());

        if (is_reading_active()) {

            // Sensores
            temperature1 = GetTemperature();
            humidity = GetHumidity();
            dew = GetDewPoint();

            bmp280_read_raw(&raw_temperature, &raw_pressure);
            int32_t temperature2 = bmp280_convert_temp(raw_temperature, &params);

            float temperature = ((temperature2 / 100.f) + temperature1) / 2.0f;

            // Verifica limite
            if (temperature > temp_limit) {
                gpio_put(LED_R, 1);
                gpio_put(LED_G, 0);
                gpio_put(LED_B, 0);
            } 
            else if(temperature > temp_limit - 5){
                gpio_put(LED_R, 1);
                gpio_put(LED_G, 1);
                gpio_put(LED_B, 0);
            }
            else {
                gpio_put(LED_R, 0);
                gpio_put(LED_G, 1);
                gpio_put(LED_B, 0);
            }
            display_show_data(temperature);

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
        
        // Pequena pausa para evitar uso excessivo da CPU
        sleep_ms(10);
    }
}