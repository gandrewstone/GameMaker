/* Self-checking test of the port's platform pieces that need no window or audio device: the settings file, the
 * picture placement maths, DOS-style file names (case, separators), directory listing and the volume controls.
 * Exit status 0 = all checks passed. Run it from a scratch folder: it creates ./pt_work. */
#include "audio.h"
#include "dosplat.h"
#include "fb.h"
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#define SETENV(k, v) _putenv_s(k, v)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir((p), 0755)
#define SETENV(k, v) setenv(k, v, 1)
#endif

static int checks, failed;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failed++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int near_f(float a, float b) { return a - b < 0.5f && b - a < 0.5f; }

static void test_settings(void)
{
    MKDIR("pt_work");
    SETENV("GM_CONFIG_DIR", "pt_work/cfg");
    gm_settings.fullscreen = 1; gm_settings.scale_mode = GM_SCALE_FITSQ; gm_settings.volume = 40; gm_settings.mute = 1; gm_settings.gamepad = 0;
    gm_settings_save();
    gm_settings.fullscreen = 0; gm_settings.scale_mode = GM_SCALE_INT43; gm_settings.volume = 100; gm_settings.mute = 0; gm_settings.gamepad = 1;
    gm_settings_load();
    CHECK(gm_settings.fullscreen == 1 && gm_settings.scale_mode == GM_SCALE_FITSQ && gm_settings.volume == 40 &&
          gm_settings.mute == 1 && gm_settings.gamepad == 0, "settings round trip: %d %d %d %d %d", gm_settings.fullscreen,
          gm_settings.scale_mode, gm_settings.volume, gm_settings.mute, gm_settings.gamepad);

    SETENV("GM_NO_SETTINGS", "1");                      /* test mode: neither read nor write */
    gm_settings.volume = 5;
    gm_settings_load();
    CHECK(gm_settings.volume == 5, "GM_NO_SETTINGS must not read the file");
    SETENV("GM_NO_SETTINGS", "");
#ifdef _WIN32
    _putenv("GM_NO_SETTINGS=");
#else
    unsetenv("GM_NO_SETTINGS");
#endif
    gm_settings.fullscreen = 0; gm_settings.scale_mode = GM_SCALE_INT43; gm_settings.volume = 100; gm_settings.mute = 0; gm_settings.gamepad = 1;
}

static void test_place(void)
{
    FbRect r;
    r = fb_place(320, 200, 1920, 1080, GM_SCALE_INT43);          /* 320x240 units: 4x -> 1280x960 */
    CHECK(near_f(r.w, 1280) && near_f(r.h, 960) && near_f(r.x, 320) && near_f(r.y, 60), "int43 1080p: %g,%g %gx%g", r.x, r.y, r.w, r.h);
    r = fb_place(320, 200, 1920, 1080, GM_SCALE_FIT43);          /* 4.5x */
    CHECK(near_f(r.w, 1440) && near_f(r.h, 1080) && near_f(r.x, 240) && near_f(r.y, 0), "fit43 1080p: %g,%g %gx%g", r.x, r.y, r.w, r.h);
    r = fb_place(320, 200, 1920, 1080, GM_SCALE_INTSQ);          /* 5x, 1:1 pixels */
    CHECK(near_f(r.w, 1600) && near_f(r.h, 1000) && near_f(r.x, 160) && near_f(r.y, 40), "intsq 1080p: %g,%g %gx%g", r.x, r.y, r.w, r.h);
    r = fb_place(320, 200, 1920, 1080, GM_SCALE_FITSQ);          /* 5.4x */
    CHECK(near_f(r.w, 1728) && near_f(r.h, 1080) && near_f(r.x, 96) && near_f(r.y, 0), "fitsq 1080p: %g,%g %gx%g", r.x, r.y, r.w, r.h);
    r = fb_place(320, 200, 200, 150, GM_SCALE_INT43);            /* smaller than the picture: fit instead of 0x */
    CHECK(near_f(r.w, 200) && near_f(r.h, 150), "int43 tiny window: %gx%g", r.w, r.h);
    r = fb_place(640, 400, 1920, 1080, GM_SCALE_INT43);          /* text screen: 640x480 units, 2x */
    CHECK(near_f(r.w, 1280) && near_f(r.h, 960), "int43 text screen: %gx%g", r.w, r.h);
    r = fb_place(320, 200, 960, 720, GM_SCALE_INT43);            /* the default window is exactly 3x */
    CHECK(near_f(r.w, 960) && near_f(r.h, 720) && near_f(r.x, 0) && near_f(r.y, 0), "int43 default window");
}

