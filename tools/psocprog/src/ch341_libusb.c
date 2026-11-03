/* ch341_libusb.c - CH341A access through libusb-1.0 (Linux, macOS, and
 * Windows with the WinUSB driver installed via Zadig). */
#include "ch341.h"
#include <stdio.h>

#ifndef HAVE_LIBUSB
ch341_t *ch341_open_libusb(int index)
{
    (void)index;
    fprintf(stderr, "psocprog: built without libusb support\n");
    return NULL;
}
#else
#include <libusb.h>
#include <stdlib.h>
#include <string.h>

#define CH341_VID 0x1A86
#define CH341_PID 0x5512
#define EP_OUT 0x02
#define EP_IN 0x82
#define TIMEOUT_MS 2000

typedef struct {
    libusb_context *ctx;
    libusb_device_handle *h;
} lu_t;

static int lu_xfer(void *c, const uint8_t *out, int outlen, uint8_t *in, int inlen)
{
    lu_t *u = c;
    int done, r;
    if (outlen > 0) {
        r = libusb_bulk_transfer(u->h, EP_OUT, (unsigned char *)out, outlen, &done, TIMEOUT_MS);
        if (r || done != outlen) {
            fprintf(stderr, "psocprog: USB write failed: %s\n", libusb_error_name(r));
            return -1;
        }
    }
    int got = 0;
    while (got < inlen) {
        uint8_t tmp[512];
        int want = inlen - got;
        int req = want > (int)sizeof tmp ? (int)sizeof tmp : ((want + 31) / 32) * 32;
        r = libusb_bulk_transfer(u->h, EP_IN, tmp, req, &done, TIMEOUT_MS);
        if (r) {
            fprintf(stderr, "psocprog: USB read failed: %s\n", libusb_error_name(r));
            return -1;
        }
        if (done > want) done = want;
        memcpy(in + got, tmp, (size_t)done);
        got += done;
        if (done == 0) break;
    }
    return got;
}

static void lu_close(void *c)
{
    lu_t *u = c;
    if (u->h) {
        libusb_release_interface(u->h, 0);
        libusb_close(u->h);
    }
    if (u->ctx) libusb_exit(u->ctx);
    free(u);
}

ch341_t *ch341_open_libusb(int index)
{
    lu_t *u = calloc(1, sizeof *u);
    libusb_device **list = NULL;
    if (!u || libusb_init(&u->ctx)) {
        fprintf(stderr, "psocprog: libusb_init failed\n");
        free(u);
        return NULL;
    }
    ssize_t n = libusb_get_device_list(u->ctx, &list);
    int seen = 0;
    for (ssize_t i = 0; i < n && !u->h; i++) {
        struct libusb_device_descriptor dd;
        if (libusb_get_device_descriptor(list[i], &dd)) continue;
        if (dd.idVendor != CH341_VID || dd.idProduct != CH341_PID) continue;
        if (seen++ != index) continue;
        int r = libusb_open(list[i], &u->h);
        if (r) {
            fprintf(stderr, "psocprog: cannot open CH341A: %s%s\n", libusb_error_name(r),
                    r == LIBUSB_ERROR_ACCESS ? " (permission denied: install the udev rule)" : "");
            u->h = NULL;
        }
    }
    if (list) libusb_free_device_list(list, 1);
    if (!u->h) {
        if (!seen) fprintf(stderr, "psocprog: CH341A (1a86:5512) not found\n");
        lu_close(u);
        return NULL;
    }
    libusb_set_auto_detach_kernel_driver(u->h, 1);
    int r = libusb_claim_interface(u->h, 0);
    if (r) {
        fprintf(stderr, "psocprog: cannot claim CH341A interface: %s\n", libusb_error_name(r));
        lu_close(u);
        return NULL;
    }
    return ch341_proto_new("libusb", lu_xfer, u, lu_close);
}
#endif
