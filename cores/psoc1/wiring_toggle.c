/* wiring_toggle.c - digitalToggle (archivo propio: solo se enlaza si se usa) */
#include "Arduino.h"

void digitalToggle(uint8_t pin)
{
    uint8_t port = (uint8_t)(pin >> 3), v;
    if (port >= PSOC_NUM_PORTS) return;
    v = (uint8_t)(psocduino_shadow[port] ^ psocduino_mask(pin));
    psocduino_shadow[port] = v;
    PSD_WR(port << 2, v);
}
