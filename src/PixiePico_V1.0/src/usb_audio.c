/* 
 * The MIT License (MIT)
 *
 * usb_audio.c 
 * 
 * Copyright (c) 2023  Hitoshi Kawaji <je1rav@gmail.com>
 *  Added USB_Audio_read() and USB_Audio_write() functions to "uac2_heatset"
 *  program in the exapmles of TinyUSB to comunicate with PC for Radio 
 *  tranceiver application.
 * 
 * Copyright (c) 2020 Jerzy Kasenberg
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
 
#include "usb_audio.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif


//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTOTYPES for Uac2 Headset
//--------------------------------------------------------------------+

// List of supported sample rates
#if defined(__RX__)
  const uint32_t sample_rates[] = {44100, 48000};
#else
  const uint32_t sample_rates[] = {44100, 48000, 88200, 96000};
#endif

uint32_t current_sample_rate  = 48000;

#define N_SAMPLE_RATES  TU_ARRAY_SIZE(sample_rates)

/* Blink pattern
 * - 25 ms   : streaming data
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
  BLINK_STREAMING = 100,
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

enum
{
  VOLUME_CTRL_0_DB = 0,
  VOLUME_CTRL_10_DB = 2560,
  VOLUME_CTRL_20_DB = 5120,
  VOLUME_CTRL_30_DB = 7680,
  VOLUME_CTRL_40_DB = 10240,
  VOLUME_CTRL_50_DB = 12800,
  VOLUME_CTRL_60_DB = 15360,
  VOLUME_CTRL_70_DB = 17920,
  VOLUME_CTRL_80_DB = 20480,
  VOLUME_CTRL_90_DB = 23040,
  VOLUME_CTRL_100_DB = 25600,
  VOLUME_CTRL_SILENCE = 0x8000,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

// Audio controls
// Current states
int8_t mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];       // +1 for master channel 0
int16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];    // +1 for master channel 0

// Buffer for microphone data
int32_t mic_buf[CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ / 4];
// Buffer for speaker data
int32_t spk_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4];
// Speaker data size received in the last frame
int spk_data_size;
// Resolution per format
const uint8_t resolutions_per_format[CFG_TUD_AUDIO_FUNC_1_N_FORMATS] = {CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX,
                                                                        CFG_TUD_AUDIO_FUNC_1_FORMAT_2_RESOLUTION_RX};
// Current resolution, update on format change
uint8_t current_resolution;


//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Helper for clock get requests
static bool tud_audio_clock_get_request(uint8_t rhport, audio_control_request_t const *request)
{
  TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);

  if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ)
  {
    if (request->bRequest == AUDIO_CS_REQ_CUR)
    {

  //#=============================================================================================
      #ifdef REFACTOR
      TU_LOG1("Clock get current freq %lu\r\n", current_sample_rate);
      #endif //REFACTOR
  //#=============================================================================================

      audio_control_cur_4_t curf = { (int32_t) tu_htole32(current_sample_rate) };
      return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &curf, sizeof(curf));
    }
    else if (request->bRequest == AUDIO_CS_REQ_RANGE)
    {
      audio_control_range_4_n_t(N_SAMPLE_RATES) rangef =
      {
        .wNumSubRanges = tu_htole16(N_SAMPLE_RATES)
      };


  //#=============================================================================================
      #ifdef REFACTOR
      TU_LOG1("Clock get %d freq ranges\r\n", N_SAMPLE_RATES);
      #endif //REFACTOR
  //#=============================================================================================
      
      for(uint8_t i = 0; i < N_SAMPLE_RATES; i++)
      {
        rangef.subrange[i].bMin = (int32_t) sample_rates[i];
        rangef.subrange[i].bMax = (int32_t) sample_rates[i];
        rangef.subrange[i].bRes = 0;
  
        //#=============================================================================================
        #ifdef REFACTOR
        TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax, (int)rangef.subrange[i].bRes);
        #endif //REFACTOR
        //#=============================================================================================
  

      }
      
      return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &rangef, sizeof(rangef));
    }
  }
  else if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
           request->bRequest == AUDIO_CS_REQ_CUR)
  {
    audio_control_cur_1_t cur_valid = { .bCur = 1 };

    //#=============================================================================================
    #ifdef REFACTOR
    TU_LOG1("Clock get is valid %u\r\n", cur_valid.bCur);
    #endif //REFACTOR
    //#=============================================================================================
  
    return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_valid, sizeof(cur_valid));
  }
//#=============================================================================================
  #ifdef REFACTOR
  TU_LOG1("Clock get request not supported, entity = %u, selector = %u, request = %u\r\n",
          request->bEntityID, request->bControlSelector, request->bRequest);
  #endif //REFACTOR
 //#=============================================================================================
          
  return false;
}

// Helper for clock set requests
static bool tud_audio_clock_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
{
  (void)rhport;

  TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);
  TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);

  if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ)
  {
    TU_VERIFY(request->wLength == sizeof(audio_control_cur_4_t));

    current_sample_rate = (uint32_t) ((audio_control_cur_4_t const *)buf)->bCur;

//#=============================================================================================
    #ifdef REFACTOR
    TU_LOG1("Clock set current freq: %ld\r\n", current_sample_rate);
    #endif //REFACTOR
//#=============================================================================================
  

    return true;
  }
  else
  {

//#=============================================================================================
    #ifdef REFACTOR    
    TU_LOG1("Clock set request not supported, entity = %u, selector = %u, request = %u\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);
    #endif //REFACTOR
//#=============================================================================================
          
    return false;
  }
}

// Helper for feature unit get requests
static bool tud_audio_feature_unit_get_request(uint8_t rhport, audio_control_request_t const *request)
{
  TU_ASSERT(request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT);

  if (request->bControlSelector == AUDIO_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR)
  {
    audio_control_cur_1_t mute1 = { .bCur = mute[request->bChannelNumber] };
  

    //#=============================================================================================
      #ifdef REFACTOR
    TU_LOG1("Get channel %u mute %d\r\n", request->bChannelNumber, mute1.bCur);
    #endif //REFACTOR
    //#=============================================================================================
  

    return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &mute1, sizeof(mute1));
  }
  else if (UAC2_ENTITY_SPK_FEATURE_UNIT && request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
  {
    if (request->bRequest == AUDIO_CS_REQ_RANGE)
    {
      audio_control_range_2_n_t(1) range_vol = {
        .wNumSubRanges = tu_htole16(1),
        .subrange[0] = { .bMin = tu_htole16(-VOLUME_CTRL_50_DB), tu_htole16(VOLUME_CTRL_0_DB), tu_htole16(256) }
      };
  
  
      //#=============================================================================================
      #ifdef REFACTOR
      TU_LOG1("Get channel %u volume range (%d, %d, %u) dB\r\n", request->bChannelNumber,
              range_vol.subrange[0].bMin / 256, range_vol.subrange[0].bMax / 256, range_vol.subrange[0].bRes / 256);
      #endif //REFACTOR
      //#=============================================================================================
  
      return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &range_vol, sizeof(range_vol));
    }
    else if (request->bRequest == AUDIO_CS_REQ_CUR)
    {
      audio_control_cur_2_t cur_vol = { .bCur = tu_htole16(volume[request->bChannelNumber]) };
  
      //#=============================================================================================
      #ifdef REFACTOR
      TU_LOG1("Get channel %u volume %d dB\r\n", request->bChannelNumber, cur_vol.bCur / 256);
      #endif //REFACTOR
      //#=============================================================================================
  

      return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_vol, sizeof(cur_vol));
    }
  }

  //#=============================================================================================
    #ifdef REFACTOR
  TU_LOG1("Feature unit get request not supported, entity = %u, selector = %u, request = %u\r\n",
          request->bEntityID, request->bControlSelector, request->bRequest);
  #endif //REFACTOR
  //#=============================================================================================
  
  return false;
}

// Helper for feature unit set requests
static bool tud_audio_feature_unit_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
{
  (void)rhport;

  TU_ASSERT(request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT);
  TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);

  if (request->bControlSelector == AUDIO_FU_CTRL_MUTE)
  {
    TU_VERIFY(request->wLength == sizeof(audio_control_cur_1_t));

    mute[request->bChannelNumber] = ((audio_control_cur_1_t const *)buf)->bCur;


    //#============================================================================================= 
    #ifdef REFACTOR
    TU_LOG1("Set channel %d Mute: %d\r\n", request->bChannelNumber, mute[request->bChannelNumber]);
    #endif //REFACTOR
    //#=============================================================================================
  

    return true;
  }
  else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
  {
    TU_VERIFY(request->wLength == sizeof(audio_control_cur_2_t));

    volume[request->bChannelNumber] = ((audio_control_cur_2_t const *)buf)->bCur;

    //#=============================================================================================
    #ifdef REFACTOR
    TU_LOG1("Set channel %d volume: %d dB\r\n", request->bChannelNumber, volume[request->bChannelNumber] / 256);
    #endif //REFACTOR
    //#=============================================================================================
  


    return true;
  }
  else
  {

    //#=============================================================================================
    #ifdef REFACTOR
    TU_LOG1("Feature unit set request not supported, entity = %u, selector = %u, request = %u\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);
    #endif //REFACTOR
    //#=============================================================================================
  
    
    return false;
  }
}

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+
// This part is from "usb2_heatset" in TinyUSB examples(device)

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
  audio_control_request_t const *request = (audio_control_request_t const *)p_request;

  if (request->bEntityID == UAC2_ENTITY_CLOCK)
    return tud_audio_clock_get_request(rhport, request);
  if (request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT)
    return tud_audio_feature_unit_get_request(rhport, request);
  
  //#=============================================================================================
  #ifdef REFACTOR  
  else
  {
    TU_LOG1("Get request not handled, entity = %d, selector = %d, request = %d\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);
  }
  #endif //REFACTOR
  //#=============================================================================================
  
  return false;
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf)
{
  audio_control_request_t const *request = (audio_control_request_t const *)p_request;

  if (request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT)
    return tud_audio_feature_unit_set_request(rhport, request, buf);
  if (request->bEntityID == UAC2_ENTITY_CLOCK)
    return tud_audio_clock_set_request(rhport, request, buf);
  


  //#=============================================================================================
    #ifdef REFACTOR
  TU_LOG1("Set request not handled, entity = %d, selector = %d, request = %d\r\n",
          request->bEntityID, request->bControlSelector, request->bRequest);
  #endif //REFACTOR
  //#=============================================================================================
  

  return false;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void)rhport;

  uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
  uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

  if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt == 0)
      blink_interval_ms = BLINK_MOUNTED;

  return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void)rhport;
  uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
  uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

  //#=============================================================================================
    #ifdef REFACTOR
  TU_LOG2("Set interface %d alt %d\r\n", itf, alt);
  #endif //REFACTOR
  //#=============================================================================================
  
  if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt != 0)
      blink_interval_ms = BLINK_STREAMING;

  // Clear buffer when streaming format is changed
  spk_data_size = 0;
  if(alt != 0)
  {
    current_resolution = resolutions_per_format[alt-1];
  }

  return true;
}

bool tud_audio_rx_done_pre_read_cb(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
{
  (void)rhport;
  (void)func_id;
  (void)ep_out;
  (void)cur_alt_setting;

  spk_data_size = tud_audio_read(spk_buf, n_bytes_received);
  return true;
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  (void)rhport;
  (void)itf;
  (void)ep_in;
  (void)cur_alt_setting;

  // This callback could be used to fill microphone data separately
  return true;
}

//--------------------------------------------------------------------+
// AUDIO Task
//--------------------------------------------------------------------+

void audio_task(void)
{
  // When new data arrived, copy data from speaker buffer, to microphone buffer
  // and send it over
  // Only support speaker & headphone both have the same resolution
  // If one is 16bit another is 24bit be care of LOUD noise !
  if (spk_data_size)
  {
    if (current_resolution == 16)
    {
      int16_t *src = (int16_t*)spk_buf;
      int16_t *limit = (int16_t*)spk_buf + spk_data_size / 2;
      int16_t *dst = (int16_t*)mic_buf;
      while (src < limit)
      {
        // Combine two channels into one
        int32_t left = *src++;
        int32_t right = *src++;
        *dst++ = (int16_t) ((left >> 1) + (right >> 1));
      }
      tud_audio_write((uint8_t *)mic_buf, (uint16_t) (spk_data_size / 2));
      spk_data_size = 0;
    }
    else if (current_resolution == 24)
    {
      int32_t *src = spk_buf;
      int32_t *limit = spk_buf + spk_data_size / 4;
      int32_t *dst = mic_buf;
      while (src < limit)
      {
        // Combine two channels into one
        int32_t left = *src++;
        int32_t right = *src++;
        *dst++ = (int32_t) ((uint32_t) ((left >> 1) + (right >> 1)) & 0xffffff00ul);
      }
      tud_audio_write((uint8_t *)mic_buf, (uint16_t) (spk_data_size / 2));
      spk_data_size = 0;
    }
  }
}

//--------------------------------------------------------------------+
// AUDIO Task for RADIO
//--------------------------------------------------------------------+

int USB_Audio_read(int16_t *audio_read_data) {
  if (spk_data_size)
  {
    if (current_resolution == 16)
    {
      int16_t *src = (int16_t*)spk_buf;
      int16_t *limit = (int16_t*)spk_buf + spk_data_size / 2;
      int32_t mono_sum = 0;
      int i=0;
      while (src < limit)
      {
        int16_t outL = *src++;
        int16_t outR = *src++;
        int32_t mono = outL + outR;
        *(audio_read_data +i)= (int16_t)(mono / 2);
        mono_sum += mono;
        i++;
      }
      spk_data_size = 0;
      if (mono_sum == 0) return 0; 
      else return i;
    }
    else if (current_resolution == 24)
    {
      int32_t *src = spk_buf;
      int32_t *limit = spk_buf + spk_data_size / 4;
      int32_t mono_sum = 0;
      int i=0;
      while (src < limit)
      {
        int32_t outL = *src++;
        int32_t outR = *src++;
        int32_t mono = (outL >> 1) + (outR >> 1);
        *(audio_read_data +i) = (int16_t)(mono >> 8);
        mono_sum += mono;
        i++;
      }
      spk_data_size = 0;
      if (mono_sum == 0) return 0; 
      else return i;
    }
  }
  return 0; 
}

void USB_Audio_write(int16_t *audio_write_data, int16_t data_number) 
{
  if (current_resolution == 16)
  {
      int16_t *dst = (int16_t*)mic_buf;
      for (int i = 0; i < data_number; i++)
      {
        *dst++ = audio_write_data[i];
      }
      tud_audio_write((uint8_t *)mic_buf, (uint16_t)(data_number * 2));   // 48 * 2 (16bit/2) = 96
  }
  else if (current_resolution == 24)
  {
      int32_t *dst = mic_buf;
      for (int i = 0; i < data_number; i++)
      {
        *dst++ = (int32_t)audio_write_data[i];
      }
      tud_audio_write((uint8_t *)mic_buf, (uint16_t)(data_number * 4));
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+

void led_blinking_task(void)
{
  static bool led_state = false;
  static uint32_t start_ms = 0;

  // Blink every interval ms
  // *fix* if (board_millis() - start_ms < blink_interval_ms) return;
  // *fix* start_ms += blink_interval_ms;

  if ((to_ms_since_boot(get_absolute_time()) - start_ms) < blink_interval_ms) return;
  start_ms += blink_interval_ms;



  //*fix* board_led_write(led_state);

//#=============================================================================================
  #ifdef REFACTOR
  gpio_put(PICO_DEFAULT_LED_PIN, led_state);
  #endif //REFACTOR  
//#=============================================================================================
  


  led_state = 1 - led_state;
}
