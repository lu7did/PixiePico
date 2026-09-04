/*
Project PixiePico

Pixie based digital transceiver firmware
Test firmware, basic GUI workbench and verification
DDS algorithm

Copyright Dr. Pedro E. Colla LU7DZ (2026)
For non-profit uses only
================================================================
Este programa  disponible es hecho público
bajo la licencia Creative Commons Attribution-ShareAlike 4.0
International (CC BY-SA 4.0).

*/
/*----------------------------------------------------------------------------
 * Version 1.0
 * - Initial release
 * - Basic TX/RX funcionality
 * - Basic board management and control
 * - Software VCO
 * - USB Audio
 *----------------------------------------------------------------------------
 */
/*---------------------------------------------------------------------------------------*
 * This library receives the considerable learning made when developing the
 * ADX-rp2040 package and specially the RDX package which provides support 
 * for the rp2040 processor albeit using a cross platform compatibility layer
 * allowing the usage of the Arduino libraries and IDE to develop for other boards
 * in general and the rp2040 in particular, repositories for these projects are
 * 
 *     ADX-rp2040    https://github.com/lu7did/ADX-rp2040
 *     RDX           https://github.com/lu7did/RDX-rp2040
 *---------------------------------------------------------------------------------------*
 */
 /*----------------------------------------------------------------------------
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the CC BY-AS License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 */

/*------------------------------------------------------------------------------------------------*
 |  IDENTIFICATION DIVISION.                                                                      |
 |  (just a programmer's joke)                                                                    |
 *------------------------------------------------------------------------------------------------*/
#define PROGNAME "PixiePico"
#define AUTHOR "Dr. Pedro E. Colla (LU7DZ)"
#define VERSION  "1.0"
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
#define  DEBUG      1

//#define  EEPROM     1   
//#define  WAITSERIAL 1
//#define  FS         1

//*==============================================================================================*
//*                                Configuration consistency rules                               *
//*==============================================================================================*

#ifndef DEBUG
#undef WAITSERIAL
#undef RTC
#endif //!DEBUG

