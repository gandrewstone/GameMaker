#include <alloc.h>
#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>

#define UPKEY     72
#define DOWNKEY   80
#define RIGHTKEY  77
#define LEFTKEY   75

#include "gen.h"
#include "Coord2d.hpp"
typedef uchar Pixel;
#include "VidDrive.hpp"
#include "d:\gmcd\bloc.h"
#include "d:\gmcd\map.h"

VideoMode *Vid;

static int loadspecblk(const char *file,int numblks,blkstruct *bl);
static int loadspecany(const char *file,unsigned int bytes, char *addr);
static int loadspecmap(const char *file);


int              ModeNum = 2000;
blkstruct       *blks    = NULL;   // addresses - Backgnd[0],mon[1],char[2]
monmapstruct     mm        [LASTMM];
mapstruct far    map       [MLEN][MWID];
RGBdata          cols      [256];
Coord2d          ScrnBlkLen;

void PrintModeInfo(VideoDriver *drvr)
  {
  VideoMode *v;
//  printf("%s:%s V%d %s\n",drvr->Object,drvr->VGAType,drvr->Version,drvr->Date);
  for (int i=0;i<=drvr->NumModes;i++)
    {
    v=drvr->GetMode(i);
    printf("%2d: %dx%d ColorDepth:%d BIOS:%x\n",i,v->Size.x,v->Size.y,v->ColDepth,v->BIOSMode);
    }
  }

static void DrawBlk(Coord2d Pos,uchar *blk)
  {
  int End = Pos.y + BLEN;
  for (;Pos.y<End;Pos.y++,blk+=BLEN) Vid->RepCols(Pos,BLEN,blk,Vid);
  }

static void DrawVert(Coord2d ScrnPos,Coord2d MapPos)
  {
  if (MapPos.x>=MLEN) MapPos.x-=MLEN;
  if (MapPos.x<0)     MapPos.x+=MLEN;

  for (int i=0;i<ScrnBlkLen.y+2;i++,MapPos.y++,ScrnPos.y+=BLEN)
    {
    if (MapPos.y>=MWID) MapPos.y-=MWID;
    DrawBlk(ScrnPos,(uchar*)blks[map[MapPos.y][MapPos.x].blk].p);
    }
  }

static void DrawHoriz(Coord2d ScrnPos,Coord2d MapPos)
  {
  if (MapPos.y>=MLEN) MapPos.y-=MWID;
  if (MapPos.y<0)     MapPos.y+=MWID;

  for (int i=0;i<ScrnBlkLen.x;i++,MapPos.x++,ScrnPos.x+=BLEN)
    {
    if (MapPos.x>=MLEN) MapPos.x-=MLEN;
    DrawBlk(ScrnPos,(uchar*)blks[map[MapPos.y][MapPos.x].blk].p);
    }
  }


static void DrawMap(Coord2d MapStPos)
  {
  Coord2d mp;

  for (mp.x=0;mp.x<ScrnBlkLen.x;mp.x++)
    for (mp.y=0;mp.y<ScrnBlkLen.y+1;mp.y++)
      DrawBlk(mp*BLEN,(uchar*)blks[map[(mp.y+MapStPos.y)%100][(mp.x+MapStPos.x)%100].blk].p);
  }

extern SetStart(ulongi StAddr);

static void ScrollIt(Coord2d StartPos)
  {
  Coord2d PixSpd(0,0);
  Coord2d PixPos    = StartPos*BLEN;
  Coord2d MapPos    = StartPos;
  boolean done      = False;
  ulongi  StAddr    = 0L;
  ulongi  OldStAddr = 0L;
  int     pause     = 1;

  while(!done)
    {
    PixPos      += PixSpd;
    StAddr      += PixSpd.x + (PixSpd.y*Vid->Size.x);

    uint inkey;
    if ( (inkey=bioskey(pause))>0)
      {
      if (pause) bioskey(0);
      switch(inkey&255)
        {
        case 'p':
        case 'P':
          pause ^= 1;
          break;
        case 13:
          printf("SPD:(%d,%d) ADDR:(%lu) ",PixSpd.x,PixSpd.y,StAddr);
          break;
        case 0:
          switch(inkey>>8)
            {
            case UPKEY:
              if (PixSpd.y > -BLEN) PixSpd.y--;
              break;
            case DOWNKEY:
              if (PixSpd.y < BLEN)  PixSpd.y++;
              break;
            case RIGHTKEY:
              if (PixSpd.x < BLEN)  PixSpd.x++;
              break;
            case LEFTKEY:
              if (PixSpd.x > -BLEN) PixSpd.x--;
              break;
            }
          break;
        case 27:
          done=True;
          break;
        }
      }

    Vid->SetViewPos(StAddr,Vid);
    if (PixPos.x>=MLEN*BLEN)
      {
      MapPos.x -= MLEN;
      PixPos.x -= MLEN*BLEN;
      }
    if (PixPos.y>=MWID*BLEN)
      {
      MapPos.y -= MWID;
      PixPos.y -= MWID*BLEN;
      }
    if (PixPos.x<0)
      {
      MapPos.x += MWID;
      PixPos.x += MLEN*BLEN;
      }
    if (PixPos.y<0)
      {
      MapPos.y += MWID;
      PixPos.y += MWID*BLEN;
      }

    Coord2d NewMapPos = PixPos/BLEN;

    while (NewMapPos.y>MapPos.y)
      {
      MapPos.y++;
      DrawHoriz(Pt(-((PixPos.x%BLEN)&0xFFFC),Vid->Size.y-(PixPos.y%BLEN)),Pt(NewMapPos.x,MapPos.y+ScrnBlkLen.y));
      }

    while (NewMapPos.y<MapPos.y)
      {
      MapPos.y--;
      DrawHoriz(Pt(-((PixPos.x%BLEN)&0xFFFC),-(PixPos.y%BLEN)),Pt(NewMapPos.x,MapPos.y));
      }

    while (NewMapPos.x>MapPos.x)
      {
      MapPos.x++;
      DrawVert(Pt(Vid->Size.x-BLEN-((PixPos.x%BLEN)&0xFFFC),-BLEN-(PixPos.y%BLEN)),Pt(MapPos.x+ScrnBlkLen.x-1,NewMapPos.y-1));
      }

    while (NewMapPos.x<MapPos.x)
      {
      MapPos.x--;
      DrawVert(Pt(Vid->Size.x-((PixPos.x%BLEN)&0xFFFC),-BLEN-(PixPos.y%BLEN)),Pt(MapPos.x,NewMapPos.y-1));
      }
    }
  }



