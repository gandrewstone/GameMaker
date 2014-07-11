#include<process.h>
#include<stdio.h>
#include<dos.h>
#include<bios.h>
#include<alloc.h>
#include<stdlib.h>
#include"gen.h"
#include"pal.h"
#include"mousefn.h"
#include"gif.h"

RGBdata colors[256];


void main(void)
  {

  int bgcol;
  FILE *fipo;
  if ((fipo=fopen("d:\\gmpro\\gmtitle.gif","rb"))==NULL)
     { printf("File Not Found!\n"); return; }

  GraphMode();
//  rewind(fipo);
  if ( (bgcol=intro(fipo,colors)) == -1) { fclose(fipo); return; }
  else
    {
    GraphMode();

    SetAllPal(colors);
//    setscrn(bgcol);
    decode(fipo,1);
//    FadeTo(colors);
   // redo=getthem();    // this returns a 1 if returning from help,etc.
    }                  // this was it acts as a redraw screen.
  fclose(fipo);
  TextMode();
  }
