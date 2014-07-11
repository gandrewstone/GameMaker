/*------------------------------------------------------------*/
/*   MAPMAKER.C                                               */
/*   Map Editor that uses the 20x20 blocks made with          */
/*      blocedit.c to make a map.                             */
/*   Used to make video games        V1.05                    */
/*   Programmer: Andy Stone          Date :10/11/90           */
/*   Last Edited: 6/16/92                                     */
/*                                                            */
/*------------------------------------------------------------*/

#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <conio.h>
#include <alloc.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include "gen.h"
#include "scrnrout.h"
#include "mousefn.h"
#include "jstick.h"
#include "gmgen.h"
#include "windio.h"
#include "palette.h"
#include "bloc.h"
#include "mon.h"
#include "map.h"
#include "graph.h"
#include "facelift.h"
#include "tranmous.hpp"

extern "C" int drawcbloc(int x,int y,unsigned char *bloc);

#define TRANSCOL 255     /* The value that is transparent in _pic fns  */
#define BACKBLK    0     /*  Background block number                 */
#define LMLEN      8
#define LMX      283     /*  Where the vert. monster list is on the  */
#define LMY       20     /* scrn. (short for LIST_MONSTER_Y)         */

#define MENUX        (LMX-39)
#define MENUY        165
#define BLOCY        (MENUY-48)
#define MONSTY       (MENUY-36)
#define ERASEY       (MENUY-24)
#define HELPY        (MENUY-12)
#define TOOL2Y       23
#define TOOLX        MENUX
#define TOOLY        (TOOL2Y-1)

#define MX           0               //  Upper left coords of map on screen
#define MY           0               // DIFFERENT FROM lowercase mx!
#define MX1          (MX+(16*BLEN))
#define MY1          (MY+(10*BLEN))

#define MAXFNBUTS    9
#define BUTX         (LMX-34)
#define BUTY         38
#define BUTY1        (BUTY+(11*8/2))
#define ZOOMX        (BUTX+8)
#define ZOOMY        (BUTY1+4+11)
#define POINTFN      0
#define LINEFN       1
#define BOXFN        2
#define BOXFILLFN    3
#define CIRCLEFN     4
#define CIRCLEFILLFN 5
#define FILLFN       6
#define COPYFN       7
#define ZOOMFN       8

#define BLOCACTIVE   1
#define MONACTIVE    2
#define NOTOOLS      4
// =---------------------------Function Declarations--------------------------=
static int retblock(int);
static int retmon(int);

static void drawmonsinmap(void);
static int  findopenmm(void);
static int  animons(void);               // Advances monster frames when ready.
static int  RetHighestNum(int x,int y,int z);

static void drawmap(void);
static void movemap(void);
static void drawthelist(void);
static void DrawvArrows(int x,int y,int blnum,char selected,unsigned char *Cols);
static void drawcompass(char dir);
static void redrawall(void);
static void UnZoomBox(int x,int y, unsigned char col);

static void initmap(void);
static void initalldata(void);
static void changemap(int x, int y,int chto=BACKBL+10);

static int edit(void);
static void DoEndDrag(void);
static void cleanup(void);

static void FindFBCols(void);
static void DrawButtons(void);
static void DrawFnBut(int butnum,int x,int y, unsigned char col[4]);

static void monstb(int x,int y,unsigned char *col);
static void lightb(int x,int y,char onoff);
static void blocb(int x,int y,unsigned char *col);
static void eraseb(int x,int y, unsigned char col[4]);
static void toolb(int x,int y, unsigned char col[4]);
static void rightb(int x,int y, unsigned char col[4]);
static void downb(int x,int y, unsigned char col[4]);
static void leftb(int x,int y, unsigned char col[4]);
static void upb(int x,int y, unsigned char col[4]);

static void FindBlkCols(void);
static void ScrnToMap(int x, int y, int *retx, int *rety);
static void MapToScrn(int x, int y, int *retx, int *rety); 
 
static void HiFill(int x, int y);
static void FillIter(int x,int y,int dir);

static void RestoreIfOld(int x, int y);
static void DrawOnlyIfNew(int x, int y);
static void UpdateShape(int x,int y,int prevx, int prevy);
static void CopyFnPt(int x,int y);
static void CopyFnErase(int x,int y);

static void HiBoxFill(int x, int y, int x1, int y1, void Point(int x,int y));
static void HiCircleFill(int centx, int centy, int x1, int y1, void point(int x,int y));
static void HiCircle(int centx, int centy, int x1, int y1, void point(int x,int y));
static void HiLine(int x1,int y1,int x2,int y2, void point(int x,int y));
static void HiBox(int x, int y, int x1, int y1, void point(int x,int y));

int sqrt(int x);

//=-----------------------------Variables------------------------------------=

extern unsigned _stklen=10000U;  // Set the stack to 10000 bytes, not 4096

static RGBdata         colors[256];
static blkstruct       *blks[2];        // addresses - Backgnd[0],mon[1]
static monstruct       mon[LASTMON];
static unsigned char   BlkCols[BACKBL];         

static unsigned char   *SaveMap=NULL;
mapstruct              map[MWID][MLEN];
monmapstruct           mm[LASTMM];

static unsigned char   EditFn=0;      // Which Edit Fn currently active
static unsigned char   FBCols[3][4];  // The colors to draw the tools in
extern unsigned char   FaceCols[MAXFACECOL];      //Screen colors
extern char            *FontPtr;
extern unsigned char   HiCols[4];
static unsigned char   OffCols[4];
static int             Firstx,Firsty;      // On drag fns the first point.
static int             Copyx=1,Copyy=1,CopyLen=1,CopyWid=1;
static unsigned char far *CopyData=NULL;
static unsigned int    CopyWhere=0;
static char            CopyPicked=FALSE;   // Whether box has been set or not
static char            DoCopy=FALSE;
static unsigned char   FillOver=0, FillCol=0;

static int             mx=0,my=0,sely=0;
static int             mlist=0,blist=0;
static int             zoom;
static int             status=BLOCACTIVE;
static char            saved=1;
static unsigned char   OldId=1,CurId=2;

extern blkstruct       *blk;         
extern int             lastbl;
extern char            mapcurfile[41];

/*=-----------------------------------Code                 */

