/* Smoke test: show a GIF (or a test pattern) through the 320x200 paletted framebuffer.
 * Env: GMVIEW_FRAMES=N runs N frames then exits; GMVIEW_SHOT=file.png saves a screenshot first. */
#include "fb.h"
#include "gif.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    GifImage g = {0};
    int have = argc > 1 && gif_load(argv[1], &g) == 0;
    if (argc > 1 && !have) fprintf(stderr, "could not load %s\n", argv[1]);

    fb_open("GameMaker port - gmview", 3);
    if (have) {
        fprintf(stderr, "%s: %dx%d, %d colors\n", argv[1], g.w, g.h, g.ncolors);
        for (int i = 0; i < 256; i++) {
            fb_pal[i].r = g.pal[i][0]; fb_pal[i].g = g.pal[i][1]; fb_pal[i].b = g.pal[i][2];
        }
        for (int y = 0; y < FB_H && y < g.h; y++)
            for (int x = 0; x < FB_W && x < g.w; x++)
                fb_pix[y * FB_W + x] = g.pix[y * g.w + x];
    } else {
        for (int i = 0; i < 256; i++)
            fb_set_vga_pal(i, (i >> 2) & 63, ((i * 2) >> 2) & 63, 63 - ((i >> 2) & 63));
        for (int y = 0; y < FB_H; y++)
            for (int x = 0; x < FB_W; x++) fb_pix[y * FB_W + x] = (uint8_t)(x ^ y);
    }

    const char *frames = getenv("GMVIEW_FRAMES");
    if (frames) {
        int n = atoi(frames);
        for (int i = 0; i < n; i++) fb_present();
        if (getenv("GMVIEW_SHOT")) fb_save_png(getenv("GMVIEW_SHOT"));
    } else {
        while (!WindowShouldClose()) fb_present();
    }
    fb_close();
    gif_free(&g);
    return 0;
}
