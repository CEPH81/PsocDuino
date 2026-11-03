/* wiring_micros.c - micros() (archivo propio: multiplica en 32 bits) */
#include "Arduino.h"

extern volatile unsigned long psocduino_ms;
extern volatile unsigned int psocduino_us;

unsigned long micros(void)
{
    unsigned long m;
    unsigned int u;
    PSD_GIE_OFF();
    m = psocduino_ms;
    u = psocduino_us;
    PSD_GIE_ON();
    return m * 1000UL + u; /* resolucion = un tic (~2 ms) */
}
