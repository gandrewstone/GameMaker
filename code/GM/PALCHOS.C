/*------------------------------------------------------------------------*/
/* palchos.c      Program that allows the user to pick palettes           */
/* Programmer: Andy Stone       Created: 12/27/90                         */
/* Last Edited 6/17/91                                                    */
/* c:\c2\tcc -ms PALCHOS.C pala.asm palc.c mousefn.c scrnrout.asm         */ 
/* windio.c genc.c gena.asm                                               */
/*------------------------------------------------------------------------*/


#include <stdio.h>
#include <process.h>
#include <dos.h>
#include <bios.h>
#include <stdlib.h>

#include "gen.h"
#include "scrnrout.h"
#include "mousefn.h"
#include "gmgen.h"
#include "windio.h"
#include "palette.h"
#include "jstick.h"
#include "facelift.h"
#include "graph.h"
#include "tranmous.hpp"

#define  TRANSCOL 255     // The value that is transparent in _pic fns
#define  TRUE 1
#define  FALSE 0
#define  COPY 0
#define  MOVE 1
#define  SLOWAMT 15
#define  CX 33
#define  CY 26
#define  CY1 CY+10
#define  CY2 CY+20
#define  CX1 (CX+MAXCOLS)
#define  MAXCOLS 256
#define  HELPFILE "pal.hlp"
#define  SPLOTX 195
#define  SPLOTY 60
#define  SPLOTX1 292
#define  SPLOTY1 157
#define  MENUX HELPX
#define  MENUY (HELPY+12)
#define  HELPX 29
#define  HELPY 59
#define  COLBX 30
#define  COLBY 106
#define  COLBLEN (MINUSB-COLBX+2*BUSIZE+1)
#define  COLBWID 11
#define  BUSIZE 16
#define  MINUSB (COLBX+67)
#define  PLUSB  (MINUSB+BUSIZE+1)
#define  TBX 30
#define  TBY 160
#define  TBLEN 153
#define  TBWID 9

static boolean ResetCols(RGBdata *colors,unsigned char *FaceCols);
static void drawcolnums(void);
static void getcolnum(unsigned char *in);
static void RotPal(int start,int end,int dir,RGBdata *pal);
static void drawscrn(void);
static void drawsplot(void);
static void drawcolorchart(void);
static void drawbuttons(void);
static void minushalf(int x,int y, unsigned char col[4]);
static void plushalf(int x,int y, unsigned char col[4]);
static void drawcolchange(void);
static void edit(void);
static void dohelp(void);
static void cleanup(void);

extern unsigned      _stklen=8000U;  // Set the stack to 6000 bytes, not 4096

static RGBdata colors[256];
ConfigStruct   cs;
static int     cnum=0;
static int     havepal=FALSE;
static char    saved=1;
extern char    curfile[31];
extern uchar   FaceCols[MAXFACECOL];      //Screen colors
extern char   *FontPtr;
extern uchar   HiCols[4];
static int     curcol;

