/* psocbuild - puente entre el Arduino IDE y el compilador ImageCraft M8C
 * (ICCM8C) de PSoC Designer.  Proyecto PsocDuino.
 *
 *   psocbuild cc      [-v] -V VARIANT -o OUT.o SRC [flags...]   compila C (.c/.cpp/.ino)
 *   psocbuild as      [-v] -V VARIANT -o OUT.o SRC [flags...]   ensambla (.asm/.S)
 *   psocbuild ar      [-v] -V VARIANT ARCHIVE OBJ                agrega a una biblioteca
 *   psocbuild link    [-v] -V VARIANT -o OUTBASE OBJ... LIB...   boot.asm + enlace -> .hex
 *   psocbuild size    MAPFILE                                    uso de flash y RAM
 *   psocbuild preproc SRC OUT                                    (para el IDE: copia)
 *   psocbuild import  PROJECT_DIR CHIP|VARIANT_DIR                crea una variante
 *     (con solo el nombre del chip se guarda en %LOCALAPPDATA%\PsocDuino\variants\CHIP,
 *      fuera del paquete, asi no se pierde al actualizar desde el Gestor de placas)
 *   psocbuild doctor  [-V VARIANT]                               diagnostico de instalacion
 *
 * ImageCraft solo compila C y sus herramientas trabajan con rutas cortas de
 * Windows, como hace PSoC Designer; los mensajes "!E file(line):msg" se
 * convierten a "file:line: error: msg" para que el IDE los resalte.
 * Variables: PSOC_DESIGNER_DIR (carpeta ...\Common\CypressSemiBuildMgr),
 *            PSOCBUILD_TOOLS (carpeta con iccm8c.exe, iasm8c.exe, ...).
 */
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define getcwd _getcwd
#define chdir _chdir
#define mkdir_p(p) _mkdir(p)
#define EXE ".exe"
#else
#include <dirent.h>
#include <unistd.h>
#define mkdir_p(p) mkdir(p, 0755)
#define EXE ""
#endif

#define MAXP 4096
#include "memory_tpl.h"
#define MAXCMD 65536

static int verbose = 0;

/* ------------------------------------------------------------ utilidades */
static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "psocbuild: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (!p) die("sin memoria");
    memcpy(p, s, n);
    return p;
}

static void slashes(char *p)
{
    for (; *p; p++)
        if (*p == '\\') *p = '/';
}

static int exists(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0;
}

static int is_dir(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0 && (st.st_mode & S_IFDIR);
}

static const char *ext(const char *p)
{
    const char *d = strrchr(p, '.'), *s = strrchr(p, '/');
    if (!d || (s && d < s)) return "";
    return d;
}

static char *read_file(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *b = malloc((size_t)n + 1);
    if (!b) die("sin memoria");
    if (fread(b, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(b); return NULL; }
    b[n] = 0;
    fclose(f);
    if (len) *len = n;
    return b;
}

static int write_file(const char *path, const char *data, long n)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t w = fwrite(data, 1, (size_t)n, f);
    return (fclose(f) || w != (size_t)n) ? -1 : 0;
}

static int copy_file(const char *from, const char *to)
{
    long n;
    char *b = read_file(from, &n);
    if (!b) return -1;
    int r = write_file(to, b, n);
    free(b);
    return r;
}

/* ruta absoluta con '/' */
static void absolute(const char *in, char *out)
{
    char cwd[MAXP];
#ifdef _WIN32
    if (!_fullpath(out, in, MAXP)) snprintf(out, MAXP, "%s", in);
    (void)cwd;
#else
    if (in[0] == '/') snprintf(out, MAXP, "%s", in);
    else if (getcwd(cwd, sizeof cwd)) snprintf(out, MAXP, "%s/%s", cwd, in);
    else snprintf(out, MAXP, "%s", in);
#endif
    slashes(out);
}

/* Ruta corta 8.3 (Windows).  Si el archivo aun no existe se acorta la carpeta. */
static void shortp(const char *in, char *out)
{
    char full[MAXP];
    absolute(in, full);
#ifdef _WIN32
    char tmp[MAXP];
    DWORD r = GetShortPathNameA(full, tmp, MAXP);
    if (r && r < MAXP) {
        snprintf(out, MAXP, "%s", tmp);
    } else {
        char dir[MAXP];
        snprintf(dir, sizeof dir, "%s", full);
        char *s = strrchr(dir, '/');
        if (s) {
            *s = 0;
            r = GetShortPathNameA(dir, tmp, MAXP);
            if (r && r < MAXP) snprintf(out, MAXP, "%s/%s", tmp, s + 1);
            else snprintf(out, MAXP, "%s", full);
        } else snprintf(out, MAXP, "%s", full);
    }
    slashes(out);
#else
    snprintf(out, MAXP, "%s", full);
#endif
}

/* Ruta larga (Windows) para guardar en variant.cfg */
static void longp(const char *in, char *out)
{
#ifdef _WIN32
    char tmp[MAXP];
    snprintf(tmp, sizeof tmp, "%s", in);
    for (char *p = tmp; *p; p++) if (*p == '/') *p = '\\';
    DWORD r = GetLongPathNameA(tmp, out, MAXP);
    if (!r || r >= MAXP) snprintf(out, MAXP, "%s", in);
#else
    snprintf(out, MAXP, "%s", in);
#endif
    slashes(out);
}

/* entre comillas si tiene espacios (Windows sin nombres cortos 8.3) */
static const char *qt(const char *p)
{
    static char buf[8][MAXP + 2];
    static int k = 0;
    if (!strpbrk(p, " \t&()")) return p;
    char *b = buf[k++ & 7];
    snprintf(b, MAXP + 2, "\"%s\"", p);
    return b;
}

static void quote_into(char *cmd, const char *arg)
{
    size_t n = strlen(cmd);
    int q = strpbrk(arg, " \t&()") != NULL;
    snprintf(cmd + n, MAXCMD - n, q ? " \"%s\"" : " %s", arg);
}

/* Ejecuta y devuelve el codigo de salida; la salida va a capture si no es NULL */
static int run(const char *cmd, const char *capture)
{
    char full[MAXCMD + 64];
    if (capture) snprintf(full, sizeof full, "%s > \"%s\" 2>&1", cmd, capture);
    else snprintf(full, sizeof full, "%s", cmd);
    if (verbose) fprintf(stderr, "> %s\n", cmd);
    fflush(stdout);
#ifdef _WIN32
    /* cmd.exe quita el primer y ultimo par de comillas: envolver otra vez */
    char w[MAXCMD + 128];
    snprintf(w, sizeof w, "\"%s\"", full);
    int r = system(w);
#else
    int r = system(full);
    if (r != -1 && r > 255) r >>= 8;
#endif
    return r;
}

/* ---------------------------------------------------------- variant.cfg */
#define MAXKV 64
static char *cfg_k[MAXKV], *cfg_v[MAXKV];
static int ncfg = 0;
static char variant_dir[MAXP];

static const char *cfg(const char *k)
{
    for (int i = 0; i < ncfg; i++)
        if (!strcmp(cfg_k[i], k)) return cfg_v[i];
    return "";
}

/* Carpeta de variantes del usuario: sobrevive a las actualizaciones del
 * paquete instalado por el Gestor de placas. */
static const char *user_variants(void)
{
    static char d[MAXP];
    const char *e = getenv("PSOCDUINO_VARIANTS");
    if (e && *e) snprintf(d, sizeof d, "%s", e);
#ifdef _WIN32
    else if (getenv("LOCALAPPDATA")) snprintf(d, sizeof d, "%s/PsocDuino/variants", getenv("LOCALAPPDATA"));
#endif
    else snprintf(d, sizeof d, "%s/.psocduino/variants", getenv("HOME") ? getenv("HOME") : ".");
    slashes(d);
    return d;
}

static void mkdirs(const char *path)
{
    char t[MAXP];
    snprintf(t, sizeof t, "%s", path);
    slashes(t);
    for (char *p = t + 1; *p; p++)
        if (*p == '/' && p[-1] != ':') { *p = 0; mkdir_p(t); *p = '/'; }
    mkdir_p(t);
}

static const char *base_name(const char *p)
{
    const char *a = strrchr(p, '/'), *b = strrchr(p, '\\');
    if (b > a) a = b;
    return a ? a + 1 : p;
}

