/* wiring_digital.c - pinMode / digitalWrite / digitalRead para PSoC 1
 *
 * Modos de manejo (PRTxDM2:DM1:DM0) segun el TRM del PSoC 1:
 *   000 pull-down resistivo   001 fuerte          010 alta Z digital
 *   011 pull-up resistivo     110 alta Z analogica 111 drenador abierto (bajo)
 * En los modos resistivos la resistencia actua segun el bit de datos, por eso
 * se usa un registro sombra: leer PRTxDR devuelve el pin, no el latch. */
#include "Arduino.h"

static uint8_t shadow[6];

static void dr_write(uint8_t port, uint8_t v)
{
    shadow[port] = v;
    switch (port) {
    case 0: PSD_PRT0DR = v; break;
    case 1: PSD_PRT1DR = v; break;
    case 2: PSD_PRT2DR = v; break;
    case 3: PSD_PRT3DR = v; break;
    case 4: PSD_PRT4DR = v; break;
    case 5: PSD_PRT5DR = v; break;
    }
}

static uint8_t dr_read(uint8_t port)
{
    switch (port) {
    case 0: return PSD_PRT0DR;
    case 1: return PSD_PRT1DR;
    case 2: return PSD_PRT2DR;
    case 3: return PSD_PRT3DR;
    case 4: return PSD_PRT4DR;
    case 5: return PSD_PRT5DR;
    }
    return 0;
}

/* aplica un bit de los tres registros de modo; dm = DM2:DM1:DM0 */
static void dm_write(uint8_t port, uint8_t mask, uint8_t dm)
{
    uint8_t keep = (uint8_t)~mask;
    uint8_t b0 = (dm & 1) ? mask : 0;
    uint8_t b1 = (dm & 2) ? mask : 0;
    uint8_t b2 = (dm & 4) ? mask : 0;
    switch (port) {
    case 0: PSD_PRT0DM2 = (uint8_t)((PSD_PRT0DM2 & keep) | b2); break;
    case 1: PSD_PRT1DM2 = (uint8_t)((PSD_PRT1DM2 & keep) | b2); break;
    case 2: PSD_PRT2DM2 = (uint8_t)((PSD_PRT2DM2 & keep) | b2); break;
    case 3: PSD_PRT3DM2 = (uint8_t)((PSD_PRT3DM2 & keep) | b2); break;
    case 4: PSD_PRT4DM2 = (uint8_t)((PSD_PRT4DM2 & keep) | b2); break;
    case 5: PSD_PRT5DM2 = (uint8_t)((PSD_PRT5DM2 & keep) | b2); break;
    }
    PSD_BANK1();
    switch (port) {
    case 0: PSD_PRT0DM0 = (uint8_t)((PSD_PRT0DM0 & keep) | b0);
            PSD_PRT0DM1 = (uint8_t)((PSD_PRT0DM1 & keep) | b1); break;
    case 1: PSD_PRT1DM0 = (uint8_t)((PSD_PRT1DM0 & keep) | b0);
            PSD_PRT1DM1 = (uint8_t)((PSD_PRT1DM1 & keep) | b1); break;
    case 2: PSD_PRT2DM0 = (uint8_t)((PSD_PRT2DM0 & keep) | b0);
            PSD_PRT2DM1 = (uint8_t)((PSD_PRT2DM1 & keep) | b1); break;
    case 3: PSD_PRT3DM0 = (uint8_t)((PSD_PRT3DM0 & keep) | b0);
            PSD_PRT3DM1 = (uint8_t)((PSD_PRT3DM1 & keep) | b1); break;
    case 4: PSD_PRT4DM0 = (uint8_t)((PSD_PRT4DM0 & keep) | b0);
            PSD_PRT4DM1 = (uint8_t)((PSD_PRT4DM1 & keep) | b1); break;
    case 5: PSD_PRT5DM0 = (uint8_t)((PSD_PRT5DM0 & keep) | b0);
            PSD_PRT5DM1 = (uint8_t)((PSD_PRT5DM1 & keep) | b1); break;
    }
    PSD_BANK0();
}

void psocduino_gpio_init(void)
{
    uint8_t p;
    for (p = 0; p < PSOC_NUM_PORTS && p < 6; p++) shadow[p] = dr_read(p);
}

void pinMode(uint8_t pin, uint8_t mode)
{
    uint8_t port = (uint8_t)(pin >> 3), mask = (uint8_t)(1 << (pin & 7)), dm;
    if (port >= PSOC_NUM_PORTS) return;
    switch (mode) {
    case OUTPUT:            dm = 1; break;
    case INPUT_PULLUP:      dm = 3; dr_write(port, (uint8_t)(shadow[port] | mask)); break;
    case INPUT_PULLDOWN:    dm = 0; dr_write(port, (uint8_t)(shadow[port] & ~mask)); break;
    case OUTPUT_OPEN_DRAIN: dm = 7; break;
    case INPUT_ANALOG:      dm = 6; break;
    default:                dm = 2; break; /* INPUT */
    }
    dm_write(port, mask, dm);
}

void digitalWrite(uint8_t pin, uint8_t val)
{
    uint8_t port = (uint8_t)(pin >> 3), mask = (uint8_t)(1 << (pin & 7));
    if (port >= PSOC_NUM_PORTS) return;
    if (val) dr_write(port, (uint8_t)(shadow[port] | mask));
    else dr_write(port, (uint8_t)(shadow[port] & ~mask));
}

void digitalToggle(uint8_t pin)
{
    uint8_t port = (uint8_t)(pin >> 3), mask = (uint8_t)(1 << (pin & 7));
    if (port >= PSOC_NUM_PORTS) return;
    dr_write(port, (uint8_t)(shadow[port] ^ mask));
}

int digitalRead(uint8_t pin)
{
    uint8_t port = (uint8_t)(pin >> 3), mask = (uint8_t)(1 << (pin & 7));
    if (port >= PSOC_NUM_PORTS) return LOW;
    return (dr_read(port) & mask) ? HIGH : LOW;
}

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (bitOrder == LSBFIRST) digitalWrite(dataPin, (uint8_t)((val >> i) & 1));
        else digitalWrite(dataPin, (uint8_t)((val >> (7 - i)) & 1));
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}

uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder)
{
    uint8_t v = 0, i;
    for (i = 0; i < 8; i++) {
        digitalWrite(clockPin, HIGH);
        if (bitOrder == LSBFIRST) v |= (uint8_t)(digitalRead(dataPin) << i);
        else v |= (uint8_t)(digitalRead(dataPin) << (7 - i));
        digitalWrite(clockPin, LOW);
    }
    return v;
}

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