QuitCodes main(int argc,char *argv[])
  {
  int ink=1,attr=1;
  int l;
  char w[700];  

#ifdef DEBUG
  printf("Initialization finished.\n");
  NewAbortMSG("GM:ATE in main procedure.\n");
#endif
#ifdef CHKREG
  if ((argc < 2) || (strcmp(argv[1],OKMSG)!=0)) RestorePal=True;
#endif
  atexit(cleanup);
  FontPtr=GetROMFont();
  if (!initmouse())
    { 
    errorbox("This application requires a Microsoft","compatible mouse! (Q)uit");
    return(quit);
    }
  // Set up colors as default.pal, and then the hardware palette if fails
  if (!loadspecany("default",".pal",WorkDir,sizeof(RGBdata)*256,(char *) colors))
    InitStartCols(colors);

  SetTextColors();
  while(1)
    {
    writestr(0,0,79,GMTOPBAR);
    writestr(0,1,31,"  Palette  Block  Monster  Map  Character  Image  Sound  Integrator  Main  Help ");
    attr=openmenu(1,3,21,11,w);
    writestr(1,3,PGMTITLECOL,"  PALETTE DESIGNER   ");
    writestr(1,4,PGMTITLECOL,"    Version "GMVER"     ");
    writestr(1,5,PGMTITLECOL,"  By Gregory Stone   ");
    writestr(1,6,PGMTITLECOL," Copyright (C) 1994  ");
    writestr(2,8,attr+14, "Choose a Palette");
    writestr(2,9,attr+14, "New Palette");
    writestr(2,10,attr+14,"Edit Chosen Palette");
    writestr(2,11,attr+14,"Save a Palette");
    writestr(2,12,attr+14,"Delete a Palette");
    writestr(2,13,attr+14,"Quit");
    ink = vertmenu(1,8,21,6,ink);
    closemenu(1,3,21,11,w);

    switch (ink)
      {
      case 1:
        if (!saved) 
          {
          ink = (errorbox("Unsaved changes--Really Load another file?","  (Y)es  (N)o")&0x00FF);
          if ( (ink!='Y') && (ink!='y') ) { ink=4; break; }
          }
        if (loadany("Enter Palette file to load:","*.pal\0",WorkDir,sizeof(RGBdata)*256,(char*) &colors,REMNAME))
                { saved=TRUE; }
        ink=3;
        break;
      case 2:
        if (!saved) 
          {
          ink = (errorbox("Unsaved changes--Really create a new palette?","  (Y)es  (N)o")&0x00FF);
          if ( (ink!='Y') && (ink!='y') ) { ink=4; break; }
          }
        havepal=FALSE;
        curfile[0]=0; // Erase old file name
        saved=TRUE;   // No file to save!
        errorbox("New palette created!","(C)ontinue",2);
        InitStartCols(colors);
        ink=3;
        break;
      case 3:
        edit();
        ink=4;
        break;
      case 4:
        ink=3;
        if (saveany("Enter name to save palette as:",".pal",WorkDir,sizeof(RGBdata)*256,(char *)&colors))
                { saved=TRUE; ink=6; }
        break;
      case 5:
        delany("Enter Palette file to delete:","*.pal\0",WorkDir);
        break;
      case 0: 
      case 6: 
        if (!saved) 
          {
          ink = (errorbox("Unsaved changes--Really Quit?","  (Y)es  (N)o")&0x00FF);
          if ( (ink!='Y') && (ink!='y') ) { ink=4; break; }
          }
        return(menu);
      }
    }
  }

static void cleanup(void)
  {
  Time.TurnOff();
  }
  