static void load_variant(const char *dir)
{
    char p[MAXP], d[MAXP];
    snprintf(d, sizeof d, "%s", dir);
    slashes(d);
    while (strlen(d) > 1 && d[strlen(d) - 1] == '/') d[strlen(d) - 1] = 0;
    absolute(d, variant_dir);
    snprintf(p, sizeof p, "%s/variant.cfg", variant_dir);
    char *b = read_file(p, NULL);
    if (!b) { /* variante importada por el usuario */
        snprintf(variant_dir, sizeof variant_dir, "%s/%s", user_variants(), base_name(d));
        snprintf(p, sizeof p, "%s/variant.cfg", variant_dir);
        b = read_file(p, NULL);
    }
    if (!b)
        die("la variante '%s' no esta importada.\n"
            "  Cree un proyecto vacio en PSoC Designer para este chip, compilelo y ejecute:\n"
            "  importar_variante.bat \"C:\\ruta\\al\\proyecto\" %s\n"
            "  (se guarda en %s/%s)", base_name(d), base_name(d), user_variants(), base_name(d));
    for (char *line = strtok(b, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        while (isspace((unsigned char)*line)) line++;
        if (*line == '#' || !*line) continue;
        char *eq = strchr(line, '=');
        if (!eq || ncfg >= MAXKV) continue;
        *eq = 0;
        cfg_k[ncfg] = xstrdup(line);
        cfg_v[ncfg] = xstrdup(eq + 1);
        ncfg++;
    }
    free(b);
}

/* carpeta CypressSemiBuildMgr */
static const char *psocdir(void)
{
    static char d[MAXP];
    if (*d) return d;
    const char *e = getenv("PSOC_DESIGNER_DIR");
    const char *cands[] = {
        e ? e : "",
        cfg("PSOCDIR"),
        "C:/Program Files (x86)/Cypress/PSoC Designer/5.4/Common/CypressSemiBuildMgr",
        "C:/Program Files/Cypress/PSoC Designer/5.4/Common/CypressSemiBuildMgr",
        "C:/Program Files (x86)/Cypress/PSoC Designer/5.3/Common/CypressSemiBuildMgr",
        "C:/Program Files/Cypress/PSoC Designer/5.3/Common/CypressSemiBuildMgr",
    };
    for (unsigned i = 0; i < sizeof cands / sizeof cands[0]; i++) {
        char t[MAXP];
        if (!*cands[i]) continue;
        snprintf(t, sizeof t, "%s/tools/include", cands[i]);
        if (is_dir(t)) { snprintf(d, sizeof d, "%s", cands[i]); slashes(d); break; }
    }
    if (!*d)
        die("no encuentro PSoC Designer (CypressSemiBuildMgr). Defina PSOC_DESIGNER_DIR,\n"
            "  por ejemplo: C:\\Program Files (x86)\\Cypress\\PSoC Designer\\5.4\\Common\\CypressSemiBuildMgr");
#ifdef _WIN32
    {
        /* iccm8c llama a icppw.exe/iccomm8c.exe desde su propia carpeta SIN comillas:
         * si la ruta tiene espacios (Windows sin nombres cortos 8.3) se rompe.
         * Solucion: un junction sin espacios hacia CypressSemiBuildMgr. */
        char sp[MAXP];
        shortp(d, sp);
        if (strchr(sp, ' ')) {
            char link[MAXP], chk[MAXP], parent[MAXP];
            const char *la = getenv("LOCALAPPDATA");
            if (la && !strchr(la, ' ')) snprintf(link, sizeof link, "%s/PsocDuino/psd", la);
            else snprintf(link, sizeof link, "C:/PsocDuino/psd");
            slashes(link);
            snprintf(chk, sizeof chk, "%s/tools/include", link);
            if (!is_dir(chk)) {
                char lb[MAXP], db[MAXP], c[3 * MAXP];
                snprintf(parent, sizeof parent, "%s", link);
                *strrchr(parent, '/') = 0;
                mkdirs(parent);
                snprintf(lb, sizeof lb, "%s", link);
                snprintf(db, sizeof db, "%s", d);
                for (char *q = lb; *q; q++) if (*q == '/') *q = '\\';
                for (char *q = db; *q; q++) if (*q == '/') *q = '\\';
                RemoveDirectoryA(lb); /* junction viejo o roto */
                snprintf(c, sizeof c, "mklink /J \"%s\" \"%s\" >nul 2>&1", lb, db);
                run(c, NULL);
            }
            if (is_dir(chk)) {
                if (verbose) fprintf(stderr, "psocbuild: PSoC Designer via %s (ruta sin espacios)\n", link);
                snprintf(d, sizeof d, "%s", link);
            } else
                fprintf(stderr, "psocbuild: aviso: la ruta de PSoC Designer tiene espacios y no pude crear\n"
                                "  %s. Cree el acceso a mano:  mklink /J %s \"%s\"\n", link, link, d);
        }
    }
#endif
    return d;
}

/* ejecutable de ImageCraft */
static const char *tool(const char *name)
{
    static char buf[8][MAXP];
    static int slot = 0;
    char *t = buf[slot++ & 7];
    const char *env = getenv("PSOCBUILD_TOOLS");
    const char *dirs[4];
    char d1[MAXP], d2[MAXP];
    int n = 0;
    if (env) dirs[n++] = env;
    if (*cfg("TOOLBIN")) dirs[n++] = cfg("TOOLBIN");
    snprintf(d1, sizeof d1, "%s/tools", psocdir());
    snprintf(d2, sizeof d2, "%s/tools/bin", psocdir());
    dirs[n++] = d1;
    dirs[n++] = d2;
    for (int i = 0; i < n; i++) {
        snprintf(t, MAXP, "%s/%s" EXE, dirs[i], name);
        if (exists(t)) {
            char s[MAXP];
            shortp(t, s);
            snprintf(t, MAXP, "%s", qt(s));
            return t;
        }
    }
    die("no encuentro %s" EXE " en %s/tools (defina PSOCBUILD_TOOLS)", name, psocdir());
    return NULL;
}

/* reemplaza @PSOCDIR@ y @VARIANT@ (con rutas cortas) */
static void expand(const char *in, char *out, size_t outsz)
{
    char pd[MAXP], vd[MAXP];
    shortp(psocdir(), pd);
    shortp(variant_dir, vd);
    size_t o = 0;
    while (*in && o + 1 < outsz) {
        if (!strncmp(in, "@PSOCDIR@", 9)) { o += (size_t)snprintf(out + o, outsz - o, "%s", pd); in += 9; }
        else if (!strncmp(in, "@VARIANT@", 9)) { o += (size_t)snprintf(out + o, outsz - o, "%s", vd); in += 9; }
        else out[o++] = *in++;
    }
    out[o < outsz ? o : outsz - 1] = 0;
}

static int find_files(const char *root, const char *file, int depth, char out[][MAXP], int max, int n);
static int score_dir(const char *path, const char *want);

static void std_includes(char *cmd)
{
    char p[MAXP], s[MAXP];
    snprintf(p, sizeof p, "%s/tools/include", psocdir());
    shortp(p, s);
    size_t n = strlen(cmd);
    snprintf(cmd + n, MAXCMD - n, " -I%s", qt(s));
    static char autoinc[MAXP];
    static int sub_ok = -1;
    if (sub_ok < 0) {
        char t[MAXP];
        snprintf(t, sizeof t, "%s/%s/m8c.inc", psocdir(), cfg("INCLUDE_SUB"));
        sub_ok = *cfg("INCLUDE_SUB") && exists(t);
    }
    if (!sub_ok && !*autoinc) { /* carpeta de m8c.inc de la familia */
        char root[MAXP], found[32][MAXP];
        snprintf(root, sizeof root, "%s/tools/include", psocdir());
        int n = find_files(root, "m8c.inc", 1, found, 32, 0), best = -1, bs = -1;
        for (int i = 0; i < n; i++) {
            int sc = score_dir(found[i], *cfg("LIBDEVICE") ? cfg("LIBDEVICE") : cfg("BASEDEVICE"));
            if (sc > bs) { bs = sc; best = i; }
        }
        if (best >= 0 && bs >= 6) {
            snprintf(autoinc, sizeof autoinc, "%s", found[best]);
            *strrchr(autoinc, '/') = 0;
        } else fprintf(stderr, "psocbuild: aviso: no encuentro m8c.inc para %s en %s\n", cfg("BASEDEVICE"), root);
    }
    if (*autoinc) {
        shortp(autoinc, s);
        size_t n2 = strlen(cmd);
        snprintf(cmd + n2, MAXCMD - n2, " -I%s", qt(s));
    }
    if (sub_ok) {
        snprintf(p, sizeof p, "%s/%s", psocdir(), cfg("INCLUDE_SUB"));
        shortp(p, s);
        n = strlen(cmd);
        snprintf(cmd + n, MAXCMD - n, " -I%s", qt(s));
    }
    shortp(variant_dir, s);
    n = strlen(cmd);
    {
        char l2[MAXP];
        snprintf(l2, sizeof l2, "%s/lib", s);
        snprintf(cmd + n, MAXCMD - n, " -I%s -I%s", qt(s), qt(l2));
    }
}

/* -I/-D/-o con ruta -> acortar */
static void add_flag(char *cmd, const char *a)
{
    char s[MAXP];
    if (!strncmp(a, "-I", 2) && a[2]) {
        shortp(a + 2, s);
        size_t n = strlen(cmd);
        snprintf(cmd + n, MAXCMD - n, " -I%s", qt(s));
    } else quote_into(cmd, a);
}

/* traduce "!E file(line):msg" -> "file:line: error: msg" */
static int show_output(const char *capfile, const char *tmpname, const char *origname)
{
    char *b = read_file(capfile, NULL);
    int errors = 0;
    if (!b) return 0;
    for (char *line = strtok(b, "\n"); line; line = strtok(NULL, "\n")) {
        size_t L = strlen(line);
        if (L && line[L - 1] == '\r') line[L - 1] = 0;
        char kind = 0;
        if ((line[0] == '!' && (line[1] == 'E' || line[1] == 'W') && line[2] == ' ')) kind = line[1];
        if (kind) {
            char *f = line + 3, *lp = strrchr(f, '('), *rp = lp ? strchr(lp, ')') : NULL;
            if (lp && rp && rp[1] == ':') {
                *lp = 0;
                *rp = 0;
                char fn[MAXP];
                snprintf(fn, sizeof fn, "%s", f);
                slashes(fn);
                if (tmpname && strstr(fn, tmpname)) snprintf(fn, sizeof fn, "%s", origname);
                fprintf(stderr, "%s:%s: %s: %s\n", fn, lp + 1, kind == 'E' ? "error" : "warning", rp + 2);
                if (kind == 'E') errors++;
                continue;
            }
            if (kind == 'E') errors++;
        }
        fprintf(stderr, "%s\n", line);
    }
    free(b);
    return errors;
}

/* --------------------------------------------------------------- cc/as */
static int cmd_compile(int argc, char **argv, int is_asm)
{
    const char *out = NULL, *src = NULL, *vdir = NULL;
    char *flags[512];
    int nf = 0;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-V") && i + 1 < argc) vdir = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) out = argv[++i];
        else if (!strcmp(argv[i], "-v")) verbose = 1;
        else if (argv[i][0] == '-' || src) { if (nf < 512) flags[nf++] = argv[i]; }
        else src = argv[i];
    }
    if (!vdir || !out || !src) die("uso: psocbuild %s -V VARIANT -o OUT.o SRC", is_asm ? "as" : "cc");
    load_variant(vdir);

    /* ImageCraft decide por la extension: .cpp/.ino -> .c, .S -> .asm */
    char work[MAXP], capture[MAXP], s[MAXP];
    const char *e = ext(src);
    const char *want = is_asm ? ".asm" : ".c";
    snprintf(work, sizeof work, "%s", src);
    int temp = 0;
    if (strcmp(e, want) && strcasecmp(e, want)) {
        snprintf(work, sizeof work, "%s.psoc%s", out, want);
        if (copy_file(src, work)) die("no puedo copiar %s", src);
        temp = 1;
    }
    snprintf(capture, sizeof capture, "%s.log", out);

    char cmd[MAXCMD];
    snprintf(cmd, sizeof cmd, "%s", tool(is_asm ? "iasm8c" : "iccm8c"));
    if (!is_asm) {
        size_t n = strlen(cmd);
        char extra[MAXP];
        expand(cfg("CFLAGS"), extra, sizeof extra);
        int has_fcpu = 0, has_ports = 0;
        for (int i = 0; i < nf; i++) {
            if (!strncmp(flags[i], "-DF_CPU=", 8)) has_fcpu = 1;
            if (!strncmp(flags[i], "-DPSOC_NUM_PORTS=", 17)) has_ports = 1;
        }
        n += (size_t)snprintf(cmd + n, MAXCMD - n, " -c -e %s -D__PSOCDUINO__", extra);
        if (!has_fcpu)
            n += (size_t)snprintf(cmd + n, MAXCMD - n, " -DF_CPU=%sUL", *cfg("F_CPU") ? cfg("F_CPU") : "24000000");
        if (!has_ports && *cfg("NUM_PORTS"))
            n += (size_t)snprintf(cmd + n, MAXCMD - n, " -DPSOC_NUM_PORTS=%s", cfg("NUM_PORTS"));
    }
    std_includes(cmd);
    for (int i = 0; i < nf; i++) add_flag(cmd, flags[i]);
    shortp(out, s);
    { size_t n = strlen(cmd); snprintf(cmd + n, MAXCMD - n, " -o %s", qt(s)); }
    shortp(work, s);
    { size_t n = strlen(cmd); snprintf(cmd + n, MAXCMD - n, " %s", qt(s)); }

    int r = run(cmd, capture);
    char tmpbase[MAXP];
    shortp(work, tmpbase);
    const char *tb = strrchr(tmpbase, '/');
    int errs = show_output(capture, tb ? tb + 1 : tmpbase, src);
    remove(capture);
    if (temp) remove(work);
    if (r == 0 && errs == 0 && !exists(out)) {
        fprintf(stderr, "psocbuild: el compilador no genero %s\n", out);
        return 1;
    }
    return (r || errs) ? 1 : 0;
}

