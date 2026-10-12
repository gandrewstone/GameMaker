// C++ replacement for CLRBLOCA.ASM: draw a 20x20 block straight to the screen, skipping colour 255 (transparent).
// Only MAPMAKER uses it. Offsets wrap at 64 KB like the original's 16-bit di.
#include "gen.h"
#include "gmgen.h"

extern uint zeroaddon;

extern "C" int drawcbloc(int x, int y, char *bloc)
  {
  uint8_t *vram = gm_dosmem + 0xA0000;
  uint16_t di = (uint16_t)((uint16_t)(y * 320) + (uint16_t)x + zeroaddon);
  for (int row = 0; row < 20; row++)
    {
    for (int col = 0; col < 20; col++)
      {
      uint8_t c = (uint8_t)bloc[row * 20 + col];
      if (c != 255) vram[(uint16_t)(di + col)] = c;
      }
    di = (uint16_t)(di + 320);
    }
  return 0;
  }
