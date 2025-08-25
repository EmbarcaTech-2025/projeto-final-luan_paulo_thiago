#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"

#include "sd_card.h"
#include "ff.h"

#include "aht10.h"
#include "bmp280.h"
#include "button.h" // 1. INCLUA O NOVO HEADER
#include "display.h"

// sd card

FRESULT fr; // variavel sd card
FATFS fs; // variavel sd card
FIL fil; // variavel sd card
int ret; // variavel sd card
char buf[100]; // buffer sd card

char filename[] = "vacina.txt"; // nome arquivo sd card

void monta_sd_card(){
    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
    }
}

void abre_sd_card_escrita(){
    // Open file for writing ()
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
    }
}

void inicializa_sd_card(){

    // Initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
    }
    monta_sd_card();
    abre_sd_card_escrita();
}

void abre_sd_card_leitura(){
    // Open file for reading
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
    }
}

void fecha_arquivo_sd_card(){
    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
    }

}

void le_arquivo_sd_card(){
    // Print every line in file over serial
    printf("Reading from file '%s':\r\n", filename);
    printf("---\r\n");
    while (f_gets(buf, sizeof(buf), &fil)) {
        printf(buf);
    }
    printf("\r\n---\r\n");
}

void desmonta_sd_card(){
    // Unmount drive
    f_unmount("0:");
}

void imprime_sd_serial(){
    abre_sd_card_leitura();
    le_arquivo_sd_card();
    fecha_arquivo_sd_card();
}

void encerra_sd_card(){
    fecha_arquivo_sd_card();
    desmonta_sd_card();
}

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
    display_init();

    bmp280_init();
    struct bmp280_calib_param params;
    bmp280_get_calib_params(&params);

    //inicializa_sd_card();

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
            //ret = f_printf(&fil, "Hum.  = %.2f %%\r\n", humidity); //escreve sd
            printf("Temp. = %.2f C (AHT10)\n", temperature1);
            //ret = f_printf(&fil, "Temp. = %.2f C (AHT10)\r\n", temperature1	); //escreve sd
            printf("Dew   = %.2f C\n", dew);
            //ret = f_printf(&fil, "Dew   = %.2f C\r\n", dew); //escreve sd
            sleep_ms(2000);
            
            // --- Leitura do BMP280 ---
            bmp280_read_raw(&raw_temperature, &raw_pressure);
            int32_t temperature2 = bmp280_convert_temp(raw_temperature, &params);
            int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);
            printf("Press. = %.3f kPa\n", pressure / 1000.f);
            printf("Temp.  = %.2f C (BMP280)\n", temperature2 / 100.f);
            printf("-----------------\n\n");
            sleep_ms(2000);
            display_show_data(temperature1, humidity, pressure);

        } else {
            // Desliga o LED para indicar que está pausado

            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // Pequeno delay para não sobrecarregar a CPU enquanto espera
            sleep_ms(100);
        }
    }
}