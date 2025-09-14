#ifndef SENSOR_H
#define SENSOR_H

#include <stdbool.h>

// Inicializa sensores (I2C, display, BMP280, etc.)
void sensor_init(void);

// Executa a lógica dos sensores (chamado dentro do loop principal)
void sensor_task(void);

// Retorna o limite de temperatura configurado
int sensor_get_temp_limit(void);

// Ajusta o limite de temperatura
void sensor_set_temp_limit(int limit);

#endif // SENSOR_H