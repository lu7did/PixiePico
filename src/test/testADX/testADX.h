//*----------------------------------------------------------------------------
//* testADX.c
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
#define DEBUG    1
#define TRACE    1
//#define FT8TEST  1

/*-------------------------------------------------
   IDENTIFICATION DIVISION.
   (just a programmer's joke)
---------------------------------------------------*/
#define PROGNAME "testADX"
#define AUTHOR "Dr. Pedro E. Colla (LU7DZ)"
#define VERSION  "0.1"
#define BUILD     "00"


#ifdef DEBUG
#define _printf(fmt, ...) \
    printf("@[%s] " fmt, __func__, ##__VA_ARGS__)
#endif     


#define GEN_FRQ_HZ    14074000L               /*Generator Frequency (in Hz)*/
#define FT8_BASE_HZ       1000L               /* FT8 base frequency (in Hz) */

char hi[128];

void DDSGenerator(void);
void core1_entry(void);


/*---------------------------------------------------
 * Define band support
*/
#define NBANDS        7
#define NMODES        4
#define SLOT          3

//***********************************************************************************************
//* The following defines the ports used to connect the hard switches (UP/DOWN/TX) and the LED
//* to the rp2040 processor in replacement of the originally used for the ADX Arduino Nano or UNO
//* (see documentation for the hardware schematic and pinout
//***********************************************************************************************
/*----
   RF and signal pin
*/
#define RFOUT          18  //RF Output Enable
#define FSKpin         27  //Frequency counter algorithm, signal input PIN (warning changes must also be done in freqPIO)

/*----
   Output control lines
*/
#define RXSW            2  //RXSW Switch (RX/TX control)

/*---
   LED
*/

#define TX              3  //TX LED
#define FT8             4  //FT8 LED
#define FT4             5  //FT4 LED
#define JS8             6  //JS8 LED
#define WSPR            7  //WSPR LED

/*---
   Calibration signal
*/

#define CAL             9  //Calibration   
/*---
   Switches
*/
#define TXSW            8  //RX-TX Switch
#define UP             10  //UP Switch
#define DOWN           11  //DOWN Switch
#define BEACON         12  //BEACON Jumper
#define SYNC           13  //Time SYNC Switch
