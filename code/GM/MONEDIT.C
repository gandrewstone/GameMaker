/*-------Monster Animator------------------------------------------------*/
/*   Feb 1, 1991      Programmer:  Ollie Stone                           */
/*   Last Edited: 7/09/1991                                              */
/*-----------------------------------------------------------------------*/

#include <process.h>
#include <stdio.h>
#include <dos.h>
#include <bios.h>
#include <alloc.h>
#include <conio.h>
#include <stdlib.h>

#include "gen.h"
#include "scrnrout.h"
#include "mousefn.h"
#include "windio.h"
#include "gmgen.h"
#include "palette.h"
#include "bloc.h"
#include "mon.h"
#include "jstick.h"
#include "graph.h"
#include "facelift.h"
#include "tranmous.hpp"


#define BLLEN    14                       /*  information on location of  */
#define BLX      20                       /* the graphical list of blocks */
#define BLY      150
#define BLX1     (BLX+(BLLEN*BLEN))
#define BLY1     (BLY+BLEN)

#define MOLEN    13                       /*  graphical list of monsters  */ 
#define MOX      32
#define MOY      18
#define MOX1     (MOX+(MOLEN*BLEN))
#define MOY1     (MOY+BLEN)

#define SLLEN    10                        /*  of frames for each monster  */ 
#define SLX      70
#define SLY      67
#define SLX1     (SLX+(SLLEN*BLEN))
#define SLY1     (SLY+BLEN)

#define TIME     200      /* number of loops before a click becomes a drag */
#define BCOL     100                       /*  border color  */
#define MX       90                     /* coords for buttons */
#define MY       119
#define HX       MX+40
#define HY       MY
#define IX       MX+80
#define IY       MY
                                                // Make-Path Screen Defines
#define LS       191                              // Box Left Side
#define TS       46                               // Box Top Side
#define PATHHELPFILE "pathedit.hlp"
#define OX       64                             // Picture coordinates
#define OY       107
#define PMX      PHX                           // coords for buttons
#define PMY      (PHY+12)
#define PHX      5
#define PHY      104
#define TX       4
#define TY       59
#define TTX      (TX+98)

#define SELX     123                            // Pickmon defines
#define SELY     90
#define MOY2     40
#define TBOXX    37
#define TBOXY    145
#define MX3      SELX+40
#define MY3      SELY-3

#define MONHELPFILE "monedit.hlp"

/*-----------------------------------------Functions */

static void initalldata(void);             /* Presets values so program won't crash */
static void cleanup(void);                 /* Frees allocated memory.               */
static void edit(void);                    /* The main controlling function.        */
static void drawlistbox(int x,int y,int blnum);
static void drawscrn(void);                  /* Draws everything on the screen.     */
static void enterpause(int frame);           /* The text version of edit().         */ 
static void dropbloc(int x,int y,int *bloc); /* Proc. when releasing a dragged bloc */
static void dropmon(int x,int y);            /* Proc. when releasing a dragged mon. */
static int animons(void);                    /* Advances monster frames when ready. */
int monnum(int rebmun);                 /* --------------------------------- */
int monnum2(int rebmun);                /* Functions for scroll_list() which */ 
int blocnum(int mun);                   /*   return next bloc to be drawn.   */ 
int selnum(int mun);                    /* --------------------------------- */
static void entermove(void);            /* enter monster movement  */
static void pickmon(void);              /* choose monster to birth at death */
static void drawbl(int x,int y,int blo,char orient);   /* draw rot/flip bloc */
static void makepath(coords move[10],char status);     /* enter movement pattern */
static void drawscrn2(int len,int wid,char spot,char spot2,int oldx,int oldy,char status,coords *move);
static void myline(int ox,int oy,int ox1,int oy1,char col);
static void lightb(int x,int y, unsigned char col[4]);
static void DrawFnButs(int x,int y,char orient);
static void infob(int x,int y, unsigned char col[4]);

extern unsigned _stklen=8000U;  /* Set the stack to 6000 bytes, not 4096 */

monstruct            mon[LASTMON];
extern blkstruct far *blk;
RGBdata              colors[256];
int                  curblk=0,curmon=0,cursel=0,pick=0;
int                  adder=0;
int                  b=0,bcol;
char                 saved=1;
extern char          curfile[31];
extern char          wrap; // if 0 tell scrollist not to go past 0 or last one
char                 stopat=0; // used with wrap to stop at any desired spot
extern unsigned char FaceCols[MAXFACECOL];      //Screen colors
extern char          *FontPtr;
extern unsigned char HiCols[4];
unsigned char        lightcol[2][4];

void main(int argc,char *argv[])
  {
  unsigned register int lx;
  int attr=0;
  char w[850];
  int choice=1;

#ifdef DEBUG
  printf("Initialization finished.\n");
  NewAbortMSG("GM:ATE in main procedure.\n");
#endif
#ifdef CHKREG
  if ((argc < 2) || (strcmp(argv[1],OKMSG)!=0)) RestorePal=True;
#endif

  atexit(cleanup);
  FontPtr=GetROMFont();

  /*---------------------Set up Block structure */
  blk= (blkstruct far *) farmalloc( (unsigned long int) ( ((unsigned long int)sizeof(blkstruct))*(MONBL+2)));
  if (blk==NULL) { errorbox("NOT ENOUGH MEMORY!","  (Q)uit"); exit(menu);}
  for (lx=0; lx<sizeof(blkstruct)*(MONBL+2 );lx++)
    *( ((unsigned char far *) blk)+lx)=0;

  /*---------------------Set up Mouse  */
  if (initmouse()==FALSE)
    { errorbox("This application requires a Microsoft","compatible mouse! (Q)uit");
      exit(quit); }

  /* ----------------------------Set up palette-------------------------- */
  /* Set up colors as default.pal, and then the hardware palette if fails */

  if (!loadspecany("default",".pal","pal\\",sizeof(RGBdata)*256,(char *) colors))
    InitStartCols(colors);

  initalldata();

  do
    {
    SetTextColors();
    writestr(0,0,79,GMTOPBAR);
    writestr(0,1,31,"  Palette  Block  Monster  Map  Character  Image  Sound  Integrator  Main  Help ");
    attr = openmenu(20,3,22,13,w);
    writestr(20,3,PGMTITLECOL,"    MONSTER  MAKER    ");
    writestr(20,4,PGMTITLECOL,"     Version "GMVER"     ");
    writestr(20,5,PGMTITLECOL,"   By  Oliver Stone   ");
    writestr(20,6,PGMTITLECOL,"  Copyright (C) 1994  ");
    writestr(20,8,attr+14, " Choose a Monster Set");
    writestr(20,9,attr+14, " New Monster Set");
    writestr(20,10,attr+14," Choose a Block Set");
    writestr(20,11,attr+14," Choose a Palette");
    writestr(20,12,attr+14," Edit Monsters");
    writestr(20,13,attr+14," Save a Monster Set");
    writestr(20,14,attr+14," Delete a Monster Set");
    writestr(20,15,attr+14," Quit");
    choice=vertmenu(20,8,22,8,choice);
    closemenu(20,3,22,13,w);
    switch (choice) {
      case 1:
        if (!saved)
          switch( errorbox("Forget unsaved changes?","(Y)es (N)o")&0x00FF ) {
            case 'y': case 'Y':
              loadany("Enter name of monster set to load: ","*.mon\0",WorkDir,sizeof(monstruct)*LASTMON,(char *)&mon,1); choice = 2; saved = 1; break;
            default: choice = 5; break;
           }
        else {
          loadany("Enter name of monster set to load: ","*.mon\0",WorkDir,sizeof(monstruct)*LASTMON,(char *)&mon,1); choice = 2; saved = 1; }
        break;
      case 2:
        if (!saved) {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice = 5; break; }
         }
        initalldata(); 
        curfile[0]=0; 
        saved = 1; 
        errorbox("New monster set created!","(C)ontinue",2);
        break;
      case 3: loadblks(2,(char*)blk); break;
      case 4: loadany("Enter the palette to load:","*.pal\0",WorkDir,sizeof(RGBdata)*256,(char *)&colors,0); break;
      case 5: edit(); break;
      case 6: 
         choice=5;
         if (saveany("Enter name of monster set to save:",".mon",WorkDir,sizeof(monstruct)*LASTMON,(char *)&mon))
           { choice = 7; saved=TRUE; } 
         break;
      case 7: delany("Delete which monster set: ","*.mon\0",WorkDir); choice = 6; break;
      case 0: choice=8;
      case 8: if (!saved)
          switch( errorbox("Unsaved changes--Really Quit?","(Y)es (N)o")&0x00FF ) {
            case 'y': case 'Y': break;
            default: choice = 5; break;
           }
     }   
    choice++;
    } while (choice != 9);
   exit(menu);
  }

