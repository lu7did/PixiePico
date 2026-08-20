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
 * - HW pin realigment 
 *
 *   
 *----------------------------------------------------------------------------
 * Version 2.0
 * - Support Si4732 chip as a receiver
 * - Major realignment of hardware allocations (pico/rp2040Z/RDX compatibility) 
 *
 *   
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
 *----------------------------------------------------------------------------
 * ADX-rp2040 digital transceiver using the rp2040 (firmware and hardware)
 * Copyright (C) 2024- Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * Original porting of the ADX Arduino based firmware to the rp2040 architecture
 * Main architecture for board management ported from this version
 *----------------------------------------------------------------------------
 * The si4732 library has been written from the ground up, but it's strongly 
 * inspired, and for some features reverse engineered from the titanic work
 * made by Ricardo Caratti (PU2CLR) and specially from his worldwide recognized
 * library for the Arduino SI4735 (https://github.com/pu2clr/SI4735)
 * Unfortunately this library has been written in C++ and it's not compatible with the
 * C/C++ rp2040 SDK and many features of the Arduino platform aren't 
 * really that portable to the rp2040 environment and Visual Studio Code.
 * But I found myself searching on that code countless times to grasp the
 * understanding of many aspects, probably this work can not be possible
 * without that library. 
 *---------------------------------------------------------------------------------------*
 * This library receives the considerable learning made when developing the
 * ADX-rp2040 package and specially the RDX package which provides support 
 * for the rp2040 processor albeit using a cross platform compatibility layer
 * allowing the usage of the Arduino libraries and IDE to develop for other boards
 * in general and the rp2040 in particular, repositories for these projects are
 * 
 *     ADX-rp2040    https://github.com/lu7did/ADX-rp2040
 *     RDX           https://github.com/lu7did/RDX-rp2040
 *---------------------------------------------------------------------------------------*
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
#define VERSION  "2.0"
#define BUILD     "00"

#define BOOL2CHAR(x)  (x==true ? "True" : "False")
//*==============================================================================================*
//*                                  Build environment                                           *
//*==============================================================================================*
#define  PICO    1
//#define  RP2040Z  1
//#define   PICOW  1 

//*==============================================================================================*
//*                                Configuration definitions                                     *
//*==============================================================================================*
#define  DEBUG      1
#define  EEPROM     1   
#define  SUPERHET   1
#define  QUAD       1
#define  SI4732     1
#define  WAITSERIAL 1
#define  FS         1
//#define  RTC        1
//#define  CAT        1    
//*==============================================================================================*
//*                                Configuration consistency rules                               *
//*==============================================================================================*

#ifdef FS                        //EEPROM emulation or USB file system, FS prevails
#undef EEPROM
#endif //FS

#ifdef SI4732                    //If Si4732 chipset enabled it's the dominant receiver
#undef SUPERHET
#undef QUAD
#undef RTC
#endif //SI4732

#ifdef CAT                      //If CAT enabled USB can not be used to debug
#undef DEBUG
#undef RTC
#endif //CAT  

#ifdef QUAD                     //If Quadrature oscillator activated all other clocks disabled
#undef SUPERHET
#undef SI4732
#endif //QUAD 

#ifndef DEBUG
#undef WAITSERIAL
#undef RTC
#endif //!DEBUG

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
#include "ADX-ddsPIO.h"
#include <ctype.h>

#ifdef SI4732
#include "si4732.h"
#endif //SI4732

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
                                "[%s]: " fmt, __func__, ##__VA_ARGS__);  \
        if (_cdc_len > 0) {                             \
            if (_cdc_len > (int)sizeof(hi))       \
                _cdc_len = sizeof(hi);            \
            cdc_write(hi, (uint16_t)_cdc_len);    \
            tud_cdc_write_flush();                \
            tud_task();                           \
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
#define DEFAULT_MODE         4
#define DEFAULT_SLOT         3
#define DEFAULT_VOLUME      60


#define NBANDS                7
#define NMODES                4

#ifdef SI4732


#endif //SI4732
//*==============================================================================================*
//*                                  Hardware configuration                                      *
//*==============================================================================================*

#if defined(PICO) || defined(RP2040Z)
#define PICO_DEFAULT_LED_PIN 25
#endif //PICO || RP2040Z

#define pin_A0               26U          //pin for ADC (A2)
#define pin_SW                3U          //pin for freq change switch (D10,input)

#define  CLK0                13           //RF output (transmitter)
#define  CLK1                12           //RF output (receiver)

#ifdef SUPERHET
#define CLK2                 14           //RF out Receiver IF (465 KHz)

#endif //SUPERHET

#ifdef QUAD
#define RFI                 14
#define RFQ                 15
#endif //QUAD

/*---
   I2C configuration control 
*/
//#define I2C_PORT           i2c0
//#define SDA                  26           //I2C SDA (Data) bus
//#define SCL                  27           //I2C SCL (Clock) bus

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


/*----
   Definitions for the Si4732 support
*/
#ifdef SI4732

#define I2C_PORT i2c0
#define SDA_PIN              16  //There must be an alternative as the rp2040Z does not exposse this
#define SCL_PIN              17  //nor this but alternatives in clear view are 26 & 27 which are also used by
#define RST_PIN               1  //pin 9 is available in rp2040Z and free in RDX but 1 is in conflict


#define SI4732_DEFAULT_REGION  "ar"
#define SI4732_DEFAULT_MODE   "ssb"
#define SI4732_DEFAULT_BAND   "20m"
#define SI4732_DEFAULT_VOLUME    50
#define SI4732_DEFAULT_MUTE       0

#define SI4732_INIT_OK            0
#define SI4732_POWER_FAILURE      1
#define SI4732_INIT_FAILURE       2

#define SI4732_LOAD_PATCH         1

#endif //SI4732

//*==============================================================================================*
//*                                  Global Memory Areas                                         *
//*==============================================================================================*

ADX_ddsPIO_t ADX;                      //*--- System Variables
char hi[128];
uint8_t marker=0;
uint32_t t;
bool blink=false;

//*--- Control block of PIO running the DCO
PioDco DCO; /* External in order to access in both cores. */

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

static volatile bool cdc_dtr = false;

//*--- Define data areas for Si4732 operation

#ifdef SI4732
si4732_t radio;
#endif //SI4732

#ifdef FS
uint8_t JSON[2048];
ADX_ddsPIO_t fs;
#endif //FS
//*==============================================================================================*
//*                                  Prototypes                                                  *
//*==============================================================================================*

int slot2Band(int s);
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

#ifdef SI4732
static si4732_region_t parse_region(char *s);
static si4732_band_preset_t parse_band(char *s);
static void si4732_set_frequency(si4732_t *radio, uint32_t f);
static si4732_status_t  si4732_setup(si4732_t *radio);
static void si4732_dump_am_seek_props(si4732_t *r);


char    si4732_region[]=SI4732_DEFAULT_REGION;
char    si4732_mode[]=SI4732_DEFAULT_MODE;
char    si4732_band[]=SI4732_DEFAULT_BAND;
//uint8_t si4732_vol=SI4732_DEFAULT_VOLUME;
bool    si4732_mute=false;
#endif //SI4732

#ifdef EEPROM
void __attribute__((unused)) readEEPROM();
void __attribute__((unused)) updateEEPROM();
void __attribute__((unused)) resetEEPROM();
#endif //EEPROM


#ifdef FS


bool msc_read_config_json(uint8_t* out, uint32_t out_sz, uint32_t* out_len);
bool json_get_value(const char* json, const char* key, char* out, uint32_t out_sz);
bool msc_write_config_json(const uint8_t* json, uint32_t len, bool commit_now);
bool usb_msc_factory_reset(bool commit_now);

//*--- FS handling API API

void msc_boot_prepare(void);
const uint8_t* msc_disk_ro_ptr(void);
uint32_t msc_disk_size_bytes(void);
bool usb_msc_fw_write_config_sys(const uint8_t* data, uint32_t len, bool commit_now);
#endif //FS


#ifdef FS
//*==============================================================================================*
//*                      Tools to manage the FileSystem contents                                 *
//* This is a simple, low capacity, FAT-12 filesystem intended to contain a configuration file   *
//* called CONFIG.SYS (yes, like the old era DOS system) with the main variables of the board    *
//* it might contain other files within the limits of it's capacity but won't be updated by the  *
//* firmware. 
//*==============================================================================================*

//*----------------------------------------------------------------------------
//*  review if the existing configuration file needs to be replaced  
//*----------------------------------------------------------------------------
static bool need_replace_config(void)
{
  
  uint32_t n = 0;

  //*--- Read CONFIG.SYS file (JSON format)
  if (!msc_read_config_json(JSON, sizeof(JSON), &n)) {

     fs.ID = 1;
     fs.mode = ADX.mode;
     fs.slot = ADX.slot;
     fs.volume = ADX.volume;
     fs.bw = ADX.bw;
     fs.frqFT8 = ADX.frqFT8;
     fs.frqbfo = ADX.frqbfo;

     sprintf((char *)JSON,"{\n"             \
                         "\"ID\" : 1\n" \
                         "\"mode\" : %d\n" \
                         "\"slot\" : %d\n" \
                         "\"volume\" : %d\n" \
                         "\"bandwidth\" : %d\n" \
                         "\"frqFT8\" : %ld\n" \
                         "\"frqbfo\" : %ld\n" \
                         "}\n",
                         fs.mode,fs.slot,fs.volume,fs.bw,fs.frqFT8,fs.frqbfo);
   
    //*--- file doesn't exists, return true so the upcall might generate it
    return true;
  }

  char val[64];

  if (json_get_value((const char*)JSON,"mode",val,sizeof(val))) {
     ADX.mode = (uint8_t)atoi(val);
  }

  if (json_get_value((const char*)JSON,"slot",val,sizeof(val))){
     ADX.slot = (uint8_t)atoi(val);
  }

  if (json_get_value((const char*)JSON,"volume",val,sizeof(val))){
     ADX.volume = (uint8_t)atoi(val);
  }

  if (json_get_value((const char*)JSON,"bandwidth",val,sizeof(val))){
     ADX.bw = (uint8_t)atoi(val);
  }

  if (json_get_value((const char*)JSON,"frqFT8",val,sizeof(val))){
     ADX.frqFT8 = (uint32_t)atol(val);
  }

  if (!json_get_value((const char*)JSON,"frqbfo",val,sizeof(val))){
     ADX.frqbfo = (uint32_t)atol(val);
  }

  //*--- Return false if no update on the file is needed
  return true;
}

//*----------------------------------------------------------------------------
//*  this is a process that takes place before the MSC system is published  
//*----------------------------------------------------------------------------
static void boot_config_phase(void)
{
  //*--- Read RAMDisk from Flash memory
  msc_boot_prepare();

  //*--- Check if a change is neededb (FALSE), otherwise assume the file needs to be regenerated
  if (!need_replace_config()) return;

  (void)msc_write_config_json(JSON, strlen((char*)JSON), true);
  
}

//*----------------------------------------------------------------------------
//*  helpers to manage disk operations over the RAMDisk 
//*----------------------------------------------------------------------------
static inline uint16_t rd16_ro(const uint8_t* p, uint32_t off)
{
  return (uint16_t)(p[off] | ((uint16_t)p[off + 1] << 8));
}

static inline uint32_t rd32_ro(const uint8_t* p, uint32_t off)
{
  return (uint32_t)p[off] |
         ((uint32_t)p[off + 1] << 8) |
         ((uint32_t)p[off + 2] << 16) |
         ((uint32_t)p[off + 3] << 24);
}

static uint16_t fat12_next_cluster(const uint8_t* fat, uint32_t fat_bytes, uint16_t cl)
{
  // FAT12 entry: 12 bits
  uint32_t i = (uint32_t)cl + ((uint32_t)cl / 2u);
  if (i + 1u >= fat_bytes) return 0x0FFFu;

  uint16_t v = (uint16_t)(fat[i] | ((uint16_t)fat[i + 1u] << 8));
  if (cl & 1u) v >>= 4;
  else        v &= 0x0FFFu;

  return (uint16_t)(v & 0x0FFFu);
}

static bool fat_parse_layout(const uint8_t* disk, uint32_t disk_bytes,
                             uint32_t* fat_off, uint32_t* fat_bytes,
                             uint32_t* root_off, uint32_t* root_bytes,
                             uint32_t* data_off, uint32_t* bytes_per_cluster,
                             uint8_t*  nfats_out,
                             bool*     is_fat12_out)
{
  if (disk_bytes < 512u) return false;

  uint16_t bps      = rd16_ro(disk, 11);
  uint8_t  spc      = disk[13];
  uint16_t rsvd     = rd16_ro(disk, 14);
  uint8_t  nfats    = disk[16];
  uint16_t root_ent = rd16_ro(disk, 17);
  uint16_t tot16    = rd16_ro(disk, 19);
  uint16_t spf16    = rd16_ro(disk, 22);
  uint32_t tot32    = rd32_ro(disk, 32);

  uint32_t tot_sec = tot16 ? (uint32_t)tot16 : tot32;

  if (bps == 0 || spc == 0 || rsvd == 0 || nfats == 0 || root_ent == 0 || spf16 == 0 || tot_sec == 0) return false;

  uint32_t bpc = (uint32_t)bps * (uint32_t)spc;

  uint32_t fo = (uint32_t)rsvd * (uint32_t)bps;
  uint32_t fb = (uint32_t)spf16 * (uint32_t)bps;

  uint32_t ro = fo + (uint32_t)nfats * fb;
  uint32_t rb = (uint32_t)root_ent * 32u;
  uint32_t doff = ro + rb;

  if (fo + fb > disk_bytes) return false;
  if (ro + rb > disk_bytes) return false;
  if (doff >= disk_bytes) return false;

  //*--- Estimate FAT16 or FAT12 using cluster qty (standard rule)

  uint32_t data_bytes = disk_bytes - doff;
  if (bpc == 0) return false;
  uint32_t clusters = data_bytes / bpc;

  bool is_fat12 = (clusters < 4085u); // FAT12 threshold

  *fat_off = fo;
  *fat_bytes = fb;
  *root_off = ro;
  *root_bytes = rb;
  *data_off = doff;
  *bytes_per_cluster = bpc;
  *nfats_out = nfats;
  *is_fat12_out = is_fat12;
  return true;
}

static bool fat_root_find_83(const uint8_t* disk, uint32_t root_off, uint32_t root_bytes,
                            const char name11[11], uint32_t* ent_off_out)
{
  for (uint32_t o = 0; o + 32u <= root_bytes; o += 32u) {
    const uint8_t* e = disk + root_off + o;

    if (e[0] == 0x00) break;        // End of directory
    if (e[0] == 0xE5) continue;     // erase
    if (e[11] == 0x0F) continue;    // LFN

    if (memcmp(e, name11, 11) == 0) {
      *ent_off_out = root_off + o;
      return true;
    }
  }
  return false;
}

//*--- read the CONFIG.SYS file 

bool msc_read_config_json(uint8_t* out, uint32_t out_sz, uint32_t* out_len)
{
  if (!out || out_sz == 0) return false;
  if (out_len) *out_len = 0;

  const uint8_t* disk = msc_disk_ro_ptr();
  uint32_t disk_bytes = msc_disk_size_bytes();

  uint32_t fat_off, fat_bytes, root_off, root_bytes, data_off, bpc;
  uint8_t nfats;
  bool is_fat12;

  if (!fat_parse_layout(disk, disk_bytes, &fat_off, &fat_bytes, &root_off, &root_bytes,
                        &data_off, &bpc, &nfats, &is_fat12)) {
    return false;
  }

  const char name11[11] = { 'C','O','N','F','I','G',' ',' ','S','Y','S' };

  uint32_t ent_off = 0;
  if (!fat_root_find_83(disk, root_off, root_bytes, name11, &ent_off)) {
    return false; // no existe
  }

  uint16_t first_cluster = rd16_ro(disk, ent_off + 26);
  uint32_t file_size     = rd32_ro(disk, ent_off + 28);

  if (file_size == 0) {
    out[0] = 0;
    if (out_len) *out_len = 0;
    return true;
  }

  //*--- validate size 

  if (file_size >= out_sz) {

    //*--- format as a string
    file_size = out_sz - 1u;
  }

  const uint8_t* fat1 = disk + fat_off;

  uint32_t copied = 0;
  uint16_t cl = first_cluster;

  //*--- valid clusters starts with 2
  if (cl < 2u) return false;

  while (copied < file_size) {
    uint32_t cl_index = (uint32_t)(cl - 2u);
    uint32_t src_off  = data_off + cl_index * bpc;

    if (src_off + bpc > disk_bytes) return false;

    uint32_t chunk = file_size - copied;
    if (chunk > bpc) chunk = bpc;

    memcpy(out + copied, disk + src_off, chunk);
    copied += chunk;

    if (copied >= file_size) break;

    //*--- next cluster 

    if (is_fat12) {
      uint16_t nxt = fat12_next_cluster(fat1, fat_bytes, cl);
      if (nxt >= 0x0FF8u) break; // end-of-chain
      if (nxt < 2u) return false;
      cl = nxt;
    } else {
      //*--- uses FAT16 to help the host to format properly if needed
      uint32_t idx = (uint32_t)cl * 2u;
      if (idx + 1u >= fat_bytes) return false;
      uint16_t nxt = (uint16_t)(fat1[idx] | ((uint16_t)fat1[idx + 1u] << 8));
      if (nxt >= 0xFFF8u) break;
      if (nxt < 2u) return false;
      cl = nxt;
    }
  }

  out[copied] = 0; //*--- terminator, either as a TEXT or JSON file
  if (out_len) *out_len = copied;
  return true;
}

//*----------------------------------------------------------------------------
//*  this is a simple JSON file parser, no frills, escapes, fancy nests 
//*----------------------------------------------------------------------------
static const char* json_skip_ws(const char* s)
{
  while (*s && (unsigned char)*s <= 0x20) s++;
  return s;
}

//*--- Look for key

static bool json_match_key(const char* p, const char* key, const char** after_key)
{
  //*--- (p) points to the start of a JSON string
  if (*p != '"') return false;
  p++;
  const char* k = key;

  while (*p && *p != '"' && *k) {
    if (*p != *k) return false;
    p++; k++;
  }
  if (*k != 0) return false;   //*--- key not ended
  if (*p != '"') return false; //*--- string not closed
  p++;                         //*--- after the " character

  if (after_key) *after_key = p;
  return true;
}

static bool json_copy_string_value(const char* p, char* out, uint32_t out_sz, const char** after_val)
{
  //*--- (p) points to the first char after the initial quote
  uint32_t n = 0;
  while (*p && *p != '"') {
    //*--- minimum support for escape characters
    if (*p == '\\' && p[1]) {
      char esc = p[1];
      if (esc == '"' || esc == '\\' || esc == '/') {
         p++; 
      }
      //*-- other escapes
    }
    if (n + 1u < out_sz) {
      out[n] = *p;
    }  
    n++;
    p++;
  }
  
  if (*p != '"') return false;
  
  p++; //*--- uses clossure

  //*--- safe ending
  if (out_sz) {
    uint32_t w = (n < (out_sz - 1u)) ? n : (out_sz - 1u);
    out[w] = 0;
  }
  if (after_val) *after_val = p;
  return true;
}

static bool json_copy_literal_value(const char* p, char* out, uint32_t out_sz, const char** after_val)
{
  // number / true / false / null  (hasta , } o whitespace)
  uint32_t n = 0;
  while (*p && *p != ',' && *p != '}' && (unsigned char)*p > 0x20) {
    if (n + 1u < out_sz) out[n] = *p;
    n++;
    p++;
  }
  if (out_sz) {
    uint32_t w = (n < (out_sz - 1u)) ? n : (out_sz - 1u);
    out[w] = 0;
  }
  if (after_val) *after_val = p;
  return (n > 0);
}

bool json_get_value(const char* json, const char* key, char* out, uint32_t out_sz)
{
  if (!json || !key || !out || out_sz == 0) return false;
  out[0] = 0;

  const char* p = json_skip_ws(json);
  if (*p != '{') return false;
  p++;

  while (*p) {
    p = json_skip_ws(p);
    if (*p == '}') return false; //*--- end of string, not found

    //*---  search for key string

    const char* after_key = NULL;
    if (*p != '"') return false;

    //*--- compare with keyword 
    if (!json_match_key(p, key, &after_key)) {
      //*--- jump over if not the one
      p++; //*--- within the string
      while (*p && *p != '"') {
        if (*p == '\\' && p[1]) p += 2;
        else p++;
      }
      if (*p != '"') return false;
      p++;
      p = json_skip_ws(p);
      if (*p != ':') return false;
      p++;
      p = json_skip_ws(p);
      //*--- jump over
      if (*p == '"') {
        p++;
        while (*p && *p != '"') {
          if (*p == '\\' && p[1]) p += 2;
          else p++;
        }
        if (*p != '"') return false;
        p++;
      } else {
        while (*p && *p != ',' && *p != '}') p++;
      }
      p = json_skip_ws(p);
      if (*p == ',') { p++; continue; }
      if (*p == '}') return false;
      continue;
    }

    //*--- key found 
    p = json_skip_ws(after_key);
    if (*p != ':') return false;
    p++;
    p = json_skip_ws(p);

    if (*p == '"') {
      p++;
      return json_copy_string_value(p, out, out_sz, NULL);
    } else {
      return json_copy_literal_value(p, out, out_sz, NULL);
    }
  }

  return false;
}
//*--- write the JSON file and commit to flash if indicated

bool msc_write_config_json(const uint8_t* json, uint32_t len, bool commit_now)
{
  if (!json) return false;

  //*--- reuse FAT16 to create or replace CONFIG.SYS
  return usb_msc_fw_write_config_sys(json, len, commit_now);
}

#endif //IF
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

  
  cdc_printf("[%s]\nREQ=%" PRIu32 " Hz\n",tag,s->f_req_hz);
  tud_pump_task();


  cdc_printf("clk_sys(now)=%" PRIu32 " Hz\n",s->clk_sys_hz);
  tud_cdc_write_flush();
  tud_pump_task();
  
  cdc_printf("SOL: clk_sys=%" PRIu32 " Hz\n     VCO=%" PRIu32 " Hz\n",s->clk_sys_hz, s->vco_hz);
  tud_cdc_write_flush();
  tud_pump_task();


  cdc_printf("Divisor post=%u/%u  fbdiv=%u refdiv=%u\n",(unsigned)s->postdiv1, (unsigned)s->postdiv2,    (unsigned)s->fbdiv, (unsigned)s->refdiv);
  tud_cdc_write_flush();
  tud_pump_task();

  cdc_printf("SOL: N=%" PRIu32 "  div=%u+%u/256  f_out=%" PRIu32 " Hz  err=%" PRId32 " Hz\n",s->N, (unsigned)s->pio_div_int, (unsigned)s->pio_div_frac, s->f_out_hz, s->err_hz);
  tud_cdc_write_flush();
  tud_pump_task();

  cdc_printf("HW : N=%" PRIu32 "  div=%u+%u/256  f_out=%" PRIu32 " Hz (est)\n",N_hw, (unsigned)di_hw, (unsigned)df_hw, fout_hw_est); 
  tud_cdc_write_flush();
  tud_pump_task();

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
//int Band_slot = SLOT;                  //This is the default band Band1=1,Band2=2,Band3=3,Band4=4
//int mode      =    4;                  //Default mode is FT8
int Band;                                //This is the default band=slot[ADX.slot-1]

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

/*----------------------------------------------------------------------------*/
/* Forces the clean up of the TUD task queue                                  */
/*----------------------------------------------------------------------------*/
void pump_tud_task() {
  for (int i=0; i<100; i++) {
     tud_task();
     sleep_ms(1);
  }
}
/*----------------------------------------------------------------------------*/
/* Convert slot to band                                                       */
/*----------------------------------------------------------------------------*/
int slot2Band(int s) {
  if (s < 1 || s > 4) {
    s = 4;
  }
  int b=slot[s-1];
  cdc_printf(" slot(%d) assigned as  band(%d)\n", s, b);
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
cdc_printf("band(%d) is slot[%d]\n", b, i);
return i;

}
/*----------------------------------------------------------------------------*/
/* Assign band                                                                */
/*----------------------------------------------------------------------------*/
void Mode_assign() {

  cdc_printf(" mode(%d) band(%d)\n", ADX.mode, Band);
  int b=band2idx(Band);
  ADX.frqFT8=Bands[b][ADX.mode-1];
  PioDCOSetFreq(&DCO, ADX.frqFT8, 0U);    //*--- Change frequency according to band and mode
  
  #ifdef SI4732
  si4732_set_frequency(&radio,ADX.frqFT8);
  #endif //SI4732

  clearLED();

  //adc_drain();
  
  switch(ADX.mode) {
     case 1: gpio_put(WSPR,true);break;
     case 2: gpio_put(JS8,true);break;
     case 3: gpio_put(FT4,true);break;
     case 4: gpio_put(FT8,true);break;
  }

#ifdef EEPROM
  
  updateEEPROM();

#endif //EEPROM

  cdc_printf(" changed mode(%d) band(%d) index(%d) freq(%ld)\n", ADX.mode, Band, b, ADX.frqFT8);

}

/*----------------------------------------------------------------------------*/
/* Assign Band                                                                */
/*----------------------------------------------------------------------------*/
void Band_assign() {

  clearLED();
  Band=slot2Band(ADX.slot);

  switch(ADX.slot) {
     case 0: blinkLED(WSPR,3,100); break;
     case 1: blinkLED(JS8,3,100); break;
     case 2: blinkLED(FT4,3,100); break;
     case 3: blinkLED(FT8,3,100); break;
  }
    
  Mode_assign();
  cdc_printf("slot=%d mode=%d band=%d\n",ADX.slot, ADX.mode, Band);
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

       uint32_t f = ADX.frqFT8;

       #ifdef QUAD
          quad_stop(&osc);
          PioDCOStart(&DCO);
       #endif //QUAD

       #ifdef SI4732
          PioDCOStart(&DCO);
       #endif //SI4732   

       PioDCOSetFreq(&DCO, f, 0U);

       gpio_put(RXSW, 0);                   //(RXSW=1 enable Ant to RX, otherwise blocks it)
       gpio_put(TX, 1);                     //Turn on the TX led
       gpio_put(TXA, 0);                    //(TXA=0 enable OE of 74244)
       defaultLED(true);

      } else {

        uint32_t f = ADX.frqFT8;
        PioDCOSetFreq(&DCO, f, 0U);

        #ifdef QUAD
           PioDCOStop(&DCO);
           quad_start(&osc, f, false, &sol);
           dump_solution("I/Q Init ", &sol, osc.pio , osc.sm);
        #endif //QUAD

        #ifdef SI4732
           PioDCOStop(&DCO);
        #endif //SI4732

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
  cdc_printf(" TX activated\n");

  while(!testButton(TXSW));
  setTX(false);

  cdc_printf(" TX deactivated\n");

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
     switch(ADX.slot) {
       case 1: gpio_put(FT8,true); break;
       case 2: gpio_put(FT4,true); break;
       case 3: gpio_put(JS8,true); break;
       case 4: gpio_put(WSPR,true); break;
  }
  
  if ((!testButton(UP)) && (testButton(DOWN))) {    //UP Button pressed, decrease band slot
     ADX.slot--;
     if (ADX.slot<1) {
        ADX.slot=4;
     }
     while(testButton(UP)==false);
     cdc_printf("<UP> Band_slot=%d\n", ADX.slot);
  }

  if ((testButton(UP)) && (!testButton(DOWN))) {    //DOWN Button pressed, increase
     ADX.slot++;
     if (ADX.slot>4) {
        ADX.slot=1;
     }
     while(testButton(DOWN)==false);
     cdc_printf("<DOWN> slot=%d\n", ADX.slot);
  }

  if (!testButton(TXSW)) {
     gpio_put(TX,false);
     Band_assign();
     cdc_printf("completed set slot=%d\n", ADX.slot);
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
       cdc_printf("Band selection action started|n");
       pump_tud_task();
     }
  }
#endif //!CAT  

  /*----
     UP(Pressed) && DOWN(!Pressed) and !Transmitting --> Increase mode in sequence
  */
  #ifndef CAT
  if (!gpio_get(UP) && gpio_get(DOWN) && Tx_Status == 0) {
    pump_tud_task();
    if (!testButton(UP) && testButton(DOWN)){
        ADX.mode=ADX.mode-1;
        if (ADX.mode<1) ADX.mode=4;
        Mode_assign();
        while(!testButton(UP));
        cdc_printf("<UP> Mode down button released\n");
        PioDCOSetFreq(&DCO,ADX.frqFT8,0U);

        #ifdef SI4732
        si4732_set_frequency(&radio,ADX.frqFT8);
        #endif //SI4732
    
        #ifdef QUAD
        quad_set_frequency(&osc, ADX.frqFT8, false, &sol);  
        dump_solution("Button", &sol, osc.pio , osc.sm);
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
        cdc_printf("<DOWN> mode pressed Band selection action started|n");
         ADX.mode=ADX.mode+1;
        if (ADX.mode>4) ADX.mode=1;
        Mode_assign();
        while(!testButton(DOWN));

        PioDCOSetFreq(&DCO,ADX.frqFT8,0U);

        #ifdef SI4732
        si4732_set_frequency(&radio,ADX.frqFT8);
        #endif 

        #ifdef QUAD
        quad_set_frequency(&osc, ADX.frqFT8, false, &sol);
        dump_solution("Button ", &sol, osc.pio , osc.sm);
        #endif //QUAD
        cdc_printf("up mode pressed\n");
     }
  }
  #endif //!CAT

  /*----
     If the TX button is pressed then activate the transmitter until the button is released
  */
  if (!gpio_get(TXSW) && Tx_Status == 0){
     cdc_printf("TX key pressed\n");
     if (!testButton(TXSW)) {
        ManualTX();
     }
     cdc_printf("TX key released\n");
  }

}
//*===============================================================================================*/
//*                                                 CORE 1 PROCESSOR                              */
//* This is the code dedicated in CORE1 to work out the DCO, deal with a precise real-time task   */                                                           */
//*===============================================================================================*/
void core1_entry()
{
    cdc_printf("Core init, DCO worker started.\n");

    //*--- Set the DCO initial (default) frequency

    uint32_t f = ADX.frqFT8 + 0U;
    PioDCOStart(&DCO);
    PioDCOSetFreq(&DCO, f, 0U);

    setTX(false);
    
    //*--- Run the main DCO algorithm. It spins forever. */

    PioDCOWorker2(&DCO);
}
//*==============================================================================================*
//*                                  Board management                                            *
//*==============================================================================================*
/*
static bool inited = false;

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  (void) lun;

  if (!inited) {
    msc_disk_init();
    inited = true;
  }
  return true;
}
*/
//*----------------------------------------------------------------------------*/
//* Clean up the TUD queue                                                     */                             
//*----------------------------------------------------------------------------*/
void tud_pump_task() {
   for (int i=0;i<10;i++) {
      tud_task();
      sleep_ms(1);
   }
}
//*----------------------------------------------------------------------------*/
//* Open the TinyUSB (TUD) interface and wait for it to be opened              */                             
//*----------------------------------------------------------------------------*/
void TUDstart(){
  //*--- Start the USB service loop

  tud_init(BOARD_TUD_RHPORT);
  //tusb_init();

  absolute_time_t t0 = get_absolute_time();
  while (!tud_mounted()) {
    tud_task();
    if (absolute_time_diff_us(t0, get_absolute_time()) > 3 * 1000 * 1000) {
        break;           // timeout 3 secs, do not hang the system
    }
  }

}

//*----------------------------------------------------------------------------*/
//* Update the EEPROM values with the current operating values                 */                             
//*----------------------------------------------------------------------------*/
#ifdef EEPROM
void __attribute__((unused)) resetEEPROM() {

  ADX_ddsPIO_t eeprom;
  
  eeprom.ID    = 0x01;
  eeprom.mode  = DEFAULT_MODE;
  eeprom.slot  = DEFAULT_SLOT;
  eeprom.volume= DEFAULT_VOLUME;
  eeprom.bw    = SI473X_AM_BW_4KHZ;
  eeprom.frqFT8= GEN_FRQ_HZ;
  eeprom.frqbfo= FREQ_BFO;

  EEPROM_write(&eeprom);
  cdc_printf("EEPROM configuration reset\nID(%d) mode(%d) slot(%d) f(%ld) bfo(%ld) volume(%d) bandwidth(%d)\n",eeprom.ID,ADX.mode,ADX.slot,ADX.frqFT8,ADX.frqbfo,ADX.volume,ADX.bw);

}

//*----------------------------------------------------------------------------*/
//* Update the EEPROM values with the current operating values                 */                             
//*----------------------------------------------------------------------------*/
void __attribute__((unused)) updateEEPROM() {

  ADX_ddsPIO_t eeprom;
  
  eeprom.ID    = 0x01;
  eeprom.mode  = (uint8_t)ADX.mode;
  eeprom.slot  = (uint8_t)ADX.slot;
  eeprom.volume=ADX.volume;
  eeprom.bw    =ADX.bw;
  eeprom.frqFT8=ADX.frqFT8;
  eeprom.frqbfo=ADX.frqbfo;

  EEPROM_write(&eeprom);
  cdc_printf("EEPROM configuration updated\nID(%d) mode(%d) slot(%d) f(%ld) bfo(%ld) volume(%d) bandwidth(%d)\n",eeprom.ID,ADX.mode,ADX.slot,ADX.frqFT8,ADX.frqbfo,ADX.volume,ADX.bw);

}

//*----------------------------------------------------------------------------*/
//* Read the EEPROM values and use them as the operating defintion             */
//*----------------------------------------------------------------------------*/
void __attribute__((unused)) readEEPROM() {
    
    //*--- update EEPROM if not initialized yet
    ADX_ddsPIO_t eeprom;
    EEPROM_read(&eeprom);

    if (eeprom.ID != 0x01) {
       updateEEPROM();
       sleep_ms(10);
       cdc_printf("EEPROM empty, reset to defaults\n");
    } else {
       ADX.mode=eeprom.mode;
       ADX.slot=eeprom.slot;
       ADX.frqFT8=eeprom.frqFT8;
       ADX.frqbfo=eeprom.frqbfo;
       ADX.volume=eeprom.volume;
       ADX.bw=eeprom.bw;
       cdc_printf("EEPROM configuration recovered\nID(%d) mode(%d) slot(%d) f(%ld) bfo(%ld) volume(%d) bandwidth(%d)\n",eeprom.ID,ADX.mode,ADX.slot,ADX.frqFT8,ADX.frqbfo,ADX.volume,ADX.bw);
      }     
}
#endif //EEPROM
//*----------------------------------------------------------------------------*/
//* Wait for the serial monitor to open before continuing                      */                             
//*----------------------------------------------------------------------------*/
void waitserial() {

  gpio_put(PICO_DEFAULT_LED_PIN,blink); 
  blink=false;
  t=to_ms_since_boot(get_absolute_time());
  while (true) {
    tud_task();                 // mantiene USB vivo
    if (tud_cdc_connected() && cdc_dtr) break;  // host abrió el puerto
    sleep_ms(1);
    if (to_ms_since_boot(get_absolute_time())-t > 200) {
        t=to_ms_since_boot(get_absolute_time());
        blink=!blink;
        gpio_put(PICO_DEFAULT_LED_PIN,blink); 
      }
  }

  gpio_put(PICO_DEFAULT_LED_PIN,0);  //*--- Turn on left when Serial monitor has been opened
  tud_pump_task();

}
#if defined(FS) || defined(EEPROM)
//*----------------------------------------------------------------------------*/
//* Check for TXSW pressed on start up and reset if detected                   */
//*----------------------------------------------------------------------------*/
bool resetDefaults() {

  if (gpio_get(TXSW)) {               //If TXSW is high then leave
     return false;
  }

  #ifdef EEPROM
   //*--- If pressed wait till release and the reset to factory defaults

  cdc_printf("Release TX button to reset\n");
  gpio_put(PICO_DEFAULT_LED_PIN,1); 
  blink=false;
  t=to_ms_since_boot(get_absolute_time());
  while (!gpio_get(TXSW)) {
    tud_task();                 // mantiene USB vivo
    if (to_ms_since_boot(get_absolute_time())-t > 2000) {
        t=to_ms_since_boot(get_absolute_time());
        blink=!blink;
        gpio_put(PICO_DEFAULT_LED_PIN,blink); 
    }
  }

  gpio_put(PICO_DEFAULT_LED_PIN,0);  //*--- Turn on left when Serial monitor has been opened
  tud_pump_task();

  resetEEPROM();
  return true;
#endif //EEPROM

#if FS

gpio_put(PICO_DEFAULT_LED_PIN,1); 
blink=false;
t=to_ms_since_boot(get_absolute_time());

while(!gpio_get(TXSW)) {
  if (to_ms_since_boot(get_absolute_time())-t > 100) {
    t=to_ms_since_boot(get_absolute_time());
    blink=!blink;
    gpio_put(PICO_DEFAULT_LED_PIN,blink); 
  }
}  
usb_msc_factory_reset(true); 
return true;
#endif //FS

}
#endif //FS || EEPROM
//*----------------------------------------------------------------------------*/
//* Setup the default configuration of the board                               */                             
//*----------------------------------------------------------------------------*/
void ADXinit(){

  ADX.mode   = DEFAULT_MODE;
  ADX.slot   = DEFAULT_SLOT;
  ADX.volume = DEFAULT_VOLUME;
  ADX.bw     = SI473X_AM_BW_4KHZ;
  ADX.frqFT8 = GEN_FRQ_HZ;
  ADX.frqbfo = FREQ_BFO;
  Band       = slot2Band(ADX.slot);

  //*--- define the DEFAULT (board) LED

  #if defined(PICO) || defined(RP2040Z)
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  defaultLED(true);
  #else
  cyw43_arch_init(); 
  #endif //!PICOW

  gpio_init(TXSW);
  gpio_set_dir(TXSW, GPIO_IN);
  gpio_pull_up(TXSW);


}

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
    gpio_pull_up(TXSW);

    gpio_init(UP);
    gpio_set_dir(UP, GPIO_IN);
    gpio_pull_up(UP);

    gpio_init(DOWN);
    gpio_set_dir(DOWN, GPIO_IN);    
    gpio_pull_up(DOWN);
    
  
    //*--- End of ADX control board initialization
    cdc_printf("ADX I/O control board initialized\n");
    tud_pump_task();
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
  ADXinit();

  // --- fase determinística de configuración (sin USB) ---
  #ifdef FS
 
  //*--- Check to reset to defaults and commit

  #ifdef FS
  bool resetConfig=resetDefaults();
  #endif //FS
  
  //*--- Read FS
  boot_config_phase();
  #endif //FS

  TUDstart();

  #ifdef WAITSERIAL
  waitserial();
  #endif //WAITSERIAL
  
  cdc_printf("%s version(%s) build(%s)\n",PROGNAME,VERSION,BUILD);
  tud_pump_task();

  ADXsetup();
  tud_pump_task();

  #if EEPROM
  //*--- Sense if the TXSW button is pressed on startup, if so reset EEPROM, then read EEPROM back
  bool resetConfig=resetDefaults();
  cdc_printf("EEPROM configuration has been RESET\n");
  tud_pump_task();
  readEEPROM();
  tud_pump_task();
  #endif //EEPROM

  #ifdef FS
  if (resetConfig) {
     cdc_printf("File system has been RESET\n");
  }
  cdc_printf("Content of CONFIG.SYS file\n%s",(const char*)JSON);
  tud_pump_task();
  cdc_printf("CONFIG.SYS recovered values\n"    \
             "mode=%d\n"  \
             "slot=%d\n"  \
             "volume=%d\n"  \
             "bandwidth=%d\n"  \
             "frqFT8=%ld\n"  \
             "frqbfo=%ld\n",fs.mode,fs.slot,fs.volume,fs.bw,fs.frqFT8,fs.frqbfo);          
  tud_pump_task();
  #endif //FS

  #ifdef RTC
  //*----------- Setup RTC, this is only used to sync seconds   ---------------*
  rtc_init();
  rtc_set_datetime(&tcpu);
  cdc_printf("Support for RTC loaded\n");
  #endif //RTC


  //*---- Start the local 465 KHz BFO oscillator if enabled
  #ifdef SUPERHET
  PIO piobfo = pio1;
  uint sm = 0;
  uint offset = (uint) pio_add_program(piobfo, &BFO_program);
  float fbfo=(float)ADX.frqbfo*1.0f;
  pio_square_wave(piobfo, sm, offset, CLK2, fbfo);
  cdc_printf("Superhet support BFO initialized\n");
  #endif //SUPERHET 

  //*--- Start the DCO (either single or dual clock)

  const uint32_t PIOclkhz = PLL_SYS_MHZ * 1000000L;
  
  //*--- Output is produced replicated at GPIO13 and GPIO14 
 
  cdc_printf("DCO sub-system initializing...\n");
  PioDCOInit(&DCO, CLK1, PIOclkhz,true);
  tud_pump_task();

  //*--- Start the quadrature oscilator

  #ifdef QUAD
  quad_init(&osc, pio1, 0,(int)RFI,(int)RFQ);
  cdc_printf("Quadrature oscillator started\n");
  #endif //QUAD


  #ifdef RTC
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
  #endif //RTC

  #ifdef SI4732
  si4732_status_t s=si4732_setup(&radio);
  cdc_printf("Si4732 initialization procedure completed rc(%d)\n",s);
  tud_pump_task();
  #endif //SI4732 
   
  //*--- Wireless library poll
  #ifdef PICOW
  cyw43_arch_poll(); 
  #endif //PICOW

  //*--- Initialize the ADX board
  cdc_printf("Core 1 started. DCO worker initializing...\n");
 
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

  //*--- If operating on a Si4732 board initialize the chipset

  cdc_printf("DCO worker launching at core 1...\n");
  multicore_launch_core1(core1_entry);
  sleep_ms(500);
  
  #ifdef QUAD
  //*--- Start the oscillator
  quad_start(&osc, ADX.frqFT8, false, &sol);
  dump_solution("Start I/Q", &sol, osc.pio , osc.sm);
  cdc_printf("Quad oscillator started\n");
  #endif //QUAD

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

//*--- Start the quadrature digital oscilator (QDO) if configured

/*
  #ifdef QUAD
  quad_init(&osc, pio1, 0,(int)RFI,(int)RFQ);
  #endif //QUAD
*/
  //*--- Get time for future synchronization

  Tx_last_mod_time=to_ms_since_boot(get_absolute_time());
  
  //*--- Enter the infinite loop
  t=to_ms_since_boot(get_absolute_time());
  blink=false;
  cdc_printf("Transceiver  ready\n");

  //*----------------------------------------------------------------------------*/
  //*                    This is the main service loop                           */
  //*----------------------------------------------------------------------------*/
  while (true)
  {
  
    //*--- TUD (TinyUSB Dispatcher call)
    tud_task();        // TinyUSB device task

    if (to_ms_since_boot(get_absolute_time())-t > 1000) {
        t=to_ms_since_boot(get_absolute_time());
        blink=!blink;
        gpio_put(PICO_DEFAULT_LED_PIN,blink); 
    }
  
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
       uint32_t f = ADX.frqFT8 + (uint32_t)audio_freq;

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

        ADX.frqFT8 = (uint32_t)freqset;       
        PioDCOSetFreq(&DCO,ADX.frqFT8,0U);

        #ifdef QUAD
        quad_set_frequency(&osc, ADX.rqFT8, false, &sol)
        dump_solution("CAT", &sol, osc.pio , osc.sm);

        #endif //QUAD

        #ifdef QUAD

        #endif //QUAD
        
        adc_fifo_drain ();
        adc_offset = adc();
      }
    }
    strcpy(sent, "FA"); // Return 11 digit frequency in Hz.
    snprintf(parameter, 12, "%011d", (int)ADX.frqFT8);
    strcat(sent,parameter); 
    strcat(sent, ";");
  }
  else if (strcmp(command,"FB")==0) {                   
    strcpy(sent, "FB"); // Return 11 digit frequency in Hz.
    snprintf(parameter, 12, "%011d", (int)ADX.frqFT8);
    strcat(sent,parameter); 
    strcat(sent, ";");
  }
  else if (strcmp(command,"IF")==0) {          
    strcpy(sent, "IF"); // Return 11 digit frequency in Hz.  
    snprintf(parameter, 12, "%011d", (int)ADX.frqFT8);
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

/*----------------------------------------------------------------------------------*/
//* Print string thru CDC                                                           */
//*---------------------------------------------------------------------------------*/
static void cdc_write_str(const char *s) {
  if (!tud_cdc_connected()) return;
  tud_cdc_write_str(s);
  tud_cdc_write_flush();
}
/*----------------------------------------------------------------------------------*/
//* strip characters from a string                                                  */
//*---------------------------------------------------------------------------------*/
void strip(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++; // Always copy the character from read to write position and advance read
        if (*pw != c) {
            pw++;      // Advance write pointer only if the character is kept
        }
    }
    *pw = '\0'; // Null-terminate the new, shorter string
}
/*----------------------------------------------------------------------------------*/
//* Print line using CDC                                                            */
//*---------------------------------------------------------------------------------*/
static void __attribute__((unused))
cdc_write_ln(const char *s) {
  cdc_write_str(s);
  cdc_write_str("\r\n");
}

