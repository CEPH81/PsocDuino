/* psocduino_perif.h - CY8C27443: perifericos por registros (biblioteca
 * PsocDuinoPerif, ver perif_hw.h). Sin User Modules: solo patas y enrutado. */
#ifndef PSOCDUINO_PERIF_H
#define PSOCDUINO_PERIF_H

#define PSD_HW 1
#define PSD_HW_ANALOG 1   /* ADC SAR 6 bits (P0_1/P0_3/P0_5/P0_7) y DAC6 en P0_5 */
#define PSD_UART_TX_PIN  P2_1   /* fila 0 salida 1 -> GlobalOutEven_1 */
#define PSD_UART_RX_PIN  P2_0   /* GlobalInEven_0 -> fila 0 entrada 0 */
#define PSD_HW_TX_OU     0x05
#define PSD_HW_TX_SRO0   0x10
#define PSD_HW_RX_IN     0xC1
#define PSD_HW_RI        0x00
#define PSD_HW_RI_KEEP   0xFC
#define PSD_PWM_PIN      P1_6   /* fila 0 salida 2 -> GlobalOutOdd_6 */
#define PSD_HW_PWM_SRO1  0x08

#endif
