/* Minimal GIF87a/89a decoder: first frame, indexed output. */
#ifndef GIF_H
#define GIF_H
#include <stdint.h>

typedef struct {
    int w, h;
    uint8_t *pix;        /* w*h palette indices (malloc'd) */
    uint8_t pal[256][3]; /* 8-bit RGB */
    int ncolors;
} GifImage;

int  gif_load(const char *path, GifImage *out);  /* 0 on success */
void gif_free(GifImage *g);
#endif
