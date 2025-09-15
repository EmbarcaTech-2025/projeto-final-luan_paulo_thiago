#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "hardware/rtc.h" // Necessário para datetime_t

#define BUFFER_SIZE 50 

typedef struct {
    float temperature;
    datetime_t timestamp; // MUDANÇA: de um simples número para a estrutura de data/hora
} temp_record_t;

void buffer_init(void);
bool buffer_add_record(float temperature);
void buffer_clear(void);
int buffer_get_count(void);
temp_record_t* buffer_get_records(void);
bool buffer_is_sending(void);
void buffer_set_sending(bool sending);


#endif