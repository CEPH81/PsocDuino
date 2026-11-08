/* wiring.c - tiempo para PSoC 1 sin modulos de usuario
 *
 * millis(): interrupcion del Sleep Timer (512 Hz, oscilador interno ILO de
 *   32 kHz).  El ILO tiene mucha tolerancia (15..64 kHz segun la hoja de
 *   datos), asi que al arrancar se mide contra el reloj de CPU (IMO, +-2.5%)
 *   y se calcula cuantos microsegundos vale cada tic.
 * delay()/delayMicroseconds(): bucles de ciclos sobre el IMO.
 * Todo en 16 bits (salvo el contador de ms): el M8C no tiene multiplicador
 * de 32 bits y esas rutinas ocupan mucha flash en los chips de 4 KB.
 *
 * Ciclos M8C: dec A = 4, jnz = 5, inc A = 4, jnc = 5, jmp = 5,
 *             tst reg[expr],expr = 9.
 */
#include "Arduino.h"

/* ciclos de llamada a psocduino_spin9() + calculo (calibrable con CalibrarDelay) */
#ifndef PSOCDUINO_CALL_CYCLES
#define PSOCDUINO_CALL_CYCLES 60
#endif
/* ciclos de una vuelta de los bucles de delayMicroseconds() */
#ifndef PSOCDUINO_LOOP_CYCLES
#define PSOCDUINO_LOOP_CYCLES 30
#endif

/* vueltas de spin9 para un bloque de 8 us y para 1 us (constantes) */
#define PSD_CYC8 (8UL * F_CPU / 1000000UL)
#define PSD_CYC1 (F_CPU / 1000000UL)
#if PSD_CYC8 > PSOCDUINO_LOOP_CYCLES + 9
#define PSD_N8 ((uint8_t)((PSD_CYC8 - PSOCDUINO_LOOP_CYCLES) / 9))
#else
#define PSD_N8 0
#endif
#if PSD_CYC1 > PSOCDUINO_LOOP_CYCLES + 9
#define PSD_N1 ((uint8_t)((PSD_CYC1 - PSOCDUINO_LOOP_CYCLES) / 9))
#else
#define PSD_N1 0
#endif
/* microsegundos que ya consume la llamada */
#define PSD_CALL_US ((unsigned int)(PSOCDUINO_CALL_CYCLES * 1000000UL / F_CPU))

#ifndef PSOCDUINO_MINCONFIG
void psocduino_gpio_init(void);
#endif
/* desplazamiento de F_CPU respecto a 24 MHz: 24 -> 0, 12 -> 1, ... 1.5 -> 4 */
#if F_CPU >= 24000000UL
#define PSD_FSHIFT 0
#elif F_CPU >= 12000000UL
#define PSD_FSHIFT 1
#elif F_CPU >= 6000000UL
#define PSD_FSHIFT 2
#elif F_CPU >= 3000000UL
#define PSD_FSHIFT 3
#else
#define PSD_FSHIFT 4
#endif

/* compartidos con millis()/micros() (wiring_millis.c, wiring_micros.c) */
volatile unsigned long psocduino_ms;
volatile unsigned int psocduino_us;
/* sin valores iniciales: en LMM irian a 'xidata' (aviso __xidata_start) */
static unsigned int us_per_tick;

/* psocduino_spin9() y psocduino_measure_tick() estan en psocduino_asm.S */
#ifdef PSOCDUINO_HOST_TEST
void psocduino_spin9(uint8_t n) { (void)n; }
unsigned int psocduino_measure_tick(void) { return 2038; } /* 24 MHz, ILO nominal */
#endif

#ifndef PSOCDUINO_HOST_TEST
#pragma interrupt_handler psocduino_sleep_isr
#endif
void psocduino_sleep_isr(void)
{
    psocduino_us += us_per_tick;
    while (psocduino_us >= 1000) {
        psocduino_us -= 1000;
        psocduino_ms++;
    }
}

void psocduino_init(void)
{
    unsigned int n, us;

    PSD_GIE_OFF();
#ifndef PSOCDUINO_MINCONFIG
    psocduino_gpio_init(); /* la configuracion de PSoC Designer pudo cambiar PRTxDR */
#endif

    /* Sleep Timer a 512 Hz y latch de su interrupcion */
    PSD_BANK1();
    PSD_OSC_CR0 = (uint8_t)(PSD_OSC_CR0 & ~PSD_OSC_SLEEP);
    PSD_BANK0();
    PSD_INT_MSK0 = (uint8_t)(PSD_INT_MSK0 | PSD_INT_SLEEP);

    /* un tic = n * 23 ciclos = n * 23 / 24 us a 24 MHz (x2 por cada mitad de
     * F_CPU). 23/24 = 1 - 1/24 ~ 1 - 1/32 - 1/128 - 1/512 (error 0,07 %): solo
     * restas y desplazamientos, sin las rutinas de multiplicar y dividir. */
    n = (unsigned int)(psocduino_measure_tick() << PSD_FSHIFT);
    us = (unsigned int)(n - (n >> 5) - (n >> 7) - (n >> 9));
    us_per_tick = us > 100 ? us : 1953; /* 1953 = 1/512 s con el ILO nominal */

    psocduino_ms = 0;
    psocduino_us = 0;
    PSD_GIE_ON();
}

void delayMicroseconds(unsigned int us)
{
    /* sin multiplicar ni dividir: bloques de 8 us y luego de 1 us */
    if (us <= PSD_CALL_US) return;
    us -= PSD_CALL_US;
    while (us >= 8) {
        psocduino_spin9(PSD_N8);
        us -= 8;
    }
    while (us) {
        psocduino_spin9(PSD_N1);
        us--;
    }
}

void delay(unsigned long ms)
{
    while (ms) {
        delayMicroseconds(1000);
        ms--;
    }
}