void main(int argc,char *argv[])
  {
  int attr=0;
  char w[1100];
  int choice=1;

#ifdef DEBUG
  printf("Initialization finished.\n");
  NewAbortMSG("GM:ATE in main procedure.\n");
#endif
#ifdef CHKREG
  if ((argc < 2) || (strcmp(argv[1],OKMSG)!=0)) RestorePal=True;
#endif
  /*--------------------Set up timer------------------------------   */
  /* Important to do right before atexit, 'cause cleanup sets it back */
  /* (And if we were to quit before, then Oldtimer would not be set  */

  atexit(cleanup);
  FontPtr=GetROMFont();

  /*--Get memory for block file */
  SaveMap= (unsigned char *) malloc(sizeof(char)*MLEN*MWID*2);
  blks[0]= (blkstruct *) malloc(sizeof(blkstruct)*(BACKBL+1));
  if (blks[0]==NULL) { errorbox("NOT ENOUGH MEMORY!","  (Q)uit"); exit(menu);}
  blks[1]= (blkstruct *) malloc(sizeof(blkstruct)*(MONBL+1));
  if (blks[1]==NULL) { errorbox("NOT ENOUGH MEMORY!","  (Q)uit"); exit(menu);}

  initalldata();

  /* Set up colors as default.pal, and then the hardware palette if fails */
  if (!loadspecany("default",".pal",WorkDir,sizeof(RGBdata)*256,(char *) colors))
    InitStartCols(colors);

  FindFBCols();

  /*--Quit if no mouse */
  if (initmouse()==FALSE)
    { errorbox("This application requires a Microsoft","compatible mouse! (Q)uit");
      exit(quit); }

  do
    {
    SetTextColors();
    attr = openmenu(30,3,26,15,w);
    moucur(FALSE);
    writestr(0,0,79,GMTOPBAR);
    writestr(0,1,31,"  Palette  Block  Monster  Map  Character  Image  Sound  Integrator  Main  Help ");
    writestr(30,3,PGMTITLECOL,"        MAP  MAKER        ");
    writestr(30,4,PGMTITLECOL,"       Version "GMVER"       ");
    writestr(30,5,PGMTITLECOL,"     By Gregory Stone     ");
    writestr(30,6,PGMTITLECOL,"    Copyright (C) 1994    ");
    writestr(31,8,attr+14, "Choose a Map");
    writestr(31,9,attr+14, "New map");
    writestr(31,10,attr+14,"Choose a Block Set ");
    writestr(31,11,attr+14,"Choose a Monster Set ");
    writestr(31,12,attr+14,"Choose Monster Block Set");
    writestr(31,13,attr+14,"Choose a Palette");
    writestr(31,14,attr+14,"Edit Chosen Map");
    writestr(31,15,attr+14,"Save Map");
    writestr(31,16,attr+14,"Delete a Map");
    writestr(31,17,attr+14,"Quit");
    choice=vertmenu(30,8,26,10,choice);
    closemenu(30,3,26,15,w);
    switch(choice)
      {
      case 1:
        if (!saved)
          {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice=8; break; }
          }
        if (loadmap())
          {
          DoCopy=FALSE;
          mx=my=0;      /* Set map back to 0,0 pos */
          choice = 3;   /* Position vert bar on load blocks */ 
          saved=1;      /* Nothing has changed since loading */
          }
        break;
      case 2:
        if (!saved) 
          {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice=8; break; }
          }
        initmap();
        DoCopy=FALSE;
        choice=3; saved=1;
      	errorbox("New map created!","(C)ontinue",2);
        break;
      case 3:
        if (loadblks(1,(char *)blks[0])) FindBlkCols();
        choice++;
        break;
      case 4:
        loadany("Enter monster set to load: ","*.mon\0",WorkDir,sizeof(monstruct)*LASTMON,(char *)&mon,0);
        choice++;
        break;
      case 5:
        loadblks(2,(char *)blks[1]);
        choice++;
        break;
      case 6:
        if (loadany("Enter the palette file to load:","*.pal\0",WorkDir,sizeof(RGBdata)*256,(char*)&colors,0))
          {
          FindFBCols();
          FindBlkCols();
          }
        choice++;
        break;
      case 7:
        edit(); 
        TextMode();
        choice++;
        break;  
      case 8:
        if (savemap()) { choice=10; saved=1; }
        else choice=7;
        mouclearbut();
        while (bioskey(1)) bioskey(0);
        break;    
      case 9:
        delany("Enter map to delete: ","*.map\0",WorkDir);
        break;
      case 0:
      case 10:                                       // Quit
        if (!saved) {
          choice = (errorbox("Unsaved changes--Really Quit?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice=8; break; }
         }
        choice=11;
        break;
      }
    } while (choice != 11);
  exit(menu);
  }

static void cleanup(void)
  {
  farfree(blks[0]);
  farfree(blks[1]);
  if (CopyData!=NULL) farfree(CopyData);
  Time.TurnOff();
  }

