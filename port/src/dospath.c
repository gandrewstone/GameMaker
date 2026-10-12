/* DOS-style file names on a case-sensitive file system. See dosplat.h.
 *
 * The 1994 engine builds names like "SAMPLE\SAMPLE.GAM" or "peach.bbl" and expects the file system to ignore case
 * and accept "\". On Linux (and macOS volumes formatted case-sensitive) that needs help: gm_resolve_path() walks the
 * name one component at a time and, where a component does not exist as written, looks for a directory entry that
 * matches it ignoring case. Where nothing matches (a file about to be created) the rest of the name is kept as is.
 */
#include "dosplat.h"
#include <string.h>

#ifndef _WIN32
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>

static int exists(const char *p) { struct stat st; return stat(p, &st) == 0; }

int gm_resolve_path(const char *dosname, char *out, size_t cap)
{
    char in[1024];
    size_t n = strlen(dosname);
    if (n >= sizeof in) n = sizeof in - 1;
    memcpy(in, dosname, n);
    in[n] = 0;
    for (size_t i = 0; i < n; i++) if (in[i] == '\\') in[i] = '/';
    const char *s = in;
    if (n >= 2 && in[1] == ':' && ((in[0] | 0x20) >= 'a' && (in[0] | 0x20) <= 'z')) s += 2;   /* "c:" */

    char res[1024];
    size_t len = 0;
    if (*s == '/') { res[len++] = '/'; while (*s == '/') s++; }
    int found_all = 1;

    while (*s) {
        char comp[300];
        size_t cl = 0;
        while (s[cl] && s[cl] != '/') cl++;
        if (cl >= sizeof comp) cl = sizeof comp - 1;
        memcpy(comp, s, cl);
        comp[cl] = 0;
        s += cl;
        while (*s == '/') s++;
        if (cl == 0) continue;

        size_t base = len;
        if (len && res[len - 1] != '/') res[len++] = '/';
        size_t at = len;
        if (len + cl + 1 >= sizeof res) break;
        memcpy(res + len, comp, cl);
        len += cl;
        res[len] = 0;

        if (found_all && strcmp(comp, ".") != 0 && strcmp(comp, "..") != 0 && !exists(res)) {
            char dir[1024];
            if (base == 0) strcpy(dir, ".");
            else { memcpy(dir, res, base); dir[base] = 0; if (!dir[0]) strcpy(dir, "/"); }
            int hit = 0;
            DIR *d = opendir(dir);
            if (d) {
                struct dirent *e;
                while ((e = readdir(d)) != NULL) {
                    if (strcasecmp(e->d_name, comp) == 0) {
                        size_t el = strlen(e->d_name);
                        if (at + el + 1 < sizeof res) { memcpy(res + at, e->d_name, el + 1); len = at + el; hit = 1; }
                        break;
                    }
                }
                closedir(d);
            }
            if (!hit) found_all = 0;                /* keep the rest as written */
        }
        if (*s) { res[len] = 0; }
    }
    res[len] = 0;
    if (len == 0) { res[0] = '.'; res[1] = 0; }
    size_t rl = strlen(res);
    if (rl >= cap) rl = cap ? cap - 1 : 0;
    if (cap) { memcpy(out, res, rl); out[rl] = 0; }
    return exists(res);
}

FILE *gm_fopen(const char *name, const char *mode)
{
    char path[1100];
    gm_resolve_path(name, path, sizeof path);
    return fopen(path, mode);
}

int gm_remove(const char *name)
{
    char path[1100];
    gm_resolve_path(name, path, sizeof path);
    return remove(path);
}

/* DOS mkdir takes only the name; the directory is created under the name as the DOS file system would find it */
int gm_mkdir(const char *name)
{
    char path[1100];
    gm_resolve_path(name, path, sizeof path);
    return mkdir(path, 0777);
}

#else   /* Windows: the C runtime already ignores case and accepts both separators */

int gm_resolve_path(const char *dosname, char *out, size_t cap)
{
    size_t n = strlen(dosname);
    if (n >= cap) n = cap ? cap - 1 : 0;
    if (cap) { memcpy(out, dosname, n); out[n] = 0; }
    return 1;
}
FILE *gm_fopen(const char *name, const char *mode) { return fopen(name, mode); }
int gm_remove(const char *name) { return remove(name); }

#endif