/*----------------------------------------------------------------------------------*/
//* TinyUSB callback to detect changes in DTR/RTS                                   */
//*---------------------------------------------------------------------------------*/
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void)itf;
  (void)rts;
  cdc_dtr = dtr;
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

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*                               Si4732 Receiver Sub-System
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

/*----------------------------------------------------------------------------------*/
//* Convert string token region code                                                */
//*---------------------------------------------------------------------------------*/
static si4732_region_t __attribute__((unused)) parse_region(char *s) {
  if (!s) return SI4732_REGION_AR;
  if (!strcmp(s, "us")) return SI4732_REGION_US;
  if (!strcmp(s, "eu")) return SI4732_REGION_EU;
  if (!strcmp(s, "jp")) return SI4732_REGION_JP;
  return SI4732_REGION_AR;
}

/*----------------------------------------------------------------------------------*/
//* parse_band                                                                      */
//*---------------------------------------------------------------------------------*/

static si4732_band_preset_t __attribute__((unused)) parse_band(char *s) {
  if (!s) return SI4732_BAND_FM_BROADCAST;
  if (!strcmp(s, "fm"))  return SI4732_BAND_FM_BROADCAST;
  if (!strcmp(s, "mw"))  return SI4732_BAND_AM_MW;
  if (!strcmp(s, "49m")) return SI4732_BAND_SW_49M;
  if (!strcmp(s, "80m")) return SI4732_BAND_HAM_80M;
  if (!strcmp(s, "40m")) return SI4732_BAND_HAM_40M;
  if (!strcmp(s, "30m")) return SI4732_BAND_HAM_30M;
  if (!strcmp(s, "20m")) return SI4732_BAND_HAM_20M;
  if (!strcmp(s, "18m")) return SI4732_BAND_HAM_17M;
  if (!strcmp(s, "15m")) return SI4732_BAND_HAM_15M;
  if (!strcmp(s, "12m")) return SI4732_BAND_HAM_12M;
  if (!strcmp(s, "10m")) return SI4732_BAND_HAM_10M;

  return SI4732_BAND_HAM_20M;
}

