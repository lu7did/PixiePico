#pragma once

// Pico SDK / TinyUSB
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE)
#define CFG_TUSB_OS                OPT_OS_PICO

// Debug (0 = off)
#define CFG_TUSB_DEBUG             0

// Device descriptors
#define CFG_TUD_ENDPOINT0_SIZE     64

// Classes enabled
#define CFG_TUD_CDC                1
#define CFG_TUD_MSC                1
#define CFG_TUD_AUDIO              1

// CDC
#define CFG_TUD_CDC_RX_BUFSIZE     256
#define CFG_TUD_CDC_TX_BUFSIZE     256

// MSC
#define CFG_TUD_MSC_EP_BUFSIZE     512

// AUDIO: para que no tire los #error EP size
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT        1
#define CFG_TUD_AUDIO_FUNC_1_N_AS_OUT        1
#define CFG_TUD_AUDIO_FUNC_1_N_AS_IN         1
#define CFG_TUD_AUDIO_EP_SZ_OUT              192
#define CFG_TUD_AUDIO_EP_SZ_IN               192

#ifndef CFG_TUD_AUDIO_EP_IN_SZ_MAX
#define CFG_TUD_AUDIO_EP_IN_SZ_MAX           CFG_TUD_AUDIO_EP_SZ_IN
#endif
#ifndef CFG_TUD_AUDIO_EP_OUT_SZ_MAX
#define CFG_TUD_AUDIO_EP_OUT_SZ_MAX          CFG_TUD_AUDIO_EP_SZ_OUT
#endif

#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ     255
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN   TUD_AUDIO_MIC_ONE_CH_DESC_LEN

