/*
Project PixiePico

Pixie based digital transceiver firmware
Test firmware, basic GUI workbench and verification
DDS algorithm

Copyright Dr. Pedro E. Colla LU7DZ (2026)
For non-profit uses only
================================================================
Este programa  disponible es hecho público
bajo la licencia Creative Commons Attribution-ShareAlike 4.0
International (CC BY-SA 4.0).

*/


//*---------------------------------------------------------------------------------------*
 //*                  Headers, libraries and re-entrancy management                        *
 //*---------------------------------------------------------------------------------------*

//*==============================================================================================*
//*                                  Prototypes                                                  *
//*==============================================================================================*
// void adc_drain();

void transmitting(void);
void receiving(void);
void audio_data_write(int16_t,int16_t);

