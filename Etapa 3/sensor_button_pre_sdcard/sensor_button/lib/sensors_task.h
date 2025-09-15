#ifndef SENSORS_TASK_H
#define SENSORS_TASK_H

#include "FreeRTOS.h"
#include "task.h"

// Inicialização da task dos sensores
void sensors_task_init(void);

float get_last_temperature(void);

bool buffer_is_sending(void);

void buffer_set_sending(bool sending);


#endif
