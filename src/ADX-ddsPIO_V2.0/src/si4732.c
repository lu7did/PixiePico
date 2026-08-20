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
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
 *                       Libraries and Packages used                        *
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
 *
 * The library has been written from the ground up, but it's strongly
 * inspired, and for some features reverse engineered from the titanic work
 * made by Ricardo Caratti (PU2CLR) and specially from his world recognized
 * library for the Arduino SI4735 (https://github.com/pu2clr/SI4735)
 * This library has been written in C++ and it's not compatible with the
 * C/C++ rp2040 SDK and many features of the Arduino platform aren't
 * really that portable to the rp2040 environment and Visual Studio Code.
 * But I found myself searching on that code countless times to grasp the
 * understanding of many aspects, probably this work can not be possible
 * without that library.
 *---------------------------------------------------------------------------------------*
 * This library receives the considerable learning made when developing the
 * ADX-rp2040 package and specially the RDX package which provides support
 * for the rp2040 processor albeit using a cross platform compatibility layer
 * allowing the usage of the Arduino libraries and IDE to develop for other boards
 * in general and the rp2040 in particular, repositories for these projects are
 *
 *     ADX-rp2040    https://github.com/lu7did/ADX-rp2040
 *     RDX           https://github.com/lu7did/RDX-rp2040
 */
//*---------------------------------------------------------------------------------------*
//*                                   includes                                            *
//*---------------------------------------------------------------------------------------*
#include "si4732.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <string.h>

//*--- Forward declarations (static)

static si4732_status_t power_up_common(si4732_t *dev,
                                       uint8_t func,
                                       bool patch_enable);

//*---------------------------------------------------------------------------------------*
//*                                  commands                                             *
//*---------------------------------------------------------------------------------------*

#define CMD_POWER_UP 0x01u
#define CMD_GET_REV 0x10u
#define CMD_POWER_DOWN 0x11u
#define CMD_SET_PROPERTY 0x12u
#define CMD_GET_PROPERTY 0x13u
#define CMD_GET_INT_STATUS 0x14u

#define CMD_FM_TUNE_FREQ 0x20u
#define CMD_FM_SEEK_START 0x21u

#define CMD_AM_TUNE_FREQ 0x40u
#define CMD_AM_SEEK_START 0x41u

#define CMD_AGC_OVERRIDE 0x48u

//*--- Generic patch load (placeholder usage)

#define CMD_LOAD_PATCH 0x15u

//*--- Read frequency

#define CMD_FM_TUNE_STATUS 0x22u
#define CMD_AM_TUNE_STATUS 0x42u

//*--- Status bits
#define ST_CTS 0x80u
#define ST_ERR 0x40u

//*---------------------------------------------------------------------------------------*
//*                                  properties                                           *
//*---------------------------------------------------------------------------------------*
#define PROP_RX_VOLUME 0x4000u
#define PROP_RX_HARD_MUTE 0x4001u

//*--- FM seek band limits
#define PROP_FM_SEEK_BAND_BOTTOM 0x1400u
#define PROP_FM_SEEK_BAND_TOP 0x1401u
#define PROP_FM_SEEK_FREQ_SPACING 0x1402u

//*--- FM soft mute
#define PROP_FM_SOFT_MUTE_RATE 0x1300u
#define PROP_FM_SOFT_MUTE_SLOPE 0x1301u
#define PROP_FM_SOFT_MUTE_MAX_ATTENUATION 0x1302u
#define PROP_FM_SOFT_MUTE_SNR_THRESHOLD 0x1303u
#define PROP_FM_SOFT_MUTE_RELEASE_RATE 0x1304u
#define PROP_FM_SOFT_MUTE_ATTACK_RATE 0x1305u

//*--- AM seek band limits / spacing
//#define PROP_AM_SEEK_BAND_BOTTOM 0x3400u
//#define PROP_AM_SEEK_BAND_TOP 0x3401u
//#define PROP_AM_SEEK_FREQ_SPACING 0x3402u

// AM seek band limits / spacing  (Si47xx)
#define PROP_AM_SEEK_BAND_BOTTOM           0x3100u
#define PROP_AM_SEEK_BAND_TOP              0x3101u
#define PROP_AM_SEEK_FREQ_SPACING          0x3102u


//*--- AM soft mute
#define PROP_AM_SOFT_MUTE_RATE 0x3300u
#define PROP_AM_SOFT_MUTE_SLOPE 0x3301u
#define PROP_AM_SOFT_MUTE_MAX_ATTENUATION 0x3302u
#define PROP_AM_SOFT_MUTE_SNR_THRESHOLD 0x3303u
#define PROP_AM_SOFT_MUTE_RELEASE_RATE 0x3304u
#define PROP_AM_SOFT_MUTE_ATTACK_RATE 0x3305u

//*---------------------------------------------------------------------------------------*
//*                                  I2C helpers                                          *
//*---------------------------------------------------------------------------------------*
si4732_status_t i2c_write_bytes(si4732_t *dev, const uint8_t *buf, size_t len)
{
  //absolute_time_t until = make_timeout_time_ms(20);
  absolute_time_t until = make_timeout_time_ms(200);
  
  int rc = i2c_write_blocking_until(dev->i2c, dev->addr, buf, len, false, until);
  if (rc < 0)
    return SI4732_ERR_I2C;
  if ((size_t)rc != len)
    return SI4732_ERR_I2C;
  return SI4732_OK;
}

si4732_status_t i2c_read_bytes(si4732_t *dev, uint8_t *buf, size_t len)
{
  //absolute_time_t until = make_timeout_time_ms(20);
  absolute_time_t until = make_timeout_time_ms(200);
  
  int rc = i2c_read_blocking_until(dev->i2c, dev->addr, buf, len, false, until);
  if (rc < 0)
    return SI4732_ERR_I2C;
  if ((size_t)rc != len)
    return SI4732_ERR_I2C;
  return SI4732_OK;
}

bool si4732_probe(si4732_t *dev)
{
  if (!dev || !dev->i2c)
    return false;

  uint8_t tmp = 0;

  //*--- Short timeout to detect bus activity

  absolute_time_t until = make_timeout_time_ms(2);

  int rc = i2c_read_blocking_until(dev->i2c, dev->addr, &tmp, 1, false, until);
  return (rc == 1);
}

//*---------------------------------------------------------------------------------------*
//*                              receiver commands                                        *
//*---------------------------------------------------------------------------------------*

//* When not used ---> static __attribute__((unused))

//*--- read status byte (updates dev->last_status)

static si4732_status_t read_status(si4732_t *dev, uint8_t *st)
{
  if (!dev || !st)
    return SI4732_ERR_ARG;

  si4732_status_t rc = i2c_read_bytes(dev, st, 1);
  if (rc == SI4732_OK)
    dev->last_status = *st;
  return rc;
}

//*--- Clear/ack interrupt/status (helps clear latched ERR on some sequences)

static si4732_status_t clear_int_status(si4732_t *dev)
{
  if (!dev)
    return SI4732_ERR_ARG;

  uint8_t cmd = CMD_GET_INT_STATUS; // 0x14
  si4732_status_t rc = i2c_write_bytes(dev, &cmd, 1);
  if (rc != SI4732_OK)
    return rc;

  uint8_t st = 0;
  rc = read_status(dev, &st); // read 1 byte back
  return rc;
}

//*--- Wait CTS with one ERR-clear retry

static si4732_status_t wait_cts(si4732_t *dev, uint32_t timeout_ms)
{
  if (!dev)
    return SI4732_ERR_ARG;

  absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
  bool cleared_once = false;

  while (!time_reached(deadline))
  {
    uint8_t st = 0;
    si4732_status_t rc = read_status(dev, &st);
    if (rc != SI4732_OK)
      return rc;

    //*--- CTS set?

    if (st & ST_CTS)
    {
      //*--- ERR set?
      if (st & ST_ERR)
      {

        //*--- Many Si47xx flows benefit from clearing status once then retrying.

        if (!cleared_once)
        {
          cleared_once = true;
          (void)clear_int_status(dev);
          sleep_ms(1);
          continue;
        }
        return SI4732_ERR_DEVICE;
      }
      return SI4732_OK;
    }

    sleep_ms(1);
  }

  return SI4732_ERR_TIMEOUT;
}

//*--- Command write: write bytes then wait CTS

static si4732_status_t cmd_write(si4732_t *dev, const uint8_t *cmd, size_t len, uint32_t timeout_ms)
{
  if (!dev || !cmd || len == 0)
    return SI4732_ERR_ARG;

  si4732_status_t rc = i2c_write_bytes(dev, cmd, len);
  if (rc != SI4732_OK)
    return rc;

  return wait_cts(dev, timeout_ms);
}

//*---------------------------------------------------------------------------------------*
//*                              Public API                                               *
//*---------------------------------------------------------------------------------------*

//*--- Init chipset

si4732_status_t si4732_init(si4732_t *dev,
                            i2c_inst_t *i2c,
                            uint8_t addr,
                            uint sda_pin, uint scl_pin,
                            uint reset_pin,
                            uint32_t baud_hz)
{
  if (!dev || !i2c)
    return SI4732_ERR_ARG;

  //*--- Clean up struct before use (kinky nasty bug might happen if not, don't ask how I know that)

  memset(dev, 0, sizeof(*dev));

  dev->i2c = i2c;
  dev->addr = addr;
  dev->sda_pin = sda_pin;
  dev->scl_pin = scl_pin;
  dev->reset_pin = reset_pin;
  dev->baud_hz = baud_hz ? baud_hz : 400000u;

  dev->mode = SI4732_MODE_FM;
  dev->freq = 0;

  //*--- I2C init

  i2c_init(dev->i2c, dev->baud_hz);
  gpio_set_function(dev->sda_pin, GPIO_FUNC_I2C);
  gpio_set_function(dev->scl_pin, GPIO_FUNC_I2C);
  gpio_pull_up(dev->sda_pin);
  gpio_pull_up(dev->scl_pin);

  //*--- RESET pin

  gpio_init(dev->reset_pin);
  gpio_set_dir(dev->reset_pin, GPIO_OUT);

  //*--- The pullup isn't really necessary on output pins, however on actual testing it changed the bug I was experiencing

  gpio_pull_up(dev->reset_pin);

  gpio_put(dev->reset_pin, 1);
  sleep_ms(10);

  //*--- Set región default (Sorry, firmware made in Argentina, then the default region is Argentina)

  dev->region = SI4732_REGION_AR;
  dev->region_profile = si4732_region_profile(dev->region);

  //*--- Reset the chip

  (void)si4732_reset_pulse(dev, 10, 50);
  sleep_ms(100);

  //*--- Quickly check if the chip answers

  if (!si4732_probe(dev))
  {
    dev->present = false;
    return SI4732_ERR_I2C;
  }

  //*--- It seems there are a chip after all

  dev->present = true;

  //*--- This is absolutely critical POWER_UP must use XOSCEN if there is crystal of 32 KHz

  si4732_status_t rc = power_up_common(dev, 0u, false); // FM_RX
  if (rc != SI4732_OK)
  {
    dev->present = false;
    return rc;
  }

  //*---  GET_REV to confirm the chip is there

  uint8_t rev[9];
  rc = si4732_get_rev(dev, rev);
  if (rc != SI4732_OK)
  {
    dev->present = false;
    return rc;
  }

  //*--- So far so good, the initialization has been completed
  dev->present = true;
  return SI4732_OK;
}

si4732_status_t si4732_reset_pulse(si4732_t *dev, uint32_t low_ms, uint32_t settle_ms)
{
  if (!dev)
    return SI4732_ERR_ARG;
  gpio_put(dev->reset_pin, 0);
  sleep_ms(low_ms);
  gpio_put(dev->reset_pin, 1);
  sleep_ms(settle_ms);
  return SI4732_OK;
}

//*--- Power up sequences
//*--- For FM: func=FM_RX (0), opmode=0x05 (analog out)
//*--- For AM: func=AM_RX (1), opmode=0x05, patch_enable optional

static si4732_status_t power_up_common(si4732_t *dev, uint8_t func, bool patch_enable)
{
  if (!dev)
    return SI4732_ERR_ARG;

  uint8_t arg1 = 0x00;

  //*--- bits: [CTSIEN][GPO2OEN][PATCH][XOSCEN][FUNC3..0]

  if (patch_enable)
    arg1 |= (1u << 5); // PATCH
  arg1 |= (1u << 4);   // XOSCEN (cristal 32.768kHz)
  arg1 |= (uint8_t)(func & 0x0Fu);

  uint8_t cmd[3] = {CMD_POWER_UP, arg1, 0x05u}; // opmode 0x05 (analog out)
  si4732_status_t rc = i2c_write_bytes(dev, cmd, sizeof(cmd));
  if (rc != SI4732_OK)
    return rc;
  if (!dev->present)
    return SI4732_ERR_DEVICE;

  //*--- POWER_UP might take a little longer

  return wait_cts(dev, 1000);
}

//*--- FM power-up

si4732_status_t si4732_power_up_fm(si4732_t *dev)
{
  if (!dev)
    return SI4732_ERR_ARG;
  if (!dev->present)
    return SI4732_ERR_DEVICE;
  dev->mode = SI4732_MODE_FM;
  return power_up_common(dev, 0u, false);
}

//*--- AM power-up


si4732_status_t si4732_power_up_am(si4732_t *dev, bool patch_enable) {
  if (!dev) return SI4732_ERR_ARG;
  if (!dev->present) return SI4732_ERR_DEVICE;

  // Asegura transición limpia de modo
  (void)si4732_power_down(dev);
  sleep_ms(10);

  dev->mode = patch_enable ? SI4732_MODE_SSB : SI4732_MODE_AM;
  return power_up_common(dev, 1u, patch_enable);
}

//*--- Power-down

si4732_status_t si4732_power_down(si4732_t *dev)
{
  if (!dev)
    return SI4732_ERR_ARG;
  if (!dev->present)
    return SI4732_ERR_DEVICE;

  uint8_t cmd = CMD_POWER_DOWN;
  return cmd_write(dev, &cmd, 1, 500);
}

//*--- Get chipset firmware review

si4732_status_t si4732_get_rev(si4732_t *dev, uint8_t out_resp[9])
{
  if (!dev || !out_resp)
    return SI4732_ERR_ARG;

  uint8_t cmd = CMD_GET_REV;

  si4732_status_t rc = i2c_write_bytes(dev, &cmd, 1);
  if (rc != SI4732_OK)
    return rc;

  rc = wait_cts(dev, 500);
  if (rc != SI4732_OK)
    return rc;

  return i2c_read_bytes(dev, out_resp, 9);
}

//*--- Set property

si4732_status_t si4732_set_property(si4732_t *dev, uint16_t prop, uint16_t value)
{

  if (!dev)
    return SI4732_ERR_ARG;
  if (!dev->present)
    return SI4732_ERR_DEVICE;

  uint8_t cmd[6] = {
      CMD_SET_PROPERTY, 0x00,
      (uint8_t)(prop >> 8), (uint8_t)(prop & 0xFF),
      (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
  return cmd_write(dev, cmd, sizeof(cmd), 500);
}

//*--- Get (read) property

si4732_status_t si4732_get_property(si4732_t *dev, uint16_t prop, uint16_t *out_value)
{
  if (!dev || !out_value)
    return SI4732_ERR_ARG;
  if (!dev->present)
    return SI4732_ERR_DEVICE;

  uint8_t cmd[4] = {CMD_GET_PROPERTY, 0x00, (uint8_t)(prop >> 8), (uint8_t)(prop & 0xFF)};
  (void)wait_cts(dev, 500);
  si4732_status_t rc = i2c_write_bytes(dev, cmd, sizeof(cmd));
  if (rc != SI4732_OK)
    return rc;
  rc = wait_cts(dev, 500);
  if (rc != SI4732_OK)
    return rc;

  uint8_t resp[4] = {0};
  rc = i2c_read_bytes(dev, resp, sizeof(resp));
  if (rc != SI4732_OK)
    return rc;
  *out_value = (uint16_t)((resp[2] << 8) | resp[3]);





  return SI4732_OK;
}

//*---------------------------------------------------------------------------------------*
//*                           Regional profiles and band setup                            *
//*---------------------------------------------------------------------------------------*

//*--- Define region profile

si4732_region_profile_t si4732_region_profile(si4732_region_t r)
{

  //*--- Practical defaults; adjust as needed for your target compliance.

  si4732_region_profile_t rp = {0};

  switch (r)
  {
  case SI4732_REGION_US:
    rp.fm_bottom_10khz = 8790;
    rp.fm_top_10khz = 10790;
    rp.fm_seek_spacing_khz = 200;
    rp.fm_tune_step_10khz = 20; // 200kHz
    rp.am_bottom_khz = 520;
    rp.am_top_khz = 1710;
    rp.am_seek_spacing_khz = 10;
    rp.am_tune_step_khz = 10;
    break;
  case SI4732_REGION_JP:
    rp.fm_bottom_10khz = 7600;
    rp.fm_top_10khz = 9500;
    rp.fm_seek_spacing_khz = 100;
    rp.fm_tune_step_10khz = 10;
    rp.am_bottom_khz = 522;
    rp.am_top_khz = 1629;
    rp.am_seek_spacing_khz = 9;
    rp.am_tune_step_khz = 9;
    break;
  case SI4732_REGION_EU:
    rp.fm_bottom_10khz = 8750;
    rp.fm_top_10khz = 10800;
    rp.fm_seek_spacing_khz = 100;
    rp.fm_tune_step_10khz = 10; // 100kHz
    rp.am_bottom_khz = 531;
    rp.am_top_khz = 1602;
    rp.am_seek_spacing_khz = 9;
    rp.am_tune_step_khz = 9;
    break;
  case SI4732_REGION_AR:
  default:
    rp.fm_bottom_10khz = 8750;
    rp.fm_top_10khz = 10800;
    rp.fm_seek_spacing_khz = 100;
    rp.fm_tune_step_10khz = 10; // 100kHz typical usage
    rp.am_bottom_khz = 530;
    rp.am_top_khz = 1710;
    rp.am_seek_spacing_khz = 10;
    rp.am_tune_step_khz = 10;
    break;
  }
  return rp;
}

//*--- Apply region profile

si4732_status_t si4732_apply_region(si4732_t *dev, si4732_region_t r)
{
  if (!dev)
    return SI4732_ERR_ARG;
  dev->region = r;
  dev->region_profile = si4732_region_profile(r);

  //*--- Apply seek limits + spacing for current mode (caller may change band later)

  if (dev->mode == SI4732_MODE_FM)
  {
    si4732_status_t rc;
    rc = si4732_set_property(dev, PROP_FM_SEEK_BAND_BOTTOM, dev->region_profile.fm_bottom_10khz);
    if (rc)
      return rc;
    rc = si4732_set_property(dev, PROP_FM_SEEK_BAND_TOP, dev->region_profile.fm_top_10khz);
    if (rc)
      return rc;
    rc = si4732_set_property(dev, PROP_FM_SEEK_FREQ_SPACING, dev->region_profile.fm_seek_spacing_khz);
    if (rc)
      return rc;
    return SI4732_OK;
  }
  else
  {
    si4732_status_t rc;
    rc = si4732_set_property(dev, PROP_AM_SEEK_BAND_BOTTOM, dev->region_profile.am_bottom_khz);
    if (rc)
      return rc;
    rc = si4732_set_property(dev, PROP_AM_SEEK_BAND_TOP, dev->region_profile.am_top_khz);
    if (rc)
      return rc;
    rc = si4732_set_property(dev, PROP_AM_SEEK_FREQ_SPACING, dev->region_profile.am_seek_spacing_khz);
    if (rc)
      return rc;
    return SI4732_OK;
  }
}

//*--- Define band preset

si4732_band_t si4732_band_preset(si4732_band_preset_t p, si4732_region_profile_t rp)
{
  si4732_band_t b = {0};
  switch (p)
  {
  case SI4732_BAND_FM_BROADCAST:
    b.mode = SI4732_MODE_FM;
    b.min = rp.fm_bottom_10khz;
    b.max = rp.fm_top_10khz;
    b.step = rp.fm_tune_step_10khz;
    b.spacing = rp.fm_seek_spacing_khz;
    break;
  case SI4732_BAND_AM_MW:
    b.mode = SI4732_MODE_AM;
    b.min = rp.am_bottom_khz;
    b.max = rp.am_top_khz;
    b.step = rp.am_tune_step_khz;
    b.spacing = rp.am_seek_spacing_khz;
    break;
  case SI4732_BAND_SW_49M:
    b.mode = SI4732_MODE_AM;
    b.min = 5800;
    b.max = 6400;
    b.step = 5;
    b.spacing = 5;
    break;
  case SI4732_BAND_HAM_40M:
    b.mode = SI4732_MODE_AM;
    b.min = 7000;
    b.max = 7300;
    b.step = 1;
    b.spacing = 1;
    break;
  case SI4732_BAND_HAM_80M:
    b.mode = SI4732_MODE_SSB;
    b.min = 3500;
    b.max = 4000;
    b.step = 1;
    b.spacing = 1;
    break;
  case SI4732_BAND_HAM_160M:
    b.mode = SI4732_MODE_SSB;
    b.min = 1800;
    b.max = 1900;
    b.step = 1;
    b.spacing = 1;
    break;
  case SI4732_BAND_HAM_30M:
    b.mode = SI4732_MODE_SSB;
    b.min = 10000;
    b.max = 10100;
    b.step = 1;
    b.spacing = 1;
    break;
  case SI4732_BAND_HAM_17M:
    b.mode = SI4732_MODE_SSB;
    b.min = 18000;
    b.max = 18200;
    b.step = 1;
    b.spacing = 1;
    break;
  case SI4732_BAND_HAM_15M:
    b.mode = SI4732_MODE_SSB;
    b.min = 21000;
    b.max = 21500;
    b.step = 1;
    b.spacing = 1;
    break;
  case SI4732_BAND_HAM_12M:
    b.mode = SI4732_MODE_SSB;
    b.min = 24000;
    b.max = 24500;
    b.step = 1;
    b.spacing = 1;
    break;
  case SI4732_BAND_HAM_10M:
    b.mode = SI4732_MODE_SSB;
    b.min = 28000;
    b.max = 29000;
    b.step = 1;
    b.spacing = 1;
    break;
  case SI4732_BAND_HAM_20M:

  b.mode = SI4732_MODE_SSB;
    b.min = 14000;
    b.max = 14250;
    b.step = 1;
    b.spacing = 1;
    break;

  case SI4732_BAND_SW_31M:
  default:
    b.mode = SI4732_MODE_AM;
    b.min = 9400;
    b.max = 10000;
    b.step = 5;
    b.spacing = 5;
    break;
  }
  return b;
}


//*--- Set specific band

si4732_status_t si4732_set_band(si4732_t *dev, const si4732_band_t *band)
{
  if (!dev || !band)
    return SI4732_ERR_ARG;

  dev->mode = band->mode;

  if (band->mode == SI4732_MODE_FM)
  {
    si4732_status_t rc;
    rc = si4732_set_property(dev, PROP_FM_SEEK_BAND_BOTTOM, (uint16_t)band->min);
    if (rc)
      return rc;
    rc = si4732_set_property(dev, PROP_FM_SEEK_BAND_TOP, (uint16_t)band->max);
    if (rc)
      return rc;
    rc = si4732_set_property(dev, PROP_FM_SEEK_FREQ_SPACING, band->spacing);
    if (rc)
      return rc;
    return SI4732_OK;
  }

  //*--- Set  AM / SSB
  si4732_status_t rc;
  rc = si4732_set_property(dev, PROP_AM_SEEK_BAND_BOTTOM, (uint16_t)band->min);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_AM_SEEK_BAND_TOP, (uint16_t)band->max);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_AM_SEEK_FREQ_SPACING, band->spacing);
  if (rc)
    return rc;
  return SI4732_OK;
}

//*---------------------------------------------------------------------------------------*
//*                           Tunning and Scanning                                        *
//*---------------------------------------------------------------------------------------*
si4732_status_t si4732_tune(si4732_t *dev, uint32_t freq)
{
  if (!dev)
    return SI4732_ERR_ARG;
  if (!dev->present)
    return SI4732_ERR_DEVICE;

  si4732_status_t rc;

  if (dev->mode == SI4732_MODE_FM)
  {
    // FM en unidades de 10kHz
    uint16_t f10 = (uint16_t)freq;
    uint8_t cmd[6] = {
        0x20, 0x00, // FM_TUNE_FREQ, ARG1
        (uint8_t)(f10 >> 8), (uint8_t)(f10 & 0xFF),
        0x00, 0x00};

    rc = cmd_write(dev, cmd, sizeof(cmd), 500); // CTS (aceptación)
    if (rc != SI4732_OK)
      return rc;

    // Esperar fin real del sintonizado
    rc = wait_stc(dev, 2000, false);
    if (rc != SI4732_OK)
      return rc;

    // Limpia el flag STC si querés (opcional pero recomendable)
    (void)wait_stc(dev, 500, true);

    dev->freq = freq;
    return SI4732_OK;
  }
  else
  {
    // AM/SSB en kHz

    uint16_t fk = (uint16_t)freq;

    uint8_t antcapH = 0x00;
    uint8_t antcapL = 0x00;

    // SW/HF: usar ANTCAPL=1 según nota de Si47xx
    if (fk > 1710) {      // umbral práctico: salís de MW
       antcapL = 0x01;
    }

    uint8_t cmd[6] = {
      0x40, 0x00,                 // AM_TUNE_FREQ, FAST=0
      (uint8_t)(fk >> 8), (uint8_t)(fk & 0xFF),
      antcapH, antcapL
    };


    rc = cmd_write(dev, cmd, sizeof(cmd), 500); // CTS (aceptación) 
    if (rc != SI4732_OK) return rc;

    rc = wait_stc(dev, 2000, false); 
    if (rc != SI4732_OK) return rc;
    (void)wait_stc(dev, 500, true);
    dev->freq = freq;
    return SI4732_OK;
  }
}

//*--- Seek

si4732_status_t si4732_seek(si4732_t *dev, bool up, bool wrap)
{
  if (!dev)
    return SI4732_ERR_ARG;

  //*--- Common bit conventions: SEEKUP bit, WRAP bit; we keep this conservative.

  uint8_t arg1 = 0;
  if (up)
    arg1 |= (1u << 3); // SEEKUP
  if (wrap)
    arg1 |= (1u << 2); // WRAP

  if (dev->mode == SI4732_MODE_FM)
  {
    uint8_t cmd[2] = {CMD_FM_SEEK_START, arg1};
    return cmd_write(dev, cmd, sizeof(cmd), 1500);
  }
  else
  {
    uint8_t cmd[2] = {CMD_AM_SEEK_START, arg1};
    return cmd_write(dev, cmd, sizeof(cmd), 1500);
  }
}

//*--- Control Audio volume

si4732_status_t si4732_set_volume(si4732_t *dev, uint8_t vol)
{
  if (!dev)
    return SI4732_ERR_ARG;
  if (vol > 63)
    vol = 63;
  return si4732_set_property(dev, PROP_RX_VOLUME, (uint16_t)vol);
}

//*--- Mute the receiver

si4732_status_t si4732_set_mute(si4732_t *dev, bool left_mute, bool right_mute)
{
  if (!dev)
    return SI4732_ERR_ARG;
  uint16_t v = 0;
  if (right_mute)
    v |= 0x0001u;
  if (left_mute)
    v |= 0x0002u;
  return si4732_set_property(dev, PROP_RX_HARD_MUTE, v);
}

//*--- Softmute (FM)

si4732_status_t si4732_set_softmute_fm(si4732_t *dev,
                                       uint16_t max_attn, uint16_t snr_thr,
                                       uint16_t slope, uint16_t attack,
                                       uint16_t release, uint16_t rate)
{
  if (!dev)
    return SI4732_ERR_ARG;
  si4732_status_t rc;
  rc = si4732_set_property(dev, PROP_FM_SOFT_MUTE_MAX_ATTENUATION, max_attn);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_FM_SOFT_MUTE_SNR_THRESHOLD, snr_thr);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_FM_SOFT_MUTE_SLOPE, slope);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_FM_SOFT_MUTE_ATTACK_RATE, attack);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_FM_SOFT_MUTE_RELEASE_RATE, release);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_FM_SOFT_MUTE_RATE, rate);
  if (rc)
    return rc;
  return SI4732_OK;
}

