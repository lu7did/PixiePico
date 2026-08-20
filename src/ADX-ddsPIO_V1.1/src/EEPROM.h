/*
 * =======================================================================================
 * EEPROM
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * emulate EEPROM in flash memory 
 * =======================================================================================*/

 //*---------------------------------------------------------------------------------------*
 //*                  Headers, libraries and re-entrancy management                        *
 //*---------------------------------------------------------------------------------------*
 #ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stddef.h>

//*---------------------------------------------------------------------------------------*
//* Define a 256 bytes storage area for the run time configuration 
//*---------------------------------------------------------------------------------------*
//#define EEPROM_SIZE         4096
//#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - EEPROM_SIZE)
#define FLASH_TARGET_OFFSET (1024 * 1024) 
#define FLASH_SECTOR_SIZE    4096 

//*---------------------------------------------------------------------------------------*
//* Define a 256 bytes storage area for the run time configuration 
//*---------------------------------------------------------------------------------------*
typedef struct {
    uint8_t ID;
    uint8_t mode;
    uint8_t Band_slot;
    uint8_t reserved[125];
} EEPROMData;

//*---------------------------------------------------------------------------------------*
//*                              Prototypes 
//*---------------------------------------------------------------------------------------*
void EEPROM_read(EEPROMData *dest);
void EEPROM_write(const EEPROMData *src);
void EEPROM_reset();


#endif
