#ifndef SENSORS_TASK_H
#define SENSORS_TASK_H

#include "FreeRTOS.h"
#include "task.h"

extern bool should_send; //compartilha a variável should_send com arquivo sd_card_log_task.c

// Inicialização da task dos sensores
void sensors_task_init(void);

#endif