static void cleanup(void)
  {
  farfree(blk);
  Time.TurnOff();
  }


static void edit(void)
  {
  register int i;
  int inkey,selec=0;
  int x=0,y=0,oldx=0,oldy=0;
  int buts=0,oldbuts=0;
  char change=FALSE,change2=MONBL;
  int grabbed = -1,grabbed2=-1;
  char str[25];

  GraphMode();
  SetCols(colors,FaceCols);
  drawscrn();
  moucur(1);

  while(TRUE)
    {
    change = FALSE;
    do
     {
      if (bioskey(1)) change = TRUE;
      oldbuts=buts;
      oldy = y;
      oldx = x;
      moustats(&x,&y,&buts);
      if (buts>0) change = TRUE;
      else {
        if ((buts!=oldbuts)&(grabbed>=0))
         { drawblk(oldx-BLEN/2,oldy-BLEN/2,&blk[MONBL+1].p[0][0]);
           dropbloc(x,y,&grabbed); moucur(1); }
        if ((buts!=oldbuts)&(grabbed2>=0)) 
         { drawblk(oldx-BLEN/2,oldy-BLEN/2,&blk[MONBL+1].p[0][0]);
           dropmon(x,y); grabbed2=-1; moucur(1); }
       }
      if ( animons() )  /* Monster picture changed--redraw all? monsters */
       {
        if (grabbed2>=0) {
          grabbed2=mon[pick].pic[mon[pick].cur.pic][0];
          drawblk(oldx-BLEN/2,oldy-BLEN/2,&blk[grabbed2].p[0][0]);
         }
        if ( (x>MOX-11)&&(x<MOX1)&&(y>MOY-14)&&(y<MOY1) )
          moucur(0);
        if ( (x>SLX-(2*BLEN)-16)&&(x<SLX-BLEN-5)&&(y>SLY-14)&&(y<SLY1) )
          moucur(0);
        drawlist(MOX,MOY,MOLEN,LASTMON,&curmon,monnum);
        drawblk(SLX-(2*BLEN)-5,SLY,&blk[mon[pick].pic[mon[pick].cur.pic][0]].p[0][0]);
       }
      if (buts==0) {
        if ( (y>SLY)&(y<SLY1)&(x>SLX)&(x<SLX1)&
             (mon[pick].pic[cursel+(x-SLX)/BLEN][0]!=MONBL) )
         {
           if (change2 != (cursel+(x-SLX)/BLEN) ) {
            change2=cursel+(x-SLX)/BLEN;
            moucur(FALSE);
            sprintf(str,"Pause Length: %3d",mon[pick].pic[cursel+(x-SLX)/BLEN][1]);
            BoxWithText(SLX+SLLEN*BLEN/2-68,SLY1+3,str,FaceCols);
            }
          }
        else if (change2!=MONBL)
          {
          change2=MONBL;
          moucur(FALSE);
          BoxFill(SLX+SLLEN*BLEN/2-68,SLY1+3,SLX+SLLEN*BLEN/2+68,SLY1+11,FaceCols[GREY2]);
          }
        moucur(TRUE);
        }
      } while(!change);
    if (bioskey(1))
      {
      inkey=bioskey(0);
      if ((inkey==0x4800)||(inkey==(RIGHTKEY<<8)))  inkey=0x002B;  //  up arrow is '+'
      if ((inkey==0x5000)||(inkey==(LEFTKEY<<8))) inkey=0x002D;  // down arrow is '-'
      switch(inkey&255)
        {
        case '_':
        case '-':  if ( (x>SLX)&&(x<SLX1)&&(y>SLY)&&(y<SLY1)&&  
                        (mon[pick].pic[cursel+(x-SLX)/BLEN][1]>1) )
                     {
                     BoxFill(SLX+SLLEN*BLEN/2+45,SLY1+3,SLX+SLLEN*BLEN/2+68,SLY1+11,FaceCols[GREEN]);
                     sprintf(str,"%3d",(--mon[pick].pic[cursel+(x-SLX)/BLEN][1]));
                     GWrite(SLX+SLLEN*BLEN/2+45,SLY1+4,FaceCols[BLACK],str);
                     saved=0;
                     }
                   break;
        case '=':
        case '+':  if ( (x>SLX)&&(x<SLX1)&&(y>SLY)&&(y<SLY1)&& 
                        (mon[pick].pic[cursel+(x-SLX)/BLEN][1]<255) )
                     {
                     BoxFill(SLX+SLLEN*BLEN/2+45,SLY1+3,SLX+SLLEN*BLEN/2+68,SLY1+11,FaceCols[GREEN]);
                     sprintf(str,"%3d",(++mon[pick].pic[cursel+(x-SLX)/BLEN][1]));
                     GWrite(SLX+SLLEN*BLEN/2+45,SLY1+4,FaceCols[BLACK],str);
                     saved=0;
                     }
                   break;
        case 'i':
        case 'I':  if ( (x>MOX)&&(x<MOX1)&&(y>MOY)&&(y<MOY1) ) {
                     if (pick != ((curmon+((x-MOX)/BLEN))%LASTMON) ) {
                       if (mon[pick].pic[0][0] == MONBL) mon[pick].used=0;
                       pick = ((curmon+((x-MOX)/BLEN))%LASTMON);
                       cursel = 0;
                       mon[pick].used=1;
                      }
                     entermove();
                     moucur(0);
                     setmoupos((MOX+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN),MOY+BLEN/2);
                     drawscrn();
                     moucur(1);
                     moucurbox(0,0,310,190);
                    }
                   break;
        case 'Q':
        case 'q':
        case  27:  TextMode();  return;
        }
      }
    if (buts>0)
      {
      if ( (grabbed>=0)|(grabbed2>=0) )
        if ((x!=oldx)|(y!=oldy))
          drawblk(oldx-BLEN/2,oldy-BLEN/2,&blk[MONBL+1].p[0][0]);
      if ( (buts!=oldbuts)&&(x<MOX1)&&(x>=MOX)&&(y>=MOY)&&(y<MOY1+10) )
        {
        if (pick != ((curmon+((x-MOX)/BLEN))%LASTMON) )
          {
          moucur(0);
          if (mon[pick].pic[0][0] == MONBL) mon[pick].used=0;
          lightb(MOX-3+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN,MOY1+2,lightcol[0]);
          pick = ((curmon+((x-MOX)/BLEN))%LASTMON);
          lightb(MOX-3+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN,MOY1+2,lightcol[1]);
          cursel = 0;
          mon[pick].used=1;
          drawblk(SLX-(2*BLEN)-5,SLY,&blk[mon[pick].pic[mon[pick].cur.pic][0]].p[0][0]);
          drawlist(SLX,SLY,SLLEN,LASTSL,&cursel,selnum);
          moucur(1);
          }
        if (buts==2)
          {
          entermove();
          setmoupos(MOX+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN,MOY+BLEN);
          SetAllPalTo(&colors[FaceCols[BLACK]]); 
          drawscrn();
          moucurbox(0,0,310,190);
          }
        }
      if ((y>MOY)&(y<MOY1))        // Do something with monster list
        {
        if ( (x>MOX-11)&&(x<MOX-3) )  // Scroll left
          {
          moucur(0);
          lightb(MOX-3+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN,MOY1+2,lightcol[0]);
          DrawArrows(MOX,MOY,MOLEN,LEFT,FaceCols);
          scroll_list(1,MOX,MOY,MOLEN,LASTMON,&curmon,monnum);
          DrawArrows(MOX,MOY,MOLEN,NONE,FaceCols);
          lightb(MOX-3+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN,MOY1+2,lightcol[1]);
          }
        if ( (x<MOX1+11)&&(x>MOX1+2) )  // Scroll right
          {
          moucur(0);
          lightb(MOX-3+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN,MOY1+2,lightcol[0]);
          DrawArrows(MOX,MOY,MOLEN,RIGHT,FaceCols);
          scroll_list(-1,MOX,MOY,MOLEN,LASTMON,&curmon,monnum);
          DrawArrows(MOX,MOY,MOLEN,NONE,FaceCols);
          lightb(MOX-3+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN,MOY1+2,lightcol[1]);
          }
        if ( (buts==oldbuts)&&(x<MOX1)&&(x>MOX)&&(grabbed2<0) )
          {
          if ((selec++)>TIME)
            {
            grabbed2= mon[pick].pic[mon[pick].cur.pic][0];
            selec=0;
            moucur(0); moucurbox(0,0,MOX1+10,MOY1+20);
            getblk(x-BLEN/2,y-BLEN/2,&blk[MONBL+1].p[0][0]);
            }
          } else selec=0;
        }
      if ((y>SLY)&(y<SLY1))
       {
        wrap=0;
        if ( (x>SLX-11)&&(x<SLX-3)&&(cursel!=0) )
         { moucur(0); DrawArrows(SLX,SLY,SLLEN,LEFT,FaceCols); scroll_list(1,SLX,SLY,SLLEN,LASTSL,&cursel,selnum); DrawArrows(SLX,SLY,SLLEN,NONE,FaceCols); }
        if ( (x<SLX1+10)&&(x>SLX1+2)&&(cursel!=LASTSL-SLLEN) )
         { moucur(0); DrawArrows(SLX,SLY,SLLEN,RIGHT,FaceCols); scroll_list(-1,SLX,SLY,SLLEN,LASTSL,&cursel,selnum); DrawArrows(SLX,SLY,SLLEN,NONE,FaceCols); }
        if ( (buts!=oldbuts)&&(x>SLX-2*BLEN)&&(x<SLX-BLEN) )
         {
          grabbed2= mon[pick].pic[mon[pick].cur.pic][0];
          moucur(0); moucurbox(0,0,(MOX1+10)*2,MOY1+20);
          getblk(x-BLEN/2,y-BLEN/2,&blk[MONBL+1].p[0][0]);
         }
        if ( (buts!=oldbuts)&&(buts==1)&&(x<SLX1)&&(x>SLX) )
         {
          selec = cursel+(x-SLX)/BLEN;
          if (selec >= (LASTSL-1) )
            putch(0x07);
          else {
            moucur(0);
            saved=0;
            if (selec>0) if (mon[pick].pic[selec-1][0]==MONBL) selec--;
            if (mon[pick].pic[selec][0] == MONBL) {
              scroll_list(-1,SLX+(selec-cursel)*BLEN,SLY,SLLEN-(selec-cursel),cursel+SLLEN+1,&selec,selnum);
              for (i=selec;i<LASTSL;i++) {
                mon[pick].pic[i-1][0]=mon[pick].pic[i][0];
                mon[pick].pic[i-1][1]=mon[pick].pic[i][1];
               }
              mon[pick].pic[LASTSL-1][0]=MONBL;
              mon[pick].pic[LASTSL-1][1]=5;
             }
            else {
              for (i=LASTSL-1;i>selec;i--) {
                mon[pick].pic[i][0]=mon[pick].pic[i-1][0];
                mon[pick].pic[i][1]=mon[pick].pic[i-1][1];
               }
              mon[pick].pic[selec][0]=MONBL;
              mon[pick].pic[selec][1]=5;
              stopat=selec; /* stop scrolling at item selec */
              selec=1;      /* scroll one item (start at selec+1) */
              scroll_list(1,SLX+((stopat-cursel+LASTSL)%LASTSL)*BLEN,SLY,SLLEN-(stopat-cursel+LASTSL)%LASTSL,LASTSL+1,&selec,selnum);
              selec=stopat;
              stopat=0;
              mon[pick].pic[MONBL-1][0]=MONBL;
              mon[pick].pic[MONBL-1][1]=5;
             }
           }
	  moucur(1);
         }
        if ( (buts!=oldbuts)&(buts==2)&(x<SLX1)&(x>SLX) )
         {
          grabbed = mon[pick].pic[(cursel+(x-SLX)/BLEN)][0];
          moucur(0); moucurbox(SLX-10,SLY-20,SLX1+10,SLY1+20);
          getblk(x-BLEN/2,y-BLEN/2,&blk[MONBL+1].p[0][0]);
         }
        wrap=1;
       }
      if ((y>BLY)&(y<BLY1))
       {
        if ((x>BLX-11)&(x<BLX-3))
          { moucur(0); DrawArrows(BLX,BLY,BLLEN,LEFT,FaceCols); scroll_list(2,BLX,BLY,BLLEN,MONBL,&curblk,blocnum); DrawArrows(BLX,BLY,BLLEN,NONE,FaceCols); }
        if ((x<BLX1+10)&(x>BLX1+2))
          { moucur(0); DrawArrows(BLX,BLY,BLLEN,RIGHT,FaceCols); scroll_list(-2,BLX,BLY,BLLEN,MONBL,&curblk,blocnum); DrawArrows(BLX,BLY,BLLEN,NONE,FaceCols); }
        if ((buts!=oldbuts)&(x<BLX1)&(x>BLX))
         {
          grabbed= (curblk+(x-BLX)/BLEN)%MONBL;
          moucur(0); moucurbox(SLX-10,SLY-20,SLX1+10,SLY1+20);
          getblk(x-BLEN/2,y-BLEN/2,&blk[MONBL+1].p[0][0]);
         }
       }
      if ( (y>MY)&&(y<MY+10)&&(x>MX)&&(x<MX+36)&&(buts!=oldbuts) )
        { TextMode(); return; }
      if ( (y>IY)&&(y<IY+10)&&(x>IX)&&(x<IX+36)&&(buts!=oldbuts) )
        {
        entermove();
        setmoupos(MOX+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN,MOY+BLEN);
        drawscrn();
        moucurbox(0,0,310,190);
        }
      if ( (y>HY)&&(y<HY+10)&&(x>HX)&&(x<HX+36)&&(buts!=oldbuts) )
        {
        while(buts) moustats(&x,&y,&buts);
        TextMode();
        moucur(0);
        DisplayHelpFile(MONHELPFILE);
        mouclearbut(); while (bioskey(1)) bioskey(0);
        GraphMode(); setmoupos(MX+16,MY+18);
        drawscrn(); moucur(1);
        }
      if ( (grabbed>=0)|(grabbed2>=0) )
        {
        if ((x!=oldx)|(y!=oldy))
          getblk(x-BLEN/2,y-BLEN/2,&blk[MONBL+1].p[0][0]);
        drawblk(x-BLEN/2,y-BLEN/2,&blk[grabbed+grabbed2+1].p[0][0]);   /* either grabbed */
        }                       /* or grabbed2 = -1.  Add one to cancel this */
      }                              /* End of mouse button hit routine */
    }
  }

