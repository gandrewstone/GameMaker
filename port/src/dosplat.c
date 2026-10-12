/* Implementation of the DOS platform layer on top of raylib. See dosplat.h.
 *
 * Everything is cooperative and single-threaded: the engine's "interrupt handlers" (INT 08h timer,
 * INT 09h keyboard) are invoked from gm_pump(), which the engine's waiting loops reach through
 * bioskey(), delay(), reads of the VGA status port and an explicit call in the main game loop.
 */
#include "dosplat.h"
#include "fb.h"
#include "audio.h"
#include "settings.h"
#include "osclock.h"
#include "font8x8.h"
#include <raylib.h>

/* GM_HEADLESS=1 (test hook): no window, no keyboard/mouse/gamepad, no audio device. Replays and scripted-input runs
 * (GM_TYPE, GM_SHOT, FDUMP, GM_WAV) work as usual; used by CI machines and by tests on systems without a display. */
static int headless;
#define IsKeyDown(k) (!headless && (IsKeyDown)(k))
#define IsMouseButtonDown(b) (!headless && (IsMouseButtonDown)(b))
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t gm_dosmem[GM_DOSMEM_SIZE];
volatile uint32_t gm_heartbeat;           /* bumped on every gm_pump(); used by the hang watchdog in gmplay_main.cpp */
union REGS gm_pseudo;

#define VRAM (gm_dosmem + 0xA0000)
#define TEXT (gm_dosmem + 0xB8000)

/* ---------------------------------------------------------------------------------------------
 * Far heap: first-fit allocator over conventional memory (segments 0x1000..0x9FFF).
 * ------------------------------------------------------------------------------------------- */
#define ARENA_BASE 0x10000u
#define ARENA_END  0xA0000u
#define MAX_BLOCKS 1024

typedef struct { uint32_t off, size; int used; } Block;
static Block blocks[MAX_BLOCKS];
static int nblocks;

static void heap_init(void)
{
    blocks[0] = (Block){ ARENA_BASE, ARENA_END - ARENA_BASE, 0 };
    nblocks = 1;
}

void *gm_farmalloc(uint32_t bytes)
{
    if (!nblocks) heap_init();
    uint32_t need = (bytes + 15u) & ~15u;
    if (need == 0) need = 16;
    for (int i = 0; i < nblocks; i++) {
        if (blocks[i].used || blocks[i].size < need) continue;
        if (blocks[i].size > need && nblocks < MAX_BLOCKS) {   /* split */
            memmove(&blocks[i + 2], &blocks[i + 1], (nblocks - i - 1) * sizeof(Block));
            blocks[i + 1] = (Block){ blocks[i].off + need, blocks[i].size - need, 0 };
            blocks[i].size = need;
            nblocks++;
        }
        blocks[i].used = 1;
        return gm_dosmem + blocks[i].off;
    }
    return NULL;
}

void gm_farfree(void *p)
{
    if (!p || !nblocks) return;
    uint32_t off = (uint32_t)((uint8_t *)p - gm_dosmem);
    for (int i = 0; i < nblocks; i++) {
        if (blocks[i].off != off || !blocks[i].used) continue;
        blocks[i].used = 0;
        if (i + 1 < nblocks && !blocks[i + 1].used) {          /* merge with next */
            blocks[i].size += blocks[i + 1].size;
            memmove(&blocks[i + 1], &blocks[i + 2], (nblocks - i - 2) * sizeof(Block));
            nblocks--;
        }
        if (i > 0 && !blocks[i - 1].used) {                    /* merge with previous */
            blocks[i - 1].size += blocks[i].size;
            memmove(&blocks[i], &blocks[i + 1], (nblocks - i - 1) * sizeof(Block));
            nblocks--;
        }
        return;
    }
}

uint32_t gm_farcoreleft(void)
{
    uint32_t n = 0;
    if (!nblocks) heap_init();
    for (int i = 0; i < nblocks; i++) if (!blocks[i].used) n += blocks[i].size;
    return n;
}

uint16_t gm_fp_seg(const void *p) { return (uint16_t)(((const uint8_t *)p - gm_dosmem) >> 4); }
uint16_t gm_fp_off(const void *p) { return (uint16_t)(((const uint8_t *)p - gm_dosmem) & 15); }

/* ---------------------------------------------------------------------------------------------
 * VGA state: DAC palette (6-bit), video mode, mouse cursor
 * ------------------------------------------------------------------------------------------- */
static uint8_t dac[256][3];
static int dac_wr_idx, dac_wr_phase, dac_rd_idx, dac_rd_phase;
static int video_mode = 3;                         /* 3 = 80x25 text, 0x13 = 320x200x256 */

/* Attribute-controller palette of the text modes: attribute 0..15 -> DAC index */
static const uint8_t atc_text[16] = { 0, 1, 2, 3, 4, 5, 0x14, 7, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F };

static int dac_ready;

static void default_palette(void)
{
    dac_ready = 1;
    memset(dac, 0, sizeof dac);
    for (int i = 0; i < 64; i++) {                 /* EGA colours: bits (R,G,B) with 0x2A / 0x15 weights */
        dac[i][0] = (uint8_t)(((i >> 2) & 1) * 0x2A + ((i >> 5) & 1) * 0x15);
        dac[i][1] = (uint8_t)(((i >> 1) & 1) * 0x2A + ((i >> 4) & 1) * 0x15);
        dac[i][2] = (uint8_t)((i & 1) * 0x2A + ((i >> 3) & 1) * 0x15);
    }
    if (video_mode == 0x13) {                      /* mode 13h: 16 EGA colours, 16 greys, rest black */
        uint8_t ega[16][3];
        for (int i = 0; i < 16; i++) memcpy(ega[i], dac[atc_text[i]], 3);
        static const uint8_t grey[16] = { 0, 5, 8, 11, 14, 17, 20, 24, 28, 32, 36, 40, 45, 50, 56, 63 };
        memset(dac, 0, sizeof dac);
        for (int i = 0; i < 16; i++) { memcpy(dac[i], ega[i], 3); dac[16 + i][0] = dac[16 + i][1] = dac[16 + i][2] = grey[i]; }
    }
}

/* A PC starts in text mode with the BIOS palette; the editors draw their first screen without setting a mode. */
static void dac_init(void) { if (!dac_ready) default_palette(); }

static int mouse_shown;
static FbRect game_rect = { 0, 0, 960, 720 };      /* where the VGA image landed in the window */

