/* wiring_micros.c - micros() (archivo propio: multiplica en 32 bits) */
#include "Arduino.h"

unsigned long psocduino_micros_parts(unsigned int *us);

unsigned long micros(void)
{
    unsigned int u;
    unsigned long m = psocduino_micros_parts(&u);
    return m * 1000UL + u; /* resolucion = un tic (~2 ms) */
}