/*----------------------------------------------------------------------------------*/
//* Set the frequency                                                               */
//*---------------------------------------------------------------------------------*/
static void si4732_set_frequency(si4732_t *radio, uint32_t f) {

  //*--- setup the operating frequency

  uint32_t fx = f/1000;
  si4732_status_t rc = si4732_tune(radio, fx);
  cdc_printf("f=%lu kHz rc=%d last=0x%02X\n",
             (unsigned long)fx, (int)rc, radio->last_status);
  tud_pump_task();
  sleep_ms(200);

  //*--- validate how the setup of the chip really is

  uint32_t rf = 0;
  uint8_t rssi=0, snr=0;
  bool stc=false;
  rc = si4732_get_tune_status(radio, true, &rf, &rssi, &snr,&stc);
  cdc_printf("rc=(%d) freq=%lu (mode=%d) rssi=%u snr=%u stc(%d) last=0x%02X\n",
              (int)rc, (unsigned long)rf, (int)radio->mode, rssi, snr, stc, radio->last_status);

}

static void si4732_dump_am_seek_props(si4732_t *r) {
  uint16_t v=0;
  si4732_get_property(r, 0x3400, &v); cdc_printf("si4732: (prop) AM_BOTTOM=%u\n", v);
  si4732_get_property(r, 0x3401, &v); cdc_printf("si4732: (prop) AM_TOP=%u\n", v);
  si4732_get_property(r, 0x3402, &v); cdc_printf("si4732: (prop) AM_SPACING=%u\n", v);
}

