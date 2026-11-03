/* wiring_digital.c - pinMode / digitalWrite / digitalRead para PSoC 1
 *
 * Modos de manejo (PRTxDM2:DM1:DM0) segun el TRM del PSoC 1:
 *   000 pull-down resistivo   001 fuerte          010 alta Z digital
 *   011 pull-up resistivo     110 alta Z analogica 111 drenador abierto (bajo)
 * En los modos resistivos la resistencia actua segun el bit de datos, por eso
 * se usa un registro sombra: leer PRTxDR devuelve el pin, no el latch.
 *
 * Registros del puerto p (direccion 4*p): DR = +0, DM2 = +3 (banco 0);
 * DM0 = +0, DM1 = +1 (banco 1). Se accede con reg[X+0] (psocduino_asm.S),
 * sin switch por puerto: el codigo no crece con el numero de puertos. */
#include "Arduino.h"

#ifdef PSOCDUINO_HOST_TEST /* prueba en PC: registros simulados */
uint8_t psocduino_reg_rd(uint8_t a) { return psd_host_regs[a]; }
uint8_t psocduino_reg1_rd(uint8_t a) { return psd_host_regs[0x100 + a]; }
void psocduino_reg_wr(unsigned int av) { psd_host_regs[av >> 8] = (uint8_t)av; }
void psocduino_reg1_wr(unsigned int av) { psd_host_regs[0x100 + (av >> 8)] = (uint8_t)av; }
#endif

static uint8_t shadow[PSOC_NUM_PORTS];

/* modo Arduino (INPUT..INPUT_ANALOG) -> DM2:DM1:DM0 */
static const uint8_t dm_of_mode[6] = { 2, 1, 3, 0, 7, 6 };

static void dr_write(uint8_t port, uint8_t v)
{
    shadow[port] = v;
    PSD_WR(port << 2, v);
}

void psocduino_gpio_init(void)
{
    uint8_t p;
    for (p = 0; p < PSOC_NUM_PORTS; p++) shadow[p] = psocduino_reg_rd((uint8_t)(p << 2));
}

void pinMode(uint8_t pin, uint8_t mode)
{
    uint8_t port = (uint8_t)(pin >> 3), mask = (uint8_t)(1 << (pin & 7));
    uint8_t a = (uint8_t)(port << 2), keep = (uint8_t)~mask, dm;
    if (port >= PSOC_NUM_PORTS) return;
    dm = mode < 6 ? dm_of_mode[mode] : 2;
    if (mode == INPUT_PULLUP) dr_write(port, (uint8_t)(shadow[port] | mask));
    else if (mode == INPUT_PULLDOWN) dr_write(port, (uint8_t)(shadow[port] & keep));
    PSD_WR(a + 3, (psocduino_reg_rd((uint8_t)(a + 3)) & keep) | ((dm & 4) ? mask : 0));
    PSD_WR1(a, (psocduino_reg1_rd(a) & keep) | ((dm & 1) ? mask : 0));
    PSD_WR1(a + 1, (psocduino_reg1_rd((uint8_t)(a + 1)) & keep) | ((dm & 2) ? mask : 0));
}

void digitalWrite(uint8_t pin, uint8_t val)
{
    uint8_t port = (uint8_t)(pin >> 3), mask = (uint8_t)(1 << (pin & 7));
    if (port >= PSOC_NUM_PORTS) return;
    dr_write(port, val ? (uint8_t)(shadow[port] | mask) : (uint8_t)(shadow[port] & ~mask));
}

void digitalToggle(uint8_t pin)
{
    uint8_t port = (uint8_t)(pin >> 3), mask = (uint8_t)(1 << (pin & 7));
    if (port >= PSOC_NUM_PORTS) return;
    dr_write(port, (uint8_t)(shadow[port] ^ mask));
}

int digitalRead(uint8_t pin)
{
    uint8_t port = (uint8_t)(pin >> 3), mask = (uint8_t)(1 << (pin & 7));
    if (port >= PSOC_NUM_PORTS) return LOW;
    return (psocduino_reg_rd((uint8_t)(port << 2)) & mask) ? HIGH : LOW;
}