/* ---------------------------------------------------------------------------------------------
 * Window / presentation
 * ------------------------------------------------------------------------------------------- */
static int window_open;
static double last_present;

static void ensure_window(void)
{
    if (window_open) return;
    headless = getenv("GM_HEADLESS") != NULL;
    if (headless) { gm_audio_set_headless(1); SetTraceLogLevel(LOG_WARNING); }
    else {
        SetTraceLogLevel(LOG_WARNING);
        fb_open("GameMaker", 3);
    }
    window_open = 1;
    gm_audio_set_volume(gm_settings.volume);
    gm_audio_set_mute(gm_settings.mute);
    last_present = gm_os_time();
}

#define RGBA(r6, g6, b6) ((uint32_t)(((r6) << 2) | ((r6) >> 4)) | ((uint32_t)(((g6) << 2) | ((g6) >> 4)) << 8) | \
                          ((uint32_t)(((b6) << 2) | ((b6) >> 4)) << 16) | 0xFF000000u)

static uint32_t text_rgba[640 * 400];

static void render_text(int mouse_cell_x, int mouse_cell_y)
{
    for (int cy = 0; cy < 25; cy++)
        for (int cx = 0; cx < 80; cx++) {
            uint8_t ch = TEXT[(cy * 80 + cx) * 2], attr = TEXT[(cy * 80 + cx) * 2 + 1];
            if (cx == mouse_cell_x && cy == mouse_cell_y) attr = (uint8_t)((attr >> 4) | (attr << 4));   /* software cursor */
            const uint8_t *fg = dac[atc_text[attr & 15]], *bg = dac[atc_text[(attr >> 4) & 7]];
            uint32_t fgc = RGBA(fg[0], fg[1], fg[2]), bgc = RGBA(bg[0], bg[1], bg[2]);
            for (int row = 0; row < 8; row++) {
                uint8_t bits = font8x8[ch * 8 + row];
                for (int col = 0; col < 8; col++) {
                    uint32_t c = (bits & (0x80 >> col)) ? fgc : bgc;
                    int x = cx * 8 + col, y = cy * 16 + row * 2;
                    text_rgba[y * 640 + x] = c;               /* 8x8 glyph stretched to 8x16 */
                    text_rgba[(y + 1) * 640 + x] = c;
                }
            }
        }
}

static void present(void)
{
    int32_t mx, my, mb;
    dac_init();
    gm_mouse_get(&mx, &my, &mb);
    if (video_mode == 0x13) {
        memcpy(fb_pix, VRAM, FB_W * FB_H);
        for (int i = 0; i < 256; i++) fb_set_vga_pal(i, dac[i][0], dac[i][1], dac[i][2]);
        if (mouse_shown) {                                 /* small arrow cursor drawn over the image */
            int px = (int)(mx / 2), py = (int)my;
            for (int r = 0; r < 10; r++)
                for (int c = 0; c <= r && c < 7; c++) {
                    int x = px + c, y = py + r;
                    if (x < FB_W && y < FB_H) fb_pix[y * FB_W + x] = (c == 0 || c == r || r == 9) ? 0 : 15;
                }
        }
        if (!headless) { fb_present(); game_rect = fb_last_rect; }       /* the rectangle is for mouse mapping */
    } else {
        render_text(mouse_shown ? (int)(mx / 8) : -1, mouse_shown ? (int)(my / 8) : -1);
        if (!headless) fb_draw_rgba(text_rgba, 640, 400, &game_rect);
    }
    last_present = gm_os_time();

    /* Test hook: GM_SHOT=<file.png> saves the exact emulated screen after GM_SHOT_AFTER seconds (default 3) and exits. */
    static const char *shot;
    static double shot_after = -1;
    if (shot_after < 0) {
        shot = getenv("GM_SHOT");
        shot_after = getenv("GM_SHOT_AFTER") ? atof(getenv("GM_SHOT_AFTER")) : 3.0;
    }
    /* If the name contains %d, a numbered screenshot is also saved every GM_SHOT_EVERY seconds (default 4). */
    static double next_seq = -1;
    if (shot && strchr(shot, '%')) {
        double every = getenv("GM_SHOT_EVERY") ? atof(getenv("GM_SHOT_EVERY")) : 4.0;
        if (next_seq < 0) next_seq = every;
        if (gm_os_time() >= next_seq) {
            static int seq;
            char name[512];
            snprintf(name, sizeof name, shot, seq++);
            if (video_mode == 0x13) fb_save_png(name);
            else { Image img = { text_rgba, 640, 400, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 }; ExportImage(img, name); }
            next_seq += every;
        }
        if (gm_os_time() >= shot_after) { if (!headless) fb_close(); exit(0); }
    } else if (shot && gm_os_time() >= shot_after) {
        if (video_mode == 0x13) fb_save_png(shot);
        else { Image img = { text_rgba, 640, 400, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 }; ExportImage(img, shot); }
        if (!headless) fb_close();
        exit(0);
    }
}

/* ---------------------------------------------------------------------------------------------
 * Interrupt vectors, PIT timer, PIC, keyboard
 * ------------------------------------------------------------------------------------------- */
static void default_isr(void) {}
static gm_isr vectors[256];
static int in_isr;

gm_isr gm_getvect(int16_t n) { return vectors[n & 255] ? vectors[n & 255] : default_isr; }
/* A program saves what getvect() returned and puts it back when it is done. For a vector nobody has set that is the stub
 * above, and it must go back as "no handler": a keyboard handler (INT 9) that does nothing would stop key presses from
 * reaching the BIOS key buffer (see key_event) for the rest of the run, e.g. at "Hit any key" after a game. */
void gm_setvect(int16_t n, gm_isr h) { vectors[n & 255] = h == default_isr ? NULL : h; }
void gm_int_enable(void) {}
void gm_int_disable(void) {}

static uint32_t pit_divisor = 0x10000;      /* 0 == 65536: 18.2 Hz */
static int pit_lo_hi;
static double next_tick;

static uint8_t kbd_data;                    /* port 0x60 */
static uint8_t pic_mask;                    /* port 0x21: bit 1 masks the keyboard IRQ */
static uint8_t kbd_held[64];                /* scan codes raised while IRQ 1 was masked */
static int kbd_held_n;

