#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// NO incluyas tusb.h aquí.
// Este archivo sólo debe contener #define CFG_*
// Incluir tusb.h aquí puede causar redefiniciones/orden incorrecto.

// Para macros TUD_AUDIO_DESC_*_LEN:
#include "class/audio/audio.h"

// -------------------- TinyUSB core --------------------

#ifndef CFG_TUSB_MCU
  #define CFG_TUSB_MCU OPT_MCU_RP2040
#endif

#define CFG_TUSB_DEBUG          0

#define CFG_TUD_ENABLED         1
#define CFG_TUD_MAX_SPEED       OPT_MODE_FULL_SPEED
#define CFG_TUD_ENDPOINT0_SIZE  64

// -------------------- USB AUDIO (UAC2) --------------------

#define CFG_TUD_AUDIO 1

#define CFG_TUD_AUDIO_ENABLE_EP_OUT 1
#define CFG_TUD_AUDIO_ENABLE_EP_IN  1

#define CFG_TUD_AUDIO_ENABLE_DECODING 0
#define CFG_TUD_AUDIO_ENABLE_ENCODING 0

// Formato
#define CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE      48000
#define CFG_TUD_AUDIO_FUNC_1_SAMPLE_BITS         16
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX        2
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX        1

#define CFG_TUD_AUDIO_EPSIZE_OUT 192
#define CFG_TUD_AUDIO_EPSIZE_IN  192

// Driver required
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT 2

// ⚠️ Debe entrar en uint8_t (<=255). 256 desborda a 0.
// 64/128 suele ser suficiente.
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ 128

#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX CFG_TUD_AUDIO_EPSIZE_OUT
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX  CFG_TUD_AUDIO_EPSIZE_IN

#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ 2048
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ  2048

// Longitud de descriptor función audio (incluye IAD)
#define PICO_UAC2_FUNC_DESC_LEN ( \
  TUD_AUDIO_DESC_IAD_LEN + \
  TUD_AUDIO_DESC_STD_AC_LEN + \
  TUD_AUDIO_DESC_CS_AC_LEN + \
  TUD_AUDIO_DESC_CLK_SRC_LEN + \
  /* Speaker */ \
  TUD_AUDIO_DESC_INPUT_TERM_LEN + \
  TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN + \
  TUD_AUDIO_DESC_OUTPUT_TERM_LEN + \
  /* Mic */ \
  TUD_AUDIO_DESC_INPUT_TERM_LEN + \
  TUD_AUDIO_DESC_OUTPUT_TERM_LEN + \
  /* Speaker AS alt0 + alt1 */ \
  TUD_AUDIO_DESC_STD_AS_INT_LEN + \
  TUD_AUDIO_DESC_STD_AS_INT_LEN + \
  TUD_AUDIO_DESC_CS_AS_INT_LEN + \
  TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN + \
  TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN + \
  TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN + \
  /* Mic AS alt0 + alt1 */ \
  TUD_AUDIO_DESC_STD_AS_INT_LEN + \
  TUD_AUDIO_DESC_STD_AS_INT_LEN + \
  TUD_AUDIO_DESC_CS_AS_INT_LEN + \
  TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN + \
  TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN + \
  TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN \
)

#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN PICO_UAC2_FUNC_DESC_LEN

#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_FORMAT_CORRECTION 1
#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_EP 0

#ifdef __cplusplus
}
#endif

#endif // _TUSB_CONFIG_H_

