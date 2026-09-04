#ifndef TESTOLED_SSD1306_H
#define TESTOLED_SSD1306_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/i2c.h"

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 32
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

typedef struct {
    i2c_inst_t *i2c;
    uint8_t address;
    uint8_t buffer[SSD1306_BUFFER_SIZE];
} ssd1306_t;

bool ssd1306_init(ssd1306_t *display, i2c_inst_t *i2c, uint8_t address);
void ssd1306_clear(ssd1306_t *display);
void ssd1306_draw_pixel(ssd1306_t *display, int x, int y, bool on);
void ssd1306_draw_char(ssd1306_t *display, int x, int y, char character,
                       unsigned scale);
void ssd1306_draw_char_color(ssd1306_t *display, int x, int y,
                             char character, unsigned scale, bool on);
void ssd1306_draw_text(ssd1306_t *display, int x, int y, const char *text,
                       unsigned scale);
void ssd1306_draw_text_color(ssd1306_t *display, int x, int y,
                             const char *text, unsigned scale, bool on);
int ssd1306_text_width(const char *text, unsigned scale);
bool ssd1306_show(ssd1306_t *display);

#endif
