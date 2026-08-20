//*----------------------------------------------------------------------------
//* testFT8.c
//*----------------------------------------------------------------------------
//* Testbed for the ADX DDS PIO-based frequency synthesizer FT8 Transceiver
//* using Raspberry Pi Pico.
//*   Basic ADX control board management (LEDs & Switches).
//*   Time synchronization (manual). 
//*   FT8_lib integration (basic).
//*   Basic FT8 transmission (no reception yet), beacon mode.
//* This is a working module of an integration effort to build an FT8 
//* transceiver using Raspberry Pi Pico as the main controller and signal
//* generator.
//* 
//* Copyright (c) 2025 by Pedro Colla (LU7DZ)
//*----------------------------------------------------------------------------
//* Largely based on the work of Roman Piksaykin (R2BDY):
//* Digital Controlled Oscillator for Raspberry Pi Pico
//* https://www.qrz.com/db/r2bdy        
//*----------------------------------------------------------------------------
//  DESCRIPTION (verbatim from Roman's original project page)
//
//      The oscillator provides precise generation of any frequency ranging
//  from 1 Hz to 33.333 MHz with tenth's of millihertz resolution (please note that
//  this is relative resolution owing to the fact that the absolute accuracy of 
//  onboard crystal of pi pico is limited; the absoulte accuracy can be provided
//  when using GPS reference option included).
//      The DCO uses phase locked loop principle programmed in C and PIO asm.
//      The DCO does *NOT* use any floating point operations - all time-critical
//  instructions run in 1 CPU cycle.
//      Currently the upper freq. limit is about 33.333 MHz and it is achieved only
//  using pi pico overclocking to 270 MHz.
//      Owing to the meager frequency step, it is possible to use 3, 5, or 7th
//  harmonics of generated frequency. Such solution completely cover all HF and
//  a portion of VHF band up to about 233 MHz.
//      Unfortunately due to pure digital freq.synthesis principle the jitter may
//  be a problem on higher frequencies. You should assess the quality of generated
//  signal if you want to emit a noticeable power.
//      This is an experimental project of amateur radio class and it is devised
//  by me on the free will base in order to experiment with QRP narrowband
//  digital modes.
//
//  PLATFORM
//      Raspberry Pi pico.
//
//  REVISION HISTORY
// 
//      Rev 0.1   31 Dec 2025   Initial release
//
//  PROJECT PAGE
//      https://github.com/lu7did/ADX-ddsPIO
//
//  LICENCE
//      MIT License (http://www.opensource.org/licenses/mit-license.php)
//
//  Copyright (c) 2025 by Dr. Pedro E. Colla
//                Email: lu7dz at gmail.com
//            (c) 2023 by Roman Piksaykin (R2BDY)
// -------------------------------------------------------------------------------------------
//  TERMS OF USE: MIT License
//*----------------------------------------------------------------------------
//  
//  Permission is hereby granted, free of charge,to any person obtaining a copy
//  of this software and associated documentation files (the Software), to deal
//  in the Software without restriction,including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY,WHETHER IN AN ACTION OF CONTRACT,TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "hardware/clocks.h"
#include "pico/stdlib.h"

#include "defines.h"

#include "piodco/piodco.h"
#include "./build/dco2.pio.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "pico/stdio/driver.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"


#include "./lib/assert.h"
#include "./debug/logutils.h"
#include "hwdefs.h"

#include <GPStime.h>

#include <hfconsole.h>

#include "./ft8_lib/common/wave.h"
#include "./ft8_lib/ft8/pack.h"
#include "./ft8_lib/ft8/encode.h"
#include "./ft8_lib/ft8/constants.h"


#include "protos.h"
#include "testFT8.h"

PioDco DCO; /* External in order to access in both cores. */

//* Operating mode flags */
bool bFT8mode = false;
bool upButton, downButton, txButton, syncButton;
bool prevUpButton = true, prevDownButton = true, prevTxButton = true, prevSyncButton = true;

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


//* FT8 related variables */

const int FifoSize=FT8_NN;
const char *message="CQ LU7DZ GF11"; //Test message
uint8_t packed[FTX_LDPC_K_BYTES];
uint8_t tones[FT8_NN];          // FT8_NN = 79, lack of better name at the moment
unsigned char ft8msg[FT8_NN];
bool bTX = false;

#ifdef FT8TEST

const uint8_t ft8_tones_cq_lu7dz_gf11[79] = {
    03, 01, 04, 00, 06, 05, 02, 00, 00, 00,
    00, 00, 00, 00, 01, 01, 03, 00, 01, 05,
    04, 07, 00, 03, 02, 00, 06, 05, 01, 02,
    04, 02, 07, 05, 00, 03, 01, 04, 00, 06,
    05, 02, 07, 01, 04, 07, 00, 01, 02, 03,
    00, 05, 06, 02, 07, 00, 03, 04, 05, 02,
    04, 02, 00, 05, 07, 01, 03, 06, 00, 05,
    01, 03, 01, 04, 00, 06, 05, 02, 02
};
#endif //FT8TEST