static int edit(void)
  {
  int x,y,buts,oldx=-1,oldy=-1,oldbuts=-1;
  int lastcmd=0;
  int changed,donething;
  int inkey;
  
  mouclearbut();
  redrawall();

  while(TRUE)
    {
    changed = FALSE;
    do
      {
      if (bioskey(1)) changed = TRUE;
      moustats(&x,&y,&buts);
      if ((oldbuts>0)&&(buts==0))
        DoEndDrag();
      if (buts>0) changed = TRUE;
      else { oldbuts=0; lastcmd=0; }
      if (animons()) 
        {
        blk=blks[1];
        if (status==MONACTIVE)
          {
          if (x>LMX) moucur(0);
          vdrawlist(LMX,LMY,LMLEN,LASTMON,&mlist,retmon);
          if (x>LMX) moucur(1);
          }
        drawmonsinmap();
        blk=blks[0];
        }
      } while(!changed);

    changed=FALSE;     
    if (buts>0)
      {
      donething=FALSE;
                             // TOGGLE MONSTER LIST 
      if (status<NOTOOLS)
        {
        if (x>=LMX-43) donething=TRUE;
        if ( (x>MENUX)&&(x<=MENUX+36) )
          {
          if ( (y<=MONSTY+10)&&(y>=MONSTY)&&(status!=MONACTIVE)&&(lastcmd!=2) )
            {
            status=MONACTIVE;
            EditFn=POINTFN;
            DoCopy=FALSE;
            moucur(FALSE);
            drawthelist();
            DrawButtons();
            if (zoom) eraseb(MENUX,ERASEY,FaceCols);
            if (sely==BACKBL+1) { sely=0; lightb(LMX+BLEN+2,LMY,1); }
            moucur(TRUE);
            lastcmd=2; oldy=y;          // so one toggle per button press
            }
      
                          // TOGGLE BLOCK LIST

          if ( (y<=BLOCY+10)&&(y>=BLOCY)&&(lastcmd!=1)&&(status!=BLOCACTIVE) )
            {
            status=BLOCACTIVE;
            moucur(FALSE);
            drawthelist();
            eraseb(MENUX,ERASEY+1,OffCols);
            if (sely==BACKBL) { sely=0; lightb(LMX+BLEN+2,LMY,1); }
            moucur(TRUE);
            lastcmd=1; oldy=y;        // So it toggles only once per button press
            }

          if ( (y<=TOOL2Y+10)&&(y>=TOOL2Y)&&(zoom) )
            { status|=NOTOOLS;  drawmap(); mouclearbut(); }

          if ( (y>=ERASEY)&&(y<=ERASEY+10)&&(status==MONACTIVE)
             &&(zoom)&&(lastcmd!=3) )
            {
            lastcmd=3;
            moucur(FALSE);
            if (sely<BACKBL)
              {
              lightb(LMX+BLEN+2,sely*BLEN+LMY,0);
              eraseb(MENUX,ERASEY+1,HiCols);
              sely=BACKBL;
              }
            else
              {
              lightb(LMX+BLEN+2,LMY,1);
              eraseb(MENUX,ERASEY,FaceCols);
              sely=0;
              }
            moucur(TRUE);
            oldx=x; oldy=y;
            }

          if ( (y>MENUY)&&(y<MENUY+9) )  // Menu button hit
            return (TRUE);
          if ( (y>=HELPY)&&(y<HELPY+9) ) // Help button hit
            {
            TextMode();
            DisplayHelpFile("map.hlp");
            redrawall();
            mouclearbut();
            }
          }

                 // LIST - either Scroll list, or choose a block
  
        if ( (x>=LMX)&&(x< LMX+BLEN+10) )
          {
          if ( (y>=LMY)&&(y<LMY+(BLEN*LMLEN))&&(sely!=(y-LMY)/BLEN) )
            {                                                 // SELECT a block
            moucur(FALSE);
            if (sely<BACKBL) lightb(LMX+BLEN+2,sely*BLEN+LMY,0);
            else if (status==MONACTIVE) eraseb(MENUX,ERASEY,FaceCols);
            sely=(y-LMY)/BLEN;
            lightb(LMX+BLEN+2,sely*BLEN+LMY,1);
            moucur(TRUE);
            }
          if ((y<LMY-3)&&(y>=LMY-8))                   // SCROLL Blocks down
            {
            if (status==BLOCACTIVE)
              {
              blk=blks[0];
              moucur(FALSE);
              DrawvArrows(LMX,LMY,LMLEN,1,FaceCols);
              vscroll_list(2,LMX,LMY,LMLEN,BACKBL,&blist,retblock); 
              DrawvArrows(LMX,LMY,LMLEN,0,FaceCols);
              moucur(TRUE);
              }
            else
              {
              blk=blks[1]; 
              moucur(FALSE);
              DrawvArrows(LMX,LMY,LMLEN,1,FaceCols);
              vscroll_list(2,LMX,LMY,LMLEN,LASTMON,&mlist,retmon); 
              DrawvArrows(LMX,LMY,LMLEN,0,FaceCols);
              moucur(TRUE);
              blk=blks[0]; 
              }
            }
                                                       // SCROLL blocks UP 
          if ( (y>LMY+(BLEN*LMLEN)+2)&&(y<=LMY+(BLEN*LMLEN)+10))  
            { 
            if (status==BLOCACTIVE)
              {
              blk=blks[0];
              moucur(FALSE);
              DrawvArrows(LMX,LMY,LMLEN,2,FaceCols);
              vscroll_list(-2,LMX,LMY,LMLEN,BACKBL,&blist,retblock); 
              DrawvArrows(LMX,LMY,LMLEN,0,FaceCols);
              moucur(TRUE);
              }
            else
              {
              blk=blks[1];
              moucur(FALSE);
              DrawvArrows(LMX,LMY,LMLEN,2,FaceCols);
              vscroll_list(-2,LMX,LMY,LMLEN,LASTMON,&mlist,retmon); 
              DrawvArrows(LMX,LMY,LMLEN,0,FaceCols);
              moucur(TRUE);
              blk=blks[0]; 
              }
            }
          oldx=x; oldy=y;
          }

                // FUNCTION BUTTON  - choose a new Fn button

        if ( (x>=BUTX)&&(x<BUTX+30)&&(y>=BUTY)&&(y<=BUTY1)
           &&(status==BLOCACTIVE) )
          {
          if      ( (x>BUTX) &&  (x<BUTX+9) ) { y -= BUTY; y /= 11; }
          else if ( (x>BUTX+16)&&(x<BUTX+25) ) { y -= BUTY; y /= 11; y += 4; }
          else y=EditFn;                          //  Do nothing (kludge!)
          if ((y!=EditFn)&&(y!=oldy))
            {
            moucur(FALSE);
            if ((y!=COPYFN)||(DoCopy==FALSE)) 
              {
              EditFn=y;
              DrawButtons();
              if ( (y==COPYFN)&&(sely<BACKBL) )   // Select the pattern copy button.
                {
                lightb(LMX+BLEN+2,sely*BLEN+LMY,0);
                sely=BACKBL+1;
                }
              }
            else                     // Unselect the Pattern copy button
              {
              DoCopy=FALSE;
              DrawFnBut(COPYFN,BUTX+16,BUTY+33,FBCols[0]);
              if (sely==BACKBL+1) sely=0;
              lightb(LMX+BLEN+2,sely*BLEN+LMY,1);
              }
            moucur(TRUE);
            }
          oldy=y;
          }

        if ( (x>=ZOOMX)&&(x<ZOOMX+10)&&(y>=ZOOMY)&&(y<ZOOMY+10)&&(lastcmd!=5) )
          {
          lastcmd=5;
          if (zoom) { zoom=FALSE; if (sely==BACKBL) sely=0; drawmap(); }
          else if (EditFn==ZOOMFN) { EditFn=0; moucur(FALSE); DrawButtons(); moucur(TRUE); }
          else
            {
            moucur(FALSE);
            DrawFnBut(ZOOMFN,ZOOMX,ZOOMY+1,FBCols[2]);
            EditFn=ZOOMFN;
            moucur(TRUE);
            }
          oldy=y;
          }
        // COMPASS - scroll map        if zoomed in
        if (zoom)
          {
          static unsigned int Rate=0;
          static unsigned int ScrollCnt=0;
          static unsigned long int OldClock=0;
          int OldChange;
          changed=0;

          do {
            OldChange=changed;
            changed&=0x01;
            if ( (y>=ZOOMY-7) && (y<ZOOMY) && (x>=ZOOMX) && (x<ZOOMX+10) )
              {                                     // scroll up
              ScrollCnt++;
              my -=1;
              if (my<0) my = MWID-1;
              changed=3;
              }
            else if ( (y>=ZOOMY+13)&&(y<ZOOMY+19)&&(x>=ZOOMX)&&(x<ZOOMX+10)  )
              {
              ScrollCnt++;
              my +=1;
              if (my>=MWID) my = 0;
              changed=9;
              }
            else if ( (x>=ZOOMX+12)&&(x<ZOOMX+18)&&(y>=ZOOMY)&&(y<ZOOMY+10)  )
              {
              ScrollCnt++;
              mx +=1;
              if (mx>=MLEN) mx = 0;
              changed=5;
              }
            else if ( (x>=ZOOMX-8)&&(x<ZOOMX-2)&&(y>=ZOOMY)&&(y<ZOOMY+10)  )
              {
              ScrollCnt++;
              mx -=1;
              if (mx<0) mx = MLEN-1;
              changed=17;
              }
            if (changed>1)
              {
              if (changed!=OldChange)   // Push in the button
                {
                moucur(FALSE);
                drawcompass((changed>>1));
                moucur(TRUE);
                }
              movemap();
              if (Clock-OldClock<3)    // adjust the scroll speed
                { if (ScrollCnt>5) Rate++; }
              else
                {
                ScrollCnt=0;
                OldClock=Clock;
                }
              delay(Rate);
              }
            moustats(&x,&y,&buts);
            } while ( (changed>1)&&(buts>0) );
          if (changed>0)
            {
            changed=0;
            drawmap();
            }
          }
        }
      else if ( (oldbuts==0)&&(x>TOOLX)&&(x<TOOLX+36)&&(y>TOOLY)&&(y<TOOLY+10) )
        {
        status&=(~NOTOOLS); donething=TRUE; drawmap(); mouclearbut();
        }
                               // DRAW OBJECT

      if ( (!donething) && (x>=MX)&&(x<MX1)&&(y>=MY)&&(y<MY1) )
        {                               // Is the cursor in the Map area?
        if ((oldbuts==0)&&(buts>0))     // First point
          {
          ScrnToMap(x,y,&Firstx,&Firsty);
          if ((oldx!=Firstx)||(oldy!=Firsty))
            {
            if (EditFn==ZOOMFN) 
              { 
              zoom=TRUE;
              mx=Firstx-7;
              my=Firsty-4;
              if (mx<0) mx+=MLEN;
              if (my<0) my+=MWID;
              EditFn=0;
              drawmap();
              oldbuts=buts; 
              }
            else if (EditFn==POINTFN) { moucur(0); changemap(Firstx,Firsty); moucur(1); }
            else if (EditFn==FILLFN) HiFill(Firstx,Firsty);
            else
              {
              oldbuts=buts;
              if (status < NOTOOLS)
                {
                if (zoom) moucurbox(MX,MY,LMX-44,MY1);
                else moucurbox(MX,MY,MX+MLEN*2-1,MY+MWID*2-1);
                }
              }
            oldx=Firstx; oldy=Firsty;
            }
          }
        else if ((oldbuts>0)&&(buts>0))           // Drag occuring
          {
          ScrnToMap(x,y,&x,&y);
          if ((x!=oldx)||(y!=oldy)) UpdateShape(x,y,oldx,oldy);
          oldbuts=buts;
          oldx=x; oldy=y;
          }
        }
      }

                                  // KEY HANDLER
    if (bioskey(1))
      {
      inkey = bioskey(0);
      switch (inkey&255)
        {
        case 0:
          changed=FALSE;
          switch(inkey>>8)
            {
            case 72: /* Up arrow */
              my -=1;
              if (my<0) my = MWID-1;
              changed=TRUE;
              break;
            case 80: /* Down arrow */
              my +=1;
              if (my>=MWID) my = 0;
              changed=TRUE;
              break;
            case 77:
              mx +=1;
              if (mx>=MLEN) mx = 0;
              changed=TRUE;
              break;
            case 75:
              mx -=1;
              if (mx<0) mx = MLEN-1;
              changed=TRUE;
              break;
            } 
          if (changed) drawmap();
          break;
        case 'Q':
        case 'q':
        case 27:
          return(TRUE);
        case 'Z':
        case 'z':
          if (zoom) zoom=FALSE;
          else zoom= TRUE;
          drawmap();
          break;
        case 'G':                        /* goto a spot in the map */
        case 'g':
          if (zoom) break;
          mx = (x-MX)/2-7;
          my = (y-MY)/2-4;
          if (mx<0) mx +=MLEN;
          if (my<0) my +=MWID;
          zoom = TRUE;
          drawmap();
                break;
        }
      }
    }
  }