/* ------------------------------------------------------------------- ar */
static int cmd_ar(int argc, char **argv)
{
    const char *vdir = NULL, *pos[2] = {0};
    int np = 0;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-V") && i + 1 < argc) vdir = argv[++i];
        else if (!strcmp(argv[i], "-v")) verbose = 1;
        else if (np < 2) pos[np++] = argv[i];
    }
    if (!vdir || np != 2) die("uso: psocbuild ar -V VARIANT ARCHIVO.a OBJETO.o");
    load_variant(vdir);
    char cmd[MAXCMD], a[MAXP], o[MAXP];
    shortp(pos[0], a);
    shortp(pos[1], o);
    snprintf(cmd, sizeof cmd, "%s -a %s %s", tool("ilibw"), qt(a), qt(o));
    return run(cmd, NULL) ? 1 : 0;
}


static void replace_all(char *s, size_t cap, const char *from, const char *to);

/* ------------------------------------------------- directorios y busqueda */
#define MAXFILES 256
/* nombres de archivos (no carpetas) de dir; devuelve cantidad */
static int list_dir(const char *dir, char names[][260], int max, int want_dirs)
{
    int n = 0;
#ifdef _WIN32
    char pat[MAXP];
    WIN32_FIND_DATAA fd;
    snprintf(pat, sizeof pat, "%s/*", dir);
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        int isd = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (fd.cFileName[0] == '.' || isd != want_dirs || n >= max) continue;
        snprintf(names[n++], 260, "%s", fd.cFileName);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR *d = opendir(dir);
    struct dirent *e;
    if (!d) return 0;
    while ((e = readdir(d)) && n < max) {
        char p[MAXP];
        if (e->d_name[0] == '.') continue;
        snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
        if (is_dir(p) != want_dirs) continue;
        snprintf(names[n++], 260, "%s", e->d_name);
    }
    closedir(d);
#endif
    return n;
}

/* busca 'file' (sin distinguir mayusculas) bajo root hasta 'depth' niveles */
static int find_files(const char *root, const char *file, int depth, char out[][MAXP], int max, int n)
{
    static char names[MAXFILES][260];
    char local[MAXFILES][260];
    int nf = list_dir(root, names, MAXFILES, 0);
    for (int i = 0; i < nf && n < max; i++)
        if (!strcasecmp(names[i], file)) snprintf(out[n++], MAXP, "%s/%s", root, names[i]);
    if (depth <= 0) return n;
    int nd = list_dir(root, names, MAXFILES, 1);
    memcpy(local, names, sizeof(local[0]) * (size_t)nd);
    for (int i = 0; i < nd && n < max; i++) {
        char sub[MAXP];
        snprintf(sub, sizeof sub, "%s/%s", root, local[i]);
        n = find_files(sub, file, depth - 1, out, max, n);
    }
    return n;
}

static int score_dir(const char *path, const char *want)
{
    /* coincidencia por prefijo de familia: CY8C24423B -> CY8C24... */
    const char *b = strrchr(path, '/');
    char dir[260];
    int sc = 0;
    if (!b) return 0;
    snprintf(dir, sizeof dir, "%.*s", (int)(b - path), path);
    b = strrchr(dir, '/');
    b = b ? b + 1 : dir;
    if (!strcasecmp(b, want)) return 100;
    for (int i = 0; b[i] && want[i] && toupper((unsigned char)b[i]) == toupper((unsigned char)want[i]); i++) sc++;
    /* mismo sufijo de familia: CY8C21234 ~ CY8C21x34 */
    {
        char dig[16] = "";
        size_t k = 0;
        for (const char *c = want + 4; *c && k + 1 < sizeof dig; c++) if (isdigit((unsigned char)*c)) dig[k++] = *c;
        dig[k] = 0;
        if (k >= 2 && strstr(b, dig + k - 2) && strlen(b) >= 2) sc += 2;
    }
    /* misma revision de silicio (sufijo A/B) */
    size_t lb = strlen(b), lw = strlen(want);
    if (lb && lw && isalpha((unsigned char)b[lb - 1]) && toupper((unsigned char)b[lb - 1]) == toupper((unsigned char)want[lw - 1])) sc += 3;
    return sc;
}

