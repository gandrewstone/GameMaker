/*-----------------------------------------------------------*/
/*   BLOCEDIT.C                                              */
/*   Graphic editor that edits 30 20x20 256 color blocks     */
/*   Used to make video games        V2.0                    */
/*   Programmer: Andy Stone          Date :10/10/90          */
/*   Last Edited: 5/26/92                                    */
/*                7/19/93                                    */
/*-----------------------------------------------------------*/

#include <bios.h>
#include <stdio.h>
#include <process.h>
#include <dos.h>
#include <conio.h>
#include <alloc.h>
#include <string.h>
#include <stdlib.h>

#include "gen.h"
#include "scrnrout.h"
#include "mousefn.h"
#include "windio.h"
#include "gmgen.h"
#include "palette.h"
#include "jstick.h"
#include "bloc.h"
#include "mon.h"
#include "facelift.h"
#include "graph.h"
#include "tranmous.hpp"

#define TRANSCOL 255     //  The value that is transparent in _pic fns
#define MCGA
#define GLEN   6         //  Number of blocks in len in width of the grid
#define BX     5
#define BX1    (BX+(BLEN*5))
#define BY     50
#define BY1    (BY+(BLEN*5))
#define GX     196
#define GY     34
#define GX1    (GX+(BLEN*GLEN))
#define GY1    (GY+(BLEN*GLEN))
#define LLEN   14
#define LY     175
#define LX     20
#define LX1    (LX+(LLEN*BLEN))
#define LY1    (LY+BLEN)
#define HELPX  130
#define HELPY  110
#define MENUX  HELPX
#define MENUY  (HELPY+12)
#define INFOX  HELPX
#define INFOY  (MENUY+12)
#define ANIX   HELPX
#define ANIY   (INFOY+12)

#define HELPFILE "bloc.hlp"

#define LASTEDITFN 7                    // Defines for the edit fn buttons
#define EBLEN   11
#define EBX     (BX+12)
#define EBY     (BY-24)
#define EBX1    (EBX+(EBLEN*LASTEDITFN)-1)
#define EBY1    (EBY+10)
#define LASTKEYBUT 9                    // Defines for the edit fn buttons
#define KBLEN   11
#define KBX     (BX+1)
#define KBY     (EBY+10)
#define KBX1    (KBX+(KBLEN*LASTKEYBUT)-1)
#define KBY1    (KBY+10)

#define SELX     123                            // Birthmon defines
#define SELY     90
#define MOLEN    13 
#define MOX      32
#define MOY2     40
#define TBOXX    75
#define TBOXY    145
#define MX3      SELX+40
#define MY3      SELY-3

#define COLX    30                             // Position of palette
#define COLY    5
#define MAXCOL  255
#define COLPX1  120                            // Position of selected color 
#define COLPX2  COLPX1+COLPLEN+3
#define COLPY   33
#define COLPLEN 29
#define COLPWID 20

#define FADETIME 10

/*=-----------------------------------Function Declarations     */
void GWriteShadow(int x, int y, unsigned char col, unsigned char col1, char *str);
static void MakeEffect(int bloc,char *str);
static void DrawGravityInfo(int blkx,int blky,blkstruct *blk);
static void DrawSolidInfo(int blkx,int blky,blkstruct *blk);

static void InitAni(void);
static void AnimateGridBlocks(int cursorx,int cursory);

int sqrt(int x);
static void drawbig(void);
static void drawgrid(void);
static void drawpal(void);
static void drawscrn(void);
static void DrawFnBut(int drawfn,int x,int y, unsigned char col[4]);
static void DrawKeyBut(int butnum,int x,int y, unsigned char col[4]);
static void AniBut(int x,int y, unsigned char col[4]);
static void ObjButton(int InOrOut, unsigned char col);
static void FindEBcols(void);
static void colprompt(void);
static void drawallblocks(void);
static void infob(int x,int y, unsigned char col[4]);
static void DrawClearBMP(int x,int y, unsigned char col[4]);
static void DrawGravBut (int x,int y, unsigned char col[4],int rotation);
static void DrawTrashCan(int x,int y, unsigned char col[4]);
static void DrawCanTop  (int x,int y, unsigned char col[4]);

void MoveSecondPoint(int x,int y);
void MoveAnyPoint(void); 
void SetFirstPoint(int x,int y);
void ErasePreviousDraw(void);

static void flip(int blocnum);
static void rotate(int blocnum);
static void reflect(int blocnum);
static void updatescrn(int blocnum);

static void HiLine(int x1,int y1,int x2,int y2, unsigned char col,int drawfn);
static void HiCircle(int centx, int centy, int x1, int y1, unsigned char bloc, int drawfn);
static void HiCircleFill(int centx, int centy, int x1, int y1, unsigned char bloc, int drawfn);
static void HiBox(int x, int y, int x1, int y1, unsigned char bloc, int drawfn);
static void HiBoxFill(int x, int y, int x1, int y1, unsigned char bloc, int drawfn);
static void HiFill(int x, int y, int bloc);

typedef struct
  {
  char x,y;
  } FillData;

static void near FillIter(FillData F);

static int animons(void);

static void initalldata(void);
static void cleargrid(void);
static void clearblk(int num, int color);
static void askcopyblock(void);
static void copyblock(int from, int to);
static void CopyInfo(int from,int to);

static void GetCurCol(char button);
static void xfer(void);                            // block transfer routine
static int  edit(void);
static int  domouse(int x,int y,int buts,int *bnum,int *grabbed);
static int  dokeys(int x,int y,int bnum);

static void NoDrawPt(int x,int y,int bloc); // Sets point, but not on screen
static void drawpt(int x, int y,int num, int fn);  // Sets point + updates scrn.
static void changeblk(int x, int y);

static void changeinfo(int bnum);
static void cleanup(void);      /* called on exit -- 'cleans up' everything */

int monnum(int rebmun);
int blocnum(int mun);    /* returns block number for scrolling_list  */
                         /* not needed for blocks, but allows one to */
                         /* use same scroll_list for monsters        */

static char birthmon(int curmon);             //Have block create a monster

extern char BlkExtensions[][7];

static void shftblk(int dir, int bnum);
static int cvtpal(void);

//=-------------------------------Variables-------------------------------

extern unsigned _stklen=9000U;  // Set the stack to 9000 bytes, not 4096

int                  FillBloc=0, FillOverCol=0; // Global used only in HiFill and FillIter
RGBdata              colors[256];   // The Red, Green, and Blue intensities of every color
extern unsigned char FaceCols[MAXFACECOL];      // Screen colors
extern char          *FontPtr;
extern unsigned char HiCols[4];
static unsigned char grid[GLEN][GLEN];
unsigned long int    nxtchg[BACKBL]; // Next time array for each block change.
unsigned char        blkmap[BACKBL]; // Current block displayed on map Array.
char                 Animate=FALSE;

static unsigned char big=0;       // The number of the block in the blowup.
static int           list=0;
static unsigned char curcol[3];   // The color selected for each mouse button
static char          cb=0;        // the current mouse button pressed 
static int           grabbed=-1;  // the block that the mouse has grabbed (if it has) 
//static int           bcol=1;      // border col 

static int           btype=1;
static int           oldcmd=0;
static char          saved=1;
extern char          curfil[41];
extern int           lastbl;
extern blkstruct     *blk;

monstruct            m[LASTMON];

unsigned char        EditFn=0;
unsigned char        EBcols[3][4];  // The colors to draw the tools and menu in

static int           Firstx=-1,Firsty=-1,Secondx=-1,Secondy=-1;
static unsigned char EraseCols[(BLEN*BLEN)+10];
static int           EraseCtr=0;
//=-----------------------------------Code----------------------------------

QuitCodes main(int argc,char *argv[])
  {
  static char w[1000];
  int choice=1;
  int attr=0;
  int temp;

#ifdef DEBUG
  printf("Initialization finished.\n");
  NewAbortMSG("GM:ATE in main procedure.\n");
#endif
#ifdef CHKREG
  if ((argc < 2) || (strcmp(argv[1],OKMSG)!=0)) RestorePal=True;
#endif
  atexit(cleanup);
  FontPtr=GetROMFont();
  blk=(blkstruct far *) farmalloc( (unsigned long int) ( ((unsigned long int)sizeof(blkstruct))*(BACKBL+1)));
  if (blk==NULL) { errorbox("NOT ENOUGH MEMORY!","  (Q)uit"); exit(menu);}

#ifdef DEBUG
  printf ("EXTRA MEMORY: Far: %lu  near: %u\n",farcoreleft(),coreleft());
#endif
  if (initmouse()==FALSE)
    { 
    printf("7a - No mouse \n");
    errorbox("This application requires a Microsoft","compatible mouse! (Q)uit");
    exit(quit);
    }
  initalldata();

  // Set up colors as default.pal, and then my fun palette if fails
  if (!loadspecany("default",".pal",WorkDir,sizeof(RGBdata)*256,(char *) colors))
    InitStartCols(colors);
  FindEBcols();
  SetTextColors();
  do
    {
    moucur(0);
    writestr(0,0,79,GMTOPBAR);
    writestr(0,1,31,"  Palette  Block  Monster  Map  Character  Image  Sound  Integrator  Main  Help ");
    attr = openmenu(7,3,23,14,w);
    writestr(7,3,PGMTITLECOL, "    BLOCK DESIGNER     ");
    writestr(7,4,PGMTITLECOL, "     Version "GMVER"      ");
    writestr(7,5,PGMTITLECOL, "   By Gregory Stone    ");
    writestr(7,6,PGMTITLECOL, "  Copyright (C) 1991   ");
    writestr(7,8,attr+14, " Choose a Block Set ");
    writestr(7,9,attr+14, " New Block Set");
    writestr(7,10,attr+14," Choose a Palette");
    writestr(7,11,attr+14," Edit Chosen Blocks");
    writestr(7,12,attr+14," Save a Block Set");
    writestr(7,13,attr+14," Transfer Between Sets");
    writestr(7,14,attr+14," Convert Palette");
    writestr(7,15,attr+14," Delete a Block Set");
    writestr(7,16,attr+14," Quit");
    choice=vertmenu(7,8,23,9,choice);
    closemenu(7,3,23,14,w);
    switch (choice) 
      {
      case 1:
        if (!saved)
          {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice=5; break; }
          }
        if ((temp=getblktype()) >0)
          {
          if (loadblks(temp,(char *)blk))
            {
            btype=temp;
            list=0; cleargrid(); big=0; choice=3; saved=1;
            }  else { choice=1; saved=1; }
          }  else choice=1;
        break;
      case 2: 
        if (!saved)
          {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice=5; break; }
          }
        if ( (temp=getblktype())>0)
          {
          initalldata();
          strcpy(curfil,WorkDir);
          btype=temp;
          if (btype==1) lastbl=BACKBL;
          if (btype==2) lastbl=MONBL;
          if (btype==3) lastbl=CHARBL;
          errorbox("New block set created!","(C)ontinue",2);
          saved=TRUE; choice=3;
          }
        else choice=2;
        break;
        case 3: 
          if (loadany("Enter the Palette name: ","*.pal\0",WorkDir,sizeof(RGBdata)*256, (char *) &colors,0))
             FindEBcols();  // Set the Edit Button Colors to neat colors
          choice++; 
          break;
        case 4: edit(); TextMode(); choice++; break;
        case 5:
          choice=4; 
          if (saveblks(btype,(char *)blk)) { choice=9; saved=TRUE; } 
          break;
        case 6: xfer(); choice=5; break;
        case 7: if (cvtpal()) choice=5; else choice=1; break;
        case 8: if ( (temp=getblktype())>0)
  delany("Delete which Block Set: ",BlkExtensions[temp-1],WorkDir);
	break;
      case 0: choice=9;
      case 9:
        if (!saved)
          {
          choice = (errorbox("Unsaved changes--Really Quit?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice=5; break; }
          }
	      choice=10;
	      break;
      }
    } while (choice != 10);
  return(menu);
  }

static void cleanup(void)
  {
  Time.TurnOff();
  farfree(blk);
  }

static int edit(void)
  {
  char str[20];
  int ox=MENUX,oy=MENUY,obuts=0;
  int mx,my,mbuts;
  int bnum=0,grabbed=-1;
  int changed=FALSE;
  unsigned long int OldClock;

  moucur(TRUE);
  mouclearbut();
  SetCols(colors,FaceCols);
  InitAni();
  drawscrn();
  setmoupos(ox,oy);
  grabbed =-1;

  OldClock=Clock;
  while(TRUE)
    {
    changed=FALSE;
    do
      {
      if ((Animate)&&(OldClock<Clock))
        {
        OldClock=Clock;
        AnimateGridBlocks(ox,oy);
        }
      if (bioskey(1)) changed=1;
      moustats(&mx,&my,&mbuts);
      if ((mx!=ox)|(my!=oy)) changed |= 2;
      if (mbuts!=obuts) changed |=4;
      if (mbuts>0) changed |=8;
      } while (!changed);

    if (mbuts>0) cb=((mbuts)&3)-1;

    if (((changed&7)>0)&&(grabbed!=-1))             // Erase block
      {
      moucur(FALSE);
      drawblk(ox-10,oy-10,&blk[BACKBL].p[0][0]);
      moucur(TRUE);
      }
 
    if (mbuts==0) { Secondx=-1; Firstx=-1; }

    if ((changed&1)>0)
      if (!dokeys(mx,my,bnum)) { FadeAllTo(colors[BKGCOL]); return (TRUE); }

    if ( ((changed&6)>0)|(mbuts>0))
      {
      if (!domouse(mx,my,mbuts,&bnum,&grabbed))
        { FadeAllTo(colors[BKGCOL]); return(TRUE); }
      }            

    if (((changed&7)>0)&(grabbed!=-1))             // draw block
      {
      moucur(FALSE);
      getblk(mx-10,my-10,&blk[BACKBL].p[0][0]);
      drawblk(mx-10,my-10,&blk[grabbed].p[0][0]);
      moucur(TRUE);
      }
    ox=mx;oy=my; obuts=mbuts;
    }
  //return(FALSE);  // never will occur, but removes warning.
  }
 
