/* issp.c - PSoC 1 ISSP algorithm.
 *
 * Sequence and timing checks reproduce "PSoC Programmer CH341A v1.22"
 * (reverse engineered), with these changes:
 *   - unknown silicon IDs are rejected instead of being treated as CY8C27xxx;
 *   - CHECKSUM-SETUP is patched with the real number of blocks per bank
 *     (the original used fixed 32/64/128 block counts per family, giving a
 *     partial checksum on several 8K/16K parts);
 *   - configurable Vdd-off time in power-on mode;
 *   - the target can be left running (XRES/power released) after programming.
 */
#include "issp.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(int ms) { Sleep((DWORD)ms); }
static unsigned long now_ms(void) { return (unsigned long)GetTickCount(); }
#else
#include <time.h>
static void sleep_ms(int ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}
static unsigned long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)ts.tv_sec * 1000UL + (unsigned long)(ts.tv_nsec / 1000000L);
}
#endif

#define BIT_SDATA 0x20  /* D5 */
#define PIN_DIR 0x29    /* D0 (XRES/power), D3 (SCLK), D5 (SDATA out) */
#define POLL_MAX 500

static uint8_t rx[2048];

const char *issp_strerror(int e)
{
    switch (e) {
    case ISSP_OK: return "ok";
    case ISSP_ERR_USB: return "fallo la comunicacion USB con el CH341A";
    case ISSP_ERR_NOTARGET: return "el PSoC no responde (revise cableado, alimentacion y modo xres/power)";
    case ISSP_ERR_FAIL: return "el PSoC no inicio la operacion (proceso fallido, intente de nuevo)";
    case ISSP_ERR_TIMEOUT: return "tiempo de espera agotado (PSoC ocupado)";
    case ISSP_ERR_PROTECTED: return "bloque de flash protegido contra lectura";
    }
    return "error desconocido";
}

const psoc_device_t *issp_find_by_id(uint16_t id)
{
    for (unsigned i = 0; i < PSOC_NUM_DEVICES; i++)
        if (((PSOC_DEVICES[i].id_hi << 8) | PSOC_DEVICES[i].id_lo) == id) return &PSOC_DEVICES[i];
    return NULL;
}

const psoc_device_t *issp_find_by_name(const char *name)
{
    for (unsigned i = 0; i < PSOC_NUM_DEVICES; i++)
        if (!strcasecmp(PSOC_DEVICES[i].name, name)) return &PSOC_DEVICES[i];
    /* allow "CY8C21234" for "CY8C21234(W)" and ordering suffixes */
    size_t n = strlen(name);
    for (unsigned i = 0; i < PSOC_NUM_DEVICES; i++)
        if (!strncasecmp(PSOC_DEVICES[i].name, name, n) &&
            (PSOC_DEVICES[i].name[n] == '(' || PSOC_DEVICES[i].name[n] == '-'))
            return &PSOC_DEVICES[i];
    return NULL;
}

void issp_set_device(issp_t *t, const psoc_device_t *dev)
{
    t->dev = dev;
    t->bpb = dev->banks >= 2 ? 128 : dev->kb * 16;
    t->blocks = dev->kb * 16;
}

/* Send a '0'/'1' vector followed by pad zero clocks; rx[] gets SDATA samples. */
static int send_vec(issp_t *t, const char *v, int pad, int *lenp)
{
    int n = (int)strlen(v);
    if (n + pad > (int)sizeof rx) return ISSP_ERR_USB;
    for (int i = 0; i < n; i++) rx[i] = v[i] == '1' ? BIT_SDATA : 0;
    memset(rx + n, 0, (size_t)pad);
    if (lenp) *lenp = n;
    if (t->d->bitstream(t->d, rx, (size_t)(n + pad))) return ISSP_ERR_USB;
    return ISSP_OK;
}

static int hi(int i) { return (rx[i] & 0x80) != 0; }

static int fail_at(const char *stage, int len)
{
    if (ch341_verbose)
        fprintf(stderr, "[issp] %s: SDATA tras el vector = %d%d%d%d ... ultimo=%d\n", stage,
                hi(len), hi(len + 1), hi(len + 2), hi(len + 3), hi(len + 18));
    return ISSP_ERR_NOTARGET;
}

static uint8_t rx_byte(int first)
{
    uint8_t v = 0;
    for (int i = first; i < first + 8; i++) v = (uint8_t)(v << 1 | hi(i));
    return v;
}