/* Bibliotecas de enlace de PSoC Designer: -L<modelo>, cms.a de la familia y arith.a */
static void auto_libs(char *out, size_t outsz)
{
    char lib[MAXP], s[MAXP], found[16][MAXP];
    const char *model = (*cfg("LMM") && strcmp(cfg("LMM"), "0")) ? "LMM" : "SMM";
    const char *base = *cfg("LIBDEVICE") ? cfg("LIBDEVICE") : cfg("BASEDEVICE");
    int nomac = strstr(cfg("CFLAGS"), "nomac") != NULL;
    snprintf(lib, sizeof lib, "%s/tools/lib", psocdir());
    size_t o = 0;
    shortp(lib, s);
    {
        char lm[MAXP];
        snprintf(lm, sizeof lm, "%s/%s", s, model);
        o += (size_t)snprintf(out + o, outsz - o, "-L%s", qt(lm));
    }

    int n = find_files(lib, "cms.a", 1, found, 16, 0), best = -1, bs = -1;
    for (int i = 0; i < n; i++) { int sc = score_dir(found[i], base); if (sc > bs) { bs = sc; best = i; } }
    if (best >= 0 && bs >= 6) { shortp(found[best], s); o += (size_t)snprintf(out + o, outsz - o, " %s", qt(s)); }
    else fprintf(stderr, "psocbuild: aviso: no encuentro cms.a para %s en %s\n", base, lib);

    char mroot[MAXP];
    snprintf(mroot, sizeof mroot, "%s/%s", lib, model);
    n = find_files(mroot, "arith.a", 3, found, 16, 0);
    best = -1; bs = -1;
    for (int i = 0; i < n; i++) {
        char up[MAXP];
        int sc = 0;
        snprintf(up, sizeof up, "%s", found[i]);
        for (char *c = up; *c; c++) *c = (char)toupper((unsigned char)*c);
        int has_nomac = strstr(up, "NOMAC") != NULL, has_mac = !has_nomac && strstr(up, "/MAC") != NULL;
        if (nomac ? has_nomac : has_mac) sc += 10;
        if (strstr(up, "UASW")) sc += 2;
        if (sc > bs) { bs = sc; best = i; }
    }
    if (best >= 0) { shortp(found[best], s); o += (size_t)snprintf(out + o, outsz - o, " %s", qt(s)); }
    else fprintf(stderr, "psocbuild: aviso: no encuentro arith.a en %s\n", mroot);
}

/* reemplaza el valor de "NOMBRE: equ X" en un .inc; devuelve 1 si existia */
static int patch_equ(char *buf, size_t cap, const char *name, const char *value)
{
    char key[96];
    snprintf(key, sizeof key, "%s:", name);
    size_t kl = strlen(key);
    for (char *p = buf; (p = strstr(p, key)); p++) {
        if (p != buf && p[-1] != '\n') continue;
        char *eq = p + kl;
        while (*eq == ' ' || *eq == '\t') eq++;
        if (strncasecmp(eq, "equ", 3)) continue;
        char *end = strchr(eq, '\n');
        if (!end) end = eq + strlen(eq);
        char repl[160];
        snprintf(repl, sizeof repl, "%s\tequ\t%s\t; PsocDuino (menu del IDE)\r", key, value);
        size_t rl = strlen(repl), tail = strlen(end);
        if ((size_t)(p - buf) + rl + tail + 1 > cap) return 0;
        memmove(p + rl, end, tail + 1);
        memcpy(p, repl, rl);
        return 1;
    }
    return 0;
}

