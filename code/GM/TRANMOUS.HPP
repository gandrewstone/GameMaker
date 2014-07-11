//#include <new.h>
#include <string.h>

//#define DEBUG

#undef MOUSE
void KeyUpdate(int KeyInfo);
extern int CurMode;

#ifdef DEBUG
extern int AbortMSGLength;
extern char AbortMSG[120];
extern int ExitValue;

char NewAbortMSG(char *str)
  {
  int t;

  t=strlen(str);
  if (t<80)
    {
    strcpy(AbortMSG,str);
    AbortMSGLength=t;
    return(TRUE);
    }
  return(FALSE);
  }
#endif

typedef void (*pvf)();
extern pvf _new_handler;

void NewBombOut(void)
  {
  TextMode();
  printf("GM:Not Enough Memory! (new operator)\n");
  exit(1);
  }

#include "genclass.hpp"
#include "timer.hpp"
TimerClass Time;                // Should be the only instance of TimerClass.
#include "windclss.hpp"
#include "gasclass.hpp"
#include "geninput.hpp"

int mouinstall=0;
extern int MouseButs;
DrawingBoard Scrn;
InputClass i;
GPFClass GPFArrow;
CursorClass Cur(&Scrn);
int Jbuttons=0;
void InitGraphics(DrawingBoard *d);

// MouseMoved
//            is called whenever the mouse is moved by the user.
// Friend function to MouseClass.
// Andy Stone                   7/22/92

void MouseMoved(void)
  {
  i.m.Change();
  }

// KeyUpdate
//           is called whenever an input button is pressed or released.
// This can be a key hit on the keyboard, a mouse button, or a joystick button.
// in upper byte, 1=keyboard, 2=mouse, 3=jstk #1, 4=jstk#2
// Andy Stone   7/22/92
void KeyUpdate(int KeyInfo)  
  {
  Jbuttons=KeyInfo&255;
  }

int initmouse(void)
  {
  //set_new_handler(NewBombOut);
  _new_handler = NewBombOut;
  Time.TurnOn();
  InitGraphics(&Scrn);
  Cur.NewGPF(&GPFArrow);
  Cur.SetHotSpot(1,0);
  CurMode=GMODE;
  Cur.Limit(0,0,319,199);
  CurMode=TMODE;
  i.Init();
  if (!i.m.Has) i.j.AddCursor(&Cur);
  else i.m.AddCursor(&Cur);
  mouinstall=i.m.Has;
  return (i.m.Has|i.j.Has);
  }

int moucur(int show)    /* show mouse cursor if show = 1 */
                        /* hide cursor if show = 0, or anything else */
  {                     /* Return cursor status if 2 */
  if (CurMode==GMODE)
    {
    if (show==2) return(Cur.Show);
    if (show==0) Cur.Off();
    if (show==1) Cur.On();
    return(TRUE);
    }
  Cur.Forget();
  return(FALSE);
  }

int setmoupos(int x,int y)  // sets the mouse position on the screen
  {
  if (CurMode==GMODE) Cur.Goto(x,y);
  else
    {
    int tx,ty;
    tx=x<<2; ty=y<<2;
    if (!InBox(Cur.Pos.x,Cur.Pos.y,tx,ty,tx+4,ty+4)) Cur.Goto(tx+2,ty+2);
    }
  return(TRUE);
  }

int moustats(int far *x,int far *y,int far *but)  // get mouse position and the buttons hit
  {
  if (Time.OldClock<Clock)
    {
    Time.OldClock=Clock;
    Time.Traverse(CallEm);
    }
  *x=Cur.Pos.x;
  *y=Cur.Pos.y;
  if (i.m.Has) *but=MouseButs;
  *but|=Jbuttons;
  if (CurMode==TMODE)
    {
    *x >>=2;
    *y >>=2;
    }
  return(TRUE);
  }

int moucurbox(int x,int y,int x1,int y1)   /* set box for mouse to move in */
  {
  if (CurMode==GMODE) Cur.Limit(x,y,x1,y1);
  return(TRUE);
  }

void InitGraphics(DrawingBoard *d)
  {
  d->WindAddr= (void *) 0xA0000000;
  d->MaxX=320;
  d->MaxY=200;
  d->XSkip=320;
  d->Point  = Low256Point;
  d->GetCol = Low256GetCol;
  d->Rep1Col= Low256Rep1Col;
  d->RepCols= Low256RepCols;
  d->GetCols= Low256GetCols;
  GPFArrow.Get(GPFofArrow);
  }

#pragma loop_opt(off)

void mouclearbut(void)
  {
  int x,y,but;
  
  if (i.m.Has) do { moustats(&x,&y,&but); } while(but);
  }

#pragma loop_opt(on)
