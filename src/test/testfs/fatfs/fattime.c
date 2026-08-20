/*
 * =======================================================================================
 * diskio
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * Implementation of a rp2040 based controller  of a physical file system (date)
 * =======================================================================================
 * This is mainly an integration effort, the code in this library has been developed 
 * from scratch for this project.
 * However the work received an huge benefit from previous work from many parties,
 * including myself as follows:
 *----------------------------------------------------------------------------
 * Version 1.0
 * - Initial release
 */

#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "ff.h"

//*--- FatFs timestamp:
//*---   bit31:25 Year from 1980 (0..127)
//*---   bit24:21 Month (1..12)
//*---   bit20:16 Day (1..31)
//*---   bit15:11 Hour (0..23)
//*---   bit10:5  Minute (0..59)
//*---   bit4:0   Second/2 (0..29)

DWORD get_fattime(void) {
  datetime_t t;
  if (rtc_get_datetime(&t)) {
    uint32_t year = (t.year >= 1980) ? (t.year - 1980) : 0;
    uint32_t mon  = (t.month >= 1 && t.month <= 12) ? t.month : 1;
    uint32_t day  = (t.day >= 1 && t.day <= 31) ? t.day : 1;
    uint32_t hour = (t.hour <= 23) ? t.hour : 0;
    uint32_t min  = (t.min <= 59) ? t.min : 0;
    uint32_t sec2 = (t.sec <= 59) ? (t.sec / 2) : 0;

    return (DWORD)((year << 25) | (mon << 21) | (day << 16) |
                   (hour << 11) | (min << 5) | (sec2));
  }

  //*--- Really don't care about the date

  return (DWORD)(((2026 - 1980) << 25) | (1 << 21) | (1 << 16));
}
