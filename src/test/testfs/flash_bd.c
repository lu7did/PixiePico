/*
 * =======================================================================================
 * flash_bd.c
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

#include "flash_bd.h"
#include <string.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#ifndef XIP_BASE
#define XIP_BASE 0x10000000u
#endif

//*--- physical implementation

static inline uint32_t lba_to_flash_addr(uint32_t lba) {
  return ADX_FLASH_FS_OFFSET + lba * ADX_MSC_SECTOR_SIZE;
}

bool flash_bd_init(void) {
  return true;
}
bool flash_bd_read_blocks(uint32_t lba, void* dst, uint32_t count) {
  if (!dst || count == 0) return false;
  if ((uint64_t)lba + (uint64_t)count > (uint64_t)ADX_MSC_SECTOR_COUNT) return false;
  uint32_t flash_addr = lba_to_flash_addr(lba);
  const uint8_t* src = (const uint8_t*)(XIP_BASE + flash_addr);
  memcpy(dst, src, (size_t)count * ADX_MSC_SECTOR_SIZE);
  return true;
}

static bool program_rmw_4k(uint32_t flash_addr, const uint8_t* data512, uint32_t offset_in_4k) {
  //*--- flash_addr: exact address where the 512 block will sit within the FS
  //*--- offset_in_4k: offset within the 4KB sector (0..3584), must be a 512 multiple
  //*--- data512: buffer 512 bytes to wrire

  //*--- Align with 4KB boundary
  uint32_t base4k = flash_addr & ~(FLASH_SECTOR_SIZE - 1u);

  //*--- Read the 4KB sector complete from XIP
  uint8_t tmp[FLASH_SECTOR_SIZE];
  const uint8_t* cur = (const uint8_t*)(XIP_BASE + base4k);
  memcpy(tmp, cur, FLASH_SECTOR_SIZE);

  //*--- Modify only the chunk under scope
  memcpy(tmp + offset_in_4k, data512, ADX_MSC_SECTOR_SIZE);

  //*--- Finalize and Clean up
  
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(base4k, FLASH_SECTOR_SIZE);
  flash_range_program(base4k, tmp, FLASH_SECTOR_SIZE);
  restore_interrupts(ints);

  return true;
}

bool flash_bd_write_blocks(uint32_t lba, const void* src, uint32_t count) {
  if (!src || count == 0) return false;
  if ((uint64_t)lba + (uint64_t)count > (uint64_t)ADX_MSC_SECTOR_COUNT) return false;

  //*--- Writing is made in 512 increments withn the 4KB sector
  for (uint32_t i = 0; i < count; i++) {
    uint32_t cur_lba = lba + i;
    uint32_t flash_addr = lba_to_flash_addr(cur_lba);

    //*--- Offset within the 4KB segment according to LBA (each LBA=512, 8 LBA allowed)
    uint32_t offset_in_4k = (flash_addr & (FLASH_SECTOR_SIZE - 1u));

    //*--- Ensure aligment within a 512 sector boundary
    if ((offset_in_4k % ADX_MSC_SECTOR_SIZE) != 0) return false;

    if (!program_rmw_4k(flash_addr, src + i * ADX_MSC_SECTOR_SIZE, offset_in_4k)) {
      return false;
    }
  }

  return true;
}

void flash_bd_erase_all(void) {
  
  //*--- Erase entire FS using 4KB multiples
  uint32_t bytes = ADX_MSC_SECTOR_COUNT * ADX_MSC_SECTOR_SIZE;
  uint32_t erase_len = (bytes + FLASH_SECTOR_SIZE - 1u) & ~(FLASH_SECTOR_SIZE - 1u);

  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(ADX_FLASH_FS_OFFSET, erase_len);
  restore_interrupts(ints);
}
