/*
 * =======================================================================================
 * usb_msc
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
#include "tusb.h"
#include "fs.h" 


//*--- Callback entries required for TinyUSB to work

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
  (void) lun;
  const char vid[] = "LU7DZ   ";
  const char pid[] = "ADX MSC+FATFS     ";
  const char rev[] = "1.0 ";
  memcpy(vendor_id, vid, 8);
  memcpy(product_id, pid, 16);
  memcpy(product_rev, rev, 4);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
  (void) lun;
  return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
  (void) lun;
  *block_size  = ADX_MSC_SECTOR_SIZE;
  *block_count = ADX_MSC_SECTOR_COUNT;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
  (void) lun; (void) power_condition; (void) start; (void) load_eject;
  return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  (void) lun;
  static uint8_t sec[ADX_MSC_SECTOR_SIZE];
  if (offset + bufsize > ADX_MSC_SECTOR_SIZE) return -1;
  if (!flash_bd_read_blocks(lba, sec, 1)) return -1;
  memcpy(buffer, sec + offset, bufsize);
  return (int32_t) bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  (void) lun;
  static uint8_t sec[ADX_MSC_SECTOR_SIZE];

  if (offset + bufsize > ADX_MSC_SECTOR_SIZE) return -1;

  if (!flash_bd_read_blocks(lba, sec, 1)) return -1;
  memcpy(sec + offset, buffer, bufsize);
  if (!flash_bd_write_blocks(lba, sec, 1)) return -1;
  return (int32_t) bufsize;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
  (void) lun; (void) buffer; (void) bufsize;
  (void) scsi_cmd;
  return 0;
}
