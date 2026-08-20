//*----------------------------------------------------------------------------
//* macros.h
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

#ifdef DEBUG
#define _PRINTF(fmt, ...) \
    printf("@ " fmt, ##__VA_ARGS__)
#endif     