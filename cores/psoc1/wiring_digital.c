/* wiring_digital.c - pinMode / digitalWrite para PSoC 1
 *
 * Modos de manejo (PRTxDM2:DM1:DM0) segun el TRM del PSoC 1:
 *   000 pull-down resistivo   001 fuerte          010 alta Z digital
 *   011 pull-up resistivo     110 alta Z analogica 111 drenador abierto (bajo)
 * En los modos resistivos la resistencia actua segun el bit de datos, por eso
 * se usa un registro sombra: leer PRTxDR devuelve el pin, no el latch.
 *
 * Registros del puerto p (direccion 4*p): DR = +0, DM2 = +3 (banco 0);
 * DM0 = +0, DM1 = +1 (banco 1). Se leen y escriben con reg[X+0] en
 * psocduino_asm.S: el codigo no crece con el numero de puertos.
 * digitalRead y digitalToggle estan en archivos propios (solo se enlazan si
 * el sketch los usa). */
#include "Arduino.h"

#ifdef PSOCDUINO_HOST_TEST /* prueba en PC: registros simulados */
uint8_t psocduino_reg_rd(uint8_t a) { return psd_host_regs[a]; }
uint8_t psocduino_reg1_rd(uint8_t a) { return psd_host_regs[0x100 + a]; }
void psocduino_reg_wr(unsigned int av) { psd_host_regs[av >> 8] = (uint8_t)av; }
void psocduino_reg1_wr(unsigned int av) { psd_host_regs[0x100 + (av >> 8)] = (uint8_t)av; }
uint8_t psocduino_mask(uint8_t pin) { return (uint8_t)(1 << (pin & 7)); }
#endif

/* despues del reset PRTxDR = 0, igual que este arreglo en bss */
uint8_t psocduino_shadow[PSOC_NUM_PORTS];

#ifndef PSOCDUINO_MINCONFIG
/* variantes con User Modules: su configuracion pudo poner PRTxDR distinto de 0 */
void psocduino_gpio_init(void)
{
    uint8_t p;
    for (p = 0; p < PSOC_NUM_PORTS; p++) psocduino_shadow[p] = psocduino_reg_rd((uint8_t)(p << 2));
}
#endif

/* bits DM2 (4), DM1 (2), DM0 (1) de cada modo, en el orden de Arduino.h:
 * INPUT 010, OUTPUT 001, PULLUP 011, PULLDOWN 000, OPEN_DRAIN 111, ANALOG 110 */
static const uint8_t psd_dm[6] = { 2, 1, 3, 0, 7, 6 };

void pinMode(uint8_t pin, uint8_t mode)
{
    uint8_t port = (uint8_t)(pin >> 3), a, m, n, d;
    if (port >= PSOC_NUM_PORTS || mode > INPUT_ANALOG) return;
    a = (uint8_t)(port << 2);
    m = psocduino_mask(pin);
    n = (uint8_t)~m;
    d = psd_dm[mode];
    if (mode == INPUT_PULLUP || mode == INPUT_PULLDOWN) digitalWrite(pin, (uint8_t)(mode == INPUT_PULLUP));
    PSD_WR(a + 3, (psocduino_reg_rd((uint8_t)(a + 3)) & n) | ((d & 4) ? m : 0));
    PSD_WR1(a, (psocduino_reg1_rd(a) & n) | ((d & 1) ? m : 0));
    PSD_WR1(a + 1, (psocduino_reg1_rd((uint8_t)(a + 1)) & n) | ((d & 2) ? m : 0));
}

void digitalWrite(uint8_t pin, uint8_t val)
{
    uint8_t port = (uint8_t)(pin >> 3), m, v;
    if (port >= PSOC_NUM_PORTS) return;
    m = psocduino_mask(pin);
    v = (uint8_t)(psocduino_shadow[port] & (uint8_t)~m);
    if (val) v |= m;
    psocduino_shadow[port] = v;
    PSD_WR(port << 2, v);
}
