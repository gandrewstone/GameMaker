/* C++ replacements for the 16-bit assembly graphics routines the engine links:
 *   SVGAA.ASM   Point, GetCol, BoxFill, Gwritestr, SvgaBufToScrn, SoftwareScroll, GetROMFont, ...
 *   BLOCA.ASM   drawblk, getblk
 *   MEMBLOCA.ASM BufDrawBlk, BufGetBlk, BufDrawSpBlk, BufDraw4x4Addr (and their *Addr forms)
 *   PALA.ASM    Palette, GetAllPal, SetAllPal, SetAllPalTo
 *
 * The originals address video memory (A000:0000) and the off-screen "scratch" buffer (ScratchSeg:0)
 * with 16-bit offsets that wrap at 64 KB; every offset below is therefore a uint16_t on top of a
 * flat 64 KB window inside the emulated address space.
 */
#include "gen.h"
#include "gmgen.h"
#include "svga.h"
#include "graph.h"
#include "bloc.h"
#include "dirty.h"
#include "pal.h"
#include "palette.h"
#include "font8x8.h"

extern uint zeroaddon;
extern uint xor;                       // GRAPHC.C: draw with XOR when non-zero
extern uchar curpage, zeropage;        // svga_port.cpp

static inline uint8_t *vram(void)    { return gm_dosmem + 0xA0000; }
static inline uint8_t *scratch(void) { return gm_dosmem + ((uint32_t)ScratchSeg << 4); }

volatile ulongi TimerCounter = 1;      // advanced by the engine's INT 8 handler (was in svgaa.asm)

/* ---------------------------------------------------------------------------------------------
 * BIOS ROM 8x8 font. The engine reads glyph bitmaps straight from the ROM table (F000:FA6E).
 * ------------------------------------------------------------------------------------------- */
extern "C" char far *GetROMFont(void)
  {
  uint8_t *rom = (uint8_t *)MK_FP(0xF000, 0xFA6E);
  static int init = 0;
  if (!init)
    {
    memcpy(rom, font8x8, sizeof font8x8);
    // Test aid: GM_ROMFONT=<file> replaces the glyphs with a 2048 byte dump of another machine's ROM font (the editors
    // oracle uses the one DOSBox shows: baseline/editors), so that screens can be compared pixel for pixel.
    if (const char *path = getenv("GM_ROMFONT"))
      if (FILE *f = fopen(path, "rb")) { if (fread(rom, 1, 2048, f) != 2048) memcpy(rom, font8x8, sizeof font8x8); fclose(f); }
    init = 1;
    }
  return (char *)rom;
  }

/* ---------------------------------------------------------------------------------------------
 * Pixel access on the visible screen (SVGAA.ASM)
 * ------------------------------------------------------------------------------------------- */
extern "C" void far BoxFill(int x, int y, int x1, int y1, unsigned char col)
  {
  uint16_t di = (uint16_t)((uint16_t)(y * 320) + (uint16_t)x);
  uint16_t width = (uint16_t)(x1 - x + 1);
  uint32_t rows = (uint16_t)(y1 - y + 1);
  if (rows == 0) rows = 65536;                  // `loop` with cx == 0 (original behaviour)
  for (; rows; rows--)
    {
    uint16_t o = di;
    for (uint16_t i = 0; i < width; i++) vram()[o++] = col;
    di = (uint16_t)(di + 320);
    }
  }

extern "C" void Point(int x, int y, unsigned char col)
  {
  uint16_t di = (uint16_t)((uint16_t)(y * 320) + (uint16_t)x + zeroaddon);
  if (xor) vram()[di] ^= col;
  else     vram()[di]  = col;
  }

extern "C" uchar GetCol(int x, int y)
  {
  return vram()[(uint16_t)((uint16_t)(y * 320) + (uint16_t)x + zeroaddon)];
  }

