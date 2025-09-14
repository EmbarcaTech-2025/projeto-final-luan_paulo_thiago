#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include "pico/stdlib.h"

// Inicializa Wi-Fi
bool wifi_init(void);

// Tenta conectar manualmente ao Wi-Fi
bool wifi_connect(void);

// Atualiza status do Wi-Fi (checa desconexão, reconexão automática, retry)
void wifi_task(void);

// Retorna se está conectado
bool wifi_is_connected(void);

#endif // WIFI_H