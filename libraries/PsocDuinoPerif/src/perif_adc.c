/* perif_adc.c - analogRead con ADCINC + PGA (placas "+ perifericos", 12 bits) o
 * con el SAR de 6 bits por registros (demas placas, ver hw_adc.c), columna 0.
 * La entrada se elige con AMX_IN (banco 0, 60h), bits 1-0: P0_1, P0_3, P0_5, P0_7.
 * P0_5 es la salida del DAC: se puede leer, pero normalmente se usan P0_1, P0_3 y P0_7. */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"
#if PSD_HW
#include "perif_hw.h"
#endif

#if PSD_HAS_ADC

#if !PSD_ADC_SAR
static unsigned char adc_on;
#endif
static unsigned char adc_shift = PSD_ADC_BITS - 10;

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
#if !PSD_ADC_SAR
    if (!adc_on) {
        PSD_ADC_START();
        adc_on = 1;
    }
#endif
    pinMode(pin, INPUT_ANALOG);
    /* P0_1 -> 0, P0_3 -> 1, P0_5 -> 2, P0_7 -> 3 */
    PSD_WR(0x60, (psocduino_reg_rd(0x60) & 0xFC) | (pin >> 1));
#if PSD_ADC_SAR
    (void)t;
    v = psd_hw_sar();                    /* SAR de 6 bits por registros */
#else
    PSD_ADC_SAMPLE();
    while (!PSD_ADC_READY())
        if (++t == 0) return 0;          /* sin interrupciones del ADC: no colgarse */
    v = PSD_ADC_GET();
#endif
    return (int)(v >> adc_shift);
}

#else
/* este chip no tiene el bloque necesario (CY8C21x34: sin DAC ni SAR) */
void analogReadResolution(uint8_t bits) { (void)bits; }
int analogRead(uint8_t pin) { (void)pin; return 0; }
#endif