int main(int argc,char *argv[])
  {
  VideoDriver *Drvr;
  Pixel Data[1000];
  char Driver[81]="tseng4k.drv";
/*  if (argc>2)
    {
    ModeNum=argv[2][0]-'0';
    printf("Using Driver Mode #%d\n",ModeNum);
    if ((bioskey(0)&255)==27) { printf("User Abort!\n"); return(0); }
    }
*/
  if (argc>1) strcpy(Driver,argv[1]);

  blks = new blkstruct [BACKBL+1];
  if (blks==NULL) {printf("Out of memory!\n"); return(1); }

  loadspecany("test.pal",sizeof(RGBdata)*256,(char *) &cols);
  loadspecblk("test.bbl",BACKBL,blks);
  loadspecmap("test.map");

  if (!(Drvr=LoadVideoDriver(Driver))) { printf("Error Loading Driver %s.\n",Driver); return(1); }
  if (!Drvr->Validate(110)) { printf("Driver too old!\n"); return(1); }

  printf("Scroller V1.00    Copyright 1995 Andrew Stone\n\n  Use the arrow keys to add speed, p toggles pause mode, hit ESC to quit.\n  Command Line: scroller [videodriver.drv]\n\n");

  printf("\nOllie: Please test each of these modes to see if the scrolling works.\n       You'll know it doesn't work if GameMaker style scrolling bugs occur.\nie:    1. Does going up right away work?\n       2. Does the screen ever 'snap'?\n       3. Are things not painted or painted where they shouldn't be?\n       4. Is the picture at certain scrolling speeds fuzzy or vibrating?\n\n");

  printf("Using Video Driver: %s\n\n",Driver);
  do{
    PrintModeInfo(Drvr);
    printf("\n Choose a Mode (0-%d):",Drvr->NumModes);
    scanf("%d",&ModeNum);
    } while((ModeNum>Drvr->NumModes)||(ModeNum<0));


  Vid=Drvr->GetMode(ModeNum);

  Vid->SetMode(Vid);
  Vid->SetAllPal(cols,Vid);
  Vid->SetViewPos(0L,Vid);

  ScrnBlkLen = Vid->Size/BLEN;

  Coord2d Pos(0,0);
  Coord2d MapPos(50,50);

  DrawMap(MapPos);
  ScrollIt(MapPos);

  TextMode();
  return(2);
  }

int loadspecany(const char *file,unsigned int bytes, char *addr)
  {
  FILE *fp;
  char result;
  char srchfor[6];
  
  if ( (fp=fopen(file,"rb")) ==NULL) return(FALSE);
  fread(addr,bytes,1,fp);
  fclose(fp);
  return(TRUE);
  }

static int loadspecblk(const char *file,int numblks,blkstruct *bl)
  {
  FILE *fp;
  if (bl!=NULL)
    {
    if ((fp=fopen(file,"rb"))==NULL) return(FALSE);
    fread((uchar *)bl,sizeof(blkstruct),numblks,fp);
    fclose(fp);
    return(TRUE);
    }
  return(False);
  }

static int loadspecmap(const char *file)
  {
  FILE *fp;
  if  ((fp=fopen(file,"rb"))==NULL) return(FALSE);
  fread( (char*) &map,sizeof(mapstruct),MLEN*MWID,fp);
  fread( (char*) &mm,sizeof(monmapstruct),LASTMM,fp); 
  fclose(fp);
  return(TRUE);
  }