static void dropbloc(int x,int y,int *bloc)
 {
  int selec,i=0;

  if ( (y>SLY)&(y<SLY1)&(x>SLX)&(x<SLX1) )
   {
    saved=0;
    selec = cursel-1+(x-SLX)/BLEN;
    drawblk(x-(x-SLX)%BLEN,SLY,&blk[*bloc].p[0][0]);
    if (mon[pick].pic[selec+1][0]==MONBL) {
      while ( (mon[pick].pic[selec][0]==MONBL) & (selec!=-1) )
       {
        scroll_list(4,SLX,SLY,(x-SLX)/BLEN,LASTSL,&cursel,selnum);
        selec--;
        i++;
       }
     }
    mon[pick].pic[selec+1][0]=*bloc;
    if (i>(x-SLX)/BLEN) i = (x-SLX)/BLEN;
    while ( (i--) && (cursel!=LASTSL-SLLEN) )
      scroll_list(-2,SLX,SLY,SLLEN,LASTSL,&cursel,selnum);
    if ( (mon[pick].pic[cursel+SLLEN-1][0]!=MONBL)&(cursel!=LASTSL-SLLEN)&((x-SLX)/BLEN==SLLEN-1) )
      scroll_list(-2,SLX,SLY,SLLEN,LASTSL,&cursel,selnum);
   }
  *bloc = -1;
  moucurbox(0,0,310,190);
  moucur(1);
 }