/* WAIT-AND-POLL: SDATA must go high right after the vector, then low.
 * The original tool polls GetInput ~500 times and then simply continues even if
 * SDATA is still high. Fast USB hosts can finish those 500 reads before a flash
 * block is written, so poll by wall clock (up to 2 s) and, like the original,
 * continue afterwards: the full verification catches a block that failed. */
#define POLL_MS 2000
static int wait_poll(issp_t *t, int len)
{
    uint32_t st;
    unsigned long t0;
    if (!hi(len + 2)) return ISSP_ERR_FAIL;
    for (int i = 0; i <= POLL_MAX + 1; i++) {
        if (t->d->get_input(t->d, &st)) return ISSP_ERR_USB;
        if (!(st & 0x80)) return ISSP_OK;
    }
    t0 = now_ms();
    while (now_ms() - t0 < POLL_MS) {
        sleep_ms(1);
        if (t->d->get_input(t->d, &st)) return ISSP_ERR_USB;
        if (!(st & 0x80)) return ISSP_OK;
    }
    if (ch341_verbose) fprintf(stderr, "[issp] SDATA sigue en alto tras %d ms; se continua como el original\n", POLL_MS);
    return ISSP_OK;
}

static int exec_vec(issp_t *t, const char *v)
{
    int len, e = send_vec(t, v, 10, &len);
    return e ? e : wait_poll(t, len);
}

/* copy vector and replace 8 bits at position pos with value (MSB first) */
static void put8(char *s, int pos, unsigned v)
{
    for (int i = 0; i < 8; i++) s[pos + i] = (v >> (7 - i)) & 1 ? '1' : '0';
}

int issp_connect(issp_t *t)
{
    int len, e;
    ch341_t *d = t->d;
    if (d->set_d5_d0(d, PIN_DIR, 0x01)) return ISSP_ERR_USB; /* XRES high / Vdd off */
    if (t->power_mode) {
        sleep_ms(t->power_off_ms);
        if (d->set_d5_d0(d, PIN_DIR, 0x00)) return ISSP_ERR_USB; /* Vdd on */
        int i;
        for (i = 0; i < 100; i++) { /* wait for SDATA low after POR */
            uint32_t st;
            if (d->get_input(d, &st)) return ISSP_ERR_USB;
            if (!(st & 0x80)) break;
        }
        if (i >= 100) return ISSP_ERR_NOTARGET;
    }
    /* INIT-1 (D0 goes low with the first bit: XRES released) */
    if ((e = send_vec(t, V_INIT1, 368, &len))) return e;
    if (!hi(len + 2) || hi(len + 367)) return fail_at("INIT-1", len);
    if ((e = send_vec(t, V_INIT2, 40, &len))) return e;
    if (!hi(len + 2) || hi(len + 39)) return fail_at("INIT-2", len);
    if ((e = send_vec(t, V_INIT3_PRE, 0, NULL))) return e;
    if ((e = send_vec(t, t->v3 ? V_INIT3_3V3 : V_INIT3_5V, 0, NULL))) return e;
    if ((e = send_vec(t, V_ID_SETUP, 20, &len))) return e;
    if (!hi(len + 2) || hi(len + 19)) return fail_at("ID-SETUP", len);
    if ((e = send_vec(t, V_READ_ID, 0, NULL))) return e;
    t->id = (uint16_t)(rx_byte(13) << 8 | rx_byte(35));
    return ISSP_OK;
}

int issp_erase(issp_t *t) { return exec_vec(t, V_ERASE); }

int issp_set_bank(issp_t *t, int bank)
{
    if (t->dev->banks < 2) return ISSP_OK;
    char v[80];
    strcpy(v, V_SET_BANK);
    put8(v, 33, (unsigned)bank);
    return send_vec(t, v, 0, NULL);
}

static int set_block(issp_t *t, int block)
{
    char v[32];
    strcpy(v, V_SET_BLOCK);
    put8(v, 11, (unsigned)block);
    return send_vec(t, v, 0, NULL);
}

int issp_program_block(issp_t *t, int bank, int block, const uint8_t data[64])
{
    (void)bank;
    uint8_t spi[192];
    int e;
    /* 64 x WRITE-BYTE "10010 aaaaaa dddddddd 111" + 2 idle bits, via hardware SPI */
    for (int k = 0; k < 64; k++) {
        uint32_t w = (0x12u << 19) | ((uint32_t)k << 13) | ((uint32_t)data[k] << 5) | (7u << 2);
        spi[3 * k] = (uint8_t)(w >> 16);
        spi[3 * k + 1] = (uint8_t)(w >> 8);
        spi[3 * k + 2] = (uint8_t)w;
    }
    if (t->d->spi(t->d, spi, sizeof spi)) return ISSP_ERR_USB;
    if ((e = set_block(t, block))) return e;
    return exec_vec(t, t->dev->program_block);
}