static void DoEndDrag(void)
  {
  int temp[4];
  
  OldId=CurId;
  CurId++;
  if (CurId==0) CurId++;
  
  moucurbox(0,0,315,199);
  if ((EditFn==COPYFN)&&(CopyPicked))  // Set the Copy box in the vars correctly
    {          
    CopyPicked= FALSE;
    DoCopy    = TRUE;
    if (CopyWhere!=0)                  // Erase the pattern select box.
      {
      temp[0]=Copyx; temp[1]=Copyy;
      temp[2]=Copyx+CopyLen-1; temp[3]=Copyy+CopyWid-1;
      MapToScrn(temp[0],temp[1],&temp[0],&temp[1]);
      MapToScrn(temp[2],temp[3],&temp[2],&temp[3]);
      if (zoom) { temp[2]+=BLEN-1; temp[3]+=BLEN-1; }
      else      { temp[2]++; temp[3]++; }
      CopyWhere=0;
      moucur(False);
      HiBox(temp[0],temp[1],temp[2],temp[3],CopyFnErase);
      moucur(True);
      }
    CopyWhere=0;   // Set to zero so next time the button is pushed it won't
                   // erase what's not there.
    if (CopyData!=NULL) farfree(CopyData);
    CopyData=NULL;
    CopyData=(unsigned char far *) farmalloc((CopyLen+1)*(CopyWid+1)*sizeof(char));
    if (CopyData==NULL) return;                // Out of memory - so don't save!

    for (temp[0]=0;temp[0]<CopyLen;temp[0]++)     // Save the selected blocks
      for (temp[1]=0;temp[1]<CopyWid;temp[1]++)   // in a temporary data file
       *(CopyData+temp[1]+(temp[0]*CopyWid))= map[Copyy+temp[1]][Copyx+temp[0]].blk;

    DrawFnBut(COPYFN,BUTX+16,BUTY+34,FBCols[1]);
    EditFn=POINTFN;
    DrawFnBut(POINTFN,BUTX,BUTY+1,FBCols[1]);
    }
  }
  
static void MapToScrn(int x, int y, int *retx, int *rety)
  {
  if (zoom)
    {  
    x-=mx; y-=my;               // Calculate where on screen if in zoom mode
    if (x<0) x=x+MLEN;          // catch wrap around
    if (y<0) y=y+MWID;          // catch wrap around
    x*=BLEN; y*=BLEN;
    *retx=x;
    *rety=y;
    }
  else
    {
    *retx=x*2; *rety= y*2;
    }
  }

static void ScrnToMap(int x, int y, int *retx, int *rety)
  {
  if (zoom)
    {
    //x-=MX; y-=MY;         // translate into map[][] coordinates  
    x /=BLEN; y /=BLEN;
    
    x += mx; y += my;

    while (x>=MLEN) x-=MLEN;
    while (y>=MWID) y-=MWID;
    *retx=x;
    *rety=y;
    }
  else
    {
    *retx = x/2;
    *rety = y/2;
    }    
  }

            // Put EITHER bloc or monster on spot, depending on which
            // is selected.
            
static void changemap(int x, int y,int chto) 
  {
  int nx,ny;

  if ((x>=MLEN)||(x<0)||(y>=MWID)||(y<0)) return;
    
  nx=x; ny=y;                           // Save the map coords
  
  MapToScrn(nx,ny,&x,&y);
   
  saved=FALSE;
  if (status&BLOCACTIVE != 0)           // Put a block down
    {
    if (chto==BACKBL+10)
      {
      if ((DoCopy)&&(CopyData!=NULL)) 
        {
        int xoff,yoff;

        xoff=(nx-Copyx)%CopyLen;
        yoff=(ny-Copyy)%CopyWid;
        if (xoff<0) xoff= CopyLen+xoff;
        if (yoff<0) yoff= CopyWid+yoff;

        map[ny][nx].blk=*(CopyData+yoff+(xoff*CopyWid));
        }
      else map[ny][nx].blk = (blist+sely)%BACKBL;
      }
    else map[ny][nx].blk= (unsigned char) chto;
    
    if (map[ny][nx].blk<BACKBL)
      {
      if (zoom)
        {
        if ((x<SIZEX)&&(y<SIZEY)&&(x>=0)&&(y>=0)) 
          {
          if (!( (status<NOTOOLS)&&(x+BLEN-1>LMX-43)&&(x<LMX+32) ))
            drawblk(x,y,&blks[0][map[ny][nx].blk].p[0][0]);

          if ( (status>=NOTOOLS)&&(x/BLEN>=TOOLX/BLEN)&&(x/BLEN<=TOOLX/BLEN+2)&&(y/BLEN==TOOLY/BLEN))
            {
            Box(TOOLX-1,TOOLY,TOOLX+36,TOOLY+11,FaceCols[GREY1]);
            Box(TOOLX-2,TOOLY-1,TOOLX+37,TOOLY+12,FaceCols[GREY2]);
            Box(TOOLX-3,TOOLY-2,TOOLX+38,TOOLY+13,FaceCols[GREY3]);
            drawborder(TOOLX+1,TOOLY+2,35,9,1,1,FaceCols);
            toolb(TOOLX,TOOLY,FaceCols);
            }
          }
        }
      else UnZoomBox(x,y,BlkCols[map[ny][nx].blk]);
      }
    }
    
  if ( (status&MONACTIVE)&&(sely!=BACKBL)&&(map[ny][nx].mon==LASTMM))
    {                                                   // Put a monster down
    map[ny][nx].mon=findopenmm();
    if (map[ny][nx].mon!=LASTMM)
      {
      mm[map[ny][nx].mon].curx=nx;
      mm[map[ny][nx].mon].cury=ny;
      mm[map[ny][nx].mon].monnum=(mlist+sely)%LASTMON;
      }
    }
  if ((sely==BACKBL)&(map[ny][nx].mon<LASTMM))           // Erase a monster
    {
    mm[map[ny][nx].mon].monnum=LASTMON; 
    map[ny][nx].mon=LASTMM;
    saved=0;
    if (zoom)
      {
      moucur(FALSE);
      drawblk(x,y,&blks[0][map[ny][nx].blk].p[0][0]);
      moucur(TRUE);
      }
    }
  }

static void initalldata(void)
  {
  unsigned register int lx,ly;

  strcpy(mapcurfile,WorkDir);

  for (lx=0; lx<sizeof(blkstruct)*(BACKBL+1);lx++)
    *( ((unsigned char far *) blks[0])+lx)=0;
  for (lx=0; lx<sizeof(blkstruct)*(MONBL+1);lx++)
    *( ((unsigned char far *) blks[1])+lx)=0;

  for (lx=0;lx<sizeof(char)*MLEN*MWID*2;lx++)
    *(SaveMap+lx)=0;

  for (lx=0; lx<MLEN;lx++)
    for (ly=0; ly<MWID;ly++)
      {
      map[ly][lx].blk=BACKBLK; 
      map[ly][lx].mon=LASTMM;
      }
 
  mx  =my    = 0;
  zoom=FALSE;

  for (lx=0; lx<LASTMON; lx++)
    {
    for (ly=0;ly<LASTSL+1;ly++)
      {
      mon[lx].pic[ly][0] = MONBL;
      mon[lx].pic[ly][1] = 5;           // define a speed to change pictures
      }                                 // 'cuz a 0 would screw things up.
    mon[lx].cur.pic = 0;
    }
  mon[0].used=1;
  for(lx=0;lx<LASTMM;lx++)
    {
    mm[lx].monnum=LASTMON;
    mm[lx].status=0;
    mm[lx].curx=-1;
    mm[lx].cury=-1;
    mm[lx].extra=0;
    mm[lx].xtra=0;
    }
  }