/* ----------------------------------------------------------------- link */
static int cmd_link(int argc, char **argv)
{
    const char *vdir = NULL, *outbase = NULL, *vcc = NULL, *cpu = NULL;
    char *objs[1024];
    int no = 0;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-V") && i + 1 < argc) vdir = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) outbase = argv[++i];
        else if (!strcmp(argv[i], "--vcc") && i + 1 < argc) vcc = argv[++i];
        else if (!strcmp(argv[i], "--cpu") && i + 1 < argc) cpu = argv[++i];
        else if (!strcmp(argv[i], "-v")) verbose = 1;
        else if (no < 1024) objs[no++] = argv[i];
    }
    if (!vdir || !outbase) die("uso: psocbuild link -V VARIANT -o SALIDA objetos...");
    load_variant(vdir);

    char outdir[MAXP], s[MAXP], cmd[MAXCMD], cwd[MAXP];
    int r = 0;
    snprintf(outdir, sizeof outdir, "%s", outbase);
    slashes(outdir);
    char *sl = strrchr(outdir, '/');
    if (sl) *sl = 0; else snprintf(outdir, sizeof outdir, ".");

    /* 1. copia de boot.asm + memory.inc + lib/GlobalParams.inc con los menus */
    char bdir[MAXP], p1[MAXP], p2[MAXP];
    snprintf(bdir, sizeof bdir, "%s/psoc_boot", outdir);
    absolute(bdir, s);
    snprintf(bdir, sizeof bdir, "%s", s);
    snprintf(p1, sizeof p1, "%s/lib", bdir);
    mkdirs(p1);
    const char *copies[] = { "boot.asm", "memory.inc" };
    for (int i = 0; i < 2; i++) {
        snprintf(p1, sizeof p1, "%s/%s", variant_dir, copies[i]);
        snprintf(p2, sizeof p2, "%s/%s", bdir, copies[i]);
        if (copy_file(p1, p2)) die("falta %s en la variante", p1);
    }
    snprintf(p1, sizeof p1, "%s/lib/GlobalParams.inc", variant_dir);
    long gl;
    char *gp = read_file(p1, &gl);
    if (!gp) die("falta %s", p1);
    size_t gcap = (size_t)gl + 4096;
    gp = realloc(gp, gcap);
    if (cpu && *cpu) {
        char v[8];
        snprintf(v, sizeof v, "%sh", cpu);
        patch_equ(gp, gcap, "CPU_CLOCK", v);
        patch_equ(gp, gcap, "CPU_CLOCK_JUST", v);
    }
    patch_equ(gp, gcap, "SLEEP_TIMER", "0h");        /* 512 Hz para millis() */
    patch_equ(gp, gcap, "SLEEP_TIMER_JUST", "0h");
    if (vcc && !strcmp(vcc, "3") && cpu && !strcmp(cpu, "3"))
        fprintf(stderr, "psocbuild: aviso: CPU a 24 MHz con 3.3 V esta fuera de la hoja de datos "
                        "(maximo 12 MHz); use 12 MHz o alimente a 5 V.\n");
    if (vcc && !strcmp(vcc, "3")) {
        patch_equ(gp, gcap, "POWER_SETTING", "08h");  /* 3.3 V, IMO 24 MHz */
        patch_equ(gp, gcap, "SUPPLY_VOLTAGE", "0h");  /* familia CY8C27x43 */
        patch_equ(gp, gcap, "TRIP_VOLTAGE", "0h");    /* LVD minimo: seguro a 3.3 V */
        patch_equ(gp, gcap, "TRIP_VOLTAGE_JUST", "0h");
    } else if (vcc && !strcmp(vcc, "5")) {
        patch_equ(gp, gcap, "POWER_SETTING", "10h");  /* 5 V, IMO 24 MHz */
        patch_equ(gp, gcap, "SUPPLY_VOLTAGE", "1h");
    }
    snprintf(p2, sizeof p2, "%s/lib/GlobalParams.inc", bdir);
    write_file(p2, gp, (long)strlen(gp));
    free(gp);

    /* 2. ensamblar boot.asm y los .asm de lib/ de la variante (configuracion global) */
    char asmobj[64][MAXP];
    int na = 0;
    {
        static char names[MAXFILES][260];
        char vlib[MAXP];
        snprintf(vlib, sizeof vlib, "%s/lib", variant_dir);
        int nf = list_dir(vlib, names, MAXFILES, 0);
        char srcs[65][MAXP];
        int ns = 0;
        snprintf(srcs[ns++], MAXP, "boot.asm");
        for (int i = 0; i < nf && ns < 65; i++)
            if (!strcasecmp(ext(names[i]), ".asm")) snprintf(srcs[ns++], MAXP, "%s/%s", vlib, names[i]);
        if (!getcwd(cwd, sizeof cwd)) die("getcwd");
        for (int i = 0; i < ns; i++) {
            char obj[MAXP], cap2[MAXP], so[MAXP], ss[MAXP], pbl[MAXP];
            const char *bn = strrchr(srcs[i], '/');
            bn = bn ? bn + 1 : srcs[i];
            snprintf(obj, sizeof obj, "%s/%.*s.o", bdir, (int)(strlen(bn) - 4), bn);
            snprintf(cap2, sizeof cap2, "%s/asm.log", bdir);
            snprintf(cmd, sizeof cmd, "%s", tool("iasm8c"));
            shortp(bdir, pbl);
            { char pl[MAXP]; snprintf(pl, sizeof pl, "%s/lib", pbl);
              size_t n = strlen(cmd); snprintf(cmd + n, MAXCMD - n, " -I%s -I%s", qt(pl), qt(pbl)); }
            std_includes(cmd);
            shortp(obj, so);
            if (i == 0) snprintf(ss, sizeof ss, "boot.asm"); else shortp(srcs[i], ss);
            { size_t n = strlen(cmd); snprintf(cmd + n, MAXCMD - n, " -o %s %s", qt(so), qt(ss)); }
            if (chdir(bdir)) die("no puedo entrar en %s", bdir);
            r = run(cmd, cap2);
            if (chdir(cwd)) die("chdir");
            if (show_output(cap2, NULL, NULL) || r) { fprintf(stderr, "psocbuild: error ensamblando %s\n", bn); return 1; }
            remove(cap2);
            if (na < 64) snprintf(asmobj[na++], MAXP, "%s", obj);
        }
    }

    /* 3. linkfile con las opciones de PSoC Designer para este chip */
    char lf[MAXP], flags[MAXP], libs[MAXCMD];
    snprintf(lf, sizeof lf, "%s/linkfile", outdir);
    expand(cfg("LINK_FLAGS"), flags, sizeof flags);
    if (!*cfg("LINK_LIBS") || strstr(cfg("LINK_LIBS"), "@AUTO@")) auto_libs(libs, sizeof libs);
    else {
        /* token a token: expandir @PSOCDIR@/@VARIANT@ y poner comillas si hace falta;
         * los .asm de lib/ ya se ensamblaron: se omite libpsoc.a (simbolos duplicados) */
        char raw[MAXCMD];
        size_t o = 0;
        snprintf(raw, sizeof raw, "%s", cfg("LINK_LIBS"));
        libs[0] = 0;
        for (char *t = strtok(raw, " "); t; t = strtok(NULL, " ")) {
            char e[MAXP];
            if (strstr(t, "lib/libpsoc.a") || strstr(t, "lib/libPSoc.a")) continue;
            expand(t, e, sizeof e);
            if (!strncmp(e, "-L", 2)) o += (size_t)snprintf(libs + o, sizeof libs - o, " -L%s", qt(e + 2));
            else o += (size_t)snprintf(libs + o, sizeof libs - o, " %s", qt(e));
        }
    }
    FILE *f = fopen(lf, "w");
    if (!f) die("no puedo escribir %s", lf);
    /* nombre interno sin puntos: ImageCraft podria tomar ".ino" como extension */
    char fw[MAXP];
    snprintf(fw, sizeof fw, "%s/psocduino_fw", outdir);
    shortp(fw, s);
    fprintf(f, "%s -m\n -o\"%s\"\n", flags, s);
    for (int i = 0; i < na; i++) { /* boot.o primero (vectores en 0) */
        shortp(asmobj[i], s);
        fprintf(f, "%s\n", qt(s));
    }
    for (int i = 0; i < no; i++) {
        shortp(objs[i], s);
        fprintf(f, "%s\n", qt(s));
    }
    fprintf(f, " %s\n", libs);
    fclose(f);

    char lfs[MAXP], cap[MAXP];
    shortp(lf, lfs);
    snprintf(cap, sizeof cap, "%s/link.log", outdir);
    snprintf(cmd, sizeof cmd, "%s @%s", tool("ilinkm8c"), qt(lfs));
    if (verbose) {
        char *b = read_file(lf, NULL);
        if (b) { fprintf(stderr, "--- linkfile ---\n%s----------------\n", b); free(b); }
    }
    r = run(cmd, cap);
    /* "symbol X is unused" es informativo */
    char *b = read_file(cap, NULL);
    int errs = 0;
    if (b) {
        for (char *line = strtok(b, "\n"); line; line = strtok(NULL, "\n")) {
            if (strstr(line, "is unused") && !verbose) continue;
            if (line[0] == '!' && line[1] == 'E') errs++;
            fprintf(stderr, "%s\n", line);
        }
        free(b);
    }
    remove(cap);
    char hex[MAXP];
    snprintf(hex, sizeof hex, "%s.hex", fw);
    if (r || errs || !exists(hex)) {
        fprintf(stderr, "psocbuild: el enlace fallo%s\n", exists(hex) ? "" : " (no se genero el .hex)");
        return 1;
    }
    const char *exts[] = { ".hex", ".mp", ".lst", ".rom", ".dbg" };
    for (unsigned i = 0; i < sizeof exts / sizeof exts[0]; i++) {
        char a[MAXP], b2[MAXP];
        snprintf(a, sizeof a, "%s%s", fw, exts[i]);
        snprintf(b2, sizeof b2, "%s%s", outbase, exts[i]);
        if (exists(a) && copy_file(a, b2)) die("no puedo copiar %s", a);
    }
    return 0;
}

/* ----------------------------------------------------------------- size */
static int cmd_size(int argc, char **argv)
{
    if (argc < 1) die("uso: psocbuild size ARCHIVO.mp");
    char *b = read_file(argv[0], NULL);
    if (!b) die("no puedo leer %s", argv[0]);
    long rom = 0, ram = 0;
    for (char *line = strtok(b, "\n"); line; line = strtok(NULL, "\n")) {
        /*   lit   0150   0227 =    551. bytes (rel,con,rom,lit) */
        char name[64];
        unsigned addr, sz;
        if (sscanf(line, " %63s %x %x =", name, &addr, &sz) != 3) continue;
        const char *at = strchr(line, '(');
        if (!at) continue;
        if (strstr(at, "rom")) {
            if (strcmp(name, "TOP")) rom += sz;
            else rom += addr + sz; /* vectores + arranque, absoluto desde 0 */
        } else if (strstr(at, "ram")) ram += sz;
    }
    free(b);
    printf("Flash: %ld bytes\nRAM: %ld bytes\n", rom, ram);
    return 0;
}

static int cmd_preproc(int argc, char **argv)
{
    /* El IDE solo necesita el texto para generar prototipos (ctags).
     * ImageCraft no tiene un modo -E compatible con gcc: se copia. */
    const char *in = NULL, *out = NULL;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-o") && i + 1 < argc) out = argv[++i];
        else if (argv[i][0] != '-' && !in) in = argv[i];
    }
    if (!in || !out) die("uso: psocbuild preproc SRC -o OUT");
    if (copy_file(in, out)) die("no puedo copiar %s", in);
    return 0;
}

/* --------------------------------------------------------------- import */
static char *mk_get(const char *mk, const char *key)
{
    static char v[MAXCMD];
    size_t kl = strlen(key);
    const char *p = mk;
    while ((p = strstr(p, key))) {
        if ((p == mk || p[-1] == '\n') && (p[kl] == '=' || (p[kl] == ':' && p[kl + 1] == '='))) {
            p += kl + (p[kl] == ':' ? 2 : 1);
            size_t i = 0;
            while (*p && *p != '\n' && *p != '\r' && i + 1 < sizeof v) v[i++] = *p++;
            v[i] = 0;
            while (i && isspace((unsigned char)v[i - 1])) v[--i] = 0;
            return v;
        }
        p++;
    }
    v[0] = 0;
    return v;
}

static void replace_all(char *s, size_t cap, const char *from, const char *to)
{
    if (!*from) return;
    char *tmp = malloc(cap);
    if (!tmp) die("sin memoria");
    size_t fl = strlen(from), o = 0;
    for (const char *p = s; *p && o + 1 < cap;) {
        if (!strncasecmp(p, from, fl)) { o += (size_t)snprintf(tmp + o, cap - o, "%s", to); p += fl; }
        else tmp[o++] = *p++;
    }
    tmp[o < cap ? o : cap - 1] = 0;
    snprintf(s, cap, "%s", tmp);
    free(tmp);
}

static long cpu_hz(const char *gp)
{
    const char *p = strstr(gp, "CPU_CLOCK:");
    if (!p) return 0;
    p += 10;
    while (*p == ' ' || *p == '\t' || !strncasecmp(p, "equ", 3)) p += (*p == ' ' || *p == '\t') ? 1 : 3;
    unsigned v = (unsigned)strtoul(p, NULL, 16) & 7;
    /* OSC_CR0[2:0]: 0=SysClk/8 1=/4 2=/2 3=/1 4=/16 5=/32 6=/128 7=/256 */
    static const long div[8] = { 8, 4, 2, 1, 16, 32, 128, 256 };
    return 24000000L / div[v];
}

