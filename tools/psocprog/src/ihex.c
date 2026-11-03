#include "ihex.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int hexv(const char *s, int n, unsigned *out)
{
    unsigned v = 0;
    for (int i = 0; i < n; i++) {
        int c = s[i];
        if (!isxdigit(c)) return -1;
        v = v << 4 | (unsigned)(isdigit(c) ? c - '0' : (tolower(c) - 'a' + 10));
    }
    *out = v;
    return 0;
}

int ihex_load(const char *path, ihex_t *h, char *err, int errlen)
{
    FILE *f = fopen(path, "r");
    char line[600];
    unsigned base = 0;
    int ln = 0, eof = 0;
    memset(h, 0, sizeof *h);
    if (!f) { snprintf(err, (size_t)errlen, "cannot open %s", path); return -1; }
    while (fgets(line, sizeof line, f)) {
        ln++;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\r' || *p == '\n' || !*p) continue;
        if (*p != ':') { snprintf(err, (size_t)errlen, "line %d: not Intel HEX", ln); goto bad; }
        p++;
        unsigned cnt, addr, type, v, sum = 0;
        if (hexv(p, 2, &cnt) || hexv(p + 2, 4, &addr) || hexv(p + 6, 2, &type)) {
            snprintf(err, (size_t)errlen, "line %d: bad record", ln); goto bad;
        }
        uint8_t data[256];
        sum = cnt + (addr >> 8) + (addr & 0xFF) + type;
        for (unsigned i = 0; i <= cnt; i++) {
            if (hexv(p + 8 + 2 * i, 2, &v)) { snprintf(err, (size_t)errlen, "line %d: truncated", ln); goto bad; }
            if (i < cnt) data[i] = (uint8_t)v;
            sum += v;
        }
        if (sum & 0xFF) { snprintf(err, (size_t)errlen, "line %d: record checksum error", ln); goto bad; }
        if (type == 0) {
            for (unsigned i = 0; i < cnt; i++) {
                unsigned a = base + addr + i;
                if (a < IHEX_FLASH_MAX) {
                    h->flash[a] = data[i];
                    if ((int)a + 1 > h->flash_len) h->flash_len = (int)a + 1;
                } else if (a >= 0x100000 && a < 0x100000 + IHEX_SEC_MAX) {
                    h->sec[a - 0x100000] = data[i];
                    if ((int)(a - 0x100000) + 1 > h->sec_len) h->sec_len = (int)(a - 0x100000) + 1;
                } else if (a == 0x200000 || a == 0x200001) {
                    h->has_checksum = 1;
                    if (a == 0x200000) h->checksum = (uint16_t)((h->checksum & 0xFF) | data[i] << 8);
                    else h->checksum = (uint16_t)((h->checksum & 0xFF00) | data[i]);
                } else {
                    snprintf(err, (size_t)errlen, "line %d: address 0x%X outside flash", ln, a);
                    goto bad;
                }
            }
        } else if (type == 1) { eof = 1; break; }
        else if (type == 2 && cnt == 2) base = (unsigned)(data[0] << 8 | data[1]) << 4;
        else if (type == 4 && cnt == 2) base = (unsigned)(data[0] << 8 | data[1]) << 16;
        /* types 3/5 (start address) ignored */
    }
    fclose(f);
    if (!eof) { snprintf(err, (size_t)errlen, "missing end-of-file record"); return -1; }
    if (!h->flash_len) { snprintf(err, (size_t)errlen, "no flash data in file"); return -1; }
    return 0;
bad:
    fclose(f);
    return -1;
}

int ihex_save(const char *path, const uint8_t *data, int len)
{
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    for (int a = 0; a < len; a += 16) {
        int n = len - a < 16 ? len - a : 16;
        unsigned sum = (unsigned)n + ((unsigned)a >> 8) + ((unsigned)a & 0xFF);
        fprintf(f, ":%02X%04X00", n, a);
        for (int i = 0; i < n; i++) { fprintf(f, "%02X", data[a + i]); sum += data[a + i]; }
        fprintf(f, "%02X\n", (0x100 - (sum & 0xFF)) & 0xFF);
    }
    fprintf(f, ":00000001FF\n");
    return fclose(f);
}