static void edit(void)
  {
  int change,ink,ascii,scan,oldcol;
  int x=0,y=0,butnum=0,oldbut=-1;
  int grabbed=-1;
  unsigned long int oldclk=1;
  int ButHit=0;
  int adder=-1;
  char morc=MOVE;

  oldcol=(curcol=1);
  GraphMode();
  SetCols(colors,FaceCols);
  SetAllPalTo(&colors[FaceCols[BLACK]]);
  drawscrn();
  FadeTo(colors);
  mouclearbut();
  setmoupos(180,170);
  moucurbox(CX-10,CY-10,CX1+10,SPLOTY1+20);
  moucur(1);
  do
    {
    change=FALSE;
    while (change==FALSE)
      {
      if (grabbed!=-1)
        {
        moucur(0);
        Line (x,CY1+1,x,CY2,BKGCOL);
        Line (x,CY2,grabbed+CX,CY2,BKGCOL);
        Point(x,CY1+1,FaceCols[GREY1]);
        Point(x,CY1+2,FaceCols[GREY1]);
        Point(x,CY1+3,FaceCols[GREY2]);
        Point(x,CY1+4,FaceCols[GREY3]);
        }
      moustats(&x,&y,&butnum);
      if (grabbed!=-1)
        {
        Line (x,CY1+1,x,CY2,grabbed);
        Line (grabbed+CX,CY1+1,grabbed+CX,CY2,grabbed);
        Line (x,CY2,grabbed+CX,CY2,grabbed);
        }
      if ((butnum&1)||(oldbut!=butnum)||(bioskey(1))) change=TRUE;
      }
    if (bioskey(1)) 
      {
      ink = bioskey(0);
      ascii = ink&255;
      scan=ink/256;
      switch (ascii)
        {
        case 0:
          switch(scan)
            {
            case 75:
              curcol--;
              if (curcol<0) curcol=MAXCOLS-1;
              drawcolchange();
              break;
            case 77:
              curcol++;
              if (curcol>=MAXCOLS) curcol=0;
              drawcolchange();
              break;
            }
          break;
        case '+':
        case '=':
          curcol++;
          if (curcol>=MAXCOLS) curcol=0;
          drawcolchange();
          break;
        case '-':
          curcol--;
          if (curcol<0) curcol=MAXCOLS-1;
          drawcolchange();
          break;
        case 'm':
        case 'M':  if (!(butnum&1)) morc=MOVE; break;
        case 'c':
        case 'C':  if (!(butnum&1)) morc=COPY; break;
        case 'q':
        case 'Q':
        case 27:
          Cur.Off();
          TextMode();
          return;
        }
      oldcol=curcol;
      }
    else
      {
      if (grabbed!=-1)  // Move or rotate the color.
        {
        Line (x,CY1+5,x,CY2,BKGCOL);
        Line (grabbed+CX,CY1+5,grabbed+CX,CY2,BKGCOL);
        Line (x,CY2,grabbed+CX,CY2,BKGCOL);
        switch (morc)
          {
          case MOVE:  RotPal(grabbed,x-CX,Sign(grabbed-x+CX),colors); break;
          case COPY:  colors[x-CX]=colors[grabbed]; break;
          }
        SetAllPal(colors);
        if (butnum<2)
          {
          if (!ResetCols(colors,FaceCols))
            {
            drawborder(CX,CY,MAXCOLS,11,5,2,FaceCols);
            drawborder(TBX,TBY,TBLEN,TBWID,2,2,FaceCols); // Erase Text box
            BoxFill(TBX,TBY,TBX+TBLEN-1,TBY+TBWID-1,FaceCols[GREY2]);
            drawcolchange();
            }
          moucur(1);
          oldbut=-1;
          }
        else { drawborder(CX,CY,MAXCOLS,11,5,2,FaceCols); }

        moucurbox(CX-10,CY-10,CX1+10,SPLOTY1+20);
        grabbed=-1;
        }
      }
    if ((y>=CY)&&(y<=CY1)&&(x>=CX)&&(x<CX1)&&(butnum>=2))    //MB2 pressed!
      {      
      if (grabbed==-1)        // Moving a color's position or copying color
        {
        oldbut=butnum;
        grabbed=x-CX;
        moucurbox(CX,CY,CX1-1,CY1);
        }
      if (!(butnum&1)) 
        switch(morc)
          {
          case MOVE:  BoxWithText(TBX,TBY,"      Move Mode    ",FaceCols); break;
          case COPY:  BoxWithText(TBX,TBY,"      Copy Mode    ",FaceCols); break;
          }
      }  
    if (butnum==1)
      {
      if ((x>MENUX)&&(x<(MENUX+36))&&(y>MENUY)&&(y<MENUY+11))  // Menu box hit
        { Cur.Off(); TextMode();  return; }
      if ((x>HELPX)&&(x<HELPX+36)&&(y>HELPY)&&(y<HELPY+11))    // Help box hit
        {
        dohelp();
        GraphMode();
        drawscrn();
        FadeTo(colors);
        mouclearbut();
        drawcolchange();
        }
      if ((y>=CY)&&(y<=CY1)&&(x>=CX)&&(x<CX1))                  // Color change
        {
        curcol= x-CX;
        if (oldcol!=curcol) { drawcolchange(); oldcol=curcol; }
        }
      if ((x<MINUSB+BUSIZE)&&(x>=MINUSB)&&(oldclk+adder<Clock))
        {
        if (adder==-1) adder=SLOWAMT;
        else adder =0;
        moucur(0);
        if ((y<COLBY+COLBWID)&&(y>COLBY)) 
          {
          minushalf(MINUSB,COLBY-1,HiCols);
          if ( --colors[curcol].red   ==255) colors[curcol].red=63;
          ButHit=1;
          saved=0;
          }
        if ((y<COLBY+2*COLBWID)&&(y>COLBY+COLBWID))
          {
          minushalf(MINUSB,COLBY+COLBWID-1,HiCols);
          if ( --colors[curcol].green ==255) colors[curcol].green=63;
          ButHit=2;
          saved=0;
          }
        if ((y<COLBY+3*COLBWID)&&(y>COLBY+2*COLBWID))
          {
          minushalf(MINUSB,COLBY+2*COLBWID-1,HiCols);
          if ( --colors[curcol].blue  ==255) colors[curcol].blue=63;
          ButHit=3;
          saved=0;
          }
        oldclk=Clock;
        moucur(1);
        drawcolnums();
        Palette(curcol,colors[curcol].red,colors[curcol].green,colors[curcol].blue);
        }
      if ((x<PLUSB+BUSIZE)&&(x>PLUSB)&&(oldclk+adder<Clock))
        {
        if (adder==-1) adder=SLOWAMT;
        else adder=0;
        moucur(0);
        if ((y<COLBY+COLBWID)&&(y>COLBY))
          {
          plushalf(PLUSB,COLBY-1,HiCols);
          if (++colors[curcol].red  >63) colors[curcol].red  =0;
          ButHit=4;
          saved=0;
          }
        if ((y<COLBY+2*COLBWID)&&(y>COLBY+COLBWID))
          {
          plushalf(PLUSB,COLBY+COLBWID-1,HiCols);
          if (++colors[curcol].green>63) colors[curcol].green=0;
          ButHit=5;
          saved=0;
          }
        if ((y<COLBY+3*COLBWID)&&(y>COLBY+2*COLBWID))
          {
          plushalf(PLUSB,COLBY+2*COLBWID-1,HiCols);
          if (++colors[curcol].blue >63) colors[curcol].blue =0;
          ButHit=6;
          saved=0;
          }
        moucur(1);  
        oldclk=Clock;
        drawcolnums();
        Palette(curcol,colors[curcol].red,colors[curcol].green,colors[curcol].blue);
        }
      if ((x<COLBX+64)&&(x>COLBX))
        {
        if ((y<COLBY+COLBWID)&&(y>COLBY))             getcolnum(&(colors[curcol].red));
        if ((y<COLBY+2*COLBWID)&&(y>COLBY+COLBWID))   getcolnum(&(colors[curcol].green));
        if ((y<COLBY+3*COLBWID)&&(y>COLBY+2*COLBWID)) getcolnum(&(colors[curcol].blue));
        }
      }
    else if (adder!=-1)
      {
      adder=-1;
      ResetCols(colors,FaceCols);
//      if (curcol==BKGCOL) {SetCols(colors,FaceCols); BoxFill(0,0,SIZEX-1,SIZEY-1,BKGCOL); }
//      drawscrn();
      SetAllPal(colors);
      }
    if (butnum==0)
      {
      if (ButHit>0)
        {
        moucur(FALSE);
        if (ButHit<4)
          minushalf(MINUSB,COLBY-2+((ButHit-1)*COLBWID),FaceCols);
        else if (ButHit<7)
          plushalf(PLUSB,COLBY-2+((ButHit-4)*COLBWID),FaceCols);
        moucur(TRUE);
        ButHit=0;
        }
      }
    } while (1);
  }