static void dropmon(int x,int y)
 {
  unsigned register int i;
  char far *poin1;
  char far *poin2;

  if ( (y>MOY)&(y<MOY1)&(x>MOX)&(x<MOX1) )
   {
    saved=0;
    poin1=(char far *)&mon[pick];
    poin2=(char far *)&mon[(curmon+(x-MOX)/BLEN)%LASTMON];
    for (i=0;i<sizeof(monstruct);i++)
      *(poin2+i) = *(poin1+i);
    drawlist(MOX,MOY,MOLEN,LASTMON,&curmon,monnum);
   }
  moucurbox(0,0,310,190);
  moucur(1);
 }

static int animons(void)
 {
  int l,i=FALSE;

  for(l=0;l<LASTMON;l++) {
    if (mon[l].used) {
      if ( (mon[l].lasttime != Clock) &
           ( (Clock-mon[l].lasttime) >= mon[l].pic[mon[l].cur.pic][1] ) ) {
        if (mon[l].pic[(++mon[l].cur.pic)][0] == MONBL)
          mon[l].cur.pic = 0;
        mon[l].lasttime=Clock;
        i=TRUE;
       }
     }
   }
  return(i);
 }

static void drawlistbox(int x,int y,int blnum)
  {
  Line(x+BLEN*blnum,y,x+BLEN*blnum,y+BLEN-1,FaceCols[GREY3]);
  Line(x-1,y,x-1,y+BLEN-1,FaceCols[GREY3]);
  Line(x-1,y-1,x+BLEN*blnum,y-1,FaceCols[GREY4]);
  DrawArrows(x,y,blnum,NONE,FaceCols);
  }
  
static void initalldata()
  {
  unsigned register int lx,ly;

  for (lx=0;lx<sizeof(monstruct)*LASTMON;lx++)
    *( ((char far *)mon)+lx)=0;
  for (lx=0; lx<LASTMON; lx++)
   {
    for (ly=0;ly<LASTSL+1;ly++)
     {
      mon[lx].pic[ly][0] = MONBL;
      mon[lx].pic[ly][1] = 5;     /* define a speed to change pictures */
     }                            /* 'cuz a 0 would screw things up.   */
    mon[lx].close[0]=8;
    mon[lx].close[1]=5;
    mon[lx].cur.pic = 0;
    mon[lx].newmon = LASTMON;
   }
  curmon=0;
  pick = 0;
  curblk=0;
  mon[0].used=1;
  }

static void drawscrn(void)
  {
  register int lx,ly;
  
  moucur(0);
  SetAllPalTo(&colors[FaceCols[BLACK]]);
  drawsurface(MOX-13,MOY-16,MOLEN*BLEN+26,100,FaceCols);
  drawlistbox(MOX,MOY,MOLEN);
  drawlistbox(BLX,BLY,BLLEN);
  drawlistbox(SLX,SLY,SLLEN);
  for (lx=0;lx<BLEN;lx++)
    for (ly=0;ly<BLEN;ly++)
      blk[MONBL].p[ly][lx] = FaceCols[GREY2];
  lightcol[0][0]=FaceCols[GREY1]; lightcol[0][1]=FaceCols[GREY4]; lightcol[0][2]=FaceCols[GREY3]; lightcol[0][3]=FaceCols[GREY2];
  lightcol[1][0]=FaceCols[GREY1]; lightcol[1][1]=FaceCols[RED2]; lightcol[1][2]=FaceCols[RED1]; lightcol[1][3]=FaceCols[RED3];
  for (lx=0;lx<MOLEN;lx++)
    lightb(MOX-3+BLEN/2+BLEN*lx,MOY1+2,lightcol[0]);
  Line(SLX-(2*BLEN)-6,SLY-1,SLX-(2*BLEN)-6,SLY+BLEN-1,FaceCols[GREY3]);
  Line(SLX-(2*BLEN)-6,SLY-1,SLX-BLEN-6,SLY-1,FaceCols[GREY4]);
  BoxFill(SLX+SLLEN*BLEN/2-68,SLY1+3,SLX+SLLEN*BLEN/2+68,SLY1+11,FaceCols[GREY2]);
  Line(SLX+SLLEN*BLEN/2-69,SLY1+2,SLX+SLLEN*BLEN/2-69,SLY1+11,FaceCols[GREY3]);
  Line(SLX+SLLEN*BLEN/2-69,SLY1+2,SLX+SLLEN*BLEN/2+68,SLY1+2,FaceCols[GREY4]);
  drawlistframe(BLX,BLY,BLLEN,FaceCols);
  drawlist(SLX,SLY,SLLEN,LASTSL,&cursel,selnum);
  drawlist(BLX,BLY,BLLEN,MONBL,&curblk,blocnum);
  drawlist(MOX,MOY,MOLEN,LASTMON,&curmon,monnum);
  drawblk(SLX-(2*BLEN)-5,SLY,&blk[mon[pick].pic[mon[pick].cur.pic][0]].p[0][0]);
  lightb(MOX+BLEN/2+((pick-curmon+LASTMON)%LASTMON)*BLEN-3,MOY1+2,lightcol[1]);
  drawsurface(MX-4,MY-3,124,18,FaceCols);
  menub(MX,MY,FaceCols);
  helpb(HX,HY,FaceCols);
  infob(IX,IY,FaceCols);
  BoxWithText(MOX+BLEN*MOLEN/2-32,MOY-12,"Monsters",FaceCols);
  BoxWithText(SLX+BLEN*SLLEN/2-32,SLY-12,"Sequence",FaceCols);
  FadeTo(colors);
  moucur(1);
 }

