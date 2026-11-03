/* issp.h - PSoC 1 In-System Serial Programming over a CH341A */
#ifndef ISSP_H
#define ISSP_H
#include "ch341.h"
#include "vectors.h"

enum { ISSP_OK = 0, ISSP_ERR_USB = -1, ISSP_ERR_NOTARGET = -2, ISSP_ERR_FAIL = -3,
       ISSP_ERR_TIMEOUT = -4, ISSP_ERR_PROTECTED = -5 };

typedef struct {
    ch341_t *d;
    int power_mode;          /* 0 = XRES reset, 1 = power-on reset (D0 switches Vdd) */
    int v3;                  /* 1 = target at 3.3 V, 0 = 5 V */
    int power_off_ms;        /* power-on mode: time with Vdd off before entry */
    const psoc_device_t *dev;
    int bpb;                 /* blocks per bank */
    int blocks;              /* total blocks (flash size / 64) */
    uint16_t id;             /* silicon ID read from the chip */
} issp_t;

const char *issp_strerror(int e);
const psoc_device_t *issp_find_by_id(uint16_t id);
const psoc_device_t *issp_find_by_name(const char *name);
void issp_set_device(issp_t *t, const psoc_device_t *dev);

int issp_connect(issp_t *t);                 /* enter ISSP mode, read ID into t->id */
int issp_erase(issp_t *t);                   /* bulk erase (also clears protection) */
int issp_set_bank(issp_t *t, int bank);
int issp_program_block(issp_t *t, int bank, int block, const uint8_t data[64]);
int issp_read_block(issp_t *t, int bank, int block, uint8_t data[64]);
int issp_secure(issp_t *t, const uint8_t *sec, int len); /* len = blocks/4 */
int issp_checksum(issp_t *t, uint16_t *sum);
int issp_release(issp_t *t, int run);        /* tristate SCLK/SDATA, D0 low if run */
#endif
