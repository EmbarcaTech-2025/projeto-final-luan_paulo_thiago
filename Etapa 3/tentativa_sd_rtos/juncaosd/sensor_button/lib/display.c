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
    display_clear();  // limpa buffer antes
    ssd1306_draw_string(ssd, x, y, (char *)text);
    render_on_display(ssd, &frame_area);
}

void display_show_data(float temperature, bool wifi_connected) {
    char buffer[32];
    display_clear();  // limpa antes de redesenhar

    snprintf(buffer, sizeof(buffer), "Temp: %.2f C", temperature);
    ssd1306_draw_string(ssd, 0, 16, buffer);

    if (wifi_connected) {
        ssd1306_draw_string(ssd, 0, 32, "WiFi: OK");
    } else {
        ssd1306_draw_string(ssd, 0, 32, "WiFi: OFF");
    }

    render_on_display(ssd, &frame_area);
}

