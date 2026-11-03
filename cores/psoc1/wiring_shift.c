/* wiring_shift.c - shiftOut / shiftIn (archivo propio: solo se enlaza si se usa) */
#include "Arduino.h"

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (bitOrder == LSBFIRST) { digitalWrite(dataPin, (uint8_t)(val & 1)); val >>= 1; }
        else { digitalWrite(dataPin, (uint8_t)((val & 0x80) ? 1 : 0)); val <<= 1; }
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}

uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder)
{
    uint8_t v = 0, i;
    for (i = 0; i < 8; i++) {
        digitalWrite(clockPin, HIGH);
        if (bitOrder == LSBFIRST) { v >>= 1; if (digitalRead(dataPin)) v |= 0x80; }
        else { v <<= 1; if (digitalRead(dataPin)) v |= 1; }
        digitalWrite(clockPin, LOW);
    }
    return v;
}
