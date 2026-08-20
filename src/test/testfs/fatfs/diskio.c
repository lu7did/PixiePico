
/*
 * =======================================================================================
 * diskio
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * Implementation of a rp2040 based controller  of a physical file system 
 * =======================================================================================
 * This is mainly an integration effort, the code in this library has been developed 
 * from scratch for this project.
 * However the work received an huge benefit from previous work from many parties,
 * including myself as follows:
 *----------------------------------------------------------------------------
 * Version 1.0
 * - Initial release
 */

#include "diskio.h"
#include "ffconf.h"
#include <string.h>
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "flash_bd.h"         
#include "hardware/flash.h"  


#define PDRV  0
static DSTATUS stat = STA_NOINIT;

//*--- Implement physical level primitives

DSTATUS disk_initialize (BYTE pdrv) {
  if (pdrv != PDRV) return STA_NOINIT;
  if (!flash_bd_init()) return STA_NOINIT;
  stat = 0;
  return stat;
}


DSTATUS disk_status (BYTE pdrv) {
  if (pdrv != PDRV) return STA_NOINIT;
  return stat;
}


DRESULT disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
  if (pdrv != PDRV) return RES_PARERR;
  return flash_bd_read_blocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
  if (pdrv != PDRV) return RES_PARERR;
  return flash_bd_write_blocks(sector, buff, count) ? RES_OK : RES_ERROR;
}


DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
  if (pdrv != 0) return RES_PARERR;

  switch (cmd) {
    case CTRL_SYNC:        return RES_OK;
    case GET_SECTOR_COUNT: *(DWORD*)buff = (DWORD)ADX_MSC_SECTOR_COUNT; return RES_OK;
    case GET_SECTOR_SIZE:  *(WORD*)buff  = (WORD)ADX_MSC_SECTOR_SIZE;  return RES_OK;
    case GET_BLOCK_SIZE: *(DWORD*)buff = (FLASH_SECTOR_SIZE / ADX_MSC_SECTOR_SIZE); return RES_OK;  // 8
    default:               return RES_PARERR;
  }
}
