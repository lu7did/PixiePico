/*
Project PixiePico

Pixie based digital transceiver firmware
Test firmware, basic build chain toolset verification

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
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"
#include "pico/stdio_usb.h"

#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 16
#endif

#define BLINK_INTERVAL_MS 500
#define WS2812_FREQUENCY_HZ 800000.0f

static inline uint32_t rgb_to_grb(uint8_t red, uint8_t green, uint8_t blue) {
    /* WS2812 expects GRB in the most significant 24 bits of the PIO word. */
    return ((uint32_t)green << 24) |
           ((uint32_t)red << 16) |
           ((uint32_t)blue << 8);
}

static void set_led(PIO pio, uint state_machine,
                    uint8_t red, uint8_t green, uint8_t blue) {
    pio_sm_put_blocking(pio, state_machine, rgb_to_grb(red, green, blue));
}

int main(void) {
    stdio_init_all();

    PIO pio = pio0;
    const uint state_machine = 0;
    const uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio,
                        state_machine,
                        offset,
                        PICO_DEFAULT_WS2812_PIN,
                        WS2812_FREQUENCY_HZ,
                        false);

    /* Give Windows time to enumerate the USB CDC virtual serial port. */
    /*
    sleep_ms(1500);

    printf("blink iniciado en Waveshare RP2040-Zero\r\n");
    printf("LED WS2812 conectado a GPIO %u\r\n", PICO_DEFAULT_WS2812_PIN);
    */

/*
 * Wait up to 15 seconds for Windows and the serial monitor to establish
 * the USB CDC connection. The LED program will continue even if no
 * monitor connects.
 */
    for (int attempt = 0; attempt < 150; ++attempt) {
        if (stdio_usb_connected()) {
            break;
    }

    sleep_ms(100);
}

    printf("blink iniciado en Waveshare RP2040-Zero\r\n");
    printf("LED WS2812 conectado a GPIO %u\r\n",
           PICO_DEFAULT_WS2812_PIN);
    fflush(stdout);

    bool is_on = false;
    unsigned long cycle = 0;

    while (true) {
        is_on = !is_on;
        ++cycle;

        if (is_on) {
            /* Green at low intensity; the onboard WS2812 is very bright. */
            set_led(pio, state_machine, 0, 24, 0);
            printf("Ciclo %lu: LED encendido\r\n", cycle);
        } else {
            set_led(pio, state_machine, 0, 0, 0);
            printf("Ciclo %lu: LED apagado\r\n", cycle);
        }
        fflush(stdout);
        sleep_ms(BLINK_INTERVAL_MS);
    }
}
