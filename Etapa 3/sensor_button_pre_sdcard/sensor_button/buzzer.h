#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stddef.h>
#include "pico/types.h"

#define BUZZER_A 21 // Pino do Buzzer A
#define BUZZER_B 10 // Pino do Buzzer B

#define BUZZER_FREQ 5000 // Hz

void pwm_buzzer_init(); // Função para configurar o pino para função PWM, define a frequência desejada e inicializa o PWM 
//desligado.

void buzzer_activation_1(uint buzzer_a, uint buzzer_b, uint32_t duty_cycle_level); // Função para realizar o acionamento dos buzzers A e B na condição de uma das grandezas medidas 
//estar próxima do valor limite estabelecido.

void buzzer_activation_2(uint buzzer_a, uint buzzer_b, uint32_t duty_cycle_level); // Função para realizar o acionamento dos buzzers A e B na condição de uma das grandezas medidas 
//ter passado do valor limite estabelecido.

#endif