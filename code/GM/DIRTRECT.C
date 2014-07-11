//---------------------------------------------------------------------------//
// DirtRect.c                      5/24/93                       Andy Stone  //
// Dirty Rectangle screen write routines for playgame.                       //
//---------------------------------------------------------------------------//
#include <alloc.h>
#include <dos.h>
#include "gen.h"
#include "svga.h"
#include "gmgen.h"
#include "graph.h"
//#define SHOWDIRTYBOXES
#define MAXDIRTY  170

void SvgaBufScrnBox(int x,int y,int x1,int y1);
void SvgaBufToScrn(unsigned int addr,int len,int wid);


unsigned char *ScratchScrn=NULL;
unsigned int ScratchSeg=0;
extern unsigned int zeroaddon;


typedef struct
  {
  unsigned int offset;
  int len,wid;
  } BoxCoords;

static BoxCoords DirtyBoxes[MAXDIRTY];
int    LastDirty = 0;

void AddDirtyRect(int x,int y, int x1, int y1)
  {
  if (LastDirty<MAXDIRTY)
    {
    if (x>x1) swap (&x,&x1);
    if (y>y1) swap (&y,&y1);
    if (x<0) x=0;
    if (y<0) y=0;
    if (x1>=SIZEX) x1=SIZEX-1;
    if (y1>=SIZEY) y1=SIZEY-1;
    DirtyBoxes[LastDirty].offset=zeroaddon+x+(y*SIZEX);
    DirtyBoxes[LastDirty].len=x1-x+1;
    DirtyBoxes[LastDirty].wid=y1-y+1;
#ifdef SHOWDIRTYBOXES
    Box(x-1,y-1,x1+1,y1+1,102);
#endif
    LastDirty++;
    }
  }

void AddDirtyRect(unsigned int addr,int len, int wid)
  {
  if (LastDirty<MAXDIRTY)
    {
    DirtyBoxes[LastDirty].offset=addr;
    DirtyBoxes[LastDirty].len=len;
    DirtyBoxes[LastDirty].wid=wid;
    LastDirty++;
    }
  }

void DrawDirtyRects(void)
  {
  int l;
// static uchar col=0;

  for(l=0;l<LastDirty;l++)
    {
    SvgaBufToScrn(DirtyBoxes[l].offset,DirtyBoxes[l].len,DirtyBoxes[l].wid);
    }
//  col++;
  LastDirty=0;
  }

void ClearDirtyRects(void)
  {
  LastDirty=0;
  }


int InitDirty(void)
  {
  if (ScratchScrn==NULL)  // Otherwise it has already been inited!
    {
    if ((ScratchScrn=(uchar *) farmalloc(65570))==NULL)
      return(FALSE);             // In case we need to round up add 32

    ScratchSeg=FP_SEG(ScratchScrn);
    ScratchSeg += FP_OFF(ScratchScrn)>>4;   // Get seg as close as possible
                                          // to actual address.
    if ((FP_OFF(ScratchScrn)&15)>0) ScratchSeg++; // Round up.
    }
  return(TRUE);
  }

void UnInitDirty(void)
  {
  ScratchSeg=0;
  if (ScratchScrn!=NULL) { farfree(ScratchScrn); ScratchScrn=NULL; }
  }

void SvgaBufScrnBox(int x,int y,int x1,int y1)
  {
  uint l;
  uint addr;
  int len;

  if (x>x1) swap (&x,&x1);
  if (y>y1) swap (&y,&y1);
  len=x1-x+1;

  for(l=y,addr = x+(y*SIZEX)+zeroaddon;l<=y1;addr+=SIZEX,l++)
    SvgaBufToScrn(len,addr);
  }

void SvgaBufToScrn(unsigned int addr,int len,int wid)
  {
  uint l;
  for(l=0;l<wid;addr+=SIZEX,l++) SvgaBufToScrn(len,addr);
  }