/* Reemplaza el vector "org 64h" (Sleep Timer) por el de PsocDuino. */
static int patch_sleep_vector(char *boot, size_t cap)
{
    char *v = strstr(boot, "org   64h");
    if (!v) v = strstr(boot, "org 64h");
    if (!v) return -1;
    char *eol = strchr(v, '\n');
    if (!eol) return -1;
    char *next = eol + 1, *end = next;
    /* hasta la siguiente linea "org" o el banner de fin */
    while (*end) {
        char *ln = end;
        while (*ln == ' ' || *ln == '\t') ln++;
        if (!strncmp(ln, "org", 3) || !strncmp(ln, ";---", 4) || !strncmp(ln, ";@", 2)) break;
        char *e2 = strchr(end, '\n');
        if (!e2) { end += strlen(end); break; }
        end = e2 + 1;
    }
    char body[256];
    snprintf(body, sizeof body, "    ljmp\t_psocduino_sleep_isr     ; PsocDuino: millis()\n    reti\n\n");
    if (strstr(next, "_SleepTimer_ISR") && strstr(next, "_SleepTimer_ISR") < end)
        fprintf(stderr, "aviso: el proyecto usa el modulo SleepTimer; PsocDuino reemplaza su vector\n");
    size_t head = (size_t)(next - boot), tail = strlen(end);
    if (head + strlen(body) + tail + 1 > cap) return -1;
    memmove(boot + head + strlen(body), end, tail + 1);
    memcpy(boot + head, body, strlen(body));
    return 0;
}


/* ------------------------------------------- importar proyecto solo generado */
static char *xml_attr(const char *xml, const char *name, char *out, size_t outsz)
{
    char key[64];
    snprintf(key, sizeof key, " %s=\"", name);
    const char *p = strstr(xml, key);
    out[0] = 0;
    if (!p) return out;
    p += strlen(key);
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < outsz) out[i++] = *p++;
    out[i] = 0;
    return out;
}

/* KB de flash por familia (CY8C2F xSS: F = familia, SS = sufijo) */
static int flash_kb(const char *dev)
{
    const char *d = dev;
    while (*d && !isdigit((unsigned char)d[0])) d++;   /* "8C21234" */
    if (*d == '8') d += 2;                              /* salta "8C" */
    if (strlen(d) < 5) return 16;
    int fam = (d[0] - '0') * 10 + (d[1] - '0'), suf = (d[3] - '0') * 10 + (d[4] - '0');
    switch (fam) {
    case 21: return suf == 23 ? 4 : 8;
    case 22: return suf == 13 ? 2 : 16;
    case 23: return 8;
    case 24: return suf == 23 ? 4 : 16;
    case 27: return 16;
    case 28: return 16;
    case 29: return 32;
    }
    return 16;
}

static int num_ports(const char *gp)
{
    int n = 0;
    for (int i = 0; i < 8; i++) {
        char k[32];
        snprintf(k, sizeof k, "PORT_%d_DRIVE_0:", i);
        if (strstr(gp, k)) n = i + 1;
    }
    return n ? n : 3;
}

static void copy_lib_files(const char *proj, const char *vdir)
{
    static char names[MAXFILES][260];
    char src[MAXP], dst[MAXP];
    snprintf(src, sizeof src, "%s/lib", proj);
    snprintf(dst, sizeof dst, "%s/lib", vdir);
    mkdirs(dst);
    int n = list_dir(src, names, MAXFILES, 0);
    for (int i = 0; i < n; i++) {
        char a[MAXP], b[MAXP];
        snprintf(a, sizeof a, "%s/%s", src, names[i]);
        snprintf(b, sizeof b, "%s/%s", dst, !strcasecmp(names[i], "libpsoc.a") ? "libpsoc.a" : names[i]);
        copy_file(a, b);
    }
}

/* memory.inc desde la plantilla y SYS_INC_CONTENTS ("A:_equ_1 B:_equ_0h ...") */
static void write_memory_inc(const char *path, const char *sysinc, const char *tools)
{
    char eqs[2048] = "";
    size_t o = 0;
    char tmp[2048];
    snprintf(tmp, sizeof tmp, "%s", sysinc);
    for (char *t = strtok(tmp, " "); t; t = strtok(NULL, " ")) {
        char *e = strstr(t, ":_equ_");
        if (!e) continue;
        *e = 0;
        o += (size_t)snprintf(eqs + o, sizeof eqs - o, "%s: equ %s   \r\n", t, e + 6);
    }
    o += (size_t)snprintf(eqs + o, sizeof eqs - o,
                          "SYSTEM_TOOLS: equ %s   \r\nSYSTEM_IDXPG_TRACKS_STK_PP: equ 1   \r\n"
                          "SYSTEM_IDXPG_TRACKS_IDX_PP: equ 0   \r\nSYSTEM_MULTIPAGE_STACK: equ 0 ",
                          *tools ? tools : "1");
    const char *at = strstr(MEMORY_INC_TPL, "@SYSINC@");
    FILE *f = fopen(path, "wb");
    if (!f) die("no puedo escribir %s", path);
    fwrite(MEMORY_INC_TPL, 1, (size_t)(at - MEMORY_INC_TPL), f);
    fputs(eqs, f);
    fputs(at + 8, f);
    fclose(f);
}

static int patch_sleep_vector(char *boot, size_t cap);
static long cpu_hz(const char *gp);