//**********************************[ BAND SELECT ]************************************************
// ADX can support up to 4 bands on board. Those 4 bands needs to be assigned to Band1 ... Band4
// from supported 8 bands.
// To change bands press SW1 and SW2 simultaneously. Band LED will flash 3 times briefly and stay
// lit for the stored band. also TX LED will be lit to indicate
// that Band select mode is active. Now change band bank by pressing SW1(<---) or SW2(--->). When
// desired band bank is selected press TX button briefly to exit band select mode.
// Now the new selected band bank will flash 3 times and then stored mode LED will be lit.
// TX won't activate when changing bands so don't worry on pressing TX button when changing bands in
// band mode.
// Assign your prefered bands to B1,B2,B3 and B4
// Supported Bands are: 80m, 40m, 30m, 20m,17m, 15m, 10m
int slot[4]   = {40,30,20,10};
int Band_slot = SLOT;                  //This is the default band Band1=1,Band2=2,Band3=3,Band4=4
int Band      =   10;                  //This is the default band

long unsigned int Bands[NBANDS][NMODES] = {
                                          { 3568600, 3578000, 3575000, 3573000},
                                          { 7038600, 7078000, 7047500, 7074000},
                                          {10138700,10130000,10140000,10136000},
                                          {14095600,14078000,14080000,14074000},
                                          {18104600,18104000,18104000,18100000},
                                          {21094600,21078000,21140000,21074000},
                                          {28124600,28078000,28180000,28074000}};

//**********************************[ END BAND SELECT ]********************************************
//*============================================================================*/
//*                              FT8 Functions                                 */
//*============================================================================*/
int FT8GenerateSymbols()
{
int rc = pack77(message, packed);

printf("Packing message: '%s'\n",message);
if (rc < 0) {
    printf("Cannot parse message! RC=%d\n",rc);
    return -2;
}

printf("Packed data: ");
for (int j = 0; j < 10; ++j) {
    printf("%02x ", packed[j]);
}
printf("\n");

// Second, encode the binary message as a sequence of FSK tones
ft8_encode(packed, tones);
                   
printf("FSK tones (calc..): ");

for (int j = 0; j < FT8_NN; ++j) {
    printf("%d", tones[j]);
}

printf("\n");

#ifdef FT8TEST
for(size_t i=0;(i<FT8_NN);i++)
{
    tones[i]=ft8_tones_cq_lu7dz_gf11[i];
}

printf("FSK tones (forced): ");
for (int j = 0; j < FT8_NN; ++j) {
    printf("%d", tones[j]);
}
printf("\n");
#endif //FT8TEST

return 0;
}