//*--- Softmute (AM)

si4732_status_t si4732_set_softmute_am(si4732_t *dev,
                                       uint16_t max_attn, uint16_t snr_thr,
                                       uint16_t slope, uint16_t attack,
                                       uint16_t release, uint16_t rate)
{
  if (!dev)
    return SI4732_ERR_ARG;
  si4732_status_t rc;
  rc = si4732_set_property(dev, PROP_AM_SOFT_MUTE_MAX_ATTENUATION, max_attn);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_AM_SOFT_MUTE_SNR_THRESHOLD, snr_thr);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_AM_SOFT_MUTE_SLOPE, slope);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_AM_SOFT_MUTE_ATTACK_RATE, attack);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_AM_SOFT_MUTE_RELEASE_RATE, release);
  if (rc)
    return rc;
  rc = si4732_set_property(dev, PROP_AM_SOFT_MUTE_RATE, rate);
  if (rc)
    return rc;
  return SI4732_OK;
}

//*--- Set AGC

si4732_status_t si4732_set_agc(si4732_t *dev, bool disable_agc, uint8_t gain_index)
{
  if (!dev)
    return SI4732_ERR_ARG;
  uint8_t cmd[3] = {CMD_AGC_OVERRIDE, (uint8_t)(disable_agc ? 0x01u : 0x00u), gain_index};
  return cmd_write(dev, cmd, sizeof(cmd), 500);
}

