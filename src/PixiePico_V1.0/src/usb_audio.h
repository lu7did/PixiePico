/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2023  Hitoshi Kawaji <je1rav@gmail.com>
 *  Heatder for usb_audio.c in which USB_Audio_read() and USB_Audio_write() 
 *  functions are added to "uac2_heatset" program in the exapmles of TinyUSB 
 *  to comunicate with PC for Radio tranceiver application.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _USB_AUDIO_H_
#define _USB_AUDIO_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include "bsp/board.h"
#include "bsp/rp2040/board.h"
#include "pico/time.h"


#include "tusb.h"
#include "usb_descriptors.h"


//void audio_task(void);
void led_blinking_task(void);

int USB_Audio_read(int16_t *);
void USB_Audio_write(int16_t *, int16_t);

#endif   // _USB_AUDIO_H_