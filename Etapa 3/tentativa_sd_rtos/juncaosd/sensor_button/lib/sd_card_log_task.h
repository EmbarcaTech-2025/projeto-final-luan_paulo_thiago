#ifndef SD_CARD_LOG_TASK_H
#define SD_CARD_LOG_TASK_H

#include "FreeRTOS.h"
#include "task.h"

void sd_card_log_task(void *params);
void sd_card_log_task_init(void);

#endif