static int import_generated(const char *proj, const char *vdir)
{
    char p[MAXP], q[MAXP], base[128], disp[128], pkg[64];
    snprintf(p, sizeof p, "%s/opts.txt", proj);
    char *opts = read_file(p, NULL);
    snprintf(p, sizeof p, "%s/PSoCConfig.xml", proj);
    char *xml = read_file(p, NULL);
    snprintf(p, sizeof p, "%s/lib/GlobalParams.inc", proj);
    char *gp = read_file(p, NULL);
    snprintf(q, sizeof q, "%s/boot.asm", proj);
    if (!opts || !xml || !gp || !exists(q))
        die("el proyecto %s no tiene la configuracion generada.\n"
            "  En PSoC Designer use Build -> Generate/Build (F6) y vuelva a importar.", proj);
    xml_attr(xml, "BASE_DEVICE", base, sizeof base);
    xml_attr(xml, "DEVICE_DISPLAY_NAME", disp, sizeof disp);
    xml_attr(xml, "PACKAGE_TYPE", pkg, sizeof pkg);
    if (!*base) die("PSoCConfig.xml sin BASE_DEVICE");

    char relstart[32], lastram[32], lmm[16], cflags[MAXP], codec[256], sysinc[2048], tools[16];
    snprintf(relstart, sizeof relstart, "%s", mk_get(opts, "RELSTART"));
    snprintf(lastram, sizeof lastram, "%s", mk_get(opts, "LASTRAM"));
    snprintf(lmm, sizeof lmm, "%s", mk_get(opts, "LMM"));
    snprintf(codec, sizeof codec, "%s", mk_get(opts, "CODECOMPRESSOR"));
    snprintf(sysinc, sizeof sysinc, "%s", mk_get(opts, "SYS_INC_CONTENTS"));
    snprintf(tools, sizeof tools, "%s", mk_get(opts, "SYSTEM_TOOLS"));
    snprintf(cflags, sizeof cflags, "%s", mk_get(opts, "MORE_CFLAGS"));
    {
        char cd[MAXP];
        snprintf(cd, sizeof cd, "%s", mk_get(opts, "CDEFINES"));
        if (*cd) { size_t n = strlen(cflags); snprintf(cflags + n, sizeof cflags - n, " %s", cd); }
    }
    int kb = flash_kb(base);
    char lastrom[16];
    snprintf(lastrom, sizeof lastrom, "0x%x", kb * 1024 - 1);
    /* project.mk (aunque la compilacion haya fallado) trae BASEDEVICE y memoria */
    char libdev[128] = "", incsub[MAXP] = "";
    snprintf(p, sizeof p, "%s/project.mk", proj);
    char *mk = read_file(p, NULL);
    if (mk) {
        if (*mk_get(mk, "BASEDEVICE")) snprintf(libdev, sizeof libdev, "%s", mk_get(mk, "BASEDEVICE"));
        if (*mk_get(mk, "LASTROM")) snprintf(lastrom, sizeof lastrom, "%s", mk_get(mk, "LASTROM"));
        if (*mk_get(mk, "LASTRAM")) snprintf(lastram, sizeof lastram, "%s", mk_get(mk, "LASTRAM"));
        if (*mk_get(mk, "RELSTART")) snprintf(relstart, sizeof relstart, "%s", mk_get(mk, "RELSTART"));
        if (*mk_get(mk, "MORE_CFLAGS")) snprintf(cflags, sizeof cflags, "%s", mk_get(mk, "MORE_CFLAGS"));
        if (*mk_get(mk, "SYS_INC_CONTENTS")) snprintf(sysinc, sizeof sysinc, "%s", mk_get(mk, "SYS_INC_CONTENTS"));
        char inc[MAXP];
        snprintf(inc, sizeof inc, "%s", mk_get(mk, "INCLUDE_PATH"));
        slashes(inc);
        char *t = strstr(inc, "/tools/include/");
        if (t) snprintf(incsub, sizeof incsub, "%s", t + 1);
        free(mk);
    }
    {   /* quitar espacios finales de cflags */
        size_t n = strlen(cflags);
        while (n && cflags[n - 1] == ' ') cflags[--n] = 0;
    }
    kb = (int)((strtoul(lastrom, NULL, 16) + 1) / 1024);

    mkdirs(vdir);
    const char *top[] = { "boot.asm", "boot.tpl", "flashsecurity.txt", "opts.txt", "PSoCConfig.xml" };
    for (unsigned i = 0; i < sizeof top / sizeof top[0]; i++) {
        snprintf(p, sizeof p, "%s/%s", proj, top[i]);
        snprintf(q, sizeof q, "%s/%s%s", vdir, top[i], i >= 3 ? ".orig" : "");
        if (exists(p)) copy_file(p, q);
    }
    copy_lib_files(proj, vdir);
    snprintf(p, sizeof p, "%s/memory.inc", proj);
    snprintf(q, sizeof q, "%s/memory.inc", vdir);
    {   /* si la compilacion fallo, memory.inc tiene solo las equ: reconstruir */
        char *mi = read_file(p, NULL);
        if (mi && strstr(mi, "macro RAM_PROLOGUE")) copy_file(p, q);
        else write_memory_inc(q, sysinc, tools);
        free(mi);
    }

    snprintf(q, sizeof q, "%s/boot.asm", vdir);
    long bl;
    char *boot = read_file(q, &bl);
    size_t cap = (size_t)bl + 1024;
    boot = realloc(boot, cap);
    if (!boot || patch_sleep_vector(boot, cap)) die("no encuentro el vector 64h (Sleep Timer) en boot.asm");
    write_file(q, boot, (long)strlen(boot));
    free(boot);

    long hz = cpu_hz(gp);
    snprintf(q, sizeof q, "%s/variant.cfg", vdir);
    FILE *f = fopen(q, "w");
    if (!f) die("no puedo escribir %s", q);
    fprintf(f, "# Generado por 'psocbuild import' (proyecto generado, sin compilar)\n");
    fprintf(f, "DEVICE=%s\nDISPLAY=%s\nPACKAGE=%s\nBASEDEVICE=%s\nLIBDEVICE=%s\nPSOCDIR=\nINCLUDE_SUB=%s\n",
            base, disp, pkg, *libdev ? libdev : base, libdev, incsub);
    fprintf(f, "F_CPU=%ld\nLMM=%s\nCFLAGS=%s\n", hz ? hz : 24000000L, *lmm ? lmm : "0", cflags);
    fprintf(f, "LINK_FLAGS=-F0x30 -g -blit:%s.%s -bdata:0.%s -rwh:SSCParmBlk:0x00F7.0x00FF%s%s\n",
            relstart, lastrom, lastram, *codec ? " " : "", codec);
    fprintf(f, "LINK_LIBS=@AUTO@\nLASTROM=%s\nLASTRAM=%s\nNUM_PORTS=%d\nFLASH_KB=%d\n", lastrom, lastram, num_ports(gp), kb);
    fclose(f);
    printf("Variante importada: %s (%s, %s), %d KB, puertos P0..P%d, %s\n", disp, base, pkg, kb,
           num_ports(gp) - 1, (*lmm && strcmp(lmm, "0")) ? "modelo de memoria grande" : "modelo pequeno");
    free(opts); free(xml); free(gp);
    return 0;
}

/* baja hasta 3 niveles buscando la carpeta con boot.asm/opts.txt/project.mk */
static int find_project(const char *root, char *out, int depth)
{
    char t[MAXP];
    const char *marks[] = { "project.mk", "opts.txt", "PSoCConfig.xml" };
    for (int i = 0; i < 3; i++) {
        snprintf(t, sizeof t, "%s/%s", root, marks[i]);
        if (exists(t)) { snprintf(out, MAXP, "%s", root); return 1; }
    }
    if (depth <= 0) return 0;
    static char names[MAXFILES][260];
    char local[64][260];
    int n = list_dir(root, names, MAXFILES, 1);
    if (n > 64) n = 64;
    memcpy(local, names, sizeof(local[0]) * (size_t)n);
    for (int i = 0; i < n; i++) {
        if (!strcasecmp(local[i], "lib") || !strcasecmp(local[i], "backup") || !strcasecmp(local[i], "obj") ||
            !strcasecmp(local[i], "output") || !strcasecmp(local[i], "Templates")) continue;
        snprintf(t, sizeof t, "%s/%s", root, local[i]);
        if (find_project(t, out, depth - 1)) return 1;
    }
    return 0;
}