static void xfer(void)
  {
  unsigned register int j,k;
  blkstruct far *blks[2] = { NULL,NULL };
  int curblk[2],lastb[2],temp[2];
  unsigned col,grabbed=255;
  int X=(BLEN*2),Y=50,LEN=12;
  char w[400],attr;
  int mx,my,oldx=0,oldy=0,buts;
  char str[30],filname[41];

  if ( (btype<1)||(btype>3) ) return;

  for (j=0;j<41;j++) filname[j]=curfil[j];
  blks[1]=blk;
  temp[1]=btype;
  switch (btype) 
    {
    case 1: lastb[1]=BACKBL; break;
    case 2: lastb[1]=MONBL; break;
    case 3: lastb[1]=CHARBL; break; 
    }
  curblk[1]=0;

  blks[0]= (blkstruct far *) malloc( (unsigned int) ( ((unsigned int)sizeof(blkstruct))*(BACKBL+1)));
  if (blks[0]==NULL) 
    { 
    errorbox("NOT ENOUGH MEMORY!","  (Q)uit");
    blk=blks[1]; 
    return; 
    }
  blk=blks[0];
  initalldata();
  attr = openmenu(9,3,27,2,w);
  writestr(9,3,attr+14,"Choose Block Set from which");
  writestr(9,4,attr+14,"   blocks are transfered:");
  do {
    curfil[0]=0;
    switch ( (temp[0]=getblktype()) ) {
      case 0: farfree(blks[0]); closemenu(9,3,27,2,w); blk=blks[1];
	for (j=0;j<41;j++) curfil[j]=filname[j]; return;
      case 1: lastb[0]=BACKBL; break;
      case 2: lastb[0]=MONBL;  break;
      case 3: lastb[0]=CHARBL; break;
     }
    curblk[0]=0;
   } while( !(loadblks(temp[0],(char *)blk)) );
  closemenu(9,3,27,2,w); 
  
  GraphMode();
  SetAllPal(colors);
  moucur(0);
  drawsurface(134,125,52,22,FaceCols);
  menub(142,130,FaceCols);
  blk = blks[0];
  drawlistframe(X,Y,LEN,FaceCols);
  drawlist(X,Y,LEN,lastb[0],&curblk[0],blocnum);
  blk=blks[1];
  drawlistframe(X,Y+BLEN+5,LEN,FaceCols);
  drawlist(X,Y+BLEN+5,LEN,lastb[1],&curblk[1],blocnum);

  moucur(1);
  while(1) {
    moustats(&mx,&my,&buts); 
    if (buts) {
      if ( (grabbed != 255)&&( (mx!=oldx)||(my!=oldy) ) ) {
	blk=blks[0];
  drawblk(oldx-BLEN/2,oldy-BLEN/2,&blk[BACKBL].p[0][0]);
  getblk(mx-BLEN/2,my-BLEN/2,&blk[BACKBL].p[0][0]);
  drawblk(mx-BLEN/2,my-BLEN/2,&blk[grabbed].p[0][0]);
	oldx=mx; oldy=my;
        }
      if ( (my>=Y)&(my<Y+BLEN)&(mx>=X)&(mx<X+BLEN*LEN)&(grabbed==255) )
        {
	blk=blks[0];
	moucur(0);
	grabbed= ((mx-X)/BLEN+curblk[0])%lastb[0];
  getblk(mx-BLEN/2,my-BLEN/2,&blk[BACKBL].p[0][0]);
	oldx=mx; oldy=my;
       }
      if ( (my>=Y)&&(my<Y+4+BLEN*2) )
        {
	if ( (my>Y+BLEN+5)||(my<Y+BLEN) )
          {
          j = (my-Y)/(BLEN+2);
	  blk=blks[j];
	  if ( (mx>X-11)&(mx<X-3) )
            {
            moucur(0);
            DrawArrows(X,Y+j*(BLEN+5),LEN,LEFT,FaceCols);
            scroll_list(2,X,Y+j*(BLEN+5),LEN,lastb[j],&curblk[j],blocnum);
            DrawArrows(X,Y+j*(BLEN+5),LEN,NONE,FaceCols);
            if (grabbed==255) moucur(1);
            }
	  if ( (mx>X+BLEN*LEN+2)&(mx<X+10+(BLEN*LEN)) )
            {
	    moucur(0);
            DrawArrows(X,Y+j*(BLEN+5),LEN,RIGHT,FaceCols);
            scroll_list(-2,X,Y+j*(BLEN+5),LEN,lastb[j],&curblk[j],blocnum);
            DrawArrows(X,Y+j*(BLEN+5),LEN,NONE,FaceCols);
	    if (grabbed==255) moucur(1);
	    }
          }
        }
      if ( (mx>142)&(mx<178)&(my>130)&(my<141) )
        {
	TextMode();
	farfree(blks[0]);
	blk = blks[1];
	lastbl = lastb[1];
	for (j=0;j<41;j++) curfil[j]=filname[j];
	return;
        }
      }
    else if (grabbed != 255)
      {
      blk=blks[0];
      drawblk(oldx-BLEN/2,oldy-BLEN/2,&blk[BACKBL].p[0][0]);
      if ( (my >= Y+BLEN+5) & (my < Y+5+BLEN*2) )
        {
	if ( (mx>X)&(mx<X+BLEN*LEN) )
          {
	  blks[1][(((mx-X)/BLEN+curblk[1])%lastb[1])]=blks[0][grabbed];
	  if ( (lastb[1]==BACKBL)&(lastb[0]!=BACKBL) )
	    for (k=0;k<BLEN;k++)
	      for (j=0;j<BLEN;j++)
		if (blks[1][(((mx-X)/BLEN+curblk[1])%lastb[1])].p[j][k]==255)
		  blks[1][(((mx-X)/BLEN+curblk[1])%lastb[1])].p[j][k]=0;
	  blks[1][(((mx-X)/BLEN+curblk[1])%lastb[1])].touchbl=(((mx-X)/BLEN+curblk[1])%lastb[1]);
	  blks[1][(((mx-X)/BLEN+curblk[1])%lastb[1])].nextbl=(((mx-X)/BLEN+curblk[1])%lastb[1]);
	  saved=0;
	  blk=blks[1];
	  drawlist(X,Y+BLEN+5,LEN,lastb[1],&curblk[1],blocnum);
          }
        }
      grabbed=255;
      moucur(1);
      }
    }
  }

static int cvtpal(void)
  {
  unsigned register int  j,k,m;
  RGBdata                c1[256];
  unsigned char          cnvt[256];
  int                    x,y,buts=0;

  if (curfil[4]==0) { errorbox("Choose a Block Set and the original","Palette first!         (O)k"); return(0); } 
  for (j=0;j<256;j++) c1[j] = colors[j];
  if (!loadany("Enter Palette to convert to:","*.pal\0",WorkDir,sizeof(RGBdata)*256,(char*) &colors,0)) { for (j=0;j<256;j++) colors[j] = c1[j]; return(0); }
  GraphMode();
  SetAllPalTo(&c1[BKGCOL]);
  moucur(0);
  drawallblocks();
  BoxWithText(132,96,"Loading",FaceCols);
  FadeTo(c1);
  BoxWithText(132,96,"Working",FaceCols);

  for (j=0;j<255;j++) {
    cnvt[j]=PalFindCol(c1[j].red,c1[j].green,c1[j].blue,colors);
   }
//  if (lastbl!=BACKBL) cnvt[255]=255;   // No longer allow any set to have color 255
  cnvt[255]=255;  
  for(m=0;m<lastbl;m++)      
    for (k=0;k<BLEN;k++)
      for (j=0;j<BLEN;j++)
	blk[m].p[j][k]=cnvt[blk[m].p[j][k]];

  FindEBcols();  // Set the Edit Button Colors to neat colors 
//  SetCols(colors,FaceCols);
  FadeAllTo(c1[BKGCOL]);
  drawallblocks();
  BoxWithText(132,96,"Working",FaceCols);
  FadeTo(colors);
  saved=0;
  while (bioskey(1)) bioskey(0);
  mouclearbut();
  if (lastbl==MONBL) y=25; else y=0;
  BoxWithText(132,96," Done! ",FaceCols);
  drawborder(83,10+y,153,9,5,1,FaceCols);
  BoxWithText(83,10+y,"Hit any key to exit",FaceCols);
  while ( (!bioskey(1))&&(!buts) ) moustats(&x,&y,&buts);
  TextMode();
  return(1);
  }

static void drawallblocks(void)
  {
  int                spo[3][12] = { {0,16,32,48,64,75,86,102,118,134,69,80},
                                    {0,14,28,42,50,58,72,86,46,54,0,0},
                                    {0,10,16,22,28,34,40,13,19,25,31,37} };
  register int j;
  
  switch(lastbl) {
    case BACKBL:
      for (j=0;j<10;j++)
        drawlist(0,j*BLEN,16,lastbl,&spo[0][j],blocnum);
      for (j=10;j<12;j++)
        drawlist(BLEN*10,(j-6)*BLEN,6,lastbl,&spo[0][j],blocnum);
      drawsurface(BLEN*6+2,BLEN*4+2,BLEN*4-4,BLEN*2-4,FaceCols);
      break;
    case MONBL:
      for (j=0;j<8;j++)
        drawlist(BLEN,j*BLEN+BLEN,14,lastbl,&spo[1][j],blocnum);
      for (j=8;j<10;j++)
        drawlist(BLEN*11,(j-4)*BLEN,4,lastbl,&spo[1][j],blocnum);
      drawsurface(BLEN*5+2,BLEN*4+2,BLEN*6-4,BLEN*2-4,FaceCols);
      drawborder(BLEN,BLEN,BLEN*14,BLEN*8,3,3,FaceCols);
      break;
    case CHARBL: 
      for (j=0;j<7;j++)
        drawlist(BLEN*3,j*BLEN+3*BLEN/2,10,lastbl,&spo[2][j],blocnum);
      for (j=7;j<12;j++)
        drawlist(BLEN*10,(j-5)*BLEN+BLEN/2,3,lastbl,&spo[2][j],blocnum);
      drawsurface(BLEN*6+2,BLEN*2+2+BLEN/2,BLEN*4-4,BLEN*5-4,FaceCols);
      drawborder(BLEN*3,BLEN+BLEN/2,BLEN*10,BLEN*7,3,3,FaceCols);
      break;
    }
  }

// Moves every pixel in a block over one pixel in any four directions
// the shifted off line wraps around.
static void shftblk(int dir, int bnum)
  {
  char temp[BLEN];
  int loopx,loopy;
  if (dir==1)  /* Right */
    {
    if (Firstx>-1) Firstx++;
    if (Firstx>=BLEN) Firstx-=BLEN;
    
    for (loopy=0;loopy<BLEN;loopy++)
      temp[loopy]=blk[bnum].p[loopy][BLEN-1];

    for (loopx=BLEN-1;loopx>0;loopx--)
      for (loopy=0;loopy<BLEN;loopy++)
	blk[bnum].p[loopy][loopx]=blk[bnum].p[loopy][loopx-1];
    for (loopy=0;loopy<BLEN;loopy++)
      blk[bnum].p[loopy][0]=temp[loopy];
    }
  if (dir==2)
    {
    if (Firstx>-1) Firstx--;
    if (Firstx<0) Firstx+=BLEN;

    for (loopy=0;loopy<BLEN;loopy++)
      temp[loopy]=blk[bnum].p[loopy][0];

    for (loopx=0;loopx<BLEN-1;loopx++)
      for (loopy=0;loopy<BLEN;loopy++)
	blk[bnum].p[loopy][loopx]=blk[bnum].p[loopy][loopx+1];
    for (loopy=0;loopy<BLEN;loopy++)
      blk[bnum].p[loopy][BLEN-1]=temp[loopy];
    }
  if (dir==3)
    {
    if (Firstx>-1) Firsty--;
    if (Firsty<0) Firsty+=BLEN;

    for (loopx=0;loopx<BLEN;loopx++)
      temp[loopx]=blk[bnum].p[0][loopx];

    for (loopy=0;loopy<BLEN-1;loopy++)
      for (loopx=0;loopx<BLEN;loopx++)
	blk[bnum].p[loopy][loopx]=blk[bnum].p[loopy+1][loopx];
    for (loopx=0;loopx<BLEN;loopx++)
      blk[bnum].p[BLEN-1][loopx]=temp[loopx];
    }
  if (dir==4)
    {
    if (Firstx>-1) Firsty++;
    if (Firsty>=BLEN) Firsty-=BLEN;

    for (loopx=0;loopx<BLEN;loopx++)
      temp[loopx]=blk[bnum].p[BLEN-1][loopx];

    for (loopy=BLEN-1;loopy>0;loopy--)
      for (loopx=0;loopx<BLEN;loopx++)
	blk[bnum].p[loopy][loopx]=blk[bnum].p[loopy-1][loopx];
    for (loopx=0;loopx<BLEN;loopx++)
      blk[bnum].p[0][loopx]=temp[loopx];
    }
  }



static int dokeys(int x,int y,int bnum)
  {
  int upkey=0,lowkey=0;
  int lastcol=255;

  if (btype==1) lastcol=254;  /* So that background blocks do not have clear */

  if (Secondx != -1) ErasePreviousDraw();

  lowkey=bioskey(0);
  upkey = (lowkey>>8)&255;
  lowkey &= 255;
  
  switch (lowkey)
    {
    case '6':
      if ((bioskey(2)&0x03)>0)  /* Shift bnum right */
        {
        shftblk(1,bnum);
        updatescrn(bnum);
        }
      break;
    case '8':
      if ((bioskey(2)&0x03)>0)  /* Shift bnum up */
        {
        shftblk(3,bnum);
        updatescrn(bnum);
        }
      break;
    case '4':
      if ((bioskey(2)&0x03)>0)  /* Shift bnum left */
        {
        shftblk(2,bnum);
        updatescrn(bnum);
        }
      break;
    case '2':
      if ((bioskey(2)&0x03)>0)  /* Shift bnum down */
        {
        shftblk(4,bnum);
        updatescrn(bnum);
        }
      break;

    case 0:
      switch(upkey)
        {
        case 72:        /* Up arrow */
          if ((bioskey(2)&0x03)>0)  /* Shift bnum right */
            {
            shftblk(3,bnum);
            updatescrn(bnum);
            }
          break;
    case 80:        /* down arrow */
      if ((bioskey(2)&0x03)>0)  /* Shift bnum right */
        {
        shftblk(4,bnum);
        updatescrn(bnum);
        }
      break;
    case 77:
      if ((bioskey(2)&0x03)>0)  /* Shift bnum right */
        {
        shftblk(1,bnum);
        updatescrn(bnum);
        }
      else  /* Change Colors */
        {
        if (curcol[0]==lastcol) curcol[0]=0;
        else curcol[0]++;
        if (curcol[1]==lastcol) curcol[1]=0;
        else curcol[1]++;
        colprompt();
        }
      break;
    case 75:
      if ((bioskey(2)&0x03)>0)  /* Rotate big left */
        {
        shftblk(2,bnum);
        updatescrn(bnum);
        }
      else
        {
        if (curcol[0]==0) curcol[0]=lastcol;
        else curcol[0]--;
        if (curcol[1]==0) curcol[1]=lastcol;
        else curcol[1]--;
        colprompt();
        }
      break;
      }
    break;
    case 'Q':
    case 'q':
    case 27:
      return(FALSE);
    case 'F':   /* Flip block over vertically */
    case 'f':
      if (bnum<BACKBL) { flip(bnum); updatescrn(bnum); saved=0;   }
      break;
    case 'S':   /* reflect block (flip horizontal) */
    case 's':
      if (bnum<BACKBL) { reflect(bnum); updatescrn(bnum); saved=0; }
      break;
    case 'R':
    case 'r':
      if (bnum<BACKBL) { rotate(bnum); updatescrn(bnum); saved=0; }
      break;
    case 'E':   /* erase block to specified color */
    case 'e':
      if (bnum<BACKBL) { clearblk(bnum,curcol[cb]); updatescrn(bnum); saved=0; }
      break;
    case 'i':           /* Info on the block */
    case 'I':
      if (bnum<BACKBL) { changeinfo(bnum); }
      break;
    case 'C':        /* make current color color under cursor */
    case 'c':
      moucur(0); 
      oldcmd=20;
      curcol[cb]= GetCol(x,y); 
      colprompt();
      moucur(1); 
      break;
    case '+':
    case '=':
      if (curcol[0]==lastcol) curcol[0]=0; 
      else curcol[0]++;
      if (curcol[1]==lastcol) curcol[1]=0;
      else curcol[1]++;
      colprompt();
      break;
    case '-':
      if (curcol[0]==0) curcol[0]=lastcol;
      else curcol[0]--;
      if (curcol[1]==0) curcol[1]=lastcol;
      else curcol[1]--;
      colprompt();
      break;
    }

  if (Secondx != -1) MoveAnyPoint();
  return(TRUE);
  }

