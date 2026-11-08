/* psocduino_perif.h - User Modules de la variante cy8c29466_perif
 * (proyecto Base2_cy8c29466 de PSoC Designer). Lo usa la biblioteca PsocDuinoPerif.
 *   UART   DCB02/DCB03  TX P2_1, RX P2_0  (VC3; Serial.begin cambia la velocidad)
 *   PWM8   DBB00        P1_6              (VC2 8 MHz, periodo 255 -> 31,25 kHz)
 *   ADCINC DBB01+ASC10+ASD20 con PGA en ACB00 (ganancia 1), 12 bits, entradas P0_1/P0_3/P0_7
 *   DAC8   ASD11+ASC21  P0_5              (8 bits, binario desplazado 0..255)
 *   I2CHW  maestro por hardware en P1_5 (SDA) / P1_7 (SCL): en el sketch
 *          #include "I2CHW_1Mstr.h" y use las funciones I2CHW_1_... de PSoC Designer
 * Filas digitales 1..3 libres.
 */
#ifndef PSOCDUINO_PERIF_H
#define PSOCDUINO_PERIF_H

#include "UART_1.h"
#include "PWM8_1.h"
#include "ADCINC.h"
#include "PGA.h"
#include "DAC8.h"

#define PSD_HAS_UART 1
#define PSD_UART_START(p)     UART_1_Start(p)
#define PSD_UART_STOP()       UART_1_Stop()
#define PSD_UART_RXSTAT()     UART_1_bReadRxStatus()
#define PSD_UART_RXDATA()     UART_1_bReadRxData()
#define PSD_UART_TXSTAT()     UART_1_bReadTxStatus()
#define PSD_UART_SEND(b)      UART_1_SendData(b)
#define PSD_UART_PARITY_NONE  UART_1_PARITY_NONE
#define PSD_UART_RX_COMPLETE  UART_1_RX_COMPLETE
#define PSD_UART_TX_EMPTY     UART_1_TX_BUFFER_EMPTY
#define PSD_UART_TX_COMPLETE  UART_1_TX_COMPLETE
#define PSD_UART_TX_PIN       P2_1
#define PSD_UART_RX_PIN       P2_0

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
#define PSD_DAC_START()       DAC8_Start(DAC8_HIGHPOWER)
#define PSD_DAC_WRITE(v)      DAC8_WriteBlind((BYTE)(v))

#define PSD_HAS_I2CHW 1
#define PSD_I2C_SDA_PIN       P1_5
#define PSD_I2C_SCL_PIN       P1_7

#endif
