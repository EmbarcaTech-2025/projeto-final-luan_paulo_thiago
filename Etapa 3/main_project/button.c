#include "button.h"
#include "hardware/gpio.h"
#include "pico/time.h"

// Variável de estado para controlar a leitura.
// 'volatile' é crucial porque esta variável é modificada por uma interrupção
// e lida no loop principal. Isso impede que o compilador otimize o acesso a ela.
static volatile bool reading_active = false;

// Constante para o tempo de debounce em milissegundos
#define DEBOUNCE_MS 250

// Armazena o tempo do último clique para o debounce
static volatile uint32_t last_press_time = 0;

/**
 * @brief Função de callback da interrupção. Chamada quando o botão é pressionado.
 * * @param gpio O pino que gerou a interrupção.
 * @param events O tipo de evento (ex: borda de subida/descida).
 */
void button_callback(uint gpio, uint32_t events) {
    // Pega o tempo atual
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Lógica de Debounce:
    // Se o tempo decorrido desde o último clique for maior que o nosso delay,
    // processa o clique.
    if (current_time - last_press_time > DEBOUNCE_MS) {
        last_press_time = current_time;  // Atualiza o tempo do último clique
        reading_active = !reading_active; // Inverte o estado (liga/desliga)
    }
}

/**
 * @brief Inicializa o pino do botão e a interrupção.
 */
void button_init(void) {
    // Configura o pino do botão
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Usa o resistor de pull-up interno

    // Configura a interrupção para ser acionada na borda de descida (quando o botão é pressionado e aterra o pino)
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);
}

/**
 * @brief Retorna o estado atual da leitura.
 */
bool is_reading_active(void) {
    return reading_active;
}