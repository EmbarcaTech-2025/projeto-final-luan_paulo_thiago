#ifndef SD_CARD_H
#define SD_CARD_H

#include "ff.h"
#include "pico/types.h"

// Inicializa a comunicação com o cartão SD.
FRESULT sd_card_init();

// Grava uma string em um arquivo CSV, adicionando uma nova linha.
// A função verifica se o arquivo existe e cria o cabeçalho se necessário.
FRESULT sd_card_append_to_csv(const char* filename, const char* data);

// Obtém e formata a hora atual do RTC em uma string.
void sd_card_get_formatted_timestamp(char* buffer, size_t size);

#endif