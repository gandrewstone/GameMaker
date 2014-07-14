// --------------------------------
//  Copyright 1994 G. Andrew Stone
//  Not public domain!
// --------------------------------

#include <bios.h>
#include <dos.h>
#include "gen.h"
#include "debug.hpp"
#include "fileclss.hpp"
#include "viddrive.hpp"
#include "fli.hpp"

#define TMODE 3
void TextMode(void)
  {
  union REGS r;
  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);
  }



void main(int argc,char *argv[])
  {
  FLI* f[20];
  FLIHeader *Head;

  int          ModeNum=4;
  char         Driver[51]="stlth64.drv";
  VideoDriver *Drvr;
  int FliCount=0;

  f[0] = new FLI;
  if (f[0]==NULL) { printf("NO memory!\n"); return; }
  printf("ARGC %d",argc);
  for (int i=1;i<argc;i++)
    {
    printf("File: %s\n",argv[i]);
    if (!f[FliCount]->File.Open(argv[i])) { printf("Cannot open %s.\n",argv[1]); }
    else
      {
      f[FliCount]->Pos.x=(FliCount%3)*340;
      f[FliCount]->Pos.y=(FliCount/3)*220;

      FliCount++;
      f[FliCount]=new FLI;
      if (f[FliCount]==NULL) break;
      }
    }
  delete f[FliCount];

  if ((Drvr=LoadVideoDriver(Driver))==NULL) { printf("Error Loading Driver.\n"); return; }


  for (i=0;i<FliCount;i++)
    {
    f[i]->Vid=Drvr->GetMode(ModeNum);
    f[i]->StepInit();
    }
  f[0]->Vid->SetMode(f[0]->Vid);

  while (!bioskey(1)) for (i=0;i<FliCount;i++) f[i]->Step();
  TextMode();
  }
