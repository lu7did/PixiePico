/*
 * =======================================================================================
 * EEPROM
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * emulate EEPROM in flash memory 
 * =======================================================================================*/

 //*---------------------------------------------------------------------------------------*
 //*                                   includes                                            *
 //*---------------------------------------------------------------------------------------*
#include "EEPROM.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

 //*---------------------------------------------------------------------------------------*
 //* read from memory                                                                      *
 //*---------------------------------------------------------------------------------------*
void EEPROM_read(ADX_ddsPIO_t *dest) {

    //*--- read can be made just mapping to the memory address

    memcpy(dest, flash_target_contents, sizeof(ADX_ddsPIO_t));
}

 //*---------------------------------------------------------------------------------------*
 //* Write to memory                                                                       *
 //*---------------------------------------------------------------------------------------*
void EEPROM_write(const ADX_ddsPIO_t *src) {

    //*--- SDK requires an aligned buffer 

    uint8_t buffer[FLASH_SECTOR_SIZE];
    memset(buffer, 0xFF, FLASH_SECTOR_SIZE); // Fill with 0xff by default
    memcpy(buffer, src, sizeof(ADX_ddsPIO_t));

    //*--- Suspend interrupts
    uint32_t ints = save_and_disable_interrupts();
    
    //*--- Erase sector
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    
    //*--- Write structure and resume interrups
    flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

//*---------------------------------------------------------------------------------------*
//* reset EEPROM                                                                          *
//*---------------------------------------------------------------------------------------*
void EEPROM_reset() {

    //*--- suspend interrupts
    
    uint32_t ints = save_and_disable_interrupts();

    //*--- Erase area and restore interrupts

    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}