/* BIOS INT 10h/13h: write a string with the ROM font in graphics mode (40x25 character cells). */
extern "C" void far Gwritestr(int x, int y, int col, char *str, int len)
  {
  for (int n = 0; n < len; n++)
    {
    const uint8_t *glyph = (const uint8_t *)font8x8 + (uint8_t)str[n] * 8;
    int cx = (x + n) * 8, cy = y * 8;
    for (int row = 0; row < 8; row++)
      for (int bit = 0; bit < 8; bit++)
        {
        int px = cx + bit, py = cy + row;
        if (px >= 320 || py >= 200) continue;
        vram()[py * 320 + px] = (glyph[row] & (0x80 >> bit)) ? (uint8_t)col : 0;
        }
    }
  }

/* ---------------------------------------------------------------------------------------------
 * Scratch buffer -> screen
 * ------------------------------------------------------------------------------------------- */
extern "C" void SvgaBufToScrn(unsigned int len, unsigned int offset)
  {
  for (uint32_t i = 0; i < len; i++)
    {
    uint16_t o = (uint16_t)(offset + i);
    vram()[o] = scratch()[o];
    }
  curpage = (uchar)(zeropage + 1);              // the original leaves curpage on the next page
  }

extern "C" void SoftwareScroll(void)
  {
  // Circular 64 KB scratch buffer, viewed from `zeroaddon`: screen[i] = scratch[zeroaddon + i].
  uint32_t n = (zeroaddon > 1536) ? 65536u : 64000u;
  for (uint32_t i = 0; i < n; i++) vram()[i] = scratch()[(uint16_t)(zeroaddon + i)];
  }

extern "C" int MoveViewScreen(unsigned long offset4, unsigned int LittleShiftx)
  {
  return 0;                                     // hardware panning does not exist; SoftwareSim never calls it
  }

extern "C" void SetPage(unsigned char pgnum) {}
extern "C" void SetTrident(void) {}
extern "C" void SetTridentStart(int highstart) {}

/* ---------------------------------------------------------------------------------------------
 * 20x20 blocks (BLOCA.ASM) -- direct to the screen
 * ------------------------------------------------------------------------------------------- */
int drawblk(int x, int y, unsigned char *bloc)
  {
  uint16_t di = (uint16_t)((uint16_t)(y * 320) + (uint16_t)x + zeroaddon);
  for (int row = 0; row < 20; row++)
    {
    for (int col = 0; col < 20; col++) vram()[(uint16_t)(di + col)] = bloc[row * 20 + col];
    di = (uint16_t)(di + 320);
    }
  return 0;
  }

int getblk(int x, int y, unsigned char *bloc)
  {
  uint16_t si = (uint16_t)((uint16_t)(y * 320) + (uint16_t)x + zeroaddon);
  for (int row = 0; row < 20; row++)
    {
    for (int col = 0; col < 20; col++) bloc[row * 20 + col] = vram()[(uint16_t)(si + col)];
    si = (uint16_t)(si + 320);
    }
  return 0;
  }

/* ---------------------------------------------------------------------------------------------
 * 20x20 blocks (MEMBLOCA.ASM) -- into/out of the scratch buffer
 * ------------------------------------------------------------------------------------------- */
static inline uint16_t scratch_addr(int x, int y)
  {
  return (uint16_t)((uint16_t)(y * 320) + (uint16_t)x + zeroaddon);
  }

static void draw_block(uint16_t di, const unsigned char *blk)
  {
  for (int row = 0; row < 20; row++)
    {
    for (int col = 0; col < 20; col++) scratch()[(uint16_t)(di + col)] = blk[row * 20 + col];
    di = (uint16_t)(di + 320);
    }
  }

static void get_block(uint16_t si, unsigned char *blk)
  {
  for (int row = 0; row < 20; row++)
    {
    for (int col = 0; col < 20; col++) blk[row * 20 + col] = scratch()[(uint16_t)(si + col)];
    si = (uint16_t)(si + 320);
    }
  }

extern "C" void BufDrawBlk(int x, int y, unsigned char *blk)          { draw_block(scratch_addr(x, y), blk); }
extern "C" void BufDrawBlkAddr(unsigned int loc, unsigned char *blk)  { draw_block((uint16_t)loc, blk); }
extern "C" void BufGetBlk(int x, int y, unsigned char *blk)           { get_block(scratch_addr(x, y), blk); }
extern "C" void BufGetBlkAddr(unsigned int loc, unsigned char *blk)   { get_block((uint16_t)loc, blk); }

