#include "sd_card_log_task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/sd_card.h"
#include "lib/sensors_task.h"

#define SD_CARD_LOG_PERIOD_MS 1000 // A cada 1 segundos

void sd_card_log_task(void *params) {
    (void)params;
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    // Inicializa o cartão SD
    if (sd_card_init() != FR_OK) {
        printf("Erro ao inicializar o cartao SD!\n");
    } else {
        printf("Cartao SD inicializado com sucesso!\n");
    }

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SD_CARD_LOG_PERIOD_MS));


        if (should_send) {               
            char log_line[256];
            char timestamp_buffer[32];
            sd_card_get_formatted_timestamp(timestamp_buffer, sizeof(timestamp_buffer));

            /*

            // Formata os dados em uma linha de texto CSV
            sprintf(log_line, "%s,%.2f,%.2f,%.2f,%.1f\n",
                timestamp_buffer,
                data.vrms,
                data.irms,
                data.v_pu,
                data.p_instant);

            */

            // Grava os dados no arquivo
            FRESULT fr = sd_card_append_to_csv("dados.csv", log_line);
            if (fr != FR_OK) {
                printf("Erro ao gravar no SD: %d\n", fr);
            } else {
                printf("Dados gravados no SD: %s", log_line);
            }
        }
    }
}

void sd_card_log_task_init(void) {

xTaskCreate(
        sd_card_log_task,
        "SDCardLogTask",
        4096,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL);

}