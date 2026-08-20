//*----------------------------------------------------------------------------
//* testDDS.c
//*----------------------------------------------------------------------------
//* Testbed for the ADX DDS PIO-based frequency synthesizer FT8 Transceiver
//* using Raspberry Pi Pico.
//* 
//* This is a working module of an integration effort to build an FT8 
//* transceiver using Raspberry Pi Pico as the main controller and signal
//* generator.
//* 
//* Copyright (c) 2025 by Pedro Colla (LU7DZ)
//*----------------------------------------------------------------------------
//* Largely based on the work of Roman Piksaykin (R2BDY):
//* Digital Controlled Oscillator for Raspberry Pi Pico
//* https://www.qrz.com/db/r2bdy        
//*----------------------------------------------------------------------------


/*-------------------------------------------------
   IDENTIFICATION DIVISION.
   (just a programmer's joke)
---------------------------------------------------*/
#define PROGNAME "testDDS"
#define AUTHOR "Dr. Pedro E. Colla (LU7DZ)"
#define VERSION  "0.1"
#define BUILD     "00"

#define DEBUG    1

#define GEN_FRQ_HZ 14074000L               /*Generator Frequency (in Hz)*/

char hi[128];

void DDSGenerator(void);
void core1_entry(void);

#ifdef DEBUG 
#define _INFOLIST(...) \
  do { \
    strcpy(hi,"@"); \
    sprintf(hi+1,__VA_ARGS__); \
    printf("%s",hi); \
    fflush(stdout); \
  } while (false)
#else //!DEBUG
#define _INFOLIST(...) (void)0
#endif //DEBUG
