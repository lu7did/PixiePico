/*
 * =======================================================================================
 * quad
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * Implementation of a digitally synthethized clock with PLL optimization 
 * =======================================================================================*/
 //*---------------------------------------------------------------------------------------*
 //*                                   includes                                            *
 //*---------------------------------------------------------------------------------------*

#include "quad.h"
#include <limits.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/irq.h"
#include "hardware/resets.h"
#include "hardware/structs/clocks.h"
#include "hardware/watchdog.h"
#include "tusb.h"
#include "quad.pio.h"

 //*---------------------------------------------------------------------------------------*
 //*                                   constants                                           *
 //*---------------------------------------------------------------------------------------*
#define VCO_MIN_HZ  400000000u
#define VCO_MAX_HZ 1600000000u

//*--- quadrature means 4 states per period, factor=256/4=64
#define STATES_PER_PERIOD  4u
#define NUMERATOR_FACTOR   (256u / STATES_PER_PERIOD)

#if (256u % STATES_PER_PERIOD) != 0
#error "STATES_PER_PERIOD must be a whole divisor of 256."
#endif

// Watchdog scratch
#define WD_MAGIC      0x46543821u  // "FT8!"
#define WD_REG_FREQ   0
#define WD_REG_MAGIC  1

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                                Utility functions
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

//*---------------------------------------------------------------------------------------*
//*                           utility and service functions                               *
//*---------------------------------------------------------------------------------------*
static inline uint32_t u32_abs_i32(int32_t x) {
  return (x < 0) ? (uint32_t)(-x) : (uint32_t)x;
}

static inline uint32_t udiv_round_u64(uint64_t num, uint32_t den) {
  return (uint32_t)((num + (uint64_t)den / 2u) / (uint64_t)den);
}

static inline uint32_t fout_from(uint32_t clk_sys_hz, uint32_t N) {
  uint64_t num = (uint64_t)clk_sys_hz * (uint64_t)NUMERATOR_FACTOR;
  return (uint32_t)(num / (uint64_t)N);
}
//*---------------------------------------------------------------------------------------*
//*  this is the optimization function which calculates which is the best system PLL      *
//*  to minimize the error of a given frequency                                           *
//*---------------------------------------------------------------------------------------*
static bool evaluate_clk_sys(uint32_t clk_sys_hz,
                             uint32_t f_req_hz,
                             quad_solution_t *out)
{
  if (!clk_sys_hz || !f_req_hz) return false;

  uint64_t num = (uint64_t)clk_sys_hz * (uint64_t)NUMERATOR_FACTOR;
  uint32_t N = udiv_round_u64(num, f_req_hz);
  if (N < 1u) N = 1u;

  uint32_t cand[3] = { (N > 1u) ? (N - 1u) : 1u, N, N + 1u };

  quad_solution_t best = {0};
  best.err_abs_hz = UINT_MAX;

  for (int i = 0; i < 3; i++) {
    uint32_t Ni = cand[i];
    uint32_t f_out = fout_from(clk_sys_hz, Ni);
    int32_t err = (int32_t)f_out - (int32_t)f_req_hz;
    uint32_t err_abs = u32_abs_i32(err);

    if (err_abs < best.err_abs_hz) {
      best.f_out_hz = f_out;
      best.err_hz = err;
      best.err_abs_hz = err_abs;
      best.N = Ni;
      best.pio_div_int  = (uint16_t)(Ni / 256u);
      best.pio_div_frac = (uint8_t)(Ni % 256u);
    }
  }

  *out = best;
  out->clk_sys_hz = clk_sys_hz;
  out->f_req_hz   = f_req_hz;
  return true;
}
//*---------------------------------------------------------------------------------------*
//*  main optimization algorithm, find both the PLL and divisor to minimize the error     *
//*  to minimize the error of a given frequency                                           *
//*---------------------------------------------------------------------------------------*
static bool find_optimal(uint32_t f_req_hz, quad_solution_t *best_out) {
  if (!best_out || !f_req_hz) return false;

  quad_solution_t best = {0};
  best.err_abs_hz = UINT_MAX;

  for (uint8_t refdiv = 1; refdiv <= 16; refdiv++) {

    const uint32_t xosc_hz = 12000000u;   // cristal real de Pico
    uint32_t f_ref = xosc_hz / refdiv;

    if (!f_ref) continue;

    for (uint16_t fbdiv = 16; fbdiv <= 320; fbdiv++) {
      uint64_t vco = (uint64_t)f_ref * (uint64_t)fbdiv;
      if (vco < VCO_MIN_HZ || vco > VCO_MAX_HZ) continue;

      for (uint8_t post1 = 1; post1 <= 7; post1++) {
        for (uint8_t post2 = 1; post2 <= post1; post2++) {
          uint32_t clk_sys =
              (uint32_t)(vco / ((uint64_t)post1 * (uint64_t)post2));
          if (!clk_sys || clk_sys > QUAD_CLK_SYS_MAX_HZ) continue;

          quad_solution_t tmp = {0};
          if (!evaluate_clk_sys(clk_sys, f_req_hz, &tmp)) continue;

          if (tmp.err_abs_hz < best.err_abs_hz) {
            best = tmp;
            best.refdiv   = refdiv;
            best.fbdiv    = fbdiv;
            best.postdiv1 = post1;
            best.postdiv2 = post2;
            best.vco_hz   = (uint32_t)vco;

            if (best.err_abs_hz == 0) {
              *best_out = best;
              return true;
            }
          }
        }
      }
    }
  }

  if (best.err_abs_hz == UINT_MAX) return false;
  *best_out = best;
  return true;
}
//*---------------------------------------------------------------------------------------*
//*  this is the function to change the PLL, it has to make it in a safe way to avoid the * 
//*  board to hand-up. First changes the clock to a different one in order to avoid having*
//*  the board with no clock during the transition, then changes PLL_SYS and switch the   *
//*  board back to use this clock (now in a safe way)                                     *
//*---------------------------------------------------------------------------------------*
static void apply_pll_sys(const quad_solution_t *s)
{
  //*--- set_sys_clock_pll() is the recommended way to change clk_sys by the SDK
  //*--- bool set_sys_clock_pll(uint32_t vco_freq, uint post_div1, uint post_div2);
  //*--- No interrupts during the switch

  uint32_t save = save_and_disable_interrupts();

  //*--- Configure PLL_SYS with the computed parameters (at the quad_solution_t structure)
  //*--- s->vco_hz, s->postdiv1, s->postdiv2 are already compliant with restrictions of rp2040
  //*--- refdiv también existe, pero set_sys_clock_pll asume XOSC=12MHz y usa el flow del SDK.
  //
  //*--- If find_optimal uses refdiv !=1, set_sys_clock_pll DOES NOT support it
  //*--- find_optimal needs to be restricted

  (void)set_sys_clock_pll(s->vco_hz, (uint)s->postdiv1, (uint)s->postdiv2);

  //*--- Ensure clk_peri follows clk_sys avoiding integrity issues
  clock_configure(clk_peri,
                  0,
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  s->clk_sys_hz,
                  s->clk_sys_hz);

  //*--- Restore interrupts after the switch                  
  restore_interrupts(save);
}
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*                              Public API
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

