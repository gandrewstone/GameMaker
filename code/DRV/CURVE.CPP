#include <alloc.h>
#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include "gen.h"
#include "pal.h"
#include "Pixel.hpp"
#include "Coord2d.hpp"
#include "VidDrive.hpp"
VideoMode *Vid;
void TextMode(void);
int ModeNum=2;

RGBdata cols[256];

void Curve(Coord2d p1,Coord2d p2,Coord2d VMid,unsigned char col,VideoMode *Vid);
void SpyraCurve(VideoMode *Vid);

void main(int argc,char *argv[])
  {
  VideoDriver *Drvr;
  Pixel Data[1000];
  char Driver[51]="ati.drv";
  if (argc>1)
    {
    ModeNum=argv[1][0]-'0';
    printf("Using Driver Mode #%d\n",ModeNum);
    bioskey(0);
    }
  if (argc>2) strcpy(Driver,argv[2]);

  if (!(Drvr=LoadVideoDriver(Driver))) { printf("Error Loading Driver.\n"); return; }
  Vid=Drvr->GetMode(ModeNum);

  Vid->SetMode(Vid);

  Coord2d From(10,10),To(10,300);
  Coord2d Mid(300,200);

  RainbowCols(cols);
  SetAllPal(cols);

/*  Curve(From,To,Mid,7,Vid);

  From.Set(400,10);
  To.Set(500,400);
  Mid.Set(400,400);
  Curve(From,To,Mid,7,Vid);
*/
  SpyraCurve(Vid);
  TextMode();
  }

void SpyraCurve(VideoMode *Vid)
  {
  boolean done=False;
  Coord2d Pts[3];
  Coord2d Add[3];
  Pixel col=0;
  int TotPts=3;

  Pts[0].Set(0,0);
  Pts[1].Set(0,0);
  Pts[2].Set(0,0);
  Add[0].Set(5,3);
  Add[1].Set(3,7);
  Add[2].Set(7,11);

  while(!done)
    {
    for (int i=0;i<TotPts;i++)
      {
      Curve(Pts[0],Pts[1],Pts[2],0,Vid);
      Pts[i]+=Add[i];
      if (Pts[i].x>=Vid->Size.x) Add[i].x *= -1;
      if (Pts[i].y>=Vid->Size.y) Add[i].y *= -1;
      if (Pts[i].x<0) Add[i].x *= -1;
      if (Pts[i].y<0) Add[i].y *= -1;
      Curve(Pts[0],Pts[1],Pts[2],col++,Vid);
      done=bioskey(1)&255;
      if (done) bioskey(0);
      if (done ==' ') { done=bioskey(0); }
      if (done != 27) done=False;
      }
    }

  }


void Curve(Coord2d p1,Coord2d p2,Coord2d Mid,unsigned char col,VideoMode *Vid)
  {
  int l;
  Coord2d VFrom,VTo,VCur,VBres,VDelta;
  Coord2d Cur,Bres,Old,Delta;
  int Steps,MaxSteps;
  float tf,VMul;

//  Vid->Line(p1,Mid,22,Vid);
//  Vid->Line(p2,Mid,22,Vid);

  VFrom = Mid - p1;              // Create the start direction vector.
  VTo   = p2 - Mid;              // Create the end direction vector.

  Delta = p2-p1;

  // Set up Bresenham's to convert 1 vector to the other.
  VDelta = (VTo-VFrom);
  VCur   = VFrom;
  Cur    = p1;
  VBres.Set(0,0);
  Bres.Set(0,0);

  MaxSteps = (Abs(Delta.x)+Abs(Delta.y));

//  printf("VDelta:(%d,%d) VTo:(%d,%d) VFrom(%d,%d)\n",VDelta.x,VDelta.y,VTo.x,VTo.y,VFrom.x,VFrom.y);

  for (Steps=0;Steps<MaxSteps;Steps++)
    {
    //bioskey(0);
    VBres += AbsC(VDelta);
    while (VBres.x>=MaxSteps) { VBres.x -= MaxSteps; VCur.x+=Sign(VDelta.x); }
    while (VBres.y>=MaxSteps) { VBres.y -= MaxSteps; VCur.y+=Sign(VDelta.y); }

    Old=Cur;

    Bres += AbsC(VCur);
    while (Bres.x>=MaxSteps) { Bres.x -= MaxSteps; Cur.x+=Sign(VCur.x); }
    while (Bres.y>=MaxSteps) { Bres.y -= MaxSteps; Cur.y+=Sign(VCur.y); }

    Vid->Line(Old,Cur,col,Vid);
//    Vid->Line(Cur,Cur+VCur/5,4,Vid);   // Draw the vector.
    }
  }


#define TMODE 3
void TextMode(void)
  {
  union REGS r;

#ifdef MOUSE
  int m;

  if (mouinstall)
    {
    m=moucur(2);
    if (m) moucur(0);
    }
#endif
  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);

#ifdef MOUSE
  if (mouinstall)
    {
    initmouse();
    MXlatx=MouseMaxx/79;
    if (MXlatx==0) MXlatx=1;
    MXlaty=MouseMaxy/24;
    if (MXlaty==0) MXlaty=1;
    if (m) moucur(True);
    }
#endif
  }