static void initmap(void)
  {
  unsigned register int lx,ly;

  strcpy(mapcurfile,WorkDir);

  for (lx=0;lx<sizeof(char)*MLEN*MWID*2;lx++)
    *(SaveMap+lx)=0;

  for (lx=0; lx<MLEN;lx++)
    for (ly=0; ly<MWID;ly++)
      {
      map[ly][lx].blk=BACKBLK; 
      map[ly][lx].mon=LASTMM;
      }
 
  mx  =my    = 0;
  zoom=FALSE;

  for (lx=0; lx<LASTMON; lx++)
   {
    for (ly=0;ly<LASTSL+1;ly++)
     {
      mon[lx].pic[ly][0] = MONBL;
      mon[lx].pic[ly][1] = 5;     /* define a speed to change pictures */
     }                            /* 'cuz a 0 would screw things up.   */
    mon[lx].cur.pic = 0;
   }
  mon[0].used=1;
  for(lx=0;lx<LASTMM;lx++)
    {
    mm[lx].monnum=LASTMON;
    mm[lx].status=0;
    mm[lx].curx=-1;
    mm[lx].cury=-1;
    mm[lx].extra=0;
    mm[lx].xtra=0;
    }
  }

static int findopenmm(void)
  {
  register int lx;
  
  for (lx=0;lx<LASTMM; lx++)
    if (mm[lx].monnum==LASTMON) return (lx);
  return(LASTMM); 
  }

static void movemap(void)
  {
  register int lx,ly,nx,ny;

  if (zoom)
    {
    for (ly=0; ly<10;ly++)
      for (lx=0;lx<12;lx++)
        {
        nx = lx+mx;
        if (nx>=MLEN) nx-=MLEN;
        ny = ly+my;
        if (ny>=MWID) ny-=MWID;
        drawblk(MX+(lx*BLEN),MY+(ly*BLEN), ((char*)blks[0][map[ny][nx].blk].p));
        }
    }
  }

static void drawmap(void)
  {
  int lx,ly,nx,ny;

  moucur(FALSE);
  if (zoom)
    {
    for (ly=0; ly<10;ly++)    // Tile the whole screen with blocks.
      for (lx=0;lx<16;lx++)
        {
        nx = lx+mx;
        if (nx>=MLEN) nx -= MLEN;
        ny = ly+my;
        if (ny>=MWID) ny -= MWID;
        drawblk(MX+(lx*BLEN),MY+(ly*BLEN),((char *)blks[0][map[ny][nx].blk].p));
        }
    }
  else
    {
    BoxFill(200,0,319,199,BKGCOL);
    Line(200,0,200,199,FaceCols[RED1]);
    for (ly=0; ly<MWID;ly++)
      for (lx=0;lx<MLEN;lx++)
        if (map[ly][lx].blk<BACKBL) UnZoomBox(MX+(lx*2),MY+(ly*2),BlkCols[map[ly][lx].blk]);
    }
  if (status<NOTOOLS)
    {
    drawsurface(LMX-41,LMY-12,72,183,FaceCols);
    Line(LMX-1,LMY-1,LMX-1,LMY+(LMLEN*BLEN)-1,FaceCols[GREY3]);
    Line(LMX-1,LMY-1,LMX+BLEN-1,LMY-1,FaceCols[GREY4]);
    DrawvArrows(LMX,LMY,LMLEN,0,FaceCols);
    for (lx=0;lx<LMLEN;lx++)
      lightb(LMX+BLEN+2,LMY+lx*BLEN,0);

    drawthelist();
    BoxFill(BUTX-3,BUTY-1,BUTX+28,ZOOMY+20,FaceCols[GREY3]);
    DrawButtons();
    drawcompass(0);

    menub(MENUX,MENUY,FaceCols);
    helpb(MENUX,HELPY,FaceCols);
    toolb(MENUX,TOOL2Y,HiCols);
    }
  else
    {
    drawsurface(TOOLX-1,TOOLY,38,12,FaceCols);
    toolb(TOOLX,TOOLY,FaceCols);

    }
  moucur(TRUE);
  }

// Drawfn =  0 = Point  1 = Line  2 = Box  3 = BoxFill  4 = Circle  
// 5 = CircleFill  6 = Fill 7 = pattern 8=zoom 

static void DrawFnBut(int butnum, int x, int y, unsigned char col[4])
  {
  char array[][26]= {"   UPUAUT•AZTUAUT5U¿ˇ",
                     "   UPUA•T•AU§UAUT5U¿ˇ",
                     "   UP\x1A™AïdVAïdVA™§5U¿ˇ",
                     "   UP\x1A™A™§\x1A™A™§\x1A™A™§5U¿ˇ",
                     "   UPUAZTYAeî•AUT5U¿ˇ",
                     "   UPUAZT©Ajî•AUT5U¿ˇ",
                     "   ™†*™Ç™®*™Ç™®*™Ç™®:™¿ˇ",
                     "   UPfAôîfAôîfAôî5U¿ˇ",
                     "   UP\x1A™AVî•AZTïA™§5U¿ˇ"};

  draw4dat(x,y,array[butnum],9,9,col);
  }

static void DrawvArrows(int x,int y,int blnum,char selected,unsigned char *Cols)
  {
  unsigned char newcol[4];
  int lp;
  
  switch(selected)
    {
    case 1:  newcol[0]=(newcol[2]=(newcol[3]=Cols[GREY1])); newcol[1]=Cols[RED3];
             upb(x+5,y-10,newcol); break;
    case 2:  newcol[0]=(newcol[2]=(newcol[3]=Cols[GREY1])); newcol[1]=Cols[RED3];
             downb(x+5,y+BLEN*blnum+2,newcol); break;
    case 0:  
    default: newcol[0]=Cols[GREY1]; newcol[2]=(newcol[3]=Cols[RED2]); newcol[1]=Cols[RED1];
             upb(x+5,y-11,newcol);
             downb(x+5,y+BLEN*blnum+1,newcol);
             break;
    }
  }

static void DrawButtons(void)
  {
  register int l,m;

  for (m=0;m<2;m++)    
    for (l=0;l<4-m;l++)
      if (EditFn==l+4*m) DrawFnBut(l+4*m,BUTX+m*16,BUTY+(l*11)+1,FBCols[1]);
      else               DrawFnBut(l+4*m,BUTX+m*16,BUTY+(l*11),FBCols[0]);
      
  if (EditFn==ZOOMFN) DrawFnBut(ZOOMFN,ZOOMX,ZOOMY+1,FBCols[2]);
  else DrawFnBut(ZOOMFN,ZOOMX,ZOOMY+zoom,(zoom ? FBCols[1] : FBCols[0]) );
  if (EditFn==COPYFN) DrawFnBut(COPYFN,BUTX+16,BUTY+34,FBCols[2]);
  else DrawFnBut(COPYFN,BUTX+16,BUTY+33+DoCopy,(DoCopy ?FBCols[1] : FBCols[0]));
  }

static void FindFBCols(void)
  {
  SetCols(colors,FaceCols);

  FBCols[0][0]=FaceCols[GREY3]; //UNSELETED BUTTONS:    0==background==dark grey
  FBCols[0][1]=FaceCols[RED1];  //1==forground==medium red
  FBCols[0][2]=FaceCols[BLACK]; //2==picture==FaceCols[BLACK]
  FBCols[0][3]=FaceCols[RED2];  //3==button side==dark red

  FBCols[1][0]=FaceCols[GREY3]; //SELETED BUTTON:       0==background==dark grey
  FBCols[1][1]=FaceCols[RED3];  //1==forground==bright red
  FBCols[1][2]=FaceCols[GREEN]; //2==picture==greenish
  FBCols[1][3]=FaceCols[GREY3]; //3==button edge(==background)==dark grey

  FBCols[2][0]=FaceCols[GREY3]; //INTERMEDIATE BUTTON:  0==background==dark grey
  FBCols[2][1]=FaceCols[RED3];  //1==forground==bright red
  FBCols[2][2]=FaceCols[BLACK]; //2==picture==black (for blinking)
  FBCols[2][3]=FaceCols[GREY3]; //3==button edge(==background)==dark grey

  OffCols[0]=FaceCols[GREY1]; //DISABLED BUTTON:    0==background==light grey
  OffCols[1]=FaceCols[GREY2]; //1==forground==bright red
  OffCols[2]=FaceCols[GREY1]; //2==button edge(==background)==dark grey
  OffCols[3]=FaceCols[BLACK]; //3==picture==black
  }

