#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "tusb.h"

// Audio: OUT estéreo 48k/16b, IN mono 48k/16b
#define SAMPLE_RATE_HZ     48000u
#define BYTES_PER_SAMPLE   2u
#define SPK_CHANNELS       2u
#define MIC_CHANNELS       1u

#define FRAMES_PER_MS      (SAMPLE_RATE_HZ / 1000u) // 48
#define SPK_BYTES_PER_MS   (FRAMES_PER_MS * SPK_CHANNELS * BYTES_PER_SAMPLE) // 192
#define MIC_BYTES_PER_MS   (FRAMES_PER_MS * MIC_CHANNELS * BYTES_PER_SAMPLE) // 96

// Buffers
static int32_t spk_buf[SPK_BYTES_PER_MS / 4];
static int32_t mic_buf[SPK_BYTES_PER_MS / 4];
static volatile int spk_data_size = 0;

// ---------------- Medición de frecuencia ----------------
#define FREQ_WINDOW_MS     100u
#define FREQ_WIN_SAMPLES   ((SAMPLE_RATE_HZ * FREQ_WINDOW_MS) / 1000u) // 4800

#define ZC_THRESH          800
#define RMS_MIN            300

typedef struct {
  uint32_t sample_count;
  uint32_t zc_count;
  uint64_t sum_sq;
  bool     armed_neg;
} freq_meter_t;

static freq_meter_t g_fm = {0};

static inline void freq_meter_reset(freq_meter_t* fm) {
  fm->sample_count = 0;
  fm->zc_count = 0;
  fm->sum_sq = 0;
}

static inline void freq_meter_feed(freq_meter_t* fm, int16_t x) {
  int32_t xi = x;
  fm->sum_sq += (uint64_t)(xi * xi);

  if (!fm->armed_neg) {
    if (x < -ZC_THRESH) fm->armed_neg = true;
  } else {
    if (x > ZC_THRESH) {
      fm->zc_count++;
      fm->armed_neg = false;
    }
  }

  fm->sample_count++;
}

static bool freq_meter_compute(freq_meter_t* fm, float* out_hz, float* out_rms) {
  if (!fm->sample_count) return false;

  double mean_sq = (double)fm->sum_sq / (double)fm->sample_count;
  double rms = sqrt(mean_sq);
  *out_rms = (float)rms;

  if (rms < RMS_MIN) return false;

  double hz = ((double)fm->zc_count * (double)SAMPLE_RATE_HZ) / (double)fm->sample_count;
  if (hz < 10.0 || hz > 20000.0) return false;

  *out_hz = (float)hz;
  return true;
}

static void audio_task(void) {
  int n = spk_data_size;
  if (!n) return;

  int16_t* src = (int16_t*) spk_buf;
  int16_t* limit = (int16_t*) ((uint8_t*)spk_buf + n);
  int16_t* dst = (int16_t*) mic_buf;

  while (src < limit) {
    int32_t left  = *src++;
    int32_t right = *src++;

    int16_t mono = (int16_t)((left >> 1) + (right >> 1));
    *dst++ = mono;

    freq_meter_feed(&g_fm, mono);

    if (g_fm.sample_count >= FREQ_WIN_SAMPLES) {
      float hz = 0.0f, rms = 0.0f;
      if (freq_meter_compute(&g_fm, &hz, &rms)) {
        printf("Freq: %.1f Hz | RMS: %.0f | ZC: %lu | N: %lu\n",
               hz, rms,
               (unsigned long)g_fm.zc_count,
               (unsigned long)g_fm.sample_count);
      }
      freq_meter_reset(&g_fm);
    }
  }

  // Enviar al host por IN (mic). En mono: 96 bytes por ms típicamente.
  // OJO: si tu host pide otro tamaño, conviene usar tud_audio_write() con el tamaño “real” requerido.
  (void) tud_audio_write((uint8_t*) mic_buf, (uint16_t)MIC_BYTES_PER_MS);

  spk_data_size = 0;
}

// ---- Callbacks TinyUSB Audio ----
// Si tu TinyUSB usa otros nombres de callback, lo ajustamos.
bool tud_audio_rx_done_pre_read_cb(uint8_t rhport,
                                   uint16_t n_bytes_received,
                                   uint8_t func_id,
                                   uint8_t ep_out,
                                   uint8_t cur_alt_setting)
{
  (void)rhport; (void)func_id; (void)ep_out; (void)cur_alt_setting;

  if (n_bytes_received > sizeof(spk_buf)) {
    (void) tud_audio_read(spk_buf, (uint16_t)sizeof(spk_buf));
    spk_data_size = 0;
    return true;
  }

  spk_data_size = (int) tud_audio_read(spk_buf, n_bytes_received);
  return true;
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting) {
  (void)rhport; (void)itf; (void)ep_in; (void)cur_alt_setting;
  return true;
}

int main(void) {
  stdio_init_all();  // UART (porque stdio_usb está deshabilitado en CMake)

  // Inicialización TinyUSB sin BOARD_TUD_RHPORT
  tusb_init();

#ifdef PICO_DEFAULT_LED_PIN
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif

  uint32_t last_blink = to_ms_since_boot(get_absolute_time());
  bool led = false;

  while (true) {
    tud_task();
    audio_task();

#ifdef PICO_DEFAULT_LED_PIN
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_blink > 500) {
      last_blink = now;
      led = !led;
      gpio_put(PICO_DEFAULT_LED_PIN, led);
    }
#endif
  }
}