//*---------------------------------------------------------------------------------------*
//*                           SSB patch (not official)                                    *
//*---------------------------------------------------------------------------------------*
si4732_status_t si4732_load_patch(si4732_t *dev, const uint8_t *patch, size_t patch_len)
{
  if (!dev || !patch || patch_len == 0) return SI4732_ERR_ARG;
  if (!dev->present) return SI4732_ERR_DEVICE;

  //*--- Mimic the algorithm used by Ricardo (PU2CLR) on his library
  if ((patch_len % 8) != 0) return SI4732_ERR_ARG;

  //*--- Ensure CTS status before start
  si4732_status_t rc = wait_cts(dev, 1000);
  if (rc != SI4732_OK) return rc;

  for (size_t off = 0; off < patch_len; off += 8) {

    //*--- Send 1 cmd + 7 data 
    rc = i2c_write_bytes(dev, &patch[off], 8);
    if (rc != SI4732_OK) return rc;

    //*--- Wait for CTS or ERR 
    rc = wait_cts(dev, 1000);
    if (rc != SI4732_OK) return rc;
  }

  return SI4732_OK;
}

//*--- switch to SSB

si4732_status_t si4732_ssb_enter(si4732_t *dev)
{
  if (!dev)
    return SI4732_ERR_ARG;
  dev->mode = SI4732_MODE_SSB;
  return SI4732_OK;
}

