#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/stdlib.h"
#include "include/ssd1306.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdbool.h>

#define I2C_SDA 14
#define I2C_SCL 15

extern uint8_t ssd[ssd1306_buffer_length];
extern struct render_area frame_area;

// Estrutura de mensagem para a fila
typedef enum {
    DISPLAY_MSG_TEXT,
    DISPLAY_MSG_DATA
} display_msg_type_t;

typedef struct {
    display_msg_type_t type;
    char text[32];
    float temperature;
    bool wifi_connected;
} display_msg_t;

extern QueueHandle_t display_queue;

void display_init(void);
void display_clear(void);
void display_show_text(int x, int y, const char *text);
void display_show_data(float temperature, bool wifi_connected);

// Inicializa task do display
void display_task_init(void);

#endif
