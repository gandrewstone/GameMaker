/* C++ replacements for the assembly input modules:
 *   MICROCNL.ASM  keyboard interrupt handlers (INT 9), keyboard IRQ mask, scan->ascii table
 *   JSTICKA.ASM   game-port joystick reading
 *   OLDMOUSE.ASM  mouse driver wrapper (INT 33h)
 *
 * The platform layer (dosplat.c) raises the keyboard "interrupt" for every key press/release, leaving
 * the scan code in port 0x60 exactly as the keyboard controller would.
 */
#include "gen.h"
#include "gmgen.h"
#include "jstick.h"
#include "mousefn.h"

extern volatile int  PendCtr;
extern volatile uchar KeysPending[30];

/* ---------------------------------------------------------------------------------------------
 * Keyboard
 * ------------------------------------------------------------------------------------------- */
uchar ascii[128] =                        // scan code -> ascii (no shift), from MICROCNL.ASM
  {
  0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', 8, 15,
  'q','w','e','r','t','y','u','i','o','p','[',']', 13, 0,
  'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
  'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, 32, 0,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 0, 19, 0, 0, '-', 0, '5',
  0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 11, 12,
  };

static uchar OldKey = 0;
static uchar InInt = 0;
static uchar CurIRQ = 0;

static void KeyInt(int microchannel)
  {
  if (!InInt)
    {
    InInt = 1;
    uchar al = OldKey;
    for (;;)
      {
      uchar cl = al;
      al = (uchar)inportb(0x60);                // the scan code
      if (microchannel && al == 0xE0) al = (uchar)inportb(0x60);   // MCA prefix: take the next byte
      if (al == cl) break;                      // same as last time: nothing new
      KeysPending[PendCtr] = al;
      PendCtr++;
      OldKey = al;
      }
    InInt = 0;
    }
  outportb(0x20, 0x20);                         // end of interrupt
  }

extern "C" void interrupt NewATKbd(void)    { KeyInt(0); }
extern "C" void interrupt NewMicroKbd(void) { KeyInt(1); }

extern "C" char KeyBoardOff(void)
  {
  CurIRQ |= 2;                                  // mask IRQ 1
  outportb(0x21, CurIRQ);
  return (char)CurIRQ;
  }

extern "C" char KeyBoardOn(void)
  {
  CurIRQ &= 0xFD;                               // unmask IRQ 1
  outportb(0x21, CurIRQ);
  return (char)CurIRQ;
  }

extern "C" char MicroChannel(void) { return 0; }   // never a Micro Channel bus

/* ---------------------------------------------------------------------------------------------
 * Joystick: the first gamepad stands in for the game-port stick (positions 0..200, centre 100, two buttons; see
 * gm_joystick_read). Without a gamepad the stick simply sits idle in the centre.
 * ------------------------------------------------------------------------------------------- */
extern "C" void ReadJoyStick(unsigned int far *x, unsigned int far *y, unsigned int far *butn)
  {
  int32_t jx, jy, jb;
  gm_joystick_read(&jx,&jy,&jb);
  *x = (unsigned int)jx;
  *y = (unsigned int)jy;
  *butn = (unsigned int)jb;
  }

#ifndef GM_NEWMOUSE      /* the graphical editors bring their own mouse stack (TRANMOUS.HPP + newmouse_port.cpp) */
/* ---------------------------------------------------------------------------------------------
 * Mouse. Coordinates follow the real driver: 640x200 virtual units in every video mode; the
 * engine divides by MXlatx/MXlaty (8 in text mode, 2 and 1 in mode 13h).
 * ------------------------------------------------------------------------------------------- */
int mouinstall = 0;
int MouseMaxy = 199;
int MouseMaxx = 639;
unsigned int MXlatx = 8;
unsigned int MXlaty = 8;
static int mx = 0, my = 0, mx1 = 639, my1 = 199;

int initmouse(void)
  {
  mouinstall = 1;
  MouseMaxx = 639;
  MouseMaxy = 199;
  return 1;
  }

int moucur(int show)
  {
  static int on = 0;
  if (!mouinstall) return on;
  if (show == 2) return on;                     // status request
  on = show ? 1 : 0;
  gm_mouse_show(on);
  return on;
  }

int setmoupos(int x, int y)
  {
  if (!mouinstall) return 0;
  gm_mouse_set((int32_t)x * (int32_t)MXlatx + (MXlatx >> 1), (int32_t)y * (int32_t)MXlaty + (MXlaty >> 1));
  return 0;
  }

int moustats(int far *x, int far *y, int far *butstats)
  {
  int32_t vx, vy, buttons;
  gm_mouse_get(&vx, &vy, &buttons);
  *butstats = (int)buttons;
  *y = (int)(vy / (int32_t)MXlaty);
  *x = (int)(vx / (int32_t)MXlatx);
  return 1;
  }

int moucurbox(int x, int y, int x1, int y1)
  {
  if (!mouinstall) return 0;
  if (x1 < x) { int t = x1; x1 = x; x = t; }
  if (y1 < y) { int t = y1; y1 = y; y = t; }
  mx = x; my = y; mx1 = x1; my1 = y1;
  return 1;
  }

int getmoubox(int far *x, int far *y, int far *x1, int far *y1)
  {
  if (!mouinstall) return 0;
  *x = mx; *y = my; *x1 = mx1; *y1 = my1;
  return 1;
  }

void mouclearbut(void)
  {
  int x, y, but;
  if (mouinstall)
    do { moustats(&x, &y, &but); gm_pump(); } while (but);
  }
#endif /* GM_NEWMOUSE */
