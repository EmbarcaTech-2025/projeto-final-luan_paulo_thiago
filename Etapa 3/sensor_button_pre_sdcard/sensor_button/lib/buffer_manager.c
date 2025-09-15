#include "buffer_manager.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "hardware/rtc.h" // Adicionar include

static temp_record_t temperature_buffer[BUFFER_SIZE];

static int buffer_index = 0;
static bool buffer_initialized = false;
static absolute_time_t last_save_time;
static bool is_sending = false; // ← Nova variável para controle

void buffer_init(void) {
    if (!buffer_initialized) {
        memset(temperature_buffer, 0, sizeof(temperature_buffer));
        buffer_index = 0;
        buffer_initialized = true;
        is_sending = false;
        last_save_time = get_absolute_time();
        printf("Buffer inicializado\n");
    }
}

bool buffer_is_sending(void) {
    return is_sending;
}

void buffer_set_sending(bool sending) {
    is_sending = sending;
    printf("Modo envio: %s\n", sending ? "ATIVO" : "INATIVO");
}

bool buffer_should_save(void) {
    // Não salva se estiver no modo de envio
    if (is_sending) {
        return false;
    }
    
    // Verifica se passou 1 minuto desde o último salvamento
    return absolute_time_diff_us(last_save_time, get_absolute_time()) >= 60000000;
}

bool buffer_add_record(float temperature) {
    if (is_sending) {
        printf("Modo envio ativo - coleta pausada\n");
        return false;
    }
    
    if (buffer_index >= BUFFER_SIZE) {
        printf("Buffer cheio!\n");
        return false;
    }
    
    // --- PONTO CHAVE: CAPTURA A DATA E HORA ATUAIS DO RTC ---
    rtc_get_datetime(&temperature_buffer[buffer_index].timestamp);
    // ----------------------------------------------------

    temperature_buffer[buffer_index].temperature = temperature;
    
    buffer_index++;
    last_save_time = get_absolute_time();
    
    // Log para verificação
    datetime_t dt = temperature_buffer[buffer_index - 1].timestamp;
    printf("Dado salvo no buffer: %.2f C em %02d/%02d/%04d %02d:%02d:%02d (total: %d)\n", 
           temperature, dt.day, dt.month, dt.year, dt.hour - 3, dt.min, dt.sec, buffer_index);
           
    return true;
}

bool buffer_save_to_sd(void) {
    // Implementação para salvar no SD card
    printf("Salvando %d registros no SD...\n", buffer_index);
    buffer_index = 0;
    return true;
}

bool buffer_is_full(void) {
    return buffer_index >= BUFFER_SIZE;
}

int buffer_get_count(void) {
    return buffer_index;
}

temp_record_t* buffer_get_records(void) {
    return temperature_buffer;
}

void buffer_clear(void) {
    buffer_index = 0;
    printf("Buffer limpo\n");
}

