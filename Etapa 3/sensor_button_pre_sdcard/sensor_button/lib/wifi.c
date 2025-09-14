#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "wifi.h"

// Credenciais
#define WIFI_SSID     "POCO X7 Pro"
#define WIFI_PASSWORD "12345678"

// Estado do Wi-Fi
static bool wifi_connected = false;
static absolute_time_t last_wifi_retry;

// Conectar ao Wi-Fi
bool wifi_connect(void) {
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

// Inicializar Wi-Fi
bool wifi_init(void) {
    if (cyw43_arch_init()) {
        printf("Falha na inicializacao do Wi-Fi\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();

    // Primeira tentativa
    wifi_connect();
    last_wifi_retry = get_absolute_time();
    return true;
}

// Atualizar estado Wi-Fi
void wifi_task(void) {
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

    // Retry manual a cada 1 min
    if (!wifi_connected &&
        absolute_time_diff_us(last_wifi_retry, get_absolute_time()) > 60000000) {
        printf("Tentando reconectar WiFi...\n");
        wifi_connect();
        last_wifi_retry = get_absolute_time();
    }
}

// Getter
bool wifi_is_connected(void) {
    return wifi_connected;
}