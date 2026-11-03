/* psocbuild - puente entre el Arduino IDE y el compilador ImageCraft M8C
 * (ICCM8C) de PSoC Designer.  Proyecto PsocDuino.
 *
 *   psocbuild cc      [-v] -V VARIANT -o OUT.o SRC [flags...]   compila C (.c/.cpp/.ino)
 *   psocbuild as      [-v] -V VARIANT -o OUT.o SRC [flags...]   ensambla (.asm/.S)
 *   psocbuild ar      [-v] -V VARIANT ARCHIVE OBJ                agrega a una biblioteca
 *   psocbuild link    [-v] -V VARIANT -o OUTBASE OBJ... LIB...   boot.asm + enlace -> .hex
 *   psocbuild size    MAPFILE                                    uso de flash y RAM
 *   psocbuild preproc SRC OUT                                    (para el IDE: copia)
 *   psocbuild import  PROJECT_DIR VARIANT_DIR                     crea una variante
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
#include <unistd.h>
#define mkdir_p(p) mkdir(p, 0755)
#define EXE ""
#endif

#define MAXP 4096
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

static void load_variant(const char *dir)
{
    char p[MAXP];
    absolute(dir, variant_dir);
    snprintf(p, sizeof p, "%s/variant.cfg", variant_dir);
    char *b = read_file(p, NULL);
    if (!b)
        die("la variante %s no esta importada (falta variant.cfg).\n"
            "  Cree un proyecto vacio en PSoC Designer para este chip, compilelo y ejecute:\n"
            "  psocbuild import <carpeta_del_proyecto> %s", variant_dir, variant_dir);
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
        if (is_dir(t)) { snprintf(d, sizeof d, "%s", cands[i]); slashes(d); return d; }
    }
    die("no encuentro PSoC Designer (CypressSemiBuildMgr). Defina PSOC_DESIGNER_DIR,\n"
        "  por ejemplo: C:\\Program Files (x86)\\Cypress\\PSoC Designer\\5.4\\Common\\CypressSemiBuildMgr");
    return NULL;
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
            snprintf(t, MAXP, "%s", s);
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

static void std_includes(char *cmd)
{
    char p[MAXP], s[MAXP];
    snprintf(p, sizeof p, "%s/tools/include", psocdir());
    shortp(p, s);
    size_t n = strlen(cmd);
    snprintf(cmd + n, MAXCMD - n, " -I%s", s);
    if (*cfg("INCLUDE_SUB")) {
        snprintf(p, sizeof p, "%s/%s", psocdir(), cfg("INCLUDE_SUB"));
        shortp(p, s);
        n = strlen(cmd);
        snprintf(cmd + n, MAXCMD - n, " -I%s", s);
    }
    shortp(variant_dir, s);
    n = strlen(cmd);
    snprintf(cmd + n, MAXCMD - n, " -I%s -I%s/lib", s, s);
}

/* -I/-D/-o con ruta -> acortar */
static void add_flag(char *cmd, const char *a)
{
    char s[MAXP];
    if (!strncmp(a, "-I", 2) && a[2]) {
        shortp(a + 2, s);
        size_t n = strlen(cmd);
        snprintf(cmd + n, MAXCMD - n, " -I%s", s);
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
        snprintf(cmd + n, MAXCMD - n, " -c -e %s -DF_CPU=%sUL -D__PSOCDUINO__", extra,
                 *cfg("F_CPU") ? cfg("F_CPU") : "24000000");
    }
    std_includes(cmd);
    for (int i = 0; i < nf; i++) add_flag(cmd, flags[i]);
    shortp(out, s);
    { size_t n = strlen(cmd); snprintf(cmd + n, MAXCMD - n, " -o %s", s); }
    shortp(work, s);
    { size_t n = strlen(cmd); snprintf(cmd + n, MAXCMD - n, " %s", s); }

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
    snprintf(cmd, sizeof cmd, "%s -a %s %s", tool("ilibw"), a, o);
    return run(cmd, NULL) ? 1 : 0;
}