typedef struct { int rkey; uint8_t scan; char ascii, shifted; } KeyMap;
static const KeyMap keymap[] = {
    {KEY_ESCAPE, 0x01, 27, 27}, {KEY_ONE, 0x02, '1', '!'}, {KEY_TWO, 0x03, '2', '@'},
    {KEY_THREE, 0x04, '3', '#'}, {KEY_FOUR, 0x05, '4', '$'}, {KEY_FIVE, 0x06, '5', '%'},
    {KEY_SIX, 0x07, '6', '^'}, {KEY_SEVEN, 0x08, '7', '&'}, {KEY_EIGHT, 0x09, '8', '*'},
    {KEY_NINE, 0x0A, '9', '('}, {KEY_ZERO, 0x0B, '0', ')'}, {KEY_MINUS, 0x0C, '-', '_'},
    {KEY_EQUAL, 0x0D, '=', '+'}, {KEY_BACKSPACE, 0x0E, 8, 8}, {KEY_TAB, 0x0F, 9, 9},
    {KEY_Q, 0x10, 'q', 'Q'}, {KEY_W, 0x11, 'w', 'W'}, {KEY_E, 0x12, 'e', 'E'}, {KEY_R, 0x13, 'r', 'R'},
    {KEY_T, 0x14, 't', 'T'}, {KEY_Y, 0x15, 'y', 'Y'}, {KEY_U, 0x16, 'u', 'U'}, {KEY_I, 0x17, 'i', 'I'},
    {KEY_O, 0x18, 'o', 'O'}, {KEY_P, 0x19, 'p', 'P'}, {KEY_LEFT_BRACKET, 0x1A, '[', '{'},
    {KEY_RIGHT_BRACKET, 0x1B, ']', '}'}, {KEY_ENTER, 0x1C, 13, 13}, {KEY_LEFT_CONTROL, 0x1D, 0, 0},
    {KEY_A, 0x1E, 'a', 'A'}, {KEY_S, 0x1F, 's', 'S'}, {KEY_D, 0x20, 'd', 'D'}, {KEY_F, 0x21, 'f', 'F'},
    {KEY_G, 0x22, 'g', 'G'}, {KEY_H, 0x23, 'h', 'H'}, {KEY_J, 0x24, 'j', 'J'}, {KEY_K, 0x25, 'k', 'K'},
    {KEY_L, 0x26, 'l', 'L'}, {KEY_SEMICOLON, 0x27, ';', ':'}, {KEY_APOSTROPHE, 0x28, '\'', '"'},
    {KEY_GRAVE, 0x29, '`', '~'}, {KEY_LEFT_SHIFT, 0x2A, 0, 0}, {KEY_BACKSLASH, 0x2B, '\\', '|'},
    {KEY_Z, 0x2C, 'z', 'Z'}, {KEY_X, 0x2D, 'x', 'X'}, {KEY_C, 0x2E, 'c', 'C'}, {KEY_V, 0x2F, 'v', 'V'},
    {KEY_B, 0x30, 'b', 'B'}, {KEY_N, 0x31, 'n', 'N'}, {KEY_M, 0x32, 'm', 'M'}, {KEY_COMMA, 0x33, ',', '<'},
    {KEY_PERIOD, 0x34, '.', '>'}, {KEY_SLASH, 0x35, '/', '?'}, {KEY_RIGHT_SHIFT, 0x36, 0, 0},
    {KEY_LEFT_ALT, 0x38, 0, 0}, {KEY_SPACE, 0x39, ' ', ' '},
    {KEY_F1, 0x3B, 0, 0}, {KEY_F2, 0x3C, 0, 0}, {KEY_F3, 0x3D, 0, 0}, {KEY_F4, 0x3E, 0, 0},
    {KEY_F5, 0x3F, 0, 0}, {KEY_F6, 0x40, 0, 0}, {KEY_F7, 0x41, 0, 0}, {KEY_F8, 0x42, 0, 0},
    {KEY_F9, 0x43, 0, 0}, {KEY_F10, 0x44, 0, 0},
    /* cursor block: the engine treats these like the numeric keypad (no E0 prefix) */
    {KEY_UP, 0x48, 0, 0}, {KEY_LEFT, 0x4B, 0, 0}, {KEY_RIGHT, 0x4D, 0, 0}, {KEY_DOWN, 0x50, 0, 0},
    {KEY_HOME, 0x47, 0, 0}, {KEY_PAGE_UP, 0x49, 0, 0}, {KEY_END, 0x4F, 0, 0}, {KEY_PAGE_DOWN, 0x51, 0, 0},
    {KEY_INSERT, 0x52, 0, 0}, {KEY_DELETE, 0x53, 0, 0},
};
#define NKEYS ((int)(sizeof keymap / sizeof keymap[0]))
static uint8_t key_down[NKEYS];
static uint8_t synth_down[NKEYS];           /* keys held by the GM_TYPE test hook, merged with the real keyboard */

/* BIOS keyboard buffer, filled only while no program handler owns INT 9 */
static uint16_t bios_buf[32];
static int bios_n;

static int synth_shift;                        /* GM_TYPE is typing a shifted character */
static int shift_held(void) { return synth_shift || IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT); }

static void raise_kbd(uint8_t code)
{
    kbd_data = code;
    in_isr++;
    vectors[9]();
    in_isr--;
}

static void key_event(int idx, int down)
{
    const KeyMap *k = &keymap[idx];
    uint8_t code = k->scan | (down ? 0 : 0x80);
    if (vectors[9]) {
        if (pic_mask & 2) { if (kbd_held_n < 64) kbd_held[kbd_held_n++] = code; }   /* IRQ 1 masked: latch it */
        else raise_kbd(code);
    } else if (down) {
        char a = shift_held() ? k->shifted : k->ascii;
        if (bios_n < 32) bios_buf[bios_n++] = (uint16_t)((k->scan << 8) | (uint8_t)a);
    }
}

/* ---------------------------------------------------------------------------------------------
 * Host controls: F11 / Alt+Enter full screen, F12 picture scaling, Alt+Up / Alt+Down volume, Alt+M mute, Alt+G capture the mouse.
 * While Alt is held the chord keys are hidden from the game. (F11/F12 are not game keys: the engine knows F1-F10.)
 * Edges are detected here rather than with raylib's IsKeyPressed(), which stays true for every call within a frame.
 * ------------------------------------------------------------------------------------------- */
static int host_alt;

static int edge(int *prev, int now) { int e = now && !*prev; *prev = now; return e; }

static void act_fullscreen(void)
{
    fb_set_fullscreen(!fb_is_fullscreen());
    gm_settings.fullscreen = fb_is_fullscreen();
    fb_toast(gm_settings.fullscreen ? "Full screen" : "Window");
}

