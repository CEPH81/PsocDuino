/* perif_hw.h - perifericos configurados por registros (sin proyecto de PSoC Designer)
 * para las placas SIN "+ perifericos". Lo incluye la biblioteca cuando la variante
 * define PSD_HW (su psocduino_perif.h solo indica las patas de cada chip).
 *
 * Bloques que usa (mismos valores que los proyectos Base2 verificados):
 *   PWM8   DBB01, reloj VC2 (8 MHz), periodo 255 -> 31,25 kHz, salida de fila 2
 *   UART   TX en DCB02, RX en DCB03, reloj VC3 (Serial.begin elige la velocidad)
 *   ADC    SAR de 6 bits por software: DAC6 en ASC10 -> ABUS0 -> comparador ACB00
 *          contra la pata (multiplexor de la columna 0: P0_1, P0_3, P0_5, P0_7)
 *   DAC    DAC6 en ASD11 -> ABUS1 -> buffer -> P0_5
 * Registros segun el PSoC TRM (001-14463). Sin interrupciones: todo por sondeo. */
#ifndef PERIF_HW_H
#define PERIF_HW_H

void psd_hw_clocks(void);                 /* VC1 = SysClk, VC2 = 8 MHz; filas LUT */
void psd_hw_gs(uint8_t pin);              /* conecta la pata a su bus global */
void psd_hw_analog(void);                 /* referencias y relojes de columna */
void psd_hw_dac6(uint8_t cr0, uint8_t code); /* code 0..62 (binario desplazado) */

#define PSD_HAS_UART 1
void psd_hw_uart_start(void);
#define PSD_UART_START(p)     psd_hw_uart_start()
#define PSD_UART_STOP()       { PSD_WR(0x2B, 0); PSD_WR(0x2F, 0); }
#define PSD_UART_RXSTAT()     psocduino_reg_rd(0x2F)          /* DCB03CR0 */
#define PSD_UART_RXDATA()     psocduino_reg_rd(0x2E)          /* DCB03DR2 */
#define PSD_UART_TXSTAT()     psocduino_reg_rd(0x2B)          /* DCB02CR0 */
#define PSD_UART_SEND(b)      PSD_WR(0x29, (b))               /* DCB02DR1 */
#define PSD_UART_PARITY_NONE  0
#define PSD_UART_RX_COMPLETE  0x08
#define PSD_UART_TX_EMPTY     0x10
#define PSD_UART_TX_COMPLETE  0x20

#define PSD_HAS_PWM 1
void psd_hw_pwm_start(void);
#define PSD_PWM_START()       psd_hw_pwm_start()
#define PSD_PWM_WRITE(v)      PSD_WR(0x26, (v))               /* DBB01DR2 */

#if PSD_HW_ANALOG
#define PSD_HAS_ADC 1
#define PSD_ADC_SAR 1
#define PSD_ADC_BITS 12                    /* 6 bits reales escalados a 12 */
unsigned int psd_hw_sar(void);
#define PSD_HAS_DAC 1
void psd_hw_dac_start(void);
#define PSD_DAC_START()       psd_hw_dac_start()
#define PSD_DAC_WRITE(v)      psd_hw_dac6(0x84, (uint8_t)((v) >= 246 ? 62 : (((v) + 2) >> 2)))
#endif

#endif