/* A 4x4 piece of a 20-wide block (source stride 20) */
extern "C" void BufDraw4x4Addr(unsigned int loc, unsigned char *pict)
  {
  uint16_t di = (uint16_t)loc;
  for (int row = 0; row < 4; row++)
    {
    for (int col = 0; col < 4; col++) scratch()[(uint16_t)(di + col)] = pict[row * 20 + col];
    di = (uint16_t)(di + 320);
    }
  }

extern "C" void BufDraw4x20Addr(unsigned int loc, unsigned char *pict)
  {
  uint16_t di = (uint16_t)loc;
  for (int row = 0; row < 20; row++)
    {
    for (int col = 0; col < 4; col++) scratch()[(uint16_t)(di + col)] = pict[row * 20 + col];
    di = (uint16_t)(di + 320);
    }
  }

/* Transparent (colour 255) block with one of 8 orientations.
 * Per orientation: step to the next source pixel in a row, extra step after each row, start offset. */
static void draw_sp_block(uint16_t di, int rot, const unsigned char *blk)
  {
  static const struct { int addx, addy, init; } T[8] = {
    {   1,    0,   0 },   // 0 normal
    { -20,  401, 380 },   // 1
    {  -1,    0, 399 },   // 2 rotate 180
    {  20, -401,  19 },   // 3
    {  -1,   40,  19 },   // 4 flip horizontal
    {  20, -399,   0 },   // 5
    {   1,  -40, 380 },   // 6 flip vertical
    { -20,  399, 399 },   // 7 and above
  };
  const int r = (rot < 0) ? 0 : (rot > 7 ? 7 : rot);
  int si = T[r].init;
  for (int row = 0; row < 20; row++)
    {
    for (int col = 0; col < 20; col++)
      {
      uint8_t px = blk[si];
      if (px != 255) scratch()[di] = px;
      di++;
      si += T[r].addx;
      }
    di = (uint16_t)(di + 300);
    si += T[r].addy;
    }
  }

extern "C" void BufDrawSpBlk(int x, int y, char rotation, unsigned char *blk)
  {
  draw_sp_block(scratch_addr(x, y), (uint8_t)rotation, blk);
  }

extern "C" void BufDrawSpBlkAddr(unsigned int loc, char rotation, unsigned char *blk)
  {
  draw_sp_block((uint16_t)loc, (uint8_t)rotation, blk);
  }


extern "C" void BufPoint(int x, int y, unsigned char col)
  {
  scratch()[scratch_addr(x, y)] = col;
  }

/* ---------------------------------------------------------------------------------------------
 * VGA palette (PALA.ASM) through the DAC ports
 * ------------------------------------------------------------------------------------------- */
extern "C" void Palette(unsigned char col, unsigned char red, unsigned char green, unsigned char blue)
  {
  outportb(0x3C8, col);
  outportb(0x3C9, red);
  outportb(0x3C9, green);
  outportb(0x3C9, blue);
  }

extern "C" void SetAllPal(RGBdata *pal)
  {
  const uint8_t *p = (const uint8_t *)pal;
  outportb(0x3C8, 0);
  for (int i = 0; i < 768; i++) outportb(0x3C9, p[i]);
  }

extern "C" void SetAllPalTo(RGBdata *pal)       // every entry gets the colour of pal[0]
  {
  const uint8_t *p = (const uint8_t *)pal;
  outportb(0x3C8, 0);
  for (int i = 0; i < 256; i++)
    for (int k = 0; k < 3; k++) outportb(0x3C9, p[k]);
  }

extern "C" void GetAllPal(RGBdata *pal)
  {
  uint8_t *p = (uint8_t *)pal;
  outportb(0x3C7, 0);
  for (int i = 0; i < 768; i++) p[i] = inportb(0x3C9);
  }