void SetFirstPoint(int x,int y)
  {
  moucur(FALSE);
  Firstx=x; Firsty=y;
  Secondx=-1;
  EraseCtr=0;
  if (EditFn==0) // draw a point
    {
    drawpt(x,y,big,1); 
    saved=0; 
    Firstx=-1;
    }  
  if (EditFn==6) // Fill a region
    {
//    if (Firstx<BLEN)&&(Fir
    HiFill(Firstx,Firsty,big);
    saved=0; 
    Firstx=-1;
    }
  moucur(TRUE);
  }
  
void MoveAnyPoint(void)  // Assumes that you've already erased the line
  {
  moucur(FALSE);
  if (Firstx!=-1)
    {
    if (EditFn==0) // draw a point
      {
      drawpt(Secondx,Secondy,big,1); 
      }  
    else
      {
      EraseCtr=0;
      switch(EditFn)
        {
        case 1:         // Line
          HiLine(Firstx,Firsty,Secondx,Secondy,big,1);
          break;
        case 2:
          HiBox(Firstx,Firsty,Secondx,Secondy,big,1);
          break;
        case 3:
          HiBoxFill(Firstx,Firsty,Secondx,Secondy,big,1);
          break;
        case 4:
          HiCircle(Firstx,Firsty,Secondx,Secondy,big,1);
          break;
        case 5:
          HiCircleFill(Firstx,Firsty,Secondx,Secondy,big,1);
          break;
        case 6:
          HiFill(Firstx,Firsty,big);
          break;
        }
      }
    }
  saved=0; 
  moucur(TRUE);
  }

  
void MoveSecondPoint(int x,int y)
  {
  moucur(FALSE);
  if (Firstx!=-1)
    {
    if (Secondx!=-1) ErasePreviousDraw();
    Secondx=x;
    Secondy=y;
    EraseCtr=0;
    switch(EditFn)
      {
      case 1:         // Line
        HiLine(Firstx,Firsty,x,y,big,1);
        break;
      case 2:
        HiBox(Firstx,Firsty,Secondx,Secondy,big,1);
        break;
      case 3:
        HiBoxFill(Firstx,Firsty,Secondx,Secondy,big,1);
        break;
      case 4:
        HiCircle(Firstx,Firsty,Secondx,Secondy,big,1);
        break;
      case 5:
        HiCircleFill(Firstx,Firsty,Secondx,Secondy,big,1);
        break;
      }
    }
  moucur(TRUE);
  }

void ErasePreviousDraw(void)
  {
  int m;
  
  m=moucur(2);
  if (m) moucur(FALSE);
  EraseCtr=0;
  switch(EditFn)
    {
    case 1:         // Line
      HiLine(Firstx,Firsty,Secondx,Secondy,big,2);
      break;
    case 2:
      HiBox(Firstx,Firsty,Secondx,Secondy,big,2);
      break;
    case 3:
      HiBoxFill(Firstx,Firsty,Secondx,Secondy,big,2);
      break;
    case 4:
      HiCircle(Firstx,Firsty,Secondx,Secondy,big,2);
      break;
    case 5:
      HiCircleFill(Firstx,Firsty,Secondx,Secondy,big,2);
      break;
    }
  if (m) moucur(TRUE);
  }


static int domouse(int x,int y,int buts,int *bnum,int *grabbed)
  {                              /* returns the block # pointed to, and */
                                 /* the pointed to x,y loc. in the block */

  static int oldx=0,oldy=0,oldbuts=0;
  int temp;
  int lastcol=255;

  if (btype==1) lastcol=254;  

  *bnum=0;
  
  if (InBox(x,y,EBX,EBY,EBX1,EBY1))  // Choosing an edit button
    {
    temp = (x-EBX)/EBLEN;          // translate x so it = a button number
    if ((temp!=EditFn)&&(buts>0))  // Do nothing if he chooses the same button
      {
      moucur(FALSE);
      DrawFnBut(EditFn,EBX+(EditFn*EBLEN),EBY,EBcols[0]); //Unhighlight old button
      EditFn=temp;
      DrawFnBut(EditFn,EBX+(EditFn*EBLEN),EBY+1,EBcols[1]); //Highlight new button
      moucur(TRUE);
      }
    }
  if (InBox(x,y,KBX,KBY,KBX1,KBY1))  // Choosing a key tool
    {
    temp = (x-KBX)/KBLEN;          // translate x so it = a button number
    if ((buts>0)&&(oldbuts==0))  // Do nothing if he chooses the same button
      {
      moucur(FALSE);
      DrawKeyBut(temp,KBX+(temp*KBLEN),KBY,EBcols[1]); //Highlight new button
      switch(temp)
        {
        case 0:        // Shift the block up
          shftblk(3,big);
          saved=0;
          break;
        case 1:        // Shift the block down
          shftblk(4,big);
          saved=0;
          break;
        case 2:        // Shift the block right
          shftblk(1,big);
          saved=0;
          break;
        case 3:      // Shift the block left
          shftblk(2,big);
          saved=0;
          break;
        case 4:   // Rotate block around center
          if (big<BACKBL) { rotate(big); saved=0; }
          break;
        case 5:   // reflect block (flip horizontal) 
          if (big<BACKBL) { reflect(big); saved=0; }
          break;
        case 6:   // Flip block over vertically 
          if (big<BACKBL) { flip(big); saved=0;   }
          break;
        case 7:
          if (curcol[0]==lastcol) curcol[0]=0; 
          else curcol[0]++;
          if (curcol[1]==lastcol) curcol[1]=0;
          else curcol[1]++;
          colprompt();
          break;
        case 8:
          if (curcol[0]==0) curcol[0]=lastcol;
          else curcol[0]--;
          if (curcol[1]==0) curcol[1]=lastcol;
          else curcol[1]--;
          colprompt();
          break;
        }
      updatescrn(big); 
      DrawKeyBut(temp,KBX+(temp*KBLEN),KBY,EBcols[0]); //Highlight new button
      moucur(TRUE);
      }
    oldx=x; oldy=y; oldbuts=buts;
    }
  if (InBox(x,y,BX,BY,BX1-1,BY1-1)) // In blow-up
    {
    *bnum=big;
    if ((*grabbed)==-1)  // If not carrying a block
      {
      if (buts>0)  // Button pressed, start or move the new line,circle,etc.
        {
        cb=buts-1;
        x=(x-BX)/5;
        y=(y-BY)/5;
        if ((oldbuts==0)||(oldcmd!=1)||((Firstx==-1)&&((x!=oldx)||(y!=oldy))))
          {
          SetFirstPoint(x,y);
          }
        else if ((x!=oldx)||(y!=oldy))
          {
          MoveSecondPoint(x,y);
          }
        oldcmd=1;
        oldx=x; oldy=y; oldbuts=buts;
        }
      else   // No Buttons Pressed.
        {
        if (Firstx==-1) saved=0; // Something has been drawn, so change the save flag
        Firstx=-1;
        oldbuts=buts;
        }
      }
    else
      {
      if (buts==0)
        {
        *bnum=*grabbed;
        big=*grabbed;
        drawbig();
        *grabbed = -1;
        moucurbox(0,0,319,199);
        oldcmd=1;
        oldx=x; oldy=y; oldbuts=buts;
        }
      }
    oldbuts=buts;
    return(TRUE);
    }

  if (InBox(x,y,COLX,COLY,COLX+lastcol,COLY+10)) // Select Palette Color
    {
    if (((oldcmd!=2)|(oldx!=x)|(oldbuts!=buts))&(buts>0)) 
      {
      cb=buts-1;
      curcol[cb]=x-COLX;
      colprompt();
      oldx=x; oldy=y; oldbuts=buts; 
      oldcmd=2;    
      return(TRUE);
      }
    }

  if (InBox(x,y,GX,GY,GX1,GY1))         // In Grid
    {
    x=(x-GX)/BLEN;
    y=(y-GY)/BLEN;

    *bnum=grid[y][x];
    if ((*grabbed)==-1)
      {
      if ((buts>0)&(oldbuts==0))    // Pick up Block
        {  
      	*grabbed=grid[y][x];
        moucurbox(BLEN/2,BLEN/2,319-BLEN/2,199-BLEN/2);
        }
      }
    else
      {
      if (buts==0)
        {
        grid[y][x]=*grabbed;
        moucur(FALSE);
        drawblk(x*BLEN+GX,y*BLEN+GY,&blk[grid[y][x]].p[0][0]);
        moucur(TRUE);
        *grabbed=-1;
        moucurbox(0,0,319,199);
        }
      }
    oldx=x; oldy=y; oldbuts=buts;
    oldcmd=10;
    return(TRUE);
    }

  if (InBox(x,y,LX,LY,LX1,LY1)) // In list
    {
    x=(x-LX)/BLEN;
    y=(y-LY)/BLEN;
    (*bnum)=(list+x)%lastbl;
    if (*grabbed==-1)
      {
      if ((buts>0)&(oldbuts==0))   // Pick up Block
        {
        *grabbed=(list+x)%lastbl;
        moucurbox(BLEN/2,BLEN/2,319-BLEN/2,199-BLEN/2);
        }
      }
    else
      {
      if ((buts==0)&(oldbuts!=0)) // Drop block
        {
        moucur(FALSE);
        copyblock(*grabbed,(list+x)%lastbl);
        updatescrn((list+x)%lastbl);
        saved=0;
        *grabbed=-1;
        moucurbox(0,0,319,199);
        moucur(TRUE);
        }
      }
    oldcmd=4; oldx=x; oldy=y; oldbuts=buts;
    return(TRUE);
    }   
  if (buts>0)
    {
    if (InBox(x,y,LX1+2,LY,LX1+11,LY1))
      { moucur(0); DrawArrows(LX,LY,LLEN,RIGHT,FaceCols); scroll_list(-4,LX,LY,LLEN,lastbl,&list,blocnum); DrawArrows(LX,LY,LLEN,NONE,FaceCols); moucur(1); oldcmd=13; return(TRUE); }
    if (InBox(x,y,LX-12,LY,LX-3,LY1))
      { moucur(0); DrawArrows(LX,LY,LLEN,LEFT,FaceCols); scroll_list(4,LX,LY,LLEN,lastbl,&list,blocnum); DrawArrows(LX,LY,LLEN,NONE,FaceCols); moucur(1); oldcmd=14; return(TRUE); }
    if (*grabbed==-1)
      {
      if (InBox(x,y,COLPX1,COLPY,COLPX1+COLPLEN,COLPY+COLPWID+10)) {oldcmd=16; GetCurCol(0); return(TRUE);}
      if (InBox(x,y,COLPX2,COLPY,COLPX2+COLPLEN,COLPY+COLPWID+10)) {oldcmd=17; GetCurCol(1); return(TRUE);}
      if (InBox(x,y,MENUX,MENUY,MENUX+35,MENUY+10))  { oldcmd=12; return(FALSE); }
      if (InBox(x,y,INFOX,INFOY,INFOX+35,INFOY+10))  { oldcmd=15; changeinfo(big); return(TRUE); }
      if (InBox(x,y,ANIX ,ANIY ,ANIX+35 ,ANIY+10 ))
        {
        if (oldbuts==0)
          {
          oldcmd=18;
          Animate ^=1;
          if (!Animate)
            {
            moucur(FALSE);
            AniBut(ANIX,ANIY,FaceCols);
            moucur(TRUE);
            InitAni();
            drawgrid();
            }
          else
            {
            moucur(FALSE);
            AniBut(ANIX,ANIY+1,HiCols);
            Line(ANIX,ANIY,ANIX+35,ANIY,FaceCols[GREY1]);
            moucur(TRUE);
            }
          }
        }
      if (InBox(x,y,HELPX,HELPY,HELPX+35,HELPY+10))
        {
        moucur(0);
        TextMode();
        DisplayHelpFile(HELPFILE);
        drawscrn();
        mouclearbut();
        oldcmd=11;
        oldbuts=FALSE;
        return(TRUE);
        }
      }
    oldbuts=buts;
    }
  
  if ((buts==0)&(*grabbed!=-1)) // Drop block nowhere if button let go
    {
    *grabbed=-1;
    moucurbox(0,0,319,199);
    }
   
  oldbuts=buts;
  return(TRUE);  // FALSE = mainmenu, TRUE = continue editing
  }

static void GetCurCol(char button)
  {
  int x,y,but,count,pointer=1;
  
  mouclearbut();
  do {
    moustats(&x,&y,&but);
    count++;
    if (count>3000)
      {
      count=0;
      curcol[button]++;
      if (curcol[button]>=MAXCOL) curcol[button]=0;
      if ((x>=COLPX1-10)&&(x<COLPX2+COLPLEN)&&(y>=COLPY-10)&&(y<COLPY+COLPWID+10))
        { moucur(0); pointer=0; }
      colprompt();
      if (!pointer) { moucur(1); pointer=1; }
      }
    if (but)
      {
      moucur(0); 
      curcol[button]= GetCol(x,y); 
      colprompt();
      moucur(1);
      }
    } while (but==0);
  mouclearbut();
  }

static void flip(int bloc)
  {
  register int lx,ly;
  int temp;
  
  if (Firstx>-1) Firsty = BLEN-1-Firsty;

  for (ly=0;ly<10;ly++)
    for (lx=0;lx<20;lx++)
      {
      temp=blk[bloc].p[ly][lx];
      blk[bloc].p[ly][lx]=blk[bloc].p[19-ly][lx];
      blk[bloc].p[19-ly][lx]=temp;
      }    
  } 
 
static void reflect(int bloc)
  {
  register int lx,ly;
  int temp;  

  if (Firstx>-1) Firstx = BLEN-1-Firstx;

  for (lx=0;lx<10;lx++)
    for (ly=0;ly<20;ly++)
      {
      temp=blk[bloc].p[ly][lx];
      blk[bloc].p[ly][lx]=blk[bloc].p[ly][19-lx];
      blk[bloc].p[ly][19-lx]=temp;
      }    
  } 