static void test_files(void)
{
    MKDIR("pt_work/Games");
    MKDIR("pt_work/Games/PEACHY");
    FILE *f = fopen("pt_work/Games/PEACHY/Peach.BBL", "wb");
    CHECK(f != NULL, "could not create the test file");
    if (f) { fputs("x", f); fclose(f); }
    f = fopen("pt_work/Games/PEACHY/readme.txt", "wb");                 /* not 8.3 shaped on purpose below */
    if (f) fclose(f);
    f = fopen("pt_work/Games/PEACHY/a_name_that_is_too_long.dat", "wb");
    if (f) fclose(f);

    char path[300];
    int found = gm_resolve_path("pt_work\\games\\peachy\\PEACH.bbl", path, sizeof path);
    CHECK(found, "resolve: existing file by other case and backslashes");
    f = gm_fopen("pt_work\\games\\peachy\\PEACH.bbl", "rb");
    CHECK(f != NULL, "gm_fopen: DOS spelling of an existing file");
    if (f) fclose(f);
    f = gm_fopen("pt_work\\Games\\NOPE\\x.dat", "rb");
    CHECK(f == NULL, "gm_fopen: missing file");

    f = gm_fopen("PT_WORK\\games\\peachy\\New.SAV", "wb");               /* new file in an existing folder, found by case */
    CHECK(f != NULL, "gm_fopen: create in a folder spelled in another case");
    if (f) { fputs("y", f); fclose(f); }
    f = fopen("pt_work/Games/PEACHY/New.SAV", "rb");
#ifdef _WIN32
    CHECK(f != NULL, "created file present");
#else
    CHECK(f != NULL, "created file must sit in the existing folder, not a lower-case twin");
#endif
    if (f) fclose(f);
    CHECK(gm_remove("pt_work\\GAMES\\peachy\\new.sav") == 0, "gm_remove with another case");

    struct ffblk ff;
    int names = 0, saw_peach = 0, saw_long = 0;
    for (int r = gm_findfirst("pt_work\\games\\peachy\\*.*", &ff, 0); r == 0; r = gm_findnext(&ff)) {
        names++;
        if (!strcmp(ff.ff_name, "PEACH.BBL")) saw_peach = 1;
        if (strstr(ff.ff_name, "TOO_LONG") || strstr(ff.ff_name, "A_NAME_T")) saw_long = 1;
    }
    CHECK(saw_peach, "findfirst/findnext: PEACH.BBL listed in upper case");
#ifndef _WIN32
    CHECK(!saw_long, "names that do not fit 8.3 are hidden, as in DOS");
#endif
    CHECK(names >= 1, "findfirst found %d names", names);
    int bbl = 0;
    for (int r = gm_findfirst("pt_work\\games\\peachy\\*.bbl", &ff, 0); r == 0; r = gm_findnext(&ff)) bbl++;
    CHECK(bbl == 1, "findfirst with a wildcard pattern found %d", bbl);
    CHECK(gm_findfirst("pt_work\\games\\peachy\\*.xyz", &ff, 0) != 0, "findfirst with no match");
    int dirs = 0;
    for (int r = gm_findfirst("pt_work\\games\\*.*", &ff, 0x10); r == 0; r = gm_findnext(&ff))
        if (ff.ff_attrib & 0x10) dirs++;
    CHECK(dirs >= 1, "findfirst with the directory attribute lists folders (%d)", dirs);
}

static void test_audio(void)
{
    gm_audio_set_volume(70);
    CHECK(gm_audio_volume() == 70, "volume set");
    gm_audio_set_volume(500);
    CHECK(gm_audio_volume() == 100, "volume clamps high");
    gm_audio_set_volume(-5);
    CHECK(gm_audio_volume() == 0, "volume clamps low");
    gm_audio_set_mute(1);
    CHECK(gm_audio_muted() == 1, "mute on");
    gm_audio_set_mute(0);
    CHECK(gm_audio_muted() == 0, "mute off");
}

int main(void)
{
    test_settings();
    test_place();
    test_files();
    test_audio();
    printf("platformtest: %d checks, %d failed\n", checks, failed);
    return failed ? 1 : 0;
}
