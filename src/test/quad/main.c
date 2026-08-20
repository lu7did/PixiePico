

//*----------------------------------------------------------------------------
//* quadra
//*----------------------------------------------------------------------------
//* Testbed for the ADX DDS PIO-based digital quadrature frequency synth (I/Q
//* using Raspberry Pi Pico.
//* This is a working module of an integration effort to build an FT8 
//* transceiver using Raspberry Pi Pico as the main controller and signal
//* generator.
//* This is a design and development from scratch
//* 
//* Copyright (c) 2025 by Pedro Colla (LU7DZ)
//*----------------------------------------------------------------------------
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "tusb.h"
#include "usb_descriptors.h"
#include "hardware/gpio.h"
#include <stdarg.h>
#include <stdio.h>
#include "quad.h"

#include <inttypes.h>
#include "hardware/clocks.h"
#include "hardware/pio.h"

#include <stdio.h>
#include <inttypes.h>


//*----------------------------------------------------------------------------
//*                             Definitions
//*----------------------------------------------------------------------------
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif
#define TX              3  //TX LED
#define TXSW            8  //RX-TX Switch

//*----------------------------------------------------------------------------
//*                             Variables
//*----------------------------------------------------------------------------
char hi[128];

//*----------------------------------------------------------------------------
//*                             Prototypes
//*----------------------------------------------------------------------------
void cdc_write(char *buf, uint16_t length);

