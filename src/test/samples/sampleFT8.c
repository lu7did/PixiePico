// main.c (Pico SDK)
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

bool ft8_message_to_tones(const char *message, uint8_t tones_out[79]);

static void print_tones(const uint8_t tones[79])
{
    // Imprime "00 03 07 ..." por consola
    for (int i = 0; i < 79; i++) {
        printf("%02u%s", (unsigned)tones[i], (i == 78) ? "\n" : " ");
    }
}

int main(void)
{
    stdio_init_all();
    sleep_ms(1500);

    const char *msg = "CQ LU7DZ GF11";

    uint8_t tones[79];
    bool ok = ft8_message_to_tones(msg, tones);
    if (!ok) {
        printf("No se pudo codificar FT8: '%s'\n", msg);
        while (true) tight_loop_contents();
    }

    printf("FT8 tones para: %s\n", msg);
    print_tones(tones);

    while (true) {
        sleep_ms(1000);
    }
}

