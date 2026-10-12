// Globals that the player defines in PLAYGAME.C / DIRTRECT.C / SVGAC.C and that the shared replacement modules
// (gfx_asm.cpp, input_asm.cpp) refer to. The editors have no off-screen scratch screen, no video pages and no keyboard
// interrupt queue: the values stay at their start-up state.
#include "gen.h"
#include "gmgen.h"

unsigned int ScratchSeg = 0;
uchar curpage = 0;
uchar zeropage = 0;
volatile int PendCtr = 0;
volatile uchar KeysPending[30];
