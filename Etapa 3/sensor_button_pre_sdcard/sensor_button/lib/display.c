#include "display.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "include/ssd1306.h"
#include <string.h>
#include <stdio.h>

uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area = {
    .start_column = 0,
    .end_column = ssd1306_width - 1,
    .start_page = 0,
    .end_page = ssd1306_n_pages - 1
};

// Fila global
QueueHandle_t display_queue = NULL;

void display_init() {
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();
    calculate_render_area_buffer_length(&frame_area);
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

void display_clear(void) {
    memset(ssd, 0, ssd1306_buffer_length);
}

void display_show_text(int x, int y, const char *text) {
    display_clear();  
    ssd1306_draw_string(ssd, x, y, (char *)text);
    render_on_display(ssd, &frame_area);
}

void display_show_data(float temperature, float humidity, float pressure, bool wifi_connected) {
    char buffer[32];
    display_clear();

    snprintf(buffer, sizeof(buffer), "T: %.1fC", temperature);
    ssd1306_draw_string(ssd, 0, 0, buffer); // primeira linha

    snprintf(buffer, sizeof(buffer), "H: %.1f%%", humidity);
    ssd1306_draw_string(ssd, 0, 20, buffer); // segunda linha (y+10 pixels)

    snprintf(buffer, sizeof(buffer), "P: %.1fhPa", pressure);
    ssd1306_draw_string(ssd, 0, 35, buffer); // terceira linha (y+20 pixels)

    snprintf(buffer, sizeof(buffer), "WiFi: %s", wifi_connected ? "ON" : "OFF");
    ssd1306_draw_string(ssd, 0, 50, buffer); // quarta linha (y+30 pixels)

    render_on_display(ssd, &frame_area);
}

// Task que consome mensagens e desenha no display
static void display_task(void *pvParameters) {
    display_msg_t msg;
    display_init();

    for (;;) {
        if (xQueueReceive(display_queue, &msg, portMAX_DELAY)) {
            switch (msg.type) {
                case DISPLAY_MSG_TEXT:
                    display_show_text(0, 16, msg.text);
                    break;
                case DISPLAY_MSG_DATA:
                    display_show_data(msg.temperature, msg.humidity, msg.pressure, msg.wifi_connected);
                    break;
            }
        }
    }
}

void display_task_init(void) {
    display_queue = xQueueCreate(5, sizeof(display_msg_t));
    xTaskCreate(
        display_task,
        "DisplayTask",
        2048,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
}