//*==============================================================================================*
//*                                  Includes and Source Libraries                               *
//*==============================================================================================*

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
//#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "ddsvco.h"
#include "rotary_encoder.h"
#include "ssd1306.h"
#include "ws2812.pio.h"
#include <inttypes.h>
#include "PixiePico.h"
#include "usb_audio.h"
#include "tusb.h"
#include "bsp/board_api.h"
//*==============================================================================================*
//*                             Macros and Structures                                            *
//*==============================================================================================*
#ifdef DEBUG
#define cdc_printf(fmt, ...)                                      \
    do {                                                          \
        int _cdc_len = snprintf(hi, sizeof(hi),                    \
                                "[%s]: " fmt,                      \
                                __func__, ##__VA_ARGS__);          \
        if (_cdc_len > 0 && tud_cdc_n_connected(0)) {             \
            if (_cdc_len >= (int)sizeof(hi))                       \
                _cdc_len = (int)sizeof(hi) - 1;                    \
            tud_cdc_n_write(0, hi, (uint32_t)_cdc_len);            \
            tud_cdc_n_write_flush(0);                              \
        }                                                         \
    } while (0)
#else  //!DEBUG
#define cdc_printf(...) (void)0
#endif //DEBUG    
//*---------------------------------------------------------------------------------------*
//* Define a 128 bytes storage area for the run time configuration 
//*---------------------------------------------------------------------------------------*
typedef struct {
    uint8_t   ID;
    uint8_t   mode;
    uint8_t   band;
    uint8_t   iVfo;
    uint8_t   iShift;
    uint8_t   iStep;
    uint32_t  freq;
    uint8_t   bw;
    uint32_t  frqFT8;
    uint8_t   reserved[115];
} PixiePico_t;

//*==============================================================================================*
//*                             Constants and parameters                                         *
//*==============================================================================================*
#define AUDIOSAMPLING    48000            // USB Audio sampling frequency (fixed)
#define PLL_SYS_MHZ        250            // RP2040 System Clock (MHz)  
#define PLL_SYS_MHZ_PLUS   250            // RP2040 System Clock (MHz) --OVERCLOCK--
#define GEN_FRQ_HZ     7074000L           // Generator Frequency (in Hz)
#define FT8_BASE_HZ       1000L           // FT8 base frequency (in Hz) <Not used>
#define DEFAULT_MODE         3            // Default mode FT8
#define DEFAULT_BAND         0            // Default band 40m
#define DEFAULT_VFO          0            // Default Vfo A
#define DEFAULT_SHIFT        0
#define DEFAULT_STEP         0
#define NBANDS               3
#define NMODES               5



PixiePico_t p;                      //*--- System Variables
char hi[128];


#define OLED_I2C i2c0
#define OLED_SDA_PIN 0
#define OLED_SCL_PIN 1
#define OLED_I2C_ADDRESS 0x3C
#define OLED_I2C_FREQUENCY_HZ 400000

#define ENCODER_CLK_PIN 29
#define ENCODER_DT_PIN  28
#define ENCODER_SW_PIN  27
#define ENCODER_FREQUENCY_STEP_HZ 10L
#define BLINK_INTERVAL_MS 500
#define WS2812_FREQUENCY_HZ 800000.0f
#define DDSVCO_OUTPUT_GPIO DDSVCO_DEFAULT_GPIO
#define DDSVCO_TUNE_SETTLE_MS 350u

#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 16
#endif

#define LONGPUSH 2000000
#define MAXMENU  5

#define pin_A0               26U          //pin for ADC (A2)

//*==============================================================================================*
//*                                  Global Memory Areas                                         *
//*==============================================================================================*

static ssd1306_t oled;
static bool oled_available = false;
static long current_frequency_hz = 7074000L;
static ddsvco_t vco;
static ddsvco_solution_t vco_solution;
static bool vco_available = false;
static bool vco_frequency_pending = false;
static absolute_time_t vco_frequency_deadline;

uint32_t timerSW = 0UL;

bool menuMode=false;
bool editMode=false;
uint8_t menuItem=0;

uint8_t  iBand=DEFAULT_BAND;   //Default band is 40m
uint8_t  iMode=DEFAULT_MODE;   //Default mode is FT8
uint8_t  iVfo=DEFAULT_VFO;    //Default VFO is A
uint16_t iShift=DEFAULT_SHIFT;  //Default Shift is 600 Hz
uint16_t iStep=DEFAULT_STEP;   //Default Step is 10 Hz
uint32_t frqFT8=FT8_BASE_HZ;

uint8_t  iLED=0;
bool     iTX=false;
bool     iWatch=false;

uint32_t fStep=ENCODER_FREQUENCY_STEP_HZ;

long unsigned int Bands[NBANDS][NMODES] = {
              { 7038600, 7078000, 7047500, 7074000,7030000},
              {14095600,14078000,14080000,14074000,14020000},
              {28124600,28078000,28180000,28074000,28020000}};



//*--- for ADC offset at transceiver
int32_t adc_offset = 0;   
uint64_t audio_freq_prev=0.0;
int Tx_Status = 0; // 0=RX, 1=TX
int Tx_Start = 0;  // 0=RX, 1=TX
int not_TX_first = 0;
uint32_t Tx_last_mod_time;
uint32_t Tx_last_time;

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
uint32_t push_last_time;  // to detect the long puch for frequency change by push switch
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

static volatile bool cdc_dtr = false;

#ifdef FS
uint8_t JSON[2048];
PixiePico_t fs;
#endif //FS
int32_t adc(); 


/*---
Wait for the Serial Monitor window to be opened
*/              
/*
static void wait_for_usb_monitor(void) {
    for (int attempt = 0; attempt < 150; ++attempt) {
        if (stdio_usb_connected()) {
            return;
        }
        sleep_ms(100);
    }
}
*/
static void wait_for_usb_monitor(void)
{
    absolute_time_t deadline =
        make_timeout_time_ms(15000);

    while (!time_reached(deadline)) {
        tud_task_ext(0, false);

        if (tud_cdc_n_connected(0)) {
            return;
        }

        sleep_ms(1);
    }
}
/*--- 
Change color of built in LED
*/
static inline uint32_t rgb_to_grb(uint8_t red, uint8_t green, uint8_t blue) {
    return ((uint32_t)green << 24) |
           ((uint32_t)red << 16) |
           ((uint32_t)blue << 8);
}

/*--- 
Turn on-off the built in LED
*/
static void set_led(PIO pio, uint state_machine,
                    uint8_t red, uint8_t green, uint8_t blue) {
    pio_sm_put_blocking(pio, state_machine, rgb_to_grb(red, green, blue));
}

/*---
Erase OLED display
*/
void clearOLED() {
    if (!oled_available) {
        return;
    }

}

/*---
Fill a rectancle on the OLED display
*/

static void fill_rectangle(ssd1306_t *display, int x, int y,
                           int width, int height, bool on) {
    for (int py = y; py < y + height; ++py) {
        for (int px = x; px < x + width; ++px) {
            ssd1306_draw_pixel(display, px, py, on);
        }
    }
}
/*---
Draw a rectangle on the OLED display
*/
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
/*---
Show contents of the OLED display
*/
static void refresh_oled(void) {
    if (oled_available) {
        (void)ssd1306_show(&oled);
    }
}

/*==========================================================================
                    Menu Management Functions
  ==========================================================================                  
*/
/*---
  Display Mode
*/
void displayMode(uint8_t m) {
    if (!oled_available) {
        return;
    }

    char mode[4];
    switch(m) {
        case 0 : {sprintf(mode,"WSPR");break;}
        case 1 : {sprintf(mode,"FT4");break;}
        case 2 : {sprintf(mode,"JS8");break;}
        case 3 : {sprintf(mode,"FT8");break;}
        case 4 : {sprintf(mode,"CW");break;}
    }
//    snprintf(visible_m
//        ode, sizeof(visible_mode), "%.3s",
//             mode != NULL ? mode : "");


    fill_rectangle(&oled, 0, 0, 20, 9, false);
    ssd1306_draw_text(&oled, 0, 1, mode, 1);
    refresh_oled();
}

/*---
Display VFO in use
*/

void displayVFO(uint8_t v) {
    if (!oled_available) {
        return;
    }
    
    char vfo[5];
    switch(v) {
        case 0  : {sprintf(vfo,"VFOA");break;}
        case 1  : {sprintf(vfo,"VFOB");break;}
        default : {sprintf(vfo,"VFO*");break;}
    }
    //snprintf(visible_vfo, sizeof(visible_vfo), "%.4s",
    //         v != NULL ? vfo : "");

    fill_rectangle(&oled, 22, 0, 27, 9, false);
    ssd1306_draw_text(&oled, 22, 1, vfo, 1);
    refresh_oled();
}

/*---
   Display RX or TX status 
*/
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

/*---
  Display emulator of a VU-Meter
*/
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

/*---
   Display Frequency
*/
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
/*=============================================================================*/

/*=============================================================================
                            Manage Panel contents
  =============================================================================*/
/*---
    Manage panel in NOT menú mode (normal operation)
*/
void displayPanel() {

    displayMode(iMode);
    displayVFO(iVfo);
    displayTX(iTX);
    displayLED(iLED);
    displayFreq(current_frequency_hz);

}
/*---
   Show menu (first level)
*/
void showMenu(uint8_t i) {
    char hi[16];
    clearOLED();
   
    if (!editMode){
       switch(i) {
           case 0 : {snprintf(hi,sizeof(hi),"%d-Band",i);break;}
           case 1 : {snprintf(hi,sizeof(hi),"%d-Mode",i);break;}
           case 2 : {snprintf(hi,sizeof(hi),"%d-VFO",i);break;}
           case 3 : {snprintf(hi,sizeof(hi),"%d-Shift",i);break;}
           case 4 : {snprintf(hi,sizeof(hi),"%d-Step",i);break;}
           case 5 : {snprintf(hi,sizeof(hi),"%d-Watch",i);break;}
        } 
    }
    fill_rectangle(&oled, 0, 0, 128, 32, false);
    ssd1306_draw_text_color(&oled, 8, 12, hi, 2, true);
    refresh_oled();
}
/*
   Menu 2nd Level (Band)
*/
void showBand() {
    char hi[16];
    clearOLED();
    sprintf(hi,"01-Band");
    fill_rectangle(&oled, 22, 0, 27, 9, false);
    fill_rectangle(&oled, 0, 0, 128, 32, false);

    ssd1306_draw_text(&oled, 1, 1, hi, 1);
    refresh_oled();

    switch(iBand) {
       case 0   : {sprintf(hi,"40m");break;}
       case 1   : {sprintf(hi,"20m");break;}
       case 2   : {sprintf(hi,"10m");break;}
    }
    ssd1306_draw_text_color(&oled, 8, 12, hi, 2, true);
    refresh_oled();

}
/*---
   Menu 2nd Level (mode)
*/
void showMode() {

    char hi[16];
    clearOLED();
    sprintf(hi,"02-Mode");
    fill_rectangle(&oled, 22, 0, 27, 9, false);
    fill_rectangle(&oled, 0, 0, 128, 32, false);

    ssd1306_draw_text(&oled, 1, 1, hi, 1);
    refresh_oled();

    switch(iMode) {
       case 0   : {sprintf(hi,"FT8");break;}
       case 1   : {sprintf(hi,"JS8");break;}
       case 2   : {sprintf(hi,"WSPR");break;}
       case 3   : {sprintf(hi,"FT4");break;}
       case 4   : {sprintf(hi,"CW");break;}
    }

    ssd1306_draw_text_color(&oled, 8, 12, hi, 2, true);
    refresh_oled();

}
/*
  Menu 2nd Level (VFO)
*/
void showVFO() {

    char hi[16];
    clearOLED();
    sprintf(hi,"03-VFO");
    fill_rectangle(&oled, 22, 0, 27, 9, false);
    fill_rectangle(&oled, 0, 0, 128, 32, false);

    ssd1306_draw_text(&oled, 1, 1, hi, 1);
    refresh_oled();

    switch(iVfo) {
       case 0   : {sprintf(hi,"VFOA");break;}
       case 1   : {sprintf(hi,"VFOB");break;}
    }

    ssd1306_draw_text_color(&oled, 8, 12, hi, 2, true);
    refresh_oled();

}
/*
  Menu 2nd Level CW Shift
*/
void showShift() {
    char hi[16];
    clearOLED();
    sprintf(hi,"03-Shift");
    fill_rectangle(&oled, 0, 0, 128, 32, false);
    fill_rectangle(&oled, 22, 0, 27, 9, false);
    ssd1306_draw_text(&oled, 1, 1, hi, 1);
    refresh_oled();

    switch(iShift) {
       case 0   : {sprintf(hi,"600 HZ");break;}
       case 1   : {sprintf(hi,"700 HZ");break;}
       case 2   : {sprintf(hi,"800 HZ");break;}
    }

    ssd1306_draw_text_color(&oled, 8, 12, hi, 2, true);
    refresh_oled();

}
/*
  Menu 2nd Level Tunning Step
*/
void showStep() {
    char hi[16];
    clearOLED();
    fill_rectangle(&oled, 22, 0, 27, 9, false);
    fill_rectangle(&oled, 0, 0, 128, 32, false);

    sprintf(hi,"04-Step");
    ssd1306_draw_text(&oled, 1, 1, hi, 1);
    refresh_oled();

    switch(iStep) {
       case 0   : {sprintf(hi,"10 HZ");fStep=10L;break;}
       case 1   : {sprintf(hi,"100 HZ");fStep=100L;break;}
       case 2   : {sprintf(hi,"1000 HZ");fStep=1000L;break;}
    }

    ssd1306_draw_text_color(&oled, 8, 12, hi, 2, true);
    refresh_oled();

}
/*
  Menu 2nd level Watchdog activation status
*/
void showWatch() {
    char hi[16];
    clearOLED();
    fill_rectangle(&oled, 22, 0, 27, 9, false);
    fill_rectangle(&oled, 0, 0, 128, 32, false);

    sprintf(hi,"05-Watch");
    ssd1306_draw_text(&oled, 1, 1, hi, 1);
    refresh_oled();

    switch(iWatch) {
       case 0   : {sprintf(hi,"Off");break;}
       case 1   : {sprintf(hi,"Off");break;}
    }

    ssd1306_draw_text_color(&oled, 8, 12, hi, 2, true);
    refresh_oled();

}
/*
   Menu 2nd Level Select which item to show
*/
void showEdit(uint8_t i) {

    switch(i) {
        case 0 : {showBand();break;}
        case 1 : {showMode();break;}
        case 2 : {showVFO();break;}
        case 3 : {showShift();break;}
        case 4 : {showStep();break;}
        case 5 : {showWatch();break;}
    }
}
/*
   Select menu either main mode or menu changing mode
*/
void displayMenu(uint8_t i) {

    if (!oled_available) {
        return;
    }

    if (!menuMode) {
        return;
    }

    if (!editMode) {
        showMenu(menuItem);
        return;
    }
    showEdit(menuItem);

}
/*
 Manages the scroll past the upper range or prior to lower range of menues
*/
static uint8_t wrap_add(uint8_t value, int delta, uint8_t count) {
    int result = ((int)value + delta) % (int)count;
    if (result < 0) {
        result += count;
    }
    return (uint8_t)result;
}

/*
 Upon exit update the menu item into operational level
*/
void updateMenu(uint8_t m, int s) {
    switch(m) {
        case 0 : {iBand=wrap_add(iBand,s,3);break;}
        case 1 : {iMode=wrap_add(iMode,s,5);break;}
        case 2 : {iVfo=wrap_add(iVfo,s,2);break;}
        case 3 : {iShift=wrap_add(iShift,s,3);break;}
        case 4 : {iStep=wrap_add(iStep,s,3);break;}
        case 5 : {iWatch=wrap_add(iWatch,s,2);break;}
    }
    displayMenu(m);
    current_frequency_hz = Bands[iBand][iMode];
    vco_frequency_pending = true;
    vco_frequency_deadline =
        delayed_by_ms(get_absolute_time(), DDSVCO_TUNE_SETTLE_MS);

}
/*=============================================================================
                            Manage Rotary Code and PushButton
  =============================================================================*/
/*---
   Detect and operate upon 
*/
  void rotaryTurn(int direction) {
    if (direction != 1 && direction != -1) {
        return;
    }

    current_frequency_hz += direction * fStep;
    if (current_frequency_hz < 0) {
        current_frequency_hz = 0;
    }
    displayFreq(current_frequency_hz);
    if (vco_available) {
       vco_frequency_pending = true;
       vco_frequency_deadline =
       delayed_by_ms(get_absolute_time(), DDSVCO_TUNE_SETTLE_MS);
       cdc_printf("Encoder: %+d; frecuencia: %ld Hz\r\n",direction, current_frequency_hz);
    } else {
       cdc_printf("VCO not available\n"); 
    }   
    //fflush(stdout);
}


/*
  Select Rotary Switch button pressed
*/
void SWclick(bool pressed) {

    cdc_printf("SW: %s\r\n", pressed ? "presionado" : "liberado");
    //fflush(stdout);

    if(pressed) {
      timerSW = time_us_32();
    } else {
      uint32_t lapSW=time_us_32()-timerSW;
      if (lapSW>LONGPUSH) {
        cdc_printf("Lap: %" PRIu32 " <longPUSH>\n", lapSW);

        if (!menuMode) {            //It is on main panel, so switch to Menu Mode
           menuMode=true;
           editMode=false;
           displayMenu(menuItem);
           cdc_printf("Switch to Menu Mode\n");
           //fflush(stdout);
           return;
        }

        if (editMode) {             //It is on menu mode edit, switch back to main panel
            menuMode=false;
            editMode=false;
            displayPanel();
            cdc_printf("Switch to Panel Mode\n");
            return;
        }
        editMode=true;
        displayMenu(menuItem);
        cdc_printf("Switch to Edit Mode\n");
        //fflush(stdout);

      } else {
        cdc_printf("Lap: %" PRIu32 " <shortPUSH>\n", lapSW);

        if (!menuMode) {

            return;   // Normal panel and press button ** DO NOTHING **
        }

        if (!editMode) {
           menuMode=false;
           editMode=false;
           displayPanel();
           cdc_printf("Switch back to Panel\n");
           //fflush(stdout);
           return;
        }
        editMode=false;
        displayMenu(menuItem);
        cdc_printf("Switch back to Menu\n");
        //fflush(stdout);
        return;

      }  
    }
}

//*----------------------------------------------------------------------------*/
//*  I/O for the ADX board controls (LED, switches and jumpers            */
//*----------------------------------------------------------------------------*/
void PixiePicoSetup(){

    //*--- GPIO setting for the ADC control (receiver) 
    gpio_init(pin_A0);
    gpio_set_dir(pin_A0, GPIO_IN); //ADC input pin
  
    //*--- End of ADX control board initialization
    cdc_printf("PixiePico Board initialized\n");
    
}
void setTX(bool t)
{
    //#----- TEST Dummy --- Switch on and off
}
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*                               MAIN                                      */
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

int main(void) {

    /*
    uint32_t pending_frequency_hz;
    if (ddsvco_take_pending_frequency(&pending_frequency_hz)) {
        current_frequency_hz = (long)pending_frequency_hz;
    }

    const ddsvco_config_t vco_config = {
        .pio = pio1,
        .state_machine = -1,
        .output_gpio = DDSVCO_OUTPUT_GPIO,
        .sys_clk_min_hz = DDSVCO_SYS_CLK_MIN_HZ,
        .sys_clk_max_hz = DDSVCO_SYS_CLK_MAX_HZ,
        .reboot_on_pll_change = false,
    };

    vco_available = ddsvco_init(&vco, &vco_config) &&
                    ddsvco_solve(&vco, (uint32_t)current_frequency_hz,
                              &vco_solution) &&
                    ddsvco_start(&vco, &vco_solution);

    */
    vco_available = false;                
    /*--- Start the other subsystems*/

    const PIO pio = pio0;
    const uint state_machine = 0;
    const uint pio_offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(
        pio,
        state_machine,
        pio_offset,
        PICO_DEFAULT_WS2812_PIN,
        WS2812_FREQUENCY_HZ,
        false
    );

    /* Rojo: se alcanzó main(), pero USB todavía no fue inicializado. */
    set_led(pio, state_machine, 24, 0, 0);

    /*
    sleep_ms(1000);
    tusb_rhport_init_t dev_init = {
      .role  = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO
    };

    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    sleep_ms(500);
    stdio_init_all();
    */



    tusb_rhport_init_t dev_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL
    };


    set_led(pio, state_machine, 24, 0, 0);  // rojo: antes de TinyUSB

bool usb_ok = tusb_init(BOARD_TUD_RHPORT, &dev_init);

if (usb_ok) {
    set_led(pio, state_machine, 24, 24, 0); // amarillo: init correcta
} else {
    set_led(pio, state_machine, 0, 0, 24);  // azul: init devolvió false
}

    
    wait_for_usb_monitor();
    set_led(pio, state_machine, 0, 0, 24);  // verde
    /*--- Initialize another PIO for the built in LED*/
    
    /*--- Define I/O mapping for I2C subsystem*/                    
    i2c_init(OLED_I2C, OLED_I2C_FREQUENCY_HZ);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);

    /*--- Initialize and define the encoder */
    rotary_encoder_t encoder;
    rotary_encoder_init(&encoder,
                        ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);

    /*--- Initialize and define the OLED display */                    
    oled_available = ssd1306_init(&oled, OLED_I2C, OLED_I2C_ADDRESS);

    /*--- Establish current frequency based on programmed band */
    if (current_frequency_hz <= 0) {
        current_frequency_hz = Bands[iBand][iMode];
    }

    /*--- Display panel */
    if (oled_available) {
        ssd1306_clear(&oled);
        (void)ssd1306_show(&oled);
        displayPanel();
    }

    /*--- Wait for USB monitor and show banner if in DEBUG mode*/

    wait_for_usb_monitor();

    cdc_printf("testVCO iniciado en Waveshare RP2040-Zero\r\n");
    cdc_printf("LED WS2812: GPIO %u\r\n", PICO_DEFAULT_WS2812_PIN);
    cdc_printf("OLED: SSD1306 128x32, I2C0, SDA=GPIO%d, SCL=GPIO%d, addr=0x%02X\r\n",
           OLED_SDA_PIN, OLED_SCL_PIN, OLED_I2C_ADDRESS);
    cdc_printf("OLED %s; pantalla FT8/VFOA/TX, frecuencia 28074.00\r\n",
           oled_available ? "detectado" : "NO detectado");
    cdc_printf("KY-040: CLK(A)=GPIO%d, DT(B)=GPIO%d, SW=GPIO%d\r\n",
           ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);
    if (vco_available) {
        cdc_printf("VCO: GPIO%u, objetivo=%" PRIu32
               " Hz, PLL_SYS=%.6f Hz\r\n",
               DDSVCO_OUTPUT_GPIO, vco_solution.target_hz,
               (double)vco_solution.sys_clk_num /
                   (double)vco_solution.sys_clk_den);
        cdc_printf("VCO: REFDIV=%u, FBDIV=%u, POSTDIV=%u/%u, VCO=%" PRIu32
               " Hz\r\n",
               (unsigned)vco_solution.pll_refdiv,
               (unsigned)vco_solution.pll_fbdiv,
               (unsigned)vco_solution.pll_postdiv1,
               (unsigned)vco_solution.pll_postdiv2,
               vco_solution.pll_vco_hz);
        cdc_printf("VCO: CLKDIV=%u+%u/256, salida=%.6f Hz, "
               "error=%+.6f Hz (%+.6f ppm)\r\n",
               (unsigned)vco_solution.pio_divider_int,
               (unsigned)vco_solution.pio_divider_frac,
               vco_solution.achieved_hz,
               vco_solution.error_hz,
               vco_solution.error_ppm);
    } else {
        cdc_printf("VCO: ERROR de inicializacion/solucion\r\n");
    }
    //fflush(stdout);

    /*--- Initialize blinking of built in LED */
    
    bool is_on = false;
    unsigned long cycle = 0;
    unsigned led_level = 0;
    absolute_time_t next_blink =
        delayed_by_ms(get_absolute_time(), BLINK_INTERVAL_MS);

    PixiePicoSetup();
      //*--- ADC (receiver) initialization
  
    pcCounter=0;
    adc_init();
    adc_select_input(0);                        // ADC input pin A0
    adc_run(true);                              // start ADC free running
    adc_set_clkdiv(249.0);                      // 192kHz sampling  (48000 / (249.0 +1) = 192)
    adc_fifo_setup(true,false,0,false,false);   // fifo
    cdc_printf("ADC receiver sub-system initialized\n");

      //*--- USB Audio initialization (initialization of monodata[])
    for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4); i++) {
       monodata[i] = 0;
    }