int issp_read_block(issp_t *t, int bank, int block, uint8_t data[64])
{
    (void)bank;
    uint8_t spi[192];
    int len, e;
    if ((e = set_block(t, block))) return e;
    if ((e = send_vec(t, V_VERIFY_SETUP, 60, &len))) return e;
    if (!hi(len + 3)) return ISSP_ERR_PROTECTED;
    if (hi(len + 59)) { /* still busy: poll a little longer */
        int i;
        for (i = 0; i < 11; i++) {
            uint32_t st;
            if (t->d->get_input(t->d, &st)) return ISSP_ERR_USB;
            if (!(st & 0x80)) break;
        }
        if (i > 10) return ISSP_ERR_TIMEOUT;
    }
    /* 64 x READ-BYTE "10110 aaaaaa Z dddddddd Z 1" + 2 idle bits */
    for (int k = 0; k < 64; k++) {
        uint32_t w = (0x16u << 19) | ((uint32_t)k << 13) | (1u << 12) | (0xFFu << 4) | (3u << 2);
        spi[3 * k] = (uint8_t)(w >> 16);
        spi[3 * k + 1] = (uint8_t)(w >> 8);
        spi[3 * k + 2] = (uint8_t)w;
    }
    if (t->d->spi(t->d, spi, sizeof spi)) return ISSP_ERR_USB;
    for (int k = 0; k < 64; k++)
        data[k] = (uint8_t)(spi[3 * k + 1] << 5 | ((spi[3 * k + 2] >> 3) & 0x1F));
    return ISSP_OK;
}

int issp_secure(issp_t *t, const uint8_t *sec, int len)
{
    int per_bank = t->bpb / 4, e;
    for (int bank = 0; bank < t->dev->banks; bank++) {
        if (bank * t->bpb >= t->blocks) break;
        if ((e = issp_set_bank(t, bank))) return e;
        for (int i = 0; i < per_bank; i++) {
            char v[32] = "100100000000000000011100"; /* WRITE-BYTE */
            int idx = bank * per_bank + i;
            unsigned val = idx < len ? sec[idx] : 0;
            for (int b = 0; b < 6; b++) v[5 + b] = (i >> (5 - b)) & 1 ? '1' : '0';
            put8(v, 11, val);
            if ((e = send_vec(t, v, 0, NULL))) return e;
        }
        if ((e = exec_vec(t, t->dev->secure))) return e;
    }
    return ISSP_OK;
}

/* find "100 11111010 dddddddd 111" (M[FA] = blocks) in a vector */
static int find_mem_fa(const char *v)
{
    int n = (int)strlen(v), i = 0;
    while (i + 22 <= n) {
        if (v[i] == '0') { i++; continue; }
        if (!strncmp(v + i, "10011111010", 11)) return i + 11;
        i += 22;
    }
    return -1;
}

int issp_checksum(issp_t *t, uint16_t *sum)
{
    unsigned total = 0;
    int e;
    for (int bank = 0; bank < t->dev->banks; bank++) {
        int nblk = t->blocks - bank * t->bpb;
        if (nblk <= 0) break;
        if (nblk > t->bpb) nblk = t->bpb;
        if ((e = issp_set_bank(t, bank))) return e;
        char v[400];
        strcpy(v, t->dev->chksum_setup);
        int pos = find_mem_fa(v);
        if (pos > 0) put8(v, pos, (unsigned)nblk & 0xFF); /* 256 -> 0 */
        if ((e = exec_vec(t, v))) return e;
        if ((e = send_vec(t, V_READ_CHECKSUM, 0, NULL))) return e;
        total += (unsigned)(rx_byte(13) << 8 | rx_byte(35));
    }
    *sum = (uint16_t)total;
    return ISSP_OK;
}

int issp_release(issp_t *t, int run)
{
    /* only D0 stays an output: SCLK/SDATA float, target keeps running */
    return t->d->set_d5_d0(t->d, 0x01, run ? 0x00 : 0x01) ? ISSP_ERR_USB : ISSP_OK;
}
