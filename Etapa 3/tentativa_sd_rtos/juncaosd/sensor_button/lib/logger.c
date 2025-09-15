/**
 * @file logger.c
 * @brief Logger simples com timestamp e mutex opcional (FreeRTOS).
 * @details
 *  Encapsula `printf` com marcação de tempo (RTC) e proteção por mutex.
 */

#include "lib/logger.h"
#include <stdio.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "FreeRTOS.h"
#include "semphr.h"

static SemaphoreHandle_t s_mutex = NULL;

/**
 * @brief Indica se o escalonador do FreeRTOS já foi iniciado.
 * @return Diferente de zero se iniciado; zero caso contrário.
 */
static inline int scheduler_started(void)
{
    return xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED;
}

/**
 * @brief Formata timestamp (data/hora + milissegundo) no buffer fornecido.
 * @param[out] buf Buffer de saída.
 * @param buflen Tamanho do buffer.
 */
static void format_timestamp(char *buf, size_t buflen)
{
    datetime_t dt;
    rtc_get_datetime(&dt);
    uint32_t msec = (uint32_t)((time_us_64() / 1000ULL) % 1000ULL);

    snprintf(buf, buflen, "%04d/%02d/%02d %02d:%02d:%02d.%03u",
             dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec, msec);
}

/**
 * @brief Inicializa o logger, criando o mutex de proteção.
 */
void logger_init(void)
{
    if (!s_mutex)
    {
        s_mutex = xSemaphoreCreateMutex();
    }
}

/**
 * @brief Escreve uma linha de log formatada com timestamp e tag.
 * @param tag Cadeia de identificação do módulo (ou NULL).
 * @param fmt Formato `printf` seguido dos respectivos argumentos variádicos.
 */
void logger_log(const char *tag, const char *fmt, ...)
{
    char ts[32];
    format_timestamp(ts, sizeof(ts));

    int locked = 0;
    if (s_mutex && scheduler_started())
    {
        if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            locked = 1;
        }
    }

    printf("%s %s: ", ts, tag ? tag : "LOG");
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");

    if (locked)
    {
        xSemaphoreGive(s_mutex);
    }
}
