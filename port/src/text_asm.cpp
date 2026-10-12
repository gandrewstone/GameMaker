/* C++ replacement for SCRNROUT.ASM: text-mode (80x25, B800:0000) windows used by the menus.
 *
 * Text memory is 2 bytes per cell: character, then attribute. Offsets are kept 16-bit like the
 * original's `di`, and the drawbox() shadow logic is transcribed step by step, quirks included.
 */
#include "gen.h"
#include "gmgen.h"
#include "scrnrout.h"

#define VIDLEN 80

unsigned char clrchar = 32;                       // char clrbox() fills with (DATASEG _clrchar)

static inline uint8_t *text(void)
  {
  static bool cleared = false;                // a PC starts with a cleared text screen (blank, light grey on black)
  if (!cleared)
    {
    cleared = true;
    for (int i = 0; i < 80 * 25; i++) { gm_dosmem[0xB8000 + i * 2] = ' '; gm_dosmem[0xB8000 + i * 2 + 1] = 7; }
    }
  return gm_dosmem + 0xB8000;
  }

static inline uint16_t cell(int x, int y) { return (uint16_t)(((uint16_t)(y * VIDLEN) + (uint16_t)x) * 2); }

extern "C" int far writestr(int x, int y, int col, const char *str)
  {
  uint16_t di = cell(x, y);
  for (; *str; str++)
    {
    text()[di++] = (uint8_t)*str;
    if (col < 256) text()[di++] = (uint8_t)col;   // col >= 256: leave the attribute alone
    else di++;
    }
  return 0;
  }

extern "C" int far writech(int x, int y, int col, int ch)
  {
  uint16_t di = cell(x, y);
  if (ch < 256) text()[di++] = (uint8_t)ch; else di++;
  if (col < 256) text()[di] = (uint8_t)col;
  return 0;
  }

extern "C" int far readch(int x, int y)
  {
  uint16_t si = cell(x, y);
  return text()[si] | (text()[(uint16_t)(si + 1)] << 8);
  }

static uint16_t box_rows(int y, int y1) { return (uint16_t)(y1 - y + 1); }

extern "C" int far clrbox(int x, int y, int x1, int y1, int attrib)
  {
  uint16_t di = cell(x, y);
  uint16_t cols = (uint16_t)(x1 - x + 1);
  uint16_t skip = (uint16_t)((VIDLEN - cols) * 2);
  for (uint16_t r = box_rows(y, y1); r; r--)
    {
    for (uint16_t c = 0; c < cols; c++)
      {
      text()[di++] = clrchar;
      text()[di++] = (uint8_t)attrib;
      }
    di = (uint16_t)(di + skip);
    }
  return 0;
  }

extern "C" int far savebox(int x, int y, int x1, int y1, char *copyto)
  {
  uint16_t si = cell(x, y);
  uint16_t bytes = (uint16_t)((x1 - x + 1) * 2);
  for (uint16_t r = box_rows(y, y1); r; r--)
    {
    for (uint16_t i = 0; i < bytes; i++) *copyto++ = (char)text()[(uint16_t)(si + i)];
    si = (uint16_t)(si + VIDLEN * 2);
    }
  return 0;
  }

extern "C" int far restorebox(int x, int y, int x1, int y1, char *copyfrom)
  {
  uint16_t di = cell(x, y);
  uint16_t bytes = (uint16_t)((x1 - x + 1) * 2);
  for (uint16_t r = box_rows(y, y1); r; r--)
    {
    for (uint16_t i = 0; i < bytes; i++) text()[(uint16_t)(di + i)] = (uint8_t)*copyfrom++;
    di = (uint16_t)(di + VIDLEN * 2);
    }
  return 0;
  }

/* Box characters: [UL, UR, LL, LR, horizontal, vertical] for line types 0 (blank), 1 (single), 2 (double) */
static const uint8_t linetypes[18] =
  {
  ' ', ' ', ' ', ' ', ' ', ' ',
  0xDA, 0xBF, 0xC0, 0xD9, 0xC4, 0xB3,
  0xC9, 0xBB, 0xC8, 0xBC, 0xCD, 0xBA,
  };


extern "C" int far drawbox(int x, int y, int x1, int y1, int col, int ltype, int shadcol)
  {
  uint8_t *t = text();
  uint16_t corners[4];
  const uint8_t color = (uint8_t)col;
  const uint8_t shade = (uint8_t)shadcol;
  int bx = ltype * 6;
  uint16_t di;
  const uint16_t span = (uint16_t)(((uint16_t)(x1 - x) - 1) << 1);   // bytes between the two side corners

  di = cell(x, y);                                // upper left
  corners[0] = di;
  t[di] = linetypes[bx]; t[di + 1] = color; di = (uint16_t)(di + 2);

  bx++;                                           // upper right
  di = (uint16_t)(di + span);
  corners[1] = di;
  t[di] = linetypes[bx]; t[di + 1] = color; di = (uint16_t)(di + 2);

  bx++;                                           // lower left
  di = (uint16_t)(((uint16_t)(y1 * VIDLEN) + (uint16_t)x) << 1);
  corners[2] = di;
  t[di] = linetypes[bx]; t[di + 1] = color; di = (uint16_t)(di + 2);

  bx++;                                           // lower right
  di = (uint16_t)(di + span);
  corners[3] = di;
  t[di] = linetypes[bx]; t[di + 1] = color; di = (uint16_t)(di + 2);

  bx++;                                           // horizontal lines
  uint16_t len = (uint16_t)(x1 - x - 1);
  di = (uint16_t)(corners[0] + 2);
  for (uint16_t i = 0; i < len; i++) { t[di++] = linetypes[bx]; t[di++] = color; }
  di = (uint16_t)(corners[2] + 2);
  for (uint16_t i = 0; i < len; i++) { t[di++] = linetypes[bx]; t[di++] = color; }

  bx++;                                           // vertical lines (+ shadow)
  const uint8_t vchar = linetypes[bx];
  const uint16_t cx = span;                       // bytes between left and right line
  const uint16_t around = (uint16_t)((VIDLEN * 2 - cx) - 4);
  uint16_t dx = (uint16_t)(y1 - y - 1);
  di = (uint16_t)(corners[1] + 2 + around);
  do
    {
    dx--;
    t[di] = vchar; t[di + 1] = color; di = (uint16_t)(di + 2);      // left vertical line
    di = (uint16_t)(di + cx);
    t[di] = vchar; t[di + 1] = color; di = (uint16_t)(di + 2);      // right vertical line
    if (shade)                                                       // two attribute-only shadow cells
      {
      t[(uint16_t)(di + 1)] = shade;
      t[(uint16_t)(di + 3)] = shade;
      }
    di = (uint16_t)(di + around);
    } while (dx != 0);

  if (!shade) return 0;
  di = (uint16_t)(di + cx + 4);                   // shadow beside the bottom row
  t[(uint16_t)(di + 1)] = shade;
  t[(uint16_t)(di + 3)] = shade;

  di = (uint16_t)(corners[2] + VIDLEN * 2 + 5);   // shadow under the box
  for (uint16_t i = 0; i < (uint16_t)(x1 - x + 1); i++) { t[di] = shade; di = (uint16_t)(di + 2); }
  return 0;
  }
