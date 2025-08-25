#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

// Defina aqui o pino GPIO que você usará para o botão.
// Certifique-se de que não está em conflito com os pinos I2C (0 e 1).
#define BUTTON_PIN 5

/**
 * @brief Inicializa o pino do botão com um pull-up interno e configura a interrupção.
 */
void button_init(void);

/**
 * @brief Verifica se a leitura de dados está ativa (controlado pelo botão).
 * * @return true se as leituras devem ocorrer, false caso contrário.
 */
bool is_reading_active(void);

#endif // BUTTON_H