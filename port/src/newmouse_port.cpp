/* C++ replacement for NEWMOUSE.ASM, the input and drawing support of the graphical editors:
 *   Clock / NewTimer / OldTimer   18.2 Hz tick counter (the editors time animation previews with it); the platform calls
 *                                 the user-timer vector INT 1Ch, which TimerClass::TurnOn() points at NewTimer
 *   SetMouseRoutine / MouseInterrupt   the INT 33h function 0Ch event handler. There is no mouse driver here: the
 *                                 handler is replaced by PollMouse(), which gm_pump() calls, and which does what
 *                                 MouseInterrupt did: move the cursor, set MouseButs and call KeyUpdate()
 *   Low256RepCols / Rep1Col / GetCols   row transfers of the DrawingBoard that TRANMOUS.HPP sets up on A000:0000
 *
 * The original is relative (mickeys). When the program moves the cursor itself (Cursor.Goto, Limit, joystick) the host
 * pointer is moved along, so that the two stay together. A browser cannot move the pointer, and there relative movement
 * lets the two drift apart (Limit, entering from another edge): the web page puts the cursor at the pointer instead,
 * whenever the pointer moves (gm_mouse_follows_host).
 */
#include "gen.h"
#include "genclass.hpp"
#include "timer.hpp"
#include "windclss.hpp"
#include "gasclass.hpp"
#include "geninput.hpp"
#include "dosplat.h"
#include "graph.h"
#include "osclock.h"

gm_ulong Clock = 0;
gm_ulong OldTimer = 0;
unsigned int MyDS = 0;

void interrupt NewTimer(void)
  {
  Clock++;
  }

// defined by the program that includes TRANMOUS.HPP, and by GENINPUT.CPP
extern CursorClass Cur;
extern unsigned int CurMode;     // GENC.C
extern int MouseButs;
void KeyUpdate(int KeyInfo);

extern "C" void MouseInterrupt(void)
  {
  // The driver called this with the event in registers. The port never installs it (see SetMouseRoutine).
  }

// The cursor spans 320x200 units in graphics mode. In text mode a row is 4 units, so the picture's 25 rows are 100 units
// high (moustats() divides by 4), while the pointer's virtual range stays 200 high.
static inline int ystep(void) { return CurMode == TMODE ? 2 : 1; }