//*--- set bfo

si4732_status_t si4732_ssb_set_bfo(si4732_t *dev, int16_t bfo_hz)
{
  (void)dev;
  (void)bfo_hz;
  // Placeholder: depends on patch command set.
  return SI4732_OK;
}

//*--- select sideband

si4732_status_t si4732_ssb_set_sideband(si4732_t *dev, bool usb)
{
  (void)dev;
  (void)usb;
  // Placeholder: depends on patch command set.
  return SI4732_OK;
}

//*--- set internal filter

si4732_status_t si4732_ssb_set_filter(si4732_t *dev, uint8_t filter_idx)
{
  (void)dev;
  (void)filter_idx;
  // Placeholder: depends on patch command set.
  return SI4732_OK;
}

//*--- Read frequency

#define CMD_FM_TUNE_STATUS 0x22u
#define CMD_AM_TUNE_STATUS 0x42u

// Devuelve la frecuencia REAL reportada por el chip.
// - En FM: retorna en unidades de 10kHz (ej: 10030 => 100.30 MHz)
// - En AM: retorna en kHz (ej: 14074 => 14.074 MHz)
si4732_status_t si4732_get_tuned_freq(si4732_t *dev, uint32_t *out_freq)
{
  if (!dev || !out_freq)
    return SI4732_ERR_ARG;
  if (!dev->present)
    return SI4732_ERR_DEVICE;

  uint8_t resp[8] = {0};
  uint8_t cmd[2];

  // arg1: INTACK. 0x00 suele funcionar bien para “solo leer estado”
  cmd[0] = (dev->mode == SI4732_MODE_FM) ? CMD_FM_TUNE_STATUS : CMD_AM_TUNE_STATUS;
  cmd[1] = 0x00;

  si4732_status_t rc = cmd_write_read(dev, cmd, sizeof(cmd), resp, sizeof(resp), 800);
  if (rc != SI4732_OK)
    return rc;

  // Formato típico: resp[2]=FREQ_H, resp[3]=FREQ_L
  uint16_t f = (uint16_t)((resp[2] << 8) | resp[3]);
  *out_freq = (uint32_t)f;

  return SI4732_OK;
}

