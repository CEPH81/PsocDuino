/* perif_serial.c - Serial sobre el User Module UART (TX/RX por bloques DCB). */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"
#if PSD_HW
#include "perif_hw.h"
#endif

#if PSD_HAS_UART

static unsigned char rx_has, rx_byte;

/* VC3 (reloj del UART = 8 x baudios): OSC_CR4 (banco 1, DEh) elige la fuente
 * (0 = SysClk 24 MHz, 2 = VC2 8 MHz) y OSC_CR3 (DFh) el divisor - 1.
 * Tabla en lugar de dividir en 32 bits: ahorra flash. */
/* 115200 (por defecto), 57600, 38400, 19200, 9600 o 4800 baudios, 8N1 */
static void ser_begin(unsigned long baud)
{
    unsigned char src = 0, div = 25;                     /* 115200 */
    if (baud == 57600) div = 51;
    else if (baud == 38400) div = 77;
    else if (baud == 19200) div = 155;
    else if (baud == 9600) { src = 2; div = 103; }       /* 8 MHz / 104 / 8 */
    else if (baud <= 4800) { src = 2; div = 207; }       /* 4800 (minimo con VC2) */
    PSD_WR1(0xDE, src);
    PSD_WR1(0xDF, div);
    rx_has = 0;
    PSD_UART_START(PSD_UART_PARITY_NONE);
}

static void ser_end(void)
{
    PSD_UART_STOP();
}

static int ser_available(void)
{
    if (!rx_has && (PSD_UART_RXSTAT() & PSD_UART_RX_COMPLETE)) {
        rx_byte = PSD_UART_RXDATA();
        rx_has = 1;
    }
    return rx_has;
}

static int ser_read(void)
{
    if (!ser_available()) return -1;
    rx_has = 0;
    return rx_byte;
}

static void ser_write(unsigned char c)
{
    while (!(PSD_UART_TXSTAT() & PSD_UART_TX_EMPTY))
        ;
    PSD_UART_SEND(c);
}

static void ser_print(const char *s)
{
    while (*s) ser_write((unsigned char)*s++);
}

static void ser_println(const char *s)
{
    ser_print(s);
    ser_write('\r');
    ser_write('\n');
}

static void ser_printNum(long n)
{
    char d[11];                     /* digitos en RAM: se envian uno a uno */
    unsigned char i = 0;
    unsigned long v;
    if (n < 0) {
        ser_write('-');
        v = (unsigned long)(-n);
    } else {
        v = (unsigned long)n;
    }
    do {
        d[i++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (i) ser_write((unsigned char)d[--i]);
}

static void ser_printlnNum(long n)
{
    ser_printNum(n);
    ser_write('\r');
    ser_write('\n');
}

static void ser_flush(void)
{
    while (!(PSD_UART_TXSTAT() & PSD_UART_TX_COMPLETE))
        ;
}

const PsdSerial Serial = {
    ser_begin, ser_end, ser_available, ser_read, ser_write,
    ser_print, ser_println, ser_printNum, ser_printlnNum, ser_flush
};

#endif
