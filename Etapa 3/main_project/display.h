#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/stdlib.h"
#include "include/ssd1306.h"

#define I2C_SDA 14
#define I2C_SCL 15

extern uint8_t ssd[ssd1306_buffer_length];
extern struct render_area frame_area;

void display_init(void);
void display_clear(void);
void display_show_text(int x, int y, const char *text);
void display_show_data(float temperature, float humidity, float pressure);

#endif
