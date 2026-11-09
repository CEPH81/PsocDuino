/* hw_common.c - relojes, filas digitales y referencias para perif_hw.h */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"

#if PSD_HW
#include "perif_hw.h"

void psd_hw_clocks(void)
{
    PSD_WR1(0xE1, 0x02);      /* OSC_CR1: VC1 = SysClk/1, VC2 = VC1/3 = 8 MHz */
    PSD_WR(0xB3, 0x33);       /* RDI0LT0/LT1: salidas de fila = salida del bloque */
    PSD_WR(0xB4, 0x33);
}

void psd_hw_gs(uint8_t pin)
{
    uint8_t a = (uint8_t)(((pin >> 3) << 2) + 2);   /* PRTxGS */
    PSD_WR(a, psocduino_reg_rd(a) | psocduino_mask(pin));
}

#if PSD_HW_ANALOG
void psd_hw_analog(void)
{
    psd_hw_clocks();
    PSD_WR(0x63, 0x15);       /* ARF_CR: AGND = Vdd/2, RefHi = Vdd, RefLo = Vss; potencia */
    PSD_WR1(0x60, (uint8_t)(psocduino_reg1_rd(0x60) | 0x05)); /* CLK_CR0: columnas 0 y 1 = VC2 */
}

/* DAC6 en binario desplazado (igual que el User Module DAC6): 31 = AGND,
 * 0..30 negativos (ASign), 32..62 positivos. FCap = 32. */
void psd_hw_dac6(uint8_t cr0, uint8_t code)
{
    uint8_t v = (uint8_t)(code >= 31 ? (code - 31) : (0x20 | (31 - code)));
    PSD_WR(cr0, (uint8_t)(0x80 | v));
}
#endif
#endif