//*--- Calibrate the ADC offset, read the DC offset value (ADC input)
  
    sleep_ms(100);
    adc_fifo_drain ();

    cdc_printf("Computing ADC offset\n");

    adc_offset = adc();
    cdc_printf("ADC input offset callibrated\n");
    cdc_printf("Transceiver  ready\n");
/*----------------------------------------------------------------------------
                           Main Loop
  ----------------------------------------------------------------------------*/
    absolute_time_t next_cdc_test =
    delayed_by_ms(get_absolute_time(), 1000);
    uint32_t cdc_counter = 0;
    
    while (true) {

    
       tud_task_ext(0, false);
        /*-------------------- Housekeeping ---------------------------------*/
        /*--- Manage rotary decoder changes */
        int steps = rotary_encoder_take_steps();
        if (steps != 0) {
        
           if (!menuMode) {
              while (steps > 0) {
                  rotaryTurn(+1);
                  --steps;
              }
              while (steps < 0) {
                  rotaryTurn(-1);
                  ++steps;
              }
           } else {
              if (!editMode) {
                 menuItem=wrap_add(menuItem, steps, MAXMENU + 1);
                 displayMenu(menuItem);
              } else {
                 updateMenu(menuItem,steps);
                 displayMenu(menuItem);
              }
           }
        }
        /*--- Manage the press of the SW of the rotary encoder */
        bool switch_pressed;
        if (rotary_encoder_poll_switch(&encoder, &switch_pressed)) {
            SWclick(switch_pressed);
        }

        /*--- Manage changes in the VCO frequency */

        if (vco_available && vco_frequency_pending &&
            time_reached(vco_frequency_deadline)) {
            vco_frequency_pending = false;

            const uint32_t previous_sys_clk_hz = clock_get_hz(clk_sys);
            const ddsvco_setfreq_result_t result =
                ddsvco_setfreq(&vco, (uint32_t)current_frequency_hz,
                               &vco_solution);

            const uint32_t new_sys_clk_hz = clock_get_hz(clk_sys);
            if (result == DDSVCO_SETFREQ_APPLIED &&
                new_sys_clk_hz != previous_sys_clk_hz) {
                /* PIO0 también deriva de clk_sys: restaurar 800 kHz del
                 * WS2812 después de modificar PLL_SYS. */
                ws2812_program_init(pio, state_machine, pio_offset,
                                    PICO_DEFAULT_WS2812_PIN,
                                    WS2812_FREQUENCY_HZ, false);
            }

            /*--- Show new solution if DEBUG is enabled*/
            cdc_printf("VCO setfreq %" PRIu32
                   " Hz: %s; PLL=%u/%u/%u/%u; "
                   "CLKDIV=%u+%u/256; salida=%.6f Hz; "
                   "error=%+.6f Hz (%+.6f ppm)\r\n",
                   (uint32_t)current_frequency_hz,
                   ddsvco_setfreq_result_name(result),
                   (unsigned)vco_solution.pll_refdiv,
                   (unsigned)vco_solution.pll_fbdiv,
                   (unsigned)vco_solution.pll_postdiv1,
                   (unsigned)vco_solution.pll_postdiv2,
                   (unsigned)vco_solution.pio_divider_int,
                   (unsigned)vco_solution.pio_divider_frac,
                   vco_solution.achieved_hz,
                   vco_solution.error_hz,
                   vco_solution.error_ppm);
            //fflush(stdout);
        }

        /*--- Manage built in LED blinking */
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
            } else {
                set_led(pio, state_machine, 0, 0, 0);
            }

            if (!menuMode) {
               displayLED(led_level);
            }
            //fflush(stdout);
        }

        /*-------------------- Housekeeping ---------------------------------*/
        
         if (Tx_Start==0) {                     //Tx_Start=0 (RX) || Tx_Start=1 (TX)
            receiving();
         } else {
            transmitting();
         }    
         sleep_ms(1);
    }
}
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

       //*---- Change Frequecy here PioDCOSetFreq(&DCO, f, 0U);
       
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