static void act_scale(void)
{
    static const char *names[GM_SCALE_COUNT] = { "Whole-number scale, 4:3", "Fit window, 4:3", "Whole-number scale, square pixels", "Fit window, square pixels" };
    gm_settings.scale_mode = (gm_settings.scale_mode + 1) % GM_SCALE_COUNT;
    fb_toast("%s", names[gm_settings.scale_mode]);
}

static void act_volume(int delta)
{
    gm_settings.volume += delta;
    if (gm_settings.volume > 100) gm_settings.volume = 100;
    if (gm_settings.volume < 0) gm_settings.volume = 0;
    gm_settings.mute = 0;
    gm_audio_set_volume(gm_settings.volume);
    gm_audio_set_mute(0);
    fb_toast("Volume %d%%", gm_settings.volume);
}

/* Mouse capture (Alt+G): the host cursor is hidden and the program's own cursor is the only one. The movement is added up
 * here (cap_x/cap_y, window pixels), so the cursor stops at the picture's edge and leaves it as soon as the mouse turns
 * back. On the web page the browser's pointer lock does the capturing (gmedit-web.js) and the page reports the position. */
static int captured;
static float cap_x, cap_y, cap_rx, cap_ry;
static void act_capture(void)
{
#ifndef __EMSCRIPTEN__
    if (!window_open || headless) return;
    if (!captured) {
        Vector2 p = GetMousePosition();
        cap_x = p.x; cap_y = p.y;
        DisableCursor();
        p = GetMousePosition();
        cap_rx = p.x; cap_ry = p.y;
        captured = 1;
        fb_toast("Mouse captured (Alt+G releases)");
    } else {
        captured = 0;
        EnableCursor();
        SetMousePosition((int)cap_x, (int)cap_y);
        fb_toast("Mouse released");
    }
#endif
}

static void act_mute(void)
{
    gm_settings.mute = !gm_settings.mute;
    gm_audio_set_mute(gm_settings.mute);
    if (gm_settings.mute) fb_toast("Sound off"); else fb_toast("Sound on (volume %d%%)", gm_settings.volume);
}

/* Test hook: GM_HOST_KEYS="f11@3,volup@4,mute@5" runs those host actions (f11, f12, volup, voldn, mute) at the
 * given seconds, because the real key chords cannot be scripted. */
static int host_test_actions(void)
{
    static const char *spec;
    static int init;
    static double done_at[32];
    if (!init) { init = 1; spec = getenv("GM_HOST_KEYS"); }
    if (!spec) return 0;
    int changed = 0, i = 0;
    for (const char *p = spec; *p; i++) {
        char name[16] = { 0 };
        double at = 0;
        int n = 0;
        if (sscanf(p, "%15[a-z0-9]@%lf%n", name, &at, &n) < 2) break;
        p += n;
        if (*p == ',') p++;
        if (i >= 32 || done_at[i] || gm_os_time() < at) continue;
        done_at[i] = 1;
        if (!strcmp(name, "f11")) act_fullscreen();
        else if (!strcmp(name, "f12")) act_scale();
        else if (!strcmp(name, "volup")) act_volume(10);
        else if (!strcmp(name, "voldn")) act_volume(-10);
        else if (!strcmp(name, "mute")) act_mute();
        changed = 1;
    }
    return changed;
}

static void host_controls(void)
{
    static int p_f11, p_f12, p_enter, p_up, p_down, p_m, p_g;
    host_alt = (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL);
    int changed = host_test_actions();

    if (edge(&p_f11, IsKeyDown(KEY_F11)) | edge(&p_enter, host_alt && IsKeyDown(KEY_ENTER))) { act_fullscreen(); changed = 1; }
    if (edge(&p_f12, IsKeyDown(KEY_F12))) { act_scale(); changed = 1; }
    int up = edge(&p_up, host_alt && IsKeyDown(KEY_UP)), down = edge(&p_down, host_alt && IsKeyDown(KEY_DOWN));
    if (up || down) { act_volume(up ? 10 : -10); changed = 1; }
    if (edge(&p_m, host_alt && IsKeyDown(KEY_M))) { act_mute(); changed = 1; }
    if (edge(&p_g, host_alt && IsKeyDown(KEY_G))) act_capture();
    if (changed) gm_settings_save();
}

static int chord_key(int rkey) { return rkey == KEY_ENTER || rkey == KEY_UP || rkey == KEY_DOWN || rkey == KEY_M || rkey == KEY_G; }

/* ---------------------------------------------------------------------------------------------
 * Gamepad. In a game it is a joystick (ReadJoyStick, see input_asm.cpp); in the menus, which read the BIOS keyboard,
 * the D-pad or left stick press the arrow keys, A is Enter and B is Esc. Start is Esc everywhere.
 * ------------------------------------------------------------------------------------------- */
static int pad_present(void) { return !headless && gm_settings.gamepad && IsGamepadAvailable(0); }

static float pad_axis(int axis)
{
    float v = GetGamepadAxisMovement(0, axis);
    return (v > -0.35f && v < 0.35f) ? 0.0f : v;
}

typedef struct { int up, down, left, right, a, b, start; } PadState;

static PadState pad_state(void)
{
    PadState s = { 0 };
    if (!pad_present()) return s;
    float ax = pad_axis(GAMEPAD_AXIS_LEFT_X), ay = pad_axis(GAMEPAD_AXIS_LEFT_Y);
    s.up = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP) || ay < 0;
    s.down = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || ay > 0;
    s.left = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || ax < 0;
    s.right = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || ax > 0;
    s.a = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    s.b = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    s.start = IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    return s;
}

/* The joystick the engine reads: positions 0..200 with the centre at 100 (the game's own calibration screen and
 * the port's defaults agree on that range), and two buttons. */
void gm_joystick_read(int32_t *x, int32_t *y, int32_t *buttons)
{
    *x = *y = 100;
    *buttons = 0;
    if (!pad_present()) return;
    PadState s = pad_state();
    if (s.left) *x = 0; else if (s.right) *x = 200;
    if (s.up) *y = 0; else if (s.down) *y = 200;
    *buttons = (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) ? 1 : 0) |
               (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) ? 2 : 0);
}

static int pad_wants_key(int rkey, const PadState *p)
{
    if (rkey == KEY_ESCAPE) return p->start || (vectors[9] == NULL && p->b);
    if (vectors[9] != NULL) return 0;                         /* in a game the pad is a joystick, not arrow keys */
    switch (rkey) {
    case KEY_UP: return p->up;
    case KEY_DOWN: return p->down;
    case KEY_LEFT: return p->left;
    case KEY_RIGHT: return p->right;
    case KEY_ENTER: return p->a;
    default: return 0;
    }
}

