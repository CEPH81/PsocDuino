/* ch341_dll.c - Windows backend through the vendor CH341DLL.DLL, so the
 * stock WCH driver (CH341WDM / CH341PAR) keeps working: no Zadig needed. */
#include "ch341.h"
#include <stdio.h>

#ifndef _WIN32
ch341_t *ch341_open_dll(int index)
{
    (void)index;
    fprintf(stderr, "psocprog: the 'dll' backend is only available on Windows\n");
    return NULL;
}
#else
#include <windows.h>
#include <stdlib.h>

typedef HANDLE(WINAPI *open_t)(ULONG);
typedef VOID(WINAPI *close_t)(ULONG);
typedef BOOL(WINAPI *d5d0_t)(ULONG, ULONG, ULONG);
typedef BOOL(WINAPI *getin_t)(ULONG, PULONG);
typedef BOOL(WINAPI *stream_t)(ULONG, ULONG);
typedef BOOL(WINAPI *bits_t)(ULONG, ULONG, PVOID);
typedef BOOL(WINAPI *spi4_t)(ULONG, ULONG, ULONG, PVOID);

typedef struct {
    HMODULE lib;
    ULONG idx;
    close_t close;
    d5d0_t d5d0;
    getin_t getin;
    bits_t bits;
    spi4_t spi4;
} dll_t;

static int w_d5d0(ch341_t *d, uint8_t dir, uint8_t data)
{
    dll_t *w = d->priv;
    return w->d5d0(w->idx, dir, data) ? 0 : -1;
}
static int w_getin(ch341_t *d, uint32_t *st)
{
    dll_t *w = d->priv;
    ULONG v = 0;
    if (!w->getin(w->idx, &v)) return -1;
    *st = v;
    return 0;
}
static int w_bits(ch341_t *d, uint8_t *buf, size_t n)
{
    dll_t *w = d->priv;
    while (n) {
        ULONG m = n > 1022 ? 1022 : (ULONG)n; /* DLL limit is 1024 */
        if (!w->bits(w->idx, m, buf)) return -1;
        buf += m;
        n -= m;
    }
    return 0;
}
static int w_spi(ch341_t *d, uint8_t *buf, size_t n)
{
    dll_t *w = d->priv;
    return w->spi4(w->idx, 0, (ULONG)n, buf) ? 0 : -1;
}
static void w_close(ch341_t *d)
{
    dll_t *w = d->priv;
    w->close(w->idx);
    FreeLibrary(w->lib);
    free(w);
    free(d);
}

ch341_t *ch341_open_dll(int index)
{
#ifdef _WIN64
    HMODULE lib = LoadLibraryA("CH341DLLA64.DLL"); /* 64-bit WCH package */
#else
    HMODULE lib = LoadLibraryA("CH341DLL.DLL"); /* 32-bit, ships with the original tool */
#endif
    if (!lib) {
        fprintf(stderr, "psocprog: CH341DLL.DLL not found (install the WCH CH341PAR driver "
                        "or put the DLL next to psocprog.exe)\n");
        return NULL;
    }
    dll_t *w = calloc(1, sizeof *w);
    ch341_t *d = calloc(1, sizeof *d);
    open_t op = (open_t)GetProcAddress(lib, "CH341OpenDevice");
    stream_t ss = (stream_t)GetProcAddress(lib, "CH341SetStream");
    w->lib = lib;
    w->idx = (ULONG)index;
    w->close = (close_t)GetProcAddress(lib, "CH341CloseDevice");
    w->d5d0 = (d5d0_t)GetProcAddress(lib, "CH341Set_D5_D0");
    w->getin = (getin_t)GetProcAddress(lib, "CH341GetInput");
    w->bits = (bits_t)GetProcAddress(lib, "CH341BitStreamSPI");
    w->spi4 = (spi4_t)GetProcAddress(lib, "CH341StreamSPI4");
    if (!op || !ss || !w->close || !w->d5d0 || !w->getin || !w->bits || !w->spi4) {
        fprintf(stderr, "psocprog: CH341DLL.DLL is missing required functions\n");
        goto fail;
    }
    if (op(w->idx) == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "psocprog: CH341A not present (is the programmer plugged in?)\n");
        goto fail;
    }
    ss(w->idx, 0x80); /* MSB first, as the original tool */
    d->name = "dll";
    d->priv = w;
    d->set_d5_d0 = w_d5d0;
    d->get_input = w_getin;
    d->bitstream = w_bits;
    d->spi = w_spi;
    d->close = w_close;
    return d;
fail:
    FreeLibrary(lib);
    free(w);
    free(d);
    return NULL;
}
#endif
