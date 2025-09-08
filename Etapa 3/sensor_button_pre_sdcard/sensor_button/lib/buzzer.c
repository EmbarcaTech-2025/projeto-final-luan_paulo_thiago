#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/types.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "buzzer.h"

void pwm_buzzer_init() { // Essa função configura o pino para função PWM, define a frequência desejada e inicializa o PWM desligado.

    gpio_set_function(BUZZER_A, GPIO_FUNC_PWM); // Define função PWM no pino do buzzer A
    gpio_set_function(BUZZER_B, GPIO_FUNC_PWM); // Define função PWM no pino do buzzer B
    uint slice_num_buzzer_a = pwm_gpio_to_slice_num(BUZZER_A); // Obtém o número do slice PWM
    uint slice_num_buzzer_b = pwm_gpio_to_slice_num(BUZZER_B); // Obtém o número do slice PWM
    uint32_t clock = clock_get_hz(clk_sys); // Frequência do clock do sistema
    uint32_t wrap = clock / BUZZER_FREQ - 1; // Valor de wrap (para controlar frequência)
    pwm_config config = pwm_get_default_config(); // Obtém configuração padrão
    pwm_config_set_wrap(&config, wrap); // Define o wrap calculado
    pwm_init(slice_num_buzzer_a, &config, true); // Inicializa o PWM com config
    pwm_init(slice_num_buzzer_b, &config, true); // Inicializa o PWM com config
    pwm_set_gpio_level(BUZZER_A, 0); // Inicia desligado (nível 0)
    pwm_set_gpio_level(BUZZER_B, 0); // Inicia desligado (nível 0)
}

void buzzer_activation_1(uint buzzer_a, uint buzzer_b, uint32_t duty_cycle_level) { // Função para realizar o acionamento dos buzzers A e B na condição de uma das grandezas medidas 
//estar próxima do valor limite estabelecido.

    pwm_set_gpio_level(buzzer_a, duty_cycle_level);
    pwm_set_gpio_level(buzzer_b, duty_cycle_level);
    
    sleep_ms(600);

    pwm_set_gpio_level(buzzer_a, 0);
    pwm_set_gpio_level(buzzer_b, 0);

    sleep_ms(400);
}

void buzzer_activation_2(uint buzzer_a, uint buzzer_b, uint32_t duty_cycle_level) { // Função para realizar o acionamento dos buzzers A e B na condição de uma das grandezas medidas 
//ter passado do valor limite estabelecido.

    pwm_set_gpio_level(buzzer_a, duty_cycle_level);
    pwm_set_gpio_level(buzzer_b, duty_cycle_level);
    
    sleep_ms(200);

    pwm_set_gpio_level(buzzer_a, 0);
    pwm_set_gpio_level(buzzer_b, 0);

    sleep_ms(100);

    pwm_set_gpio_level(buzzer_a, duty_cycle_level);
    pwm_set_gpio_level(buzzer_b, duty_cycle_level);
    
    sleep_ms(200);

    pwm_set_gpio_level(buzzer_a, 0);
    pwm_set_gpio_level(buzzer_b, 0);

    sleep_ms(700);
}