static void drawcompass(char dir)
  { 
  char hilite;
  unsigned char col[4];
  
  if (zoom)
    {
    hilite = (dir&0x01);
    upb(ZOOMX,ZOOMY-8+hilite,FBCols[hilite]);
    dir >>= 1;  hilite = (dir&0x01);
    rightb(ZOOMX+12,ZOOMY-1+hilite,FBCols[hilite]);
    dir >>= 1;  hilite = (dir&0x01);
    downb(ZOOMX,ZOOMY+11+hilite,FBCols[hilite]);
    dir >>= 1;  hilite = (dir&0x01);
    leftb(ZOOMX-10,ZOOMY-1+hilite,FBCols[hilite]);
    }
  else
    {
    col[0]=col[2]=col[3]=FaceCols[GREY3];
    col[1]=FaceCols[GREY2];
    upb(ZOOMX,ZOOMY-7,col);
    downb(ZOOMX,ZOOMY+12,col);
    leftb(ZOOMX-10,ZOOMY,col);
    rightb(ZOOMX+12,ZOOMY,col);
    }
  }

static void drawthelist(void)
  {
  int m;

  if (status==MONACTIVE)
    {
    blk=blks[1];
    vdrawlist(LMX,LMY,LMLEN,LASTMON,&mlist,retmon);
    if (sely<BACKBL) lightb(LMX+BLEN+2,sely*BLEN+LMY,1);
    if (zoom) eraseb(MENUX,ERASEY,FaceCols);
    else      { eraseb(MENUX,ERASEY+1,OffCols); if (sely==BACKBL) sely=0;}
    if (sely<BACKBL) lightb(LMX+BLEN+2,sely*BLEN+LMY,1);
    blocb(MENUX,BLOCY,FaceCols);
    monstb(MENUX,MONSTY+1,HiCols);
    blk=blks[0];
    }
  else
    {
    blk=blks[0];
    eraseb(MENUX,ERASEY+1,OffCols);
    blocb(MENUX,BLOCY+1,HiCols);
    monstb(MENUX,MONSTY,FaceCols);
    vdrawlist(LMX,LMY,LMLEN,BACKBL,&blist,retblock);
    if (sely>=BACKBL) sely=0;
    lightb (LMX+BLEN+2,sely*BLEN+LMY,1);
    }
  }

static void drawmonsinmap(void)
  {
  int mnum=0;
  register int lx,ly;
  int x;
  int nx,ny;
  int moux,mouy,moub;
  
  if (zoom)
    {
    moustats(&moux,&mouy,&moub);
    moux=(moux/BLEN);
    mouy=(mouy/BLEN);

    for (ly=0; ly<10;ly++)
      for (lx=0;lx<16;lx++)
        {
        moub=FALSE;
        nx = lx+mx; if (nx>=MLEN) nx-=MLEN;
        ny = ly+my; if (ny>=MWID) ny-=MWID;
        if (map[ny][nx].mon<LASTMM)
          {
          x=MX+(lx*BLEN);
 
          if      ( (status<NOTOOLS)&&(x+BLEN-1>LMX-43)&&(x<LMX+32) )  ;
          else if ( (status>=NOTOOLS)&&(lx>=TOOLX/BLEN)&&(lx<=TOOLX/BLEN+1)
                  &&(ly==TOOLY/BLEN) )  ;
          else
            {
            if ((moux>=lx-1)&&(moux<=lx)&&(mouy>=ly-1)&&(mouy<=ly))
              { moub=TRUE; moucur(FALSE); }
            mnum=mm[map[ny][nx].mon].monnum;
            drawblk (MX+(lx*BLEN),MY+(ly*BLEN),((char*)blks[0][map[ny][nx].blk].p));
            drawcbloc(MX+(lx*BLEN),MY+(ly*BLEN),((char*)blks[1][mon[mnum].pic[mon[mnum].cur.pic][0]].p));
            if (moub) moucur(TRUE);
            }
          if ( (status>=NOTOOLS)&&(x+BLEN-1>TOOLX-3)&&(x<TOOLX+40) )
            {
            moucur(FALSE);
            Box(TOOLX-1,TOOLY,TOOLX+36,TOOLY+11,FaceCols[GREY1]);
            Box(TOOLX-2,TOOLY-1,TOOLX+37,TOOLY+12,FaceCols[GREY2]);
            Box(TOOLX-3,TOOLY-2,TOOLX+38,TOOLY+13,FaceCols[GREY3]);
//            drawborder(TOOLX+1,TOOLY+2,35,9,1,1,FaceCols);
            toolb(TOOLX,TOOLY,FaceCols);
            moucur(TRUE);
            }
          }
      	}
    }  
  }

static void redrawall(void)
  {
  GraphMode();
  SetAllPalTo(&colors[BKGCOL]);
  setmoupos(MENUX+18,MENUY+5);
  drawmap();
  FadeTo(colors);
  }

int retblock(int blnum)
  {
  return(blnum);
  }

int retmon(int rebmun)      /* returns the current bloc number of the */
 {                          /*         monster number 'rebnum'.       */
 animons(); 
 return(mon[rebmun].pic[mon[rebmun].cur.pic][0]);
 }

static int animons(void)
 {
  int l,i=FALSE;

  for(l=0;l<LASTMON;l++) {
    if (mon[l].used) {
      if ( (mon[l].lasttime != Clock) &&
           ( (Clock-mon[l].lasttime) >= mon[l].pic[mon[l].cur.pic][1] ) )
        {
        if (mon[l].pic[(++mon[l].cur.pic)][0] == MONBL)
          mon[l].cur.pic = 0;
        mon[l].lasttime=Clock;
        i=TRUE;
        }
     }
   }
  return(i);
 }

static void lightb(int x,int y,char onoff)
  {
  static char lightar[] = {"@\x1Aêj‰k‰j§\x1Aê@"};
  unsigned char col[4];
  
  if (onoff) { col[0]=FaceCols[GREY1]; col[1]=FaceCols[RED2];  col[2]=FaceCols[RED1];  col[3]=FaceCols[RED3];  }
  else       { col[0]=FaceCols[GREY1]; col[1]=FaceCols[GREY4]; col[2]=FaceCols[GREY3]; col[3]=FaceCols[GREY2]; }

  draw4dat(x,y+BLEN/2-3,lightar,7,6,col);
  }

static void monstb(int x,int y, unsigned char col[4])
  {
  static char monstar[]="         UUUUUUU@UUUUUUUTW]u◊ﬂﬂﬂTWﬂw}›W]]’Wˇw◊W__UWwww’◊]]’óW]uﬂW_›÷UUUUUUUT)UUUUUUUh™™™™™™™Ä";
  draw4dat(x,y,monstar,35,10,col);
  }

static void blocb(int x,int y, unsigned char col[4])
  {
  static char blocar[] = "         UUUUUUU@UUUUUUUT_◊U}_]uTU]wU◊u›’UU_◊U◊u_UUU]wU◊u›’Uï_◊˝}_]uVUUUUUUUT)UUUUUUUh™™™™™™™Ä";
  draw4dat(x,y,blocar,35,10,col);
  }

static void eraseb(int x,int y, unsigned char col[4])
  {
  static char erasear[] = "         UUUUUUU@UUUUUUUT_˜ı}__ıTU]W]◊u]UUU_◊ıˇ__’UU]W]◊U›UUï_˜]◊_ıVUUUUUUUT)UUUUUUUh™™™™™™™Ä";
  draw4dat(x,y,erasear,35,10,col);
  }
  

static void toolb(int x,int y, unsigned char col[4])
  {
  static char toolar[] = "         UUUUUUU@UUUUUUUTıı}u_UTUWW]◊uuUUUWW]◊u_UUUWW]◊uU’UïWUı}UVUUUUUUUT)UUUUUUUh™™™™™™™Ä";
  draw4dat(x,y,toolar,35,10,col);
  }

