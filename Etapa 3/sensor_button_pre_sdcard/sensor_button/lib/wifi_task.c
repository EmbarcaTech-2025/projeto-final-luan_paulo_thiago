#include "wifi_task.h"
#include <stdio.h>
#include "pico/stdlib.h"

#define WIFI_SSID      "POCO X7 Pro"
#define WIFI_PASSWORD  "12345678"

// Status da conexão
bool wifi_connected = false;

// Função auxiliar de conexão
static bool connect_wifi() {
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
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        wifi_connected = false;
        return false;
    }
}

// Task responsável pelo WiFi
static void wifi_task(void *pvParameters) {
    // Configura como station
    cyw43_arch_enable_sta_mode();

    // Tenta primeira conexão
    connect_wifi();

    TickType_t last_retry = xTaskGetTickCount();

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

        // Tentativa manual a cada 60s
        if (!wifi_connected &&
            (xTaskGetTickCount() - last_retry) > pdMS_TO_TICKS(60000)) {
            printf("Tentando reconectar WiFi...\n");
            connect_wifi();
            last_retry = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // verifica a cada 2s
    }
}

// Inicialização da task
void wifi_task_init(void) {
    if (cyw43_arch_init()) {
        printf("Falha na inicializacao do Wi-Fi\n");
        return;
    }

    xTaskCreate(
        wifi_task,        // função
        "WiFiTask",       // nome
        2048,             // stack
        NULL,             // parâmetro
        tskIDLE_PRIORITY + 2, // prioridade
        NULL              // handle
    );
}