static void poll_keys(void)
{
    if (!headless) host_controls();
    PadState pad = pad_state();
    for (int i = 0; i < NKEYS; i++) {
        int rk = keymap[i].rkey;
        int real = IsKeyDown(rk) && !(host_alt && chord_key(rk));
        int d = real || synth_down[i] || pad_wants_key(rk, &pad);
        if (d != key_down[i]) { key_down[i] = (uint8_t)d; key_event(i, d); }
    }
}

/* Test hook: GM_TYPE="text" types into the engine after GM_TYPE_AT seconds (default 2), one key every
 * 80 ms. In the text: \n = Enter, \b = Backspace, \e = Escape, \l \r \u \d = arrow keys, \\ = backslash,
 * and "\hNN" holds the key typed next for NN * 80 ms (so a game can be steered). Used by automated UI tests. */
static int find_key_for_char(char c, int *shift)
{
    for (int i = 0; i < NKEYS; i++) {
        if (keymap[i].ascii == c && c) { *shift = 0; return i; }
        if (keymap[i].shifted == c && c && keymap[i].shifted != keymap[i].ascii) { *shift = 1; return i; }
    }
    return -1;
}

/* Maps a GM_TYPE escape letter to a raylib key, or -1. */
static int escape_key(char e)
{
    switch (e) {
    case 'n': return KEY_ENTER;
    case 'b': return KEY_BACKSPACE;
    case 'e': return KEY_ESCAPE;
    case 'l': return KEY_LEFT;
    case 'r': return KEY_RIGHT;
    case 'u': return KEY_UP;
    case 'd': return KEY_DOWN;
    case '\\': return KEY_BACKSLASH;
    default: return -1;
    }
}

static int key_index(int rkey)
{
    for (int i = 0; i < NKEYS; i++) if (keymap[i].rkey == rkey) return i;
    return -1;
}

static void type_keys(void)
{
    static const char *buf;
    static int pos = -1, held = -1, hold_left;
    static double next;
    if (pos < 0) {
        buf = getenv("GM_TYPE");
        pos = 0;
        next = gm_os_time() + (getenv("GM_TYPE_AT") ? atof(getenv("GM_TYPE_AT")) : 2.0);
        if (!buf) return;
    }
    if (!buf || gm_os_time() < next) return;
    next = gm_os_time() + 0.08;
    if (held >= 0) {                                   /* release the key typed on the previous step */
        if (hold_left-- > 0) return;
        synth_down[held] = 0; synth_shift = 0; held = -1; return;
    }
    if (!buf[pos]) return;

    int hold = 0, shift = 0, idx = -1;
    char c = buf[pos++];
    if (c == '\\' && buf[pos] == 'w') {                /* \wNN: pause NN * 80 ms */
        next = gm_os_time() + ((buf[pos + 1] - '0') * 10 + (buf[pos + 2] - '0')) * 0.08;
        pos += 3;
        return;
    }
    if (c == '\\' && buf[pos] == 'h') {                /* \hNN<key>: hold the next key NN * 80 ms */
        hold = (buf[pos + 1] - '0') * 10 + (buf[pos + 2] - '0');
        pos += 3;
        c = buf[pos++];
    }
    if (c == '\\') idx = key_index(escape_key(buf[pos++]));  /* an escape such as \n or \r */
    else           idx = find_key_for_char(c, &shift);        /* a plain character */
    if (idx < 0) return;
    held = idx; hold_left = hold; synth_shift = shift;
    synth_down[idx] = 1;
}

static void unmask_kbd(void)
{
    while (kbd_held_n && !(pic_mask & 2) && vectors[9]) {
        uint8_t code = kbd_held[0];
        memmove(kbd_held, kbd_held + 1, --kbd_held_n);
        raise_kbd(code);
    }
}

