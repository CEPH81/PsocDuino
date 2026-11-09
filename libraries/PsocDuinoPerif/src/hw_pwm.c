/* hw_pwm.c - PWM8 en DBB01 configurado por registros */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"

#if PSD_HW
#include "perif_hw.h"

void psd_hw_pwm_start(void)
{
    psd_hw_clocks();
    PSD_WR(0x27, 0);                                     /* DBB01CR0: parado */
    PSD_WR1(0x24, 0x21);                                 /* DBB01FN: PWM, comparacion <= */
    PSD_WR1(0x25, 0x06);                                 /* DBB01IN: reloj VC2 */
    PSD_WR1(0x26, 0x06);                                 /* DBB01OU: salida de fila 2 */
    PSD_WR(0x25, 0xFF);                                  /* DR1: periodo 255 */
    PSD_WR(0x26, 0x00);                                  /* DR2: ancho */
    PSD_WR(0xB6, psocduino_reg_rd(0xB6) | PSD_HW_PWM_SRO1);  /* RDI0SRO1 -> bus global */
    pinMode(PSD_PWM_PIN, OUTPUT);
    psd_hw_gs(PSD_PWM_PIN);
    PSD_WR(0x27, 0x01);                                  /* arrancar */
}
#endif