int blocnum(int mun)
 {
  if ( animons() )
   {
    drawblk(SLX-(2*BLEN)-5,SLY,&blk[mon[pick].pic[mon[pick].cur.pic][0]].p[0][0]);
    drawlist(MOX,MOY,MOLEN,LASTMON,&curmon,monnum);
   }
  return(mun);
 }

int monnum(int rebmun)      /* returns the current bloc number of the */
 {                          /*         monster number 'rebmun'.       */
  int i;

  if (animons())
    drawblk(SLX-(2*BLEN)-5,SLY,&blk[mon[pick].pic[mon[pick].cur.pic][0]].p[0][0]);
  i=mon[rebmun].pic[mon[rebmun].cur.pic][0];
  return(i);
 }

int monnum2(int rebmun)      /* returns the current bloc number of the */
 {                          /*         monster number 'rebmun'.       */
  int i;

  if (mon[pick].newmon!=LASTMON)
    if (animons())
      drawblk(SELX,SELY,&blk[mon[mon[pick].newmon].pic[mon[mon[pick].newmon].cur.pic][0]].p[0][0]);
  i=mon[rebmun].pic[mon[rebmun].cur.pic][0];
  return(i);
 }

int selnum(int mun)         /* returns the next picture number in a */
 {                          /*          monster   series            */
  int i;

  if ( animons() )
   {
    drawblk(SLX-(2*BLEN)-5,SLY,&blk[mon[pick].pic[mon[pick].cur.pic][0]].p[0][0]);
    drawlist(MOX,MOY,MOLEN,LASTMON,&curmon,monnum);
   }
  i = mon[pick].pic[mun+stopat][0];
  return(i);
 }

static void entermove(void)
 {
  char str[60];
  char tcol=2;
  int choice=1;
  int temp=0;
  char info[2][15] = { "independent of","relative to\0\0\0" };
  int attr=0;
  char w[1000];

  if (!mon[pick].used) { return; }
  moucur(0);
  TextMode();
  do {
    clrbox(0,0,79,23,1);
    drawbox (0,0,77,22,1,2,8);
    writestr(28,2,14,"Monster Movement Menu");
    sprintf(str,"Monster #%3d",pick +1);
    writestr(31,3,4,str);

    writestr(3,6,tcol,"Return to graphical edit screen");
    writestr(3,7,tcol,"Help");
    if (mon[pick].towards>=64)
      sprintf(str,"Move Randomly, speed %2d                                 ",(mon[pick].towards>>6) );
    else if (mon[pick].towards>0)
      sprintf(str,"Normal movement always towards the character, speed %2d  ",mon[pick].towards);
    else if (mon[pick].towards==0)
      sprintf(str,"Move according to the movement pattern                  ");
    else if (mon[pick].towards<0)
      sprintf(str,"Normal movement always away from the character, speed %2d",-mon[pick].towards);
    writestr(3,9,tcol,str);
    if (!mon[pick].towards)
    writestr(3,10,tcol,"Make movement pattern");
    else writestr(3,10,tcol,"                     ");
    if (!mon[pick].activate) sprintf(str,"No Attack Movement                             ");
    else sprintf(str,"Attack Movement is %s the character   ",&info[mon[pick].activate-1][0]);
    writestr(3,11,tcol,str);
    if (mon[pick].activate) writestr(3,12,tcol,"Make attack movement pattern");
    else writestr(3,12,tcol,"                            ");
    if (mon[pick].thru) writestr(3,13,tcol,"Monster blocked by solid blocks  ");
    else writestr(3,13,tcol,"Monster passes through all blocks");
    sprintf(str,"Power Level: %3d",mon[pick].power);
    writestr(3,14,tcol,str);
    if (mon[pick].end == -2) sprintf(str,"Monster never dies                 ");
    else if (mon[pick].end == -1) sprintf(str,"Monster dies after path            ");
    else if (mon[pick].end == 0) sprintf(str,"Monster lives until another hits it");
    else sprintf(str,"Monster dies after %d time units   ",mon[pick].end);
    writestr(3,15,tcol,str);
    if (mon[pick].newmon == LASTMON) sprintf(str,"No Monster is Born at Death            ");
    else sprintf(str,"Another Monster is Born at Death (#%d) ",mon[pick].newmon+1);
    writestr(3,16,tcol,str);
    sprintf(str,"If Killed, Increase Score by %d Points.    ",mon[pick].upscore);
    writestr(3,17,tcol,str);


    moucur(1);
    choice=vertmenu(3,6,56,12,choice);
    moucur(0);
    switch (choice) {
      case  0: choice=1; break;
      case  2: mouclearbut();
                DisplayHelpFile(PATHHELPFILE);
                mouclearbut(); while (bioskey(1)) bioskey(0);
                break;
      case  4: attr = openmenu(10,10,9,4,w);
                writestr(10,10,attr+15," Away");
                writestr(10,11,attr+15," Pattern");
                writestr(10,12,attr+15," Towards");
                writestr(10,13,attr+15," Random");
                mon[pick].towards = vertmenu(10,10,9,4,Sign(mon[pick].towards)+2+(mon[pick].towards/1000)) - 2;
                closemenu(10,10,9,4,w);
                saved=0;
                if (mon[pick].towards==2) mon[pick].towards=64; /* 1<<6 */
                if (mon[pick].towards) {
                  str[0]=0;
                  if (qwindow(8,13,2,"Enter Speed: ",str))
                    {
                    sscanf(str,"%d",&temp);
                    if ( (temp>0)&(temp<=20) ) mon[pick].towards*=temp;
                    if (mon[pick].end == -1) mon[pick].end = 0;
                    }
                 }
                else mon[pick].thru=0;
                break;
      case  5: if (!mon[pick].towards) {
                 makepath(mon[pick].march[0],4);
                 TextMode();
                } break;
      case  6: mon[pick].activate++; mon[pick].activate%=3; saved=0; break;
      case  7: if (mon[pick].activate>0) {
                 makepath(mon[pick].march[1],mon[pick].activate+1);
                 TextMode();
                } break;
      case  8: 
               mon[pick].thru^=1; 
               saved=0;
               break;
      case  9: str[0]=0; saved=0;
               if (qwindow(8,13,3,"Enter Power Level: ",str))
                 {
                 sscanf(str,"%d",&temp);
                 if ( (temp>=0)&(temp<256) ) mon[pick].power=temp;
                 }
               break;
      case  10: attr = openmenu(10,8,18,6,w);
                writestr(10,8,attr+4," Monster Dies:");
                writestr(10,10,attr+15," Never");
                writestr(10,11,attr+15," After Pattern");
                writestr(10,12,attr+15," Only When Killed");
                writestr(10,13,attr+15," After Time");
                if (mon[pick].end > 0) mon[pick].end = 1;
                mon[pick].end = vertmenu(10,10,18,4,mon[pick].end+3) - 3;
                closemenu(10,8,18,6,w);
                saved=0;
                if (mon[pick].end>0)
                  {
                  str[0]=0;
                  if (qwindow(8,13,4,"Enter Time: ",str))
                    {
                    sscanf(str,"%d",&temp);
                    if ( (temp>0)&(temp<10000) ) mon[pick].end=temp;
                    }
                  }
                if ( (mon[pick].end == -1)&(mon[pick].towards!=0) ) {
                  errorbox("Pattern is undefined!","      (O)k");
                  mon[pick].end=0;
                 }
                break;
      case 11: pickmon(); break;
      case 12: str[0]=0; saved=0;
               if (qwindow(8,13,5,"Enter New Score Increase: ",str))
                 {
                 sscanf(str,"%d",&temp);
                 if ( (temp>-10000)&(temp<10000) ) mon[pick].upscore=temp;
                 }
               temp=mon[pick].upscore;

               break;
     }
   } while(choice!=1);
  GraphMode();
  SetAllPal(colors);
  mouclearbut();
  return;
 }

