#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"

#include "lib/aht10.h"
#include "lib/bmp280.h"
#include "lib/button.h"
#include "lib/display.h"
#include "lib/joystick.h"
#include "lib/led.h"
#include "lib/thingspeak.h"
#include "lib/buzzer.h"

#include "ff.h"
#include "hardware/spi.h"


#define WIFI_SSID "POCO X7 Pro"
#define WIFI_PASSWORD "12345678"

int temp_limit = 25;         // limite inicial
bool setting_mode = false;   // se está no modo ajuste
bool wifi_connected = false; // status da conexão WiFi
absolute_time_t last_thingspeak_send; // tempo do último envio
absolute_time_t last_wifi_retry;      // tempo da última tentativa WiFi

// Função para conectar ao WiFi com retry
bool connect_wifi() {
    printf("Tentando conectar ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID, WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK, 5000) == 0) {
        printf("Conectado ao Wi-Fi!\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        wifi_connected = true;
        return true;
    } else {
        printf("Falha na conexão Wi-Fi\n");
        wifi_connected = false;
        return false;
    }
}

int main() {
    stdio_init_all();
    button_init();
    joystick_init();
    led_init();
    pwm_buzzer_init();

    // Inicializa Wi-Fi
    if (cyw43_arch_init()) {
        printf("Falha na inicializacao do Wi-Fi\n");
        return -1;
    }

    // Configura como station (cliente)
    cyw43_arch_enable_sta_mode();

    // Tenta conectar ao WiFi pela primeira vez
    connect_wifi();
    last_wifi_retry = get_absolute_time();

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
        
    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
        if (link_status < 0 || link_status == CYW43_LINK_DOWN) {
            if (wifi_connected) {
                printf("WiFi desconectado!\n");
                wifi_connected = false;
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        }
    } else {
        if (!wifi_connected) {
            printf("WiFi reconectado automaticamente.\n");
            wifi_connected = true;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        }
    }

    // Tentar reconexão manual a cada 1 minuto, se desconectado
    if (!wifi_connected &&
        absolute_time_diff_us(last_wifi_retry, get_absolute_time()) > 60000000) {
        printf("Tentando reconectar WiFi...\n");
        connect_wifi();
        last_wifi_retry = get_absolute_time();
    }

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

            // --- Controle de LEDs e envio de dados ---
            bool should_send = false;

            if (temperature > temp_limit) {
                // Estado crítico - vermelho
                gpio_put(LED_R, 1);
                gpio_put(LED_G, 0);
                gpio_put(LED_B, 0);
                should_send = true;
                buzzer_activation_2(BUZZER_A, BUZZER_B, 7812);

            } else if (temperature > temp_limit - 5) {
                // Estado de alerta - amarelo
                gpio_put(LED_R, 1);
                gpio_put(LED_G, 1);
                gpio_put(LED_B, 0);
                should_send = true;
                buzzer_activation_1(BUZZER_A, BUZZER_B, 7812);

            } else {
                // Estado normal - verde
                gpio_put(LED_R, 0);
                gpio_put(LED_G, 1);
                gpio_put(LED_B, 0);
                should_send = false; // não envia
            }

            // --- Envio ao ThingSpeak somente em alerta/crítico ---
            if (should_send && wifi_connected &&
                absolute_time_diff_us(last_thingspeak_send, get_absolute_time()) > 30000000) {
                thingspeak_send(temperature);
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

        // Pequena pausa para evitar uso excessivo da CPU
        sleep_ms(10);
    }
}
