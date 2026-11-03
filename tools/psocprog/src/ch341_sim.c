/* ch341_sim.c - software model of a CH341A wired to a PSoC 1, used by the
 * test-suite (and "psocprog -b sim") to exercise the full USB packet path
 * without hardware.
 *
 * Target model: ISSP words (22 bits)
 *   100 aaaaaaaa dddddddd 111          write SRAM
 *   101 aaaaaaaa Z dddddddd Z 1        read SRAM (target drives SDATA)
 *   110 aaaaaaaa dddddddd 111          write register; R[FF]=0x12 runs SSC R[F0]
 * SSC: 1 ReadBlock, 2 WriteBlock, 4 Protect, 5 EraseAll, 6 TableRead,
 *      7 Checksum.  Blocks of 64 bytes at SRAM 0x80..0xBF, block # in M[FA],
 *      bank in R[FA] (written with XIO set).
 * The target changes SDATA on the falling SCLK edge, so the CH341 (which
 * samples right after the rising edge) sees data one clock "late", exactly
 * like the bit positions used by the original Windows tool.
 */
#include "ch341.h"
#include "vectors.h"
#include "issp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const psoc_device_t *dev;
    uint8_t flash[4][256][64];
    uint8_t prot[4][64];
    uint8_t mem[256], reg[256];
    int bank;
    /* pins */
    uint8_t out, dir, sclk;
    /* ISSP word decoder */
    int nbits;
    uint32_t word;
    int driving;     /* target drives SDATA */
    int drive_val;
    int pending;     /* value to drive after next falling edge (-1 none) */
    int busy;        /* busy ticks remaining */
    uint8_t rdata;
    int powered;
    /* host side helpers */
    uint8_t inq[4096];
    int inlen;
    int stats_clocks;
} sim_t;

static int sim_sdata(sim_t *s)
{
    if (s->busy > 0) return 1;
    if (s->driving) return s->drive_val;
    return (s->out >> 5) & 1; /* host level through the series resistor */
}

static uint8_t pins(sim_t *s)
{
    return (uint8_t)((s->out & 0x3F & s->dir) | (s->out & 0x3F & ~s->dir) |
                     (sim_sdata(s) ? 0x80 : 0));
}

static void ssc(sim_t *s, int op)
{
    int blk = s->mem[0xFA], b = s->bank & 3;
    int bpb = s->dev->banks >= 2 ? 128 : s->dev->kb * 16;
    s->busy = 12;
    switch (op) {
    case 1: /* read block */
        if (blk < bpb && ((s->prot[b][blk / 4] >> (2 * (3 - blk % 4))) & 3)) {
            s->busy = 0; /* protected: never goes busy */
            break;
        }
        memcpy(s->mem + 0x80, s->flash[b][blk], 64);
        break;
    case 2: memcpy(s->flash[b][blk], s->mem + 0x80, 64); s->busy = 40; break;
    case 4: memcpy(s->prot[b], s->mem + 0x80, (size_t)bpb / 4); s->busy = 40; break;
    case 5: memset(s->flash, 0, sizeof s->flash); memset(s->prot, 0, sizeof s->prot);
            s->busy = 200; break;
    case 6: s->mem[0xF8] = s->dev->id_hi; s->mem[0xF9] = s->dev->id_lo; break;
    case 7: {
        int n = blk ? blk : 256;
        unsigned sum = 0;
        for (int i = 0; i < n && i < 256; i++)
            for (int k = 0; k < 64; k++) sum += s->flash[b][i][k];
        s->mem[0xF8] = (uint8_t)sum;
        s->mem[0xF9] = (uint8_t)(sum >> 8);
        break;
    }
    case 9: break; /* calibrate (init) */
    default: break;
    }
}

/* one SCLK cycle: host bit sampled on the rising edge */
static void clock_rise(sim_t *s)
{
    s->stats_clocks++;
    if (s->busy > 0) { s->busy--; return; }
    int bit = (s->out >> 5) & 1;
    if (s->driving) { /* during the read phase host bits are ignored */
        s->nbits++;
        if (s->nbits == 22) { s->driving = 0; s->nbits = 0; s->word = 0; }
        return;
    }
    if (s->nbits == 0 && !bit) return; /* idle zeros */
    s->word = (s->word << 1) | (uint32_t)bit;
    s->nbits++;
    if (s->nbits == 11 && (s->word >> 8) == 5) { /* read SRAM: drive data */
        s->rdata = s->mem[s->word & 0xFF];
        s->driving = 1;
        s->drive_val = 0; /* Z cell */
        return;
    }
    if (s->nbits == 22) {
        int op = (int)(s->word >> 19), a = (int)((s->word >> 11) & 0xFF),
            d = (int)((s->word >> 3) & 0xFF);
        s->nbits = 0;
        s->word = 0;
        if (getenv("SIM_TRACE")) fprintf(stderr, "sim: op%d %02X=%02X\n", op, a, d);
        if (op == 4) s->mem[a] = (uint8_t)d;
        else if (op == 6) {
            s->reg[a] = (uint8_t)d;
            if (a == 0xFA && (s->reg[0xF7] & 0x10)) s->bank = d;
            if (a == 0xFF && d == 0x12) s->pending = 1; /* start SSC after edge */
        }
    }
}

