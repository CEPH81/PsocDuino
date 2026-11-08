/* perif_adc.c - analogRead con ADCINC + PGA (columna analogica 0).
 * La entrada se elige con AMX_IN (banco 0, 60h), bits 1-0: P0_1, P0_3, P0_5, P0_7.
 * P0_5 es la salida del DAC: se puede leer, pero normalmente se usan P0_1, P0_3 y P0_7. */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"

#if PSD_HAS_ADC

static unsigned char adc_on, adc_shift = PSD_ADC_BITS - 10;

void analogReadResolution(uint8_t bits)
{
    if (bits < 6) bits = 6;
    if (bits > PSD_ADC_BITS) bits = PSD_ADC_BITS;
    adc_shift = (unsigned char)(PSD_ADC_BITS - bits);
}

int analogRead(uint8_t pin)
{
    unsigned int t = 0, v;
    if (pin != P0_1 && pin != P0_3 && pin != P0_5 && pin != P0_7) return 0;
    if (!adc_on) {
        PSD_ADC_START();
        adc_on = 1;
    }
    pinMode(pin, INPUT_ANALOG);
    /* P0_1 -> 0, P0_3 -> 1, P0_5 -> 2, P0_7 -> 3 */
    PSD_WR(0x60, (psocduino_reg_rd(0x60) & 0xFC) | (pin >> 1));
    PSD_ADC_SAMPLE();
    while (!PSD_ADC_READY())
        if (++t == 0) return 0;          /* sin interrupciones del ADC: no colgarse */
    v = PSD_ADC_GET();
    return (int)(v >> adc_shift);
}

#endif
