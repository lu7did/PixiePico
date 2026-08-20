/*
 * =======================================================================================
 * ADX-ddsPIO
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * new generation rp2040 ADX based digital transceiver 
 * 
 * This is mainly an integration effort with some new code developed for this project,
 * some unique features has been developed for this firmware as well such as the
 * quadrature digital frequency synth.
 *  
 * The integration effort is being built on top of previous work from many parties,
 * including myself as follows:
 * 
 *----------------------------------------------------------------------------
 * Version 1.0
 * - Initial release
 * - Basic TX/RX funcionality
 * - Basic board management and control
 * - Software DCO
 * - USB Audio
 *----------------------------------------------------------------------------
 * Version 1.1
 * - EEPROM support (save configuration)
 * - CAT support (TS 2000 emulation)
 * - Dual Clock option, separated RX LO and TX RF OUT signals
 *----------------------------------------------------------------------------
 * Version 1.2
 * - Support for BFO at 465 KHz (superheterodyne version)
 * - Enhance performance of the Dual Clock option, separated RX/TX RF out
 *----------------------------------------------------------------------------
 * Version 1.3
 * - Implementation of digital quadrature frequency synth (quad)
 * - HW pin realigment to allow for the future use of I2C at pins 26/27
 *----------------------------------------------------------------------------
 
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
 *                       Libraries and Packages used                        *
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
  * 
 * pico-hf-oscillator rp2040 DDS firmware by  Roman Piksaykin (R2BDY):
 * Digital Controlled Oscillator for Raspberry Pi Pico
 * https://www.qrz.com/db/r2bdy
 *
 *----------------------------------------------------------------------------
 * 
 * ADX transceiver original hardware and firmware design 
 * ADX_UNO_V1.1 - Version release date: 08/05/2022
 * by Barb(Barbaros ASUROGLU) - WB2CBA - 2022
 * https://github.com/WB2CBA/ADX
 * 
 *----------------------------------------------------------------------------
 * 
 * QP-7C_rp2040 digital mode transceiver with CAT control 
 * Copyright (C) 2023- Hitoshi Kawaji <je1rav@gmail.com>
 * https://github.com/je1rav/QP-7C_RP2040_CAT
 *
 *----------------------------------------------------------------------------
 * 
 * ft8_lib FT8 library
 * Copyright (C) 2018 by Karliss Goba (YL3JG)
 * https://github.com/kgoba/ft8_lib
 * 
 * This library is not part of the ADX-ddsPIO firmware as no actual FT8
 * coding is generated on the board, however it's a fundamental tool for
 * the development and integration process, without it the project would have
 * been considerably more difficult
 *
 *----------------------------------------------------------------------------
 * 
 * ADX-rp2040 digital transceiver using the rp2040 (firmware and hardware)
 * Copyright (C) 2024- Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * Original porting of the ADX Arduino based firmware to the rp2040 architecture
 * Main architecture for board management ported from this version
 *
 *----------------------------------------------------------------------------
 * 
 * And, of course....
 * 
 * The WSJT-X authors, who developed a very interesting and novel communications protocol
 * The details of FT4 and FT8 procotols and decoding/encoding are described here:
 * https://physics.princeton.edu/pulsar/k1jt/FT4_FT8_QEX.pdf
 *
 * This program (firmware) do not generate nor decode FT8 signals, therefore
 * no part of the WSJT-X code has been involved. But the board is intended to
 * be used by a program such as WSJT-X, therefore the acknowledgement is in 
 * order
 *----------------------------------------------------------------------------
 * 
 * And, of course....
 * 
 * Hans Summers (G0UPL) who in a sense started all this over with his initial
 * (superb) QCX design moving forward the frontier of the homebrew and 
 * returning it to the hands of radio amateurs, after few decades of opting
 * between either minimalists design or commercial devices he develop the basis
 * for a revolution that allowed hams to create a functional and capable
 * station being build with homebrew spirit.
 * 
 *----------------------------------------------------------------------------
 *
 * And, of course ....
 * 
 * Guido (PE1NNZ) who adapted a concept to use SSB generation by the 4th method
 * which is to split the components of amplitude and phase to reintroduce it
 * later and was able to cram down it to be able to run on a very small
 * controller (Arduino) which allow the QCX design to be span into a new 
 * generation of affordable equipment, the uSDX class transceivers.
 * 
 * ----------------------------------------------------------------------------
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*------------------------------------------------------------------------------------------------*
 |  IDENTIFICATION DIVISION.                                                                      |
 |  (just a programmer's joke)                                                                    |
 *------------------------------------------------------------------------------------------------*/
#define PROGNAME "ADX-ddsPIO"
#define AUTHOR "Dr. Pedro E. Colla (LU7DZ)"
#define VERSION  "1.3"
#define BUILD     "00"

#define BOOL2CHAR(x)  (x==true ? "True" : "False")
//*==============================================================================================*
//*                                  Build environment                                           *
//*==============================================================================================*
#define  RP2040Z  1
//#define  PICO    1
//#define   PICOW  1 


//*==============================================================================================*
//*                                Configuration definitions                                     *
//*==============================================================================================*
#define  DEBUG     1
#define  EEPROM    1   
//#define  CAT       1    
#define  BOOTSYNC  1
#define  SUPERHET  1
#define  DUALCLK   1
//#define QUAD 1
//*==============================================================================================*
//*                                Configuration consistency rules                               *
//*==============================================================================================*
#ifdef CAT                      //If CAT enabled USB can not be used to debug
#undef DEBUG
#endif //CAT  

#ifdef QUAD                     //If Quadrature oscillator activated all other clocks disabled
#undef DUALCLK
#undef SUPERHET
#endif //QUAD 

//*==============================================================================================*
//*                                  Includes and Source Libraries                               *
//*==============================================================================================*
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "bsp/rp2040/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <hardware/watchdog.h>    //watchdog (not used yet)
#include "usb_audio.h"
#include <inttypes.h>
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "pico/stdio/driver.h"
#include "piodco.h"
#include "../build/dco2.pio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#ifdef SUPERHET
#include "BFO.pio.h"
#endif //SUPERHET

