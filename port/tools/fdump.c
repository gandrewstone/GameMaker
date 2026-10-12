/* Reader/comparer for FRAMEDUMP files written by the instrumented DOS playgame.
 *
 *   fdump list  a.bin                 one line per frame (tick, score, scene, char pos, pan)
 *   fdump png   a.bin N out.png       export frame N (palette-expanded) as a PNG
 *   fdump diff  a.bin b.bin           compare two dumps: header fields and pixel counts
 *
 * Record layout (little-endian, 64794 bytes): "FRM1", i32 tick, i32 timer, i32 score,
 * i16 scene, i16 charx, i16 chary, u16 zeroaddon, i16 zeropage, 768B 6-bit palette, 64000B pixels.
 */
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PIXELS 64000
#define REC    (26 + 768 + PIXELS)

typedef struct {
    int32_t tick, timer, score;
    int16_t scene, cx, cy;
    uint16_t zadd;
    int16_t zpage;
    uint8_t pal[768];
    uint8_t pix[PIXELS];
} Frame;

static int16_t i16(const uint8_t *p) { return (int16_t)(p[0] | (p[1] << 8)); }
static int32_t i32(const uint8_t *p) { return (int32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24)); }

static uint8_t *slurp(const char *path, long *n)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(2); }
    fseek(f, 0, SEEK_END); *n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc(*n);
    if (fread(b, 1, *n, f) != (size_t)*n) { fprintf(stderr, "short read %s\n", path); exit(2); }
    fclose(f);
    if (*n % REC) fprintf(stderr, "warning: %s size %ld is not a multiple of %d\n", path, *n, REC);
    return b;
}

static int get(const uint8_t *b, long n, int i, Frame *fr)
{
    if ((long)(i + 1) * REC > n) return 0;
    const uint8_t *p = b + (long)i * REC;
    if (memcmp(p, "FRM1", 4)) { fprintf(stderr, "bad magic at frame %d\n", i); return 0; }
    fr->tick = i32(p + 4); fr->timer = i32(p + 8); fr->score = i32(p + 12);
    fr->scene = i16(p + 16); fr->cx = i16(p + 18); fr->cy = i16(p + 20);
    fr->zadd = (uint16_t)i16(p + 22); fr->zpage = i16(p + 24);
    memcpy(fr->pal, p + 26, 768);
    memcpy(fr->pix, p + 26 + 768, PIXELS);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: fdump list|png|diff ...\n"); return 2; }
    long n; uint8_t *a = slurp(argv[2], &n);
    int frames = (int)(n / REC);
    static Frame fa, fb;

    if (!strcmp(argv[1], "list")) {
        for (int i = 0; i < frames; i++) {
            get(a, n, i, &fa);
            printf("%3d tick=%-5d timer=%-6d score=%-6d scene=%d char=(%d,%d) pan=%u/%d\n",
                   i, fa.tick, fa.timer, fa.score, fa.scene, fa.cx, fa.cy, fa.zadd, fa.zpage);
        }
    } else if (!strcmp(argv[1], "png") && argc == 5) {
        int i = atoi(argv[3]);
        if (!get(a, n, i, &fa)) { fprintf(stderr, "no frame %d\n", i); return 1; }
        static uint8_t rgb[320 * 200 * 4];
        for (int k = 0; k < 320 * 200; k++) {
            const uint8_t *c = fa.pal + fa.pix[k] * 3;
            for (int ch = 0; ch < 3; ch++) rgb[k * 4 + ch] = (uint8_t)((c[ch] << 2) | (c[ch] >> 4));
            rgb[k * 4 + 3] = 255;
        }
        Image img = { rgb, 320, 200, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        return ExportImage(img, argv[4]) ? 0 : 1;
    } else if (!strcmp(argv[1], "diff") && argc == 4) {
        long m; uint8_t *b = slurp(argv[3], &m);
        int fc = (int)(m / REC), common = frames < fc ? frames : fc;
        int bad_logic = 0, bad_pan = 0, bad_pix = 0, shown = 0, same_pan = 0, same_pan_bad = 0;
        printf("frames: %d vs %d\n", frames, fc);
        for (int i = 0; i < common; i++) {
            get(a, n, i, &fa); get(b, m, i, &fb);
            /* logical game state must match exactly; the hardware pan offset and raw video
             * memory depend on sound-clock timing and are reported separately */
            int logic = fa.tick != fb.tick || fa.score != fb.score || fa.scene != fb.scene ||
                        fa.cx != fb.cx || fa.cy != fb.cy;
            int pan = fa.zadd != fb.zadd || fa.zpage != fb.zpage;
            int diffpix = 0;
            for (int k = 0; k < PIXELS; k++) diffpix += fa.pix[k] != fb.pix[k];
            bad_logic += logic; bad_pan += pan; bad_pix += diffpix != 0;
            if (!logic && !pan) {                          /* same state and scroll offset: pixels must match exactly */
                same_pan++;
                if (diffpix) {
                    same_pan_bad++;
                    if (same_pan_bad <= 10) printf("frame %3d: SAME PAN, %d pixels differ\n", i, diffpix);
                }
            }
            if ((logic || pan || diffpix) && shown++ < 10)
                printf("frame %3d:%s%s %d pixels differ  A=(%d,%d,score %d,pan %u/%d) B=(%d,%d,score %d,pan %u/%d)\n",
                       i, logic ? " LOGIC" : "", pan ? " pan" : "", diffpix,
                       fa.cx, fa.cy, fa.score, fa.zadd, fa.zpage, fb.cx, fb.cy, fb.score, fb.zadd, fb.zpage);
        }
        printf("with identical state and pan (%d frames): %d differ in pixels\n", same_pan, same_pan_bad);
        printf("of %d frames: %d differ in logic, %d in pan offset, %d in pixels\n",
               common, bad_logic, bad_pan, bad_pix);
        return bad_logic || frames != fc;
    } else { fprintf(stderr, "bad arguments\n"); return 2; }
    return 0;
}
