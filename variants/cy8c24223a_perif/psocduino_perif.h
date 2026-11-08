/* psocduino_perif.h - User Modules de la variante cy8c24223a_perif
 * (proyecto Base2_cy8c24223A de PSoC Designer). Lo usa la biblioteca PsocDuinoPerif.
 *   UART   DCB02/DCB03  TX P1_4, RX P1_2  (VC3; Serial.begin cambia la velocidad)
 *   PWM8   DBB01        P1_6              (VC2 8 MHz, periodo 255 -> 31,25 kHz)
 *   ADCINC DBB00+ASC10+ASD20 con PGA en ACB00, 12 bits, entradas P0_1/P0_3/P0_7
 *   DAC8   ASD11+ASC21  P0_5              (8 bits, binario desplazado 0..255)
 */
#ifndef PSOCDUINO_PERIF_H
#define PSOCDUINO_PERIF_H

#include "UART.h"
#include "PWM8_1.h"
#include "ADCINC.h"
#include "PGA.h"
#include "DAC8_1.h"

#define PSD_HAS_UART 1
#define PSD_UART_START(p)     UART_Start(p)
#define PSD_UART_STOP()       UART_Stop()
#define PSD_UART_RXSTAT()     UART_bReadRxStatus()
#define PSD_UART_RXDATA()     UART_bReadRxData()
#define PSD_UART_TXSTAT()     UART_bReadTxStatus()
#define PSD_UART_SEND(b)      UART_SendData(b)
#define PSD_UART_PARITY_NONE  UART_PARITY_NONE
#define PSD_UART_RX_COMPLETE  UART_RX_COMPLETE
#define PSD_UART_TX_EMPTY     UART_TX_BUFFER_EMPTY
#define PSD_UART_TX_COMPLETE  UART_TX_COMPLETE
#define PSD_UART_TX_PIN       P1_4
#define PSD_UART_RX_PIN       P1_2

#define PSD_HAS_PWM 1
#define PSD_PWM_PIN           P1_6
#define PSD_PWM_START()       PWM8_1_Start()
#define PSD_PWM_WRITE(v)      PWM8_1_WritePulseWidth(v)

#define PSD_HAS_ADC 1
#define PSD_ADC_BITS          12
#define PSD_ADC_START()       { PGA_Start(PGA_HIGHPOWER); ADCINC_Start(ADCINC_HIGHPOWER); }
#define PSD_ADC_SAMPLE()      ADCINC_GetSamples(1)
#define PSD_ADC_READY()       ADCINC_fIsDataAvailable()
#define PSD_ADC_GET()         ADCINC_wClearFlagGetData()

#define PSD_HAS_DAC 1
#define PSD_DAC_PIN           P0_5
#define PSD_DAC_START()       DAC8_1_Start(DAC8_1_HIGHPOWER)
#define PSD_DAC_WRITE(v)      DAC8_1_WriteBlind((BYTE)(v))

#endif
