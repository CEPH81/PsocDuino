/* wiring_millis.c - millis() (archivo propio: solo se enlaza si se usa) */
#include "Arduino.h"

extern volatile unsigned long psocduino_ms;

unsigned long millis(void)
{
    unsigned long m;
    PSD_GIE_OFF();
    m = psocduino_ms;
    PSD_GIE_ON();
    return m;
}