#ifdef QUAD
#include "quad.pio.h"
#include "quad.h"
#endif //SUPERHET

#ifdef EEPROM
#include "EEPROM.h"
#endif //EEPROM

#ifdef PICOW
#include "pico/cyw43_arch.h"
#endif //PICOW

//*==============================================================================================*
//*                             Macros and Structures                                            *
//*==============================================================================================*
#ifdef DEBUG
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
#else  //!DEBUG
#define cdc_printf(...) (void)0
#endif //DEBUG    

//*==============================================================================================*
//*                             Constants and parameters                                         *
//*==============================================================================================*
#define AUDIOSAMPLING    48000            // USB Audio sampling frequency (fixed)
#define PLL_SYS_MHZ        270            // RP2040 System Clock (MHz)  
#define PLL_SYS_MHZ_PLUS   290            // RP2040 System Clock (MHz) --OVERCLOCK--
#define GEN_FRQ_HZ    14074000L           // Generator Frequency (in Hz)
#define FT8_BASE_HZ       1000L           // FT8 base frequency (in Hz) <Not used>
#define FREQ_BFO        446400L           // BFO Frequency 

#define SLOT                  3
#define NBANDS                7
#define NMODES                4

//*==============================================================================================*
//*                                  Hardware configuration                                      *
//*==============================================================================================*

#if defined(PICO) || defined(RP2040Z)
#define PICO_DEFAULT_LED_PIN 25
#endif //PICO || RP2040Z


#define pin_A0               28U          //pin for ADC (A2)
#define pin_SW                3U          //pin for freq change switch (D10,input)

#define RFOUT                13           //RF out pin
#define RFLO                 14           //RF out Receiver LO

#ifdef SUPERHET
#define RFIF                 15           //RF out Receiver IF (465 KHz)

#endif //SUPERHET

#define SDA                  26           //I2C SDA (Data) bus
#define SCL                  27           //I2C SCL (Clock) bus

/*----
   Output control lines
*/
#define RXSW                  2  //RXSW Switch (RX/TX control)
#define TXA                   9  //OE signal to driver

/*---
   LED
*/

#define TX                    3  //TX LED
#define FT8                   4  //FT8 LED
#define FT4                   5  //FT4 LED
#define JS8                   6  //JS8 LED
#define WSPR                  7  //WSPR LED

/*---
   Switches
*/    
#define TXSW                  8  //RX-TX Switch
#define UP                   10  //UP Switch
#define DOWN                 11  //DOWN Switch
#define BEACON               12  //BEACON Jumper
#define SYNC                 13  //Time SYNC Switch


//*==============================================================================================*
//*                                  Global Memory Areas                                         *
//*==============================================================================================*
uint32_t frqFT8  = GEN_FRQ_HZ;
uint32_t frqbfo  = FREQ_BFO;
char hi[80];
uint8_t marker=0;

//*--- Control block of PIO running the DCO
PioDco DCO; /* External in order to access in both cores. */
PioDco DCO2;

//*--- for ADC offset at transceiver
int32_t adc_offset = 0;   

uint64_t audio_freq_prev=0.0;

int Tx_Status = 0; // 0=RX, 1=TX
int Tx_Start = 0;  // 0=RX, 1=TX
int not_TX_first = 0;
uint32_t Tx_last_mod_time;
uint32_t Tx_last_time;
uint32_t push_last_time;  // to detect the long puch for frequency change by push switch

//*--- for determination of Audio signal frequency 
int16_t mono_prev=0;  
int16_t mono_preprev=0;  
float delta_prev=0;
int16_t sampling=0;
int16_t cycle=0;
uint32_t cycle_frequency[136];

//*--- for USB Audio
int16_t monodata[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4];
int16_t adc_data[CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ / 2];
int16_t pcCounter;

int audio_read_number=0;

//*--- for CDC buffering
char cdc_read_buf[64];
char cdc_write_buf[64];

/*---- Define RTC structure but do not expect a RTC to be present */

datetime_t tcpu = {
    .year  = 2026,
    .month = 01,
    .day   = 01,
    .dotw  = 4,  // 0 is Sunday, so 4 is Thursday
    .hour  = 0,
    .min   = 00,
    .sec   = 00
};

//*==============================================================================================*
//*                                  Prototypes                                                  *
//*==============================================================================================*
void core1_entry(void);
void cdc_write(char *, uint16_t);
uint32_t cdc_read(void);

int32_t adc(); 

// void adc_drain();

void transmitting(void);
void receiving(void);
void audio_data_write(int16_t,int16_t);
void cat(void);
void clearLED();
void blinkLED(uint8_t _gpio, uint8_t n, uint ms);

//*--- Define quadrature QDO working areas
#ifdef QUAD
quad_solution_t sol;
quad_osc_t      osc;
#endif //QUAD
//*==============================================================================================*
//*                            Debug tools for Quadrature oscillator if defined                  *
//*==============================================================================================*

#if defined(DEBUG) && defined(QUAD)

static void read_sm_clkdiv(PIO pio, uint sm, uint16_t *di, uint8_t *df);
static uint32_t calc_fout_est(uint32_t clk_sys_hz, uint32_t N_q8_8);
static void dump_solution(const char *tag,const quad_solution_t *s,PIO pio, uint sm);

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

#endif //DEBUG