//*---------------------------------------------------------------------------------------*
//*                       Quad clock initialization                                       *
//*---------------------------------------------------------------------------------------*
bool quad_init(quad_osc_t *q, PIO pio, uint sm)
{
  if (!q) return false;

  q->pio = pio;
  q->sm  = sm;
  q->running = false;
  q->initialized = false;

  //*--- Load PIO program
  int offset = pio_add_program(pio, &quad_program);
  if (offset < 0) {
    return false;   // no hay espacio en la PIO
  }
  q->offset = (uint)offset;

  //*--- Set base configuration of the SM
  
  pio_sm_config c = quad_program_get_default_config(q->offset);
  sm_config_set_set_pins(&c, QUAD_I_PIN, 2);

  //*--- Init GPIO output functions

  pio_gpio_init(pio, QUAD_I_PIN);
  pio_gpio_init(pio, QUAD_Q_PIN);

  //*--- Define two consecutive pins as output
  
  pio_sm_set_consecutive_pindirs(pio, sm, QUAD_I_PIN, 2, true);

  //*--- Reset the SM (but do not start as yet)
  
  pio_sm_init(pio, sm, q->offset, &c);
  pio_sm_set_enabled(pio, sm, false);

  q->initialized = true;
  return true;
}
//*---------------------------------------------------------------------------------------*
//*                       Stop the quad clock                                             *
//*---------------------------------------------------------------------------------------*
void quad_stop(quad_osc_t *q) {
  if (!q || !q->initialized) return;
  pio_sm_set_enabled(q->pio, q->sm, false);
  q->running = false;
}
//*---------------------------------------------------------------------------------------*
//*                       Start the quad clock                                            *
//*---------------------------------------------------------------------------------------*
bool quad_start(quad_osc_t *q,
                uint32_t f_hz,
                bool reboot_after,
                quad_solution_t *sol_out)
{
  if (!q || !q->initialized || f_hz == 0) return false;

  quad_solution_t sol = {0};
  if (!find_optimal(f_hz, &sol)) return false;

  //*--- this is a special configuration when the directive is to reboot after the change

  if (reboot_after) {
    quad_request_frequency_and_reboot(f_hz);
  }



//#define QUICKFIX
  #ifdef QUICKFIX 

  //*---- DEBUG
  //*---- This is a temporal fix to test the PIO management separate from
  //*---- the optimization algorithm, basically it starts the operation wieh
  //*---- a predefined (manually optimized) solution for 3.5 MHz
if (f_hz == 3500000u) {

  //*--- Force 270 MHz as the board clock
  set_sys_clock_khz(270000, true);

  pio_sm_set_enabled(q->pio, q->sm, false);
  pio_sm_restart(q->pio, q->sm);
  pio_sm_clear_fifos(q->pio, q->sm);

  //*--- clkdiv = 19 + 73/256  => ~3.500101 MHz
  pio_sm_set_clkdiv_int_frac(q->pio, q->sm, 19, 73);
  pio_sm_exec(q->pio, q->sm, pio_encode_jmp(q->offset));
  pio_sm_set_enabled(q->pio, q->sm, true);
  q->running = true;

  //*--- Format solution structure with a manually set solution

  if (sol_out) {
    sol_out->f_req_hz = f_hz;
    sol_out->clk_sys_hz = 270000000u;
    sol_out->pio_div_int = 19;
    sol_out->pio_div_frac = 73;
  }
  return true;
}
//*--- 
#endif //QUICKFIX

  //*--- This is the standard operation with optimization
  //*--- Stop the SM to avoid erratic inconsistent states
  pio_sm_set_enabled(q->pio, q->sm, false);

  //*--- Check if the solution requires to change the board clock
  uint32_t current = clock_get_hz(clk_sys);
  if (current != sol.clk_sys_hz) {
    apply_pll_sys(&sol);
  }

  //*--- Read the current clk_sys to verify the change has been made
  uint32_t clk_real = clock_get_hz(clk_sys);
  if (clk_real == 0) return false;

  //*---Recalculate  divisor Q8.8 (N = clk_sys * 64 / f) with rounding
  // N represents clkdiv * 256
  
  uint64_t num = (uint64_t)clk_real * (uint64_t)NUMERATOR_FACTOR; // 64
  uint32_t N = (uint32_t)((num + (uint64_t)f_hz / 2u) / (uint64_t)f_hz);
  if (N < 1u) N = 1u;

  uint16_t div_int  = (uint16_t)(N / 256u);
  uint8_t  div_frac = (uint8_t)(N % 256u);

  //*--- Invalid divisor if 0
  if (div_int == 0 && div_frac == 0) return false;

  //*--- Reset SM to a known state and apply divisor
  pio_sm_restart(q->pio, q->sm);
  pio_sm_clear_fifos(q->pio, q->sm);

  pio_sm_set_clkdiv_int_frac(q->pio, q->sm, div_int, div_frac);

  //*--- Reboot the PIO (RISK CPU PC to start of the program)
  pio_sm_exec(q->pio, q->sm, pio_encode_jmp(q->offset));

  //*--- Enable SM
  pio_sm_set_enabled(q->pio, q->sm, true);
  q->running = true;

  //*--- Complete solution structure consistent with clk_real
  if (sol_out) {
    *sol_out = sol;                     // Preserve choosen PLL
    sol_out->f_req_hz = f_hz;
    sol_out->clk_sys_hz = clk_real;
    sol_out->N = N;
    sol_out->pio_div_int  = div_int;
    sol_out->pio_div_frac = div_frac;

    //*--- Set the effective frequency as per the optimization model
    //*--- f_out = clk_sys*64/N  (because there are 4 states and Q8.8)
    sol_out->f_out_hz = (uint32_t)((num) / (uint64_t)N);
    sol_out->err_hz = (int32_t)sol_out->f_out_hz - (int32_t)f_hz;
    sol_out->err_abs_hz = u32_abs_i32(sol_out->err_hz);
  }
  return true;
}
//*---------------------------------------------------------------------------------------*
//*                       Change the frequency                                            *
//*---------------------------------------------------------------------------------------*
bool quad_set_frequency(quad_osc_t *q,
                        uint32_t f_hz,
                        bool reboot_after,
                        quad_solution_t *sol_out)
{

  //*--- The frequency switch should not be while the board is actually been used so
  //*--- a brief glich (interruption and reset) of USB communications might occur
  //*--- if this is severe the best course of action is to force a reboot after the
  //*--- change, the caller is responsible to remember when booting which was the
  //*--- new frequency (flash memory perhaps)

  if (!q || !q->initialized || f_hz == 0) return false;

  quad_solution_t sol = {0};
  if (!find_optimal(f_hz, &sol)) return false;

  //*--- If enabled here a boot is performed
  if (reboot_after) {
    quad_request_frequency_and_reboot(f_hz);
  }

  bool was_running = q->running;

  //*--- Stop the SM while the configuration change is being made
  pio_sm_set_enabled(q->pio, q->sm, false);
  q->running = false;

  //*--- Change the board clk_sys is needed as recommended by the optimization algorithm
  uint32_t current = clock_get_hz(clk_sys);
  if (current != sol.clk_sys_hz) {
    apply_pll_sys(&sol);
  }

  //*--- Read actual clk_sys  after the change to validate it
  uint32_t clk_real = clock_get_hz(clk_sys);
  if (clk_real == 0) return false;

  //*--- Recompute  divisor Q8.8 (N = clk_sys * 64 / f) with rounding
  uint64_t num = (uint64_t)clk_real * (uint64_t)NUMERATOR_FACTOR; // 64
  uint32_t N = (uint32_t)((num + (uint64_t)f_hz / 2u) / (uint64_t)f_hz);
  if (N < 1u) N = 1u;

  uint16_t div_int  = (uint16_t)(N / 256u);
  uint8_t  div_frac = (uint8_t)(N % 256u);

  if (div_int == 0 && div_frac == 0) return false;

  //*--- Clean reboot of the PIO SM applying the new divisor
  pio_sm_restart(q->pio, q->sm);
  pio_sm_clear_fifos(q->pio, q->sm);
  pio_sm_set_clkdiv_int_frac(q->pio, q->sm, div_int, div_frac);
  pio_sm_exec(q->pio, q->sm, pio_encode_jmp(q->offset));

  //*--- Resume executiong
  if (was_running) {
    pio_sm_set_enabled(q->pio, q->sm, true);
    q->running = true;
  }

  if (sol_out) {
    *sol_out = sol;
    sol_out->f_req_hz = f_hz;
    sol_out->clk_sys_hz = clk_real;
    sol_out->N = N;
    sol_out->pio_div_int  = div_int;
    sol_out->pio_div_frac = div_frac;

    sol_out->f_out_hz = (uint32_t)(num / (uint64_t)N);
    sol_out->err_hz = (int32_t)sol_out->f_out_hz - (int32_t)f_hz;
    sol_out->err_abs_hz = u32_abs_i32(sol_out->err_hz);
  }

  return true;
}

