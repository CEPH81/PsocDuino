/* Arduino.h - API Arduino para Cypress PSoC 1 (M8C) - PsocDuino
 * Compilador ImageCraft ICCM8C (C89): int = 16 bits, long = 32 bits. */
#ifndef Arduino_h
#define Arduino_h

/* ---- tipos ---- */
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned int uint16_t;
typedef signed int int16_t;
typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef unsigned char boolean;
typedef unsigned char bool;
typedef unsigned char byte;
typedef unsigned int word;
#ifndef true
#define true 1
#define false 0
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ---- constantes ---- */
#define HIGH 1
#define LOW 0

#define INPUT 0            /* alta impedancia digital */
#define OUTPUT 1           /* push-pull fuerte */
#define INPUT_PULLUP 2     /* resistencia a Vdd (5.6 k) */
#define INPUT_PULLDOWN 3   /* resistencia a GND (5.6 k) */
#define OUTPUT_OPEN_DRAIN 4 /* drena a GND, alta impedancia en HIGH */
#define INPUT_ANALOG 5     /* alta impedancia analogica (sin buffer digital) */

#define LSBFIRST 0
#define MSBFIRST 1

/* Pines: numero = puerto * 8 + bit.  P1_0 / P1_1 son SDATA / SCLK de ISSP. */
#define P0_0 0
#define P0_1 1
#define P0_2 2
#define P0_3 3
#define P0_4 4
#define P0_5 5
#define P0_6 6
#define P0_7 7
#define P1_0 8
#define P1_1 9
#define P1_2 10
#define P1_3 11
#define P1_4 12
#define P1_5 13
#define P1_6 14
#define P1_7 15
#define P2_0 16
#define P2_1 17
#define P2_2 18
#define P2_3 19
#define P2_4 20
#define P2_5 21
#define P2_6 22
#define P2_7 23
#define P3_0 24
#define P3_1 25
#define P3_2 26
#define P3_3 27
#define P3_4 28
#define P3_5 29
#define P3_6 30
#define P3_7 31
#define P4_0 32
#define P4_1 33
#define P4_2 34
#define P4_3 35
#define P4_4 36
#define P4_5 37
#define P4_6 38
#define P4_7 39
#define P5_0 40
#define P5_1 41
#define P5_2 42
#define P5_3 43
#define P5_4 44
#define P5_5 45
#define P5_6 46
#define P5_7 47

#ifndef PSOC_NUM_PORTS
#define PSOC_NUM_PORTS 3
#endif
#define NUM_DIGITAL_PINS (PSOC_NUM_PORTS * 8)
#ifndef LED_BUILTIN
#define LED_BUILTIN P1_2
#endif

#ifndef F_CPU
#define F_CPU 24000000UL
#endif

/* ---- macros de Arduino ---- */
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define abs(x) ((x) > 0 ? (x) : -(x))
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define sq(x) ((x) * (x))
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))
#define bit(b) (1UL << (b))
#define bitRead(v, b) (((v) >> (b)) & 0x01)
#define bitSet(v, b) ((v) |= (1UL << (b)))
#define bitClear(v, b) ((v) &= ~(1UL << (b)))
#define bitWrite(v, b, x) ((x) ? bitSet(v, b) : bitClear(v, b))
#define clockCyclesPerMicrosecond() (F_CPU / 1000000UL)

#include "psoc_regs.h"
#define interrupts() PSD_GIE_ON()
#define noInterrupts() PSD_GIE_OFF()

/* ---- funciones ---- */
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
void digitalToggle(uint8_t pin);

unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout);

long map(long x, long in_min, long in_max, long out_min, long out_max);
void randomSeed(unsigned long seed);
long random(long howbig);
long randomRange(long howsmall, long howbig);

void setup(void);
void loop(void);

/* interno (psocduino_asm.S, convencion fastcall16 de ImageCraft) */
void psocduino_init(void);
#ifndef PSOCDUINO_HOST_TEST
#pragma fastcall16 psocduino_spin9
#pragma fastcall16 psocduino_measure_tick
#endif
void psocduino_spin9(uint8_t n);            /* espera exacta de 9*n ciclos de CPU */
unsigned int psocduino_measure_tick(void);  /* iteraciones de 23 ciclos por tic */
#endif