//*============================================================================*/
//*                              Service Functions                             */
//*============================================================================*/
void blinkLED(int ms)
{
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(ms);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    sleep_ms(ms);
}
//*----------------------------------------------------------------------------*/
void initBoard(){
    
    //* Set system clock to maximum (270 MHz)
    const uint32_t clkhz = PLL_SYS_MHZ * 1000000L;
    set_sys_clock_khz(clkhz / 1000L, true);

    //* Initialize stdio and other hardware elements
    stdio_init_all();
    sleep_ms(1000);
 
    //*--- Initialize the default LED (board) pin
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);


}
//*----------------------------------------------------------------------------*/
void ADXsetup(){

    
    gpio_init(RXSW);
    gpio_set_dir(RXSW, GPIO_OUT);   
    gpio_put(RXSW, 0); //Set RX mode

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
    gpio_put(TX, 0); //Set RX mode (off)

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

    //*--- End of ADX control board initialization
    printf("%s ADX control board initialized\n",__func__);

}
//*----------------------------------------------------------------------------*/
//* Sync time when SYNC button is pressed and set the FT8 mode                 */
//*----------------------------------------------------------------------------*/
bool syncTime() {
    bool bFT8=false;
    char datetime_str[64];
    
    while (gpio_get(SYNC) == 0) {
        //sleep_ms(500);
        bFT8 = true;
        blinkLED(100);
        blinkLED(100);
        sleep_ms(100);
        blinkLED(100);
        blinkLED(100);
        rtc_set_datetime(&tcpu);

    }
    if (bFT8){
       rtc_get_datetime(&tcpu);
       datetime_to_str(datetime_str, sizeof(datetime_str), &tcpu);
       printf("FT8 mode set with current time: %s\n", datetime_str);
    } else {
       printf("LED Test mode set...\n");
    }
    return bFT8;
     
}
/*----------------------------------------------------------------------------*/
/* Test a single button                                                       */
/*----------------------------------------------------------------------------*/
bool testButton(uint gpio) {
    bool bstate = gpio_get(gpio);
    return bstate;
}
/*----------------------------------------------------------------------------*/
/* Test buttons and light LEDs accordingly                                    */
/*----------------------------------------------------------------------------*/
void testLED() {

upButton = testButton(UP);
downButton = testButton(DOWN);
txButton = testButton(TXSW);
syncButton = testButton(SYNC);

if (upButton == false) {
    if (prevUpButton == true) {
        printf("UP button pressed\n");
        gpio_put(FT8, 1);
        prevUpButton = false;
    } 
} else {
    if (prevUpButton == false) {
        printf("UP button released\n");
        gpio_put(FT8, 0);
        prevUpButton = true;    
    }            
}

if (downButton == false) {
    if (prevDownButton == true) {
        printf("DOWN button pressed\n");
        gpio_put(FT4, 1);
        prevDownButton = false;
   } 
} else {
    if (prevDownButton == false) {
        printf("DOWN button released\n");
        gpio_put(FT4, 0);
        prevDownButton = true;    
    }            
}


if (txButton == false) {
    if (prevTxButton == true) {
        printf("TX button pressed\n");
        gpio_put(TX, 1);
        prevTxButton = false;
    } 
} else {
    if (prevTxButton == false) {
        printf("TX button released\n");
        gpio_put(TX, 0);
        prevTxButton = true;    
    }            
}

if (syncButton == false) {
    if (prevSyncButton == true) {
        printf("SYNC button pressed\n");
        gpio_put(WSPR, 1);
        prevSyncButton = false;
    } 
} else {
    if (prevSyncButton == false) {
        printf("SYNC button released\n");
        gpio_put(WSPR, 0);
        prevSyncButton = true;    
    }            
}
       
}
/*----------------------------------------------------------------------------*/
/* Control transmitter TX status and LED                                      */
/*----------------------------------------------------------------------------*/
void setTX(bool state) {
    if (state) {
        gpio_put(TX, 1);
        bTX = true;
    } else {
        gpio_put(TX, 0);
        bTX = false;
    }
}   
/*----------------------------------------------------------------------------*/
/* Multiply the tone shift accounting for the Hz and milliHz                  */                                         
/*----------------------------------------------------------------------------*/
void computeFSK(uint8_t index,
                uint32_t *Hz,
                uint32_t *mHz)
{
    /* This is the FT8 FSK whole index {0..7} * 6*/
    *Hz = index * 6;

    /* Fractional part: index * 0.25 = (index % 4) * 250 milliseconds */
    *mHz = (index * 250) % 1000;

    /* Adjust if the fraction exceeds 1 unit */
    if (index >= 4) {
        *Hz += index / 4;
    }
}
//*============================================================================*/
//*                              Main body                                     */
//*============================================================================*/
int main() 
{

    //*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
    //*                          S E T U P                                      *
    //*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

    //*----------- Initial Processor board initialization & setup ---------------*
    initBoard();

    //*----------- Setup RTC, this is only used to sync seconds   ---------------*
    rtc_init();
    rtc_set_datetime(&tcpu);
    
    //*-------------------       Init USB and other peripherals   ---------------*
    stdio_usb_init();
    while (!stdio_usb_connected()) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(22);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(22);
   }

   //*--------------------------------------------------------------------------*
    //_INFOLIST("%s Firmware %s version(%s) build(%s)\n",__func__,PROGNAME,VERSION,BUILD);
    printf("Firmware %s version(%s) build(%s)\n",PROGNAME,VERSION,BUILD);
    printf("Config: FT8(%d) FT4(%d) JS8(%d) WSPR(%d) RFOUT(%d)\n",FT8,FT4,JS8,WSPR, RFOUT);
    fflush(stdout);
    //*--------------------------------------------------------------------------*
    static_assert(FT8 == 4, "FT8 no es GPIO4");
    static_assert(FT4 == 5, "FT4 no es GPIO5");
    static_assert(JS8 == 6, "JS8 no es GPIO6");
    static_assert(WSPR == 7, "WSPR no es GPIO7");
    static_assert(RFOUT == 18, "RFOUT no es GPIO18");

    //*------------------------ Set operating mode -------------------------------*
      
    //*------------------------------------------------------------
    //_INFOLIST("%s Hardware initialization\n",__func__);

    //*** Initialize DCO on core 1 ***/
    const uint32_t clkhz = PLL_SYS_MHZ * 1000000L;

    printf("Core 1 started. DCO worker initializing...\n");
    assert_(0 == PioDCOInit(&DCO, RFOUT, clkhz));

