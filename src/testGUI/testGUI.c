/*
Project PixiePico

Pixie based digital transceiver firmware
Test firmware, basic GUI workbench and verification

Copyright Dr. Pedro E. Colla LU7DZ (2026)
For non-profit uses only
================================================================
Este programa  disponible es hecho público
bajo la licencia Creative Commons Attribution-ShareAlike 4.0
International (CC BY-SA 4.0).

*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "rotary_encoder.h"
#include "ssd1306.h"
#include "ws2812.pio.h"
#include <inttypes.h>

#define OLED_I2C i2c0
#define OLED_SDA_PIN 0
#define OLED_SCL_PIN 1
#define OLED_I2C_ADDRESS 0x3C
#define OLED_I2C_FREQUENCY_HZ 400000

#define ENCODER_CLK_PIN 29
#define ENCODER_DT_PIN 28
#define ENCODER_SW_PIN 27
#define ENCODER_FREQUENCY_STEP_HZ 10L
#define LONGPUSH 2000000
#define BLINK_INTERVAL_MS 500
#define WS2812_FREQUENCY_HZ 800000.0f

#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 16
#endif

static ssd1306_t oled;
static bool oled_available = false;
static long current_frequency_hz = 28074000L;
uint32_t timerSW = 0UL;

static inline uint32_t rgb_to_grb(uint8_t red, uint8_t green, uint8_t blue) {
    return ((uint32_t)green << 24) |
           ((uint32_t)red << 16) |
           ((uint32_t)blue << 8);
}

static void set_led(PIO pio, uint state_machine,
                    uint8_t red, uint8_t green, uint8_t blue) {
    pio_sm_put_blocking(pio, state_machine, rgb_to_grb(red, green, blue));
}

static void wait_for_usb_monitor(void) {
    for (int attempt = 0; attempt < 150; ++attempt) {
        if (stdio_usb_connected()) {
            return;
        }
        sleep_ms(100);
    }
}

static void fill_rectangle(ssd1306_t *display, int x, int y,
                           int width, int height, bool on) {
    for (int py = y; py < y + height; ++py) {
        for (int px = x; px < x + width; ++px) {
            ssd1306_draw_pixel(display, px, py, on);
        }
    }
}

static void draw_rectangle(ssd1306_t *display, int x, int y,
                           int width, int height, bool on) {
    for (int px = x; px < x + width; ++px) {
        ssd1306_draw_pixel(display, px, y, on);
        ssd1306_draw_pixel(display, px, y + height - 1, on);
    }
    for (int py = y; py < y + height; ++py) {
        ssd1306_draw_pixel(display, x, py, on);
        ssd1306_draw_pixel(display, x + width - 1, py, on);
    }
}

static void refresh_oled(void) {
    if (oled_available) {
        (void)ssd1306_show(&oled);
    }
}

void displayMode(const char *mode) {
    if (!oled_available) {
        return;
    }

    char visible_mode[4];
    snprintf(visible_mode, sizeof(visible_mode), "%.3s",
             mode != NULL ? mode : "");

    fill_rectangle(&oled, 0, 0, 20, 9, false);
    ssd1306_draw_text(&oled, 0, 1, visible_mode, 1);
    refresh_oled();
}

void displayVFO(const char *vfo) {
    if (!oled_available) {
        return;
    }

    char visible_vfo[5];
    snprintf(visible_vfo, sizeof(visible_vfo), "%.4s",
             vfo != NULL ? vfo : "");

    fill_rectangle(&oled, 22, 0, 27, 9, false);
    ssd1306_draw_text(&oled, 22, 1, visible_vfo, 1);
    refresh_oled();
}

void displayTX(bool transmitting) {
    if (!oled_available) {
        return;
    }

    if (transmitting) {
        /* TX: illuminated background and dark letters. */
        fill_rectangle(&oled, 51, 0, 16, 9, true);
        ssd1306_draw_text_color(&oled, 53, 1, "TX", 1, false);
    } else {
        /* RX: dark background and illuminated letters. */
        fill_rectangle(&oled, 51, 0, 16, 9, false);
        ssd1306_draw_text(&oled, 53, 1, "RX", 1);
    }

    refresh_oled();
}

void displayLED(unsigned level) {
    if (!oled_available) {
        return;
    }

    if (level > 5) {
        level = 5;
    }

    const int led_start_x = 75;
    const int led_width = 5;
    const int led_height = 7;
    const int led_spacing = 2;

    fill_rectangle(&oled, led_start_x, 0, 33, 9, false);

    for (int led = 0; led < 5; ++led) {
        const int x = led_start_x + led * (led_width + led_spacing);
        if ((unsigned)led < level) {
            fill_rectangle(&oled, x, 1, led_width, led_height, true);
        } else {
            draw_rectangle(&oled, x, 1, led_width, led_height, true);
        }
    }

    refresh_oled();
}

