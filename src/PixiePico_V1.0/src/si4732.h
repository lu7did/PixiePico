#pragma once
 /*
 * =======================================================================================
 * si4732
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * Implementation of a rp2040 based controller  of a si4732 digital receiver 
 * =======================================================================================
 * This is mainly an integration effort, the code in this library has been developed 
 * from scratch for this project.
 * However the work received an huge benefit from previous work from many parties,
 * including myself as follows:
 *----------------------------------------------------------------------------
 * Version 1.0
 * - Initial release
 *----------------------------------------------------------------------------
 * SI4732 driver - RP2040 Pico SDK - I2C
 * Modular driver intended for later integration with larger TinyUSB projects.
 *
 * Notes:
 * - SSB patch is left as placeholder (all zeros) for now.
 * - Region profiles + band presets are provided.
 *----------------------------------------------------------------------------
  */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hardware/i2c.h"

//*----------------------------------------------------------------------------------------*
//*                                Defines                                                 *
//*----------------------------------------------------------------------------------------*
#ifdef __cplusplus
extern "C" {
#endif

//*--  7-bit I2C address (commonly 0x11; some modules may use 0x63 depending on SEN strap)
#ifndef SI4732_I2C_ADDR_DEFAULT
#define SI4732_I2C_ADDR_DEFAULT 0x11u
#endif

 //*----------------------------------------------------------------------------------------*
 //*                                 Control structures and definitions                    *
 //*---------------------------------------------------------------------------------------*
typedef enum {
  SI4732_OK = 0,
  SI4732_ERR_I2C = -1,
  SI4732_ERR_TIMEOUT = -2,
  SI4732_ERR_ARG = -3,
  SI4732_ERR_DEVICE = -4,
  SI4732_ERR_PARAM = -5

} si4732_status_t;

typedef enum {
  SI4732_MODE_FM  = 0,
  SI4732_MODE_AM  = 1,   // AM/SW/LW
  SI4732_MODE_SSB = 2
} si4732_mode_t;

typedef enum {
  SI4732_REGION_US = 0,
  SI4732_REGION_EU = 1,
  SI4732_REGION_JP = 2,
  SI4732_REGION_AR = 3   // Argentina (práctico: FM 100 kHz; AM 10 kHz)
} si4732_region_t;

typedef struct {
  //*--- Seek band limits and seek spacing
  
  uint16_t fm_bottom_10khz;      // e.g. 8750
  uint16_t fm_top_10khz;         // e.g. 10800
  uint16_t fm_seek_spacing_khz;  // 50/100/200 typical (we use 100 for AR/EU)
  uint16_t fm_tune_step_10khz;   // tune step in 10kHz units (10=100kHz)
  uint16_t am_bottom_khz;        // e.g. 520
  uint16_t am_top_khz;           // e.g. 1710
  uint16_t am_seek_spacing_khz;  // 9/10
  uint16_t am_tune_step_khz;     // 1/5/9/10 etc
} si4732_region_profile_t;

typedef struct {
  si4732_mode_t mode;
  uint32_t min;                  // FM: 10kHz units; AM/SSB: kHz
  uint32_t max;                  // same
  uint16_t step;                 // FM: 10kHz units; AM/SSB: kHz
  uint16_t spacing;              // seek spacing in kHz
} si4732_band_t;

typedef enum {
  SI4732_BAND_FM_BROADCAST = 0,
  SI4732_BAND_AM_MW,

  // --- HF Ham bands ---
  SI4732_BAND_HAM_160M,
  SI4732_BAND_HAM_80M,
  SI4732_BAND_HAM_60M,
  SI4732_BAND_HAM_40M,
  SI4732_BAND_HAM_30M,
  SI4732_BAND_HAM_20M,
  SI4732_BAND_HAM_17M,
  SI4732_BAND_HAM_15M,
  SI4732_BAND_HAM_12M,
  SI4732_BAND_HAM_10M,

  // --- Shortwave broadcast (not used really)
  SI4732_BAND_SW_49M,
  SI4732_BAND_SW_31M

} si4732_band_preset_t;

#define SI473X_PROP_AM_CHANNEL_FILTER 0x3102

typedef enum {
  SI473X_AM_BW_6KHZ  = 0,
  SI473X_AM_BW_4KHZ  = 1,
  SI473X_AM_BW_3KHZ  = 2,
  SI473X_AM_BW_2KHZ  = 3,
  SI473X_AM_BW_1KHZ  = 4,
  SI473X_AM_BW_1_8KHZ= 5,
  SI473X_AM_BW_2_5KHZ= 6
} si473x_am_bw_t;

// Configura el ancho de banda AM/SSB y opcionalmente el filtro AMPLFLT.




typedef struct {
  i2c_inst_t *i2c;
  uint8_t addr;

  uint sda_pin;
  uint scl_pin;
  uint reset_pin;
  uint32_t baud_hz;

  si4732_region_t region;
  si4732_region_profile_t region_profile;

  si4732_mode_t mode;
  uint32_t freq;                       // FM: 10kHz units; AM/SSB: kHz
  bool     present;                    // true if the Si4732 answered when the init was performed
  uint8_t  last_status;                // Last reading status (CTS status)
} si4732_t;
 //*---------------------------------------------------------------------------------------*
 //*                                 Prototypes                                            *
 //*---------------------------------------------------------------------------------------*
 // Agregar cerca de tus typedefs/enums:

si4732_status_t si4732_set_am_bandwidth(si4732_t *dev,
                                        si473x_am_bw_t bw,
                                        bool power_line_noise_reject,
                                        uint32_t timeout_ms);

//*--- init 
si4732_status_t si4732_init(si4732_t *dev,
                            i2c_inst_t *i2c,
                            uint8_t addr,
                            uint sda_pin, uint scl_pin,
                            uint reset_pin,
                            uint32_t baud_hz);

//*--- Reset helper (active-low). Pulses reset low then releases high.

si4732_status_t si4732_reset_pulse(si4732_t *dev, uint32_t low_ms, uint32_t settle_ms);
si4732_status_t si4732_power_up_fm(si4732_t *dev);
si4732_status_t si4732_power_up_am(si4732_t *dev, bool patch_enable);
si4732_status_t si4732_power_down(si4732_t *dev);

si4732_status_t si4732_get_rev(si4732_t *dev, uint8_t out_resp[9]);

//*--- Properties
si4732_status_t si4732_set_property(si4732_t *dev, uint16_t prop, uint16_t value);
si4732_status_t si4732_get_property(si4732_t *dev, uint16_t prop, uint16_t *out_value);

//*--- Region and band settings

si4732_region_profile_t si4732_region_profile(si4732_region_t r);
si4732_status_t si4732_apply_region(si4732_t *dev, si4732_region_t r);

si4732_band_t si4732_band_preset(si4732_band_preset_t p, si4732_region_profile_t rp);
si4732_status_t si4732_set_band(si4732_t *dev, const si4732_band_t *band);

//*--- Tuning and Seek functions

si4732_status_t si4732_tune(si4732_t *dev, uint32_t freq);
si4732_status_t si4732_seek(si4732_t *dev, bool up, bool wrap);

//*--- Audio and AGC control

si4732_status_t si4732_set_volume(si4732_t *dev, uint8_t vol);
si4732_status_t si4732_set_mute(si4732_t *dev, bool left_mute, bool right_mute);
si4732_status_t si4732_set_softmute_fm(si4732_t *dev,
                                       uint16_t max_attn, uint16_t snr_thr,
                                       uint16_t slope, uint16_t attack,
                                       uint16_t release, uint16_t rate);
si4732_status_t si4732_set_softmute_am(si4732_t *dev,
                                       uint16_t max_attn, uint16_t snr_thr,
                                       uint16_t slope, uint16_t attack,
                                       uint16_t release, uint16_t rate);
si4732_status_t si4732_set_agc(si4732_t *dev, bool disable_agc, uint8_t gain_index);

//*--- SSB wrappers & patch

extern const uint8_t si4732_ssb_patch[];
extern const size_t  si4732_ssb_patch_len;


//*---  Generic patch loader (chunked). Placeholder patch is all zeros.
si4732_status_t si4732_load_patch(si4732_t *dev, const uint8_t *patch, size_t patch_len);

//*--- SSB wrappers (no-op placeholders until real patch command set is added)
si4732_status_t si4732_ssb_enter(si4732_t *dev);
si4732_status_t si4732_ssb_set_bfo(si4732_t *dev, int16_t bfo_hz);
si4732_status_t si4732_ssb_set_sideband(si4732_t *dev, bool usb); // true=USB false=LSB
si4732_status_t si4732_ssb_set_filter(si4732_t *dev, uint8_t filter_idx);
// --- Read back tuned frequency from the chip (FM: 10kHz units, AM: kHz)

si4732_status_t si4732_get_tuned_freq(si4732_t *dev, uint32_t *out_freq);
si4732_status_t si4732_dump_tune_status(si4732_t *dev, uint8_t out8[8]);

si4732_status_t si4732_get_tune_status(si4732_t *dev,
                                       bool clear_int,
                                       uint32_t *out_freq,
                                       uint8_t *out_rssi,
                                       uint8_t *out_snr,
                                       bool *out_stc);


si4732_status_t cmd_write_read(si4732_t *dev,
                                      const uint8_t *cmd, size_t cmd_len,
                                      uint8_t *resp, size_t resp_len,
                                      uint32_t timeout_ms);
                                      
si4732_status_t wait_stc(si4732_t *dev, uint32_t timeout_ms, bool clear_int);
si4732_status_t i2c_read_bytes(si4732_t *dev, uint8_t *buf, size_t len);
si4732_status_t i2c_write_bytes(si4732_t *dev, const uint8_t *buf, size_t len);



//*--- Probe if the board has an active Si4732 device
bool si4732_probe(si4732_t *dev);


#ifdef __cplusplus
}
#endif