static void PollMouse(void)
  {
  static int started = 0;
  static int32_t lastx, lasty;            // pointer position at the last poll (virtual 640x200 units)
  static int curx, cury;                  // cursor position after the last poll
  int32_t vx, vy, buts;

  gm_mouse_get(&vx, &vy, &buts);
  if (!started || Cur.Pos.x != curx || Cur.Pos.y != cury)
    {
    // first poll, or the program moved the cursor: bring the host pointer to it (hot spot of the cursor image included)
    if (started || Cur.Pos.x != 0 || Cur.Pos.y != 0) gm_mouse_set(Cur.Pos.x * 2 + 1, Cur.Pos.y * ystep() + 1);
    gm_mouse_get(&vx, &vy, &buts);
    lastx = vx; lasty = vy;
    curx = Cur.Pos.x; cury = Cur.Pos.y;
    started = 1;
    }
  // A new window first reports the pointer at (0,0), then where it really is. That is not the user moving the mouse, so
  // for the first moments the pointer only sets the reference position.
  static double settle_until = -1;
  if (settle_until < 0) settle_until = gm_os_time() + 0.6;
  if (gm_os_time() < settle_until) { lastx = vx; lasty = vy; }
  bool moved = vx != lastx || vy != lasty;
  int dx, dy;
  if (!gm_mouse_follows_host())
    {
    // DOS behaviour (the oracle in baseline/editors): by movement, which Limit() clamps and loses. Native builds warp the
    // host pointer along with the cursor, so the two stay together; captured, there is no host pointer to keep company.
    dx = (int)(vx / 2) - (int)(lastx / 2);              // in cursor units: 320 across the picture
    dy = (int)(vy / ystep()) - (int)(lasty / ystep());
    }
  else
    {
    // Web page: the cursor goes where the host pointer is (the same cell as gm_mouse_set aims at), whatever way the
    // pointer came in. Movement that Limit() or an earlier poll dropped therefore cannot pile up as an offset. The pointer is only
    // looked at when it moved, so a cursor the program moved itself (keys, joystick) stays where the program put it:
    // a browser cannot warp the host pointer to follow.
    dx = moved ? vx / 2 - Cur.Pos.x : 0;
    dy = moved ? vy / ystep() - Cur.Pos.y : 0;
    }
  // Cursor.Move() ignores movement while the cursor is being drawn (Moving). The pointer position is only taken as the
  // new reference once the movement has been applied, otherwise that part of the movement would be lost for good.
  if (!Cur.Moving) { lastx = vx; lasty = vy; }

  if ((dx != 0 || dy != 0) && !Cur.Moving)
    {
    // The original fed mickey counters to MouseClass::Change(), whose loops only end through 16-bit wrap-around when a
    // counter is cumulative, and which the DOS driver fed with raw movement. The pointer position is exact here, so
    // the cursor (there is one: Cur, see TRANMOUS.HPP) is moved by the difference.
    Cur.Move(dx, dy);
    curx = Cur.Pos.x; cury = Cur.Pos.y;
    }
  if ((int)buts != MouseButs)
    {
    MouseButs = (int)buts;
    KeyUpdate((2 << 8) | (MouseButs & 255));        // 1=keyboard 2=mouse 3,4=joystick 1,2
    }
  }

extern "C" void SetMouseRoutine(void Routine(void))
  {
  gm_set_pump_hook(PollMouse);
  }

// ---------------------------------------------------------------------------------------------------------------
// Row transfers. The asm compares with signed 16-bit jumps and addresses A000:0000 with 16-bit offsets; both kept.
// ---------------------------------------------------------------------------------------------------------------
static inline unsigned char *board(DrawingBoard *d) { return (unsigned char *)d->WindAddr; }

static bool clip_row(unsigned int x, unsigned int y, unsigned int *len, DrawingBoard *d)
  {
  short sy = (short)y, sx = (short)x;
  if (sy >= d->MaxY) return false;
  if (sx >= d->MaxX) return false;
  short room = (short)(d->MaxX - sx);
  if (room < (short)*len) *len = (unsigned short)room;
  return true;
  }

static inline unsigned short row_offset(unsigned int x, unsigned int y, DrawingBoard *d)
  {
  return (unsigned short)((unsigned short)(d->XSkip * (unsigned short)y) + (unsigned short)x);
  }

extern "C" void Low256RepCols(unsigned int x, unsigned int y, unsigned int len, unsigned char *col, DrawingBoard *d)
  {
  if (!clip_row(x, y, &len, d)) return;
  unsigned short off = row_offset(x, y, d);
  for (unsigned int i = 0; i < (unsigned short)len; i++) board(d)[(unsigned short)(off + i)] = col[i];
  }

extern "C" void Low256Rep1Col(unsigned int x, unsigned int y, unsigned int len, unsigned int col, DrawingBoard *d)
  {
  if (!clip_row(x, y, &len, d)) return;
  unsigned short off = row_offset(x, y, d);
  for (unsigned int i = 0; i < (unsigned short)len; i++) board(d)[(unsigned short)(off + i)] = (unsigned char)col;
  }

extern "C" void Low256GetCols(unsigned int x, unsigned int y, unsigned int len, unsigned char *col, DrawingBoard *d)
  {
  if (!clip_row(x, y, &len, d)) return;
  unsigned short off = row_offset(x, y, d);
  for (unsigned int i = 0; i < (unsigned short)len; i++) col[i] = board(d)[(unsigned short)(off + i)];
  }