void displayFreq(long frequency_hz) {
    if (!oled_available) {
        return;
    }

    if (frequency_hz < 0) {
        frequency_hz = 0;
    }

    const long frequency_khz = frequency_hz / 1000L;
    const long hundreds_and_tens_hz = (frequency_hz % 1000L) / 10L;

    char frequency_text[16];
    char superscript_text[3];
    snprintf(frequency_text, sizeof(frequency_text), "%ld", frequency_khz);
    snprintf(superscript_text, sizeof(superscript_text), "%02ld",
             hundreds_and_tens_hz);

    /*
     * Prefer scale 3. Reduce it only if a larger kHz value would not fit beside
     * its two-digit superscript.
     */
    unsigned frequency_scale = 3;
    const unsigned fraction_scale = 1;
    const int gap = 3;

    while (frequency_scale > 1 &&
           ssd1306_text_width(frequency_text, frequency_scale) + gap +
               ssd1306_text_width(superscript_text, fraction_scale) >
               SSD1306_WIDTH) {
        --frequency_scale;
    }

    const int total_width =
        ssd1306_text_width(frequency_text, frequency_scale) + gap +
        ssd1306_text_width(superscript_text, fraction_scale);
    const int frequency_x = (SSD1306_WIDTH - total_width) / 2;
    const int frequency_y = SSD1306_HEIGHT - (int)(7u * frequency_scale);
    const int superscript_y = 11;

    fill_rectangle(&oled, 0, 10, SSD1306_WIDTH, SSD1306_HEIGHT - 10, false);
    ssd1306_draw_text(&oled, frequency_x, frequency_y,
                      frequency_text, frequency_scale);
    ssd1306_draw_text(&oled,
                      frequency_x +
                          ssd1306_text_width(frequency_text, frequency_scale) +
                          gap,
                      superscript_y,
                      superscript_text, fraction_scale);

    refresh_oled();
}

void rotaryTurn(int direction) {
    if (direction != 1 && direction != -1) {
        return;
    }

    current_frequency_hz += direction * ENCODER_FREQUENCY_STEP_HZ;
    if (current_frequency_hz < 0) {
        current_frequency_hz = 0;
    }
    displayFreq(current_frequency_hz);
    printf("Encoder: %+d; frecuencia: %ld Hz\r\n",
           direction, current_frequency_hz);
    fflush(stdout);
}

void SWclick(bool pressed) {
    displayTX(pressed);
    if(pressed) {
      timerSW = time_us_32();
    } else {
      uint32_t lapSW=time_us_32()-timerSW;
      if (lapSW>LONGPUSH) {
        printf("Lap: %" PRIu32 " <longPUSH>\n", lapSW);
      } else {
        printf("Lap: %" PRIu32 " <shortPUSH>\n", lapSW);
      }  
    }
    printf("SW: %s\r\n", pressed ? "presionado" : "liberado");
    fflush(stdout);
}

int main(void) {
    stdio_init_all();

    const PIO pio = pio0;
    const uint state_machine = 0;
    const uint pio_offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, state_machine, pio_offset,
                        PICO_DEFAULT_WS2812_PIN,
                        WS2812_FREQUENCY_HZ, false);

    i2c_init(OLED_I2C, OLED_I2C_FREQUENCY_HZ);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);

    rotary_encoder_t encoder;
    rotary_encoder_init(&encoder,
                        ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);

    oled_available = ssd1306_init(&oled, OLED_I2C, OLED_I2C_ADDRESS);

    if (oled_available) {
        ssd1306_clear(&oled);
        (void)ssd1306_show(&oled);
        displayMode("FT8");
        displayVFO("VFOa");
        displayTX(true);
        displayLED(0);
        displayFreq(current_frequency_hz);
    }

    wait_for_usb_monitor();

    printf("testGUI iniciado en Waveshare RP2040-Zero\r\n");
    printf("LED WS2812: GPIO %u\r\n", PICO_DEFAULT_WS2812_PIN);
    printf("OLED: SSD1306 128x32, I2C0, SDA=GPIO%d, SCL=GPIO%d, addr=0x%02X\r\n",
           OLED_SDA_PIN, OLED_SCL_PIN, OLED_I2C_ADDRESS);
    printf("OLED %s; pantalla FT8/VFOA/TX, frecuencia 28074.00\r\n",
           oled_available ? "detectado" : "NO detectado");
    printf("KY-040: CLK(A)=GPIO%d, DT(B)=GPIO%d, SW=GPIO%d\r\n",
           ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);
    fflush(stdout);

    bool is_on = false;
    unsigned long cycle = 0;
    unsigned led_level = 0;
    absolute_time_t next_blink =
        delayed_by_ms(get_absolute_time(), BLINK_INTERVAL_MS);

    while (true) {
        int steps = rotary_encoder_take_steps();
        while (steps > 0) {
            rotaryTurn(+1);
            --steps;
        }
        while (steps < 0) {
            rotaryTurn(-1);
            ++steps;
        }

        bool switch_pressed;
        if (rotary_encoder_poll_switch(&encoder, &switch_pressed)) {
            SWclick(switch_pressed);
        }

        if (time_reached(next_blink)) {
            next_blink = delayed_by_ms(next_blink, BLINK_INTERVAL_MS);
            is_on = !is_on;
            ++cycle;
            ++led_level;
            if (led_level > 5) {
                led_level = 0;
            }

            if (is_on) {
                set_led(pio, state_machine, 0, 24, 0);
                //printf("Ciclo %lu: LED encendido\r\n", cycle);
            } else {
                set_led(pio, state_machine, 0, 0, 0);
                //printf("Ciclo %lu: LED apagado\r\n", cycle);
            }

            displayLED(led_level);
            fflush(stdout);
        }

        sleep_ms(1);
    }
}
