/*
 * =======================================================================================
 * flash_bd
 * 
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
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef ADX_FLASH_EEPROM_OFFSET
  #define ADX_FLASH_EEPROM_OFFSET   (1024u * 1024u)   // Reserve EEPROM
#endif

#ifndef ADX_FLASH_EEPROM_SIZE
  #define ADX_FLASH_EEPROM_SIZE     (4096u)           // 4KB
#endif

//*--- Logical MSC/FATFS
#ifndef ADX_MSC_SECTOR_SIZE
  #define ADX_MSC_SECTOR_SIZE       512u
#endif

//*--- Total MSC disk size
//*--- Making it smaller creates unstability, at least on the MacOS.
#ifndef ADX_FLASH_FS_SIZE
  #define ADX_FLASH_FS_SIZE         (2u * 1024u * 1024u)  // 2MB=Stability compromise
#endif

//*--- FS Offset in flash memory, following the EEPROM
#ifndef FLASH_SECTOR_SIZE
  #define FLASH_SECTOR_SIZE         4096u
#endif

#define ADX_ALIGN_UP(x, a)          (((x) + ((a)-1u)) & ~((a)-1u))
#define ADX_FLASH_FS_OFFSET         ADX_ALIGN_UP((ADX_FLASH_EEPROM_OFFSET + ADX_FLASH_EEPROM_SIZE), FLASH_SECTOR_SIZE)
#define ADX_MSC_SECTOR_COUNT        (ADX_FLASH_FS_SIZE / ADX_MSC_SECTOR_SIZE)

// API block device
bool     flash_bd_init(void);
uint32_t flash_bd_block_count(void);
uint32_t flash_bd_block_size(void);

bool flash_bd_read_blocks(uint32_t lba, void* buffer, uint32_t block_count);
bool flash_bd_write_blocks(uint32_t lba, const void* buffer, uint32_t block_count);

//*--- required for clean formatting
void flash_bd_erase_all(void);