static void rightb(int x,int y, unsigned char col[4])
  {
  char rightar[] = "  @ P T U U@U@U¿W \\ p ¿ ";
  draw4dat(x,y,rightar,7,11,col);
  }

static void downb(int x,int y, unsigned char col[4])
  {
  char downar[] = "   UUP’Up5U¿\x0DW \\   ";
  draw4dat(x,y,downar,11,6,col);
  }

static void leftb(int x,int y, unsigned char col[4])
  {
  char leftar[] = "      UUUU ’ 5 \x0D ";
  draw4dat(x,y,leftar,7,11,col);
  }

static void upb(int x,int y, unsigned char col[4])
  {
  char upar[]= {"    P T U U@UUPˇˇ"};
  draw4dat(x,y,upar,11,6,col);
  }

static void HiFill(int x, int y)
  {
  moucur(0);
  FillOver=map[y][x].blk;
  if (DoCopy)
    {
    DoCopy=FALSE;
    FillCol=BACKBL;
    FillIter(x,y,1);    // Fill With a never used masking color
    DoCopy=TRUE;
    FillCol=BACKBL+10;  
    FillOver=BACKBL;
    FillIter(x,y,1);    // Fill with the pattern.
    }
  else if (FillOver!=(blist+sely)%BACKBL) 
    {
    FillCol=(blist+sely)%BACKBL;
    FillIter(x,y,1);
    }
  moucur(1);
  }

// inline
static int GetPt(int x,int y)
  {
  if ((x<MLEN)&&(x>=0)&&(y<MWID)&&(y>=0)) 
    return(map[y][x].blk);
  else return(BACKBL+1);
  }

//inline
static void SetPt(int x,int y)
  {
//  if ((FillCol==BACKBL)&&(x>Copyx)&&(y>Copyy)&&(x<Copyx+CopyLen)&&(y<Copyy+CopyWid)) return;
  changemap(x,y,FillCol);
  }

static void FillIter(int x,int y,int dir)
  {
  char done=FALSE;
  char SepFlag=0;   
  int Nextx=-1,Nexty;
  int Otherx=-1,Othery;

  if (_SP<100) return;  // Make sure there are no stack overflows

  while (GetPt(--x,y)==FillOver);  // Move x as far left as possible
  x++;  // The above while moves it one too far.

  while(!done)
    {
    SetPt(x,y);    
  
    if (GetPt(x,y-dir)==FillOver)  // check the other direction
      {
      if (Otherx==-1) { Otherx=x; Othery=y-dir; }
      else if ((SepFlag&2)==2)  
        {
        FillIter(Otherx,Othery,-1*dir);  // Ok to do it now 'cause we know it's closed off
        SepFlag &=1;     // Set up next opposite direction to do
        Otherx=x;           //   |
        Othery=y-dir;       //   |
        }
      }
    else if (Otherx!=-1) SepFlag |=2;   

    if (GetPt(x,y+dir)==FillOver)  // Need to look for next starting spot up
      {
      if (Nextx==-1) { Nextx=x; Nexty=y+dir; } // Remember first way up
      else if ((SepFlag&1)==1)  // two seperate ways up - call FillIter for #2
        {
        FillIter(x,y+dir,dir);
        SepFlag &=2;   // Set to false in case 3 seperate ways up. 
        }
      }
    else if (Nextx!=-1) SepFlag |=1;   
      
    x++;

    if (GetPt(x,y)!=FillOver)
      {
      if (Nextx==-1) done=TRUE;
      x=Nextx;
      y=Nexty;
      Nextx=-1;
      while (GetPt(--x,y)==FillOver);  // Move x as far left as possible
      x++;  // above while moves it one too far            
      SepFlag &=2;  // Set same dir sep to 0
      }
    }
  if (Otherx!=-1) FillIter(Otherx,Othery,-1*dir); 
  }  

static void UpdateShape(int x,int y,int prevx, int prevy)
  {
  moucur(0);
  switch (EditFn)
    {
    case LINEFN:
      HiLine(Firstx,Firsty,x,y,DrawOnlyIfNew);
      HiLine(Firstx,Firsty,prevx,prevy,RestoreIfOld);
      break;
    case BOXFN:
      HiBox(Firstx,Firsty,x,y,DrawOnlyIfNew);
      HiBox(Firstx,Firsty,prevx,prevy,RestoreIfOld);
      break;
    case BOXFILLFN:
      HiBoxFill(Firstx,Firsty,x,y,DrawOnlyIfNew);
      HiBoxFill(Firstx,Firsty,prevx,prevy,RestoreIfOld);
      break;
    case CIRCLEFN:
      HiCircle(Firstx,Firsty,x,y,DrawOnlyIfNew);
      HiCircle(Firstx,Firsty,prevx,prevy,RestoreIfOld);
      break;
    case CIRCLEFILLFN:
      HiCircleFill(Firstx,Firsty,x,y,DrawOnlyIfNew);
      HiCircleFill(Firstx,Firsty,prevx,prevy,RestoreIfOld);
      break;
    case COPYFN:
      int temp[4];
      CopyPicked=TRUE;      
      if (CopyWhere!=0)   // Erase old select box
        {
        if (Firstx<prevx) { temp[0]=Firstx; temp[2]=prevx; }
        else              { temp[0]=prevx;  temp[2]=Firstx;}
        if (Firsty<prevy) { temp[1]=Firsty; temp[3]=prevy; }
        else              { temp[1]=prevy;  temp[3]=Firsty;}
        MapToScrn(temp[0],temp[1],&temp[0],&temp[1]);
        MapToScrn(temp[2],temp[3],&temp[2],&temp[3]);
        if (zoom) { temp[2]+=BLEN-1; temp[3]+=BLEN-1; }
        else      { temp[2]++; temp[3]++; }
        CopyWhere=0;  
        HiBox(temp[0],temp[1],temp[2],temp[3],CopyFnErase);
        }
      if (Firstx<x) { temp[0]=Firstx; temp[2]=x; }
      else              { temp[0]=x;  temp[2]=Firstx;}
      if (Firsty<y) { temp[1]=Firsty; temp[3]=y; }
      else              { temp[1]=y;  temp[3]=Firsty;}
      Copyx=temp[0]; Copyy=temp[1];
      CopyLen=temp[2]-temp[0]+1;
      CopyWid=temp[3]-temp[1]+1;
      MapToScrn(temp[0],temp[1],&temp[0],&temp[1]);
      MapToScrn(temp[2],temp[3],&temp[2],&temp[3]);
      if (zoom) { temp[2]+=BLEN-1; temp[3]+=BLEN-1; }
      else      { temp[2]++; temp[3]++; }
      CopyWhere=0;
      HiBox(temp[0],temp[1],temp[2],temp[3],CopyFnPt);
      break;
    }
  moucur(TRUE);
  OldId=CurId;
  CurId++;
  if (CurId==0) CurId++;
  }


static void CopyFnPt(int x,int y)
  {
  if ((x>=0)&&(x<SIZEX)&&(y>=0)&&(y<SIZEY)) 
    {
    SaveMap[CopyWhere]=GetCol(x,y);
    CopyWhere++;
    Point(x,y,FBCols[1][3]);
    }
  }

static void CopyFnErase(int x,int y)
  {
  if ((x>=0)&&(x<SIZEX)&&(y>=0)&&(y<SIZEY)) 
    {
    Point(x,y,SaveMap[CopyWhere]);
    SaveMap[CopyWhere]=0;
    CopyWhere++;
    }  
  }



static void DrawOnlyIfNew(int x, int y)
  {
  if ((x>=MLEN)||(x<0)||(y>=MWID)||(y<0)) return;  // Filter off screen points

  if ((*(SaveMap+((x+(y*MLEN))*2)+1)!=OldId)&&(*(SaveMap+((x+(y*MLEN))*2)+1)!=CurId))
    {
    *(SaveMap+((x+(y*MLEN))*2))=map[y][x].blk;
    changemap(x,y);
    }
  *(SaveMap+((x+(y*MLEN))*2)+1)=CurId;
  }