static void clock_fall(sim_t *s)
{
    if (s->pending == 1) { s->pending = -1; ssc(s, s->reg[0xF0]); return; }
    if (s->driving) {
        /* nbits = clocks of this word so far; data cells are clocks 12..19
         * and appear on SDATA after the falling edge of their own clock */
        int i = s->nbits;
        if (i >= 13 && i <= 20) s->drive_val = (s->rdata >> (20 - i)) & 1;
        else s->drive_val = 0;
    }
}

static void set_out(sim_t *s, uint8_t v)
{
    uint8_t old = s->out;
    s->out = v;
    if (!(s->dir & 0x08)) return;
    int was = (old >> 3) & 1, now = (v >> 3) & 1;
    if (!was && now) clock_rise(s);
    if (was && !now) clock_fall(s);
    /* D0 high = XRES asserted (or power off): reset the ISSP state */
    if ((v & 1) && (s->dir & 1)) { s->nbits = 0; s->driving = 0; s->busy = 0; }
}

static int sim_xfer(void *c, const uint8_t *out, int outlen, uint8_t *in, int inlen)
{
    sim_t *s = c;
    s->inlen = 0;
    for (int p = 0; p < outlen;) {
        uint8_t cmd = out[p];
        int plen = outlen - p > 32 ? 32 : outlen - p;
        if (cmd == 0xAB) {
            for (int j = 1; j < plen; j++) {
                uint8_t x = out[p + j];
                if (x == 0x20) break;
                if ((x & 0xC0) == 0x80) set_out(s, x & 0x3F);
                else if ((x & 0xC0) == 0x40) s->dir = x & 0x3F;
                else if (x == 0x00) s->inq[s->inlen++] = pins(s);
            }
        } else if (cmd == 0xA8) { /* LSB-first hardware SPI on D3/D5/D7 */
            s->dir |= 0x28;
            for (int j = 1; j < plen; j++) {
                uint8_t o = out[p + j], r = 0;
                for (int b = 0; b < 8; b++) {
                    set_out(s, (uint8_t)((s->out & ~0x28) | (((o >> b) & 1) << 5)));
                    set_out(s, s->out | 0x08);
                    if (sim_sdata(s)) r |= (uint8_t)(1 << b);
                    set_out(s, s->out & ~0x08);
                }
                s->inq[s->inlen++] = r;
            }
        } else if (cmd == 0xA0) {
            if (s->busy > 0) s->busy -= 4; /* time passes while polling */
            uint8_t st[6] = { pins(s), 0, 0, 0, 0, 0 };
            memcpy(s->inq + s->inlen, st, 6);
            s->inlen += 6;
            plen = 1;
        } else if (cmd == 0xAA) {
            plen = 3;
        }
        p += plen;
    }
    if (inlen > s->inlen) {
        fprintf(stderr, "sim: short read (%d < %d)\n", s->inlen, inlen);
        return -1;
    }
    if (inlen) memcpy(in, s->inq, (size_t)inlen);
    return inlen;
}

static void sim_close(void *c)
{
    sim_t *s = c;
    const char *st = getenv("PSOCPROG_SIM_STATE");
    if (st) {
        FILE *f = fopen(st, "wb");
        if (f) { fwrite(s->flash, 1, sizeof s->flash, f); fwrite(s->prot, 1, sizeof s->prot, f); fclose(f); }
    }
    const char *dump = getenv("PSOCPROG_SIM_STATS");
    if (dump) fprintf(stderr, "sim: %d SCLK cycles\n", s->stats_clocks);
    free(s);
}

ch341_t *ch341_open_sim(const char *chip)
{
    sim_t *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    if (!chip || !*chip) chip = "CY8C27443";
    s->dev = issp_find_by_name(chip);
    if (!s->dev) {
        fprintf(stderr, "sim: unknown chip %s\n", chip);
        free(s);
        return NULL;
    }
    s->pending = -1;
    /* pre-fill flash with garbage so erase is observable */
    memset(s->flash, 0xA5, sizeof s->flash);
    const char *st = getenv("PSOCPROG_SIM_STATE"); /* persist flash between runs */
    if (st) {
        FILE *f = fopen(st, "rb");
        if (f) {
            if (fread(s->flash, 1, sizeof s->flash, f) != sizeof s->flash ||
                fread(s->prot, 1, sizeof s->prot, f) != sizeof s->prot)
                memset(s->prot, 0, sizeof s->prot);
            fclose(f);
        }
    }
    return ch341_proto_new("sim", sim_xfer, s, sim_close);
}
