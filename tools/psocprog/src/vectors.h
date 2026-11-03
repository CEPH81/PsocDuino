/* ISSP vectors and device table for Cypress PSoC 1 - see vectors.c */
#ifndef PSOC_VECTORS_H
#define PSOC_VECTORS_H

extern const char V_INIT1[];
extern const char V_INIT2[];
extern const char V_INIT3_PRE[];
extern const char V_INIT3_3V3[];
extern const char V_INIT3_5V[];
extern const char V_ID_SETUP[];
extern const char V_READ_ID[];
extern const char V_SET_BANK[];
extern const char V_SET_BLOCK[];
extern const char V_VERIFY_SETUP[];
extern const char V_ERASE[];
extern const char V_READ_CHECKSUM[];
extern const char V_CHKSUM_SETUP_A[];
extern const char V_CHKSUM_SETUP_B[];
extern const char V_CHKSUM_SETUP_C[];
extern const char V_CHKSUM_SETUP_D[];
extern const char V_CHKSUM_SETUP_E[];
extern const char V_PROGRAM_BLOCK_A[];
extern const char V_PROGRAM_BLOCK_B[];
extern const char V_PROGRAM_BLOCK_C[];
extern const char V_SECURE_A[];
extern const char V_SECURE_B[];
extern const char V_SECURE_C[];

typedef struct {
    const char *name;
    unsigned char id_hi, id_lo;   /* silicon ID */
    unsigned char kb, banks;      /* flash size (KB) and number of banks */
    const char *chksum_setup, *program_block, *secure; /* family vectors */
} psoc_device_t;

extern const psoc_device_t PSOC_DEVICES[];
extern const unsigned PSOC_NUM_DEVICES;
#endif