static void RestoreIfOld(int x, int y)
  {
  if ((x>=MLEN)||(x<0)||(y>=MWID)||(y<0)) return;  // Filter off screen points

  if (*(SaveMap+((x+(y*MLEN))*2)+1)==OldId)
    {
    map[y][x].blk=*(SaveMap+((x+(y*MLEN))*2));
    *(SaveMap+((x+(y*MLEN))*2)+1)=0;  // 0 means nothing covering it
    changemap(x,y,map[y][x].blk);
    }
  }
  
static void HiBoxFill(int x, int y, int x1, int y1, void point(int x,int y))
  {
  register int lx,ly;

  if (x>x1) swap (&x,&x1);
  if (y>y1) swap (&y,&y1);

  for (ly=y;ly<=y1;ly++) 
    for (lx=x;lx<=x1;lx++)
      point(lx,ly);
  }

static void HiBox(int x, int y, int x1, int y1, void point(int x,int y))
  {
  register int l;
  int len;

  if (x>x1) swap (&x,&x1);
  if (y>y1) swap (&y,&y1);
  
  HiLine(x,y,x1,y,point);
  if ((y!=y1)&&(x!=x1)) HiLine(x1,y+1,x1,y1,point);
  if ((x!=x1)&&(y!=y1)) HiLine(x1-1,y1,x,y1,point);
  if (y+1<y1) HiLine(x,y1-1,x,y+1,point);
  }

static void HiLine(int x1,int y1,int x2,int y2, void point(int x,int y))
  {
  register int l;
  int dx=0,dy=0, sdx=0,sdy=0, absdx=0, absdy=0;
  int x=0,y=0;
  
  dx = x2-x1;
  dy = y2-y1;
  sdx=Sign(dx);
  sdy=Sign(dy);
  absdx=abs(dx);
  absdy=abs(dy);

  point(x1,y1);

  if (absdx>=absdy)
    {
    for (l=0; l<absdx; l++)
      {
      y += absdy;
      if (y>=absdx)
	{
	y-=absdx;
	y1 += sdy;
	}
      x1 += sdx;
      point(x1,y1);
      }
    }
  else
    {
    for (l=0; l<absdy; l++)
      {
      x += absdx;
      if (x>=absdy)
	{
	x -= absdy;
	x1 += sdx;
	}
      y1 +=sdy;
      point(x1,y1);
      }
    }
  }


static void HiCircle(int centx, int centy, int x1, int y1, void point(int x,int y) )
  {
  int RadSquared=0;
  register int x=0,y=0;
  long int radius=1;

  x1-=centx;
  y1-=centy;

  RadSquared = ((x1*x1)+(y1*y1));
  radius = (long int) RadSquared;
  radius *= (7071* 7071);
  radius /=(10000*10000);  
  radius=(long int) sqrt( (int) radius );
 // radius *= .707106781; // # is cos(PI/4) - makes radius only go halfway around the circle

  for (y=0; y<=radius; y++)
    {
    x = sqrt(RadSquared - (y*y) );
    point(centx+x,centy+y);
    point(centx+x,centy-y);
    if (x>0)
      {
      point(centx-x,centy+y);
      point(centx-x,centy-y);
      }
    if (x!=y)  // Catches when theta = PI/4 (or y = radius*.707...)
      {
      point(centx+y,centy+x);
      point(centx+y,centy-x);
      if (y>0)
        {
        point(centx-y,centy+x);
        point(centx-y,centy-x);
        }
      }
    }
  }

int sqrt(int x)
  {
  unsigned int squ=0,oldsqu=0;
  unsigned int guess=0;

  x=abs(x);

  if (x==0) return(0);
  if (x==1) return(1);
  while (squ<x)
    {
    oldsqu=squ;
    guess++;
    squ=guess*guess;
    }

  if (abs(squ-x)<=abs(oldsqu-x))
    return(guess);
  return(guess-1);
  }

/*                         End of function Circle                        */
/*-----------------------------------------------------------------------*/
/*      Function circlefill fills a circle.   HIGH LEVEL                 */

static void HiCircleFill(int centx, int centy, int x1, int y1, void point(int x,int y))
  {

  int RadSquared=0;
  register int x=0,y=0;
  int radius=1;

  x1-=centx;
  y1-=centy;

  RadSquared = ((x1*x1)+(y1*y1));
  radius=sqrt(RadSquared);

  HiLine(centx-radius,centy,centx+radius,centy,point);
  for (y=1; y<= radius; y++)
    {
    x = sqrt(RadSquared - (y*y));
    HiLine(centx-x,centy+y,centx+x,centy+y,point);
    HiLine(centx-x,centy-y,centx+x,centy-y,point);
    }
  }

static void UnZoomBox(int x,int y, unsigned char col)
  {
  unsigned char far *vptr = (unsigned char far *) 0xA0000000;

  vptr+=x+(SIZEX*y);
  *vptr=col;
  vptr++;
  *vptr=col;
  vptr+=SIZEX-1;
  *vptr=col;
  vptr++;
  *vptr=col;
  }

/*
static void FindBlkCols(void)
  {
  register int y,x,l;
  int ColTally[256];
  int largest;

  for (l=0;l<BACKBL;l++)
    {
    for (x=0;x<256;x++) ColTally[x]=0;  // Clear tally variables
    for (y=0;y<BLEN;y++)
      for (x=0;x<BLEN;x++)
        ColTally[ ((unsigned char) blks[0][l].p[y][x])]++;
    largest=0;
    BlkCols[l]=0;
    for (x=0;x<256;x++)   // Find the most present color and use it 
      if (ColTally[x]>largest) { BlkCols[l]=x; largest=ColTally[x]; }      
    }  
  }
*/
static int RetHighestNum(int x,int y,int z)
  {
  if ((x>=y)&&(x>=z)) return(x);
  if ((y>=x)&&(y>=z)) return(y);
  return(z); 
  }


static void FindBlkCols(void)
  {
  register int y,x,l,l1;
  unsigned char highest;
  int largest;
  int ColTally[256];
  unsigned char ColUsed[256];
  char GenCol[256];
  RGBdata  *c;
  int Tally[8];

  for (l=0;l<256;l++)  // Classify each color as a primary or secondary color.
    {
    GenCol[l]=0;
    c=&colors[l];
    highest=RetHighestNum(c->red,c->green,c->blue);
    if (highest>10)
      {
      if (c->red>highest*2/3) GenCol[l] |=1;
      GenCol[l] <<=1;
      if (c->green>highest*2/3) GenCol[l] |=1;
      GenCol[l] <<=1;
      if (c->blue>highest*2/3) GenCol[l] |=1;
      }
    ColUsed[l]=0;  // Initalize this array for later use
    }

  for (l=0;l<BACKBL;l++)
    {
    for (l1=0;l1<8;l1++) Tally[l1]=0;   // Clear the prev block's tally
    for (y=0;y<BLEN;y++)                // Find which Primary or secondary
      for (x=0;x<BLEN;x++)              //   color is prevalent.
        Tally[GenCol[blks[0][l].p[y][x]]]+=1+((BLEN/2)-abs((BLEN/2)-y))/2 +((BLEN/2)-abs((BLEN/2)-x))/2;
    highest=0;
    for (l1=0;l1<8;l1++)                // highest gets the color which
      if (Tally[highest]<Tally[l1]) highest=l1; // is most common.
    // Find the mode specific color in the prim. and secondary color class    
    for (x=0;x<256;x++) ColTally[x]=0;  // Clear tally variables
    for (y=0;y<BLEN;y++)
      for (x=0;x<BLEN;x++)
        if (GenCol[blks[0][l].p[y][x]]==highest)  // In the color class
          ColTally[ ((unsigned char) blks[0][l].p[y][x])]++;
    largest=0;
    BlkCols[l]=0;
    for (x=0;x<256;x++)   // Find the most present color and use it 
      if (((ColUsed[x]==0)&&(ColTally[x]>largest))||(largest==0)) 
        { BlkCols[l]=x; largest=ColTally[x]; }      
    ColUsed[BlkCols[l]]=1;
    }
  }
