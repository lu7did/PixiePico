#include "usb_descriptors.h"
#include "tusb.h"
#include <string.h>

#define USB_VID   0xCafe
#define USB_PID   0x4011
#define USB_BCD   0x0100

// --------------------------------------------------------------------
// Device descriptor
// --------------------------------------------------------------------
const tusb_desc_device_t desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,

  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,

  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

  .idVendor           = USB_VID,
  .idProduct          = USB_PID,
  .bcdDevice          = USB_BCD,

  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,

  .bNumConfigurations  = 0x01
};

uint8_t const* tud_descriptor_device_cb(void) {
  return (uint8_t const*) &desc_device;
}

//*--- Configuration descriptor

#ifndef TUD_AUDIO_MIC_ONE_CH_DESC_LEN
// Si tu TinyUSB no define el _DESC_LEN, definí un fallback.
// OJO: esto puede variar entre versiones. Si te falla el tamaño total,
// te digo cómo obtener el valor real con un compile-time assert.
#define TUD_AUDIO_MIC_ONE_CH_DESC_LEN  (0)  // fallback seguro para compilar; ajustar si tu versión lo define distinto
#endif

#ifndef CFG_TUD_AUDIO_FUNC_1_DESC_LEN
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN TUD_AUDIO_MIC_ONE_CH_DESC_LEN
#endif

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN + TUD_AUDIO_MIC_ONE_CH_DESC_LEN)
#define EPNUM_AUDIO_CTRL  0x00
uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4,                         //* CDC
                     EPNUM_CDC_NOTIF, 8,
                     EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5,                         //* MSC
                     EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),

  TUD_AUDIO_MIC_ONE_CH_DESCRIPTOR(                           //* AUDIO
  ITF_NUM_AUDIO_CONTROL, // _itfnum
  0x06,                  // _stridx
  2,                     // _nBytesPerSample (ej: 2 bytes = 16-bit)
  16,                    // _nBitsUsedPerSample
  EPNUM_AUDIO_IN,        // _epin (ej 0x84)
  CFG_TUD_AUDIO_EP_SZ_IN // _epsize
),

};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void) index;
  return desc_configuration;
}

//*--- Descriptors

static const char* string_desc_arr[] = {
  (const char[]){ 0x09, 0x04 }, // 0: English (US)
  "LU7DZ",                      // 1: Manufacturer
  "ADX-ddsPIO Composite",       // 2: Product
  "0001",                       // 3: Serial
  "ADX CDC",                    // 4
  "ADX MSC",                    // 5
  "ADX Audio",                  // 6
};

static uint16_t _desc_str[32];

//*--- Callback de inicialización

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void) langid;
  uint8_t chr_count;

  if (index == 0) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    uint8_t const max = (uint8_t)(sizeof(string_desc_arr) / sizeof(string_desc_arr[0]));
    if (index >= max) return NULL;

    const char* str = string_desc_arr[index];
    chr_count = 0;
    while (str[chr_count] && chr_count < 31) {
      _desc_str[1 + chr_count] = (uint16_t) str[chr_count];
      chr_count++;
    }
  }

  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}
