/* wiring_math.c - map / random (archivo propio: la aritmetica de 32 bits
 * solo se enlaza si el sketch usa estas funciones) */
#include "Arduino.h"

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
