/* PsocDuinoPerif.h - perifericos para PSoC 1 con PsocDuino (C89 para ImageCraft).
 * Solo para las placas "PSoC 1 ... + perifericos": usan los User Modules de su
 * proyecto de PSoC Designer (ver psocduino_perif.h de la variante).
 *
 *   Serial.begin(115200);  Serial.print("texto");  Serial.printNum(n);
 *   Serial.available(); Serial.read(); Serial.write(c);
 *   analogRead(P0_3)        0..1023 (analogReadResolution(8..12) para cambiarlo)
 *   analogWrite(P1_6, v)    PWM 0..255 a 31,25 kHz en la pata PWM de la placa
 *   dacWrite(v)             DAC 0..255 en P0_5 (0 V .. Vdd)
 *
 * ImageCraft solo compila C: Serial es una estructura con punteros a funcion, por
 * eso los numeros se imprimen con printNum/printlnNum (C no tiene sobrecarga).
 * Los textos de print/println deben ser constantes (van en la flash). */
#ifndef PsocDuinoPerif_h
#define PsocDuinoPerif_h

#include <Arduino.h>

typedef struct {
    void (*begin)(unsigned long baud);   /* 4800..115200 baudios, 8N1 */
    void (*end)(void);
    int (*available)(void);
    int (*read)(void);                   /* -1 si no hay dato */
    void (*write)(unsigned char c);
    void (*print)(const char *s);        /* texto en la flash ("...") */
    void (*println)(const char *s);
    void (*printNum)(long n);
    void (*printlnNum)(long n);
    void (*flush)(void);
} PsdSerial;

extern const PsdSerial Serial;

int analogRead(uint8_t pin);
void analogReadResolution(uint8_t bits);
void analogWrite(uint8_t pin, uint8_t value);
void dacWrite(uint8_t value);

#endif
