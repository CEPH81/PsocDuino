/* psoc_regs.h - registros M8C usados por el nucleo PsocDuino.
 * Direcciones del PSoC 1 Technical Reference Manual: 0x1xx = banco 1 (XIO).
 * Prefijo PSD_ para no chocar con m8c.h de PSoC Designer. Generado. */
#ifndef PSOC_REGS_H
#define PSOC_REGS_H

#ifdef PSOCDUINO_HOST_TEST
/* compilacion de prueba en PC: registros en memoria */
extern unsigned char psd_host_regs[512];
#define PSD_PRT0DR psd_host_regs[0x000]
#define PSD_PRT0DM2 psd_host_regs[0x003]
#define PSD_PRT0DM0 psd_host_regs[0x100]
#define PSD_PRT0DM1 psd_host_regs[0x101]
#define PSD_PRT1DR psd_host_regs[0x004]
#define PSD_PRT1DM2 psd_host_regs[0x007]
#define PSD_PRT1DM0 psd_host_regs[0x104]
#define PSD_PRT1DM1 psd_host_regs[0x105]
#define PSD_PRT2DR psd_host_regs[0x008]
#define PSD_PRT2DM2 psd_host_regs[0x00B]
#define PSD_PRT2DM0 psd_host_regs[0x108]
#define PSD_PRT2DM1 psd_host_regs[0x109]
#define PSD_PRT3DR psd_host_regs[0x00C]
#define PSD_PRT3DM2 psd_host_regs[0x00F]
#define PSD_PRT3DM0 psd_host_regs[0x10C]
#define PSD_PRT3DM1 psd_host_regs[0x10D]
#define PSD_PRT4DR psd_host_regs[0x010]
#define PSD_PRT4DM2 psd_host_regs[0x013]
#define PSD_PRT4DM0 psd_host_regs[0x110]
#define PSD_PRT4DM1 psd_host_regs[0x111]
#define PSD_PRT5DR psd_host_regs[0x014]
#define PSD_PRT5DM2 psd_host_regs[0x017]
#define PSD_PRT5DM0 psd_host_regs[0x114]
#define PSD_PRT5DM1 psd_host_regs[0x115]
#define PSD_PRT6DR psd_host_regs[0x018]
#define PSD_PRT6DM2 psd_host_regs[0x01B]
#define PSD_PRT6DM0 psd_host_regs[0x118]
#define PSD_PRT6DM1 psd_host_regs[0x119]
#define PSD_PRT7DR psd_host_regs[0x01C]
#define PSD_PRT7DM2 psd_host_regs[0x01F]
#define PSD_PRT7DM0 psd_host_regs[0x11C]
#define PSD_PRT7DM1 psd_host_regs[0x11D]
#define PSD_INT_CLR0 psd_host_regs[0x0DA]
#define PSD_INT_MSK0 psd_host_regs[0x0E0]
#define PSD_OSC_CR0 psd_host_regs[0x1E0]
#define PSD_ASM(x) ((void)0)
#else
#define PSD_ASM(x) asm(x)
#pragma ioport PSD_PRT0DR: 0x000  /* Port 0 Data */
unsigned char PSD_PRT0DR;
#pragma ioport PSD_PRT0DM2: 0x003  /* Port 0 Drive Mode 2 */
unsigned char PSD_PRT0DM2;
#pragma ioport PSD_PRT0DM0: 0x100  /* Port 0 Drive Mode 0 (banco 1) */
unsigned char PSD_PRT0DM0;
#pragma ioport PSD_PRT0DM1: 0x101  /* Port 0 Drive Mode 1 (banco 1) */
unsigned char PSD_PRT0DM1;
#pragma ioport PSD_PRT1DR: 0x004  /* Port 1 Data */
unsigned char PSD_PRT1DR;
#pragma ioport PSD_PRT1DM2: 0x007  /* Port 1 Drive Mode 2 */
unsigned char PSD_PRT1DM2;
#pragma ioport PSD_PRT1DM0: 0x104  /* Port 1 Drive Mode 0 (banco 1) */
unsigned char PSD_PRT1DM0;
#pragma ioport PSD_PRT1DM1: 0x105  /* Port 1 Drive Mode 1 (banco 1) */
unsigned char PSD_PRT1DM1;
#pragma ioport PSD_PRT2DR: 0x008  /* Port 2 Data */
unsigned char PSD_PRT2DR;
#pragma ioport PSD_PRT2DM2: 0x00B  /* Port 2 Drive Mode 2 */
unsigned char PSD_PRT2DM2;
#pragma ioport PSD_PRT2DM0: 0x108  /* Port 2 Drive Mode 0 (banco 1) */
unsigned char PSD_PRT2DM0;
#pragma ioport PSD_PRT2DM1: 0x109  /* Port 2 Drive Mode 1 (banco 1) */
unsigned char PSD_PRT2DM1;
#pragma ioport PSD_PRT3DR: 0x00C  /* Port 3 Data */
unsigned char PSD_PRT3DR;
#pragma ioport PSD_PRT3DM2: 0x00F  /* Port 3 Drive Mode 2 */
unsigned char PSD_PRT3DM2;
#pragma ioport PSD_PRT3DM0: 0x10C  /* Port 3 Drive Mode 0 (banco 1) */
unsigned char PSD_PRT3DM0;
#pragma ioport PSD_PRT3DM1: 0x10D  /* Port 3 Drive Mode 1 (banco 1) */
unsigned char PSD_PRT3DM1;
#pragma ioport PSD_PRT4DR: 0x010  /* Port 4 Data */
unsigned char PSD_PRT4DR;
#pragma ioport PSD_PRT4DM2: 0x013  /* Port 4 Drive Mode 2 */
unsigned char PSD_PRT4DM2;
#pragma ioport PSD_PRT4DM0: 0x110  /* Port 4 Drive Mode 0 (banco 1) */
unsigned char PSD_PRT4DM0;
#pragma ioport PSD_PRT4DM1: 0x111  /* Port 4 Drive Mode 1 (banco 1) */
unsigned char PSD_PRT4DM1;
#pragma ioport PSD_PRT5DR: 0x014  /* Port 5 Data */
unsigned char PSD_PRT5DR;
#pragma ioport PSD_PRT5DM2: 0x017  /* Port 5 Drive Mode 2 */
unsigned char PSD_PRT5DM2;
#pragma ioport PSD_PRT5DM0: 0x114  /* Port 5 Drive Mode 0 (banco 1) */
unsigned char PSD_PRT5DM0;
#pragma ioport PSD_PRT5DM1: 0x115  /* Port 5 Drive Mode 1 (banco 1) */
unsigned char PSD_PRT5DM1;
#pragma ioport PSD_PRT6DR: 0x018  /* Port 6 */
unsigned char PSD_PRT6DR;
#pragma ioport PSD_PRT6DM2: 0x01B  /* Port 6 */
unsigned char PSD_PRT6DM2;
#pragma ioport PSD_PRT6DM0: 0x118  /* Port 6 */
unsigned char PSD_PRT6DM0;
#pragma ioport PSD_PRT6DM1: 0x119  /* Port 6 */
unsigned char PSD_PRT6DM1;
#pragma ioport PSD_PRT7DR: 0x01C  /* Port 7 */
unsigned char PSD_PRT7DR;
#pragma ioport PSD_PRT7DM2: 0x01F  /* Port 7 */
unsigned char PSD_PRT7DM2;
#pragma ioport PSD_PRT7DM0: 0x11C  /* Port 7 */
unsigned char PSD_PRT7DM0;
#pragma ioport PSD_PRT7DM1: 0x11D  /* Port 7 */
unsigned char PSD_PRT7DM1;
#pragma ioport PSD_INT_CLR0: 0x0DA  /* Interrupt Clear 0 */
unsigned char PSD_INT_CLR0;
#pragma ioport PSD_INT_MSK0: 0x0E0  /* Interrupt Mask 0 */
unsigned char PSD_INT_MSK0;
#pragma ioport PSD_OSC_CR0: 0x1E0  /* Oscillator Control 0 (banco 1) */
unsigned char PSD_OSC_CR0;
#endif

#define PSD_INT_SLEEP   0x40   /* INT_MSK0 / INT_CLR0: Sleep Timer */
#define PSD_OSC_SLEEP   0x18   /* OSC_CR0[4:3]: 00 = 512 Hz */

/* banco de registros y GIE (registro F de la CPU) */
#define PSD_BANK1()  PSD_ASM("or  F, 10h")
#define PSD_BANK0()  PSD_ASM("and F, EFh")
#define PSD_GIE_ON() PSD_ASM("or  F, 01h")
#define PSD_GIE_OFF() PSD_ASM("and F, FEh")
#endif