//*------------------------ Set the ADX hardware          -----------------------*
    //* *** Initialize ADX control board */
    printf("Initializing ADX control board...\n");
    ADXsetup();

//*------------------------ Sync time and define mode    -----------------------*
    bFT8mode = syncTime();

//*------------------------ if in FT8 mode generate message ---------------------*

    if (bFT8mode) {
       printf("Generating FT8 symbols...\n");
       FT8GenerateSymbols();
    }   

//*-------------------------------------------------------
    //_INFOLIST("%s Launching core 1 for DCO worker...\n",__func__);
    
    printf("launching DCO worker on core 1...\n");
    multicore_launch_core1(core1_entry);

//*-------------------------------------------------------
    //_INFOLIST("%s Launching core 0 Generator\n",__func__);
    printf("launching DDS Generator...\n");
    DDSGenerator();
}
/*============================================================================*/
/*                              CORE 1 PROCESSOR                              */
/* This is the code of dedicated core.                                        */
/* We deal with extremely precise real-time task.                             */
/*============================================================================*/
void core1_entry()
{
    printf("Core 1: DCO worker started.\n");

    /* Run DCO. */
    PioDCOStart(&DCO);

    /* Run the main DCO algorithm. It spins forever. */
    PioDCOWorker2(&DCO);
}

/*============================================================================*/
/*                              CORE 0 PROCESSOR                              */
/* This is the code to setup and launch the generator                         */                              
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/* DDS generator is an idle management function as the DDS is in core1        */                                   
/*----------------------------------------------------------------------------*/
void RAM (DDSGenerator)(void)
{
    int iFT8= 0;
    uint32_t frqFT8 = GEN_FRQ_HZ;
    uint32_t baseFT8 = FT8_BASE_HZ;
    uint32_t intHz, fracHz;
    static absolute_time_t t0;
    bool bFT8 = false;
    bool manualTX = false;

    for(;;)
    {
       /* This generates a fixed frequency signal set by GEN_FRQ_HZ. */
       
        if (!bFT8mode) {
            PioDCOSetFreq(&DCO, GEN_FRQ_HZ, 0u);
            testLED();
            continue;
        }

        // Get the current datetime from the RTC
        rtc_get_datetime(&tcpu);

        // Access the seconds member directly
        uint8_t current_seconds = tcpu.sec;
        
        if ((tcpu.sec == 0) && bTX == false) {
            // Start of a new minute
            printf("FT8 frame windows started: %02d:%02d TX turned on\n", tcpu.hour, tcpu.min);

            // Here you would trigger the FT8 transmission for the new minute
            PioDCOStart(&DCO);
            setTX(true);
            bFT8=false;
            iFT8=0;
            t0 = get_absolute_time();
        } else {
            if (((tcpu.sec == 15) || iFT8 >= FT8_NN) && bTX == true) {
                setTX(false);
                printf("FT8 frame window ended: %02d:%02d TX turned off\n", tcpu.hour, tcpu.min);
                PioDCOStop(&DCO);
                bFT8=false;
                iFT8=0;
                gpio_put(FT8, bFT8);
            }
        }


        if (bTX && !manualTX) {
               computeFSK(tones[iFT8], &intHz, &fracHz);
               uint32_t f = frqFT8 + baseFT8 + intHz;
               PioDCOStart(&DCO);
               PioDCOSetFreq(&DCO, f, fracHz);
               #ifdef TRACE
               printf("FT8symbol[%d] tone[%d] shift(%lu/%lu) f[%lu Hz]\n", iFT8, tones[iFT8],intHz,fracHz,f); 
               #endif //TRACE
               
               sleep_ms(160);
               iFT8++;
        } else {

               int64_t diff_us = absolute_time_diff_us(t0, get_absolute_time());
               if (diff_us >= 100000) {
                   t0 = get_absolute_time();
                   if (bFT8 == false) {
                      bFT8= true;
                   } else {
                      bFT8 = false;
                   }
                   gpio_put(FT8, bFT8);
               }

               if (testButton(TXSW) == false && manualTX==false) {
                   printf("Manual TX button pressed. Starting TX...\n");
                   fflush(stdout);
                   PioDCOStart(&DCO);
                   PioDCOSetFreq(&DCO, GEN_FRQ_HZ, 0UL);
                   setTX(true);
                   manualTX = true;
               } else {
                    if (testButton(TXSW) == true && manualTX == true) {
                      printf("Manual TX button released. Stopping TX...\n");
                      fflush(stdout);
                      PioDCOStop(&DCO);
                      setTX(false);
                      manualTX = false;
                    }
               }
               
        }
    }  
}   


