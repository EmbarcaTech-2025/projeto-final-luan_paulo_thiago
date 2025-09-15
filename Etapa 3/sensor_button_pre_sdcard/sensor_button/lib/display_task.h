#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdbool.h>

// Estrutura de dados enviada para o display
typedef struct {
    float temperature;
    bool wifi_status;
    bool reading_active;
    bool setting_mode;
    int temp_limit;
} display_data_t;

extern QueueHandle_t display_queue;

void display_task_init(void);

#endif
