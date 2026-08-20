#pragma once

// TinyUSB configuration for RP2040 (CDC only for now; audio reserved for future integration)

#ifdef __cplusplus
extern "C" {
#endif

// USB root hub port
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE)
#define CFG_TUSB_RHPORT1_MODE (0)

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif

// Controls
#define CFG_TUD_ENDPOINT0_SIZE 64

// Class drivers
#define CFG_TUD_CDC       1
#define CFG_TUD_MSC       0
#define CFG_TUD_HID       0
#define CFG_TUD_MIDI      0
#define CFG_TUD_VENDOR    0
#define CFG_TUD_AUDIO     0  // reservado para futura integración USB Audio

// CDC
#define CFG_TUD_CDC_RX_BUFSIZE   512
#define CFG_TUD_CDC_TX_BUFSIZE   512

#ifdef __cplusplus
}
#endif
