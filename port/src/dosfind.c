/* Borland findfirst/findnext over the Win32 API. Names come back DOS-style: 8.3 short names, upper case.
 * (Kept apart from dosplat.c because <windows.h> clashes with raylib's symbol names.) */
#include "dosplat.h"
#include <ctype.h>
#include <string.h>

#include "osclock.h"

#ifdef _WIN32
#include <windows.h>

double gm_os_time(void)
{
    static LARGE_INTEGER freq, t0;
    LARGE_INTEGER t;
    if (!freq.QuadPart) { QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&t0); }
    QueryPerformanceCounter(&t);
    return (double)(t.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
}

void gm_os_sleep_ms(unsigned ms) { Sleep(ms); }
void gm_os_yield(void) {}

static void fill_ffblk(struct ffblk *f, const WIN32_FIND_DATAA *d)
{
    const char *name = d->cAlternateFileName[0] ? d->cAlternateFileName : d->cFileName;   /* short name if any */
    size_t n = strlen(name);
    if (n > sizeof f->ff_name - 1) n = sizeof f->ff_name - 1;
    for (size_t i = 0; i < n; i++) f->ff_name[i] = (char)toupper((unsigned char)name[i]);
    f->ff_name[n] = 0;
    f->ff_attrib = (char)((d->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? 0x10 : 0) |
                          (d->dwFileAttributes & FILE_ATTRIBUTE_READONLY ? 0x01 : 0) |
                          (d->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? 0x02 : 0) |
                          (d->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? 0x04 : 0) |
                          (d->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? 0x20 : 0));
    f->ff_fsize = (int32_t)d->nFileSizeLow;
}

/* DOS rule: an entry matches if its attributes are within (normal files | requested attributes) */
static int attr_ok(const struct ffblk *f, int16_t want)
{
    return ((unsigned char)f->ff_attrib & ~(unsigned)(want | 0x20 | 0x01)) == 0;
}

static int16_t last_attrib;

int16_t gm_findfirst(const char *pattern, struct ffblk *f, int16_t attrib)
{
    WIN32_FIND_DATAA d;
    HANDLE h = FindFirstFileA(pattern, &d);
    if (h == INVALID_HANDLE_VALUE) return -1;
    f->ff_handle = (intptr_t)h;
    last_attrib = attrib;
    for (;;) {
        fill_ffblk(f, &d);
        if (attr_ok(f, attrib)) return 0;
        if (!FindNextFileA(h, &d)) { FindClose(h); f->ff_handle = 0; return -1; }
    }
}

int16_t gm_findnext(struct ffblk *f)
{
    HANDLE h = (HANDLE)f->ff_handle;
    WIN32_FIND_DATAA d;
    if (!h) return -1;
    while (FindNextFileA(h, &d)) {
        fill_ffblk(f, &d);
        if (attr_ok(f, last_attrib)) return 0;
    }
    FindClose(h);
    f->ff_handle = 0;
    return -1;
}
#else
#include <time.h>

double gm_os_time(void)
{
    static struct timespec t0;
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    if (!t0.tv_sec && !t0.tv_nsec) t0 = t;
    return (double)(t.tv_sec - t0.tv_sec) + (double)(t.tv_nsec - t0.tv_nsec) / 1e9;
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
/* In a browser the page can only run while this code is parked: emscripten_sleep() hands control back (Asyncify). */
void gm_os_sleep_ms(unsigned ms) { emscripten_sleep(ms); }
void gm_os_yield(void) { emscripten_sleep(0); }
#else
void gm_os_sleep_ms(unsigned ms)
{
    struct timespec d = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&d, NULL);
}
void gm_os_yield(void) {}
#endif

/* POSIX: list a directory and match the DOS wildcard pattern ("*", "?", case-insensitive) ourselves.
 * Only names that fit 8.3 are reported, because that is all DOS (and the engine's 14-byte name slots) can hold. */
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#include <stdlib.h>

typedef struct { DIR *dir; char base[1024]; char pat[64]; int16_t attrib; } Find;

static int wild(const char *p, const char *s)
{
    for (; *p; p++, s++) {
        if (*p == '*') {
            while (p[1] == '*') p++;
            if (!p[1]) return 1;
            for (; *s; s++) if (wild(p + 1, s)) return 1;
            return wild(p + 1, s);
        }
        if (!*s) return 0;
        if (*p != '?' && toupper((unsigned char)*p) != toupper((unsigned char)*s)) return 0;
    }
    return !*s;
}

static int is_83(const char *n)
{
    if (!strcmp(n, ".") || !strcmp(n, "..")) return 1;
    const char *dot = strchr(n, '.');
    size_t base = dot ? (size_t)(dot - n) : strlen(n);
    if (base == 0 || base > 8) return 0;
    if (dot && (strchr(dot + 1, '.') || strlen(dot + 1) > 3)) return 0;
    return 1;
}

static int next_entry(Find *h, struct ffblk *f)
{
    struct dirent *e;
    while ((e = readdir(h->dir)) != NULL) {
        if (!is_83(e->d_name) || !wild(h->pat, e->d_name)) continue;
        char path[1400];
        struct stat st;
        snprintf(path, sizeof path, "%s/%s", h->base, e->d_name);
        if (stat(path, &st) != 0) continue;
        unsigned attr = S_ISDIR(st.st_mode) ? 0x10 : 0x20;
        if (!(st.st_mode & S_IWUSR)) attr |= 0x01;
        if (e->d_name[0] == '.' && strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) attr |= 0x02;
        if ((attr & ~(unsigned)(h->attrib | 0x20 | 0x01)) != 0) continue;   /* DOS rule, as on Windows above */
        size_t n = 0;
        for (; e->d_name[n]; n++) f->ff_name[n] = (char)toupper((unsigned char)e->d_name[n]);
        f->ff_name[n] = 0;
        f->ff_attrib = (char)attr;
        f->ff_fsize = (int32_t)st.st_size;
        return 0;
    }
    closedir(h->dir);
    free(h);
    f->ff_handle = 0;
    return -1;
}

int16_t gm_findfirst(const char *pattern, struct ffblk *f, int16_t attrib)
{
    char dosdir[1024], pat[64];
    const char *slash = NULL;
    for (const char *p = pattern; *p; p++) if (*p == '\\' || *p == '/') slash = p;
    size_t dl = slash ? (size_t)(slash - pattern) : 0;
    if (dl >= sizeof dosdir) return -1;
    memcpy(dosdir, pattern, dl);
    dosdir[dl] = 0;
    const char *pt = slash ? slash + 1 : pattern;
    if (strlen(pt) >= sizeof pat) return -1;
    strcpy(pat, !strcmp(pt, "*.*") ? "*" : pt);       /* DOS: *.* matches every name */

    Find *h = (Find *)calloc(1, sizeof *h);
    if (!h) return -1;
    if (slash && dl == 0) strcpy(dosdir, "/");         /* "\file" is the root */
    gm_resolve_path(dl || slash ? dosdir : ".", h->base, sizeof h->base);
    h->dir = opendir(h->base);
    if (!h->dir) { free(h); return -1; }
    strcpy(h->pat, pat);
    h->attrib = attrib;
    f->ff_handle = (intptr_t)h;
    return (int16_t)next_entry(h, f);
}

int16_t gm_findnext(struct ffblk *f)
{
    Find *h = (Find *)f->ff_handle;
    if (!h) return -1;
    return (int16_t)next_entry(h, f);
}
#endif