//*==============================================================================================*
//*                                         BAND SELECT AND MANAGEMENT                           *
//*==============================================================================================*
// ADX can support up to 4 bands on board. Those 4 bands needs to be assigned to Band1 ... Band4
// from supported 8 bands.
// To change bands press UP and DOWN simultaneously. 
// The Band LED will flash 3 times briefly and stay lit for the stored band. also TX LED will be lit to indicate
// that Band select mode is active. Now change band bank by pressing SW1(<---) or SW2(--->). When
// desired band bank is selected press TX button briefly to exit band select mode.
// Now the new selected band bank will flash 3 times and then stored mode LED will be lit.
// TX won't activate when changing bands so don't worry on pressing TX button when changing bands in
// band mode.
// Assign your prefered bands to B1,B2,B3 and B4
// Supported Bands are: 80m, 40m, 30m, 20m,17m, 15m, 10m
//*----------------------------------------------------------------------------------------------*
int slot[4]   = {40,30,20,10};
int Band_slot = SLOT;                  //This is the default band Band1=1,Band2=2,Band3=3,Band4=4
int Band      =   20;                  //This is the default band
int mode      =    4;                  //Default mode is FT8

long unsigned int Bands[NBANDS][NMODES] = {
                                          { 3568600, 3578000, 3575000, 3573000},
                                          { 7038600, 7078000, 7047500, 7074000},
                                          {10138700,10130000,10140000,10136000},
                                          {14095600,14078000,14080000,14074000},
                                          {18104600,18104000,18104000,18100000},
                                          {21094600,21078000,21140000,21074000},
                                          {28124600,28078000,28180000,28074000}};
//*----------------------------------------------------------------------------------------------*
#ifdef SUPERHET
/*----------------------------------------------------------------------------*/
/* Create PIO configuration for a square wave clock                           */
/*----------------------------------------------------------------------------*/
static void pio_square_wave(PIO pio, uint sm, uint offset, uint pin, float target_hz) {

    // Con este PIO: 2 instrucciones por período => f_sm = 2 * f_out
    float f_sm = 2.0f * target_hz;

    uint32_t clk_sys_hz = clock_get_hz(clk_sys);
    float div = (float)clk_sys_hz / f_sm;

    pio_sm_config c = BFO_program_get_default_config(offset);

    // Usamos SET para escribir "pins", por eso hay que setear el pin como SET pin
    sm_config_set_set_pins(&c, pin, 1);
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

    // Divisor fraccional del SM
    sm_config_set_clkdiv(&c, div);

    // Arranca en 0 para que sea determinístico
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
    uint32_t fx=(uint32_t)target_hz;

    cdc_printf("clk_sys=%u Hz, target=%ld Hz, div=%f\n",(unsigned)clk_sys_hz, fx, (double)div);
}
#endif //SUPERHET

#ifdef EEPROM
/*----------------------------------------------------------------------------*/
/* updateEEPROM                                                               */
/*----------------------------------------------------------------------------*/
void updateEEPROM() {

  EEPROMData eeprom;
  EEPROM_read(&eeprom);

  cdc_printf("Configuration      mode(%d) band(%d)\n",mode,Band_slot);
  cdc_printf("Read EEPROM ID(%d) mode(%d) band(%d)\n",eeprom.ID,eeprom.mode,eeprom.Band_slot);

  //*--- Update configuration datadata
  eeprom.ID = 0x01;
  eeprom.mode = (uint8_t)mode;
  eeprom.Band_slot = (uint8_t)Band_slot;

  cdc_printf("Write EEPROM ID(%d) mode(%d) band(%d)\n",eeprom.ID,eeprom.mode,eeprom.Band_slot);
  EEPROM_write(&eeprom);

}
#endif //EEPROM
/*----------------------------------------------------------------------------*/
/* Convert slot to band                                                       */
/*----------------------------------------------------------------------------*/
int slot2Band(int s) {
  if (s < 1 || s > 4) {
    s = 4;
  }
  int b=slot[s-1];
  cdc_printf("Slot(%d) --> Band(%d)\n", s, b);
  return b;
}
/*----------------------------------------------------------------------------*/
/* Convert band number to index                                               */
/*----------------------------------------------------------------------------*/
int band2idx(int b){

int i=0;

switch(b) {
  case 80: i=0; break;
  case 40: i=1; break;
  case 30: i=2; break;
  case 20: i=3; break;
  case 17: i=4; break;
  case 15: i=5; break;
  case 10: i=6; break;

  default:
    i=6; break;
}
cdc_printf("band(%d) idx(%d)\n", b, i);
return i;

}
/*----------------------------------------------------------------------------*/
/* Assign band                                                                */
/*----------------------------------------------------------------------------*/
void Mode_assign() {

  cdc_printf("Assigning mode(%d) for Band(%d)\n", mode, Band);
  int b=band2idx(Band);
  frqFT8=Bands[b][mode-1];
  
  clearLED();

  //adc_drain();
  
  switch(mode) {
     case 1: gpio_put(WSPR,true);break;
     case 2: gpio_put(JS8,true);break;
     case 3: gpio_put(FT4,true);break;
     case 4: gpio_put(FT8,true);break;
  }

#ifdef EEPROM
  
  EEPROMData eeprom;
  EEPROM_read(&eeprom);
  if (eeprom.ID == 0x01) {
     eeprom.mode = (uint8_t)mode;
     eeprom.Band_slot = (uint8_t)Band_slot;
     EEPROM_write(&eeprom);
     cdc_printf("Updated EEPROM mode(%d) slot(%d)\n",mode,Band_slot);
  } 

#endif //EEPROM

  cdc_printf("transceiver mode mode(%d) Band(%d) index(%d) freq(%ld)\n", mode, Band, b, frqFT8);

}