/* ----------------------------------------------------------------- link */
static int cmd_link(int argc, char **argv)
{
    const char *vdir = NULL, *outbase = NULL;
    char *objs[1024];
    int no = 0;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-V") && i + 1 < argc) vdir = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) outbase = argv[++i];
        else if (!strcmp(argv[i], "-v")) verbose = 1;
        else if (no < 1024) objs[no++] = argv[i];
    }
    if (!vdir || !outbase) die("uso: psocbuild link -V VARIANT -o SALIDA objetos...");
    load_variant(vdir);

    char outdir[MAXP], s[MAXP], cmd[MAXCMD], cwd[MAXP];
    snprintf(outdir, sizeof outdir, "%s", outbase);
    slashes(outdir);
    char *sl = strrchr(outdir, '/');
    if (sl) *sl = 0; else snprintf(outdir, sizeof outdir, ".");

    /* 1. boot.asm de la variante (con el vector del Sleep Timer de PsocDuino) */
    char bootobj[MAXP], bootcap[MAXP];
    snprintf(bootobj, sizeof bootobj, "%s/boot.o", outdir);
    absolute(bootobj, s);
    snprintf(bootobj, sizeof bootobj, "%s", s);
    snprintf(bootcap, sizeof bootcap, "%s/boot.log", outdir);
    absolute(bootcap, s);
    snprintf(bootcap, sizeof bootcap, "%s", s);
    snprintf(cmd, sizeof cmd, "%s", tool("iasm8c"));
    std_includes(cmd);
    shortp(bootobj, s);
    { size_t n = strlen(cmd); snprintf(cmd + n, MAXCMD - n, " -o %s boot.asm", s); }
    if (!getcwd(cwd, sizeof cwd)) die("getcwd");
    if (chdir(variant_dir)) die("no puedo entrar en %s", variant_dir);
    int r = run(cmd, bootcap);
    if (chdir(cwd)) die("chdir");
    if (show_output(bootcap, NULL, NULL) || r) { fprintf(stderr, "psocbuild: error ensamblando boot.asm\n"); return 1; }
    remove(bootcap);

    /* 2. linkfile con las mismas opciones que genero PSoC Designer */
    char lf[MAXP], flags[MAXP], libs[MAXCMD];
    snprintf(lf, sizeof lf, "%s/linkfile", outdir);
    expand(cfg("LINK_FLAGS"), flags, sizeof flags);
    expand(cfg("LINK_LIBS"), libs, sizeof libs);
    FILE *f = fopen(lf, "w");
    if (!f) die("no puedo escribir %s", lf);
    /* nombre interno sin puntos: ImageCraft podria tomar ".ino" como extension */
    char fw[MAXP];
    snprintf(fw, sizeof fw, "%s/psocduino_fw", outdir);
    shortp(fw, s);
    fprintf(f, "%s -m\n -o\"%s\"\n", flags, s);
    shortp(bootobj, s);
    fprintf(f, "%s\n", s);
    for (int i = 0; i < no; i++) {
        shortp(objs[i], s);
        fprintf(f, "%s\n", s);
    }
    fprintf(f, " %s\n", libs);
    fclose(f);

    char lfs[MAXP], cap[MAXP];
    shortp(lf, lfs);
    snprintf(cap, sizeof cap, "%s/link.log", outdir);
    snprintf(cmd, sizeof cmd, "%s @%s", tool("ilinkm8c"), lfs);
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

static int cmd_import(int argc, char **argv)
{
    if (argc < 2) die("uso: psocbuild import CARPETA_PROYECTO_PSOC_DESIGNER CARPETA_VARIANTE");
    char proj[MAXP], vdir[MAXP], p[MAXP], q[MAXP];
    absolute(argv[0], proj);
    absolute(argv[1], vdir);

    /* el proyecto puede ser la carpeta del workspace o la del proyecto */
    snprintf(p, sizeof p, "%s/project.mk", proj);
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
    free(gp);

    char cflags[MAXP];
    snprintf(cflags, sizeof cflags, "%s%s%s", more_cflags, *cdefines ? " " : "", cdefines);
    if (*lmm && strcmp(lmm, "0")) {
        size_t n = strlen(cflags);
        snprintf(cflags + n, sizeof cflags - n, " -Wf-LMM -D__LMM__");
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