static void rotate(int bloc)
  {
  register int lx,ly;
  
  if (Firstx>-1) 
    {
    lx = Firstx;
    Firstx = Firsty;
    Firsty = BLEN-1-lx;
    }
    
  for (lx=0;lx<20;lx++)
    for (ly=0;ly<20;ly++)
      blk[lastbl].p[ly][lx]=blk[bloc].p[lx][BLEN-1-ly];
  copyblock(lastbl,bloc);
  } 

  
static int chooselist(int num)
  {
  if (grabbed==-1) grabbed= num;
  else   // copy block (user has chosen a block, moved somewhere, and let go)
    {
    moucur(FALSE);
    copyblock(grabbed,num);
    drawlist(LX,LY,LLEN,lastbl,&list,blocnum);
    drawgrid();
    if (num==big) drawbig();
    moucur(TRUE);
    grabbed=-1;
    }
  return(1);
  }

static void CopyInfo(int from,int to)
  {
  unsigned char *fromPtr,*toPtr;
  int loop;

  fromPtr=blk[from].p[0];
  fromPtr+=(BLEN*BLEN);
  toPtr=blk[to].p[0];
  toPtr+=(BLEN*BLEN);

  for (loop=0;loop<sizeof(blkstruct)-(BLEN*BLEN);loop++,toPtr++,fromPtr++)
    *toPtr=*fromPtr;

  if (blk[from].touchbl==from) blk[to].touchbl=to;
  if (blk[from].nextbl==from) blk[to].nextbl=to;
  }

static void copyblock(int from, int to)
  {
  register int yl,xl;

  for (xl=0;xl<BLEN;xl++)
    {
    for (yl=0;yl<BLEN;yl++)
      {
      blk[to].p[yl][xl] = blk[from].p[yl][xl];
      }
    }
  }

static void changeblk(int x,int y)
  {
  grid[y][x] = grabbed;
  moucur(FALSE);
  drawblk((x*BLEN)+GX,(y*BLEN)+GY,&blk[grid[y][x]].p[0][0]);
  moucur(TRUE);
  }

static char birthmon(int choice)         //Have block create a monster
  {
  unsigned register int  j;
  blkstruct far          *blk2;
  int                    curmon;
  int                    x,y,buts,inkey,blocsize;
  char                   tempname[41];

  strcpy((char*)tempname,(char*)curfil);
  blk2=blk;
  blocsize=lastbl;
  TextMode();                                   //Load monster information
  if (!loadany("Enter name of monster set to load: ","*.mon\0",WorkDir,sizeof(monstruct)*LASTMON,(char *)&m,0)) { GraphMode(); return(0); }
  blk= (blkstruct far *) farmalloc(sizeof(blkstruct)*(MONBL+1));
  if (blk==NULL) 
    { errorbox("NOT ENOUGH MEMORY!","  (M)enu"); blk=blk2; GraphMode(); return(0);}
  for (j=0; j<sizeof(blkstruct)*(MONBL+1);j++) // Initalize new block set
    *( ((unsigned char far *) blk)+j)=0;
  if (!loadblks(2,(char*)blk))
    { errorbox("Could not load block set!","  (M)enu");
      farfree(blk); blk=blk2; GraphMode(); return(0); }

  GraphMode();
  moucur(0);
  SetAllPalTo(&colors[BKGCOL]);
  BoxFill(0,0,SIZEX-1,SIZEY-1,BKGCOL);
  drawlistframe(MOX,MOY2,MOLEN,FaceCols);
  drawborder(SELX,SELY,BLEN,BLEN,2,2,FaceCols);
  drawsurface(MX3-2,MY3-1,40,27,FaceCols);
  noneb(MX3,MY3,FaceCols);
  doneb(MX3,MY3+13,FaceCols);
  moucurbox(0,0,319,199);
  drawlist(MOX,MOY2,MOLEN,LASTMON,&curmon,monnum);
  if (choice==LASTMON)
    { BoxFill(SELX,SELY,SELX+BLEN-1,SELY+BLEN-1,FaceCols[GREY2]); curmon=0; }
  else
    { drawblk(SELX,SELY,&blk[m[choice].pic[m[choice].cur.pic][0]].p[0][0]); curmon=choice; }
  drawborder(TBOXX,TBOXY,169,9,5,2,FaceCols);
  BoxWithText(TBOXX,TBOXY,"Pick Monster to Birth",FaceCols);
  FadeTo(colors);
  moucur(1);
  while(TRUE)
    {
    moustats(&x,&y,&buts);
    if (animons())   // Animate all of the monsters
      {
      // the -10,-15 are the cursor dimensions.
      if (InBox(x,y,MOX-10,MOY2-15,MOX+(MOLEN*BLEN),MOY2+BLEN)) moucur(0);
      drawlist(MOX,MOY2,MOLEN,LASTMON,&curmon,monnum);
      if (choice!=LASTMON)
        {
        if (InBox(x,y,SELX-10,SELY-15,SELX+BLEN,SELY+BLEN)) moucur(0);
        drawblk(SELX,SELY,&blk[m[choice].pic[m[choice].cur.pic][0]].p[0][0]);
        }
      moucur(1);
      }
    if (buts) 
      {
      if ( (x>MX3)&&(x<MX3+36) )
        {
        if ( (y>MY3+12)&&(y<MY3+22) )
          {
          strcpy((char*)curfil,(char*)tempname);
          mouclearbut();
          farfree(blk);
          blk=blk2; lastbl=blocsize;
          return(choice);
          }
        if ( (y>MY3)&&(y<MY3+10) )
          {
          BoxFill(SELX,SELY,SELX+BLEN-1,SELY+BLEN-1,FaceCols[GREY2]);
          choice=LASTMON;
          saved=0;
          }
        }
      if ( (y>=MOY2)&&(y<MOY2+BLEN) )
        {
        moucur(0);
        if ( (x>=MOX)&&(x<MOX+MOLEN*BLEN) )
          {
          choice = (curmon+(x-MOX)/BLEN)%LASTMON;
          drawblk(SELX,SELY,&blk[m[choice].pic[m[choice].cur.pic][0]].p[0][0]);
          saved=0;
          }
        if ( (x>MOX-11)&&(x<MOX-3) )
          { DrawArrows(MOX,MOY2,MOLEN,LEFT,FaceCols); scroll_list(2,MOX,MOY2,MOLEN,LASTMON,&curmon,monnum); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        if ( (x<MOX+BLEN*MOLEN+10)&&(x>MOX+BLEN*MOLEN+2) )
          { DrawArrows(MOX,MOY2,MOLEN,RIGHT,FaceCols); scroll_list(-2,MOX,MOY2,MOLEN,LASTMON,&curmon,monnum); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        moucur(1);
        }
      }
    }
  return(0);  // Will never happen, but removes warning.
  }

static void initalldata(void)
  {
  register int lx,ly,l1;

  for (l1=0; l1<BACKBL;l1++) 
    { 
    clearblk(l1,0);
    blk[l1].Xtra[0]=0;
    blk[l1].Xtra[1]=0;
    blk[l1].Birthmon=LASTMON;
    blk[l1].Birthtime=1;
    blk[l1].hitpt=0; 
    blk[l1].scorepts=0;
    blk[l1].chhitpt=0;        /* Amount to change character points */
    blk[l1].ntime=0;          /* Change block time value, 0=never change */
    blk[l1].obj=FALSE;        /* if character can pick it up */
    blk[l1].eamt=0;           /* how much affect repetition counter has */
    blk[l1].solid=15;
    blk[l1].grav=15;
    blk[l1].touchbl=l1;
    blk[l1].nextbl=l1;
    blk[l1].effect=1;
    }
  cleargrid();
  big=0;
  list=0;
  curcol[0]=0;
  curcol[1]=0;
  curcol[2]=0;
  }

static void cleargrid(void)
  {
  register int ly,lx;

  for (lx=0; lx<GLEN;lx++)
    for (ly=0;ly<GLEN;ly++)
      grid[ly][lx] = 0;
  }


static void clearblk(int num, int color)
  {
  register int lx,ly;

  for(lx=0;lx<BLEN;lx++)
    for (ly=0;ly<BLEN;ly++)
      blk[num].p[ly][lx]=color;
  }


/*----------------------------Screen Modification Routines------------------*/


static void drawpt(int x,int y,int num,int fn=1) /* draws a point in every instance of a */
  {                                 /* block.                               */
  int lx,ly;
  unsigned char col;

  if ((x>=BLEN)||(x<0)||(y>=BLEN)||(y<0)) return;

  if (fn ==1) 
    { 
    col = curcol[cb]; 
    EraseCols[EraseCtr] = blk[num].p[y][x]; 
    EraseCtr++; 
    }
  if (fn == 2) 
    { 
    col = EraseCols[EraseCtr]; 
    EraseCtr++;
     
    }
  if (EraseCtr>BLEN*BLEN+5) EraseCtr=BLEN*BLEN+5;
  blk[num].p[y][x] = col;

  BoxFill ( (x*5)+BX+1, (y*5)+BY+1, (x*5)+BX+4, (y*5)+BY+4,col);
  for (lx=0; lx<GLEN; lx++)
    for (ly=0; ly<GLEN; ly++)
      if (grid[ly][lx] == num) Point(GX+(BLEN*lx)+x,GY+(BLEN*ly)+y,col);
  if ((list<=num)&(list+LLEN>=num))
     Point(LX+(BLEN*(num-list))+x,LY+y,col);
  if ((list+LLEN>lastbl-1)&( ((list+LLEN)%lastbl)>num))
     {
     lx=lastbl-list+num;
     Point(LX+(BLEN*lx)+x,LY+y,col);
     }
  }

static void drawscrn(void)
  {
  register int ly,lx;
  
  moucur(0);
  GraphMode();
  SetAllPalTo(&colors[BKGCOL]);
  BoxFill(0,0,SIZEX-1,SIZEY-1,BKGCOL);
  drawborder(COLX,COLY,MAXCOL+1,11,4,1,FaceCols); //  Draw palette
  drawpal();
                                                  //  Draw selected color box
  drawborder(COLPX1,COLPY,2*COLPLEN+3,COLPWID+12,2,2,FaceCols);
  Box(COLPX1+COLPLEN,COLPY-1,COLPX1+COLPLEN+1,COLPY+COLPWID-1,FaceCols[GREY1]);
  Line(COLPX1+COLPLEN+2,COLPY,COLPX1+COLPLEN+2,COLPY+COLPWID-1,FaceCols[GREY3]);
  BoxFill(COLPX1-1,COLPY+COLPWID,COLPX2+COLPLEN,COLPY+COLPWID+11,FaceCols[GREY1]);
  colprompt();

  drawsurface(BX-2,BY-26,105,141,FaceCols);             //  Draw zoom window
  Box(BX,BY,BX+(BLEN*5),BY+(BLEN*5),FaceCols[GREY4]);
  BoxFill(BX+1,BY+1,BX+(BLEN*5)-1,BY+(BLEN*5)-1,FaceCols[BLACK]);
  for (ly=BY+11;ly<BY1;ly+=25)
       { Line (BX,ly,BX,ly+3,FaceCols[GREY1]);
				 Line (BX1,ly,BX1,ly+3,FaceCols[GREY1]); }
  for (lx=BX+11;lx<BX1;lx+=25)
       { Line (lx,BY,lx+3,BY,FaceCols[GREY1]);
				 Line (lx,BY1,lx+3,BY1,FaceCols[GREY1]); }
  drawbig();
                                                 //  Draw the tool buttons
  BoxFill(KBX-1,EBY,KBX1-1,KBY1+1,FaceCols[GREY3]);
  for (ly=0;ly<LASTEDITFN;ly++)
    DrawFnBut(ly,EBX+(ly*EBLEN),EBY,EBcols[0]);
  DrawFnBut(EditFn,EBX+(EditFn*EBLEN),EBY+1,EBcols[1]); // Highlighted button
  for (ly=0;ly<LASTKEYBUT;ly++)
    DrawKeyBut(ly,KBX+(ly*KBLEN),KBY,EBcols[0]);

  drawborder(GX,GY,BLEN*GLEN,BLEN*GLEN,2,2,FaceCols);   //  Draw the grid box
  drawgrid();

  drawlistframe(LX,LY,LLEN,FaceCols);                   //  Draw the bloc list
  drawlist(LX,LY,LLEN,lastbl,&list,blocnum);

  drawsurface(HELPX-3,HELPY-2,42,52,FaceCols);          //  Draw the menu
  helpb(HELPX,HELPY,FaceCols);
  menub(MENUX,MENUY,FaceCols);
  infob(INFOX,INFOY,FaceCols);
  if (Animate)
    {
    Line(ANIX,ANIY,ANIX+35,ANIY,FaceCols[GREY1]);
    AniBut(ANIX,ANIY,HiCols);
    }
  else AniBut(ANIX,ANIY,FaceCols);

  moucurbox(0,0,319,199);
  FadeTo(colors,FADETIME);
  moucur(TRUE);
  }

/*
static void BoxWithText(int x,int y,char *str)
  {
  Line(x-1,y-1,x+strlen(str)*8,y-1,FaceCols[GREY4]);
  Line(x-1,y,x-1,y+8,FaceCols[GREY3]);
  BoxFill(x,y,x+strlen(str)*8,y+8,FaceCols[GREEN]);
  GWrite(x+1,y+1,FaceCols[BLACK],str);
  }

static void DrawArrows(char selected)
  {
  unsigned char left1,left2,left3,right1,right2,right3;
  int lp;
  
  switch(selected)
    {
    case 1:  left1=FaceCols[GREY1]; left2=(left3=FaceCols[RED3]); right1=(right2=FaceCols[RED1]); right3=FaceCols[RED2]; break;
    case 2:  right1=FaceCols[GREY1]; right2=(right3=FaceCols[RED3]); left1=(left2=FaceCols[RED1]); left3=FaceCols[RED2]; break;
    case 0:  
    default: left1=(right1=(left2=(right2=FaceCols[RED1]))); left3=(right3=FaceCols[RED2]); break;
    }
  for (lp=0;lp<7;lp++)
    {
    Line(lp+LX-11,LY+BLEN/2-lp,lp+LX-11,LY+BLEN/2+lp,left2);
    Line(LX1+10-lp,LY+BLEN/2-lp,LX1+10-lp,LY+BLEN/2+lp,right2);
    Point(lp+LX-11,LY+BLEN/2-lp,left1);
    Point(lp+LX-11,LY+BLEN/2+lp,left1);
    Point(LX1+10-lp,LY+BLEN/2-lp,right1);
    Point(LX1+10-lp,LY+BLEN/2+lp,right1);
    }
  Line(LX-4,LY+BLEN/2-6,LX-4,LY+BLEN/2+6,left3);
  Line(LX1+3,LY+BLEN/2-6,LX1+3,LY+BLEN/2+6,right3);
  }

    
static void drawborder(int x,int y,int xlen,int ylen,int xthick,int ythick)
  {   //Draw border AROUND object at x,y with dimensions xlen,ylen
  Box(x-xthick-3,y-ythick-3,x+xlen+xthick+1,y+ythick+ylen+1,FaceCols[GREY3]);
  Box(x-xthick-2,y-ythick-2,x+xlen+xthick,y+ythick+ylen,FaceCols[GREY2]);
  BoxFill(x-xthick-1,y-ythick-1,x-1,y+ylen+ythick-1,FaceCols[GREY1]);
  BoxFill(x+xlen,y-ythick-1,x+xlen+xthick-1,y+ylen+ythick-1,FaceCols[GREY1]);
  if (xlen>0)
    {
    BoxFill(x,y-ythick-1,x+xlen,y-2,FaceCols[GREY1]);
    BoxFill(x,y+ylen,x+xlen,y+ylen+ythick-1,FaceCols[GREY1]);
    Line(x-1,y-1,x+xlen-1,y-1,FaceCols[GREY4]);
    }
  if (ylen>0) Line(x-1,y,x-1,y+ylen-1,FaceCols[GREY3]);
  }
*/  

static void drawbig(void)
  {
  char str[41];
  register int lx,ly;
  int m;
  
  m=moucur(2);
  if (m) moucur(FALSE);
  for (ly=BY;ly<BY1;ly+=5)
    for (lx=BX;lx<BX1;lx+=5)
      BoxFill(lx+1,ly+1,lx+4,ly+4,blk[big].p[(ly-BY)/5][(lx-BX)/5]);

  sprintf(str,"Block #%3d",big); 
  BoxWithText(BX+7,BY1+4,str,FaceCols);
  if (m) moucur(TRUE);
  }

static void drawpal(void)
  {
  register int lx;
  int m;
  m=moucur(2);
  
  if (m) moucur(0);
  for (lx = COLX;lx<COLX+MAXCOL+1; lx++) Line (lx,COLY,lx,COLY+10,lx-COLX);
  if (m) moucur(1); 
  }

static void drawgrid(void)
  {
  register int ly,lx;
  int m;

  m = moucur(2);
  if (m) moucur(0);

  for (lx=GX;lx<GX1;lx+=BLEN)
    for (ly=GY; ly<GY1;ly+=BLEN)
      drawblk(lx,ly,&blk[grid[(ly-GY)/BLEN][(lx-GX)/BLEN]].p[0][0]);
  if (m) moucur(1);
  }

static void InitAni(void)
  {
  register int loop;
  for (loop=0;loop<lastbl;loop++)
    {
    if (blk[loop].ntime>0) nxtchg[loop]=Clock+blk[loop].ntime;
    else                   nxtchg[loop]=1;  // Never change
    blkmap[loop]=blk[loop].nextbl;
    }
  }

static void AnimateGridBlocks(int ox,int oy)
  {
  int lx,ly;
  int bl;
  if (InBox(ox,oy,GX,GY,GX1,GY1))
    {
    moucur(FALSE);
    if (grabbed!=-1) drawblk(ox-10,oy-10,&blk[BACKBL].p[0][0]);
    }
  for (lx=0;lx<GLEN;lx++)
    for (ly=0; ly<GLEN;ly++)
      {
      bl=grid[ly][lx];
      if ((nxtchg[bl]<=Clock)&&(blk[bl].ntime!=0)&&(blk[bl].nextbl!=bl))
        drawblk((lx*BLEN)+GX,(ly*BLEN)+GY,&blk[blkmap[grid[ly][lx]]].p[0][0]);
      }
  for (lx=0;lx<lastbl;lx++)
    {
    if ((nxtchg[lx]<=Clock)&&(blk[lx].ntime!=0))
      {
      nxtchg[lx]=Clock+blk[blkmap[lx]].ntime;
      blkmap[lx]=blk[blkmap[lx]].nextbl;
      }
    }
  if (InBox(ox,oy,GX,GY,GX1,GY1))
    {
    moucur(TRUE);
    if (grabbed!=-1) drawblk(ox-10,oy-10,&blk[grabbed].p[0][0]);
    }
  }

static void updatescrn(int bloc)  /* redraws every instance of block 'bloc' on screen */
  {
  int lx,ly;
  int m;

  if (bloc==big) drawbig();   /* redraw zoom if changed */
  m=moucur(2);
  if (m) moucur(FALSE);
  for (lx=0; lx<GLEN; lx++)   /* redraw blocks in the grid */
    for (ly=0; ly<GLEN; ly++)
      if (grid[ly][lx] == bloc) drawblk(GX+(BLEN*lx),GY+(BLEN*ly),&blk[bloc].p[0][0]);

  if ((list<=bloc)&(list+LLEN>=bloc)) drawblk(LX+(BLEN*(bloc-list)),LY,&blk[bloc].p[0][0]);

  if ((list+LLEN>lastbl-1)&( ((list+LLEN)%lastbl)>bloc))
     {
     lx=lastbl-list+bloc;
     drawblk(LX+(lx*BLEN),LY,&blk[bloc].p[0][0]);
     }
  if (m) moucur(TRUE);
  }

static void colprompt(void)
  {
  unsigned char cols[4];
  char str[4];
  int m;

  cols[0]=FaceCols[BLACK]; cols[1]=FaceCols[GREEN]; cols[2]=FaceCols[GREY4]; cols[3]=FaceCols[GREY4];

  m=moucur(2);

  if (m) moucur(FALSE); 
  BoxFill(COLPX1,COLPY,COLPX1+COLPLEN-1,COLPY+COLPWID-1,curcol[0]); 
  if (curcol[0]!=TRANSCOL)
    {
    sprintf(str,"%3d",curcol[0]);
    BoxWithText(COLPX1+2,COLPY+COLPWID+3,str,FaceCols);
    }
  else DrawClearBMP(COLPX1+2,COLPY+COLPWID+3,cols);

  BoxFill(COLPX2,COLPY,COLPX2+COLPLEN-1,COLPY+COLPWID-1,curcol[1]);
  if (curcol[1]!=TRANSCOL)
    {
    sprintf(str,"%3d",curcol[1]);
    BoxWithText(COLPX2+2,COLPY+COLPWID+3,str,FaceCols);
    }
  else DrawClearBMP(COLPX2+2,COLPY+COLPWID+3,cols);

  if (m) moucur(TRUE);
  }

int blocnum(int mun)
 {
 return(mun);
 }


// Drawfn = 
// 0 = Point
// 1 = Line
// 2 = Box
// 3 = BoxFill
// 4 = Circle
// 5 = CircleFill
// 6 = Fill
//GAS
static void DrawFnBut(int drawfn,int x,int y, unsigned char col[4])
  {
  char array[][26]= {"   UPUAUT¥AZTUAUT5UÀÿð", 
                     "   UPUA¥T¥AU¤UAUT5UÀÿð",
                     "   UP" "\x1A" "ªA•dVA•dVAª¤5UÀÿð",
                     "   UP" "\x1A" "ªAª¤" "\x1A" "ªAª¤" "\x1A" "ªAª¤5UÀÿð",
                     "   UPUAZTYAe”¥AUT5UÀÿð",
                     "   UPUAZT©Aj”¥AUT5UÀÿð",
                     "   ª *ª‚ª¨*ª‚ª¨*ª‚ª¨:ªÀÿð"};

  draw4dat(x,y,array[drawfn],9,9,col);
  }

// up, down,right,left,rotate,h swap, v swap,+,-

void DrawKeyBut(int butnum,int x,int y, unsigned char col[4])
  {
  char array[][26]= {"   UP¥Aj”" "\x1A" "ªAZT¥AZT5UÀÿð",
                     "   UP¥AZT¥Aª¤©AZT5UÀÿð",
                     "   UPeAV”" "\x1A" "ªAª¤iAVT5UÀÿð",
                     "   UP•AiT" "\x1A" "ªAª¤•AYT5UÀÿð",
                     "   UP•AiT" "\x1A" "ªAid–AUd5UÀÿð",
                     "   UPeAª”eAYTªAYT5UÀÿð",
                     "   UPUAe”jA©”YAU”5UÀÿð",
                     "   UP¥AZT" "\x1A" "ªAª¤¥AZT5UÀÿð",
                     "   UPUAUT" "\x1A" "ªAª¤UAUT5UÀÿð"};

  draw4dat(x,y,array[butnum],9,9,col);
  }


static void FindEBcols(void)
  {
  SetCols(colors,FaceCols);

  EBcols[0][0]=FaceCols[GREY3]; //UNSELETED BUTTONS:    0==background==dark grey
  EBcols[0][1]=FaceCols[RED1];  //1==forground==medium red
  EBcols[0][2]=FaceCols[BLACK]; //2==picture==FaceCols[BLACK]
  EBcols[0][3]=FaceCols[RED2];  //3==button side==dark red

  EBcols[1][0]=FaceCols[GREY3]; //SELETED BUTTON:       0==background==dark grey
  EBcols[1][1]=FaceCols[RED3];  //1==forground==bright red
  EBcols[1][2]=FaceCols[GREEN]; //2==picture==greenish
  EBcols[1][3]=FaceCols[GREY3]; //3==button edge(==background)==dark grey
  }

void HiLine(int x1,int y1,int x2,int y2, unsigned char bloc,int drawfn)
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

  drawpt(x1,y1,bloc,drawfn);

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
      drawpt(x1,y1,bloc,drawfn);
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
      drawpt(x1,y1,bloc,drawfn);
      }
    }
  }