/*----------------------------------------------------------------------------*/
/* Assign Band                                                                */
/*----------------------------------------------------------------------------*/
void Band_assign() {

  clearLED();
  Band=slot2Band(Band_slot);

  switch(Band_slot) {
     case 0: blinkLED(WSPR,3,100); break;
     case 1: blinkLED(JS8,3,100); break;
     case 2: blinkLED(FT4,3,100); break;
     case 3: blinkLED(FT8,3,100); break;
  }
    
  Mode_assign();
  cdc_printf("band_slot=%d mode=%d band=%d\n",Band_slot, mode, Band);
}
//*==============================================================================================*
//*                          Services and board management functions                             *
//*==============================================================================================*
//*----------------------------------------------------------------------------*/
//* Blink a LED a given number of times with a given interval to signal things */
//*----------------------------------------------------------------------------*/
void blinkLED(uint8_t _gpio, uint8_t n, uint ms)
{
    for (int i=0;i<n;i++) {
        uint32_t t=to_ms_since_boot(get_absolute_time());
        gpio_put(_gpio,1);

        while ( (to_ms_since_boot(get_absolute_time())-t) < ms) {}
        
        t=to_ms_since_boot(get_absolute_time());
        gpio_put(_gpio,0);
        
        while ( (to_ms_since_boot(get_absolute_time())-t) < ms) {}
    }
}
/*----------------------------------------------------------------------------*/
/* Manage differences on default led according with pico models               */
/*----------------------------------------------------------------------------*/
void defaultLED(bool v){

#if defined(PICO) || defined(RP2040Z) 
gpio_put(PICO_DEFAULT_LED_PIN, v);
#else
cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, v); 
hi[0]=(uint8_t)v;
#endif //RP2040Z || PICO

}
/*----------------------------------------------------------------------------*/
/* Clear all LEDS                                                             */
/*----------------------------------------------------------------------------*/
void clearLED() {

  gpio_put(FT8, 0);
  gpio_put(FT4, 0);
  gpio_put(JS8, 0);
  gpio_put(WSPR, 0);
  gpio_put(TX,0);  
  
}
/*----------------------------------------------------------------------------*/
/* Test a single button                                                       */
/*----------------------------------------------------------------------------*/
bool testButton(uint _gpio) {

    if (!gpio_get(_gpio)) {
       uint32_t t=to_ms_since_boot(get_absolute_time());
       while ( ( to_ms_since_boot(get_absolute_time()) -t) < 100);
       if (!gpio_get(_gpio)) { 
          return false;
       }
    }
    return true;

}
/*----------------------------------------------------------------------------*/
/* Control transmitter TX/RX status, TX LED and RX enable signals             */
/*----------------------------------------------------------------------------*/
void setTX(bool state) {
      

    if (state) {

       uint32_t f = frqFT8;

       #ifdef QUAD
          quad_stop(&osc);
          PioDCOStart(&DCO);
       #endif //QUAD

       PioDCOSetFreq(&DCO, f, 0U);

       gpio_put(RXSW, 0);                   //(RXSW=1 enable Ant to RX, otherwise blocks it)
       gpio_put(TX, 1);                     //Turn on the TX led
       gpio_put(TXA, 0);                    //(TXA=0 enable OE of 74244)
       defaultLED(true);

      } else {

        uint32_t f = frqFT8;

        PioDCOSetFreq(&DCO, f, 0U);

        #ifdef QUAD
           PioDCOStop(&DCO);
           quad_start(&osc, f, false, &sol);
           dump_solution("<1>", &sol, osc.pio , osc.sm);

        #endif //QUAD

        cycle = 0;
        sampling = 0;
        mono_preprev = 0;
        mono_prev = 0; 
        gpio_put(TXA, 1);                  //Disable OE of the 74244 driver
        gpio_put(RXSW, 1);                 //Set RX mode, connect Ant to receiver
        gpio_put(TX, 0);                   //Turn off the TX Led
        defaultLED(false);
    }
}   

/*----------------------------------------------------------------------------*/
/* Place transmitter in TX manually if the button TX is pressed               */                                         
/*----------------------------------------------------------------------------*/
void ManualTX() {

  if (testButton(TXSW)) return;

  setTX(true);
  cdc_printf("Manual TX activated\n");

  while(!testButton(TXSW));
  setTX(false);

  cdc_printf("Manual TX deactivated\n");

}
/*----------------------------------------------------------------------------*/
/* Change the band                                                            */                                         
/*----------------------------------------------------------------------------*/
void Band_Select() {


  gpio_put(TX,true);
  clearLED();
  
  blinkLED(FT8,3,1000);   

  while (true){  
 
     clearLED();
     switch(Band_slot) {
       case 1: gpio_put(FT8,true); break;
       case 2: gpio_put(FT4,true); break;
       case 3: gpio_put(JS8,true); break;
       case 4: gpio_put(WSPR,true); break;
  }
  
  if ((!testButton(UP)) && (testButton(DOWN))) {    //UP Button pressed, decrease band slot
     Band_slot--;
     if (Band_slot<1) {
        Band_slot=4;
     }
     while(testButton(UP)==false);
     cdc_printf("<UP> Band_slot=%d\n", Band_slot);
  }

  if ((testButton(UP)) && (!testButton(DOWN))) {    //DOWN Button pressed, increase
     Band_slot++;
     if (Band_slot>4) {
        Band_slot=1;
     }
     while(testButton(DOWN)==false);
     cdc_printf("<DOWN> Band_slot=%d\n", Band_slot);
  }

  if (!testButton(TXSW)) {
     gpio_put(TX,false);
     Band_assign();
     cdc_printf("completed set Band_slot=%d\n", Band_slot);
     return;
  }
}

}

