/* KEYS.EXE - drives a DOS program that reads its keyboard through BIOS INT 16h from a script, and dumps the screen
 * on request. Used to run the shipped GameMaker editors the same way as the port (see README.md in this folder).
 *
 *   KEYS script.txt program.exe [args]
 *
 * The script has one event per line, each with a delay in BIOS ticks (18.2 per second) counted from the moment the
 * previous event happened (a key counts as happened when the program takes it):
 *
 *   K <delay> <hex>      the key <hex> (scan code << 8 | ASCII, as INT 16h returns it) becomes available
 *   S <delay> <hex>      the shift-state byte returned by INT 16h function 2 becomes <hex> (3 = shift held)
 *   D <delay> <name>     write the screen to file <name>: video mode (1 byte), the 256 DAC colours (768 bytes of 6 bit
 *                        values), then 64000 bytes (mode 13h) or 4000 bytes (text mode)
 *   F <delay> <name>     write the BIOS 8x8 ROM font (2048 bytes, characters 0..255) to file <name>
 *   M <delay> <dx> <dy> <buttons>   the mouse moves by (dx,dy) cursor units (320 wide) and the button state becomes <buttons>\n *                        (1 left, 2 right, 4 middle). KEYS then plays the mouse driver: it takes INT 33h functions 0 and 0Ch\n *                        and calls the program's event handler itself, with the movement as mickey counters (2 per unit,\n *                        counting on from event to event, as a driver reports them).\n *   Q <delay>            end the program (DOS function 4Ch from the hook)
 *
 * The hook runs on a stack of its own: it is called on the stack of the program being driven, whose SS is not this
 * program's DS, and the C library (and the addresses of locals passed to DOS) assume SS == DS in the small model.
 * Everything the hook touches is static.
 *
 * Build: bcc -ms keys.c (Borland C++ 3.1, see build_keys.ps1; the inline assembly needs TASM on the path).
 */
#pragma inline
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>

#define MAXEV 600
#define STK 2048
#define GRACE 3                          /* ticks the timer waits for the keyboard hook to take an event first: that is a safe point of the program */

typedef struct { char kind; unsigned long delay; unsigned val; int dx, dy; char name[16]; } Ev;

static Ev ev[MAXEV];
static int nev = 0, cur = 0;
static unsigned long lastt = 0;
static unsigned shiftflags = 0;
static void interrupt (*old16)(void);

/* state of the hook (static: see above) */
static unsigned oldss, oldsp;
static char stk[STK];
static unsigned s_ax, r_ax;
static int r_zf;                          /* -1 leave, 0 clear ZF, 1 set ZF */

/* state of dump() */
static unsigned char dacbuf[768];
static union REGS dr;
static struct SREGS dsr;
static int hdump;
static unsigned char dmode;
static unsigned dlen, dwritten;
static unsigned char far *dvid;
static unsigned key;

static unsigned long ticks(void)
  {
  return *(unsigned long far *) MK_FP(0x40, 0x6C);
  }

static void dump(const char *name)
  {
  dr.h.ah = 0x0F; int86(0x10, &dr, &dr); dmode = dr.h.al & 0x7F;
  dr.x.ax = 0x1017; dr.x.bx = 0; dr.x.cx = 256;
  dsr.es = FP_SEG(dacbuf); dr.x.dx = FP_OFF(dacbuf);
  int86x(0x10, &dr, &dr, &dsr);
  if (dmode == 0x13) { dvid = (unsigned char far *) MK_FP(0xA000, 0); dlen = 64000U; }
  else               { dvid = (unsigned char far *) MK_FP(0xB800, 0); dlen = 4000; }
  if (_dos_creat(name, 0, &hdump) != 0) return;
  _dos_write(hdump, &dmode, 1, &dwritten);
  _dos_write(hdump, dacbuf, 768, &dwritten);
  _dos_write(hdump, dvid, dlen, &dwritten);
  _dos_close(hdump);
  }

/* the two halves of the ROM 8x8 font: INT 10h function 1130h with BH = 3 (characters 0..127) and 4 (128..255) */
static unsigned fseg, foff;

/* BP is the frame pointer of the C code, so the call is made in assembly with BP saved */
static void romfont(unsigned which)
  {
  asm push bp
  asm mov ax, 0x1130
  asm mov bx, which
  asm mov bh, bl
  asm xor bl, bl
  asm int 0x10
  asm mov fseg, es
  asm mov foff, bp
  asm pop bp
  }

