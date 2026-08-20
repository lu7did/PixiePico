/*
 * =======================================================================================
 * testFS
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
 * Implementation of small USB Filesystem using USB MSC, this stores a JSON
 * file called CONFIG.TXT (with random data in this demo) to be used as a
 * configuration tool
 *----------------------------------------------------------------------------
 */

char hi[512];


//*--- Defines
#define PICO_DEFAULT_LED_PIN 25
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
            tud_task();                           \
        }                                               \
    } while (0)



//*--- Imcludes

#include <stdio.h>
#include <stdlib.h>   
#include "pico/stdlib.h"
#include "tusb.h"
#include "fs.h"
#include <string.h>


//*--- Global areas

bool blink=false;
char region[16] = {0};
char vco_str[32] = {0};
char bfo_str[32] = {0};
char note[32] = {0};
char json[512];
size_t n = 0;
char buf[256];

//*-- Callback to manage to wait till serial monitor is available
static volatile bool cdc_dtr = false;

//*--- Prototypes
void cdc_write(char *buf, uint16_t length);


//*----------------------------------------------------------------------------
//*                         USB CDC functions
//*----------------------------------------------------------------------------
//*--- Write a buffer
void cdc_write(char *buf, uint16_t length)
{
  tud_cdc_write(buf, length);
  tud_cdc_write_flush();
}

//*--- Write a string
static void cdc_write_str(const char *s) {
  if (!tud_cdc_connected()) return;
  tud_cdc_write_str(s);
  tud_cdc_write_flush();
}

//*--- Callback to receive control line changes (DTR/RTS)
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void)itf;
  (void)rts;
  cdc_dtr = dtr;
}

//*--- Minimum Audio callbacks (not used, just for future integration)

bool tud_audio_set_req_cb(uint8_t rhport, tusb_control_request_t const* request, uint8_t* buffer, uint16_t bufsize) {
  (void)rhport; (void)request; (void)buffer; (void)bufsize;
  return false;
}
bool tud_audio_get_req_cb(uint8_t rhport, tusb_control_request_t const* request, uint8_t* buffer, uint16_t bufsize) {
  (void)rhport; (void)request; (void)buffer; (void)bufsize;
  return false;
}
bool tud_audio_rx_done_pre_read_cb(uint8_t func_id, uint8_t ep_out, uint16_t cur_alt_setting) {
  (void)func_id; (void)ep_out; (void)cur_alt_setting;
  return true;
}
bool tud_audio_tx_done_post_load_cb(uint8_t func_id, uint8_t ep_in, uint16_t cur_alt_setting) {
  (void)func_id; (void)ep_in; (void)cur_alt_setting;
  return true;
}

//*--- This is to force characters to be drained from the TUD pipe

void tud_pump() {
     for (int i=0; i<100; i++) {
     tud_task();
     sleep_ms(1);
  }

}

//*--- Main

int main(void) {

  //*--- Init I/O
  stdio_init_all();
  sleep_ms(1200);
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  //*--- Init the MSC device  

  bool FSinit = fs_init_and_mount();
  
  //*--- Read a buffer and perform different operations to check availability

  bool FSread = fs_read_text(buf, sizeof(buf), &n);
  int fr_mount = fs_last_fr_mount();
  int fr_mkfs  = fs_last_fr_mkfs();
  int fr_open  = fs_last_fr_open();
  int fr_read  = fs_last_fr_read();
  sleep_ms(1000);

  //*--- Read the entire CONFIG.TXT configuration file, can not send over serial yet
  n = 0;
  bool ok_read = fs_read_text(json, sizeof(json), &n);

  //*--- Extract different keys from the JSON file

  bool ok_region = ok_read && fs_get_kv(json, "region", region, sizeof(region));
  bool ok_vco = ok_read && fs_get_kv(json, "vco_hz", vco_str, sizeof(vco_str));
  bool ok_bfo = ok_read && fs_get_kv(json, "bfo_hz", bfo_str, sizeof(bfo_str));
  bool ok_note = ok_read && fs_get_kv(json, "note", note, sizeof(note));
  uint32_t vco_hz = ok_vco ? (uint32_t)strtoul(vco_str, NULL, 10) : 120000000u;
  uint32_t bfo_hz = ok_bfo ? (uint32_t)strtoul(bfo_str, NULL, 10) : 455000u;

  //*--- Unmount USB MSC, from now on the FileSysten won't be available 2) Desmontar FatFs antes de exponer MSC (bien)
  fs_unmount();

  //*--- Now starts the Audio and CDC portions
  tusb_init();

  absolute_time_t t0 = get_absolute_time();
  while (!tud_mounted()) {
    tud_task();
    if (absolute_time_diff_us(t0, get_absolute_time()) > 3 * 1000 * 1000) {
       break;           // timeout 3 secs, do not hang the system
    }
  }

  //*--- Hold any writting till the serial monitor is activated, blink fast meanwhile

  blink=false;
  gpio_put(PICO_DEFAULT_LED_PIN,blink);
  absolute_time_t t=to_ms_since_boot(get_absolute_time());
  while (true) {
    tud_task();                                  // USB keepalive
    if (tud_cdc_connected() && cdc_dtr) break;   // USB open port
    sleep_ms(1);
    if (to_ms_since_boot(get_absolute_time())-t > 200) {
        t=to_ms_since_boot(get_absolute_time());
        blink=!blink;
        gpio_put(PICO_DEFAULT_LED_PIN,blink);
      }
  }
  sleep_ms(1000);
  tud_pump();

  //*--- Now the Serial monitor is available, so print all the witheld info
  cdc_printf("ADX-ddsPIO Composite: CDC+MSC+Audio with FatFs-on-Flash\n"); 
  tud_pump();

  cdc_printf("Access Statistics\nFS init=%d read=%d n=%u fr_mount=%d fr_mkfs=%d fr_open=%d fr_read=%d\n",
           FSinit, FSread, (unsigned)n, fr_mount, fr_mkfs, fr_open, fr_read);
  tud_pump();

  if (FSread) {
     cdc_printf("JSON content:\n%s\n", buf); 
  }
  tud_pump();

  cdc_printf("Values by key\nread=%d n=%u\nregion_ok=%d region=[%s]\nvco_ok=%d vco=[%s]\nvco_hz=%lu\nnote_ok=%d note=[%s]\n",
           ok_read, (unsigned)n, ok_region, region, ok_vco, vco_str, (unsigned long)vco_hz,ok_note,note);
  tud_pump();


  //*--- Once everything has been printed just enter an infinite loop blinking slow

  while (true) {
    tud_task();
    if (to_ms_since_boot(get_absolute_time())-t > 1000) {
        t=to_ms_since_boot(get_absolute_time());
        blink=!blink;
        gpio_put(PICO_DEFAULT_LED_PIN,blink);
      }
}
}