static int cmd_import(int argc, char **argv)
{
    if (argc < 2) die("uso: psocbuild import CARPETA_PROYECTO_PSOC_DESIGNER CARPETA_VARIANTE");
    char proj[MAXP], vdir[MAXP], p[MAXP], q[MAXP];
    absolute(argv[0], proj);
    {
        char found[MAXP];
        if (!find_project(proj, found, 3))
            die("no encuentro un proyecto de PSoC Designer en %s (falta project.mk / opts.txt)", proj);
        snprintf(proj, sizeof proj, "%s", found);
    }
    if (!strchr(argv[1], '/') && !strchr(argv[1], '\\')) /* solo el nombre: carpeta de usuario */
        snprintf(vdir, sizeof vdir, "%s/%s", user_variants(), argv[1]);
    else absolute(argv[1], vdir);
    mkdirs(vdir);

    /* el proyecto puede ser la carpeta del workspace o la del proyecto */
    snprintf(p, sizeof p, "%s/project.mk", proj);
    if (!exists(p)) return import_generated(proj, vdir);
    {   /* project.mk sin linkfile/libpsoc.a: la compilacion de PSoC Designer fallo */
        char l1[MAXP], l2[MAXP], l3[MAXP];
        snprintf(l1, sizeof l1, "%s/linkfile", proj);
        snprintf(l2, sizeof l2, "%s/lib/libpsoc.a", proj);
        snprintf(l3, sizeof l3, "%s/lib/libPSoC.a", proj);
        if (!exists(l1) || (!exists(l2) && !exists(l3))) return import_generated(proj, vdir);
    }
    if (!exists(p)) die("no encuentro %s: elija la carpeta del proyecto (la que tiene project.mk,\n"
                        "  boot.asm y lib/) y compilelo antes en PSoC Designer", p);
    char *mk = read_file(p, NULL);
    snprintf(p, sizeof p, "%s/linkfile", proj);
    char *lk = read_file(p, NULL);
    if (!lk) die("falta %s: compile (Build) el proyecto en PSoC Designer primero", p);
    snprintf(p, sizeof p, "%s/lib/libpsoc.a", proj);
    if (!exists(p)) {
        snprintf(p, sizeof p, "%s/lib/libPSoC.a", proj);
        if (!exists(p)) die("falta lib/libpsoc.a: compile el proyecto en PSoC Designer primero");
    }

    char device[128], base[128], psd_short[MAXP], psd_long[MAXP], inc[MAXP];
    snprintf(device, sizeof device, "%s", mk_get(mk, "DEVICE"));
    snprintf(base, sizeof base, "%s", mk_get(mk, "BASEDEVICE"));
    snprintf(psd_short, sizeof psd_short, "%s", mk_get(mk, "PSOCDIR"));
    slashes(psd_short);
    longp(psd_short, psd_long);
    snprintf(inc, sizeof inc, "%s", mk_get(mk, "INCLUDE_PATH"));
    slashes(inc);
    char lmm[32], more_cflags[MAXP], cdefines[MAXP];
    snprintf(lmm, sizeof lmm, "%s", mk_get(mk, "LMM"));
    snprintf(more_cflags, sizeof more_cflags, "%s", mk_get(mk, "MORE_CFLAGS"));
    snprintf(cdefines, sizeof cdefines, "%s", mk_get(mk, "CDEFINES"));
    if (!*device || !*psd_short) die("project.mk incompleto (DEVICE/PSOCDIR)");

    /* include del dispositivo, relativo a PSOCDIR */
    char inc_sub[MAXP] = "";
    if (!strncasecmp(inc, psd_short, strlen(psd_short))) {
        char l[MAXP];
        longp(inc, l);
        if (!strncasecmp(l, psd_long, strlen(psd_long)) && l[strlen(psd_long)] == '/')
            snprintf(inc_sub, sizeof inc_sub, "%s", l + strlen(psd_long) + 1);
        else snprintf(inc_sub, sizeof inc_sub, "%s", inc + strlen(psd_short) + 1);
    }

    /* linkfile: opciones (antes de -o) y bibliotecas (desde lib/libpsoc.a) */
    for (char *c = lk; *c; c++) if (*c == '\r' || *c == '\n' || *c == '\t') *c = ' ';
    slashes(lk);
    char flags[MAXP] = "", libs[MAXCMD] = "";
    {
        char *o = strstr(lk, "-o\"");
        if (!o) o = strstr(lk, "-o");
        if (!o) die("linkfile sin -o");
        size_t n = (size_t)(o - lk);
        if (n >= sizeof flags) n = sizeof flags - 1;
        memcpy(flags, lk, n);
        flags[n] = 0;
        char *l = NULL, *t;
        if ((t = strstr(lk, "lib/libPSoc.a")) || (t = strstr(lk, "lib/libpsoc.a")) ||
            (t = strstr(lk, "lib/libPSoC.a")) || (t = strstr(lk, "lib/libPSOC.a"))) {
            l = t;
            while (l > lk && l[-1] != ' ') l--;
        }
        if (!l) die("linkfile sin lib/libpsoc.a");
        snprintf(libs, sizeof libs, "%s", l);
        /* ./lib/libPSoc.a -> @VARIANT@/lib/libpsoc.a ; PSOCDIR -> @PSOCDIR@ */
        char *lp = strstr(libs, "lib/libP");
        if (!lp) lp = strstr(libs, "lib/libp");
        char rest[MAXCMD];
        snprintf(rest, sizeof rest, "@VARIANT@/lib/libpsoc.a%s", strchr(lp, ' ') ? strchr(lp, ' ') : "");
        snprintf(libs, sizeof libs, "%s", rest);
        replace_all(libs, sizeof libs, psd_short, "@PSOCDIR@");
        replace_all(libs, sizeof libs, psd_long, "@PSOCDIR@");
        while (*libs && libs[strlen(libs) - 1] == ' ') libs[strlen(libs) - 1] = 0;
        while (flags[0] == ' ') memmove(flags, flags + 1, strlen(flags));
        while (*flags && flags[strlen(flags) - 1] == ' ') flags[strlen(flags) - 1] = 0;
        /* PSoC Designer compacta espacios dobles */
        while (strstr(flags, "  ")) replace_all(flags, sizeof flags, "  ", " ");
        while (strstr(libs, "  ")) replace_all(libs, sizeof libs, "  ", " ");
    }

    /* copiar archivos */
    mkdir_p(vdir);
    snprintf(q, sizeof q, "%s/lib", vdir);
    mkdir_p(q);
    const char *top[] = { "boot.asm", "boot.tpl", "memory.inc", "flashsecurity.txt", "project.mk", "linkfile" };
    for (unsigned i = 0; i < sizeof top / sizeof top[0]; i++) {
        snprintf(p, sizeof p, "%s/%s", proj, top[i]);
        snprintf(q, sizeof q, "%s/%s%s", vdir, top[i], (i >= 4) ? ".orig" : "");
        if (exists(p) && copy_file(p, q)) die("no puedo copiar %s", p);
    }
#ifdef _WIN32
    {
        WIN32_FIND_DATAA fd;
        snprintf(p, sizeof p, "%s/lib/*", proj);
        HANDLE h = FindFirstFileA(p, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                snprintf(p, sizeof p, "%s/lib/%s", proj, fd.cFileName);
                snprintf(q, sizeof q, "%s/lib/%s", vdir, fd.cFileName);
                for (char *c = q + strlen(vdir) + 5; *c; c++)
                    if (!strcasecmp(c, "libPSoc.a") || !strcasecmp(c, "libpsoc.a")) { strcpy(c, "libpsoc.a"); break; }
                copy_file(p, q);
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
    }
#else
    {
        char c2[MAXCMD];
        snprintf(c2, sizeof c2, "cp \"%s\"/lib/* \"%s/lib/\" 2>/dev/null; cd \"%s/lib\" && for f in libPSoc.a libPSoC.a libPSOC.a; do [ -f $f ] && mv $f libpsoc.a; done; true",
                 proj, vdir, vdir);
        if (system(c2) == -1) die("cp");
    }
#endif

    /* boot.asm: vector del Sleep Timer -> millis() */
    snprintf(q, sizeof q, "%s/boot.asm", vdir);
    long bl;
    char *boot = read_file(q, &bl);
    if (!boot) die("el proyecto no tiene boot.asm");
    size_t cap = (size_t)bl + 1024;
    boot = realloc(boot, cap);
    if (!boot || patch_sleep_vector(boot, cap)) die("no encuentro el vector 64h (Sleep Timer) en boot.asm");
    write_file(q, boot, (long)strlen(boot));
    free(boot);

    /* reloj de CPU */
    snprintf(p, sizeof p, "%s/lib/GlobalParams.inc", vdir);
    char *gp = read_file(p, NULL);
    long hz = gp ? cpu_hz(gp) : 0;
    int nports = gp ? num_ports(gp) : 3;
    free(gp);

    char cflags[MAXP];
    snprintf(cflags, sizeof cflags, "%s%s%s", more_cflags, *cdefines ? " " : "", cdefines);
    /* MORE_CFLAGS ya trae -Wf-LMM2/-Wf-LMM8 -D_LMM en los modelos grandes */
    {
        size_t n = strlen(cflags);
        while (n && cflags[n - 1] == ' ') cflags[--n] = 0;
    }

    snprintf(q, sizeof q, "%s/variant.cfg", vdir);
    FILE *f = fopen(q, "w");
    if (!f) die("no puedo escribir %s", q);
    fprintf(f, "# Generado por 'psocbuild import' desde %s\n", proj);
    fprintf(f, "DEVICE=%s\nBASEDEVICE=%s\nPSOCDIR=%s\nINCLUDE_SUB=%s\n", device, base, psd_long, inc_sub);
    fprintf(f, "F_CPU=%ld\nLMM=%s\nCFLAGS=%s\n", hz ? hz : 24000000L, *lmm ? lmm : "0", cflags);
    fprintf(f, "LINK_FLAGS=%s\nLINK_LIBS=%s\n", flags, libs);
    fprintf(f, "LASTROM=%s\n", mk_get(mk, "LASTROM"));
    fprintf(f, "LASTRAM=%s\n", mk_get(mk, "LASTRAM"));
    fprintf(f, "NUM_PORTS=%d\n", nports);
    fclose(f);
    printf("Variante %s importada: %s (base %s), CPU %ld Hz%s\n", vdir, device, base, hz,
           (*lmm && strcmp(lmm, "0")) ? ", modelo de memoria grande" : "");
    printf("  enlace: %s\n  bibliotecas: %s\n", flags, libs);
    free(mk);
    free(lk);
    return 0;
}

/* --------------------------------------------------------------- doctor */
static int cmd_doctor(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
        if (!strcmp(argv[i], "-V") && i + 1 < argc) load_variant(argv[++i]);
    printf("PSoC Designer: %s\n", psocdir());
    const char *names[] = { "iccm8c", "iasm8c", "ilinkm8c", "ilibw" };
    for (int i = 0; i < 4; i++) printf("  %-9s %s\n", names[i], tool(names[i]));
    if (ncfg) {
        printf("Variante %s\n", variant_dir);
        for (int i = 0; i < ncfg; i++) printf("  %s=%s\n", cfg_k[i], cfg_v[i]);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "uso: psocbuild cc|as|ar|link|size|preproc|import|doctor ... (ver codigo)\n");
        return 2;
    }
    const char *c = argv[1];
    argc -= 2;
    argv += 2;
    if (!strcmp(c, "cc")) return cmd_compile(argc, argv, 0);
    if (!strcmp(c, "as")) return cmd_compile(argc, argv, 1);
    if (!strcmp(c, "ar")) return cmd_ar(argc, argv);
    if (!strcmp(c, "link")) return cmd_link(argc, argv);
    if (!strcmp(c, "size")) return cmd_size(argc, argv);
    if (!strcmp(c, "preproc")) return cmd_preproc(argc, argv);
    if (!strcmp(c, "import")) return cmd_import(argc, argv);
    if (!strcmp(c, "doctor")) return cmd_doctor(argc, argv);
    if (!strcmp(c, "--version")) { puts("psocbuild 0.2.0"); return 0; }
    die("comando desconocido: %s", c);
    return 2;
}