static void NoDrawPt(int x,int y,int bloc)
  {
  if ((x<BLEN)&&(x>=0)&&(y<BLEN)&&(y>=0)) blk[bloc].p[y][x] = curcol[cb];
  }

static void HiCircle(int centx, int centy, int x1, int y1, unsigned char bloc, int drawfn )
  {
  int RadSquared=0;
  register int x=0,y=0;
  long int radius=1;

  if (drawfn==1)
    {
    copyblock(bloc,lastbl);  
    x1-=centx;
    y1-=centy;
    RadSquared = ((x1*x1)+(y1*y1));
    radius = (long int) RadSquared;
    radius *= 8;
    radius /=10;  
    radius=(long int) sqrt( (int) radius );
   // radius *= .707106781; // # is cos(PI/4) - makes radius only go halfway around the circle
  
    for (y=0; y<=radius; y++)
      {
      x = sqrt(RadSquared - (y*y) );
      NoDrawPt(centx+x,centy+y,bloc);
      if (y>0) NoDrawPt(centx+x,centy-y,bloc);
      if (x>0)
        {
        NoDrawPt(centx-x,centy+y,bloc);
        if (y>0) NoDrawPt(centx-x,centy-y,bloc);
        }
      if (x!=y)  // Catches when theta = PI/4 (or y = radius*.707...)
        {
        NoDrawPt(centx+y,centy+x,bloc);
        if (x>0) NoDrawPt(centx+y,centy-x,bloc);
        if (y>0)
          {
          NoDrawPt(centx-y,centy+x,bloc);
          if (x>0) NoDrawPt(centx-y,centy-x,bloc);
          }
        }
      }
    updatescrn(bloc);
    }
  else copyblock(lastbl,bloc);
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

  if (abs(squ-x)<=abs(oldsqu-x)) return(guess);
  return(guess-1);
  }

/*                         End of function Circle                        */
/*-----------------------------------------------------------------------*/
/*      Function circlefill fills a circle.   HIGH LEVEL                 */

static void HiCircleFill(int centx, int centy, int x1, int y1, unsigned char bloc, int drawfn )
  {
  register int loop=0;
  int RadSquared=0;
  register int x=0,y=0;
  int radius=1;

  if (drawfn==1)
    {
    copyblock(bloc,lastbl);  
    x1-=centx;
    y1-=centy;
  
    RadSquared = ((x1*x1)+(y1*y1));
    radius=sqrt(RadSquared);
  
    for (loop=centx-radius;loop<=centx+radius;loop++) NoDrawPt(loop,centy,bloc);
    for (y=1; y<= radius; y++)
      {
      x = sqrt(RadSquared - (y*y));
      for (loop=centx-x;loop<=centx+x;loop++) 
        {
        NoDrawPt(loop,centy+y,bloc);
        NoDrawPt(loop,centy-y,bloc);
        }
      }
    updatescrn(bloc);
    }
  else copyblock(lastbl,bloc);
  }

static void HiBox(int x, int y, int x1, int y1, unsigned char bloc, int drawfn )
  {
  register int l;
  int len;

  if (x>x1) swap (&x,&x1);
  if (y>y1) swap (&y,&y1);
  
  HiLine(x,y,x1,y,bloc,drawfn);
  if ((y!=y1)&&(x!=x1)) HiLine(x1,y+1,x1,y1,bloc,drawfn);
  if ((x!=x1)&&(y!=y1)) HiLine(x1-1,y1,x,y1,bloc,drawfn);
  if (y+1<y1) HiLine(x,y1-1,x,y+1,bloc,drawfn);
  }

static void HiBoxFill(int x, int y, int x1, int y1, unsigned char bloc, int drawfn )
  {
  register int lx,ly;
  int len;

  if (x>x1) swap (&x,&x1);
  if (y>y1) swap (&y,&y1);

  if (drawfn==1)
    {
    copyblock(bloc,lastbl);  
    for (ly=y;ly<=y1;ly++) 
      for (lx=x;lx<=x1;lx++)
        blk[bloc].p[ly][lx] = curcol[cb];
    updatescrn(bloc);
    }
  else copyblock(lastbl,bloc);
  }

static void HiFill(int x, int y, int bloc)
  {
  FillData F;
  if (!InBox(x,y,0,0,BLEN-1,BLEN-1)) return;
  FillOverCol=blk[bloc].p[y][x];
  if (FillOverCol==curcol[cb]) return;
  FillBloc = bloc;
  F.x=x;
  F.y=y;
  FillIter(F);
//  updatescrn(bloc);
  }
  
static void near FillIter(FillData F)
  {
  drawpt(F.x,F.y,FillBloc,1);
  F.y++;
  if ((F.y<BLEN)&&(blk[FillBloc].p[F.y][F.x]==FillOverCol)) FillIter(F);
  F.y-=2;
  if ((F.y>=0)  &&(blk[FillBloc].p[F.y][F.x]==FillOverCol)) FillIter(F);
  F.y++;
  F.x++;
  if ((F.x<BLEN)&&(blk[FillBloc].p[F.y][F.x]==FillOverCol)) FillIter(F);
  F.x-=2;
  if ((F.x>=0)  &&(blk[FillBloc].p[F.y][F.x]==FillOverCol)) FillIter(F);
  }

static int animons(void)
 {
  int l,i=FALSE;

  for(l=0;l<LASTMON;l++) {
    if (m[l].used) {
      if ( (m[l].lasttime != Clock) &
           ( (Clock-m[l].lasttime) >= m[l].pic[m[l].cur.pic][1] ) ) {
        if (m[l].pic[(++m[l].cur.pic)][0] == MONBL)
          m[l].cur.pic = 0;
        m[l].lasttime=Clock;
        i=TRUE;
       }
     }
   }
  return(i);
 }


static void infob(int x,int y, unsigned char col[4])
  {
  static char infoar[] =
  {"         UUUUUUU@UUUUUUUTU]××ÕUTUU]÷u]uUUUU]ÿ]uUUUU]ßu]uUU•U]×uWÕUVUUUUUUUT)UUUUUUUhªªªªªªª€"};

  draw4dat(x,y,infoar,35,10,col);
  }