si4732_status_t si4732_get_tune_status(si4732_t *dev,
                                       bool clear_int,
                                       uint32_t *out_freq,
                                       uint8_t *out_rssi,
                                       uint8_t *out_snr,
                                       bool *out_stc)
{
  if (!dev || !out_freq)
    return SI4732_ERR_ARG;
  if (!dev->present)
    return SI4732_ERR_DEVICE;

  uint8_t cmd[2];
  uint8_t resp[8] = {0};

  if (dev->mode == SI4732_MODE_FM)
  {
    cmd[0] = 0x22; // FM_TUNE_STATUS
  }
  else
  {
    cmd[0] = 0x42; // AM_TUNE_STATUS
  }
  cmd[1] = clear_int ? 0x01 : 0x00;

  si4732_status_t rc = cmd_write_read(dev, cmd, sizeof(cmd), resp, sizeof(resp), 500);
  if (rc != SI4732_OK)
    return rc;

  dev->last_status = resp[0];

  uint8_t resp1 = resp[1];
  if (out_stc)
    *out_stc = (resp1 & 0x01u) ? 1u : 0u;

  uint16_t f = (uint16_t)((resp[2] << 8) | resp[3]);
  *out_freq = (uint32_t)f;

  if (out_rssi)
    *out_rssi = resp[4];
  if (out_snr)
    *out_snr = resp[5];

  return SI4732_OK;
}

