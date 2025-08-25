#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Defina qual instância I2C usar (i2c0 ou i2c1)
#define I2C_PORT i2c0

// Pinos do barramento I2C (BitDogLab usa GPIO14 SDA, GPIO15 SCL para i2c1)
#define PIN_SDA 0
#define PIN_SCL 1

int main() {
    stdio_init_all();

    sleep_ms(2000);
    printf("Inicializando conexão i2c");

    // Inicializa I2C a 100 kHz
    i2c_init(I2C_PORT, 100 * 1000);

    // Configura os pinos como função I2C
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);

    // Ativa pull-ups internos (às vezes precisa de resistores externos também)
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    sleep_ms(2000); // tempo para estabilizar antes de começar
    printf("\n=== Scanner I2C - Raspberry Pi Pico W ===\n");

    while (true) {
        printf("Escaneando barramento I2C...\n");

        for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
            uint8_t rxdata;

            //Envia leitura sem dados para verificar resposta do dispositivo
            int ret = i2c_read_timeout_us(I2C_PORT, addr, &rxdata, 1, false, 1000);

            if (ret >= 0) {
                printf("Dispositivo encontrado no endereco 0x%02X\n", addr);
            }
        }

        printf("Escaneamento concluido!\n\n");
        sleep_ms(5000); // espera 5s antes de repetir
    }
}
