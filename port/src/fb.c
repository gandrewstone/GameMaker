#include "fb.h"
#include "settings.h"
#include <raylib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

uint8_t fb_pix[FB_W * FB_H];
FbColor fb_pal[256];
FbRect fb_last_rect = { 0, 0, 960, 720 };

static Texture2D tex;
static int tex_w, tex_h;
static uint32_t rgba[FB_W * FB_H];
static int fullscreen;
static int saved_x, saved_y, saved_w, saved_h;

static char toast[96];
static double toast_until;

static void make_texture(int w, int h)
{
    static uint32_t blank[640 * 400];   // zero-filled; the real pixels arrive with UpdateTexture
    if (tex_w) UnloadTexture(tex);
    Image img = { blank, w, h, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    tex_w = w;
    tex_h = h;
}

/* raylib carries on after a failed window set-up and then crashes; stop here with a readable message instead. */
static void log_callback(int level, const char *fmt, va_list args)
{
    char msg[512];
    vsnprintf(msg, sizeof msg, fmt, args);
    if (strstr(msg, "Failed to initialize") || strstr(msg, "Failed to open display") || strstr(msg, "Failed to create")) {
        fprintf(stderr, "gmplay: cannot open a window: %s\n", msg);
#if !defined(_WIN32) && !defined(__APPLE__)
        fprintf(stderr, "gmplay: is there a display? (DISPLAY=%s, WAYLAND_DISPLAY=%s)\n",
                getenv("DISPLAY") ? getenv("DISPLAY") : "unset", getenv("WAYLAND_DISPLAY") ? getenv("WAYLAND_DISPLAY") : "unset");
#endif
        exit(1);
    }
    if (level >= LOG_WARNING) fprintf(stderr, "%s\n", msg);
}

static int fb_opened;

void fb_open(const char *title, int scale)
{
    SetTraceLogCallback(log_callback);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(FB_W * scale, FB_H * 6 / 5 * scale, title);       /* 4:3, like the monitors these games were made for */
    SetWindowMinSize(FB_W, FB_H * 6 / 5);
    SetExitKey(KEY_NULL);               /* Esc belongs to the game; closing the window is the way out of the program */
    SetTargetFPS(60);
    make_texture(FB_W, FB_H);
    fb_opened = 1;
#ifndef __EMSCRIPTEN__                 /* a browser only allows full screen from a click, so the page does it */
    if (gm_settings.fullscreen) fb_set_fullscreen(1);
#endif
}

void fb_close(void)
{
    if (!fb_opened) return;             /* idempotent: the web page closes the window when a program ends, and so may the engine */
    fb_opened = 0;
    UnloadTexture(tex);
    CloseWindow();
}

void fb_set_vga_pal(int idx, int r6, int g6, int b6)
{
    /* expand 6-bit DAC to 8 bits: v<<2 | v>>4 */
    fb_pal[idx].r = (uint8_t)((r6 << 2) | (r6 >> 4));
    fb_pal[idx].g = (uint8_t)((g6 << 2) | (g6 >> 4));
    fb_pal[idx].b = (uint8_t)((b6 << 2) | (b6 >> 4));
}

static void convert(void)
{
    for (int i = 0; i < FB_W * FB_H; i++) {
        FbColor c = fb_pal[fb_pix[i]];
        rgba[i] = c.r | (c.g << 8) | (c.b << 16) | 0xFF000000u;
    }
}

int fb_save_png(const char *path)
{
    convert();
    Image img = { rgba, FB_W, FB_H, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    return ExportImage(img, path) ? 0 : -1;
}

void fb_set_fullscreen(int on)
{
    on = on != 0;
    if (on == fullscreen) return;
#ifdef __EMSCRIPTEN__
    /* The page owns full screen (it needs a user gesture, and also locks the Esc key while full screen) and reports back
     * through gm_web_command(); until it does, assume the request works. */
    emscripten_run_script(on ? "window.gmSetFullscreen && window.gmSetFullscreen(true)" : "window.gmSetFullscreen && window.gmSetFullscreen(false)");
    fullscreen = on;
    return;
#endif
    if (on) {                           /* remember the window so that leaving full screen restores it */
        Vector2 p = GetWindowPosition();
        saved_x = (int)p.x; saved_y = (int)p.y; saved_w = GetScreenWidth(); saved_h = GetScreenHeight();
        ToggleBorderlessWindowed();
    } else {
        ToggleBorderlessWindowed();
        if (saved_w > 0) { SetWindowSize(saved_w, saved_h); SetWindowPosition(saved_x, saved_y); }
    }
    fullscreen = on;
}

int fb_is_fullscreen(void) { return fullscreen; }
void fb_note_fullscreen(int on) { fullscreen = on != 0; }     /* the browser changed it (Esc, its own button) */

void fb_toast(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(toast, sizeof toast, fmt, ap);
    va_end(ap);
    toast_until = GetTime() + 1.5;
}

/* Where the image goes inside a window of aw x ah pixels. A "4:3" mode stretches the picture to the shape of the
 * monitors of the time (VGA's 320x200 and the 640x400 text screen both filled a 4:3 screen, so their pixels were
 * taller than wide); a "square" mode keeps 1:1 pixels. "Integer" modes use only whole multiples of the picture. */
FbRect fb_place(int w, int h, float aw, float ah, int mode)
{
    float bw = (float)w, bh = (mode == GM_SCALE_INT43 || mode == GM_SCALE_FIT43) ? bw * 0.75f : (float)h;
    float s = aw / bw < ah / bh ? aw / bw : ah / bh;
    if (mode == GM_SCALE_INT43 || mode == GM_SCALE_INTSQ) {
        float k = (float)(int)s;
        if (k >= 1.0f) s = k;               /* a window smaller than the picture falls back to fitting */
    }
    FbRect r = { 0, 0, bw * s, bh * s };
    r.x = (float)(int)((aw - r.w) / 2);
    r.y = (float)(int)((ah - r.h) / 2);
    return r;
}

void fb_draw_rgba(const uint32_t *px, int w, int h, FbRect *out_rect)
{
    if (w != tex_w || h != tex_h) make_texture(w, h);
    UpdateTexture(tex, px);
    BeginDrawing();
    ClearBackground(BLACK);
    float sw = (float)GetRenderWidth(), sh = (float)GetRenderHeight();
    FbRect r = fb_place(w, h, sw, sh, gm_settings.scale_mode);
    DrawTexturePro(tex, (Rectangle){0, 0, (float)w, (float)h}, (Rectangle){r.x, r.y, r.w, r.h},
                   (Vector2){0, 0}, 0, WHITE);
    if (toast_until > GetTime()) {          /* short status message (full screen, volume, ...) over the picture */
        int size = (int)(sh / 28), pad = size / 2;
        int tw = MeasureText(toast, size);
        DrawRectangle(pad, pad, tw + 2 * pad, size + pad, (Color){0, 0, 0, 170});
        DrawText(toast, pad * 2, pad + pad / 2, size, WHITE);
    }
    EndDrawing();
    fb_last_rect = r;
    if (out_rect) *out_rect = r;
}

void fb_present(void)
{
    convert();
    fb_draw_rgba(rgba, FB_W, FB_H, 0);
}
