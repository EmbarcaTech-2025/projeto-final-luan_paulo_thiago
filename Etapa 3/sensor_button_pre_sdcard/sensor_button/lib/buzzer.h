#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include "pico/types.h"

// Pinos dos buzzers
#define BUZZER_A 21 
#define BUZZER_B 10 

#define BUZZER_FREQ 5000 // Hz

// Inicialização
void pwm_buzzer_init(void);

// Acionamentos básicos
void buzzer_activation_1(uint buzzer_a, uint buzzer_b, uint32_t duty_cycle_level);
void buzzer_activation_2(uint buzzer_a, uint buzzer_b, uint32_t duty_cycle_level);

// Atualiza buzzer conforme limite e temperatura
void buzzer_update_status(float temp, int limit);

#endif // BUZZER_H
