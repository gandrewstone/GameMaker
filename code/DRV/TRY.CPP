#include <alloc.h>
#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include "gen.h"

#include "Coord2d.hpp"

typedef uchar Pixel;

#include "VidDrive.hpp"

VideoMode *Vid;

void TextMode(void);

int ModeNum=0;

void main(int argc,char *argv[])
  {
  VideoDriver *Drvr;
  Pixel Data[1000];
  char Driver[51]="stlth64.drv";
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
  Vid->Clear(5,Vid);
  bioskey(0);
  Coord2d Pos(0,0);
  for (;Pos.x<Vid->Size.x;Pos.x++)
    for (Pos.y=0;Pos.y<Vid->Size.y;Pos.y++)
      Vid->Point(Pos,Pos.x+Pos.y,Vid);

  Pos.x=100; Pos.y=100;
  for (int i=0;i<1000;i++) Data[i]=1;
  Vid->GetCols(Pos,200,Data,Vid);

  Vid->Point(MakeCoord(90,100),110,Vid);
  Pos.x=0;
  for (Pos.y=0;Pos.y<Vid->Size.y;Pos.y++) Vid->Rep1Col(Pos,Pos.y,Pos.y,Vid);
  bioskey(0);
  Vid->Clear(6,Vid);
  Pos.x=10; Pos.y=100;
  Vid->RepCols(Pos,200,Data,Vid);
  bioskey(0);
  Vid->Clear(18,Vid);
  Vid->SetMode(Vid);
  Coord2d zero(0,0);
  for (Pos.x=0;Pos.x<Vid->Size.x;Pos.x+=20)
    for (Pos.y=0;Pos.y<Vid->Size.y;Pos.y+=10)
      Vid->Line(zero,Pos,Pos.x+Pos.y,Vid);
  bioskey(0);
  for (Pos.x=0;Pos.x<Vid->Size.x;Pos.x+=20)
    for (Pos.y=0;Pos.y<Vid->Size.y;Pos.y+=20)
      Vid->BoxFill(Pos,Pos+20,Pos.x+(Pos.y/20),Vid);

  ulongi StAddr = 0;
  Pos.Set(0,0);
  while(bioskey(1)==0)
    {
    Vid->SetViewPos(StAddr,Vid);
    StAddr += Vid->Size.x/4;
//    StAddr &= 0x7FFF;
//    while ( (inportb(0x3da)&8)==8)
      {
      Vid->Line(zero,Pos,Pos.x+Pos.y,Vid);
      Pos.x+=20;
      if (Pos.x>Vid->Size.x) { Pos.x = 0; Pos.y+=10; }
      if (Pos.y>Vid->Size.y) Pos.y=0;
      }
    while ( (inportb(0x3da)&8)==0);
    }
  bioskey(0);
  for (Pos.x=0;Pos.x<Vid->Size.x;Pos.x+=20)
    for (Pos.y=0;Pos.y<Vid->Size.y;Pos.y+=10)
      Vid->Line(zero,Pos,Pos.x+Pos.y,Vid);
  Vid->SetViewPos(0x0000FFFFL/4,Vid);
  bioskey(0);
  Vid->Rep1Col(Pt(0,Vid->Size.y),Vid->Size.y*Vid->Size.x,1,Vid);
  bioskey(0);
  for (Pos.x=0;Pos.x<Vid->Size.x;Pos.x+=20)
    for (Pos.y=Vid->Size.y;Pos.y<Vid->Size.y*2;Pos.y+=10)
      Vid->Line(zero,Pos,Pos.x+Pos.y,Vid);
  bioskey(0);


  TextMode();
  }