static void fire_timer(void)
{
    double now = gm_os_time();
    double period = (pit_divisor ? pit_divisor : 0x10000) / 1193182.0;
    if (next_tick == 0) next_tick = now + period;
    static unsigned fired, dropped;
    static double next_report;
    /* Catch up on ticks missed while the engine was busy (the music's tempo and the game's clock count them); only a
     * long stall (about 1.4 s) is given up on so that this cannot spiral. A small budget here made music slow and
     * uneven in the browser, where the main loop is often held up for longer than a few ticks. */
    int budget = 200;
    while (now >= next_tick && budget--) {
        next_tick += period;
        fired++;
        if (vectors[8] && !(pic_mask & 1)) { in_isr++; vectors[8](); in_isr--; }
        if (vectors[0x1C]) { in_isr++; vectors[0x1C](); in_isr--; }      /* the BIOS tick handler calls INT 1Ch */
    }
    if (now >= next_tick) { dropped += (unsigned)((now - next_tick) / period) + 1; next_tick = now + period; }
    if (getenv("GM_TIMERLOG") && now >= next_report) {       /* test aid: how many timer ticks the engine got */
        unsigned audio_fed, audio_starved;
        gm_audio_stats(&audio_fed, &audio_starved);
        if (next_report) fprintf(stderr, "timer: %u ticks fired, %u dropped; audio: %u chunks fed, %u starved; %u pumps\n", fired, dropped, audio_fed, audio_starved, (unsigned)gm_heartbeat);
        next_report = now + 5.0;
    }
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
/* Commands from the web page, which has buttons for what a browser keeps from the keyboard (F11, F12, Alt chords).
 * They are queued here and applied from gm_pump(), so that nothing in the engine runs while the page calls in.
 *   1 volume 0..100   2 toggle mute   3 next picture mode   4 picture mode = arg   5 the browser's full-screen state = arg */
enum { WEB_Q = 16 };
static volatile int web_q_cmd[WEB_Q], web_q_arg[WEB_Q];
static volatile unsigned web_q_head, web_q_tail;

EMSCRIPTEN_KEEPALIVE void gm_web_command(int cmd, int arg)
{
    unsigned h = web_q_head;
    if (h - web_q_tail >= WEB_Q) return;
    web_q_cmd[h % WEB_Q] = cmd;
    web_q_arg[h % WEB_Q] = arg;
    web_q_head = h + 1;
}

static void web_apply(void)
{
    while (web_q_tail != web_q_head) {
        int c = web_q_cmd[web_q_tail % WEB_Q], a = web_q_arg[web_q_tail % WEB_Q];
        web_q_tail++;
        switch (c) {
        case 1:
            gm_settings.volume = a < 0 ? 0 : (a > 100 ? 100 : a);
            gm_settings.mute = 0;
            gm_audio_set_volume(gm_settings.volume);
            gm_audio_set_mute(0);
            fb_toast("Volume %d%%", gm_settings.volume);
            break;
        case 2: act_mute(); break;
        case 3: act_scale(); break;
        case 4:
            if (a >= 0 && a < GM_SCALE_COUNT) gm_settings.scale_mode = a;
            break;
        case 5:
            fb_note_fullscreen(a);
            gm_settings.fullscreen = a != 0;
            break;
        default: continue;
        }
        gm_settings_save();
    }
}
#endif

static void kev_poll(void);                 /* GM_KEYSCRIPT events that are not keys (mouse, dump, quit): defined with the BIOS keyboard below */
static void (*pump_hook)(void);
void gm_set_pump_hook(void (*hook)(void)) { pump_hook = hook; }

void gm_pump(void)
{
    gm_heartbeat++;
    if (in_isr) return;
    ensure_window();
#ifdef __EMSCRIPTEN__
    web_apply();
#endif
    if (!headless && WindowShouldClose()) { fb_close(); exit(0); }
    if (gm_os_time() - last_present >= 1.0 / 60.0) present();
    poll_keys();
    type_keys();
    kev_poll();
    gm_audio_pump();
    fire_timer();
    if (pump_hook) { in_isr++; pump_hook(); in_isr--; }
#ifdef __EMSCRIPTEN__
    /* Every wait in the engine ends up here, which makes this the one place to give the browser a turn (to draw the
     * frame, deliver key and gamepad events and refill the audio buffer). Often enough for that, rarely enough that
     * Asyncify's unwinding does not dominate. */
    static double last_yield;
    double t = gm_os_time();
    if (t - last_yield >= 0.004) { last_yield = t; gm_os_yield(); }
#endif
}

/* ---------------------------------------------------------------------------------------------
 * Mouse (virtual 640x200 driver coordinates, see input_asm.cpp)
 * ------------------------------------------------------------------------------------------- */
void gm_mouse_show(int16_t on) { mouse_shown = on; }

#ifdef __EMSCRIPTEN__
/* The pointer in canvas pixels (the canvas buffer is what raylib draws into), 0 until the pointer has been over the page */
EM_JS(int, gm_js_pointer_x, (void), {
  if (!Module.gmPtr) {
    var st = Module.gmPtr = { x: 0, y: 0, seen: false };
    window.addEventListener('pointermove', function (e) {
      if (!document.pointerLockElement) { st.x = e.clientX; st.y = e.clientY; st.seen = true; }
    }, true);
    /* captured: the pointer itself does not move, the movement is added up. mousemove rather than pointermove, which Firefox
     * does not send while the pointer is locked. */
    window.addEventListener('mousemove', function (e) {
      if (document.pointerLockElement) { st.x += e.movementX; st.y += e.movementY; st.seen = true; }
    }, true);
  }
  var st = Module.gmPtr, c = Module.canvas || document.getElementById('canvas');
  if (!st.seen || !c) return -1;
  var r = c.getBoundingClientRect();
  Module.gmPtrY = (st.y - r.top) * c.height / r.height;
  return Math.round((st.x - r.left) * c.width / r.width * 16);
});
EM_JS(int, gm_js_pointer_y, (void), { return Math.round((Module.gmPtrY || 0) * 16); });
EM_JS(int, gm_js_locked, (void), { return document.pointerLockElement ? 1 : 0; });
static int gm_web_pointer(float *x, float *y)
{
    int ix = gm_js_pointer_x();
    if (ix < 0) return 0;
    *x = ix / 16.0f; *y = gm_js_pointer_y() / 16.0f;
    return 1;
}
#endif

static int32_t headless_mx, headless_my, headless_buttons;     /* headless: the pointer stays where the engine last put it, like a still mouse */

void gm_mouse_get(int32_t *vx, int32_t *vy, int32_t *buttons)
{
    if (headless) { *vx = headless_mx; *vy = headless_my; *buttons = headless_buttons; return; }
    if (!window_open) { *vx = *vy = *buttons = 0; return; }
    Vector2 p = GetMousePosition();
    float s = (float)GetRenderWidth() / (float)GetScreenWidth();   /* DPI scale: render px per logical px */
#ifdef __EMSCRIPTEN__
    {   /* raylib's browser pointer is scaled by the window size GLFW remembers from InitWindow (960x720), not by the canvas
         * that fills the page, so it moves slower than the mouse. The page knows where the pointer really is. */
        float wx, wy;
        if (gm_web_pointer(&wx, &wy)) { p.x = wx / s; p.y = wy / s; }
        int lock = gm_js_locked();                      /* the page captured the mouse (pointer lock) */
        if (lock && !captured) { captured = 1; cap_x = cap_rx = p.x; cap_y = cap_ry = p.y; }
        else if (!lock && captured) captured = 0;
    }
#endif
    if (captured) {                                    /* add up the movement; the host cursor is not shown */
        cap_x += p.x - cap_rx; cap_y += p.y - cap_ry;
        cap_rx = p.x; cap_ry = p.y;
        float lx = game_rect.x / s, hx = (game_rect.x + game_rect.w) / s, ly = game_rect.y / s, hy = (game_rect.y + game_rect.h) / s;
        cap_x = cap_x < lx ? lx : (cap_x > hx ? hx : cap_x);
        cap_y = cap_y < ly ? ly : (cap_y > hy ? hy : cap_y);
        p.x = cap_x; p.y = cap_y;
    }
    float nx = (p.x * s - game_rect.x) / game_rect.w, ny = (p.y * s - game_rect.y) / game_rect.h;
    nx = nx < 0 ? 0 : (nx > 1 ? 1 : nx);
    ny = ny < 0 ? 0 : (ny > 1 ? 1 : ny);
    *vx = (int32_t)(nx * 639.0f + 0.5f);
    *vy = (int32_t)(ny * 199.0f + 0.5f);
    *buttons = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 1 : 0) | (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? 2 : 0) |
               (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ? 4 : 0);
}

int gm_mouse_follows_host(void)
{
#ifdef __EMSCRIPTEN__
    return !headless && !captured;
#else
    return 0;
#endif
}