/*----------------------------------------------------------------------------*/
/* Check buttons                                                              */                                         
/*----------------------------------------------------------------------------*/
void checkButtons() {

  /*------------------------------------------------
     Explore and handle interactions with the user
     thru the UP/DOWN or TX buttons
     Only TX button is operational if CAT is active
   -------------------------------------------------*/

  /*----
     UP(Pressed) && DOWN(Pressed) && !Transmitting --> Start band selection mode
  */
#ifndef CAT
  if ((!gpio_get(UP)) && (!gpio_get(DOWN)) && (Tx_Status==0)) {
     if(!testButton(UP) && !testButton(DOWN)){
       while(!testButton(UP) && !testButton(DOWN));
       Band_Select();
     }
  }
#endif //!CAT  

  /*----
     UP(Pressed) && DOWN(!Pressed) and !Transmitting --> Increase mode in sequence
  */
  #ifndef CAT
  if (!gpio_get(UP) && gpio_get(DOWN) && Tx_Status == 0) {
     if (!testButton(UP) && testButton(DOWN)){
        mode=mode-1;
        if (mode<1) mode=4;
        Mode_assign();
        while(!testButton(UP));
        PioDCOSetFreq(&DCO,frqFT8,0U);
    
        #ifdef QUAD
        quad_set_frequency(&osc, frqFT8, false, &sol);  
        dump_solution("<3>", &sol, osc.pio , osc.sm);
        #endif //QUAD

     }
  }
  #endif //!CAT

  /*----
     UP(!Pressed) && DOWN(Pressed) && !Transmitting --> decrease mode in sequence
  */
  #ifndef CAT
  if (gpio_get(UP) && !gpio_get(DOWN) && Tx_Status == 0) {
     if (testButton(UP) && !testButton(DOWN)){
        mode=mode+1;
        if (mode>4) mode=1;
        Mode_assign();
        while(!testButton(DOWN));

        PioDCOSetFreq(&DCO,frqFT8,0U);

        #ifdef QUAD
        quad_set_frequency(&osc, frqFT8, false, &sol);
        dump_solution("<4>", &sol, osc.pio , osc.sm);
        #endif //QUAD

     }
  }
  #endif //!CAT

  /*----
     If the TX button is pressed then activate the transmitter until the button is released
  */
  if (!gpio_get(TXSW) && Tx_Status == 0){
     if (!testButton(TXSW)) {
        ManualTX();
     }
  }

}
//*===============================================================================================*/
//*                                                 CORE 1 PROCESSOR                              */
//* This is the code dedicated in CORE1 to work out the DCO, deal with a precise real-time task   */                                                           */
//*===============================================================================================*/
void core1_entry()
{
    cdc_printf("Core 1: DCO worker started.\n");

    //*--- Set the DCO initial (default) frequency

    uint32_t f = frqFT8 + 0U;
    PioDCOStart(&DCO);
    PioDCOSetFreq(&DCO, f, 0U);

    //*--- Run the main DCO algorithm. It spins forever. */

    PioDCOWorker2(&DCO);
}
//*==============================================================================================*
//*                                  Board management                                            *
//*==============================================================================================*

//*----------------------------------------------------------------------------*/
//* Setup I/O for the ADX board controls (LED, switches and jumpers            */
//*----------------------------------------------------------------------------*/
void ADXsetup(){

    gpio_init(RXSW);
    gpio_set_dir(RXSW, GPIO_OUT);   
    gpio_put(RXSW, 1); //Set RX mode

    gpio_init(FT8);
    gpio_set_dir(FT8, GPIO_OUT);   
    gpio_put(FT8, 0); //Turn OFF FT8 LED
 
    gpio_init(FT4);
    gpio_set_dir(FT4, GPIO_OUT);   
    gpio_put(FT4, 0); //Turn OFF FT4 LED

    gpio_init(JS8);
    gpio_set_dir(JS8, GPIO_OUT);
    gpio_put(JS8, 0); //Turn OFF JS8 LED

    gpio_init(WSPR);
    gpio_set_dir(WSPR, GPIO_OUT);
    gpio_put(WSPR, 0); //Turn OFF WSPR LED    

    gpio_init(TX);
    gpio_set_dir(TX, GPIO_OUT);
    gpio_put(TX, 0); //Set TX mode (off)


    gpio_init(TXA);
    gpio_set_dir(TXA, GPIO_OUT);
    gpio_put(TXA, 1);                      //Set TX mode (74244 off)

//*--- (Input switches)

    gpio_init(TXSW);
    gpio_set_dir(TXSW, GPIO_IN);

    gpio_init(UP);
    gpio_set_dir(UP, GPIO_IN);

    gpio_init(DOWN);
    gpio_set_dir(DOWN, GPIO_IN);    

    gpio_init(SYNC);
    gpio_set_dir(SYNC, GPIO_IN);
    
    gpio_init(BEACON);
    gpio_set_dir(BEACON, GPIO_IN);

    #ifdef EEPROM
    
    //*--- update EEPROM if not initialized yet
    EEPROMData eeprom;
    EEPROM_read(&eeprom);

    if (eeprom.ID != 0x01) {
       eeprom.ID = 0x01;
       eeprom.mode = (uint8_t)mode;
       eeprom.Band_slot = (uint8_t)Band_slot;
       EEPROM_write(&eeprom);
       marker=1;
       sleep_ms(10);
    } else {
       mode=eeprom.mode;
       Band_slot=eeprom.Band_slot;
    }
        
    #endif //EEPROM

    //*--- End of ADX control board initialization
    cdc_printf("ADX I/O control board initialized\n");

}

//*===============================================================================================*/
//*                                                 CORE 0 PROCESSOR                              */
//* This is the code dedicated to manage USB, board LED, switches and the transceiver FSM         */                                                           */
//*===============================================================================================*/


