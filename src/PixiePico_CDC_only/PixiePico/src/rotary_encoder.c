/*
Project PixiePico
rotary_encoder.c/.h

Pixie based digital transceiver firmware
Manage the rotary encoder function

Copyright Dr. Pedro E. Colla LU7DZ (2026)
For non-profit uses only
================================================================
Este programa  disponible es hecho público
bajo la licencia Creative Commons Attribution-ShareAlike 4.0
International (CC BY-SA 4.0).

*/

#include "rotary_encoder.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#define ENCODER_COUNTS_PER_DETENT 4
#define SWITCH_DEBOUNCE_MS 20

static uint encoder_pin_a;
static uint encoder_pin_b;
static volatile uint8_t previous_ab;
static volatile int quarter_step_accumulator;
static volatile int pending_steps;

static const int8_t transition_table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

static void encoder_gpio_irq(uint gpio, uint32_t events) {
    (void)gpio;
    (void)events;

    const uint8_t current_ab =
        (uint8_t)((gpio_get(encoder_pin_a) ? 2u : 0u) |
                  (gpio_get(encoder_pin_b) ? 1u : 0u));
    const uint8_t transition = (uint8_t)((previous_ab << 2) | current_ab);
    previous_ab = current_ab;
    quarter_step_accumulator += transition_table[transition];

    if (quarter_step_accumulator >= ENCODER_COUNTS_PER_DETENT) {
        ++pending_steps;
        quarter_step_accumulator = 0;
    } else if (quarter_step_accumulator <= -ENCODER_COUNTS_PER_DETENT) {
        --pending_steps;
        quarter_step_accumulator = 0;
    }
}

void rotary_encoder_init(rotary_encoder_t *encoder,
                         uint pin_a, uint pin_b, uint pin_sw) {
    encoder->pin_a = pin_a;
    encoder->pin_b = pin_b;
    encoder->pin_sw = pin_sw;
    encoder_pin_a = pin_a;
    encoder_pin_b = pin_b;

    gpio_init(pin_a);
    gpio_set_dir(pin_a, GPIO_IN);
    gpio_pull_up(pin_a);
    gpio_init(pin_b);
    gpio_set_dir(pin_b, GPIO_IN);
    gpio_pull_up(pin_b);
    gpio_init(pin_sw);
    gpio_set_dir(pin_sw, GPIO_IN);
    gpio_pull_up(pin_sw);

    previous_ab = (uint8_t)((gpio_get(pin_a) ? 2u : 0u) |
                            (gpio_get(pin_b) ? 1u : 0u));
    quarter_step_accumulator = 0;
    pending_steps = 0;
    encoder->raw_pressed = !gpio_get(pin_sw);
    encoder->stable_pressed = encoder->raw_pressed;
    encoder->raw_changed_at = get_absolute_time();

    gpio_set_irq_enabled_with_callback(
        pin_a, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true,
        &encoder_gpio_irq);
    gpio_set_irq_enabled(
        pin_b, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}

int rotary_encoder_take_steps(void) {
    const uint32_t interrupt_state = save_and_disable_interrupts();
    const int steps = pending_steps;
    pending_steps = 0;
    restore_interrupts(interrupt_state);
    return steps;
}

bool rotary_encoder_poll_switch(rotary_encoder_t *encoder, bool *pressed) {
    const bool sample_pressed = !gpio_get(encoder->pin_sw);

    if (sample_pressed != encoder->raw_pressed) {
        encoder->raw_pressed = sample_pressed;
        encoder->raw_changed_at = get_absolute_time();
    }

    const int64_t stable_time_us =
        absolute_time_diff_us(encoder->raw_changed_at, get_absolute_time());

    if (encoder->raw_pressed != encoder->stable_pressed &&
        stable_time_us >= SWITCH_DEBOUNCE_MS * 1000) {
        encoder->stable_pressed = encoder->raw_pressed;
        *pressed = encoder->stable_pressed;
        return true;
    }

    return false;
}