static void dumpfont(const char *name)
  {
  if (_dos_creat(name, 0, &hdump) != 0) return;
  romfont(3);
  _dos_write(hdump, (void far *) MK_FP(fseg, foff), 1024, &dwritten);
  romfont(4);
  _dos_write(hdump, (void far *) MK_FP(fseg, foff), 1024, &dwritten);
  _dos_close(hdump);
  }

/* the mouse: KEYS answers INT 33h function 0 (reset) and 0Ch (event handler) and calls the handler itself */
#define MICK 2
static unsigned hnd[2];                   /* the handler: offset, segment */
static unsigned m_ax, m_bx, m_cx, m_dx, m_si, m_di;
static unsigned mbuttons = 0;
static int mick_x = 0, mick_y = 0;          /* the driver reports counters that run on, not the movement of one event */
static int usemouse = 0;

static void call_handler(void)
  {
  asm push ds
  asm push es
  asm push bp
  asm push si
  asm push di
  asm mov ax, m_ax
  asm mov bx, m_bx
  asm mov cx, m_cx
  asm mov dx, m_dx
  asm mov si, m_si
  asm mov di, m_di
  asm call dword ptr hnd
  asm pop di
  asm pop si
  asm pop bp
  asm pop es
  asm pop ds
  }

static void mouse_event(int dx, int dy, unsigned buttons)
  {
  unsigned mask = 0, changed = buttons ^ mbuttons;
  if (dx || dy) mask |= 1;
  if (changed & 1) mask |= (buttons & 1) ? 2 : 4;
  if (changed & 2) mask |= (buttons & 2) ? 8 : 16;
  if (changed & 4) mask |= (buttons & 4) ? 32 : 64;
  mbuttons = buttons;
  if (!hnd[1]) return;
  m_ax = mask; m_bx = buttons; m_cx = 0; m_dx = 0;
  mick_x += dx * MICK; mick_y += dy * MICK;
  m_si = (unsigned) mick_x; m_di = (unsigned) mick_y;
  call_handler();
  }
static unsigned char far *indos;          /* DOS's "in use" flag */
static int busy = 0;                      /* the events are being run: nothing may start them again */

/* Runs the events that are due and are not keys, in order, until a key comes next. They happen from the INT 16h hook (a program
 * that waits for the keyboard) and from the timer tick (a program that only waits for the mouse). From the timer only while DOS
 * is idle, because the DOS calls of D, F and Q cannot be reentered; mouse and shift events need no DOS. */
static void run_nonkeys(int from_timer)
  {
  if (busy) return;
  busy = 1;
  for (;;)
    {
    Ev *e;
    if (cur >= nev) break;
    e = &ev[cur];
    if (e->kind == 'K') break;
    if (ticks() - lastt < e->delay + (from_timer ? GRACE : 0)) break;
    if (from_timer && e->kind != 'M' && e->kind != 'S' && *indos) break;
    if (e->kind == 'S') shiftflags = e->val;
    else if (e->kind == 'D') dump(e->name);
    else if (e->kind == 'F') dumpfont(e->name);
    else if (e->kind == 'M') mouse_event(e->dx, e->dy, e->val);
    else if (e->kind == 'Q') { _AX = 0x4C00; geninterrupt(0x21); }
    cur++;
    lastt = ticks();
    }
  busy = 0;
  }

/* 1 and the key in `key` if a key event is due now (consumed if `take`) */
static int due_key(int take)
  {
  Ev *e;
  run_nonkeys(0);
  if (cur >= nev) return 0;
  e = &ev[cur];
  if (e->kind != 'K' || ticks() - lastt < e->delay) return 0;
  key = e->val;
  if (take) { cur++; lastt = ticks(); }
  return 1;
  }
/* INT 16h on the private stack */
static void work(void)
  {
  unsigned char fn = s_ax >> 8;
  r_zf = -1;
  r_ax = s_ax;
  if (fn == 0x00 || fn == 0x10)
    {
    while (!due_key(1)) ;
    r_ax = key;
    }
  else if (fn == 0x01 || fn == 0x11)
    {
    if (due_key(0)) { r_ax = key; r_zf = 0; }
    else r_zf = 1;
    }
  else if (fn == 0x02 || fn == 0x12)
    r_ax = (s_ax & 0xFF00) | (shiftflags & 0xFF);
  }