//*----------------------------------------------------------------------------*/
//*                         This is the main                                   */
//* First all relevant hardware and control structures are initialized, then   */
//* an infinite loop is entered which actually manages the transceiver FSM     */
//*----------------------------------------------------------------------------*/
int main(void)
{

  stdio_init_all();

  //*--- Overclock the board a little

  const uint32_t clkhz = PLL_SYS_MHZ * 1000000L;
  set_sys_clock_khz(clkhz / 1000L, true);

  //*--- Initialize stdio and other hardware elements

  stdio_init_all();
  sleep_ms(500);
 
  #ifdef BOOTSYNC

  //*----------- Setup RTC, this is only used to sync seconds   ---------------*
  rtc_init();
  rtc_set_datetime(&tcpu);
  
  #endif //BOOTSYNC

  //*--- define the DEFAULT (board) LED

  #if defined(PICO) || defined(RP2040Z)

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  defaultLED(true);

  #else

  cyw43_arch_init(); 

  #endif //!PICOW

  #if defined(PICO) || defined(RP2040Z)
  ADXsetup();
  #endif //PICO

  //*---- Start the local 465 KHz BFO oscillator if enabled
  #ifdef SUPERHET
  
  PIO piobfo = pio1;
  uint sm = 0;
  uint offset = (uint) pio_add_program(piobfo, &BFO_program);
  float fbfo=(float)frqbfo*1.0f;
  pio_square_wave(piobfo, sm, offset, RFIF, fbfo);

  #endif //SUPERHET 

  //*--- Start the DCO (either single or dual clock)

  const uint32_t PIOclkhz = PLL_SYS_MHZ * 1000000L;
  cdc_printf("Core 1 started. DCO worker initializing...\n");

  //*fix* PioDCOInit(&DCO, RFOUT, PIOclkhz);
  //*--- Output is produced replicated at GPIO13 and GPIO14 
 
  #ifdef DUALCLK
  PioDCOInit(&DCO, RFOUT, PIOclkhz,true);
  #else
  PioDCOInit(&DCO, RFOUT, PIOclkhz,false);
  #endif //DUALCLK

  //*--- Start the quadrature oscilator

  #ifdef QUAD
  quad_init(&osc, pio1, 0);
  #endif //QUAD

  //*--- Start the USB service loop

  tud_init(BOARD_TUD_RHPORT);

  #ifdef BOOTSYNC
  bool synced=false;
  while (!gpio_get(TXSW)) {
      blinkLED(TX,1,500);
      rtc_set_datetime(&tcpu);
      synced=true;
  };

  if (synced){
     char datetime_str[64];
     rtc_get_datetime(&tcpu);
     datetime_to_str(datetime_str, sizeof(datetime_str), &tcpu);
     cdc_printf("Internal clock has been set to: %s\n", datetime_str);
  }

  #endif //BOOTSYNC

  
  //*--- Wireless library poll
  #ifdef PICOW
  cyw43_arch_poll(); 
  #endif //PICOW

  //*--- Initialize the ADX board

  cdc_printf("Core 1 started. DCO worker initializing...\n");

  //*--- Previous ADXSetup

  
  //*--- Sync time and define mode 
  #if defined(PICO) || defined(RP2040Z)
  Band_assign();
  #endif //PICO || RP2040Z

  //*--- GPIO setting for the ADC control (receiver) 
  gpio_init(pin_A0);
  gpio_set_dir(pin_A0, GPIO_IN); //ADC input pin

  //*--- Force TX to be off
  gpio_put(TXA,1);
  
    //*--- Turn off the DEFAULT pin and launch the Core1 process
  
  #if defined(PICO) || defined(RP2040Z)
  defaultLED(false);
  #endif //PICO

  cdc_printf("launching DCO worker on core 1...\n");
  multicore_launch_core1(core1_entry);
  sleep_ms(500);
  
  #ifdef QUAD

  //*--- Start the oscillator
  quad_start(&osc, frqFT8, false, &sol);
  dump_solution("<0>", &sol, osc.pio , osc.sm);
  
  #endif //QUAD

  //*--- ADC (receiver) initialization
  
  pcCounter=0;
  adc_init();
  adc_select_input(0);                        // ADC input pin A0
  adc_run(true);                              // start ADC free running
  adc_set_clkdiv(249.0);                      // 192kHz sampling  (48000 / (249.0 +1) = 192)
  adc_fifo_setup(true,false,0,false,false);   // fifo
  cdc_printf("ADC receiver system initialized\n");


  //*--- USB Audio initialization (initialization of monodata[])
  for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4); i++) {
    monodata[i] = 0;
  }

//*--- Calibrate the ADC offset, read the DC offset value (ADC input)
  
  sleep_ms(100);
  adc_fifo_drain ();
  adc_offset = adc();

