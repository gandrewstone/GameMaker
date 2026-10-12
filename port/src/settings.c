#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir((p), 0755)
#endif

GmSettings gm_settings = { 0, GM_SCALE_INT43, 100, 0, 1 };

static const char *scale_names[GM_SCALE_COUNT] = { "int43", "fit43", "intsq", "fitsq" };
const char *gm_scale_name(int mode) { return mode >= 0 && mode < GM_SCALE_COUNT ? scale_names[mode] : "?"; }

static int config_path(char *out, size_t cap, int make_dir)
{
    char dir[600];
    const char *over = getenv("GM_CONFIG_DIR");
    if (over && *over) snprintf(dir, sizeof dir, "%s", over);
    else {
#ifdef _WIN32
        const char *a = getenv("APPDATA");
        if (!a) return 0;
        snprintf(dir, sizeof dir, "%s\\gmplay", a);
#elif defined(__APPLE__)
        const char *h = getenv("HOME");
        if (!h) return 0;
        snprintf(dir, sizeof dir, "%s/Library/Application Support/gmplay", h);
#else
        const char *x = getenv("XDG_CONFIG_HOME"), *h = getenv("HOME");
        if (x && *x) snprintf(dir, sizeof dir, "%s/gmplay", x);
        else if (h) snprintf(dir, sizeof dir, "%s/.config/gmplay", h);
        else return 0;
#endif
    }
    if (make_dir) {                                   /* create the folder and its parent (one level is enough here) */
        MKDIR(dir);
        char parent[600];
        snprintf(parent, sizeof parent, "%s", dir);
        for (size_t i = strlen(parent); i > 0; i--)
            if (parent[i - 1] == '/' || parent[i - 1] == '\\') { parent[i - 1] = 0; break; }
        if (parent[0]) { MKDIR(parent); MKDIR(dir); }
    }
    snprintf(out, cap, "%s/gmplay.ini", dir);
    return 1;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void gm_settings_load(void)
{
    char path[700];
    if (getenv("GM_NO_SETTINGS") || !config_path(path, sizeof path, 0)) return;
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[200];
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        const char *k = line, *v = eq + 1;
        int n = atoi(v);
        if (!strcmp(k, "fullscreen")) gm_settings.fullscreen = n != 0;
        else if (!strcmp(k, "volume")) gm_settings.volume = clampi(n, 0, 100);
        else if (!strcmp(k, "mute")) gm_settings.mute = n != 0;
        else if (!strcmp(k, "gamepad")) gm_settings.gamepad = n != 0;
        else if (!strcmp(k, "scale"))
            for (int i = 0; i < GM_SCALE_COUNT; i++) if (!strncmp(v, scale_names[i], strlen(scale_names[i]))) gm_settings.scale_mode = i;
    }
    fclose(f);
}

void gm_settings_save(void)
{
    char path[700];
    if (getenv("GM_NO_SETTINGS") || !config_path(path, sizeof path, 1)) return;
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "fullscreen=%d\nscale=%s\nvolume=%d\nmute=%d\ngamepad=%d\n", gm_settings.fullscreen,
            scale_names[gm_settings.scale_mode], gm_settings.volume, gm_settings.mute, gm_settings.gamepad);
    fclose(f);
}