static void AniBut(int x,int y, unsigned char col[4])
  {
  static char animar[] =
  {"         UUUUUUU@UUUUUUUTõ×wW__ßÔW]÷wßu×]UWýÿwÿ×_UW]ßwwu×]U—]×wWu×_ÖUUUUUUUT)UUUUUUUhªªªªªªª€"};
  draw4dat(x,y,animar,35,10,col);
  }

static void DrawClearBMP(int x,int y, unsigned char col[4])
  {
  static char array[]=
  {"UUUUUUUUUUUUTTUUEEA@AUQQQQTDEUUUUUUUUUUUUU@"};
   draw4dat(x,y,array,24,8,col);
  }

int monnum(int rebmun)      /* returns the current bloc number of the */
  {                          /*         monster number 'rebnum'.       */
  int i;

  animons();
  i=m[rebmun].pic[m[rebmun].cur.pic][0];
  return(i);
  }

  #define BLOCX  50
  #define BLOCY  71
  #define TITLEX (BLOCX-25)
  #define TITLEY (BLOCY-55)
  #define ClearScreen() BoxFill(0,0,SIZEX,SIZEY,BKGCOL);
  #define DISTAWAY 10
  #define TMENUX 115
  #define TMENUY 15
  #define TMENULEN (SIZEX-TMENUX-5)
  #define TMENUWID 80
  #define TIMEX  TMENUX+18
  #define TIMEY  (TMENUY+TMENUWID+20)
  #define MONX   TIMEX+70
  #define MONY   TIMEY
  #define TOUCHX (TMENUX+130)
  #define TOUCHY (TMENUY+25)
  #define SCOREY (TMENUY+(10*5))
  #define HITPTSY (TMENUY+(10*6))
  #define EFFECTY (TMENUY+(10*7))
  #define EMPTYCOL FaceCols[GREY4]
  #define OBJECTX 20
  #define OBJECTY (LY-(2*10)-13)
  #define TRASHX  (SIZEX-50)
  #define TRASHY  (LY-43)
  #define TTOPX   (TRASHX-1)
  #define TTOPY   (TRASHY-10)
  #define INFODONEX (TRASHX-12)
  #define INFODONEY (TMENUY+TMENUWID+11)


#define TIMEBLOCK 1
#define SCORE     2
#define HITPTS    3
#define EFFECT    4
#define EAMOUNT   5
#define MONBIRTH  6
#define MONTIME   7
#define OBJECT    8
#define TRASHCAN  9

static void MakeEffect(int bloc,char *str)
  {
  switch(blk[bloc].effect)
    {
    case 1: 
      sprintf(str,"Character Lives");
      break;
    case 3:
    sprintf(str,"Character Money");
      break;
    case 20: 
    case 21: 
    case 22: 
    case 23: 
    case 24:
      sprintf(str,"Special Counter %d",(blk[bloc].effect-19));
      break;
    default:
      sprintf(str,"Repetition Cnt %d",(blk[bloc].effect-4));
      break;
    }
  }

void GWriteShadow(int x, int y, unsigned char col, unsigned char col1, char *str)
  {
  GWrite(x+1,y+1,col1,str);
  GWrite(x,y,col,str);
  }

static void DrawInfoScrn(int b)
  {
  unsigned char FourCols[4];
  char str[41];
  char m;
  int temp;

  m=moucur(2);
  moucur(FALSE);

  drawlistframe(LX,LY,LLEN,FaceCols);                   //  Draw the bloc list
  drawlist(LX,LY,LLEN,lastbl,&list,blocnum);

  drawsurface(INFODONEX-3,INFODONEY,40,13,FaceCols);
  doneb(INFODONEX,INFODONEY,FaceCols);

  FourCols[0]=BKGCOL;
  FourCols[1]=FaceCols[GREY1];
  FourCols[2]=FaceCols[GREY2];
  FourCols[3]=FaceCols[GREY4];

  DrawTrashCan(TRASHX,TRASHY,FourCols);
  DrawCanTop(TTOPX,TTOPY,FourCols);

  sprintf(str,"Block %3d",b);
  temp=strlen(str);

  drawsurface(TITLEX-5,TITLEY-3,(8*temp)+7,13,FaceCols);
  GWriteShadow(TITLEX,TITLEY,FaceCols[GREY3],FaceCols[GREY4],str);

  drawborder(BLOCX,BLOCY,BLEN,BLEN,1,1,FaceCols);    // Border around block
  drawblk(BLOCX,BLOCY,blk[b].p[0]);

  // Draw the block animation box
  drawsurface(TIMEX-20,TIMEY-10,BLEN+18+12+3,BLEN+24+8+5,FaceCols);
  GWriteShadow(TIMEX-18,TIMEY-10,FaceCols[GREY3],FaceCols[GREY4],"Change\n\nto");
  GWriteShadow(TIMEX-18,TIMEY+BLEN+1,FaceCols[GREY3],FaceCols[GREY4],"After");
  GWriteShadow(TIMEX-18,TIMEY+BLEN+18,FaceCols[GREY3],FaceCols[GREY4],"Ticks");
  sprintf(str,"%4d",blk[b].ntime);
  GWrite(TIMEX-8,TIMEY+BLEN+11,FaceCols[GREY3],str);
  if ((blk[b].nextbl!=b)&&(blk[b].ntime!=0))
    drawblk(TIMEX,TIMEY,blk[blk[b].nextbl].p[0]);
  else BoxFill(TIMEX,TIMEY,TIMEX+BLEN-1,TIMEY+BLEN-1,EMPTYCOL);


  if ((lastbl==BACKBL)||(lastbl==MONBL))
    {
    drawsurface(TMENUX-2,TMENUY-2,TMENULEN+3,TMENUWID+3,FaceCols);            // Border around block
    GWriteShadow(TMENUX+(TMENULEN-(23*8))/2,TMENUY,FaceCols[GREY3],FaceCols[GREY4],"Character Touches Block");
    GWriteShadow(TMENUX+1,TMENUY+14,FaceCols[GREY3],FaceCols[GREY4],"Change:");

    sprintf(str,"Score by");
    GWriteShadow(TMENUX+5,SCOREY,FaceCols[GREY3],FaceCols[GREY4],str);
    sprintf(str,"%5d",blk[b].scorepts);
    GWriteShadow(TMENUX+TMENULEN-40,SCOREY,FaceCols[GREY3],FaceCols[GREY4],str);

    sprintf(str,"Hit points by",blk[b].chhitpt);
    GWriteShadow(TMENUX+5,HITPTSY,FaceCols[GREY3],FaceCols[GREY4],str);
    sprintf(str,"%5d",blk[b].chhitpt);
    GWriteShadow(TMENUX+TMENULEN-40,HITPTSY,FaceCols[GREY3],FaceCols[GREY4],str);

    MakeEffect(b,str);
    GWriteShadow(TMENUX+5,EFFECTY,FaceCols[GREY3],FaceCols[GREY4],str);
    sprintf(str,"by%5d",blk[b].eamt);
    GWriteShadow(TMENUX+TMENULEN-56,EFFECTY,FaceCols[GREY3],FaceCols[GREY4],str);

    drawborder(BLOCX,BLOCY-DISTAWAY-2,BLEN,3,1,1,FaceCols);        // Top Solid
    drawborder(BLOCX-DISTAWAY-2,BLOCY,3,BLEN,1,1,FaceCols);        // Left
    drawborder(BLOCX,BLOCY+BLEN+DISTAWAY-1,BLEN,3,1,1,FaceCols);   // Bottom
    drawborder(BLOCX+BLEN+DISTAWAY-1,BLOCY,3,BLEN,1,1,FaceCols);   // Right
    DrawSolidInfo(BLOCX,BLOCY,&blk[b]);
    }

  if (lastbl==BACKBL)
    {
    drawborder(TOUCHX+1,TOUCHY+1,BLEN-1,BLEN-1,1,1,FaceCols);  // Border touch block

    drawsurface(MONX-20,MONY-10,BLEN+20+16+3,BLEN+24+8+5,FaceCols);
    GWriteShadow(MONX-18,MONY-10,FaceCols[GREY3],FaceCols[GREY4],"Create");
    GWriteShadow(MONX-18,MONY,FaceCols[GREY3],FaceCols[GREY4],"Monster");
    GWriteShadow(MONX-18,MONY+20,FaceCols[GREY3],FaceCols[GREY4],"Every");
    GWriteShadow(MONX-18,MONY+38,FaceCols[GREY3],FaceCols[GREY4],"Ticks");

    if (blk[b].Birthmon==LASTMON) strcpy(str,"None");
    else sprintf(str,"%4d",blk[b].Birthmon+1);
    GWrite(MONX-8,MONY+10,FaceCols[GREY3],str);       // variables
    sprintf(str,"%4d",blk[b].Birthtime);
    GWrite(MONX,MONY+30,FaceCols[GREY3],str);

    GWriteShadow(TMENUX+5,TOUCHY+6,FaceCols[GREY3],FaceCols[GREY4],"Block to");
    if (blk[b].touchbl!=b)
      {
      sprintf(str,"%5d",blk[b].touchbl);
      GWriteShadow(TMENUX+TMENULEN-40,TOUCHY+6,FaceCols[GREY3],FaceCols[GREY4],str);
      drawblk(TOUCHX,TOUCHY,blk[blk[b].touchbl].p[0]);
      }
    else
      {
      GWriteShadow(TMENUX+TMENULEN-32,TOUCHY+6,FaceCols[GREY3],FaceCols[GREY4],"None");
      BoxFill(TOUCHX,TOUCHY,TOUCHX+BLEN-1,TOUCHY+BLEN-1,EMPTYCOL);
      }
    DrawGravityInfo(BLOCX,BLOCY,&blk[b]);

    ObjButton(blk[b].obj,FaceCols[GREY3]);
    }
  if (m) moucur(TRUE);
  }

static void ObjButton(int in,unsigned char col)
  {
  int m;
  if (lastbl==BACKBL)
    {
    m=moucur(2);
    moucur(FALSE);
    drawsurface(OBJECTX,OBJECTY,(9*8),2*10,FaceCols);
    if (in)
      {
      GWriteShadow(OBJECTX,OBJECTY,col,FaceCols[GREY4],"Inventory");
      GWriteShadow(OBJECTX+12,OBJECTY+10,col,FaceCols[GREY4],"object");
      }
    else
      {
      GWriteShadow(OBJECTX+12,OBJECTY,col,FaceCols[GREY4],"Not an");
      GWriteShadow(OBJECTX+12,OBJECTY+10,col,FaceCols[GREY4],"object");
      }
    if (m) moucur(TRUE);
    }
  }