//*--- Start the quadrature digital oscilator (QDO) if configured

  #ifdef QUAD
  quad_init(&osc, pio1, 0);
  #endif //QUAD

  //*--- Get time for future synchronization

  Tx_last_mod_time=to_ms_since_boot(get_absolute_time());
  
  //*--- Enter the infinite loop

  //*----------------------------------------------------------------------------*/
  //*                    This is the main service loop                           */
  //*----------------------------------------------------------------------------*/
  while (true)
  {
  
    //*--- TUD (TinyUSB Dispatcher call)
    tud_task();        // TinyUSB device task

    //*--- Check for buttons and other status changes 
    #if defined(PICO) || defined(RP2040Z)
    checkButtons();
    #endif //PICO

    //*--- CAT (using CDC, not implemented yet
    cat(); // remote control (simulating Kenwood TS-2000) 


    if (Tx_Start==0) {                     //Tx_Start=0 (RX) || Tx_Start=1 (TX)
        receiving();
    } else {
        transmitting();
    }
    
  }
}
/*===================================[ End of Main]=================================================*/
//*----------------------------------------------------------------------------*/
//*  This procedure controls the main transmission cycle of the transceiver    */
//*  The actual FSK data is created by an external program (i.e. WSJT-X) and   */
//*  sent to this firmware over USB audio
//*----------------------------------------------------------------------------*/
void transmitting(){
  
  uint64_t audio_freq;


  //*--- Check if there are digital audio samples to read, if so start the transmission cycle

  if (audio_read_number > 0) {

    //*--- Process the samples

    for (int i=0;i<audio_read_number;i++){
      
      int16_t mono = monodata[i];
 
      //*--- Detect an upward moving zero crossing

      if ((mono_prev < 0) && (mono >= 0)) {
         int16_t difference = mono - mono_prev;
         float delta = (float)mono_prev / (float)difference;
         float period = ((float)1.0 + delta_prev) + (float)sampling - delta;
         audio_freq = (uint64_t)(AUDIOSAMPLING/(double)period); // in Hz    

         //*--- Compute the period and FSK frequency, discard if above or below limits
         if ((audio_freq > 200) && (audio_freq < 3000)){
            cycle_frequency[cycle]=(uint32_t)audio_freq;
            cycle++;
         }
       
         delta_prev = delta;
         sampling = 0;
         mono_preprev = mono_prev;
         mono_prev = mono;     

      } else {

        //*--- This is not a zero crossing, ignore samples

        sampling++;
        mono_preprev = mono_prev;
        mono_prev = mono;
      }
    }

    //*--- When enough data has been collected (10 mSec) an average is computed to compensate for errorr

    if ((cycle > 0) && ((to_ms_since_boot(get_absolute_time()) - Tx_last_mod_time) > 10)){      //inhibit the frequency change faster than 20mS
       audio_freq = 0;
       for (int i = 0;i < cycle;i++){
          audio_freq += cycle_frequency[i];
       }
       audio_freq = audio_freq / (uint64_t)cycle;
       uint32_t f = frqFT8 + (uint32_t)audio_freq;

       //*--- as the FSK frequency has been detected change the DCO accordingly

       PioDCOSetFreq(&DCO, f, 0U);
       
       cdc_printf("FSK(%" PRIu64 ") Hz\n ",audio_freq);

       //*--- and initialize next averaging cycle

       cycle = 0;
       Tx_last_mod_time = to_ms_since_boot(get_absolute_time()); ;
    }

    //*--- More cycles needs to be averaged, continue

    not_TX_first = 1;
    Tx_last_time = to_ms_since_boot(get_absolute_time());
  
  } else { 

    //*--- No USB audio has been detected for a while, wait 100 mSecs and declare the frame to be terminated

    if ((to_ms_since_boot(get_absolute_time()) - Tx_last_time) >= 100 && Tx_Start==1)  {     // If USBaudio data is not received for more than 50 ms during transmission, the system moves to receiving. 
      cdc_printf("End of FT8 transmission\n");
      Tx_Start = 0;
      setTX(false);

      //*--- Prepare for next cycle

      cycle = 0;
      sampling = 0;
      mono_preprev = 0;
      mono_prev = 0;     

      //*--- Return the DCO frequency to the base in order to operate as a receiver

      return;
    }
  } 
  audio_read_number = USB_Audio_read(monodata);
}
//*----------------------------------------------------------------------------*/
//*                    This is receiving functions                             */
//* While no data is being sent over USB Audio the RX signals are digitized and*/
//* sent over USB to the receiver program (external, likely WSJT-X)            */
//*                  THIS FUNCTION IS ONLY PARTIALLY IMPLEMENTED               */
//*----------------------------------------------------------------------------*/
void receiving() {

  audio_read_number = USB_Audio_read(monodata); // read in the USB Audio buffer to check the transmitting
  if (audio_read_number != 0) 
  {
    cdc_printf("Start of FT8 transmission\n");
    Tx_last_time=to_ms_since_boot(get_absolute_time());

    Tx_Start=1;
    setTX(true);

    return;
  }

  int16_t rx_adc = (int16_t)(adc() - adc_offset); //read ADC data (8kHz sampling)
  
  // write the same 6 stereo data to PC for 48kHz sampling (up-sampling: 8kHz x 6 = 48 kHz)
  
  for (int i=0;i<6;i++){
    audio_data_write(rx_adc, rx_adc);
  }
  
  return;

}
/*----------------------------------------------------------------------------*/
//*  Audio data is sent over USB (Receiver)                                   */
/*----------------------------------------------------------------------------*/
void audio_data_write(int16_t left, int16_t right) {
  
  if (pcCounter >= (48)) {                           //48: audio data number in 1ms
    USB_Audio_write(adc_data, pcCounter);
    pcCounter = 0;  
  }
  adc_data[pcCounter] = (int16_t)((left + right) / 2);
  pcCounter++;
}

//*-----------------------   Integrate AD/C functions before removal --------------------------------
void receive(){

  Tx_Status=0;


  // initialization of monodata[]
  
  for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4); i++) {
    monodata[i] = 0;
  } 
  
  // initialization of ADC and the data write counter
  pcCounter=0;
  adc_fifo_drain ();                     //initialization of adc fifo
  adc_run(true);                         //start ADC free running


}

