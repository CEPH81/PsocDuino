/* wiring_read.c - digitalRead (archivo propio: solo se enlaza si se usa) */
#include "Arduino.h"

int digitalRead(uint8_t pin)
{
    uint8_t port = (uint8_t)(pin >> 3);
    if (port >= PSOC_NUM_PORTS) return LOW;
    return (psocduino_reg_rd((uint8_t)(port << 2)) & psocduino_mask(pin)) ? HIGH : LOW;
}