static void getcolnum(unsigned char *in)
  {
  char str[4]="";
  int temp;

  moucur(0);
  BoxWithText(TBX,TBY,"Enter new value:   ",FaceCols);
  GGet(TBX+137,TBY+1,FaceCols[BLACK],FaceCols[GREEN],str,2);
  sscanf(str,"%d",&temp);
  if ( (temp<64)&&(temp>-1) ) { *in=temp; saved=0; }
  Palette(curcol,colors[curcol].red,colors[curcol].green,colors[curcol].blue);
//  if (curcol==BKGCOL) { SetCols(colors,FaceCols); BoxFill(0,0,SIZEX-1,SIZEY-1,BKGCOL); }
//  drawscrn();
  moucur(1);
  if (!ResetCols(colors,FaceCols)) drawcolnums();
  SetAllPal(colors);
  }

static void drawcolchange(void)
  {
  drawsplot();
  drawcolnums();
  Palette(curcol,colors[curcol].red,colors[curcol].green,colors[curcol].blue);
  }        

static void drawcolnums()
  {         
  char str[5];
  
  moucur(0);
  sprintf (str,"%2d",colors[curcol].red);
  BoxFill(COLBX+49,COLBY+1,COLBX+64,COLBY+8,FaceCols[GREEN]);
  GWrite(COLBX+49,COLBY+1,FaceCols[BLACK],str);
  sprintf (str,"%2d",colors[curcol].green);
  BoxFill(COLBX+49,COLBY+1+COLBWID,COLBX+64,COLBY+8+COLBWID,FaceCols[GREEN]);
  GWrite(COLBX+49,COLBY+COLBWID+1,FaceCols[BLACK],str);
  sprintf (str,"%2d",colors[curcol].blue);
  BoxFill(COLBX+49,COLBY+1+2*COLBWID,COLBX+64,COLBY+8+2*COLBWID,FaceCols[GREEN]);
  GWrite(COLBX+49,COLBY+1+2*COLBWID,FaceCols[BLACK],str);
  moucur(1);
  }

static void drawscrn(void)
  {
  moucur(0);
//  SetCols(colors,FaceCols);
  drawborder(TBX,TBY,TBLEN,TBWID,2,2,FaceCols);            // Draw text box
  BoxFill(TBX,TBY,TBX+TBLEN-1,TBY+TBWID-1,FaceCols[GREY2]);
  drawborder(CX,CY,MAXCOLS,11,5,2,FaceCols);               //  Draw palette
  drawcolorchart();
  drawbuttons();
  drawsplot();
  moucur(1);
  drawcolnums();
  }

