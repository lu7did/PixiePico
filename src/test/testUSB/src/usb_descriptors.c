#include <string.h>

#include "tusb.h"
#include "class/audio/audio.h"

#include "usb_descriptors.h"

#define _PID_MAP(itf, n)  ((CFG_TUD_##itf) << (n))
#define USB_PID           (0x4000 | _PID_MAP(AUDIO, 4))

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCafe,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

uint8_t const* tud_descriptor_device_cb(void) {
  return (uint8_t const*) &desc_device;
}

#define EPNUM_AUDIO_OUT   0x01
#define EPNUM_AUDIO_IN    0x81

#define UAC2_ENTITY_SPK_INPUT_TERMINAL   0x01
#define UAC2_ENTITY_SPK_FEATURE_UNIT     0x02
#define UAC2_ENTITY_SPK_OUTPUT_TERMINAL  0x03
#define UAC2_ENTITY_CLOCK                0x04
#define UAC2_ENTITY_MIC_INPUT_TERMINAL   0x11
#define UAC2_ENTITY_MIC_OUTPUT_TERMINAL  0x13

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + CFG_TUD_AUDIO_FUNC_1_DESC_LEN)

#define AC_CS_TOTAL_LEN ( \
  TUD_AUDIO_DESC_CLK_SRC_LEN + \
  TUD_AUDIO_DESC_INPUT_TERM_LEN + \
  TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN + \
  TUD_AUDIO_DESC_OUTPUT_TERM_LEN + \
  TUD_AUDIO_DESC_INPUT_TERM_LEN + \
  TUD_AUDIO_DESC_OUTPUT_TERM_LEN \
)

uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // IAD: AC + AS(speaker) + AS(mic)
  TUD_AUDIO_DESC_IAD(ITF_NUM_AUDIO_CONTROL, 3, 0x00),

  TUD_AUDIO_DESC_STD_AC(ITF_NUM_AUDIO_CONTROL, 0x00, 0x00),

  TUD_AUDIO_DESC_CS_AC(0x0200, AUDIO_FUNC_HEADSET, AC_CS_TOTAL_LEN, AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS),

  TUD_AUDIO_DESC_CLK_SRC(
    UAC2_ENTITY_CLOCK,
    AUDIO_CLOCK_SOURCE_ATT_INT_PRO_CLK,
    (AUDIO_CTRL_RW << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS),
    0x00,
    0x00
  ),

  // Speaker: USB streaming -> headphones
  TUD_AUDIO_DESC_INPUT_TERM(
    UAC2_ENTITY_SPK_INPUT_TERMINAL,
    AUDIO_TERM_TYPE_USB_STREAMING,
    0x00,
    UAC2_ENTITY_CLOCK,
    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX,
    AUDIO_CHANNEL_CONFIG_NON_PREDEFINED,
    0x00, 0x00, 0x00
  ),

  TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL(
    UAC2_ENTITY_SPK_FEATURE_UNIT,
    UAC2_ENTITY_SPK_INPUT_TERMINAL,
    (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS) | (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS),
    (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS) | (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS),
    (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS) | (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS),
    0x00
  ),

  TUD_AUDIO_DESC_OUTPUT_TERM(
    UAC2_ENTITY_SPK_OUTPUT_TERMINAL,
    AUDIO_TERM_TYPE_OUT_HEADPHONES,
    0x00,
    UAC2_ENTITY_SPK_FEATURE_UNIT,
    UAC2_ENTITY_CLOCK,
    0x0000,
    0x00
  ),

  // Mic: generic mic -> USB streaming
  TUD_AUDIO_DESC_INPUT_TERM(
    UAC2_ENTITY_MIC_INPUT_TERMINAL,
    AUDIO_TERM_TYPE_IN_GENERIC_MIC,
    0x00,
    UAC2_ENTITY_CLOCK,
    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX,
    AUDIO_CHANNEL_CONFIG_NON_PREDEFINED,
    0x00, 0x00, 0x00
  ),

  TUD_AUDIO_DESC_OUTPUT_TERM(
    UAC2_ENTITY_MIC_OUTPUT_TERMINAL,
    AUDIO_TERM_TYPE_USB_STREAMING,
    0x00,
    UAC2_ENTITY_MIC_INPUT_TERMINAL,
    UAC2_ENTITY_CLOCK,
    0x0000,
    0x00
  ),

  // Speaker AS alt0 + alt1
  TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING_SPK, 0x00, 0x00, 0x05),
  TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING_SPK, 0x01, 0x01, 0x05),

  TUD_AUDIO_DESC_CS_AS_INT(
    UAC2_ENTITY_SPK_INPUT_TERMINAL,
    AUDIO_CTRL_NONE,
    AUDIO_FORMAT_TYPE_I,
    AUDIO_DATA_FORMAT_TYPE_I_PCM,
    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX,
    AUDIO_CHANNEL_CONFIG_NON_PREDEFINED,
    0x00
  ),

  TUD_AUDIO_DESC_TYPE_I_FORMAT(2, CFG_TUD_AUDIO_FUNC_1_SAMPLE_BITS),

  TUD_AUDIO_DESC_STD_AS_ISO_EP(
    EPNUM_AUDIO_OUT,
    (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ADAPTIVE | TUSB_ISO_EP_ATT_DATA),
    CFG_TUD_AUDIO_EPSIZE_OUT,
    0x01
  ),

  TUD_AUDIO_DESC_CS_AS_ISO_EP(
    AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK,
    AUDIO_CTRL_NONE,
    AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC,
    0x0001
  ),

  // Mic AS alt0 + alt1
  TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING_MIC, 0x00, 0x00, 0x04),
  TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING_MIC, 0x01, 0x01, 0x04),

  TUD_AUDIO_DESC_CS_AS_INT(
    UAC2_ENTITY_MIC_OUTPUT_TERMINAL,
    AUDIO_CTRL_NONE,
    AUDIO_FORMAT_TYPE_I,
    AUDIO_DATA_FORMAT_TYPE_I_PCM,
    CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX,
    AUDIO_CHANNEL_CONFIG_NON_PREDEFINED,
    0x00
  ),

  TUD_AUDIO_DESC_TYPE_I_FORMAT(2, CFG_TUD_AUDIO_FUNC_1_SAMPLE_BITS),

  TUD_AUDIO_DESC_STD_AS_ISO_EP(
    EPNUM_AUDIO_IN,
    (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS | TUSB_ISO_EP_ATT_DATA),
    CFG_TUD_AUDIO_EPSIZE_IN,
    0x01
  ),

  TUD_AUDIO_DESC_CS_AS_ISO_EP(
    AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK,
    AUDIO_CTRL_NONE,
    AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED,
    0x0000
  ),
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void) index;
  return desc_configuration;
}

static char const* string_desc_arr[] = {
  (const char[]) { 0x09, 0x04 },
  "RP2040",
  "Pico UAC2 Loopback+Freq",
  "000001",
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void) langid;

  uint8_t chr_count;

  if (index == 0) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= (sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) return NULL;

    const char* str = string_desc_arr[index];
    chr_count = (uint8_t) strlen(str);
    if (chr_count > 31) chr_count = 31;

    for (uint8_t i = 0; i < chr_count; i++) _desc_str[1 + i] = (uint16_t) str[i];
  }

  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}

