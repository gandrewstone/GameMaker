#include <alloc.h>
#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "gen.h"

#include "Coord2d.hpp"
#include "Pixel.hpp"
#include "VidDrive.hpp"

extern "C" far VideoMode Scrn;
VideoMode   *Vid;

void TextMode(void);

extern "C"
  {
  void SetMode (VideoMode *v);
  void SetVESAMode (VideoMode *v);

  void Clear   (Pixel Col,VideoMode *v);
  void Point   (Coord2d Pos, Pixel col,VideoMode *v);
  Pixel GetCol (Coord2d Pos,VideoMode *v);
  void Rep1Col (Coord2d Pos,int Num,Pixel col,VideoMode *v);
  void Rep1ColRev (Coord2d Pos,int Num,Pixel col,VideoMode *v);
  void RepCols (Coord2d Pos,int Num,Pixel *cols,VideoMode *v);
  void RepColsRev (Coord2d Pos,int Num,Pixel *cols,VideoMode *v);
  int  GetCols (Coord2d Pos,int Num,Pixel *cols,VideoMode *v);
  int  GetColsRev (Coord2d Pos,int Num,Pixel *cols,VideoMode *v);
  void BoxFill (Coord2d Start,Coord2d End,Pixel col,VideoMode *v);
  void DrawBlock (Coord2d Pos,Block b,VideoMode *v);
//static void SetViewPos (unsigned long int pos,VideoMode *v);
//static void SetWritePage (unsigned long int pos,VideoMode *v);
  void Box(Coord2d Start,Coord2d End,Pixel col,VideoMode *v);
  void Line(Coord2d Start,Coord2d End,Pixel col,VideoMode *v);
  void SetAllPal(RGBdata *pal,VideoMode *v);
  void far GetAllPal(RGBdata *pal,VideoMode *v);
  void SetAllPalTo(RGBdata *pal,VideoMode *v);
  void Palette(int num,RGBdata pal,VideoMode *v);
  }

void main(void)
  {
  Pixel Data[500];
  Pixel DataIn[500];

  Vid=&Scrn;
  SetVESAMode(Vid);
  bioskey(0);
  Coord2d Pos(0,0);
/*
  for (;Pos.x<Vid->Size.x;Pos.x++)
    for (Pos.y=0;Pos.y<Vid->Size.y;Pos.y++)
      Point(Pos,Pos.x+Pos.y,Vid);
*/
/*  for (Pos.x=0;Pos.x<Vid->Size.x;Pos.x++)
    for (Pos.y=0;Pos.y<Vid->Size.y;Pos.y++)
      if (GetCol(Pos,Vid)!=((Pos.y+Pos.x)&255))
        {
        TextMode();
        printf("GetCol Incorrect at (%d,%d),%d",Pos.x,Pos.y,GetCol(Pos,Vid));
        return;
        }
*/
/*  Pos.x=100; Pos.y=100;
  for (int i=0;i<500;i++) Data[i]=1;
  GetCols(Pos,200,Data,Vid);
*/
/*  for (Pos.x=100;Pos.x<300;Pos.x++)
    {
    if (Data[Pos.x-100]!=((100+Pos.x)&255) )
      {
      TextMode();
      printf("GetCols Incorrect at (%d,%d) Should be:%d. is:%d. ",Pos.x,Pos.y,((Pos.y+Pos.x)&255),Data[Pos.x-100]);
      printf("%d %d %d %d %d",Data[0],Data[1],Data[2],Data[3],Data[4]);
      return;
      }
    }
*/

//  Point(MakeCoord(90,100),110,Vid);
  for (Pos.x=0,Pos.y=306;Pos.y<320;Pos.y++)
    Rep1Col(Pos,Pos.y,Pos.y,Vid);
//  Clear(6,Vid);
  bioskey(0);

  // RepCols over a page boundary.
  Pos.x=0; Pos.y=307;
  RepCols(Pos,306,Data,Vid);
  GetCols(Pos,306,DataIn,Vid);
  for (Pos.x=0;Pos.x<306;Pos.x++)
    {
    if (Data[Pos.x]!=DataIn[Pos.x])
      {
      TextMode();
      printf("GetCols Incorrect at (%d,%d) Should be:%d. is:%d. ",Pos.x,Pos.y,Data[Pos.x],DataIn[Pos.x]);
      printf("%d %d %d %d %d",Data[0],Data[1],Data[2],Data[3],Data[4]);
      return;
      }
    }

//  bioskey(0);
//  Clear(18,Vid);
  Coord2d zero(0,0);
/*  for (Pos.x=0;Pos.x<320;Pos.x+=20)
    for (Pos.y=0;Pos.y<200;Pos.y+=10)
      Line(zero,Pos,Pos.x+Pos.y,Vid);
  bioskey(0);
  for (Pos.x=0;Pos.x<320;Pos.x+=20)
    for (Pos.y=0;Pos.y<200;Pos.y+=20)
      BoxFill(Pos,Pos+20,Pos.x+(Pos.y/20),Vid);
*/
  bioskey(0);
  TextMode();
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