si4732_status_t si4732_dump_tune_status(si4732_t *dev, uint8_t out8[8])
{
  if (!dev || !out8)
    return SI4732_ERR_ARG;
  if (!dev->present)
    return SI4732_ERR_DEVICE;

  uint8_t cmd[2];
  cmd[0] = (dev->mode == SI4732_MODE_FM) ? CMD_FM_TUNE_STATUS : CMD_AM_TUNE_STATUS;
  cmd[1] = 0x00;

  return cmd_write_read(dev, cmd, sizeof(cmd), out8, 8, 800);
}

si4732_status_t wait_stc(si4732_t *dev, uint32_t timeout_ms, bool clear_int)
{
  if (!dev)
    return SI4732_ERR_ARG;

  absolute_time_t deadline = make_timeout_time_ms(timeout_ms);

  while (!time_reached(deadline))
  {

    uint8_t cmd[2];
    uint8_t resp[8] = {0};

    if (dev->mode == SI4732_MODE_FM)
    {
      cmd[0] = 0x22; // FM_TUNE_STATUS
      cmd[1] = clear_int ? 0x01 : 0x00;
    }
    else
    {
      cmd[0] = 0x42; // AM_TUNE_STATUS
      cmd[1] = clear_int ? 0x01 : 0x00;
    }

    si4732_status_t rc = cmd_write_read(dev, cmd, sizeof(cmd), resp, sizeof(resp), 500);
    if (rc != SI4732_OK)
      return rc;

    // resp[0] = STATUS (CTS/ERR)
    // resp[1] = RESP1  (incluye STCINT / STC bit según comando)
    // En Si47xx típicamente STC está en bit0 de resp[1] para TUNE_STATUS.
    //uint8_t resp1 = resp[1];

    // Guardar status “real” observado
    dev->last_status = resp[0];

    //if (resp1 & 0x01u)
    //{
      // STC listo
    //  return SI4732_OK;
    //}

// resp[0] = STATUS
   if (resp[0] & 0x01u) {
      return SI4732_OK; // STCINT
   }
    sleep_ms(5);
  }

  return SI4732_ERR_TIMEOUT;
}

