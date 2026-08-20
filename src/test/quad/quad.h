/*
 * =======================================================================================
 * quad
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * Implementation of a digitally synthethized clock with PLL optimization 
 * =======================================================================================*/
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "hardware/pio.h"

#ifdef __cplusplus
extern "C" {
#endif

//*--------------------------------------------------
//* Used GPIO pins
//*--------------------------------------------------
#define QUAD_I_PIN   14u
#define QUAD_Q_PIN   15u

//*--- This is the maximum allowed clock, the solutions must not go higher than that
#define QUAD_CLK_SYS_MAX_HZ 270000000u

//*------------------------------------------------------
//* Structure to store the optimization result
//*------------------------------------------------------
typedef struct {
  uint32_t f_req_hz;
  uint32_t f_out_hz;
  int32_t  err_hz;
  uint32_t err_abs_hz;

  uint8_t  refdiv;
  uint16_t fbdiv;
  uint8_t  postdiv1;
  uint8_t  postdiv2;
  uint32_t vco_hz;
  uint32_t clk_sys_hz;

  uint16_t pio_div_int;
  uint8_t  pio_div_frac;
  uint32_t N;
} quad_solution_t;

//*--------------------------------------------------
//* Clock overall status
//*--------------------------------------------------
typedef struct {
  PIO  pio;
  uint sm;
  uint offset;
  bool initialized;
  bool running;
} quad_osc_t;

//*----------------------------------------------------
//* External API as seen by the caller
//*----------------------------------------------------
bool quad_init(quad_osc_t *q, PIO pio, uint sm);

bool quad_start(quad_osc_t *q,
                uint32_t f_hz,
                bool reboot_after,
                quad_solution_t *sol_out);

bool quad_set_frequency(quad_osc_t *q,
                        uint32_t f_hz,
                        bool reboot_after,
                        quad_solution_t *sol_out);

void quad_stop(quad_osc_t *q);

//*--------------------------------------------------
//* Reboot flow (watchdog scratch)
//*--------------------------------------------------
void quad_request_frequency_and_reboot(uint32_t f_hz);

bool quad_boot_get_requested_frequency(uint32_t *f_hz_out);

bool quad_boot_apply_requested_or_default(quad_osc_t *q,
                                          uint32_t default_hz,
                                          quad_solution_t *sol_out);

//*------------------------------------------------------
// USB safe reclock (required by TinyUSB)
//*------------------------------------------------------
bool quad_usb_safe_reclock(const quad_solution_t *s, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif