#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"

#include "aht10.h"
#include "bmp280.h"
#include "button.h" // 1. INCLUA O NOVO HEADER

int main() {
    stdio_init_all();
    button_init(); // 2. INICIALIZE O BOTÃO
    
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // A Pico W LED inicialmente fica desligada
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    printf("Hello, Sensors!\n");
    printf("Pressione o botao no pino GPIO %d para iniciar/parar as leituras.\n", BUTTON_PIN);


    // I2C is "open drain", pull ups to keep signal high when no data is being sent
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

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
        // 3. VERIFICA O ESTADO ANTES DE LER OS DADOS
        if (is_reading_active()) {
            // Liga o LED para indicar que está lendo
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            
            // --- Leitura do AHT10 ---
            temperature1 = GetTemperature();
            humidity= GetHumidity();
            dew = GetDewPoint();
            printf("-----------------\n");
            printf("Hum.  = %.2f %%\n", humidity);
            printf("Temp. = %.2f C (AHT10)\n", temperature1);
            printf("Dew   = %.2f C\n", dew);
            sleep_ms(2000);
            
            // --- Leitura do BMP280 ---
            bmp280_read_raw(&raw_temperature, &raw_pressure);
            int32_t temperature2 = bmp280_convert_temp(raw_temperature, &params);
            int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);
            printf("Press. = %.3f kPa\n", pressure / 1000.f);
            printf("Temp.  = %.2f C (BMP280)\n", temperature2 / 100.f);
            printf("-----------------\n\n");
            sleep_ms(2000);

        } else {
            // Desliga o LED para indicar que está pausado
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // Pequeno delay para não sobrecarregar a CPU enquanto espera
            sleep_ms(100);
        }
    }
}