void gm_mouse_set(int32_t vx, int32_t vy)
{
    if (headless) { headless_mx = vx; headless_my = vy; return; }
    if (!window_open) return;
    float s = (float)GetRenderWidth() / (float)GetScreenWidth();
    float tx = (game_rect.x + vx / 639.0f * game_rect.w) / s, ty = (game_rect.y + vy / 199.0f * game_rect.h) / s;
    if (captured) { cap_x = tx; cap_y = ty; return; }  /* the host pointer is not shown, nothing to move */
    SetMousePosition((int)tx, (int)ty);
}

/* ---------------------------------------------------------------------------------------------
 * Ports
 * ------------------------------------------------------------------------------------------- */
uint8_t gm_inportb(uint16_t port)
{
    if (port >= 0x3C7 && port <= 0x3C9) dac_init();
    switch (port) {
    case 0x3DA: {                       /* VGA input status: bit3 vertical retrace, bit0 display disabled */
        static uint32_t n;
        static long last_frame = -1;
        gm_pump();
        n++;
        int retrace;
        if (headless) {
            retrace = (n & 15) == 15;                       /* test mode: retrace comes round every 16 reads, no waiting */
        } else {
            /* One retrace per 70 Hz frame: the bit is set at the first read after the frame boundary and clear for the rest
             * of the frame. (A fixed 1 ms window is caught by a loop that polls constantly, but a browser build only gets to
             * poll every few milliseconds, misses most windows, and every fade and wipe in the games runs several times
             * too slowly.) */
            long frame = (long)(gm_os_time() * 70.0);
            retrace = frame != last_frame;
            if (retrace) last_frame = frame;
        }
        return (uint8_t)((retrace ? 8 : 0) | (n & 1));
    }
    case 0x3C9:
        if (dac_rd_idx < 256) {
            uint8_t v = dac[dac_rd_idx][dac_rd_phase];
            if (++dac_rd_phase == 3) { dac_rd_phase = 0; dac_rd_idx++; }
            return v;
        }
        return 0;
    case 0x21: return pic_mask;
    case 0x60: return kbd_data;
    case 0x61: return 0;
    case 0x201: return 0xFF;            /* no joystick: all axes/buttons idle */
    default:   return 0;
    }
}

uint16_t gm_inport(uint16_t port) { return gm_inportb(port); }

void gm_outportb(uint16_t port, uint8_t v)
{
    if (port >= 0x3C7 && port <= 0x3C9) dac_init();
    switch (port) {
    case 0x3C7: dac_rd_idx = v; dac_rd_phase = 0; break;
    case 0x3C8: dac_wr_idx = v; dac_wr_phase = 0; break;
    case 0x3C9:
        dac[dac_wr_idx & 255][dac_wr_phase] = v & 63;
        if (++dac_wr_phase == 3) { dac_wr_phase = 0; dac_wr_idx++; }
        break;
    case 0x21: pic_mask = v; unmask_kbd(); break;
    case 0x43: pit_lo_hi = 0; break;
    case 0x40:                           /* PIT channel 0 reload value, low byte then high byte */
        if (!pit_lo_hi) { pit_divisor = (pit_divisor & 0xFF00u) | v; pit_lo_hi = 1; }
        else { pit_divisor = (pit_divisor & 0x00FFu) | ((uint32_t)v << 8); pit_lo_hi = 0; next_tick = 0; }
        break;
    default: break;                      /* EOI, speaker, CRTC, sequencer, ... are ignored */
    }
}

void gm_outport(uint16_t port, uint16_t v)
{
    gm_outportb(port, (uint8_t)v);
    gm_outportb((uint16_t)(port + 1), (uint8_t)(v >> 8));
}
/* ---------------------------------------------------------------------------------------------
 * GM_KEYSCRIPT=<file>: the keyboard of a test run (baseline/editors/KEYS.C does the same under DOS). One event per line,
 * with a delay in BIOS ticks (18.2 per second) counted from the previous event (a key counts when the program takes it):
 *   K <delay> <hex>   the key <scan code << 8 | ASCII> becomes available      S <delay> <hex>   shift-state byte
 *   D <delay> <name>  dump the screen: mode byte, 768 DAC bytes, 64000 (mode 13h) or 4000 (text) bytes
 *   Q <delay>         end the program
 * While a script is active the real keyboard and GM_TYPE are ignored.
 * ------------------------------------------------------------------------------------------- */
typedef struct { char kind; double delay; unsigned val; int dx, dy; char name[16]; } KeyEv;
static KeyEv *kev;
static int kev_n, kev_cur = -1;                        /* -1: not looked at yet, -2: no script */
static double kev_last;
static unsigned kev_shift;

static double bios_ticks(void) { return gm_os_time() * 18.2065; }

static void kev_load(void)
{
    const char *path = getenv("GM_KEYSCRIPT");
    kev_cur = -2;
    if (!path) return;
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "GM_KEYSCRIPT: cannot open %s\n", path); return; }
    char line[120];
    kev = (KeyEv *)calloc(1000, sizeof(KeyEv));
    while (fgets(line, sizeof line, f) && kev_n < 1000) {
        char k = line[0];
        if (k != 'K' && k != 'S' && k != 'D' && k != 'F' && k != 'M' && k != 'Q') continue;
        KeyEv *e = &kev[kev_n];
        unsigned long d = 0;
        unsigned v = 0;
        char name[16] = "";
        if (k == 'D' || k == 'F') { if (sscanf(line + 1, "%lu %12s", &d, name) != 2) continue; strcpy(e->name, name); }
        else if (k == 'M') { if (sscanf(line + 1, "%lu %d %d %u", &d, &e->dx, &e->dy, &v) != 4) continue; e->val = v; }
        else if (k == 'Q') { if (sscanf(line + 1, "%lu", &d) != 1) continue; }
        else { if (sscanf(line + 1, "%lu %x", &d, &v) != 2) continue; e->val = v; }
        e->kind = k; e->delay = (double)d;
        kev_n++;
    }
    fclose(f);
    kev_cur = 0;
    kev_last = bios_ticks();
}

static void kev_dump(const char *name)
{
    FILE *f = fopen(name, "wb");
    if (!f) return;
    int gfx = video_mode == 0x13;
    fputc(gfx ? 0x13 : 3, f);
    for (int i = 0; i < 256; i++) fwrite(dac[i], 1, 3, f);
    fwrite(gfx ? VRAM : TEXT, 1, gfx ? 64000 : 4000, f);
    fclose(f);
}

/* the script's events that are not keys, when they are due and no key is next: they do not need the program to ask for the keyboard
 * (a mouse-only wait such as the integrator's graph never does) */
