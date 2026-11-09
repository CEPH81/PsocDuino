/* hw_dac.c - DAC6 en ASD11 -> bus analogico 1 -> buffer -> P0_5 (por registros) */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"

#if PSD_HW
#include "perif_hw.h"
#if PSD_HW_ANALOG

void psd_hw_dac_start(void)
{
    psd_hw_analog();
    PSD_WR(0x85, 0x80);       /* ASD11CR1: entrada A = RefHi */
    PSD_WR(0x86, 0xA0);       /* ASD11CR2: salida al bus analogico, autocero */
    PSD_WR(0x87, 0x33);       /* ASD11CR3: ref A = AGND, FSW1, FSW0, potencia alta */
    psd_hw_dac6(0x84, 31);
    PSD_WR1(0x62, (uint8_t)(psocduino_reg1_rd(0x62) | 0x20)); /* ABF_CR0: buffer columna 1 -> P0_5 */
    pinMode(P0_5, INPUT_ANALOG);
}

#endif
#endif