//*----------------------------------------------------------------------------
//*                         CDC Debug print macro
//*----------------------------------------------------------------------------
#define cdc_printf(fmt, ...)                           \
    do {                                                \
        int _cdc_len = snprintf(hi,               \
                                sizeof(hi),       \
                                (fmt), ##__VA_ARGS__);  \
        if (_cdc_len > 0) {                             \
            if (_cdc_len > (int)sizeof(hi))       \
                _cdc_len = sizeof(hi);            \
            cdc_write(hi, (uint16_t)_cdc_len);    \
            tud_cdc_write_flush();                \
        }                                               \
    } while (0)


//*----------------------------------------------------------------------------
//*                       Blink default (board) LED
//*----------------------------------------------------------------------------
static void blink_code(int n) {
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  while (1) {
    for (int i = 0; i < n; i++) {
      gpio_put(PICO_DEFAULT_LED_PIN, 1); sleep_ms(150);
      gpio_put(PICO_DEFAULT_LED_PIN, 0); sleep_ms(150);
    }
    sleep_ms(700);
  }
}
//*----------------------------------------------------------------------------
//*                    CDC Write and flush buffer function
//*----------------------------------------------------------------------------
void cdc_write(char *buf, uint16_t length) {

  tud_cdc_write(buf, length);
  tud_cdc_write_flush();
}
//*----------------------------------------------------------------------------
//*              Read actual divider of the SM (Int and Frac Q8.8) 
//*----------------------------------------------------------------------------
static void read_sm_clkdiv(PIO pio, uint sm, uint16_t *di, uint8_t *df) {

  //*---- SMx_CLKDIV INT part at bits 31:16 and FRAC at bits 18:8 (8 bits)

  uint32_t reg = pio->sm[sm].clkdiv;
  *di = (uint16_t)(reg >> 16);
  *df = (uint8_t)((reg >> 8) & 0xffu);
}
//*----------------------------------------------------------------------------
//*   Estimation of f_out from clk_sys and N (with N= div_int*256 + div_frac)
//*----------------------------------------------------------------------------
static uint32_t calc_fout_est(uint32_t clk_sys_hz, uint32_t N_q8_8) {

  //*--- for a quad PIO with 4 states f_out = clk_sys * 64 / N

  const uint32_t factor = 64u;
  uint64_t num = (uint64_t)clk_sys_hz * (uint64_t)factor;
  return (uint32_t)(num / (uint64_t)N_q8_8);
}
//*----------------------------------------------------------------------------
//*   Utility function to print optimization solution for debugging purposes
//*----------------------------------------------------------------------------
static void dump_solution(const char *tag,
                          const quad_solution_t *s,
                          PIO pio, uint sm) {

//*--- This function reads what the current Hw status really is in order to 
//*--- check if the setup of the solution has been successfully deployed                            
  uint32_t clk_now = clock_get_hz(clk_sys);
  uint16_t di_hw; uint8_t df_hw;
  read_sm_clkdiv(pio, sm, &di_hw, &df_hw);

  uint32_t N_hw = (uint32_t)di_hw * 256u + (uint32_t)df_hw;
  uint32_t fout_hw_est = calc_fout_est(clk_now, N_hw);

  char b[320];
  //*--- different values are now printed, the message is split to be handled
  //*--- properly by the USB CDC, menwhile the USB service task is called to 
  //*--- clean up the buffers and release space

  int n = snprintf(b, sizeof(b),"\n[%s]\n REQ=%" PRIu32 "\n",tag,s->f_req_hz);
  cdc_write(b, (uint16_t)n);
  tud_cdc_write_flush();
  for(int n=0;n<10;n++) {tud_task();}

  n = snprintf(b, sizeof(b),"clk_sys(now)=%" PRIu32 "\n",s->clk_sys_hz);
  cdc_write(b, (uint16_t)n);
  tud_cdc_write_flush();
  for(int n=0;n<10;n++) {tud_task();}

  n = snprintf(b, sizeof(b),"SOL: clk_sys=%" PRIu32 "\n  vco=%" PRIu32 "\n",s->clk_sys_hz, s->vco_hz);
  cdc_write(b, (uint16_t)n);
  tud_cdc_write_flush();
  for(int n=0;n<10;n++) {tud_task();}

  n = snprintf(b, sizeof(b),"post=%u/%u  fbdiv=%u refdiv=%u\n",(unsigned)s->postdiv1, (unsigned)s->postdiv2,    (unsigned)s->fbdiv, (unsigned)s->refdiv);
  cdc_write(b, (uint16_t)n);
  tud_cdc_write_flush();
  for(int n=0;n<10;n++) {tud_task();}

  n = snprintf(b, sizeof(b),"SOL: N=%" PRIu32 "  div=%u+%u/256  f_out=%" PRIu32 "  err=%" PRId32 "\n",s->N, (unsigned)s->pio_div_int, (unsigned)s->pio_div_frac, s->f_out_hz, s->err_hz);
  cdc_write(b, (uint16_t)n);
  tud_cdc_write_flush();
  for(int n=0;n<10;n++) {tud_task();}

  n = snprintf(b, sizeof(b),"HW : N=%" PRIu32 "  div=%u+%u/256  f_out_est=%" PRIu32 "\n",N_hw, (unsigned)di_hw, (unsigned)df_hw, fout_hw_est); 
  cdc_write(b, (uint16_t)n);
  tud_cdc_write_flush();
  for(int n=0;n<10;n++) {tud_task();}

}
//*----------------------------------------------------------------------------
//*                    Main of the program
//*----------------------------------------------------------------------------
int main() {

//*--- Initialize the board

  sleep_ms(200);
  stdio_init_all();
  sleep_ms(200);

//*--- Force clk_sys=270 MHz to start
  set_sys_clock_khz(270000, true);

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);

//*--- Initialize the board resources to be used by this program

  gpio_init(TXSW);
  gpio_set_dir(TXSW, GPIO_IN);

  gpio_init(TX);
  gpio_set_dir(TX, GPIO_OUT); //TX →　1, RX →　0 (for Driver switch)

//*--- Initialize the USB CDC

  tud_init(BOARD_TUD_RHPORT);

//*--- Initialize the quadrature oscillator 

  quad_osc_t osc;
  if (!quad_init(&osc, pio0, 0)) {
    blink_code(1); // quad_init falló (por ejemplo pio_add_program < 0)
  }

//*--- Starts at 3.5 MHz without any optimization, just to start the clock

  quad_solution_t sol;
  if (!quad_start(&osc, 3573000u, false, &sol)) {
    blink_code(2); // quad_start falló
  }

//*--- Prepare for the main loop, the PIO clock is running

  bool keypress=false;
  bool blink=false;
  uint8_t b=0;
  uint32_t f=3573000u;
  uint32_t t=to_ms_since_boot(get_absolute_time());

//*--- At this point a I/Q clock signal should be present at GPIO14 and GPIO15  
  
  while (true) {

    //*--- This is a debug logic, detect if the TX switch has been pressed
    if (!gpio_get(TXSW)) {    

       //*--- Turn on the TX LED and flag the condition, also send a debug message
       gpio_put(TX,1);
       keypress=true;
       cdc_printf("TX key pressed\n");

       //*--- Wait till the switch is released, execute the USB task while waiting
       while(!gpio_get(TXSW)) {
         tud_task();
       }
       //*--- Upon release turn off the TX LED
       gpio_put(TX,0);

    }
    //*--- just blink the TX led every second to note the waiting pattern

    if (to_ms_since_boot(get_absolute_time())-t > 1000) {
        t=to_ms_since_boot(get_absolute_time());
        blink=!blink;
        gpio_put(TX,blink);
    }

    //*--- If the switch has been pressed and released then advance the band
    //*--- just to test the optimization solution for several classic FT8 frequencies

    if (keypress) {
       keypress=false;
       cdc_printf("TX key released\n");      
       sleep_ms(100);

       switch(b) {
        case 0 : f =  3573000u; break;
        case 1 : f =  7074000u; break;
        case 2 : f = 14074000u; break;
        case 3 : f = 21074000u; break;
        case 4 : f = 28074000u; break;
       }
       b++;
       if (b>4) b=0;

       //*--- Change the clock frequency, in this case the flag forces the board not to
       //*--- reboot when doint it
       
       //quad_set_frequency(&osc, f, false, &sol);
      if (quad_set_frequency(&osc, f, false, &sol)) {
         tud_task();
         dump_solution("SET", &sol, osc.pio, osc.sm);
         tud_task();
      } else {
         cdc_printf("Error while calling quad_set_frequency()\n");
      }

      for(int n=0;n<10;n++) {tud_task();}

    }
    tud_task(); // TinyUSB device task

}
}