static void makepath(coords move[10],char status)
 {
  register int j;
  int mx,my,oldx,oldy,lastx,lasty;
  char spot=-1,spot2=-1,frombox=1,redraw=0;
  int inkey,buts;
  char mystr[20];
  int rs,bs,wid,len;

  
  if (status==4) { wid = 30; len=30; }
  else           { len=(640/BLEN); wid=(400/BLEN); }
  rs = LS+len; bs = TS+wid; 
  for (j=0;j<10;j++) {
    move[j].x+=len/2;
    move[j].y+=wid/2;
   } 
  GraphMode();
  moucur(0);
  mx=(oldx=LS+(lastx=len/2));
  my=(oldy=TS+(lasty=wid/2));
  while ( (move[spot+1].pic>0)&&(spot<9) ) {
    spot++;  oldx=(mx=(lastx=move[spot].x)+LS);  oldy=(my=(lasty=move[spot].y)+TS); }
  if (spot>=0) {  /*oldx=mx+1;  oldy=my+1;*/  spot2=0; }
  setmoupos(lastx+LS,lasty+TS );
  drawscrn2(len,wid,spot,spot2,oldx,oldy,status,move);
  sprintf(mystr,"(%3d,%3d)",lastx-len/2,-lasty+wid/2);
  BoxWithText(TTX,TY-1,mystr,FaceCols);
  mouclearbut();

  while (1) 
    {
    moustats(&mx,&my,&buts);
    if (frombox)
      {
      if ( (mx>=rs)||(mx<LS)||(my<TS)||(my>=bs) )
        {
        my=(my-TS)*4+TS; mx=(mx-LS)*4+LS;
        setmoupos(mx,my); moucur(1);
        frombox=0;
        BoxFill(TTX,TY-1,TTX+72,TY+7,FaceCols[GREY2]);
        }
      }
    else
      {
      if ( (mx>=LS)&&(mx<LS+len*4)&&(my<TS+wid*4)&&(my>=TS) )
        {
        my=(my-TS)/4+TS; mx=(mx-LS)/4+LS;
        setmoupos(mx,my); moucur(0);
        frombox=1;
        sprintf(mystr,"(%3d,%3d)",mx-LS-len/2,-my+TS+wid/2);
        BoxWithText(TTX,TY-1,mystr,FaceCols);
        }
      }
    if ( (mx>=LS)&&(mx<rs)&&(my>=TS)&&(my<bs) )
      {
      if ( (oldx!=mx)||(oldy!=my) )
        {
        if ( (status==3)&&(spot<0) ) Point(LS+4*(oldx-LS),TS+(oldy-TS)*4,BKGCOL);
        else myline(lastx,lasty,oldx-LS,oldy-TS,BKGCOL);
        redraw=1;
        sprintf(mystr,"(%3d,%3d)",mx-LS-len/2,-my+TS+wid/2);
        BoxWithText(TTX,TY-1,mystr,FaceCols);
        oldx=mx;  oldy=my;
        }
      if ( (buts>0)&&(spot<9) )
        { 
        saved=0;
        lastx=mx-LS;
        lasty=my-TS;
        spot++; spot2= spot;
        move[spot].x=lastx;
        move[spot].y=lasty;
        move[spot].pic=1;
        redraw=1;                                 //Redraw the path box
        mouclearbut(); buts=0;
        }
      }
    else
      {
      if (buts>0)
        {
        if ((mx>PMX)&&(mx<(PMX+36))&&(my>PMY)&&(my<PMY+11))  // Menu box hit
          {
          moucurbox(0,0,315,195);
          for (j=0;j<10;j++)
            {
            move[j].x-=len/2;
            move[j].y-=wid/2;
            } 
          TextMode();
          mouclearbut();
          return;
          }
        if ((mx>PHX)&&(mx<(PHX+36))&&(my>PHY)&&(my<PHY+11))  // Help box hit
          {
          while(buts) moustats(&mx,&my,&buts);
          TextMode();
          moucur(0);
          DisplayHelpFile(PATHHELPFILE);                 //Ollie 7-29
          mouclearbut(); while (bioskey(1)) bioskey(0);
          GraphMode(); setmoupos(PHX+18,PHY+5);
          drawscrn2(len,wid,spot,spot2,oldx,oldy,status,move); moucur(1);
          }
        if ( (my>OY+9)&&(my<OY+19)&&(spot2>-1) )
          {
          if ( (mx>OX+BLEN+31)&&(mx<OX+BLEN+41) )              // toggle rotate top bloc
            { move[spot2].orient^=0x01; if (move[spot2].orient&0x04) move[spot2].orient^=0x02; } //set rotate bit.  if either flip or rotate, switch them
          if ( (mx>OX+BLEN+41)&&(mx<OX+BLEN+51) )           // toggle h. flip top bloc
            move[spot2].orient^=0x04;
          if ( (mx>OX+BLEN+51)&&(mx<OX+BLEN+61) )           // toggle v. flip top bloc
            move[spot2].orient^=0x06;                    // Hey, they work. don't question it.
          mouclearbut();
          redraw=1;
          }
        }
      }
    if (bioskey(1))
      {
      inkey = bioskey(0);
      if ( (inkey&0x00FF)!=0 )
        {
        switch (inkey&0x00FF) {
          case 0x08:
            if (spot>-1)
              { 
              BoxFill(LS,TS,LS+len*4-4,TS+wid*4-4,BKGCOL);
              oldx=lastx+LS; oldy=lasty+TS;
              move[spot].x=0; move[spot].y=0; move[spot].pic=0;
              spot--;  if (spot2>spot) spot2=spot;
              if (spot>-1)             // (if spot is STILL > -1 after spot--
                { 
                lastx = move[spot].x; 
                lasty = move[spot].y;
                }
              else                    //      spot == -1
                {
                lastx=len/2;  lasty=wid/2;
                }
              oldx=lastx+LS; oldy=lasty+TS;
              saved=0;
              }
            break;
          case 'S': case 's':
            if (spot2>-1) { move[spot2].orient^=4; saved=0; }
            break;
          case 'R': case 'r':
            if (spot2>-1)
              {
              if (move[spot2].orient%4 == 3) move[spot2].orient&=4;
              else move[spot2].orient++;
              saved=0;
              }
            break;
          case 27:
            moucurbox(0,0,315,195);
            for (j=0;j<10;j++)
              {
              move[j].x-=len/2;
              move[j].y-=wid/2;
              } 
            TextMode();
            return;
            default: break;
          }
        }
      else
        {
        inkey >>= 8;
        switch (inkey) {
          case 75:   /* left arrow */
            if (spot>-1)
              { spot2+=spot; spot2%=(spot+1); }
            break;
          case 77:   /* right arrow */
            if (spot>-1)
              { spot2++; spot2%=(spot+1); }
            break;
          case 72:   /* up arrow */
            if (spot2>-1)
              if (move[spot2].pic<50) { move[spot2].pic++; saved=0; }
            break;
          case 80:   /* down arrow */
            if (spot2>-1)
              if (move[spot2].pic>1) { move[spot2].pic--; saved=0; }
            break;
          }
        }
      redraw=1;
      }
    if (animons())
      if (spot2>-1)
        {
        if ( (mx>OX-11)&&(mx<OX+BLEN)&&(my>OY-14)&&(my<OY+BLEN) ) moucur(0);
        drawbl(OX,OY,mon[pick].pic[mon[pick].cur.pic][0],move[spot2].orient);
        if ( (mx>OX-11)&&(mx<OX+BLEN)&&(my>OY-14)&&(my<OY+BLEN) ) moucur(1);
        }
      else
        {
        if ( (mx>OX-11)&&(mx<OX+BLEN)&&(my>OY-14)&&(my<OY+BLEN) ) moucur(0);
        drawbl(OX,OY,mon[pick].pic[mon[pick].cur.pic][0],0);
        if ( (mx>OX-11)&&(mx<OX+BLEN)&&(my>OY-14)&&(my<OY+BLEN) ) moucur(1);
        }
    if (redraw)
      {
      redraw=0;
      if ( (mx>OX-11)&&(mx<OX+110)&&(my>OY-14)&&(my<OY+BLEN+26) ) moucur(0);
      if (spot>-1)
        {
        if (status!=3) myline(len/2,wid/2,move[0].x,move[0].y,FaceCols[RED1]);
        for (j=0;j<spot;j++)
          myline(move[j].x,move[j].y,move[j+1].x,move[j+1].y,FaceCols[RED1]);
        if (spot2>0)
          myline(move[spot2-1].x,move[spot2-1].y,move[spot2].x,move[spot2].y,FaceCols[GREEN]);
        else if (status!=3) myline(len/2,wid/2,move[0].x,move[0].y,FaceCols[GREEN]);
             else           Point(LS+move[0].x*4,TS+move[0].y*4,FaceCols[GREEN]);
        DrawFnButs(OX+BLEN+31,OY+9,move[spot2].orient);
        sprintf(mystr,"At Speed: %3d ",move[spot2].pic); 
        BoxWithText(OX-2,OY+18+BLEN,mystr,FaceCols);
        sprintf(mystr,"Go To: %3d,%3d",move[spot2].x-len/2,-move[spot2].y+wid/2);
        BoxWithText(OX-2,OY+6+BLEN,mystr,FaceCols);
        drawbl(OX,OY,mon[pick].pic[mon[pick].cur.pic][0],move[spot2].orient);
        }
      else
        {
        BoxFill(OX-2,OY+6+BLEN,OX+110,OY+14+BLEN,FaceCols[GREY2]);
        BoxFill(OX-2,OY+18+BLEN,OX+110,OY+26+BLEN,FaceCols[GREY2]);
        DrawFnButs(OX+BLEN+31,OY+9,0);
        }
      if ((spot==-1)&&(status==3))
        Point(LS+(oldx-LS)*4,TS+(my-TS)*4,FaceCols[RED1]);
      else myline(lastx,lasty,oldx-LS,oldy-TS,FaceCols[RED1]);
      if ( (mx>OX-11)&&(mx<OX+110)&&(my>OY-14)&&(my<OY+BLEN+26) ) moucur(1);
      }
    }
  }