static void kev_poll(void)
{
    if (kev_cur == -1) kev_load();
    if (kev_cur < 0) return;
    while (kev_cur < kev_n) {
        KeyEv *e = &kev[kev_cur];
        if (e->kind == 'K' || bios_ticks() - kev_last < e->delay) break;
        if (e->kind == 'S') kev_shift = e->val;
        else if (e->kind == 'D') kev_dump(e->name);
        else if (e->kind == 'F') { }
        else if (e->kind == 'M') {
            int ystep = video_mode == 0x13 ? 1 : 2;
            headless_mx += e->dx * 2; headless_my += e->dy * ystep;
            if (headless_mx < 0) headless_mx = 0;
            if (headless_mx > 639) headless_mx = 639;
            if (headless_my < 0) headless_my = 0;
            if (headless_my > 199) headless_my = 199;
            headless_buttons = (int32_t)e->val;
        }
        else if (e->kind == 'Q') exit(0);
        kev_cur++;
        kev_last = bios_ticks();
    }
}
/* the key that is due now (consumed if `take`); events that are not keys are carried out on the way */
static int kev_key(unsigned *key, int take)
{
    for (;;) {
        if (kev_cur >= kev_n) return 0;
        KeyEv *e = &kev[kev_cur];
        if (bios_ticks() - kev_last < e->delay) return 0;
        if (e->kind == 'K') {
            *key = e->val;
            if (take) { kev_cur++; kev_last = bios_ticks(); }
            return 1;
        }
        if (e->kind == 'S') kev_shift = e->val;
        else if (e->kind == 'D') kev_dump(e->name);
        else if (e->kind == 'F') { /* the ROM font of a DOS machine: nothing to write here */ }
        else if (e->kind == 'M') {                          /* the mouse moves by cursor units (320 across, a text row is 4): headless pointer */
            int ystep = video_mode == 0x13 ? 1 : 2;
            headless_mx += e->dx * 2; headless_my += e->dy * ystep;
            if (headless_mx < 0) headless_mx = 0; if (headless_mx > 639) headless_mx = 639;
            if (headless_my < 0) headless_my = 0; if (headless_my > 199) headless_my = 199;
            headless_buttons = (int32_t)e->val;
        }
        else if (e->kind == 'Q') exit(0);
        kev_cur++;
        kev_last = bios_ticks();
    }
}


/* ---------------------------------------------------------------------------------------------
 * BIOS
 * ------------------------------------------------------------------------------------------- */
uint16_t gm_bioskey(int16_t cmd)
{
    gm_pump();
    if (kev_cur == -1) kev_load();
    if (kev_cur != -2) {                                       /* a test script plays the keyboard */
        unsigned key;
        if (cmd == 2) return (uint16_t)(kev_shift & 0xFF);
        if (cmd == 1) return kev_key(&key, 0) ? (uint16_t)key : 0;
        while (!kev_key(&key, 1)) { gm_pump(); gm_os_sleep_ms(1); }
        return (uint16_t)key;
    }
    if (cmd == 2) return (uint16_t)(shift_held() ? 3 : 0);
    if (cmd == 1) return bios_n ? bios_buf[0] : 0;
    while (!bios_n) { gm_pump(); gm_os_sleep_ms(1); }          /* cmd 0: block */
    uint16_t k = bios_buf[0];
    memmove(bios_buf, bios_buf + 1, (--bios_n) * sizeof bios_buf[0]);
    return k;
}

int16_t gm_kbhit(void) { return gm_bioskey(1) != 0; }
int16_t gm_getch(void) { return (int16_t)(gm_bioskey(0) & 0xFF); }

int16_t gm_int86(int16_t intno, union REGS *in, union REGS *out)
{
    ensure_window();
    if (out != in) *out = *in;
    switch (intno) {
    case 0x10:
        if (in->h.ah == 0x00) {                               /* set video mode */
            int mode = in->h.al & 0x7F;
            video_mode = (mode == 0x13) ? 0x13 : 3;
            if (!(in->h.al & 0x80)) {                         /* clear screen unless bit 7 */
                memset(VRAM, 0, 64000);
                for (int i = 0; i < 80 * 25; i++) { TEXT[i * 2] = ' '; TEXT[i * 2 + 1] = 7; }
            }
            default_palette();
            return 0;
        }
        break;
    case 0x16: return 0;                                      /* keyboard typematic rate etc. */
    case 0x33: out->x.ax = 0; return 0;                       /* no mouse driver (replaced natively) */
    default: break;
    }
    static uint8_t warned[256];
    if (!warned[intno & 255]) { warned[intno & 255] = 1; fprintf(stderr, "gm_int86: unhandled INT %02Xh AX=%04X\n", intno & 255, in->x.ax); }
    return 0;
}

/* ---------------------------------------------------------------------------------------------
 * Time, sound
 * ------------------------------------------------------------------------------------------- */
void gm_delay(uint32_t ms)
{
    ensure_window();
    double end = gm_os_time() + ms / 1000.0;
    do { gm_pump(); gm_os_sleep_ms(1); } while (gm_os_time() < end);
}

void gm_sound(uint16_t hz) { gm_speaker(hz); }       /* PC speaker */
void gm_nosound(void) { gm_speaker(0); }


/* ---------------------------------------------------------------------------------------------
 * Borland C runtime random numbers (CRTL/CLIB/RAND.C): seed = 0x015A4E35 * seed + 1, 15 bits from bit 16.
 * ------------------------------------------------------------------------------------------- */
static int32_t rand_seed = 1;

void gm_srand(uint16_t seed) { rand_seed = seed; }

int16_t gm_rand(void)
{
    rand_seed = (int32_t)((uint32_t)rand_seed * 0x015A4E35u + 1u);       /* 32-bit wraparound */
    return (int16_t)((rand_seed >> 16) & 0x7FFF);
}

uint32_t gm_farsize(const void *p)
{
    uint32_t off = (uint32_t)((const uint8_t *)p - gm_dosmem);
    for (int i = 0; i < nblocks; i++)
        if (blocks[i].off == off && blocks[i].used) return blocks[i].size;
    return 0;
}

/* ---------------------------------------------------------------------------------------------
 * exit() with a memory (see dosplat.h)
 * ------------------------------------------------------------------------------------------- */
int gm_exit_code;
void (*gm_exit_hook)(void);
void gm_exit(int code)
{
    gm_exit_code = code;
    if (gm_exit_hook) gm_exit_hook();
    exit(code);
}
