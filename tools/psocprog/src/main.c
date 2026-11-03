/* psocprog - programador ISSP para Cypress PSoC 1 con un CH341A.
 * Proyecto PsocDuino.  Reimplementacion multiplataforma y mejorada de
 * "PSoC Programmer CH341A v1.22" (antiradio.narod.ru), pensada para usarse
 * desde el Arduino IDE o la linea de comandos. */
#include "ch341.h"
#include "issp.h"
#include "ihex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define PSOCPROG_VERSION "1.0.0"

typedef enum { A_IDENT, A_WRITE, A_READ, A_ERASE, A_CHECKSUM, A_LIST } action_t;

static int quiet = 0;
#define say(...) do { if (!quiet) { printf(__VA_ARGS__); fflush(stdout); } } while (0)

static void usage(void)
{
    printf(
"psocprog %s - programador PSoC 1 (ISSP) con CH341A  [PsocDuino]\n\n"
"Uso: psocprog [opciones] [accion]\n\n"
"Acciones:\n"
"  -i, --identify          detectar el chip y mostrar su ID (por defecto)\n"
"  -w, --write FILE.hex    borrar, programar, verificar y proteger\n"
"  -r, --read FILE.hex     leer la flash completa a un archivo\n"
"  -e, --erase             borrar el chip completo\n"
"  -k, --checksum          checksum de la flash del chip\n"
"      --list              lista de chips soportados\n\n"
"Opciones:\n"
"  -c, --chip NAME         chip esperado (auto = cualquiera detectado)\n"
"  -m, --mode xres|power   entrada a programacion: XRES o encendido (xres)\n"
"  -V, --vdd 5|3.3         tension de programacion del chip (5)\n"
"  -b, --backend B         auto | dll | libusb | sim[:CHIP]   (auto)\n"
"  -n, --index N           numero de CH341A si hay varios (0)\n"
"      --verify M          full | checksum | none   (full)\n"
"      --no-secure         no escribir los bits de seguridad del .hex\n"
"      --protect           proteger toda la flash (modo W) al terminar\n"
"      --run | --hold      al terminar: dejar el chip corriendo | en reset (run)\n"
"      --force             programar aunque el ID no coincida con --chip\n"
"      --retries N         reintentos de conexion (3)\n"
"      --power-off-ms N    tiempo sin Vdd en modo power (100)\n"
"  -q, --quiet / -v, --verbose / --version / -h, --help\n\n"
"Conexion CH341A -> PSoC: D3=SCLK(P1[1])  D5 -[1k]- SDATA(P1[0]) -- D7\n"
"                        D0=XRES (modo xres) o control de Vdd (modo power)\n",
    PSOCPROG_VERSION);
}

#ifdef _WIN32
#include <windows.h>
static double now_s(void) { return (double)GetTickCount64() / 1000.0; }
#else
static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec / 1e9;
}
#endif

static ch341_t *open_backend(const char *b, int index)
{
    if (!strncmp(b, "sim", 3)) return ch341_open_sim(b[3] == ':' ? b + 4 : NULL);
    if (!strcmp(b, "dll")) return ch341_open_dll(index);
    if (!strcmp(b, "libusb")) return ch341_open_libusb(index);
#ifdef _WIN32
    ch341_t *d = ch341_open_dll(index);
    if (d) return d;
#endif
    return ch341_open_libusb(index);
}

static void progress(const char *what, int done, int total, int *last)
{
    int pct = total ? done * 100 / total : 100;
    if (pct / 10 != *last / 10 || done == total) {
        say("  %s %3d%%  (%d/%d bloques)\n", what, pct, done, total);
        *last = pct;
    }
}

static int connect_retry(issp_t *t, int retries)
{
    int e = ISSP_ERR_NOTARGET;
    for (int i = 0; i <= retries; i++) {
        e = issp_connect(t);
        if (e == ISSP_OK || e == ISSP_ERR_USB) return e;
    }
    return e;
}