//*---------------------------------------------------------------------------------------*
//*  USB safe reclock                                                                     *
//*---------------------------------------------------------------------------------------*
bool quad_usb_safe_reclock(const quad_solution_t *s, uint32_t timeout_ms) {
  if (!s || s->clk_sys_hz == 0) return false;

  tud_disconnect();
  sleep_ms(150);

  for (int i = 0; i < 20; i++) {
    tud_task();
    sleep_ms(1);
  }

  apply_pll_sys(s);
  sleep_ms(10);

  reset_block(RESETS_RESET_USBCTRL_BITS);
  sleep_us(20);
  unreset_block_wait(RESETS_RESET_USBCTRL_BITS);

  tusb_init();

  tud_connect();

  absolute_time_t t0 = get_absolute_time();
  while (!tud_mounted()) {
    tud_task();
    if (absolute_time_diff_us(t0, get_absolute_time()) >
        (int64_t)timeout_ms * 1000) {
      return false;
    }
  }
  return true;
}

//*---------------------------------------------------------------------------------------*
//*  reboot helpers                                                                       *
//*---------------------------------------------------------------------------------------*
void quad_request_frequency_and_reboot(uint32_t f_hz) {
  watchdog_hw->scratch[WD_REG_FREQ]  = f_hz;
  watchdog_hw->scratch[WD_REG_MAGIC] = WD_MAGIC;
  watchdog_reboot(0, 0, 0);
  while (true) tight_loop_contents();
}

bool quad_boot_get_requested_frequency(uint32_t *f_hz_out) {
  if (!f_hz_out) return false;
  if (watchdog_hw->scratch[WD_REG_MAGIC] != WD_MAGIC) return false;

  uint32_t f = watchdog_hw->scratch[WD_REG_FREQ];
  watchdog_hw->scratch[WD_REG_MAGIC] = 0;
  watchdog_hw->scratch[WD_REG_FREQ]  = 0;

  if (!f) return false;
  *f_hz_out = f;
  return true;
}

bool quad_boot_apply_requested_or_default(quad_osc_t *q,
                                         uint32_t default_hz,
                                         quad_solution_t *sol_out)
{
  uint32_t f = default_hz;
  (void)quad_boot_get_requested_frequency(&f);
  return quad_start(q, f, false, sol_out);
}
