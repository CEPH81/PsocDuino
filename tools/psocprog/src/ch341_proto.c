/* ch341_proto.c - CH341A USB command encoding (reverse engineered from
 * CH341DLL.DLL v2008 as used by PSoC Programmer CH341A, cross-checked with
 * the public flashrom ch341a_spi driver).
 *
 *   0xA0                      GET_STATUS  -> pin bytes (bit7 of byte0 = D7)
 *   0xAA 0x60|mode 0x00        I2C_STREAM SET (SPI/I2C speed)
 *   0xA8 b0..b30              SPI_STREAM, the chip shifts LSB first
 *   0xAB ...  0x20            UIO_STREAM: 0x80|v OUT, 0x40|v DIR, 0x00 IN,
 *                             0xC0|us DELAY, 0x20 END
 */
#include "ch341.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PKT 32
#define UIO 0xAB
#define UIO_OUT 0x80
#define UIO_DIR 0x40
#define UIO_IN 0x00
#define UIO_END 0x20
#define SPI_CMD 0xA8
#define STATUS_CMD 0xA0
#define I2C_CMD 0xAA
#define BITS_PER_PKT 7      /* same as the DLL: 1 + 7*4 + 1 = 30 bytes */
#define PKTS_PER_XFER 8     /* queue several packets per USB transfer */

int ch341_verbose = 0;

typedef struct {
    ch341_xfer_fn xfer;
    void *ctx;
    void (*close_ctx)(void *);
} proto_t;

static uint8_t rev8(uint8_t b)
{
    b = (uint8_t)((b & 0xF0) >> 4 | (b & 0x0F) << 4);
    b = (uint8_t)((b & 0xCC) >> 2 | (b & 0x33) << 2);
    b = (uint8_t)((b & 0xAA) >> 1 | (b & 0x55) << 1);
    return b;
}

static int p_set_d5_d0(ch341_t *d, uint8_t dir, uint8_t data)
{
    proto_t *p = d->priv;
    uint8_t c[4] = { UIO, (uint8_t)(UIO_OUT | (data & 0x3F)),
                     (uint8_t)(UIO_DIR | (dir & 0x3F)), UIO_END };
    return p->xfer(p->ctx, c, 4, NULL, 0);
}

static int p_get_input(ch341_t *d, uint32_t *st)
{
    proto_t *p = d->priv;
    uint8_t c = STATUS_CMD, r[6] = {0};
    if (p->xfer(p->ctx, &c, 1, r, 6) < 0) return -1;
    *st = r[0] | (uint32_t)(r[1] & 0xEF) << 8 | (uint32_t)(r[2] & 0x80) << 16;
    return 0;
}

static int p_bitstream(ch341_t *d, uint8_t *buf, size_t n)
{
    proto_t *p = d->priv;
    uint8_t out[PKT * PKTS_PER_XFER];
    size_t i = 0;
    while (i < n) {
        int olen = 0, ilen = 0;
        size_t start = i;
        for (int k = 0; k < PKTS_PER_XFER && i < n; k++) {
            uint8_t *q = out + olen;
            int j = 0;
            q[j++] = UIO;
            for (int b = 0; b < BITS_PER_PKT && i < n; b++, i++) {
                uint8_t v = buf[i] & 0x37;
                q[j++] = (uint8_t)(UIO_OUT | v);        /* SCLK low  */
                q[j++] = (uint8_t)(UIO_OUT | v | 0x08); /* SCLK high */
                q[j++] = UIO_IN;                        /* sample    */
                q[j++] = (uint8_t)(UIO_OUT | v);        /* SCLK low  */
                ilen++;
            }
            q[j++] = UIO_END;
            while (j < PKT) q[j++] = 0;
            olen += PKT;
        }
        if (p->xfer(p->ctx, out, olen, buf + start, ilen) < 0) return -1;
    }
    return 0;
}

static int p_spi(ch341_t *d, uint8_t *buf, size_t n)
{
    proto_t *p = d->priv;
    uint8_t out[PKT];
    size_t i = 0;
    while (i < n) {
        int m = (int)(n - i > PKT - 1 ? PKT - 1 : n - i);
        out[0] = SPI_CMD;
        for (int k = 0; k < m; k++) out[1 + k] = rev8(buf[i + k]);
        if (p->xfer(p->ctx, out, m + 1, buf + i, m) < 0) return -1;
        for (int k = 0; k < m; k++) buf[i + k] = rev8(buf[i + k]);
        i += m;
    }
    return 0;
}

static void p_close(ch341_t *d)
{
    proto_t *p = d->priv;
    if (p->close_ctx) p->close_ctx(p->ctx);
    free(p);
    free(d);
}

ch341_t *ch341_proto_new(const char *name, ch341_xfer_fn xfer, void *ctx,
                         void (*close_ctx)(void *))
{
    ch341_t *d = calloc(1, sizeof *d);
    proto_t *p = calloc(1, sizeof *p);
    if (!d || !p) { free(d); free(p); return NULL; }
    p->xfer = xfer; p->ctx = ctx; p->close_ctx = close_ctx;
    d->name = name; d->priv = p;
    d->set_d5_d0 = p_set_d5_d0;
    d->get_input = p_get_input;
    d->bitstream = p_bitstream;
    d->spi = p_spi;
    d->close = p_close;
    /* SPI/I2C speed mode 0, as CH341SetStream(0, 0x80) in the original. */
    uint8_t c[3] = { I2C_CMD, 0x60, 0x00 };
    if (xfer(ctx, c, 3, NULL, 0) < 0) { p_close(d); return NULL; }
    return d;
}