static void DrawFnButs(int x,int y,char orient)
  {
  char funar[3][26]= {"   UPeAV”" "\x1A" "ªA–”eA•T5UÀÿð",
                      "   UPeAª”eAYTªAYT5UÀÿð",
                      "   UPUAe”jA©”YAU”5UÀÿð"};
  unsigned char col[2][4] = { { FaceCols[GREY3],FaceCols[RED1],
                       FaceCols[BLACK],FaceCols[RED2] },
                     { FaceCols[GREY3],FaceCols[RED3],
                       FaceCols[GREEN],FaceCols[GREY3] } };
  char dir[6];
  int j;

  for (j=0;j<3;j++)
    { dir[j]=orient&0x01; orient>>=1; }
    
  draw4dat(x,y+dir[0],funar[0],9,9,col[dir[0]]);
  draw4dat(x+11,y+(dir[1]^dir[2]),funar[1],9,9,col[(dir[1]^dir[2])]);
  draw4dat(x+22,y+dir[1],funar[2],9,9,col[dir[1]]);
  }

static void drawscrn2(int len,int wid,char spot,char spot2,int oldx,int oldy,char status,coords *move)
  {
  char mystr[30];
  register int j;
    
  SetAllPalTo(&colors[FaceCols[BLACK]]);
  BoxFill(0,0,319,199,BKGCOL);
  
  drawsurface(TX-2,TY-16,TTX+77-TX,38,FaceCols);
  GWrite(TX+54,TY-14,FaceCols[RED1],"Next Leg");
  GWrite(TX,TY,FaceCols[RED1],"Map position");
  GWrite(TX,TY+12,FaceCols[RED1],"Relative to");
  if (status==3) BoxWithText(TTX,TY+11,"Character",FaceCols);
    else BoxWithText(TTX,TY+11," Monster ",FaceCols);
  BoxFill(TTX,TY-1,TTX+72,TY+7,FaceCols[GREY2]);
  Line(TTX-1,TY-1,TTX-1,TY+7,FaceCols[GREY2]);
  Line(TTX-1,TY-2,TTX+72,TY-2,FaceCols[GREY2]);
  

  drawsurface(OX-5,OY-5,118,BLEN+34,FaceCols);
  GWrite(OX+BLEN+4,OY-2,FaceCols[RED1],"Current Leg");
  BoxFill(OX+BLEN+28,OY+9,OX+BLEN+65,OY+20,FaceCols[GREY3]);
  DrawFnButs(OX+BLEN+31,OY+9,0);
  BoxFill(OX-2,OY-2,OX+BLEN+1,OY+BLEN+1,FaceCols[GREEN]);
  Line(OX-3,OY-2,OX-3,OY+BLEN+1,FaceCols[GREY3]);
  Line(OX-3,OY-3,OX+BLEN+1,OY-3,FaceCols[GREY4]);
  drawbl(OX,OY,mon[pick].pic[mon[pick].cur.pic][0],0);
  BoxFill(OX-2,OY+BLEN+6,OX+110,OY+BLEN+14,FaceCols[GREY2]);
  Line(OX-3,OY+BLEN+5,OX-3,OY+BLEN+14,FaceCols[GREY3]);
  Line(OX-3,OY+BLEN+5,OX+110,OY+BLEN+5,FaceCols[GREY4]);
  BoxFill(OX-2,OY+18+BLEN,OX+110,OY+26+BLEN,FaceCols[GREY2]);
  Line(OX-3,OY+BLEN+17,OX-3,OY+BLEN+26,FaceCols[GREY3]);
  Line(OX-3,OY+BLEN+17,OX+110,OY+BLEN+17,FaceCols[GREY4]);

  drawsurface(PHX-3,PHY-2,42,28,FaceCols);          //  Draw the menu
  helpb(PHX,PHY,FaceCols);
  menub(PMX,PMY,FaceCols);

  drawborder(LS,TS,len*4-3,wid*4-3,2,2,FaceCols);
  Point(LS+(len/2)*4,TS+(wid/2)*4,FaceCols[RED1]);
  if (spot>-1)
    { 
    if (status!=3) myline(len/2,wid/2,move[0].x,move[0].y,FaceCols[RED1]);
    for (j=0;j<spot;j++)
      myline(move[j].x,move[j].y,move[j+1].x,move[j+1].y,FaceCols[RED1]);
    myline(move[spot].x,move[spot].y,oldx-LS,oldy-TS,FaceCols[GREEN]);
    if (spot2>0)
      myline(move[spot2-1].x,move[spot2-1].y,move[spot2].x,move[spot2].y,FaceCols[GREEN]);
    else if (status!=3) myline(len/2,wid/2,move[0].x,move[0].y,FaceCols[GREEN]);
         else           Point(LS+move[0].x*4,TS+move[0].y*4,FaceCols[GREEN]);
    sprintf(mystr,"At Speed: %3d ",move[spot2].pic); 
    BoxWithText(OX-2,OY+18+BLEN,mystr,FaceCols);
    sprintf(mystr,"Go To: %3d,%3d",move[spot2].x-len/2,-move[spot2].y+wid/2);
    BoxWithText(OX-2,OY+6+BLEN,mystr,FaceCols);
    }
  else                    //      spot == -1
    {
    if (status==3) Point(LS+(oldx-LS)*4,TS+(oldy-TS)*4,FaceCols[GREEN]);
    else           myline(len/2,wid/2,oldx-LS,oldy-TS,FaceCols[GREEN]);
    }

  FadeTo(colors);
  }
  
