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

/* ciclos de llamada a psocduino_spin9() + calculo del bucle (calibrable) */
#ifndef PSOCDUINO_CALL_CYCLES
#define PSOCDUINO_CALL_CYCLES 60
#endif
/* ciclos de una vuelta del bucle de bloques de delayMicroseconds() */
#ifndef PSOCDUINO_LOOP_CYCLES
#define PSOCDUINO_LOOP_CYCLES 40
#endif

/* ciclos de CPU por cada medio microsegundo: 48, 24, 12, 6 o 3 */
#define PSD_HALF_US ((unsigned int)(F_CPU / 500000UL))
/* delayMicroseconds() espera en bloques de 32 us */
#define PSD_BLOCK_US 32
#define PSD_BLOCK_CYC (PSD_BLOCK_US * F_CPU / 1000000UL)
#if PSD_BLOCK_CYC > PSOCDUINO_LOOP_CYCLES + PSOCDUINO_CALL_CYCLES
#define PSD_BLOCK_N ((uint8_t)((PSD_BLOCK_CYC - PSOCDUINO_LOOP_CYCLES - PSOCDUINO_CALL_CYCLES) / 9))
#else
#define PSD_BLOCK_N 0
#endif

void psocduino_gpio_init(void);

static volatile unsigned long ms_count;
static volatile unsigned int us_acc;
static unsigned int us_per_tick = 1953; /* 1/512 s con el ILO nominal */

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
    us_acc += us_per_tick;
    while (us_acc >= 1000) {
        us_acc -= 1000;
        ms_count++;
    }
}

void psocduino_init(void)
{
    unsigned int n, us;

    PSD_GIE_OFF();
    psocduino_gpio_init();

    /* Sleep Timer a 512 Hz y latch de su interrupcion */
    PSD_BANK1();
    PSD_OSC_CR0 = (uint8_t)(PSD_OSC_CR0 & ~PSD_OSC_SLEEP);
    PSD_BANK0();
    PSD_INT_MSK0 = (uint8_t)(PSD_INT_MSK0 | PSD_INT_SLEEP);

    /* un tic = n * 23 ciclos (+9 cada 256); en us = ciclos * 2 / PSD_HALF_US,
     * calculado en 16 bits sin desbordar */
    n = psocduino_measure_tick();
    us = (n / PSD_HALF_US) * 46U + ((n % PSD_HALF_US) * 46U) / PSD_HALF_US
         + ((n >> 8) * 18U) / PSD_HALF_US;
    if (us > 100) us_per_tick = us;

    ms_count = 0;
    us_acc = 0;
    PSD_GIE_ON();
}

unsigned long millis(void)
{
    unsigned long m;
    PSD_GIE_OFF();
    m = ms_count;
    PSD_GIE_ON();
    return m;
}

void delayMicroseconds(unsigned int us)
{
    unsigned int cycles;
    while (us > PSD_BLOCK_US) {
        psocduino_spin9(PSD_BLOCK_N);
        us -= PSD_BLOCK_US;
    }
    cycles = (us * PSD_HALF_US) >> 1; /* us <= 32: sin desborde */
    if (cycles > PSOCDUINO_CALL_CYCLES + 9)
        psocduino_spin9((uint8_t)((cycles - PSOCDUINO_CALL_CYCLES) / 9));
}

void delay(unsigned long ms)
{
    while (ms) {
        delayMicroseconds(1000);
        ms--;
    }
}

/* para micros() (wiring_micros.c): ms y us acumulados leidos juntos */
unsigned long psocduino_micros_parts(unsigned int *us)
{
    unsigned long m;
    PSD_GIE_OFF();
    m = ms_count;
    *us = us_acc;
    PSD_GIE_ON();
    return m;
}
