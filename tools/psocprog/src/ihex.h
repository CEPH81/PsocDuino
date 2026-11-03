/* ihex.h - Intel HEX as produced by PSoC Designer:
 *   0x000000..          flash image
 *   0x100000..          flash security (1 byte per 4 blocks, 2 bits/block)
 *   0x200000..0x200001  16-bit checksum of the flash image (big endian) */
#ifndef IHEX_H
#define IHEX_H
#include <stdint.h>

#define IHEX_FLASH_MAX 0x10000
#define IHEX_SEC_MAX 256

typedef struct {
    uint8_t flash[IHEX_FLASH_MAX];
    int flash_len;          /* highest flash address written + 1 */
    uint8_t sec[IHEX_SEC_MAX];
    int sec_len;
    int has_checksum;
    uint16_t checksum;
} ihex_t;

int ihex_load(const char *path, ihex_t *h, char *err, int errlen);
int ihex_save(const char *path, const uint8_t *data, int len);
#endif
