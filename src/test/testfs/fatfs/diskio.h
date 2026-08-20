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

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#include "integer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef BYTE  DSTATUS;

typedef enum {
  RES_OK = 0,
  RES_ERROR,
  RES_WRPRT,
  RES_NOTRDY,
  RES_PARERR
} DRESULT;

#define STA_NOINIT    0x01
#define STA_NODISK    0x02
#define STA_PROTECT   0x04

DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status (BYTE pdrv);
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);

#define CTRL_SYNC           0
#define GET_SECTOR_COUNT    1
#define GET_SECTOR_SIZE     2
#define GET_BLOCK_SIZE      3

#ifdef __cplusplus
}
#endif

#endif
