/*
 * CDC-only diagnostic implementation.
 *
 * This keeps main.c unchanged while the TinyUSB Audio class and all of its
 * callbacks are absent from the firmware.  Enable the real implementation
 * with -DPIXIE_ENABLE_USB_AUDIO=ON at CMake configure time.
 */
#include "usb_audio.h"

int USB_Audio_read(int16_t *audio_read_data)
{
    (void)audio_read_data;
    return 0;
}

void USB_Audio_write(int16_t *audio_write_data, int16_t data_number)
{
    (void)audio_write_data;
    (void)data_number;
}

void led_blinking_task(void)
{
}
