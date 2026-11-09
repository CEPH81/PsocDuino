/* hw_uart.c - UART en DCB02 (TX) y DCB03 (RX) configurado por registros */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"

#if PSD_HW
#include "perif_hw.h"

void psd_hw_uart_start(void)
{
    psd_hw_clocks();
    PSD_WR(0x2B, 0); PSD_WR(0x2F, 0);                    /* bloques parados */
    PSD_WR1(0x28, 0x0D);                                 /* DCB02FN: UART TX */
    PSD_WR1(0x29, 0x01);                                 /* DCB02IN: reloj VC3 */
    PSD_WR1(0x2A, PSD_HW_TX_OU);                         /* DCB02OU: salida de fila */
    PSD_WR1(0x2C, 0x05);                                 /* DCB03FN: UART RX */
    PSD_WR1(0x2D, PSD_HW_RX_IN);                         /* DCB03IN: datos de fila, VC3 */
    PSD_WR1(0x2E, 0x00);
    PSD_WR(0xB5, psocduino_reg_rd(0xB5) | PSD_HW_TX_SRO0);   /* RDI0SRO0 -> bus global */
    PSD_WR(0xB0, (uint8_t)((psocduino_reg_rd(0xB0) & PSD_HW_RI_KEEP) | PSD_HW_RI)); /* RDI0RI */
    pinMode(PSD_UART_TX_PIN, OUTPUT);
    pinMode(PSD_UART_RX_PIN, INPUT_PULLUP);
    psd_hw_gs(PSD_UART_TX_PIN);
    psd_hw_gs(PSD_UART_RX_PIN);
    PSD_WR(0x2B, 0x01);                                  /* arrancar TX y RX, 8N1 */
    PSD_WR(0x2F, 0x01);
}
#endif
