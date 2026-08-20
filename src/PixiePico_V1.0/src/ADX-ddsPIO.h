/*
 * =======================================================================================
 * ADX-ddsPIO
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * new generation rp2040 ADX based digital transceiver 
 * 
 * This is mainly an integration effort with some new code developed for this project,
 * some unique features has been developed for this firmware as well such as the
 * quadrature digital frequency synth.
 *----------------------------------------------------------------------------------------*/

 //*---------------------------------------------------------------------------------------*
 //*                  Headers, libraries and re-entrancy management                        *
 //*---------------------------------------------------------------------------------------*
 #ifndef ADX_ddsPIO_H
#define ADX_ddsPIO_H


//*---------------------------------------------------------------------------------------*
//* Define a 128 bytes storage area for the run time configuration 
//*---------------------------------------------------------------------------------------*
typedef struct {
    uint8_t  ID;
    uint8_t  mode;
    uint8_t  slot;
    uint8_t  volume;
    uint32_t frqFT8;
    uint32_t frqbfo;
    uint8_t  bw;
    uint8_t  reserved[115];
} ADX_ddsPIO_t;

#endif
