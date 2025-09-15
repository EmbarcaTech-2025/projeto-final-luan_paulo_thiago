#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lib/button.h"
#include "lib/joystick.h"
#include "lib/led.h"
#include "lib/buzzer.h"
#include "lib/display.h"
#include "lib/buffer_manager.h"
#include "lib/thingspeak.h"

#include "lib/wifi_task.h"
#include "lib/sensors_task.h"

#include "lib/logger.h"
#include "lib/rtc_ntp.h"

static bool wifi_was_connected = false;
static absolute_time_t last_buffer_save;

// Task para gerenciar o buffer
// No buffer_manager_task:
static void buffer_manager_task(void *pvParameters) {
    buffer_init();
    last_buffer_save = get_absolute_time();
    
    for (;;) {
        // Verifica mudanças no status do WiFi
        if (!wifi_connected && wifi_was_connected) {
            printf("WiFi desconectado - Iniciando salvamento em buffer\n");
            buffer_set_sending(false);
            last_buffer_save = get_absolute_time();
        }
        
        if (wifi_connected && !wifi_was_connected) {
            printf("WiFi reconectado - Preparando para enviar dados acumulados\n");
            buffer_set_sending(true);
            
            int record_count = buffer_get_count();
            if (record_count > 0) {
                printf("Enviando %d registros do buffer para ThingSpeak...\n", record_count);
                
                temp_record_t* records = buffer_get_records();
                thingspeak_send_batch(records, record_count);
                
                buffer_clear();
                buffer_set_sending(false);
                
                printf("Todos os registros do buffer foram enviados\n");
            } else {
                printf("Nenhum dado no buffer para enviar\n");
                buffer_set_sending(false);
            }
        }
        
        // Salva dados apenas quando: WiFi offline + Leitura ativa + Não está enviando
        if (!wifi_connected && !buffer_is_sending() && is_reading_active()) {
            uint64_t time_diff = absolute_time_diff_us(last_buffer_save, get_absolute_time());
            if (time_diff >= 180000000) {
                float current_temp = get_last_temperature();
                if (current_temp != 0.0f) {
                    buffer_add_record(current_temp);
                    last_buffer_save = get_absolute_time();
                    printf("Dado salvo no buffer (leitura ON): %.2f°C\n", current_temp);
                }
            }
        }
        
        wifi_was_connected = wifi_connected;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    stdio_init_all();
    
    // INICIALIZA LOGGER E RTC
    logger_init();
    rtc_ntp_init(); // Inicializa o RTC

    button_init();
    joystick_init();
    led_init();
    buzzer_task_init();

    printf("Sistema de Monitoramento com Buffer\n");

    // Inicializa tasks
    wifi_task_init();
    sensors_task_init();
    display_task_init();
    thingspeak_task_init();
    
    // Cria task do buffer manager
    xTaskCreate(
        buffer_manager_task,
        "BufferManager",
        2048,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    vTaskStartScheduler();

    while (1) {
        // Não deve chegar aqui
    }
}