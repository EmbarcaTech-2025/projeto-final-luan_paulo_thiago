#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stddef.h>
#include "pico/types.h"
#include "FreeRTOS.h"
#include "queue.h"

#define BUZZER_A 21 // Pino do Buzzer A
#define BUZZER_B 10 // Pino do Buzzer B
#define BUZZER_FREQ 2000 // Hz

// Tipos de comando para o buzzer
typedef enum {
    BUZZER_ALERT_1,     // Alerta nível 1 (próximo do limite)
    BUZZER_ALERT_2,     // Alerta nível 2 (acima do limite)
    BUZZER_BEEP,        // Beep simples customizado
    BUZZER_OFF          // Desliga o buzzer
} buzzer_cmd_type_t;

// Estrutura de comando para o buzzer
typedef struct {
    buzzer_cmd_type_t type;
    uint32_t duration_ms; // Usado apenas para BUZZER_BEEP
} buzzer_cmd_t;

extern uint buzzer_a;
extern uint buzzer_b;
extern uint duty_cycle_level;
extern QueueHandle_t buzzer_queue;

// Funções de inicialização
void pwm_buzzer_init(void);
void buzzer_task_init(void);

// Funções de comando (thread-safe)
bool buzzer_send_command(buzzer_cmd_t cmd);
bool buzzer_alert_1(void);
bool buzzer_alert_2(void);
bool buzzer_beep_command(uint32_t duration_ms);
bool buzzer_off(void);

// Funções internas (usadas pela task)
void buzzer_activation_1(void);
void buzzer_activation_2(void);
void buzzer_beep(uint32_t duration_ms);
void buzzer_emergency_stop(void);

#endif