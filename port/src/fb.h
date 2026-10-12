/* 8-bit paletted framebuffer presented through raylib (replaces the DOS video drivers). */
#ifndef GM_FB_H
#define GM_FB_H
#include <stdint.h>

#define FB_W 320
#define FB_H 200

typedef struct { uint8_t r, g, b; } FbColor;

extern uint8_t fb_pix[FB_W * FB_H];   /* indexed pixels, row-major */
extern FbColor fb_pal[256];           /* full 8-bit-per-channel palette */

void fb_open(const char *title, int scale);
void fb_close(void);
void fb_set_vga_pal(int idx, int r6, int g6, int b6);  /* classic 6-bit VGA DAC values */
typedef struct { float x, y, w, h; } FbRect;
/* Draws any RGBA image (little-endian 0xAABBGGRR words), letterboxed; reports where it landed. */
void fb_draw_rgba(const uint32_t *px, int w, int h, FbRect *out_rect);
int  fb_save_png(const char *path);                    /* exact 320x200 frame, no window involved */
void fb_set_fullscreen(int on);                        /* borderless full screen on/off */
int  fb_is_fullscreen(void);
void fb_note_fullscreen(int on);                       /* web: the browser changed full screen without us asking */
void fb_toast(const char *fmt, ...);                   /* short message shown over the picture for 1.5 s */
extern FbRect fb_last_rect;                            /* where the last picture landed in the window (mouse mapping) */
/* Where a w x h picture goes in an aw x ah window for a GM_SCALE_* mode (the maths behind fb_draw_rgba). */
FbRect fb_place(int w, int h, float aw, float ah, int mode);
void fb_present(void);                                /* convert + draw + swap; call once per frame */
#endif