/*----------------------------------------------------------------------------------*/
//* Master setup procedure for the si4732 chip                                      */
//*---------------------------------------------------------------------------------*/
 static si4732_status_t  si4732_setup(si4732_t *radio) {

  cdc_printf("Starting setup procedure\n");

  //*--- Ensure the RESET pin stays high at the start
  gpio_put(RST_PIN,1);

  //*--- Initialize device

  si4732_status_t rc = si4732_init(radio, I2C_PORT, SI4732_I2C_ADDR_DEFAULT, SDA_PIN, SCL_PIN, RST_PIN, 400000);
  cdc_printf("init rc=(%d) present=%d last=0x%02X\n", (int)rc, (int)radio->present, radio->last_status);

  //*--- If init went ok then power up (boot the internal engine)

  if (rc == SI4732_OK) {

      rc = si4732_power_down(radio);
      cdc_printf("power down AM/SSB rc=(%d) last_status=0x%02X\n",(int)rc, radio->last_status);
      tud_pump_task();
      sleep_ms(200);

      rc = si4732_power_up_am(radio, false);
      cdc_printf("power up AM/SSB rc=(%d) last_status=0x%02X\n",(int)rc, radio->last_status);
      tud_pump_task();
      sleep_ms(200);

       //*--- If the power up went wrong message it

     if (rc != SI4732_OK) {
        cdc_printf("power up failure to power up device rc(%d)\n",rc);
        return (si4732_status_t)SI4732_POWER_FAILURE;
     }
  } else {
     cdc_printf("power failure to initialize device rc(%d)\n",rc);
     return (si4732_status_t)SI4732_INIT_FAILURE;
  }
  
    //*--- Apply default volume, this is fixed since the transceiver has no volume control

  if (ADX.volume > 64) {
     cdc_printf("volume out of range\n");
     ADX.volume=63;
  }

  rc = si4732_set_volume(radio, ADX.volume);
  cdc_printf("set volume vol(%d) rc=%d last_status=0x%02X\n",(int)ADX.volume,(int)rc, radio->last_status);

  //*--- Unmute the receiver

  rc = si4732_set_mute(radio, false, false);
  cdc_printf("set mute rc=(%d) last_status=0x%02X\n",(int)rc, radio->last_status);

  //*--- Now set Ham 20m band ---

  si4732_band_preset_t bp = SI4732_BAND_HAM_20M;
  si4732_band_t b = si4732_band_preset(bp, radio->region_profile);
  b.mode = SI4732_MODE_AM;                 // <-- importante: primero AM

  cdc_printf("band preset mode(%d) bottom(%lu) top(%lu)\n",
            (int)b.mode, (unsigned long)b.min, (unsigned long)b.max);
  tud_pump_task();
  sleep_ms(200);

  rc = si4732_set_band(radio, &b);
  cdc_printf("set band rc(%d)\n", (int)rc);
  tud_pump_task();
  sleep_ms(200);

  //*--- Trace how the setup has been reflected on the chip

  si4732_dump_am_seek_props(radio);
  tud_pump_task();
  sleep_ms(200);


//*-- The properties might not follow the set, however unless a seek is made aren't that useful

  uint16_t am_bot=0, am_top=0;
  si4732_get_property(radio, 0x3400, &am_bot);
  si4732_get_property(radio, 0x3401, &am_top);
  cdc_printf("WARNING band NOT applied (bot=%u top=%u)\n", am_bot, am_top);

  //*--- Tune on the default frequency
  
  si4732_set_frequency(radio,ADX.frqFT8);

  //*--- Now start the process to reset to SSB, handshake and load the patch

  cdc_printf("NOW it reset to SSB");
  
  //*--- first power down
  rc = si4732_power_down(radio);
  cdc_printf("power down rc=%d\n", (int)rc);
  tud_pump_task();
  sleep_ms(200);
  
  //*--- then reset the chip
  (void)si4732_reset_pulse(radio, 10, 100);
  cdc_printf("reset pulse rc=%d\n", (int)rc);
  sleep_ms(200);

  //*--- Then power up for AM with patch flag activated
  rc = si4732_power_up_am(radio, true);    
  cdc_printf("power up patch rc=%d\n", (int)rc);
  tud_pump_task();
  sleep_ms(200);

  //*--- Load SSB patch

  cdc_printf("Status before patch: present=%d mode=%d last=0x%02X\n",
             radio->present, radio->mode, radio->last_status);
  rc = si4732_load_patch(radio, si4732_ssb_patch, si4732_ssb_patch_len);
  cdc_printf("load patch) rc=%d\n", (int)rc);
  
  if (rc != SI4732_OK) {
     (void)si4732_power_down(radio);
     (void)si4732_reset_pulse(radio, 10, 100);
     (void)si4732_power_up_am(radio, true);
     cdc_printf("load patch failed chip reset performed\n");
  }

  tud_pump_task();
  sleep_ms(400);

  //*--- Now place the driver in SSB mode
  //radio->mode = SI4732_MODE_SSB;
  rc=si4732_ssb_enter(radio);
  cdc_printf("ssb mode enter SSB mode rc(%d)\n",rc);

  if (ADX.bw > 6){
     ADX.bw=SI473X_AM_BW_4KHZ; 
  }
  rc = si4732_set_am_bandwidth(radio,ADX.bw , true, 200);
  cdc_printf("set bandwidth(%d) rc(%d)\n",ADX.bw, rc);
  tud_pump_task();
  sleep_ms(400);

  //*--- Apply the band required
  si4732_band_t b2 = si4732_band_preset(SI4732_BAND_HAM_20M, radio->region_profile);
  b2.mode = SI4732_MODE_SSB; 

  //*--- Set band
  rc = si4732_set_band(radio, &b2);
  cdc_printf("set band post-patch) rc=%d\n", (int)rc);
  si4732_dump_am_seek_props(radio);
  tud_pump_task();
  sleep_ms(200);

  //*--- Tune on the default frequency
  
  si4732_set_frequency(radio,ADX.frqFT8);
  cdc_printf("Frequency set to %ld Hz\n",ADX.frqFT8);
  cdc_printf("Setup completed\n");
  return (si4732_status_t)SI4732_INIT_OK;
}