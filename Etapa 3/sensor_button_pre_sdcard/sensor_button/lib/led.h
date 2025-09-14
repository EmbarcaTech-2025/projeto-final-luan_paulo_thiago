#ifndef LED_H
#define LED_H

#include <stdbool.h>

// Definições de pinos
#define LED_G 11
#define LED_B 12  
#define LED_R 13  

// Inicialização
void led_init(void);

// Controle direto
void led_set(int r, int g, int b);
void led_off(void);

// Atualiza cor do LED conforme temperatura
// Retorna true se deve enviar dados (alerta ou crítico)
bool led_update_status(float temp, int limit);

#endif // LED_H
