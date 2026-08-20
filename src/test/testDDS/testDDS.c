//*----------------------------------------------------------------------------
//* testDDS.c
//*----------------------------------------------------------------------------
//* Testbed for the ADX DDS PIO-based frequency synthesizer FT8 Transceiver
//* using Raspberry Pi Pico.
//* 
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

#include "hardware/clocks.h"
#include "pico/stdlib.h"

#include "defines.h"

#include "piodco/piodco.h"
#include "./build/dco2.pio.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "pico/stdio/driver.h"

#include "./lib/assert.h"
#include "./debug/logutils.h"
#include "hwdefs.h"

#include <GPStime.h>

#include <hfconsole.h>

#include "protos.h"
#include "testDDS.h"

//#define GEN_FRQ_HZ 29977777L

PioDco DCO; /* External in order to access in both cores. */
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
//*============================================================================*/
//*                              Main body                                     */
//*============================================================================*/
int main() 
{

    //*----------- Initial Processor board initialization & setup ---------------*
    initBoard();

    stdio_usb_init();
    while (!stdio_usb_connected()) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(22);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(22);
   }
   
    _INFOLIST("%s Firmware %s version(%s) build(%s)\n",__func__,PROGNAME,VERSION,BUILD);

    //*------------------------------------------------------------
    _INFOLIST("%s Hardware initialization\n",__func__);
    
//*-------------------------------------------------------
    _INFOLIST("%s Launching core 1 for DCO worker...\n",__func__);
    multicore_launch_core1(core1_entry);

//*-------------------------------------------------------
    _INFOLIST("%s Launching core 0 Generator\n",__func__);
    DDSGenerator();
}
/*============================================================================*/
/*                              CORE 1 PROCESSOR                              */
/* This is the code of dedicated core.                                        */
/* We deal with extremely precise real-time task.                             */
/*============================================================================*/
void core1_entry()
{
    const uint32_t clkhz = PLL_SYS_MHZ * 1000000L;

    /* Initialize DCO */
    assert_(0 == PioDCOInit(&DCO, 6, clkhz));

    /* Run DCO. */
    PioDCOStart(&DCO);

    /* Set initial freq. */
    assert_(0 == PioDCOSetFreq(&DCO, GEN_FRQ_HZ, 0u));

    /* Run the main DCO algorithm. It spins forever. */
    PioDCOWorker2(&DCO);
}

/*============================================================================*/
/*                              CORE 0 PROCESSOR                              */
/* This is the code to setup and launch the generator                         */                              
/*============================================================================*/

void RAM (DDSGenerator)(void)
{
    for(;;)
    {
       /* This generates a fixed frequency signal set by GEN_FRQ_HZ. */
       
        PioDCOSetFreq(&DCO, GEN_FRQ_HZ, 0u);
        blinkLED(1000);
    }
}
