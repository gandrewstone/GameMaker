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
#include "xmsvid.hpp"

XMS XMSDriverInfo;

VideoMode   *Vid;
XMSVideoMode XMSvideo;

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

  if ((Drvr=LoadVideoDriver(Driver))==0) { printf("Error Loading Driver.\n"); return; }
  Vid=Drvr->GetMode(ModeNum);

  XMSvideo.Size.x = 320;
  XMSvideo.Size.y = 200;
  XMSvideo.LineSep= 320;
  XMSVideoInit(&XMSvideo);
  bioskey(0);

  XMSvideo.SetMode(&XMSvideo);
//  Vid->SetMode(Vid);
  Coord2d Pos(0,0);
  for (Pos.y=0;Pos.y<XMSvideo.Size.y;Pos.y++)
    {
//  for (Pos.x=0;Pos.x<XMSvideo.Size.x;Pos.x++)
      {
      XMSvideo.Point(Pos,Pos.x+Pos.y,&XMSvideo);
  //    Vid->Point(Pos,XMSvideo.GetCol(Pos,&XMSvideo),Vid);
      XMSvideo.GetCol(Pos,&XMSvideo);
      }
    }
  Vid->Clear(5,Vid);

  Pos.x=0;Pos.y=0;
  bioskey(0);
  XMSvideo.GetCols(Pos,0x8FFE,(Pixel*)0xA0000000,&XMSvideo);
  Pos.x=0x8FFE;
  XMSvideo.GetCols(Pos,0x1,(Pixel*)0xA0008FFF,&XMSvideo);

  bioskey(0);
  XMSVideoUnInit(&XMSvideo);
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