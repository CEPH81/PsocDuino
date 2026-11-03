/* wiring.c - tiempo para PSoC 1 sin modulos de usuario
 *
 * millis(): interrupcion del Sleep Timer (512 Hz, oscilador interno ILO de
 *   32 kHz).  El ILO tiene mucha tolerancia (15..64 kHz segun la hoja de
 *   datos), asi que al arrancar se mide contra el reloj de CPU (IMO, +-2.5%)
 *   y se calcula cuantos microsegundos vale cada tic.
 * delay()/delayMicroseconds(): bucles de ciclos exactos sobre el IMO.
 *
 * Ciclos M8C: dec A = 4, jnz = 5, inc A = 4, jnc = 5, jmp = 5,
 *             tst reg[expr],expr = 9.
 */
#include "Arduino.h"

/* ciclos de llamada a psocduino_spin9() + calculo del bucle (calibrable) */
#ifndef PSOCDUINO_CALL_CYCLES
#define PSOCDUINO_CALL_CYCLES 60
#endif

void psocduino_gpio_init(void);

uint8_t psocduino_spin_n;           /* parametro para el ensamblador */
uint8_t psocduino_cal_lo, psocduino_cal_hi;
static volatile unsigned long ms_count;
static volatile unsigned int us_acc;
static unsigned int us_per_tick = 1953; /* 1/512 s con el ILO nominal */

void psocduino_spin9(uint8_t n)
{
    if (!n) return;
    psocduino_spin_n = n;
    PSD_ASM("mov A, [_psocduino_spin_n]");
    PSD_ASM("psocduino_spin_loop: dec A");
    PSD_ASM("jnz psocduino_spin_loop");
}

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

/* Cuenta iteraciones de 23 ciclos durante un tic del Sleep Timer. */
static unsigned int measure_tick(void)
{
    PSD_INT_CLR0 = (uint8_t)~PSD_INT_SLEEP;            /* borrar pendiente */
    while (!(PSD_INT_CLR0 & PSD_INT_SLEEP)) { }        /* sincronizar */
    PSD_INT_CLR0 = (uint8_t)~PSD_INT_SLEEP;
    PSD_ASM("mov X, 0");
    PSD_ASM("mov A, 0");
    PSD_ASM("psocduino_cal_loop: tst reg[0DAh], 40h");
    PSD_ASM("jnz psocduino_cal_done");
    PSD_ASM("inc A");
    PSD_ASM("jnc psocduino_cal_loop");
    PSD_ASM("inc X");
    PSD_ASM("jmp psocduino_cal_loop");
    PSD_ASM("psocduino_cal_done: mov [_psocduino_cal_lo], A");
    PSD_ASM("mov [_psocduino_cal_hi], X");
    PSD_INT_CLR0 = (uint8_t)~PSD_INT_SLEEP;
    return (unsigned int)psocduino_cal_hi << 8 | psocduino_cal_lo;
}

void psocduino_init(void)
{
    unsigned long cycles;
    unsigned int n;

    PSD_GIE_OFF();
    psocduino_gpio_init();

    /* Sleep Timer a 512 Hz y latch de su interrupcion */
    PSD_BANK1();
    PSD_OSC_CR0 = (uint8_t)(PSD_OSC_CR0 & ~PSD_OSC_SLEEP);
    PSD_BANK0();
    PSD_INT_MSK0 = (uint8_t)(PSD_INT_MSK0 | PSD_INT_SLEEP);

    n = measure_tick();
    cycles = (unsigned long)n * 23UL + (unsigned long)(n >> 8) * 9UL;
    if (cycles > 1000UL) /* cycles * 1e6 / F_CPU sin desbordar */
        us_per_tick = (unsigned int)((cycles * 1000UL) / (F_CPU / 1000UL));

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

unsigned long micros(void)
{
    unsigned long m;
    unsigned int u;
    PSD_GIE_OFF();
    m = ms_count;
    u = us_acc;
    PSD_GIE_ON();
    return m * 1000UL + u; /* resolucion = un tic (~2 ms) */
}

void delayMicroseconds(unsigned int us)
{
    unsigned long cycles = (unsigned long)us * (F_CPU / 1000000UL);
    const unsigned long chunk = 255UL * 9UL + PSOCDUINO_CALL_CYCLES;
    while (cycles > chunk) {
        psocduino_spin9(255);
        cycles -= chunk;
    }
    if (cycles > PSOCDUINO_CALL_CYCLES + 9)
        psocduino_spin9((uint8_t)((cycles - PSOCDUINO_CALL_CYCLES) / 9));
}

void delay(unsigned long ms)
{
    while (ms--) delayMicroseconds(1000);
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static unsigned long rnd_state = 1;

void randomSeed(unsigned long seed)
{
    if (seed) rnd_state = seed;
}

long random(long howbig)
{
    if (howbig <= 0) return 0;
    rnd_state = rnd_state * 1103515245UL + 12345UL;
    return (long)((rnd_state >> 8) % (unsigned long)howbig);
}

long randomRange(long howsmall, long howbig)
{
    if (howsmall >= howbig) return howsmall;
    return random(howbig - howsmall) + howsmall;
}
