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
#include "ADX-ddsPIO.h"

//*---------------------------------------------------------------------------------------*
//* Define a 256 bytes storage area for the run time configuration 
//*---------------------------------------------------------------------------------------*
#define FLASH_TARGET_OFFSET (1024 * 1024) 
#define FLASH_SECTOR_SIZE    4096 



//*---------------------------------------------------------------------------------------*
//*                              Prototypes 
//*---------------------------------------------------------------------------------------*
void EEPROM_read(ADX_ddsPIO_t *dest);
void EEPROM_write(const ADX_ddsPIO_t *src);
void EEPROM_reset();


#endif
