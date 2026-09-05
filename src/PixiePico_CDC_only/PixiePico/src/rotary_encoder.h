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

#ifndef TESTGUI_ROTARY_ENCODER_H
#define TESTGUI_ROTARY_ENCODER_H

#include <stdbool.h>
#include "pico/types.h"

typedef struct {
    uint pin_a;
    uint pin_b;
    uint pin_sw;
    bool raw_pressed;
    bool stable_pressed;
    absolute_time_t raw_changed_at;
} rotary_encoder_t;

void rotary_encoder_init(rotary_encoder_t *encoder,
                         uint pin_a, uint pin_b, uint pin_sw);
int rotary_encoder_take_steps(void);
bool rotary_encoder_poll_switch(rotary_encoder_t *encoder, bool *pressed);

#endif
