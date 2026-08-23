#include "ssd1306.h"

#include <string.h>

#include "pico/stdlib.h"

static bool send_command(ssd1306_t *display, uint8_t command) {
    const uint8_t packet[2] = {0x00, command};
    return i2c_write_blocking(display->i2c, display->address,
                              packet, sizeof(packet), false) == sizeof(packet);
}

static bool send_commands(ssd1306_t *display,
                          const uint8_t *commands, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (!send_command(display, commands[i])) {
            return false;
        }
    }
    return true;
}

bool ssd1306_init(ssd1306_t *display, i2c_inst_t *i2c, uint8_t address) {
    display->i2c = i2c;
    display->address = address;
    ssd1306_clear(display);

    /* SSD1306 initialization for a 128x32 panel using its charge pump. */
    static const uint8_t init_sequence[] = {
        0xAE,       /* display off */
        0xD5, 0x80, /* clock divider */
        0xA8, 0x1F, /* multiplex ratio: 32 rows */
        0xD3, 0x00, /* display offset */
        0x40,       /* start line 0 */
        0x8D, 0x14, /* charge pump on */
        0x20, 0x00, /* horizontal addressing mode */
        0xA1,       /* segment remap */
        0xC8,       /* COM scan direction remapped */
        0xDA, 0x02, /* sequential COM pins for 128x32 */
        0x81, 0x8F, /* contrast */
        0xD9, 0xF1, /* pre-charge period */
        0xDB, 0x40, /* VCOMH deselect level */
        0xA4,       /* display follows RAM */
        0xA6,       /* normal, not inverted */
        0x2E,       /* scrolling off */
        0xAF        /* display on */
    };

    sleep_ms(20);
    if (!send_commands(display, init_sequence, sizeof(init_sequence))) {
        return false;
    }

    return ssd1306_show(display);
}

void ssd1306_clear(ssd1306_t *display) {
    memset(display->buffer, 0, sizeof(display->buffer));
}

void ssd1306_draw_pixel(ssd1306_t *display, int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) {
        return;
    }

    const size_t index = (size_t)x + ((size_t)y / 8u) * SSD1306_WIDTH;
    const uint8_t mask = (uint8_t)(1u << ((unsigned)y & 7u));

    if (on) {
        display->buffer[index] |= mask;
    } else {
        display->buffer[index] &= (uint8_t)~mask;
    }
}

/* Return one 5-pixel-wide column at a time for uppercase 5x7 glyphs. */
static uint8_t glyph_column(char character, unsigned column) {
    if (column >= 5) {
        return 0;
    }

    if (character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }

    static const uint8_t digits[10][5] = {
        {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}
    };
    static const uint8_t letters[26][5] = {
        {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
        {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
        {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
        {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
        {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
        {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
        {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
        {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
        {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
        {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
        {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
        {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
        {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
    };

    if (character >= '0' && character <= '9') {
        return digits[(unsigned)(character - '0')][column];
    }
    if (character >= 'A' && character <= 'Z') {
        return letters[(unsigned)(character - 'A')][column];
    }

    switch (character) {
        case '.': { static const uint8_t g[5] = {0,0x60,0x60,0,0}; return g[column]; }
        case '-': { static const uint8_t g[5] = {8,8,8,8,8}; return g[column]; }
        case '_': { static const uint8_t g[5] = {0x40,0x40,0x40,0x40,0x40}; return g[column]; }
        case ':': { static const uint8_t g[5] = {0,0x36,0x36,0,0}; return g[column]; }
        default: return 0;
    }
}

void ssd1306_draw_char_color(ssd1306_t *display, int x, int y,
                             char character, unsigned scale, bool on) {
    if (scale == 0) {
        return;
    }

    for (unsigned column = 0; column < 5; ++column) {
        const uint8_t bits = glyph_column(character, column);
        for (unsigned row = 0; row < 7; ++row) {
            if ((bits & (1u << row)) == 0) {
                continue;
            }
            for (unsigned dx = 0; dx < scale; ++dx) {
                for (unsigned dy = 0; dy < scale; ++dy) {
                    ssd1306_draw_pixel(display,
                        x + (int)(column * scale + dx),
                        y + (int)(row * scale + dy), on);
                }
            }
        }
    }
}

void ssd1306_draw_char(ssd1306_t *display, int x, int y, char character,
                       unsigned scale) {
    ssd1306_draw_char_color(display, x, y, character, scale, true);
}

void ssd1306_draw_text_color(ssd1306_t *display, int x, int y,
                             const char *text, unsigned scale, bool on) {
    while (*text != '\0') {
        ssd1306_draw_char_color(display, x, y, *text, scale, on);
        x += (int)(6u * scale);
        ++text;
    }
}

void ssd1306_draw_text(ssd1306_t *display, int x, int y, const char *text,
                       unsigned scale) {
    ssd1306_draw_text_color(display, x, y, text, scale, true);
}

int ssd1306_text_width(const char *text, unsigned scale) {
    const size_t length = strlen(text);
    if (length == 0 || scale == 0) {
        return 0;
    }
    return (int)((length * 6u - 1u) * scale);
}

bool ssd1306_show(ssd1306_t *display) {
    static const uint8_t address_commands[] = {
        0x21, 0x00, SSD1306_WIDTH - 1,
        0x22, 0x00, (SSD1306_HEIGHT / 8) - 1
    };

    if (!send_commands(display, address_commands, sizeof(address_commands))) {
        return false;
    }

    uint8_t packet[SSD1306_BUFFER_SIZE + 1];
    packet[0] = 0x40; /* following bytes are display RAM data */
    memcpy(&packet[1], display->buffer, sizeof(display->buffer));

    return i2c_write_blocking(display->i2c, display->address,
                              packet, sizeof(packet), false) == sizeof(packet);
}
