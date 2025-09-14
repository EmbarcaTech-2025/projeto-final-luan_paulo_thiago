#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"

// Variáveis globais de status
extern bool wifi_connected;

// Inicialização da task WiFi
void wifi_task_init(void);

#endif
