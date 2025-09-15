#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/types.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "buzzer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Fila para comandos do buzzer
QueueHandle_t buzzer_queue = NULL;

// Variáveis globais
uint buzzer_a = BUZZER_A;
uint buzzer_b = BUZZER_B;
uint duty_cycle_level = 32768;

void pwm_buzzer_init() {
    gpio_set_function(BUZZER_A, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER_B, GPIO_FUNC_PWM);

    uint slice_num_buzzer_a = pwm_gpio_to_slice_num(BUZZER_A);
    uint slice_num_buzzer_b = pwm_gpio_to_slice_num(BUZZER_B);

    uint32_t clock = clock_get_hz(clk_sys);
    uint32_t wrap = clock / BUZZER_FREQ - 1;

    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, wrap);

    pwm_init(slice_num_buzzer_a, &config, true);
    pwm_init(slice_num_buzzer_b, &config, true);

    pwm_set_gpio_level(BUZZER_A, 0);
    pwm_set_gpio_level(BUZZER_B, 0);
}

void buzzer_activation_1() {
    pwm_set_gpio_level(buzzer_a, duty_cycle_level);
    pwm_set_gpio_level(buzzer_b, duty_cycle_level);
    vTaskDelay(pdMS_TO_TICKS(100));
    pwm_set_gpio_level(buzzer_a, 0);
    pwm_set_gpio_level(buzzer_b, 0);
    vTaskDelay(pdMS_TO_TICKS(5000));
}

// Função para tocar um beep simples
void buzzer_beep(uint32_t duration_ms) {
    pwm_set_gpio_level(buzzer_a, duty_cycle_level);
    pwm_set_gpio_level(buzzer_b, duty_cycle_level);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    pwm_set_gpio_level(buzzer_a, 0);
    pwm_set_gpio_level(buzzer_b, 0);
}

// Task principal do buzzer
static void buzzer_task(void *pvParameters) {
    buzzer_cmd_t cmd;
    
    // Inicializa o PWM do buzzer
    pwm_buzzer_init();
    
    for (;;) {
        // Espera por comandos na fila
        if (xQueueReceive(buzzer_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            switch (cmd.type) {
                case BUZZER_ALERT_1:
                    buzzer_activation_1();
                    break;
                    
                   
                case BUZZER_BEEP:
                    buzzer_beep(cmd.duration_ms);
                    break;
                    
                case BUZZER_OFF:
                    pwm_set_gpio_level(buzzer_a, 0);
                    pwm_set_gpio_level(buzzer_b, 0);
                    break;
                    
                default:
                    break;
            }
        }
    }
}

// Inicializa a task do buzzer
void buzzer_task_init(void) {
    // Cria a fila de comandos
    buzzer_queue = xQueueCreate(10, sizeof(buzzer_cmd_t));
    
    if (buzzer_queue == NULL) {
        printf("Erro ao criar fila do buzzer!\n");
        return;
    }
    
    // Cria a task do buzzer
    xTaskCreate(
        buzzer_task,
        "BuzzerTask",
        512,  // Stack size
        NULL,
        tskIDLE_PRIORITY + 1,  // Prioridade
        NULL
    );
}

// Função para enviar comandos para o buzzer
bool buzzer_send_command(buzzer_cmd_t cmd) {
    if (buzzer_queue == NULL) {
        return false;
    }
    
    return xQueueSend(buzzer_queue, &cmd, 0) == pdTRUE;
}

// Funções de conveniência
bool buzzer_alert_1(void) {
    buzzer_cmd_t cmd = { .type = BUZZER_ALERT_1 };
    return buzzer_send_command(cmd);
}

bool buzzer_alert_2(void) {
    buzzer_cmd_t cmd = { .type = BUZZER_ALERT_2 };
    return buzzer_send_command(cmd);
}

bool buzzer_beep_command(uint32_t duration_ms) {
    buzzer_cmd_t cmd = { .type = BUZZER_BEEP, .duration_ms = duration_ms };
    return buzzer_send_command(cmd);
}

bool buzzer_off(void) {
    buzzer_cmd_t cmd = { .type = BUZZER_OFF };
    return buzzer_send_command(cmd);
}

void buzzer_emergency_stop(void) {
    // Limpa todos os comandos pendentes na fila
    xQueueReset(buzzer_queue);
    
    // Desliga o PWM imediatamente
    pwm_set_gpio_level(buzzer_a, 0);
    pwm_set_gpio_level(buzzer_b, 0);
}