si4732_status_t cmd_write_read(si4732_t *dev,
                               const uint8_t *cmd, size_t cmd_len,
                               uint8_t *resp, size_t resp_len,
                               uint32_t timeout_ms)
{
  if (!dev || !cmd || cmd_len == 0 || !resp || resp_len == 0)
    return SI4732_ERR_ARG;

  // Asegurar que el chip esté listo para aceptar comando
  si4732_status_t rc = wait_cts(dev, timeout_ms);
  if (rc != SI4732_OK)
    return rc;

  rc = i2c_write_bytes(dev, cmd, cmd_len);
  if (rc != SI4732_OK)
    return rc;

  // Esperar CTS nuevamente antes de leer respuesta
  rc = wait_cts(dev, timeout_ms);
  if (rc != SI4732_OK)
    return rc;

  return i2c_read_bytes(dev, resp, resp_len);
}
si4732_status_t si4732_set_am_bandwidth(si4732_t *dev,
                                        si473x_am_bw_t bw,
                                        bool power_line_noise_reject,
                                        uint32_t timeout_ms)
{
  if (!dev) return SI4732_ERR_PARAM;
  if ((uint8_t)bw > 6) return SI4732_ERR_PARAM;
  if (timeout_ms == 0) return SI4732_ERR_PARAM;
  
  // AM_CHANNEL_FILTER: bit8 = AMPLFLT, bits3:0 = AMCHFLT :contentReference[oaicite:3]{index=3}
  
  uint16_t v = (uint16_t)((uint8_t)bw & 0x0F);
  if (power_line_noise_reject) v |= (1u << 8);

  return si4732_set_property(dev, SI473X_PROP_AM_CHANNEL_FILTER, v);
}