static void changeinfo(int b)
  {
  char str[40];
  int addamt=0;
  int inkey;
  int sx,sy;                   // <--Ah, good--vars. for mouse tracking!
  int mx,my,mbuts,ox,oy,obuts; // <--What's this? Old value storage and extras!?
  unsigned char changed=FALSE;
  int bnum,grabbed=-1;         // <--What this function needs is some more
  int temp;                    //    vars. for mouse movement tracking :-)
  int done=FALSE;
  int Window=0,OWindow=0;

  moustats(&sx,&sy,&mbuts);
  moucur(FALSE);
  FadeAllTo(colors[BKGCOL],FADETIME);
  BoxFill(0,0,SIZEX,LY-5,BKGCOL);
  DrawInfoScrn(b);
  moucur(TRUE);
  FadeTo(colors,FADETIME);

  done=FALSE;
  while(!done)
    {
    changed=FALSE;
    do
      {
      if (bioskey(1)) changed=1;
      moustats(&mx,&my,&mbuts);
      if ((mx!=ox)|(my!=oy)) changed |= 2;
      if (mbuts!=obuts) changed |=4;
      if (mbuts>0) changed |=8;
      } while (!changed);

    if (((changed&7)>0)&&(grabbed!=-1))             // Erase block
      {
      moucur(FALSE);
      drawblk(ox-10,oy-10,&blk[BACKBL].p[0][0]);
      moucur(TRUE);
      }
 
    if (changed&1)
      {
      inkey=bioskey(0);
      switch (inkey&255)
        {
        case 0:
          switch(inkey>>8)
            {
            case RIGHTKEY:
             addamt=1;
             break;
            case LEFTKEY:
             addamt=-1;
             break;
            case UPKEY:
             addamt=100;
             break;
            case DOWNKEY:
             addamt=-100;
             break;
            }
          break;
        case 27:
          done=TRUE;
          addamt=0;
          break;
        case 8:
          if (Window==SCORE)   blk[b].scorepts =0;
          if (Window==HITPTS)  blk[b].chhitpt  =0;
          if (Window==EAMOUNT) blk[b].eamt     =0;
          if (Window==MONTIME) blk[b].Birthtime=1;
          if (Window==TIMEBLOCK) blk[b].ntime  =0;
          addamt=0;
          if (InBox(mx,my,TIMEX,TIMEY,TIMEX+BLEN,TIMEY+BLEN))
            {
            blk[b].nextbl=b;
            blk[b].ntime=0;
            moucur(FALSE);
            BoxFill(TIMEX,TIMEY,TIMEX+BLEN-1,TIMEY+BLEN-1,EMPTYCOL);
            moucur(TRUE);
            }
          if (lastbl==BACKBL)
            {
            if (InBox(mx,my,TOUCHX,TOUCHY,TOUCHX+BLEN,TOUCHY+BLEN))
              {
              blk[b].touchbl=b;
              moucur(FALSE);
              BoxFill(TOUCHX,TOUCHY,TOUCHX+BLEN-1,TOUCHY+BLEN-1,EMPTYCOL);
              BoxFill(TMENUX+TMENULEN-32,TOUCHY+6,TMENUX+TMENULEN,TOUCHY+14,FaceCols[GREY1]);
              GWriteShadow(TMENUX+TMENULEN-32,TOUCHY+6,FaceCols[GREY3],FaceCols[GREY4],"None");
              moucur(TRUE);
              }
            }
          break;
        }
      moucur(FALSE);
      if ((Window!=0)&&(addamt!=0)) saved=FALSE;
      switch (Window)
        {
        case MONBIRTH:
          if (lastbl==BACKBL)
            {
            if (addamt>LASTMON) addamt=10;
            if (addamt<-LASTMON) addamt = -10;
            if ((addamt<0)&&(blk[b].Birthmon<-addamt))      // Wrap if would
            blk[b].Birthmon=LASTMON;                        // be < 0
            else blk[b].Birthmon+=addamt;
            if (blk[b].Birthmon>LASTMON) blk[b].Birthmon=0; // Wrap if too big
            if (blk[b].Birthmon==LASTMON) strcpy(str,"None");
            else sprintf(str,"%4d",blk[b].Birthmon+1);
            BoxFill(MONX-8,MONY+10,MONX-8+(8*4),MONY+10+6,FaceCols[GREY1]);
            GWrite(MONX-8,MONY+10,FaceCols[RED3],str);
            }
          break;
        case MONTIME:
          if (lastbl==BACKBL)
            {
            blk[b].Birthtime+=addamt;
            if (blk[b].Birthtime<1)    blk[b].Birthtime=9999;
            if (blk[b].Birthtime>9999) blk[b].Birthtime=1;
            sprintf(str,"%4d",blk[b].Birthtime);
            BoxFill(MONX,MONY+30,MONX+(8*4),MONY+30+6,FaceCols[GREY1]);
            GWrite(MONX,MONY+30,FaceCols[RED3],str);
            }
          break;
        case TIMEBLOCK:
          blk[b].ntime+=addamt;
          if (blk[b].ntime<0) blk[b].ntime=9999;
          if (blk[b].ntime>9999) blk[b].ntime=0;
          sprintf(str,"%4d",blk[b].ntime);
          BoxFill(TIMEX-8,TIMEY+BLEN+11,TIMEX+BLEN+2,TIMEY+BLEN+17,FaceCols[GREY1]);
          GWrite(TIMEX-8,TIMEY+BLEN+11,FaceCols[RED3],str);
          break;
        case SCORE:
          if (lastbl!=CHARBL)
            {
            blk[b].scorepts+=addamt;
            if (blk[b].scorepts>9999)  blk[b].scorepts -=2*9999;
            if (blk[b].scorepts<-9999) blk[b].scorepts +=2*9999;
            BoxFill(TMENUX+TMENULEN-(5*8),SCOREY,TMENUX+TMENULEN,SCOREY+8,FaceCols[GREY1]);
            sprintf(str,"%5d",blk[b].scorepts);
            GWriteShadow(TMENUX+TMENULEN-(5*8),SCOREY,FaceCols[RED3],FaceCols[GREY4],str);
            }
          break;

        case HITPTS:
          if (lastbl!=CHARBL)
            {
            blk[b].chhitpt+=addamt;
            if (blk[b].chhitpt>9999)  blk[b].chhitpt -= 2*9999;
            if (blk[b].chhitpt<-9999) blk[b].chhitpt += 2*9999;
            BoxFill(TMENUX+TMENULEN-(5*8),HITPTSY,TMENUX+TMENULEN,HITPTSY+8,FaceCols[GREY1]);
            sprintf(str,"%5d",blk[b].chhitpt);
            GWriteShadow(TMENUX+TMENULEN-(5*8),HITPTSY,FaceCols[RED3],FaceCols[GREY4],str);
            }
          break;
        case EFFECT:
          if (lastbl!=CHARBL)
            {
            blk[b].effect+=addamt;
            if ((blk[b].effect==2)||(blk[b].effect==4)) blk[b].effect+=Sign(addamt);
            if (blk[b].effect<1) blk[b].effect=24;
            if (blk[b].effect>24) blk[b].effect=1;
            MakeEffect(b,str);
            BoxFill(TMENUX+5,EFFECTY,TMENUX+TMENULEN-56,EFFECTY+8,FaceCols[GREY1]);
            GWriteShadow(TMENUX+5,EFFECTY,FaceCols[RED3],FaceCols[GREY4],str);
            }
          break;
        case EAMOUNT:
          if (lastbl!=CHARBL)
            {
            blk[b].eamt+=addamt;
            if (blk[b].eamt>9999)  blk[b].eamt -= 2*9999;
            if (blk[b].eamt<-9999) blk[b].eamt += 2*9999;
            sprintf(str,"%5d",blk[b].eamt);
            BoxFill(TMENUX+TMENULEN-40,EFFECTY,TMENUX+TMENULEN,EFFECTY+8,FaceCols[GREY1]);
            GWriteShadow(TMENUX+TMENULEN-40,EFFECTY,FaceCols[RED3],FaceCols[GREY4],str);
            }
          break;
        case OBJECT:
          if (addamt!=0)
            {
            if (blk[b].obj!=0) blk[b].obj=0;
            else blk[b].obj=1;
            }
          ObjButton(blk[b].obj,FaceCols[RED3]);
          break;
        }
      moucur(TRUE);
      }

    OWindow=Window;       // See if the cursor has gone over another menu
    Window=0;             // item.  If so, highlight it and set a flag.

    if ((InBox(mx,my,TRASHX,TTOPY,TRASHX+25,TRASHY+35))||
          ((OWindow==TRASHCAN)&&
           (InBox(mx,my,TRASHX,TTOPY-BLEN,TRASHX+25,TRASHY+35))
          ))
      {
      unsigned char FourCols[4];
      Window=TRASHCAN;
      if (OWindow!=TRASHCAN)
        {
        FourCols[0]=BKGCOL;
        FourCols[1]=FaceCols[GREY1];
        FourCols[2]=FaceCols[GREY2];
        FourCols[3]=FaceCols[GREY4];
        moucur(FALSE);
        BoxFill(TTOPX,TTOPY+2,TTOPX+27,TTOPY+16,BKGCOL);
        DrawTrashCan(TRASHX,TRASHY,FourCols);
        DrawCanTop(TTOPX,TTOPY-BLEN,FourCols);
        moucur(TRUE);
        }
      }
    else if (InBox(mx,my,INFODONEX,INFODONEY,INFODONEX+35,INFODONEY+10))
      {
      if ((mbuts>0)&&(obuts==0)) done=TRUE;
      }

    if (InBox(mx,my,TIMEX-19,TIMEY-9,TIMEX+BLEN+12,TIMEY+BLEN+24))
      {
      Window=TIMEBLOCK;
      if (OWindow!=TIMEBLOCK)
        {
        sprintf(str,"%4d",blk[b].ntime);
        BoxFill(TIMEX-8,TIMEY+BLEN+11,TIMEX+BLEN+2,TIMEY+BLEN+17,FaceCols[GREY1]);
        GWrite(TIMEX-8,TIMEY+BLEN+11,FaceCols[RED3],str);
        }
      }
    if (lastbl!=CHARBL)
      {
      if (InBox(mx,my,MENUX,SCOREY,MENUX+TMENULEN,SCOREY+8))
        {
        Window=SCORE;
        if (OWindow!=SCORE)
          {
          sprintf(str,"Score by",blk[b].scorepts);
          moucur(FALSE);
          BoxFill(TMENUX+5,SCOREY,TMENUX+5+(8*strlen(str)),SCOREY+8,FaceCols[GREY1]);
          GWriteShadow(TMENUX+5,SCOREY,FaceCols[RED3],FaceCols[GREY4],str);
          moucur(TRUE);
          }
        }
      if (InBox(mx,my,MENUX,HITPTSY,MENUX+TMENULEN,HITPTSY+8))
        {
        Window=HITPTS;
        if (OWindow!=HITPTS)
          {
          sprintf(str,"Hit points by",blk[b].chhitpt);
          moucur(FALSE);
          BoxFill(TMENUX+5,HITPTSY,TMENUX+5+(8*strlen(str)),HITPTSY+8,FaceCols[GREY1]);
          GWriteShadow(TMENUX+5,HITPTSY,FaceCols[RED3],FaceCols[GREY4],str);
          moucur(TRUE);
          }
        }
      if (InBox(mx,my,MENUX,EFFECTY,MENUX+TMENULEN-(9*9),EFFECTY+8))
        {
        Window=EFFECT;
        if (OWindow!=EFFECT)
          {
          MakeEffect(b,str);
          moucur(FALSE);
          BoxFill(TMENUX+5,EFFECTY,TMENUX+5+(8*strlen(str)),EFFECTY+8,FaceCols[GREY1]);
          GWriteShadow(TMENUX+5,EFFECTY,FaceCols[RED3],FaceCols[GREY4],str);
          moucur(TRUE);
          }
        }
      if (InBox(mx,my,MENUX+TMENULEN-(8*9),EFFECTY,MENUX+TMENULEN,EFFECTY+8))
        {
        Window=EAMOUNT;
        if (OWindow!=EAMOUNT)
          {
          moucur(FALSE);
          BoxFill(TMENUX+TMENULEN-(7*8),EFFECTY,TMENUX+TMENULEN,EFFECTY+8,FaceCols[GREY1]);
          sprintf(str,"by%5d",blk[b].eamt);
          GWriteShadow(TMENUX+TMENULEN-56,EFFECTY,FaceCols[RED3],FaceCols[GREY4],str);
          moucur(TRUE);
          }
        }
      }
    if (lastbl==BACKBL)
      {
      if (InBox(mx,my,MONX-20,MONY-8,MONX+BLEN+16+3,MONY+16))
        {
        Window=MONBIRTH;
        if (blk[b].Birthmon==LASTMON) strcpy(str,"None");
        else sprintf(str,"%4d",blk[b].Birthmon+1);
        GWrite(MONX-8,MONY+10,FaceCols[RED3],str);       // variables
        }
      if (InBox(mx,my,MONX-20,MONY+16,MONX+BLEN+16+3,MONY+45))
        {
        Window=MONTIME;
        sprintf(str,"%4d",blk[b].Birthtime);
        GWrite(MONX,MONY+30,FaceCols[RED3],str);
        }
      if (InBox(mx,my,OBJECTX,OBJECTY,OBJECTX+(9*8),OBJECTY+(2*10)))
        {
        Window=OBJECT;
        if (OWindow!=OBJECT) ObjButton(blk[b].obj,FaceCols[RED3]);
        }
      }

    if (OWindow!=Window)     // Unhighlight old highlighted menu option
      {
      moucur(FALSE);
      switch (OWindow)
        {
        case MONBIRTH:
          if (blk[b].Birthmon==LASTMON) strcpy(str,"None");
          else sprintf(str,"%4d",blk[b].Birthmon+1);
          GWrite(MONX-8,MONY+10,FaceCols[GREY3],str);       
          break;
        case MONTIME:
          sprintf(str,"%4d",blk[b].Birthtime);
          GWrite(MONX,MONY+30,FaceCols[GREY3],str);
          break;
        case TIMEBLOCK:
          sprintf(str,"%4d",blk[b].ntime);
          BoxFill(TIMEX-8,TIMEY+BLEN+11,TIMEX+BLEN+2,TIMEY+BLEN+17,FaceCols[GREY1]);
          GWrite(TIMEX-8,TIMEY+BLEN+11,FaceCols[GREY3],str);
          break;
        case SCORE:
          sprintf(str,"Score by",blk[b].scorepts);
          GWriteShadow(TMENUX+5,SCOREY,FaceCols[GREY3],FaceCols[GREY4],str);
          sprintf(str,"%5d",blk[b].scorepts);
          GWriteShadow(TMENUX+TMENULEN-40,SCOREY,FaceCols[GREY3],FaceCols[GREY4],str);
          break;
        case HITPTS:
          sprintf(str,"Hit points by",blk[b].chhitpt);
          GWriteShadow(TMENUX+5,HITPTSY,FaceCols[GREY3],FaceCols[GREY4],str);
          sprintf(str,"%5d",blk[b].chhitpt);
          GWriteShadow(TMENUX+TMENULEN-40,HITPTSY,FaceCols[GREY3],FaceCols[GREY4],str);
          break;
        case EFFECT:
          MakeEffect(b,str);
          GWriteShadow(TMENUX+5,EFFECTY,FaceCols[GREY3],FaceCols[GREY4],str);
          break;
        case EAMOUNT:
          sprintf(str,"by%5d",blk[b].eamt);
          GWriteShadow(TMENUX+TMENULEN-56,EFFECTY,FaceCols[GREY3],FaceCols[GREY4],str);
          break;
        case OBJECT:
          ObjButton(blk[b].obj,FaceCols[GREY3]);
          break;
        case TRASHCAN:
          unsigned char str[4];
          str[0]=BKGCOL;
          str[1]=FaceCols[GREY1];
          str[2]=FaceCols[GREY2];
          str[3]=FaceCols[GREY4];
          DrawTrashCan(TRASHX,TRASHY,str);
          DrawCanTop(TTOPX,TTOPY,str);
          drawsurface(INFODONEX-3,INFODONEY,40,13,FaceCols);
          doneb(INFODONEX,INFODONEY,FaceCols);
          break;
        }
      moucur(TRUE);
      }

    if ((mbuts>0)&&(obuts==0))
      {
      if (Window==OBJECT)
        {
        saved=FALSE;
        if (blk[b].obj!=0) blk[b].obj=0;
        else blk[b].obj=1;
        ObjButton(blk[b].obj,FaceCols[RED3]);
        }
      if (Window==MONBIRTH)
        {
        blk[b].Birthmon=birthmon(blk[b].Birthmon);
        moucur(FALSE);
        FadeAllTo(colors[BKGCOL],FADETIME);
        BoxFill(0,0,SIZEX,LY-5,BKGCOL);
        DrawInfoScrn(b);
        moucur(TRUE);
        FadeTo(colors,FADETIME);
        }
      if (InBox(mx,my,BLOCX+2,BLOCY-DISTAWAY-27,BLOCX+18,BLOCY-DISTAWAY-27+16))
        {
        saved=FALSE;
        if (blk[b].grav&SOLTOP) blk[b].grav&= (~SOLTOP);
        else blk[b].grav|=SOLTOP;
        DrawGravityInfo(BLOCX,BLOCY,&blk[b]);
        }
      else if (InBox(mx,my,BLOCX+2,BLOCY+BLEN+DISTAWAY+10,BLOCX+18,BLOCY+BLEN+DISTAWAY+28))
        {
        saved=FALSE;
        if (blk[b].grav&SOLBOT) blk[b].grav&= (~SOLBOT);
        else blk[b].grav|=SOLBOT;
        DrawGravityInfo(BLOCX,BLOCY,&blk[b]);
        }
      else if (InBox(mx,my,BLOCX-DISTAWAY-27,BLOCY+2,BLOCX-DISTAWAY-27+16,BLOCY+18))
        {
        saved=FALSE;
        if (blk[b].grav&SOLLEF) blk[b].grav&= (~SOLLEF);
        else blk[b].grav|=SOLLEF;
        DrawGravityInfo(BLOCX,BLOCY,&blk[b]);
        }
      else if (InBox(mx,my,BLOCX+DISTAWAY+BLEN+10,BLOCY+2,BLOCX+DISTAWAY+BLEN+26,BLOCY+18))
        {
        saved=FALSE;
        if (blk[b].grav&SOLRIG) blk[b].grav&= (~SOLRIG);
        else blk[b].grav|=SOLRIG;
        DrawGravityInfo(BLOCX,BLOCY,&blk[b]);
        }
      else if (InBox(mx,my,BLOCX-1,BLOCY-DISTAWAY-3,BLOCX+BLEN-1,BLOCY-DISTAWAY))
        {
        saved=FALSE;
        if (blk[b].solid&SOLTOP) blk[b].solid &= (~SOLTOP);
        else blk[b].solid|=SOLTOP;
        DrawSolidInfo(BLOCX,BLOCY,&blk[b]);
        }
      else if (InBox(mx,my,BLOCX-1,BLOCY+BLEN+DISTAWAY-2,BLOCX+BLEN-1,BLOCY+BLEN+DISTAWAY+1))
        {
        saved=FALSE;
        if (blk[b].solid&SOLBOT) blk[b].solid &= (~SOLBOT);
        else blk[b].solid|=SOLBOT;
        DrawSolidInfo(BLOCX,BLOCY,&blk[b]);
        }
      else if (InBox(mx,my,BLOCX-1,BLOCY+BLEN+DISTAWAY-2,BLOCX+BLEN-1,BLOCY+BLEN+DISTAWAY+1))
        {
        saved=FALSE;
        if (blk[b].solid&SOLBOT) blk[b].solid &= (~SOLBOT);
        else blk[b].solid|=SOLBOT;
        DrawSolidInfo(BLOCX,BLOCY,&blk[b]);
        }
      else if (InBox(mx,my,BLOCX-DISTAWAY-3,BLOCY-1,BLOCX-DISTAWAY,BLOCY+BLEN-1))
        {
        saved=FALSE;
        if (blk[b].solid&SOLLEF) blk[b].solid &= (~SOLLEF);
        else blk[b].solid|=SOLLEF;
        DrawSolidInfo(BLOCX,BLOCY,&blk[b]);
        }
      else if (InBox(mx,my,BLOCX+BLEN+DISTAWAY-2,BLOCY-1,BLOCX+BLEN+DISTAWAY+1,BLOCY+BLEN-1))
        {
        saved=FALSE;
        if (blk[b].solid&SOLRIG) blk[b].solid &= (~SOLRIG);
        else blk[b].solid|=SOLRIG;
        DrawSolidInfo(BLOCX,BLOCY,&blk[b]);
        }
      }

    if (InBox(mx,my,BLOCX,BLOCY,BLOCX+BLEN,BLOCY+BLEN))
      {
      if (grabbed==-1)
        {
        if ((mbuts>0)&&(obuts==0))   // Pick up Block
          {
          grabbed = b;
          moucurbox(BLEN/2,BLEN/2,319-BLEN/2,199-BLEN/2);
          }
        }
      else
        {
        if ((mbuts==0)&&(obuts!=0))  // Drop block
          {
          moucur(FALSE);
          b=grabbed;
          DrawInfoScrn(b);
          saved=0;
          grabbed=-1;
          moucurbox(0,0,319,199);
          moucur(TRUE);
          }
        }
      }

    if ((InBox(mx,my,TOUCHX,TOUCHY,TOUCHX+BLEN,TOUCHY+BLEN))&&(lastbl==BACKBL))
      {
      if (grabbed==-1)
        {
        if ((mbuts>0)&&(obuts==0))   // Pick up Block
          {
          if (blk[b].touchbl!=b)
            {
            saved=FALSE;
            grabbed = blk[b].touchbl;
            blk[b].touchbl=b;
            moucur(FALSE);
            BoxFill(TOUCHX,TOUCHY,TOUCHX+BLEN-1,TOUCHY+BLEN-1,EMPTYCOL);
            BoxFill(TMENUX+TMENULEN-32,TOUCHY+6,TMENUX+TMENULEN,TOUCHY+14,FaceCols[GREY1]);
            GWriteShadow(TMENUX+TMENULEN-32,TOUCHY+6,FaceCols[GREY3],FaceCols[GREY4],"None");
            moucur(TRUE);
            }
          moucurbox(BLEN/2,BLEN/2,319-BLEN/2,199-BLEN/2);
          }
        }
      else
        {
        if ((mbuts==0)&&(obuts!=0))  // Drop block
          {
          moucur(FALSE);

          blk[b].touchbl=grabbed;
          BoxFill(TMENUX+TMENULEN-32,TOUCHY+6,TMENUX+TMENULEN,TOUCHY+14,FaceCols[GREY1]);
          if (blk[b].touchbl!=b)
            {
            GWriteShadow(TMENUX+5,TOUCHY+6,FaceCols[GREY3],FaceCols[GREY4],"Block to");
            sprintf(str,"%5d",blk[b].touchbl);
            GWriteShadow(TMENUX+TMENULEN-40,TOUCHY+6,FaceCols[GREY3],FaceCols[GREY4],str);
            drawblk(TOUCHX,TOUCHY,blk[blk[b].touchbl].p[0]);
            }
          else
            {
            GWriteShadow(TMENUX+TMENULEN-32,TOUCHY+6,FaceCols[GREY3],FaceCols[GREY4],"None");
            BoxFill(TOUCHX,TOUCHY,TOUCHX+BLEN-1,TOUCHY+BLEN-1,EMPTYCOL);
            }
          saved=FALSE;
          grabbed=-1;
          moucurbox(0,0,319,199);
          moucur(TRUE);
          }
        }
      }

    if (InBox(mx,my,TIMEX,TIMEY,TIMEX+BLEN,TIMEY+BLEN))
      {
      if (grabbed==-1)
        {
        if ((mbuts>0)&&(obuts==0))   // Pick up Block
          {
          if ((blk[b].nextbl!=b)&&(blk[b].ntime!=0))
            {
            grabbed = blk[b].nextbl;
            blk[b].nextbl=b;
            moucur(FALSE);
            BoxFill(TIMEX,TIMEY,TIMEX+BLEN-1,TIMEY+BLEN-1,EMPTYCOL);
            moucur(TRUE);
            }
          moucurbox(BLEN/2,BLEN/2,319-BLEN/2,199-BLEN/2);
          }
        }
      else
        {
        if ((mbuts==0)&&(obuts!=0))  // Drop block
          {
          moucur(FALSE);
          blk[b].nextbl=grabbed;
          if (blk[b].ntime==0)
            {
            blk[b].ntime=1;
            sprintf(str,"%4d",blk[b].ntime);
            BoxFill(TIMEX-8,TIMEY+BLEN+11,TIMEX+BLEN+2,TIMEY+BLEN+17,FaceCols[GREY1]);
            GWrite(TIMEX-8,TIMEY+BLEN+11,FaceCols[RED3],str);
            }
          if (blk[b].nextbl!=b) drawblk(TIMEX,TIMEY,blk[grabbed].p[0]);
          else BoxFill(TIMEX,TIMEY,TIMEX+BLEN-1,TIMEY+BLEN-1,EMPTYCOL);
          saved=0;
          grabbed=-1;
          moucurbox(0,0,319,199);
          moucur(TRUE);
          }
        }
      }

    if (InBox(mx,my,LX,LY,LX1,LY1)) // In list
      {
      bnum=(list+((mx-LX)/BLEN))%lastbl;
      if (grabbed==-1)
        {
        if ((mbuts>0)&(obuts==0))   // Pick up Block
          {
          grabbed=bnum;
          moucurbox(BLEN/2,BLEN/2,319-BLEN/2,199-BLEN/2);
          }
        }
      else
        {
        if ((mbuts==0)&(obuts!=0)) // Drop block
          {
          moucur(FALSE);
          CopyInfo(grabbed,bnum);
          if (list<=bnum) drawblk(LX+(BLEN*(bnum-list)),LY,blk[bnum].p[0]);
          else drawblk(LX+(BLEN*((lastbl-list)+bnum)),LY,blk[bnum].p[0]);
          saved=0;
          grabbed=-1;
          moucurbox(0,0,319,199);
          moucur(TRUE);
          }
        }
      }
    if (mbuts>0)
      {
      if (InBox(mx,my,LX1+2,LY,LX1+11,LY1)) { moucur(0); DrawArrows(LX,LY,LLEN,RIGHT,FaceCols); scroll_list(-4,LX,LY,LLEN,lastbl,&list,blocnum); DrawArrows(LX,LY,LLEN,NONE,FaceCols); moucur(1); }
      if (InBox(mx,my,LX-12,LY,LX-3,LY1)) { moucur(0); DrawArrows(LX,LY,LLEN,LEFT,FaceCols); scroll_list(4,LX,LY,LLEN,lastbl,&list,blocnum); DrawArrows(LX,LY,LLEN,NONE,FaceCols); moucur(1); }
      }
    if ((mbuts==0)&(grabbed!=-1)) // Drop block nowhere if button let go
      {
      grabbed=-1;
      moucurbox(0,0,319,199);
      }
    if (((changed&7)>0)&(grabbed!=-1))             // draw block
      {
      moucur(FALSE);
      getblk(mx-10,my-10,&blk[BACKBL].p[0][0]);
      drawblk(mx-10,my-10,&blk[grabbed].p[0][0]);
      moucur(TRUE);
      }
    ox=mx; oy=my; obuts=mbuts;
    }

  FadeAllTo(colors[BKGCOL],FADETIME);
  InitAni();
  drawscrn();
  setmoupos(sx,sy);
  }