static void drawsplot(void)
  {
  char str[20];
  
  drawborder(SPLOTX,SPLOTY,SPLOTX1-SPLOTX,SPLOTY1-SPLOTY+12,2,2,FaceCols);
  BoxFill(SPLOTX,SPLOTY,SPLOTX1-1,SPLOTY1-1,curcol);
  BoxFill(SPLOTX-1,SPLOTY1,SPLOTX1-1,SPLOTY1+1,FaceCols[GREY1]);
  if (curcol!=TRANSCOL)   sprintf(str," Color #%3d ",curcol);
  else                    sprintf(str,"    Clear   ");
  BoxWithText(SPLOTX,SPLOTY1+3,str,FaceCols);
  }

static void drawbuttons(void)
  {
  int lp;
  
  drawborder(COLBX,COLBY,COLBLEN,3*COLBWID-2,2,2,FaceCols);
  BoxWithText(COLBX,COLBY,"Red     ",FaceCols);
  BoxWithText(COLBX,COLBY+COLBWID,"Green   ",FaceCols);
  BoxWithText(COLBX,COLBY+2*COLBWID,"Blue    ",FaceCols);
  Line(COLBX,COLBY-2+2*COLBWID,COLBX+COLBLEN,COLBY-2+2*COLBWID,FaceCols[GREY1]);
  Line(COLBX,COLBY-2+COLBWID,COLBX+COLBLEN,COLBY-2+COLBWID,FaceCols[GREY1]);
  BoxFill(COLBX+65,COLBY-1,MINUSB-1,COLBY+3*COLBWID-3,FaceCols[GREY1]);
  for (lp=0;lp<3;lp++)
    {
    plushalf(PLUSB,COLBY-2+(lp*COLBWID),FaceCols);
    minushalf(MINUSB,COLBY-2+(lp*COLBWID),FaceCols);
    Line(PLUSB-1,COLBY-1+(lp*COLBWID),PLUSB-1,COLBY-3+((lp+1)*COLBWID),FaceCols[GREY2]);
    Point(PLUSB-1,COLBY-3+((lp+1)*COLBWID),FaceCols[GREY4]);
    }
  drawsurface(HELPX-2,HELPY-1,40,26,FaceCols);
  menub(MENUX,MENUY,FaceCols);
  helpb(HELPX,HELPY,FaceCols);
  }
  
static void drawcolorchart(void)
  {
  int x;
  for (x=CX;x<CX1;x++) Line(x,CY,x,CY1,x-CX);
  }

static void RotPal(int start,int end,int dir,RGBdata *pal)
  {
  register int l;
  RGBdata temp;

  saved =0;
  if (end<start) swap(&start,&end);

  if(dir>0)
    {
    temp.red=pal[end].red;
    temp.green=pal[end].green;
    temp.blue=pal[end].blue;

    for(l=end;l>start;l--)
      insertcol(l,pal[l-1].red,pal[l-1].green,pal[l-1].blue,pal);
    pal[start]=temp;
    }
  if(dir<0)
    {
    temp.red=pal[start].red;
    temp.green=pal[start].green;
    temp.blue=pal[start].blue;

    for(l=start;l<end;l++)
      insertcol(l,pal[l+1].red,pal[l+1].green,pal[l+1].blue,pal);
    pal[end]=temp;
    //.red=temp.red;
    //pal[end].green=temp.green;
    //pal[end].blue=temp.blue;
    }
  }

static void dohelp(void)
  {
  moucur(0);
  TextMode();
  DisplayHelpFile(HELPFILE);
  moucur(1);
  }
  
void plushalf(int x,int y, unsigned char col[4])
  {
  char plusar[]= {"    UUU@UUUTUWUTUWUUUõUUWUUUWUVUUUTUUUhªªª€"};
   draw4dat(x,y,plusar,15,10,col);
  }

void minushalf(int x,int y, unsigned char col[4])
  {
  char minusar[]= {"    UUUUUUUUUUUUUU_ýUUUUU•UUUUUU)UUUªªª"};
   draw4dat(x,y,minusar,15,10,col);
  }

static boolean ResetCols(RGBdata *colors,uchar *FaceCols)
  {
  uchar OldCols[MAXFACECOL];
  int   l;

  for (l=0;l<MAXFACECOL;l++) OldCols[l]=FaceCols[l];
  SetCols(colors, FaceCols);
  if (OldCols[BKGCOL]!=FaceCols[BKGCOL])
    {
    BoxFill(0,0,SIZEX-1,SIZEY-1,BKGCOL);
    drawscrn();
    return(TRUE);
    }
  else
    {
    for (l=1;l<MAXFACECOL;l++) if (OldCols[l]!=FaceCols[l])
      { drawscrn(); return(TRUE); }
    }
  return(FALSE);
  }
