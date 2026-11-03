/* ch341.h - minimal CH341A abstraction used by psocprog.
 *
 * Pin usage (same wiring as the original "PSoC Programmer CH341A"):
 *   D0  -> XRES (XRES mode)  or  power switch (Power-On mode, active low)
 *   D3  -> SCLK  (PSoC P1[1])
 *   D5  -> SDATA (PSoC P1[0]) through a series resistor (host -> target)
 *   D7  <- SDATA (PSoC P1[0]) direct (target -> host)
 */
#ifndef CH341_H
#define CH341_H
#include <stdint.h>
#include <stddef.h>

typedef struct ch341 ch341_t;

struct ch341 {
    const char *name;
    void *priv;
    /* UIO: set direction/data of D0..D5 (bit=1 output). */
    int (*set_d5_d0)(ch341_t *d, uint8_t dir, uint8_t data);
    /* Read pin status; bit7 = D7. */
    int (*get_input)(ch341_t *d, uint32_t *status);
    /* Bit-banged stream: for each byte, bits (b & 0x37) are put on D0..D5
     * while D3 is pulsed low->high->low; buf is overwritten with the pin
     * state sampled after the rising edge (bit7 = D7).  n <= 1024. */
    int (*bitstream)(ch341_t *d, uint8_t *buf, size_t n);
    /* Hardware SPI (D3 clock, D5 out, D7 in), MSB first, no chip select. */
    int (*spi)(ch341_t *d, uint8_t *buf, size_t n);
    void (*close)(ch341_t *d);
};

/* Backends. Each returns NULL on failure (and prints why). */
ch341_t *ch341_open_libusb(int index);
ch341_t *ch341_open_dll(int index);
ch341_t *ch341_open_sim(const char *chip);

/* Generic packet-level driver used by libusb and the simulator. */
typedef int (*ch341_xfer_fn)(void *ctx, const uint8_t *out, int outlen,
                             uint8_t *in, int inlen);
ch341_t *ch341_proto_new(const char *name, ch341_xfer_fn xfer, void *ctx,
                         void (*close_ctx)(void *));

extern int ch341_verbose;
#endif