static void DrawGravityInfo(int blkx,int blky,blkstruct *blk)
  {
  char m;
  unsigned char DullCols[4];
  unsigned char SelCols[4];
  unsigned char *ptr;
  #define GRAVCOL FaceCols[RED3]
  #define NOGRAVCOL FaceCols[GREY2]

  m=moucur(2);
  moucur(FALSE);
  DullCols[0]=SelCols[0]=0;
  SelCols[1]=FaceCols[RED1];
  DullCols[1]=FaceCols[GREY2];
  SelCols[2]=FaceCols[RED2];
  DullCols[2]=FaceCols[GREY4];

  if (blk->grav&SOLTOP) ptr=SelCols;
  else ptr=DullCols;
  DrawGravBut(blkx+2,blky-DISTAWAY-27,ptr,0);

  if (blk->grav&SOLBOT) ptr=SelCols;
  else ptr=DullCols;

  DrawGravBut(blkx+2,blky+BLEN+DISTAWAY+10,ptr,2);

  if (blk->grav&SOLLEF) ptr=SelCols;
  else ptr=DullCols;

  DrawGravBut(blkx-DISTAWAY-27,blky+2,ptr,3);

  if (blk->grav&SOLRIG) ptr=SelCols;
  else ptr=DullCols;

  DrawGravBut(blkx+BLEN+DISTAWAY+10,blky+2,ptr,1);
  if (m) moucur(TRUE);
  }

static void DrawSolidInfo(int blkx,int blky,blkstruct *blk)
  {
  char m;
  register int loop;
  unsigned char color;

  #define BSOLCOL FaceCols[RED3]
  #define NOTSOLCOL FaceCols[GREY2]

  m=moucur(2);
  moucur(FALSE);

  drawborder(BLOCX,BLOCY-DISTAWAY-2,BLEN,3,1,1,FaceCols);        // Top Solid
  if (blk->solid&SOLTOP)
    BoxFill(blkx-1,blky-DISTAWAY-3,blkx+BLEN-1,blky-DISTAWAY,BSOLCOL);
  else BoxFill(blkx,blky-DISTAWAY-2,blkx+BLEN-1,blky-DISTAWAY,NOTSOLCOL);

  drawborder(BLOCX,BLOCY+BLEN+DISTAWAY-1,BLEN,3,1,1,FaceCols);   // Bottom
  if (blk->solid&SOLBOT)
    BoxFill(blkx-1,blky+BLEN+DISTAWAY-2,blkx+BLEN-1,blky+BLEN+DISTAWAY+1,BSOLCOL);
  else BoxFill(blkx,blky+BLEN+DISTAWAY-1,blkx+BLEN-1,blky+BLEN+DISTAWAY+1,NOTSOLCOL);

  drawborder(BLOCX-DISTAWAY-2,BLOCY,3,BLEN,1,1,FaceCols);        // Left
  if (blk->solid&SOLLEF)
    BoxFill(blkx-DISTAWAY-3,blky-1,blkx-DISTAWAY,blky+BLEN-1,BSOLCOL);
  else BoxFill(blkx-DISTAWAY-2,blky,blkx-DISTAWAY,blky+BLEN-1,NOTSOLCOL);

  drawborder(BLOCX+BLEN+DISTAWAY-1,BLOCY,3,BLEN,1,1,FaceCols);   // Right
  if (blk->solid&SOLRIG)
    BoxFill(blkx+BLEN+DISTAWAY-2,blky-1,blkx+BLEN+DISTAWAY+1,blky+BLEN-1,BSOLCOL);
  else BoxFill(blkx+BLEN+DISTAWAY-1,blky,blkx+BLEN+DISTAWAY+1,blky+BLEN-1,NOTSOLCOL);

  if (m) moucur(TRUE);
  }

void DrawGravBut(int x,int y, unsigned char col[4],int rotation)
  {
  char array[]= {   0,  2,128,  0,  0, 10,160,  0,  0, 41,104,  0,  0,149, 86,  0,  2, 85, 85,128,  9, 85, 85, 96, 37, 85, 85, 88,169, 85, 85,106, 42,165, 90,168,  0, 41,104,  0,  0,  9, 96,  0,  0,  9, 96,  0,  0,  9, 96,  0,  0,  9, 96,  0,  0,  9, 96,  0,  0,  2,128,  0};
  draw4dat(x,y,array,15,15,col,rotation);
  }

void DrawTrashCan(int x,int y, unsigned char col[4])
  {
  char array[]= {   0,  0,255,250,160,  0,  0, 63,255,255,170, 86,128, 63,  3,255,254,169, 86,139,192,  3,255,250,149, 90, 91,  0, 15,255,234, 90,246, 85,108, 63,250,170,243,105, 85, 85, 86,170,172, 54,151,213, 85, 90,178,195,105,127, 85,181,172, 44, 54,151,229,108,218, 14,195,105,126, 86,  9,160,236, 54,151,229,114,154, 14,195,105,122, 87, 41,160,236, 54,151,165,126,154, 14,195,105,122, 87,233,160,236, 54,151,165,126,154, 14,195,105,122, 87,233,160,236,
                     54,151,165,126,154, 14,195,105,106, 87,233,160,236, 54,150,165,126,154, 14,195,105,106, 87,169,160,236, 54,150,165,122,154, 14,195,105,106, 87,169,160,236, 54,150,165,122,154, 62,195,105,106, 87,169,163,236, 54,150,165,122,154, 62,195,105,106, 87,169,163,236, 54,150,165,122,154, 62,195,105,106, 87,169,163,236, 54,150,165,122,154, 62,195,105,106, 87,169,163,236,249,150,165,106,154, 62,255,229, 90, 86,169,162,175,192,229, 85, 90, 90,171,
                     192,  0,  3,149, 86,172,  0,  0,};

  draw4dat(x,y,array,25,34,col);
  }

void DrawCanTop(int x,int y, unsigned char col[4])
  {
  //char array[]= {"                        <      k      ¯      <           :Z¿  ÿÕj«ÿ ?¥ªj¿ÿðjUjªÿÿëUªªª¯ê¿%UjªªªüUUVªªÀ  %U«  "};
  char array[]= {   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 60,  0,  0,  0,  0,  0,  0,107,  0,  0,  0,  0,  0,  0,175,  0,  0,  0,  0,  0,  0, 60,  0,  0,  0,  0,  0,  0, 28,  0,  0,  0,  0,  0, 58, 90,191,  0,  0,  3,255,213,106,171,255,  0, 63,165,170,106,191,255,240,106, 85,106,170,255,255,235, 85,170,170,170,175,234,191, 37, 85,106,170,170,170,252,  2, 85, 85, 86,170,170,192,  0,  0, 37, 85,171,  0,  0,};
  draw4dat(x,y,array,27,15,col,8);
  }