//*-----------------------   Integrate AD/C functions before removal --------------------------------
void adc_drain(){

  adc_fifo_drain ();
  adc_offset = adc();
  push_last_time = 0;

}
//*---------------------------------------------------------------------------------*/
//*                        ADC Sub-System                                           */
//* The ADC sub-system samples the mixer output of the receiver and sent it over USB*/
//*---------------------------------------------------------------------------------*/
int32_t adc() {
  
  int32_t adc = 0;
  for (int i=0;i<24;i++){             // 192kHz/24 = 8kHz
    adc += adc_fifo_get_blocking();   // read from ADC fifo
  }  
  return adc;
}
//*---------------------------------------------------------------------------------*/
//*                        CAT Sub-System                                           */
//* Receives CAT commands over USB Serial emulation (CDC) and changes the           */
//* operation of the transceiver accordingly                                        */
//* The CAT protocol being used is for a Kenwood TS-2000                            */
//* original: "ft8qrp_cat11.ico" from https://www.elektronik-labor.de/HF/FT8QRP.html*/
//* with some modifications mainly to be adapted to C language made by Hitoshi      */
//*---------------------------------------------------------------------------------*/
void cat(void) 
{  

#ifdef CAT

  char receivedPart1[40];
  char receivedPart2[40];    
  char command[3];
  char command2[3];  
  char parameter[38];
  char parameter2[38]; 
  char sent[42];
  char sent2[42];

  sprintf(receivedPart1,"%s","");
  sprintf(receivedPart2,"%s","");
  
  //*--- Receive and decode the CAT command

  uint8_t received_length = (uint8_t)cdc_read(); 
  if (received_length == 0) return;
  for (int i = 0;i<received_length;i++){                   //to Upper case
    if('a' <= cdc_read_buf[i] && cdc_read_buf[i] <= 'z'){
        cdc_read_buf[i] = cdc_read_buf[i] - ('a' - 'A');
    } 
    if (cdc_read_buf[i] == '\n') {
      received_length--;              //replace(from "\n" to "")
    }
  }

  //*--- Parse the CAT command

  char data[64];
  int bufferIndex = 0;
  uint16_t part1_length = 0;
  uint16_t part2_length = 0; 
  
  for (int i = 0; i < received_length; ++i)
  {
    if (cdc_read_buf[i] != ';')
    {
      data[i] = cdc_read_buf[i];
    }
    else
    {
      if (bufferIndex == 0)
      { 
        for(int ii = 0; ii < i; ++ii) {
          receivedPart1[ii] = data[ii];
        }
        part1_length = (uint16_t)i;
        bufferIndex++;
      }
      else
      {  
        for(int ii = part1_length + 1; ii < i; ++ii) {
          receivedPart2[ii-part1_length-1] = data[ii];
        }
        part2_length = (uint16_t)(i - 1)- part1_length; 
        bufferIndex++;
      }
    }
  }

  //*--- Operate CAT commands

  strncpy(command, receivedPart1, 2); 
  command[2] = '\0'; 
  if (part1_length > 2){
    strncpy(parameter, receivedPart1+2, part1_length - 2); 
    parameter[part1_length - 2] = '\0';
  }
  if (bufferIndex == 2){

//*- fix    
    //strncpy(command2, receivedPart2, 2);
    snprintf(command2, sizeof command2, "%.2s", receivedPart2);
//*- end of fix 

    command[2] = '\0'; 
    strncpy(parameter2, receivedPart2+2, part2_length -2);
    parameter2[part2_length - 2] = '\0';
  }

  if (strcmp(command,"FA")==0){          
    if (sizeof(part1_length) <= 2)
    {          
      long int freqset = strtol(parameter, NULL, 10);
      if (freqset >= 1000000 && freqset <= 54000000){

        frqFT8 = (uint32_t)freqset;       
        PioDCOSetFreq(&DCO,frqFT8,0U);

        #ifdef QUAD
        quad_set_frequency(&osc, frqFT8, false, &sol)
        dump_solution("CAT", &sol, osc.pio , osc.sm);

        #endif //QUAD

        #ifdef QUAD

        #endif //QUAD
        
        adc_fifo_drain ();
        adc_offset = adc();
      }
    }
    strcpy(sent, "FA"); // Return 11 digit frequency in Hz.
    snprintf(parameter, 12, "%011d", (int)frqFT8);
    strcat(sent,parameter); 
    strcat(sent, ";");
  }
  else if (strcmp(command,"FB")==0) {                   
    strcpy(sent, "FB"); // Return 11 digit frequency in Hz.
    snprintf(parameter, 12, "%011d", (int)frqFT8);
    strcat(sent,parameter); 
    strcat(sent, ";");
  }
  else if (strcmp(command,"IF")==0) {          
    strcpy(sent, "IF"); // Return 11 digit frequency in Hz.  
    snprintf(parameter, 12, "%011d", (int)frqFT8);
    strcat(sent, parameter);
    strcat(sent, "0001+0000000000"); 
    snprintf(parameter, 2, "%d", Tx_Status);
    strcat(sent, parameter);  
    strcat(sent, "20000000;");          //USB   
  }
  else if (strcmp(command,"MD")==0) {
    strcpy(sent, "MD2");                //USB
  }
  else  if (strcmp(command,"ID")==0)  {  
    strcpy(sent,"ID019;");
  }
  else  if (strcmp(command,"PS")==0)  {  
    strcpy(sent, "PS1;");
  }
  else  if (strcmp(command,"AI")==0)  {  
    strcpy(sent, "AI0;");
  }
  else  if (strcmp(command,"RX")==0)  {  
    strcpy(sent, "RX0;");                //Just ignore TX/RX commands, it's a VOX transceiver
  }
  else  if (strcmp(command,"TX")==0)  {  
    strcpy(sent, "TX0;");
  }
  else  if (strcmp(command,"AG")==0)  {  
    strcpy(sent, "AG0000;");
  }
  else  if (strcmp(command,"XT")==0) {  
    strcpy(sent, "XT0;");
  }
  else  if (strcmp(command,"RT")==0)  {  
    strcpy(sent, "RT0;");
  }
  else  if (strcmp(command,"RC")==0)  {  
    strcpy(sent, ";");
  }
  else  if (strcmp(command,"RS")==0)  {  
    strcpy(sent, "RS0;");
  }
  else  if (strcmp(command,"VX")==0)  {  
    strcpy(sent, "VX0;"); 
  }
  else  {
    strcpy(sent, "?;"); 
  }
//----------------------------------------------------
  if (strcmp(command2,"ID")==0)   {  
    strcpy(sent2, "ID019;");
  }
  else  {
    strcpy(sent2, "?;"); 
  }               
  
  if (bufferIndex == 2)  {
    cdc_write(sent2, (uint16_t)strlen(sent2));
  }        
  else  {
    cdc_write(sent, (uint16_t)strlen(sent));
  }  
#endif //CAT
}

//*---------------------------------------------------------------------------------*/
//* Functions to manage the CDC serial emulation (for debug and CAT)                */
//*---------------------------------------------------------------------------------*/
void cdc_write(char *buf, uint16_t length)
{
  tud_cdc_write(buf, length);
  tud_cdc_write_flush();
}

#ifdef CAT
void cdc_write_int(int64_t integer) 
{
  char buf[64];
  int length = sprintf(buf, "%lld", integer);
  tud_cdc_write(buf, (uint32_t)length);
  tud_cdc_write_flush();
}
#endif //CAT


#ifdef CAT
uint32_t cdc_read(void)
{
  if ( tud_cdc_available() )
  {
    // read data
    uint32_t count = tud_cdc_read(cdc_read_buf, sizeof(cdc_read_buf));
    (void) count;
      return count;
  }
  else{
    return 0;
  }
}
#endif //CAT