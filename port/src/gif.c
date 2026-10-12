#include "gif.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const uint8_t *p, *end; } Rd;

static int rd_u8(Rd *r) { return r->p < r->end ? *r->p++ : -1; }
static int rd_u16(Rd *r) { int a = rd_u8(r), b = rd_u8(r); return (a < 0 || b < 0) ? -1 : a | (b << 8); }

/* Gather sub-blocks into one buffer. */
static uint8_t *rd_blocks(Rd *r, size_t *len)
{
    size_t cap = 4096, n = 0;
    uint8_t *buf = malloc(cap);
    int sz;
    while ((sz = rd_u8(r)) > 0) {
        if (r->p + sz > r->end) break;
        if (n + sz > cap) { cap = (n + sz) * 2; buf = realloc(buf, cap); }
        memcpy(buf + n, r->p, sz); n += sz; r->p += sz;
    }
    *len = n;
    return buf;
}

static void lzw(const uint8_t *src, size_t slen, int minbits, uint8_t *dst, size_t dlen)
{
    int clear = 1 << minbits, eoi = clear + 1, next = clear + 2, bits = minbits + 1, prev = -1;
    static uint16_t prefix[4096];
    static uint8_t suffix[4096], stack[4097];
    uint32_t acc = 0;
    int nacc = 0;
    size_t si = 0, di = 0;

    for (int i = 0; i < clear; i++) { prefix[i] = 0xFFFF; suffix[i] = (uint8_t)i; }
    while (di < dlen) {
        while (nacc < bits && si < slen) { acc |= (uint32_t)src[si++] << nacc; nacc += 8; }
        if (nacc < bits) break;
        int code = acc & ((1 << bits) - 1);
        acc >>= bits; nacc -= bits;
        if (code == clear) { next = clear + 2; bits = minbits + 1; prev = -1; continue; }
        if (code == eoi) break;

        int sp = 0, cur = code;
        if (code >= next) {          /* KwKwK case: string is prev + first(prev) */
            if (prev < 0) break;
            stack[sp++] = 0;         /* placeholder, patched with first char below */
            cur = prev;
        }
        while (cur >= clear) { stack[sp++] = suffix[cur]; cur = prefix[cur]; }
        uint8_t first = suffix[cur];
        stack[sp++] = first;
        if (code >= next) stack[0] = first;
        while (sp && di < dlen) dst[di++] = stack[--sp];

        if (prev >= 0 && next < 4096) {
            prefix[next] = (uint16_t)prev; suffix[next] = first; next++;
            if (next == (1 << bits) && bits < 12) bits++;
        }
        prev = code;
    }
}

int gif_load(const char *path, GifImage *g)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(n);
    if (fread(data, 1, n, f) != (size_t)n) { fclose(f); free(data); return -1; }
    fclose(f);

    Rd r = { data, data + n };
    int rc = -1;
    memset(g, 0, sizeof *g);
    if (n < 13 || memcmp(data, "GIF8", 4)) goto done;
    r.p += 6;
    int sw = rd_u16(&r), sh = rd_u16(&r), flags = rd_u8(&r);
    rd_u8(&r); rd_u8(&r);
    if (flags & 0x80) {
        g->ncolors = 2 << (flags & 7);
        for (int i = 0; i < g->ncolors; i++)
            for (int k = 0; k < 3; k++) g->pal[i][k] = (uint8_t)rd_u8(&r);
    }
    for (;;) {
        int b = rd_u8(&r);
        if (b == 0x21) {
            size_t l;
            rd_u8(&r);
            free(rd_blocks(&r, &l));
        } else if (b == 0x2C) {
            int ix = rd_u16(&r), iy = rd_u16(&r), iw = rd_u16(&r), ih = rd_u16(&r), lf = rd_u8(&r);
            if (lf & 0x80) {
                int nc = 2 << (lf & 7);
                g->ncolors = nc;
                for (int i = 0; i < nc; i++)
                    for (int k = 0; k < 3; k++) g->pal[i][k] = (uint8_t)rd_u8(&r);
            }
            int minbits = rd_u8(&r);
            size_t l;
            uint8_t *comp = rd_blocks(&r, &l);
            uint8_t *img = calloc((size_t)iw * ih, 1);
            lzw(comp, l, minbits, img, (size_t)iw * ih);
            free(comp);
            if (lf & 0x40) { /* de-interlace */
                static const int st[4] = {0, 4, 2, 1}, sp[4] = {8, 8, 4, 2};
                uint8_t *tmp = malloc((size_t)iw * ih);
                int row = 0;
                for (int p = 0; p < 4; p++)
                    for (int y = st[p]; y < ih; y += sp[p], row++)
                        memcpy(tmp + (size_t)y * iw, img + (size_t)row * iw, iw);
                free(img);
                img = tmp;
            }
            g->w = sw; g->h = sh;
            g->pix = calloc((size_t)sw * sh, 1);
            for (int y = 0; y < ih && y + iy < sh; y++)
                for (int x = 0; x < iw && x + ix < sw; x++)
                    g->pix[(size_t)(y + iy) * sw + x + ix] = img[(size_t)y * iw + x];
            free(img);
            rc = 0;
            break;
        } else break;
    }
done:
    free(data);
    return rc;
}

void gif_free(GifImage *g) { free(g->pix); g->pix = NULL; }
