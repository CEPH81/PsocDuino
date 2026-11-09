/* hw_adc.c - ADC SAR de 6 bits por software (idea del SAR6 de Cypress, AN2239):
 * DAC6 en ASC10 -> bus analogico 0 (con muestreo y retencion) -> entrada + del
 * comparador ACB00; entrada - = pata elegida con AMX_IN (columna 0). La CPU hace
 * la busqueda binaria leyendo el comparador en CMP_CR0 bit 4. Sin bloques
 * digitales, sin RAM extra, sin interrupciones. */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"

#if PSD_HW
#include "perif_hw.h"
#if PSD_HW_ANALOG

static uint8_t sar_on;

unsigned int psd_hw_sar(void)
{
    uint8_t code = 0, bit = 32, t;
    if (!sar_on) {
        psd_hw_analog();
        PSD_WR(0x81, 0x40);   /* ASC10CR1: entrada A = RefHi */
        PSD_WR(0x82, 0xA0);   /* ASC10CR2: salida al bus analogico, autocero */
        PSD_WR(0x83, 0x33);   /* ASC10CR3: ref A = AGND, FSW1, FSW0, potencia alta */
        PSD_WR(0x71, 0x01);   /* ACB00CR0: resistencias a AGND (sin realimentacion) */
        PSD_WR(0x72, 0x7E);   /* ACB00CR1: bus de comparador, - = pata, + = ABUS0 */
        PSD_WR(0x73, 0x03);   /* ACB00CR2: modo comparador, potencia alta */
        PSD_WR(0x70, 0x00);
        sar_on = 1;
    }
    do {
        t = (uint8_t)(code | bit);
        if (t <= 62) {
            psd_hw_dac6(0x80, t);
            delayMicroseconds(10);                       /* varios ciclos de columna */
            if (!(psocduino_reg_rd(0x64) & 0x10)) code = t;   /* DAC < pata */
        }
        bit >>= 1;
    } while (bit);
    /* 0..62 -> 0..4092 (12 bits): code * 66 sin multiplicar */
    return (unsigned int)(((unsigned int)code << 6) + ((unsigned int)code << 1));
}

#endif
#endif
