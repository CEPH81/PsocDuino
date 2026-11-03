/* wiring_pulse.c - pulseIn (archivo propio: solo se enlaza si se usa) */
#include "Arduino.h"

unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout)
{
    unsigned long start = micros(), t0;
    uint8_t want = state ? HIGH : LOW;
    if (!timeout) timeout = 1000000UL;
    while (digitalRead(pin) == want) if (micros() - start > timeout) return 0;
    while (digitalRead(pin) != want) if (micros() - start > timeout) return 0;
    t0 = micros();
    while (digitalRead(pin) == want) if (micros() - start > timeout) return 0;
    return micros() - t0;
}
