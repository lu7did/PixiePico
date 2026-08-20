/*
 * =======================================================================================
 * si4723_rp2040
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
 *----------------------------------------------------------------------------
 *                       This program uses a RDX_rp2040 board
 *  modifications will be made later to adapt the hardware configuration to suit
 *  the ADX-ddsPIO project which is the one that will receive the integration 
 *----------------------------------------------------------------------------
 *
 * Implementation of a SI4732 based demo with TinyUSB CDC (stand-alone, integration-friendly)
 * - CDC command interface for debug/control
 * - I2C on GPIO16 (SDA) and GPIO17 (SCL)
 *
 * Commands (send via USB serial):
 *   help
 *   region us|eu|jp|ar
 *   mode 
 *   fm|am
 *   band fm|mw|49m|40m|31m
 *   tune <freq>
 *     - FM: <freq> in 10kHz units (e.g. 10030 -> 100.30 MHz)
 *     - AM: <freq> in kHz (e.g. 7100)
 *   seek up|down
 *   vol <0..63>
 *   mute <0|1>
 *----------------------------------------------------------------------------
  */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "tusb.h"
#include "si4732.h"

char hi[128];
bool blink=false;

static volatile bool cdc_dtr = false;
//*----------------------------------------------------------------------------
//*                             Prototypes
//*----------------------------------------------------------------------------
void cdc_write(char *buf, uint16_t length);

//*----------------------------------------------------------------------------
//*                             Board configuration
//*----------------------------------------------------------------------------
#define I2C_PORT i2c0
//#define I2C_PORT i2c1

//*--- this is the pinout configuration of the RDX board used to develop this

#define SDA_PIN        16          //There must be an alternative as the rp2040Z does not exposse this
#define SCL_PIN        17          //nor this but alternatives in clear view are 26 & 27 which are also used by
                                   //the ADC block and therefore in conflict with the RX integration
//#define SDA_PIN        0         //or 0 and 1 which in the RDX board 1 is used for the RESET and it'd hard to 
//#define SCL_PIN        1         //introduce a MOD for that


#define RST_PIN         1          //pin 9 is available in rp2040Z and free in RDX but 1 is in conflict
//#define RST_PIN         9        //so at this point pin assignment is used differently in rp2040Z and RDX

#define TX              3  //TX LED
#define FT8             4  //FT8 LED
#define TXSW            8  //RX-TX Switch

#define BLINKFAST 200
#define BLINKSLOW 1000

//*----------------------------------------------------------------------------
//*                         USB CDC functions
//*----------------------------------------------------------------------------
static void cdc_write_str(const char *s) {
  if (!tud_cdc_connected()) return;
  tud_cdc_write_str(s);
  tud_cdc_write_flush();
}

static void cdc_write_ln(const char *s) {
  cdc_write_str(s);
  cdc_write_str("\r\n");
}
//*----------------------------------------------------------------------------
//*                      Command parsing and execution
//*----------------------------------------------------------------------------

//*--- decode and apply region

static si4732_region_t parse_region(const char *s) {
  if (!s) return SI4732_REGION_AR;
  if (!strcmp(s, "us")) return SI4732_REGION_US;
  if (!strcmp(s, "eu")) return SI4732_REGION_EU;
  if (!strcmp(s, "jp")) return SI4732_REGION_JP;
  return SI4732_REGION_AR;
}

//*--- decode and apply band

static si4732_band_preset_t parse_band(const char *s) {
  if (!s) return SI4732_BAND_FM_BROADCAST;
  if (!strcmp(s, "fm"))  return SI4732_BAND_FM_BROADCAST;
  if (!strcmp(s, "mw"))  return SI4732_BAND_AM_MW;
  if (!strcmp(s, "49m")) return SI4732_BAND_SW_49M;
  if (!strcmp(s, "40m")) return SI4732_BAND_SW_40M;
  return SI4732_BAND_SW_31M;
}

//*--- print help 