static void interrupt hook16(unsigned bp, unsigned di, unsigned si, unsigned ds, unsigned es,
                             unsigned dx, unsigned cx, unsigned bx, unsigned ax, unsigned ip, unsigned cs, unsigned flags)
  {
  s_ax = ax;
  asm cli
  asm mov oldss, ss
  asm mov oldsp, sp
  asm mov ax, ds
  asm mov ss, ax
  asm lea sp, stk
  asm add sp, STK
  asm sti
  work();
  asm cli
  asm mov ss, oldss
  asm mov sp, oldsp
  ax = r_ax;
  if (r_zf == 0) flags &= ~0x40;
  else if (r_zf == 1) flags |= 0x40;
  }

static void interrupt (*old33)(void);
static unsigned s33_ax, s33_dx, s33_es, r33_ax, r33_bx;

static void work33(void)
  {
  r33_ax = s33_ax; r33_bx = 0;
  if (s33_ax == 0) { r33_ax = 0xFFFF; r33_bx = 2; }
  else if ((s33_ax & 0xFF) == 0x0C) { hnd[0] = s33_dx; hnd[1] = s33_es; }
  }

static void interrupt hook33(unsigned bp, unsigned di, unsigned si, unsigned ds, unsigned es,
                             unsigned dx, unsigned cx, unsigned bx, unsigned ax, unsigned ip, unsigned cs, unsigned flags)
  {
  s33_ax = ax; s33_dx = dx; s33_es = es;
  asm cli
  asm mov oldss, ss
  asm mov oldsp, sp
  asm mov ax, ds
  asm mov ss, ax
  asm lea sp, stk
  asm add sp, STK
  asm sti
  work33();
  asm cli
  asm mov ss, oldss
  asm mov sp, oldsp
  if (s33_ax == 0) { ax = r33_ax; bx = r33_bx; }
  }
static void interrupt (*old1C)(void);
static char stk2[STK];
static unsigned oldss2, oldsp2;

static void interrupt hook1C(unsigned bp, unsigned di, unsigned si, unsigned ds, unsigned es,
                             unsigned dx, unsigned cx, unsigned bx, unsigned ax, unsigned ip, unsigned cs, unsigned flags)
  {
  asm cli
  asm mov oldss2, ss
  asm mov oldsp2, sp
  asm mov ax, ds
  asm mov ss, ax
  asm lea sp, stk2
  asm add sp, STK
  asm sti
  run_nonkeys(1);
  asm cli
  asm mov ss, oldss2
  asm mov sp, oldsp2
  (*old1C)();
  }
int main(int argc, char *argv[])
  {
  FILE *fp;
  char line[80], k;
  unsigned long d;
  unsigned v;
  char name[16];
  int rc;

  if (argc < 3) { printf("usage: KEYS script program [args]\n"); return 2; }
  fp = fopen(argv[1], "r");
  if (!fp) { printf("KEYS: cannot open %s\n", argv[1]); return 2; }
  while (fgets(line, sizeof line, fp) && nev < MAXEV)
    {
    k = line[0];
    if (k != 'K' && k != 'S' && k != 'D' && k != 'F' && k != 'M' && k != 'Q') continue;
    ev[nev].kind = k;
    ev[nev].name[0] = 0;
    ev[nev].val = 0;
    ev[nev].dx = ev[nev].dy = 0;
    d = 0;
    if (k == 'D' || k == 'F') { if (sscanf(line + 1, "%lu %12s", &d, name) != 2) continue; strcpy(ev[nev].name, name); }
    else if (k == 'M') { if (sscanf(line + 1, "%lu %d %d %u", &d, &ev[nev].dx, &ev[nev].dy, &v) != 4) continue; ev[nev].val = v; usemouse = 1; }
    else if (k == 'Q') { if (sscanf(line + 1, "%lu", &d) != 1) continue; }
    else { if (sscanf(line + 1, "%lu %x", &d, &v) != 2) continue; ev[nev].val = v; }
    ev[nev].delay = d;
    nev++;
    }
  fclose(fp);

  lastt = ticks();
  old16 = getvect(0x16);
  setvect(0x16, hook16);
  asm push es
  asm mov ah, 0x34
  asm int 0x21
  asm mov word ptr indos, bx
  asm mov word ptr indos + 2, es
  asm pop es
  old1C = getvect(0x1C); setvect(0x1C, hook1C);
  if (usemouse) { old33 = getvect(0x33); setvect(0x33, hook33); }
  rc = spawnv(P_WAIT, argv[2], (char **) (argv + 2));
  setvect(0x1C, old1C);
  setvect(0x16, old16);
  if (usemouse) setvect(0x33, old33);
  if (rc == -1) { printf("KEYS: cannot run %s\n", argv[2]); return 3; }
  return rc;
  }
