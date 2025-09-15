#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <stdbool.h>
#include <time.h>

#define BUFFER_SIZE 1440

typedef struct {
    float temperature;
    uint32_t timestamp;
} temp_record_t;

void buffer_init(void);
bool buffer_add_record(float temperature);
bool buffer_save_to_sd(void);
bool buffer_is_full(void);
int buffer_get_count(void);
temp_record_t* buffer_get_records(void);
void buffer_clear(void);
bool buffer_should_save(void);
bool buffer_is_sending(void); // ← Nova função
void buffer_set_sending(bool sending); // ← Nova função

#endif