int main(int argc, char **argv)
{
    action_t act = A_IDENT;
    const char *file = NULL, *chip = "auto", *backend = "auto", *verify = "full";
    int index = 0, run = 1, force = 0, retries = 3, no_secure = 0, protect = 0;
    issp_t t;
    memset(&t, 0, sizeof t);
    t.power_off_ms = 100;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
#define ARG() (i + 1 < argc ? argv[++i] : (fprintf(stderr, "falta valor para %s\n", a), exit(2), ""))
        if (!strcmp(a, "-w") || !strcmp(a, "--write")) { act = A_WRITE; file = ARG(); }
        else if (!strcmp(a, "-r") || !strcmp(a, "--read")) { act = A_READ; file = ARG(); }
        else if (!strcmp(a, "-e") || !strcmp(a, "--erase")) act = A_ERASE;
        else if (!strcmp(a, "-i") || !strcmp(a, "--identify")) act = A_IDENT;
        else if (!strcmp(a, "-k") || !strcmp(a, "--checksum")) act = A_CHECKSUM;
        else if (!strcmp(a, "--list")) act = A_LIST;
        else if (!strcmp(a, "-c") || !strcmp(a, "--chip")) chip = ARG();
        else if (!strcmp(a, "-m") || !strcmp(a, "--mode")) {
            const char *m = ARG();
            if (!strcasecmp(m, "xres")) t.power_mode = 0;
            else if (!strcasecmp(m, "power")) t.power_mode = 1;
            else { fprintf(stderr, "modo invalido: %s\n", m); return 2; }
        } else if (!strcmp(a, "-V") || !strcmp(a, "--vdd")) {
            const char *v = ARG();
            if (!strcmp(v, "5")) t.v3 = 0;
            else if (!strcmp(v, "3.3") || !strcmp(v, "3")) t.v3 = 1;
            else { fprintf(stderr, "tension invalida: %s\n", v); return 2; }
        } else if (!strcmp(a, "-b") || !strcmp(a, "--backend")) backend = ARG();
        else if (!strcmp(a, "-n") || !strcmp(a, "--index")) index = atoi(ARG());
        else if (!strcmp(a, "--verify")) verify = ARG();
        else if (!strcmp(a, "--no-secure")) no_secure = 1;
        else if (!strcmp(a, "--protect")) protect = 1;
        else if (!strcmp(a, "--run")) run = 1;
        else if (!strcmp(a, "--hold")) run = 0;
        else if (!strcmp(a, "--force")) force = 1;
        else if (!strcmp(a, "--retries")) retries = atoi(ARG());
        else if (!strcmp(a, "--power-off-ms")) t.power_off_ms = atoi(ARG());
        else if (!strcmp(a, "-q") || !strcmp(a, "--quiet")) quiet = 1;
        else if (!strcmp(a, "-v") || !strcmp(a, "--verbose")) ch341_verbose = 1;
        else if (!strcmp(a, "--version")) { printf("psocprog %s\n", PSOCPROG_VERSION); return 0; }
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(); return 0; }
        else { fprintf(stderr, "opcion desconocida: %s (use --help)\n", a); return 2; }
    }
    if (strcmp(verify, "full") && strcmp(verify, "checksum") && strcmp(verify, "none")) {
        fprintf(stderr, "--verify debe ser full, checksum o none\n");
        return 2;
    }

    if (act == A_LIST) {
        printf("%-20s %-6s %-6s %s\n", "Chip", "ID", "Flash", "Bancos");
        for (unsigned i = 0; i < PSOC_NUM_DEVICES; i++)
            printf("%-20s %02X%02X   %2dK    %d\n", PSOC_DEVICES[i].name, PSOC_DEVICES[i].id_hi,
                   PSOC_DEVICES[i].id_lo, PSOC_DEVICES[i].kb, PSOC_DEVICES[i].banks);
        return 0;
    }

    const psoc_device_t *want = NULL;
    if (strcasecmp(chip, "auto")) {
        want = issp_find_by_name(chip);
        if (!want) { fprintf(stderr, "chip desconocido: %s (use --list)\n", chip); return 2; }
    }

    static ihex_t hex;
    if (act == A_WRITE) {
        char err[200];
        if (ihex_load(file, &hex, err, sizeof err)) {
            fprintf(stderr, "error en %s: %s\n", file, err);
            return 1;
        }
    }

    double t0 = now_s();
    t.d = open_backend(backend, index);
    if (!t.d) return 1;
    say("psocprog %s  backend=%s  modo=%s  Vdd=%s\n", PSOCPROG_VERSION, t.d->name,
        t.power_mode ? "power" : "xres", t.v3 ? "3.3V" : "5V");

    int rc = 1, e;
    if ((e = connect_retry(&t, retries))) {
        fprintf(stderr, "error: %s\n", issp_strerror(e));
        goto out;
    }
    const psoc_device_t *dev = issp_find_by_id(t.id);
    say("ID de silicio: 0x%04X -> %s\n", t.id, dev ? dev->name : "desconocido");
    if (!dev) {
        if (!(want && force)) {
            fprintf(stderr, "error: chip no reconocido (ID 0x%04X). Revise el cableado; "
                            "use --chip X --force para forzar.\n", t.id);
            goto out;
        }
        dev = want;
    } else if (want && want != dev) {
        if (!force) {
            fprintf(stderr, "error: se esperaba %s pero se detecto %s "
                            "(elija el chip correcto o use --force)\n", want->name, dev->name);
            goto out;
        }
        dev = want;
    }
    issp_set_device(&t, dev);
    say("Chip: %s  flash=%dK  bancos=%d  bloques=%d\n", dev->name, dev->kb, dev->banks, t.blocks);

    if (act == A_IDENT) { rc = 0; goto out; }

    if (act == A_CHECKSUM) {
        uint16_t s;
        if ((e = issp_checksum(&t, &s))) { fprintf(stderr, "error: %s\n", issp_strerror(e)); goto out; }
        say("Checksum del chip: 0x%04X\n", s);
        rc = 0;
        goto out;
    }

    if (act == A_ERASE) {
        say("Borrando...\n");
        if ((e = issp_erase(&t))) { fprintf(stderr, "error al borrar: %s\n", issp_strerror(e)); goto out; }
        say("Chip borrado.\n");
        rc = 0;
        goto out;
    }

    if (act == A_READ) {
        static uint8_t img[IHEX_FLASH_MAX];
        int prot = 0, last = -1, done = 0;
        say("Leyendo...\n");
        for (int bank = 0; bank < dev->banks; bank++) {
            if (bank * t.bpb >= t.blocks) break;
            if ((e = issp_set_bank(&t, bank))) break;
            for (int b = 0; b < t.bpb && bank * t.bpb + b < t.blocks; b++) {
                uint8_t *p = img + (bank * t.bpb + b) * 64;
                e = issp_read_block(&t, bank, b, p);
                if (e == ISSP_ERR_PROTECTED) { prot++; memset(p, 0, 64); e = 0; }
                if (e) break;
                progress("Leyendo", ++done, t.blocks, &last);
            }
            if (e) break;
        }
        if (e) { fprintf(stderr, "error de lectura: %s\n", issp_strerror(e)); goto out; }
        if (prot) say("Aviso: %d bloques protegidos (leidos como 00).\n", prot);
        if (ihex_save(file, img, t.blocks * 64)) { fprintf(stderr, "no se pudo escribir %s\n", file); goto out; }
        say("Guardado %s (%d bytes).\n", file, t.blocks * 64);
        rc = 0;
        goto out;
    }

    /* ---- A_WRITE ---- */
    int size = dev->kb * 1024;
    if (hex.flash_len > size) {
        fprintf(stderr, "error: el programa ocupa %d bytes y %s solo tiene %d\n", hex.flash_len,
                dev->name, size);
        goto out;
    }
    int nblk = (hex.flash_len + 63) / 64;
    unsigned img_sum = 0;
    for (int i = 0; i < nblk * 64; i++) img_sum += hex.flash[i];
    say("Programa: %d bytes (%d bloques, %d%% de la flash)\n", hex.flash_len, nblk,
        hex.flash_len * 100 / size);
    if (hex.has_checksum && hex.checksum != (uint16_t)img_sum)
        say("Aviso: el checksum del .hex (0x%04X) no coincide con los datos (0x%04X)\n",
            hex.checksum, (uint16_t)img_sum);

    say("Borrando...\n");
    if ((e = issp_erase(&t))) { fprintf(stderr, "error al borrar: %s\n", issp_strerror(e)); goto out; }
    if ((e = connect_retry(&t, retries))) { fprintf(stderr, "error: %s\n", issp_strerror(e)); goto out; }

    int last = -1, done = 0;
    for (int bank = 0; bank < dev->banks && done < nblk; bank++) {
        if ((e = issp_set_bank(&t, bank))) break;
        for (int b = 0; b < t.bpb && done < nblk; b++) {
            if ((e = issp_program_block(&t, bank, b, hex.flash + done * 64))) break;
            progress("Programando", ++done, nblk, &last);
        }
        if (e) break;
    }
    if (e) {
        fprintf(stderr, "error programando el bloque %d: %s\n", done, issp_strerror(e));
        goto out;
    }

    if (!strcmp(verify, "full")) {
        if ((e = connect_retry(&t, retries))) { fprintf(stderr, "error: %s\n", issp_strerror(e)); goto out; }
        last = -1; done = 0;
        int bad = -1;
        for (int bank = 0; bank < dev->banks && done < nblk && bad < 0; bank++) {
            if ((e = issp_set_bank(&t, bank))) break;
            for (int b = 0; b < t.bpb && done < nblk; b++) {
                uint8_t rd[64];
                if ((e = issp_read_block(&t, bank, b, rd))) break;
                if (memcmp(rd, hex.flash + done * 64, 64)) {
                    for (int k = 0; k < 64; k++)
                        if (rd[k] != hex.flash[done * 64 + k]) { bad = done * 64 + k; break; }
                    fprintf(stderr, "error de verificacion en 0x%04X: leido %02X, esperado %02X\n",
                            bad, rd[bad % 64], hex.flash[bad]);
                    break;
                }
                progress("Verificando", ++done, nblk, &last);
            }
            if (e) break;
        }
        if (e) { fprintf(stderr, "error verificando: %s\n", issp_strerror(e)); goto out; }
        if (bad >= 0) goto out;
    }

    int sec_any = 0;
    for (int i = 0; i < hex.sec_len; i++) sec_any |= hex.sec[i];
    if (protect) { /* todos los bloques en modo W (proteccion total) */
        hex.sec_len = t.blocks / 4;
        memset(hex.sec, 0xFF, (size_t)hex.sec_len);
        sec_any = 1;
    }
    if (hex.sec_len && sec_any && !no_secure) {
        say("Escribiendo bits de seguridad...\n");
        if ((e = issp_secure(&t, hex.sec, hex.sec_len))) {
            fprintf(stderr, "error en seguridad: %s\n", issp_strerror(e));
            goto out;
        }
    }

    if (strcmp(verify, "none")) {
        uint16_t s;
        if ((e = issp_checksum(&t, &s))) { fprintf(stderr, "error de checksum: %s\n", issp_strerror(e)); goto out; }
        if (s == (uint16_t)img_sum) say("Checksum OK: 0x%04X\n", s);
        else if (!strcmp(verify, "checksum")) {
            fprintf(stderr, "error: checksum del chip 0x%04X, esperado 0x%04X\n", s, (uint16_t)img_sum);
            goto out;
        } else say("Aviso: checksum del chip 0x%04X, esperado 0x%04X (los datos se verificaron byte a byte)\n",
                   s, (uint16_t)img_sum);
    }
    say("Listo: %s programado en %.1f s.\n", dev->name, now_s() - t0);
    rc = 0;

out:
    if (t.d) {
        issp_release(&t, run && rc == 0);
        t.d->close(t.d);
    }
    return rc;
}