static void pickmon(void)
  {
  int x,y,buts;
  static int curmon2=0;

  GraphMode();
  SetAllPalTo(&colors[BLACK]);
  drawlistframe(MOX,MOY2,MOLEN,FaceCols);
  drawborder(SELX,SELY,BLEN,BLEN,2,2,FaceCols);
  drawsurface(MX3-2,MY3-1,40,27,FaceCols);
  noneb(MX3,MY3,FaceCols);
  doneb(MX3,MY3+13,FaceCols);
  moucurbox(1,1,319,199);
  drawlist(MOX,MOY2,MOLEN,LASTMON,&curmon2,monnum2);
  if (mon[pick].newmon==LASTMON)
    BoxFill(SELX,SELY,SELX+BLEN-1,SELY+BLEN-1,FaceCols[GREY2]);
  else drawblk(SELX,SELY,&blk[mon[mon[pick].newmon].pic[mon[mon[pick].newmon].cur.pic][0]].p[0][0]);
  drawborder(TBOXX,TBOXY,249,9,5,2,FaceCols);
  BoxWithText(TBOXX,TBOXY,"Pick Monster to Appear at Death",FaceCols);
  FadeTo(colors);
  moucur(1);
  while(1) {
    moustats(&x,&y,&buts);
    if (animons())
      {
      moucur(0);
      drawlist(MOX,MOY2,MOLEN,LASTMON,&curmon2,monnum2);
      if (mon[pick].newmon!=LASTMON)
        drawblk(SELX,SELY,&blk[mon[mon[pick].newmon].pic[mon[mon[pick].newmon].cur.pic][0]].p[0][0]);
      moucur(1);
      }
    if (buts)
      {
      if ( (x>MX3)&&(x<MX3+36) )
        {
        if ( (y>MY3+12)&&(y<MY3+22) ) { mouclearbut(); TextMode(); return; }
        if ( (y>MY3)&&(y<MY3+10) )
          {
          BoxFill(SELX,SELY,SELX+BLEN-1,SELY+BLEN-1,FaceCols[GREY2]);
          mon[pick].newmon=LASTMON;
          saved=0;
          }
        }
      if ( (y>=MOY2)&&(y<MOY2+BLEN) )
        {
        moucur(0);
        if ( (x>=MOX)&&(x<MOX1) )
          {
          mon[pick].newmon = (curmon2+(x-MOX)/BLEN)%LASTMON;
          drawblk(SELX,SELY,&blk[mon[mon[pick].newmon].pic[mon[mon[pick].newmon].cur.pic][0]].p[0][0]);
          saved=0;
          }
        if ( (x>MOX-11)&&(x<MOX-3) )
          { DrawArrows(MOX,MOY2,MOLEN,LEFT,FaceCols); scroll_list(2,MOX,MOY2,MOLEN,LASTMON,&curmon2,monnum2); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        if ( (x<MOX1+10)&&(x>MOX1+2) )
          { DrawArrows(MOX,MOY2,MOLEN,RIGHT,FaceCols); scroll_list(-2,MOX,MOY2,MOLEN,LASTMON,&curmon2,monnum2); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        moucur(1);
       }
     }
   }
 }

static void myline(int ox,int oy,int ox1,int oy1,char col)
 {
  Line(ox*4+LS,oy*4+TS,ox1*4+LS,oy1*4+TS,col);
 }

static void drawbl(int x,int y,int blo,char orient)
 {
  register int j,k;

  for (j=0;j<BLEN;j++)
    for (k=0;k<BLEN;k++)
      switch(orient) {
        case 0: Point(x+j,y+k,blk[blo].p[k][j]);               break;
        case 1: Point(x+j,y+k,blk[blo].p[BLEN-j-1][k]);        break;
        case 2: Point(x+j,y+k,blk[blo].p[BLEN-k-1][BLEN-j-1]); break;
        case 3: Point(x+j,y+k,blk[blo].p[j][BLEN-k-1]);        break;
        case 4: Point(x+j,y+k,blk[blo].p[k][BLEN-j-1]);        break;
        case 5: Point(x+j,y+k,blk[blo].p[j][k]);               break;
        case 6: Point(x+j,y+k,blk[blo].p[BLEN-k-1][j]);        break;
        case 7: Point(x+j,y+k,blk[blo].p[BLEN-j-1][BLEN-k-1]); break;
       }
  }

static void lightb(int x,int y, unsigned char col[4])
  {
  static char lightar[] = "@\x1Ajäkäj¤\x1A@";
  if ((x>MOX)&(x<MOX1)) draw4dat(x,y,lightar,7,6,col);
  }

static void infob(int x,int y, unsigned char col[4])
  {
  static char infoar[] = "         UUUUUUU@UUUUUUUTU]××ÕUTUU]÷u]uUUUU]ÿ]uUUUU]ßu]uUU•U]×uWÕUVUUUUUUUT)UUUUUUUhªªªªªªª€";
  draw4dat(x,y,infoar,35,10,col);
  }