static void print_help(void) {
  cdc_write_ln("command SI4732 CDC Demo commands:");
  cdc_write_ln("  help");
  cdc_write_ln("  region us|eu|jp|ar");
  cdc_write_ln("  mode fm|am");
  cdc_write_ln("  band fm|mw|49m|40m|31m");
  cdc_write_ln("  tune <freq>   (FM: 10kHz units, AM: kHz)");
  cdc_write_ln("  seek up|down");
  cdc_write_ln("  vol <0..63>");
  cdc_write_ln("  mute <0|1>");
}

//*--- Actual command processor

static void process_line(si4732_t *radio, char *line) {
  
  //*--- tokenize and process
  
  char *cmd = strtok(line, " \t\r\n");
  if (!cmd) {
    cdc_printf("command **Error** no command received\n");
    return;
  }

  cdc_printf("command received(%s)\n",cmd);

  if (!strcmp(cmd, "help")) {
    print_help();
    return;
  }

  if (!strcmp(cmd, "region")) {
    char *arg = strtok(NULL, " \t\r\n");
    si4732_region_t r = parse_region(arg);
    si4732_status_t rc = si4732_apply_region(radio, r);  
    cdc_printf("command region set rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);

    return;
  }

  if (!strcmp(cmd, "mode")) {
    char *arg = strtok(NULL, " \t\r\n");
    if (arg && !strcmp(arg, "fm")) {
      si4732_status_t rc = si4732_power_up_fm(radio);
      cdc_printf("command mode FM rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);

      return;
    }
    if (arg && !strcmp(arg, "am")) {
      si4732_status_t rc = si4732_power_up_am(radio, false);
      cdc_printf("command mode AM rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);

      return;
    }
    cdc_write_ln("command **error*  mode fm|am");

    return;
  }

  if (!strcmp(cmd, "band")) {
    char *arg = strtok(NULL, " \t\r\n");
    si4732_band_preset_t bp = parse_band(arg);
    si4732_band_t b = si4732_band_preset(bp, radio->region_profile);

    //*--- Ensure correct power up for FM/AM

    si4732_status_t rc = (b.mode == SI4732_MODE_FM) ? si4732_power_up_fm(radio) : si4732_power_up_am(radio, false);
    cdc_printf("command power up rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);

    if (rc != SI4732_OK) { cdc_write_ln("ERR: power up failed"); return; }

    rc = si4732_set_band(radio, &b);
    cdc_printf("command band set rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);
    return;
  }

  if (!strcmp(cmd, "tune")) {
    char *arg = strtok(NULL, " \t\r\n");
    if (!arg) { cdc_write_ln("ERR: tune <freq>"); return; }
    uint32_t f = (uint32_t)strtoul(arg, NULL, 10);
    si4732_status_t rc = si4732_tune(radio, f);
    cdc_printf("command tune rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);

    return;
  }

  if (!strcmp(cmd, "seek")) {
    char *arg = strtok(NULL, " \t\r\n");
    if (!arg) { cdc_write_ln("ERR: seek up|down"); return; }
    bool up = !strcmp(arg, "up");
    si4732_status_t rc = si4732_seek(radio, up, true);
    cdc_printf("command seek rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);

    return;
  }
  


  if (!strcmp(cmd, "loadpatch")) {
     si4732_status_t rc = si4732_load_patch(radio, si4732_ssb_patch, si4732_ssb_patch_len);
     cdc_printf("command load patch rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);
     return;
  }


  if (!strcmp(cmd, "vol")) {
    char *arg = strtok(NULL, " \t\r\n");
    if (!arg) { cdc_write_ln("ERR: vol <0..63>"); return; }
    uint32_t v = (uint32_t)strtoul(arg, NULL, 10);
    si4732_status_t rc = si4732_set_volume(radio, (uint8_t)v);
    cdc_printf("command vol seet rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);

    return;
  }

  if (!strcmp(cmd, "mute")) {
    char *arg = strtok(NULL, " \t\r\n");
    if (!arg) { cdc_write_ln("ERR: mute <0|1>"); return; }
    int m = atoi(arg);
    si4732_status_t rc = si4732_set_mute(radio, m != 0, m != 0);
    cdc_printf("command mute rc=%d last_status=0x%02X\n",(int)rc, radio->last_status);

    return;
  }

  cdc_write_ln("ERR: unknown command. try 'help'\n");
}

//*--- strip a string from a given character 

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

//*----------------------------------------------------------------------------
//*                    CDC Write and flush buffer function
//*----------------------------------------------------------------------------
void cdc_write(char *buf, uint16_t length) {

  tud_cdc_write(buf, length);
  tud_cdc_write_flush();
}


// Callback de TinyUSB: cambios de DTR/RTS
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void)itf;
  (void)rts;
  cdc_dtr = dtr;
}
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*                                           MAIN 
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

uint32_t blinkrate=BLINKSLOW;


int main(void) {

  //*--- NOTE: We do not rely on stdio over USB; we use TinyUSB CDC directly.
  //*--- this is the environment used by the ADX-ddsPIO projects and therefore
  //*--- integration with that project will be mitigated.

  stdio_init_all();

  //*--- Init USB stack

  tusb_init();
  //tud_init(BOARD_TUD_RHPORT);    //*--- You might actually need to use this when integrating later

  absolute_time_t t0 = get_absolute_time();
  while (!tud_mounted()) {
    tud_task();
    if (absolute_time_diff_us(t0, get_absolute_time()) > 3 * 1000 * 1000) {
        break;           // timeout 3 secs, do not hang the system
    }
  }

//*--- Initialize the board resources to be used by this program

  gpio_init(TXSW);
  gpio_set_dir(TXSW, GPIO_IN);
  gpio_pull_up(TXSW);

  gpio_init(TX);
  gpio_set_dir(TX, GPIO_OUT); //TX →　1, RX →　0 (for Driver switch)
  gpio_put(TX,0);             //Turn TX LED off, use it as a marker for debugging

  gpio_init(FT8);
  gpio_set_dir(FT8, GPIO_OUT); //TX →　1, RX →　0 (for Driver switch)
  gpio_put(FT8,0);             //Turn TX LED off, use it as a marker for debugging

  //*--- Debug construct to force the flow wait for the CDC stream to be active

  /*
  while(gpio_get(TXSW)) {tud_task();};     //Wait till the TX Switch is pressed (to be able to see the messages)
  gpio_put(TX,1);            //Turn TX LED on, ready to start
  while(!gpio_get(TXSW)) {tud_task();};   //Wait till the TX switch is released
  gpio_put(TX,0);
  */

  uint32_t t;
  gpio_put(TX,1); 

  blink=false;
  t=to_ms_since_boot(get_absolute_time());
  while (true) {
    tud_task();                 // mantiene USB vivo
    if (tud_cdc_connected() && cdc_dtr) break;  // host abrió el puerto
    sleep_ms(1);
    if (to_ms_since_boot(get_absolute_time())-t > 200) {
        t=to_ms_since_boot(get_absolute_time());
        blink=!blink;
        gpio_put(TX,blink);
        gpio_put(FT8,!blink);
 
      }
  }

  for (int i=0; i<100; i++) {
     tud_task();
     sleep_ms(1);
  }

  tud_task();
  cdc_printf("\nUSB Bus initialized properly and ready\n");
  for (int i=0;i<10;i++){tud_task();}
  gpio_put(TX,0); 
  gpio_put(FT8,0); 
  
  tud_task();
  //*--- Ensure reset is high

  gpio_put(RST_PIN,1);
  gpio_put(TX,1);

  //*--- Init radio chip
  
  si4732_t radio;
  si4732_status_t rc = si4732_init(&radio, I2C_PORT, SI4732_I2C_ADDR_DEFAULT, SDA_PIN, SCL_PIN, RST_PIN, 400000);
  cdc_printf("Si4732 init rc=%d present=%d last=0x%02X\n", (int)rc, (int)radio.present, radio.last_status);

if (rc == SI4732_OK) {
  rc = si4732_power_up_fm(&radio);
  cdc_printf("Si4732 power_up_fm rc=%d last=0x%02X\n", (int)rc, radio.last_status);

  if (rc == SI4732_OK) {
    (void)si4732_apply_region(&radio, SI4732_REGION_AR);
    (void)si4732_set_volume(&radio, 30);
  }
}

  //*--- Line buffer
  static char line[128];
  size_t idx = 0;

  //*--- Prepare for the main loop, the PIO clock is running

  bool keypress=false;
  t=to_ms_since_boot(get_absolute_time());
  
  static bool greeted = false;
  uint8_t c=0;


  //*---- Forever loop

  cdc_printf("Board initialization completed\n");

  while (true) {
    tud_task();

    //*--------------------------------------------------------------------------------
    //*--- This is a debug logic, detect if the TX switch has been pressed
    //*--- perform a different function each time it's pressed
    //*--------------------------------------------------------------------------------

    if (!gpio_get(TXSW)) {    

       //*--- Turn on the TX LED and flag the condition, also send a debug message

       gpio_put(TX,1);
       keypress=true;
       cdc_printf("loop() TX key pressed\n");

       //*--- Wait till the switch is released, execute the USB task while waiting
       while(!gpio_get(TXSW)) {
         tud_task();
       }
       //*--- Upon release turn off the TX LED

       gpio_put(TX,0);

    }
    //*--- just blink the TX led every second to note the waiting pattern

    if (to_ms_since_boot(get_absolute_time())-t > blinkrate) {
        t=to_ms_since_boot(get_absolute_time());
        blink=!blink;
        gpio_put(TX,blink);
    }

    //*--- If the switch has been pressed and released then advance the band
    //*--- just to test the optimization solution for several classic FT8 frequencies

    if (keypress) {
       keypress=false;
       cdc_printf("loop() TX key released\n");      
       sleep_ms(5);
       greeted=false;

      for(int n=0;n<10;n++) {tud_task();}

    }
    tud_task(); // TinyUSB device task

    if (tud_cdc_connected()) {

      if (!greeted) {
         greeted = true;
         cdc_printf("CDC Serial ready Ok(%d). Type 'help'.\n",(int)radio.present);

         if (!radio.present) {
            cdc_write_ln("WARN: SI4732 not detected on I2C. Driver disabled; CDC stays alive.\n");
         }
         idx++;
         switch(c) {
          case 0: sprintf(line,"help");break;
          case 1: sprintf(line,"mode am");break;
          case 2: sprintf(line,"mode fm");break;
          case 3: sprintf(line,"band fm");break;
          case 4: sprintf(line,"band mw");break;
          case 5: sprintf(line,"band 49m");break;
          case 6: sprintf(line,"band 40m");break;
          case 7: sprintf(line,"band 31m");break;
          case 8: sprintf(line,"tune 710");break;
          case 9: sprintf(line,"tune 10230");break;
          case 10: sprintf(line,"seek up");break;
          case 11: sprintf(line,"seek down");break;
          case 12: sprintf(line,"vol 0");break;
          case 13: sprintf(line,"vol 30");break;
          case 14: sprintf(line,"vol 63");break;
          case 15: sprintf(line,"mute 0");break;
          case 16: sprintf(line,"mute 1");break;
          case 17: sprintf(line,"region ar");break;
          case 18: sprintf(line,"loadpatch");break;

        }
        cdc_printf("loop() Executing automatic FSM state(%d) cmd(%s)\n",c,line);
        c++;
        if (c>18) {c=0;}
        unsigned int len=strlen(line);

        if (len<=128) {
           line[len]='\n';
           line[len+1]='\0';
        } else {
          cdc_printf("loop() Command line longer than allowed\n");
        }
        process_line(&radio,line);     
    }

      /* Disable processing of manual characters at this point

      while (tud_cdc_available()) {
        char c = (char)tud_cdc_read_char();
        if (c == '\r') continue;
        if (c == '\n') {
          line[idx] = 0;
          if (idx > 0) process_line(&radio, line);
          idx = 0;
        } else if (idx < sizeof(line) - 1) {
          line[idx++] = c;
        }
      }

      */

    } else {
      //*--- reset greet flag when disconnected

      static bool last = false;
      
      if (last) { /* just disconnected */ }
      last = false;
      idx = 0;
    }

    sleep_ms(1);
  }
}
