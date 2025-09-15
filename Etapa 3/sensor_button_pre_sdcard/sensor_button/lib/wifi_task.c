#include "wifi_task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/rtc_ntp.h"

#define WIFI_SSID               "POCO X7 Pro"
#define WIFI_PASSWORD           "12345678"

// --- Parâmetros para a lógica de reconexão ---
#define WIFI_CHECK_INTERVAL_MS      2000  // Intervalo da verificação de status: 2 segundos
#define WIFI_RETRY_BACKOFF_MIN_MS   30000  // Tempo mínimo de espera para reconectar: 30 segundos
#define WIFI_CONNECT_TIMEOUT_MS     10000 // Tempo máximo para a tentativa de conexão: 10 segundos

// Status da conexão (variável global)
bool wifi_connected = false;

// Função auxiliar de conexão
static bool connect_wifi() {
    printf("Tentando conectar ao Wi-Fi '%s'...\n", WIFI_SSID);

    // Tenta conectar com um timeout definido
    if (cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID, WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS) == 0) {
        
        printf("Wi-Fi conectado com sucesso!\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        wifi_connected = true;

        // Sincroniza o relógio via NTP após conectar
        printf("Sincronizando relógio via NTP...\n");
        if (rtc_ntp_sync("pool.ntp.org", 5000)) {
            printf("Relógio sincronizado com sucesso!\n");
        } else {
            printf("Falha ao sincronizar o relógio.\n");
        }
        return true;

    } else {
        printf("Falha na tentativa de conexão Wi-Fi.\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        wifi_connected = false;
        return false;
    }
}

// Task responsável pelo gerenciamento do Wi-Fi
static void wifi_task(void *pvParameters) {
    cyw43_arch_enable_sta_mode();

    uint32_t backoff_ms = WIFI_RETRY_BACKOFF_MIN_MS;
    TickType_t last_retry_ticks = 0;

    for (;;) {
        // A função cyw43_tcpip_link_status é mais completa que a cyw43_wifi_link_status
        int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

        if (link_status == CYW43_LINK_UP) {
            // Se a conexão está OK
            if (!wifi_connected) {
                printf("Wi-Fi estabelecido (link UP).\n");
                wifi_connected = true;
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                backoff_ms = WIFI_RETRY_BACKOFF_MIN_MS; // Reseta o backoff para a próxima vez
            }
        } else {
            // Se a conexão caiu ou não foi estabelecida
            if (wifi_connected) {
                printf("Wi-Fi desconectado! (status: %d)\n", link_status);
                wifi_connected = false;
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                last_retry_ticks = xTaskGetTickCount(); // Inicia o timer para a primeira tentativa
            }

            // Verifica se já é hora de tentar reconectar
            if ((xTaskGetTickCount() - last_retry_ticks) >= pdMS_TO_TICKS(backoff_ms)) {
                if (connect_wifi()) {
                    // Se conectou, a própria função já ajusta o estado
                }
                last_retry_ticks = xTaskGetTickCount(); // Atualiza o tempo da última tentativa
            }
        }

        vTaskDelay(pdMS_TO_TICKS(WIFI_CHECK_INTERVAL_MS));
    }
}

// Inicialização da task
void wifi_task_init(void) {
    if (cyw43_arch_init()) {
        printf("Falha na inicializacao do Wi-Fi\n");
        return;
    }

    xTaskCreate(
        wifi_task,
        "WiFiTask",
        2048,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
}