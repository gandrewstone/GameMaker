/*----------------------------------------------------*/
/* Character Editor      by: Oliver Stone             */
/* Last edited   11-17-91                             */
/*----------------------------------------------------*/

#include <stdio.h>
#include <alloc.h>
#include <dos.h>
#include <bios.h>
#include <process.h>
#include <stdlib.h>
#include <conio.h>
#include "gen.h"
#include "gmgen.h"
#include "graph.h"
#include "scrnrout.h"
#include "mousefn.h"
#include "windio.h"
#include "palette.h"
#include "char.h"
#include "bloc.h"
#include "mon.h"
#include "sound.h"
#include "jstick.h"
#include "tranmous.hpp"
#include "facelift.h"

#define GL         6                             // L-Left, T-Top,G-Grid
#define GT         (BLY1+39)
#define GRIDLEN    4                             // # of blocks on edge of grid
#define GRIDWID    5                             // # of blocks on side of grid
#define GXC        (GL+(GRIDLEN*BLEN)/2)         // mouse position to center box
#define GYC        (GT+((GRIDWID-3)*BLEN)/2)     //           in grid.
#define BX         (GL+(GRIDLEN*BLEN-32)/2)      // x pos. of rotate buttons
#define SL         (SIZEX+GL+(GRIDLEN*BLEN))/2   // Animated Area initial coords
#define ST         ((SIZEY+BLY1)/2-BLEN)
#define BLLIST     14                            // BL-Area to show blocks
#define BLX        20                            // LIST-number on screen
#define BLY        36                            // X-Left, Y-Top,X1-Bottom,etc
#define BLX1       (BLX+(BLLIST*BLEN)-1)
#define BLY1       (BLY+BLEN)
#define TX         5
#define T1         5
#define T2         (T1+12)
#define MX         180                           // M- Menu Button
#define MY         (T1+5)
#define HX         (MX+40)                       // H- Help Button
#define SX         (MX+80)                       // S- Shoot Button

#define INVX(j,k)  (75+j*(BLEN+5))               // Screen Pos for inventory
#define INVY(j,k)  (79+k*(BLEN+5))               //      items.
#define INVX1(j,k) (74+BLEN+j*(BLEN+5))
#define INVY1(j,k) (78+BLEN+k*(BLEN+5))
#define HELPFILE   "charedit.hlp"

#define SELX     123                             // Birthmon defines
#define SELY     91
#define MOLEN    13 
#define MOX      32
#define MOY2     40
#define TBOX(x)  (159-(x*4))              // X and Y position of text box
#define TBOXY    145                      // where x is the number of 
#define MX3      SELX+40                  // letters in the text (centers text)
#define MY3      SELY-3
#define MX4      223
#define MY4      95

int blocnum(int mun);                    // Used by bloc list scrolling rout.
int monnum(int rebmun);                  // Used by monster list scrolling rout.
static void cleanup(void);
static void initalldata(void);
static int enterseq(sequence *cur,int f);   // Create animation sequence
static int anichar(sequence *cur,int *curfram); // Advance frame # at proper time
static int animons(void);                   // Advance frames for monster if ap.
static void edit(void);                          // Main Menu
static void stage_two(int cn,int ch,char allow); // Secondary Menu
static void setjstk(int cn,int ch);         // put Joystick status in sequence
static void inventinit(int cn);             // Define starting inventory
static void getthekey(int cn,int ch);       // put key in sequence
static void getsound(int cn,int ch);        // put sound in sequence
static void drawscrn(void);
static void grid(void);                     // Drawing routines
static void drawblocbox(char col);
static void drawinv(int cn);
static void DrawFnButs(int x,int y,char orient);
static void birthmon(frames *cur,int f);         /*  XX  */
static void blocattrib(chrstruct *pptr,int f);             /* connects an item with sequence */
static void drawchar(int x,int y,frames now,char col2);
static void drawbl(int x,int y,int blo,char dir);          /* Drawing routines */
static void shootb(int x,int y, unsigned char col[4]);
static void rotate(int *x,int *y,char *spot,sequence now,int dir);

static void InitMeters (int start,int end);

extern unsigned _stklen=8000U;  /* Set the stack to 8000 bytes, not 4096 */

chrstruct            p[MAXPLAY];
extern blkstruct far *blk;
sndstruct            s[LASTSND];
ConfigStruct         cs;
char                 DigiSnd[LASTSND][9];
RGBdata              colors[256];
monstruct            m[LASTMON];
int                  curblk,curfram;
static char          saved=1;
unsigned long int    oldtime=0;
extern char          curfile[31];
extern unsigned char FaceCols[MAXFACECOL];      //Screen colors
extern char          *FontPtr;
extern unsigned char HiCols[4];


static void InitMeters (int start,int end)
  {
  int l;
  for (l=start;l<end;l++)
    if (p[0].meter[l]!=1)
      p[0].meter[l]=1;
  }


QuitCodes main(int argc,char *argv[])
  {
  register int  j;
  int           attr=0;
  char          w[1000]="NotInit";    //Used to initialize SB and for menus
  int           choice=1;

#ifdef DEBUG
  printf("Initialization finished.\n");
  NewAbortMSG("GM:ATE in main procedure.\n");
#endif
#ifdef CHKREG
  if ((argc < 2) || (strcmp(argv[1],OKMSG)!=0)) RestorePal=True;
#endif
  atexit(cleanup);
  setcbrk(0);              //------Turn off Control-Break Checking------
  FontPtr=GetROMFont();

 // -------------------------Set up palette-------------------------------------
 // Set up colors as default.pal, or Andy's palette if default.pal doesn't exist

  if (!loadspecany("default",".pal",WorkDir,sizeof(RGBdata)*256,(char *) colors))
    InitStartCols(colors);

 // ---------------------Set up Block structure
  blk=(blkstruct far *) farmalloc(sizeof(blkstruct)*(CHARBL+2));
  if (blk==NULL) { errorbox("NOT ENOUGH MEMORY!","  (Q)uit"); return(menu);}
  for (j=0; j<sizeof(blkstruct)*(CHARBL+2);j++)
    *( ((unsigned char far *) blk)+j)=0;

  /*---------------------Set up Mouse  */
  if (initmouse()==FALSE)
    { errorbox("This application requires a Microsoft","compatible mouse! (Q)uit");
      return(quit); }

  LoadConfigData(&cs);
  for (j=0;j<7;j++) if (cs.SndDrvr[j]!=w[j]) j=20;     // if NotInit set j = any number > 7;
  if (j<8)
    errorbox("Run PLAYGAME to initialize","sound card information.",5);
  else if (cs.ForceSnd==SndBlaster)
    {
    int temp;
    if ( (temp=InitSbVocDriver(&cs))!=TRUE)
      {
      if (temp==0) errorbox("Sound card voice driver cannot be loaded!",".VOC file sounds cannot be played.",5);
      if (temp==-1) errorbox("Sound Blaster interrupt or port is set up incorrectly",".VOC file sounds cannot be played.",5);
      }
    }
  initalldata();

  do
    {
    SetTextColors();
    writestr(0,0,79,GMTOPBAR);
    writestr(0,1,31,"  Palette  Block  Monster  Map  Character  Image  Sound  Integrator  Main  Help ");
    attr = openmenu(30,3,24,14,w);
    writestr(30,3,PGMTITLECOL,"    CHARACTER  MAKER    ");
    writestr(30,4,PGMTITLECOL,"      Version "GMVER"      ");
    writestr(30,5,PGMTITLECOL,"    By  Oliver Stone    ");
    writestr(30,6,PGMTITLECOL,"   Copyright (C) 1994   ");
    writestr(30,8,attr+14, " Choose a Character Set");
    writestr(30,9,attr+14, " New Character Set");
    writestr(30,10,attr+14," Choose a Block Set");
    writestr(30,11,attr+14," Choose a Palette");
    writestr(30,12,attr+14," Choose a Sound Set");
    writestr(30,13,attr+14," Edit Characters");
    writestr(30,14,attr+14," Save Character Set");
    writestr(30,15,attr+14," Delete Character Set");
    writestr(30,16,attr+14," Quit");
    mouclearbut();
    choice=vertmenu(30,8,24,9,choice);
    closemenu(30,3,24,14,w);
    switch (choice) {
      case 1:  if (!saved) {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice=6; break; }
         }
        loadany("Enter name of character set to load: ","*.chr\0",WorkDir,sizeof(chrstruct)*MAXPLAY,(char *)&p,1);
        InitMeters(20,30);
        choice=2; saved=1; break;
      case 2:  if (!saved) {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice=6; break; }
         }
        initalldata(); choice=2; saved=1; curfile[0]=0; 
      	errorbox("New character created!","(C)ontinue",2);
        break;
      case 3: loadblks(3,(char *) blk); break;
      case 4: loadany("Enter the palette to load:","*.pal\0",WorkDir,sizeof(RGBdata)*256,(char *)&colors,0); break;
      case 5: LoadSnd("Enter name of sound set to load: ","*.snd\0",WorkDir,(unsigned char *) &s[0],&DigiSnd[0][0],0); break;
      case 6: edit(); break;
      case 7:
        choice=6; 
        if (saveany("Enter name of character set to save:",".chr",WorkDir,sizeof(chrstruct)*MAXPLAY,(char *)&p))
          { choice=8; saved=1; }
        break;
      case 8: delany("Delete which character set: ","*.chr\0",WorkDir); choice = 7; break;
      case 0: choice =9;
      case 9: if (!saved) {
                choice = (errorbox("Unsaved changes--Really Quit?","  (Y)es  (N)o")&0x00FF);
                if ( (choice!='Y') && (choice!='y') ) choice=6;
                else choice=9;
               }
              break;
     }
    choice++;
    } while (choice != 10);
   return(menu);
  }

static void edit(void)
 {
  register int j;
  char str[60];
  char tcol=2,colu=1;
  int choice=1,charnum=0;
  int temp;
  char keystr[MAXSEQ][12];

  moucur(0);
  TextMode();

  while (1)
    {
    drawbox (0,0,77,22,1,2,8);
    clrbox(1,1,76,21,1);
    writestr(26,2,14,"Character Sequence Menu");

    for (j=3;j<MAXSEQ;j++) {
      if ( (temp = p[charnum].sq[j].key&0x00FF) != 0 ) {
         if (temp==13) sprintf(keystr[j],"'Enter'    ");
         else if (temp==9) sprintf(keystr[j],"'Tab'      ");
         else if (temp<=26) sprintf(keystr[j],"Control-%c  ",temp+64);
         else sprintf(keystr[j],"'%c'        ",temp);
       }
      else switch(p[charnum].sq[j].key) {
        case 0:      sprintf(keystr[j],"Undefined  "); break;
        case 18432:  sprintf(keystr[j],"Up Arrow   "); break;
        case 19200:  sprintf(keystr[j],"Left Arrow "); break;
        case 19712:  sprintf(keystr[j],"Right Arrow"); break;
        case 20480:  sprintf(keystr[j],"Down Arrow "); break;
        case 18688:  sprintf(keystr[j],"Up-Right   "); break;
        case 18176:  sprintf(keystr[j],"Up-Left    "); break;
        case 20224:  sprintf(keystr[j],"Down-Left  "); break;
        case 20736:  sprintf(keystr[j],"Down-Right "); break;

        default: sprintf(keystr[j],"%5d      ",p[charnum].sq[j].key);
       }
     }
    writestr(4,5,4,"Return to previous menu");
    sprintf(str,"Hit points at start: %3d",p[charnum].meter[0]);
    writestr(4,6,4,str);
    sprintf(str,"Number of lives: %3d",p[charnum].meter[1]+1);
    writestr(4,7,4,str);
    writestr(4,9,4,"   ----Switch Columns---->");
    writestr(4,10,tcol,"Idle");
    writestr(4,11,tcol,"Die");
    writestr(4,12,tcol,"Injured");
    sprintf(str,"Pick up an object:    %s",keystr[3]);
    writestr(4,13,tcol,str);
    sprintf(str,"Drop an object:       %s",keystr[4]);
    writestr(4,14,tcol,str);
    for (j=5;j<MAXSEQ/2;j++) {
      sprintf(str,"Key for sequence %2d:  %s",j-4,keystr[j]);
      writestr(4,j+10,tcol,str);
     }
    writestr(4,MAXSEQ/2+10,4,"   ----Switch Columns---->");

/*    sprintf(str,"Character #%d",charnum+1);
    writestr(42,5,4,str);
*/
    writestr(42,5,4,"Define inventory at start");
    if (p[charnum].meter[3] == INF_REPEAT) 
      sprintf(str,"Set money at start:  N/A ");
    else sprintf(str,"Set money at start: %5d",p[charnum].meter[3]-1);
    writestr(42,6,4,str);
    writestr(42,9,4,"   <----Switch Columns----");
    for (j=MAXSEQ/2;j<MAXSEQ;j++) {
      sprintf(str,"Key for sequence %2d:  %s",j-4,keystr[j]);
      writestr(42,j-MAXSEQ/2+10,tcol,str);
     }
    writestr(42,MAXSEQ-MAXSEQ/2+10,4,"   <----Switch Columns----");

    if (colu==1) {
      moucur(1);
      mouclearbut();
      choice=vertmenu(4,5,33,MAXSEQ-MAXSEQ/2+6,choice);
      moucur(0);
      switch (choice) {
        case 0:
        case 1:   clrbox(0,0,77,22,1);  return;
        case 2:   str[0]=0; qwindow(8,13,3,"Enter hit points at start: ",str);
                  sscanf(str,"%d",&temp);
                  if ( (temp>0)&&(temp<256) )
                    {
                    p[charnum].meter[0] = temp;
                    saved=0;
                    }
                  break;  
        case 3:   str[0]=0; qwindow(8,13,3,"Enter number of lives: ",str);
                  sscanf(str,"%d",&temp);
                  if ( (temp>0)&&(temp<257) ) {
                    p[charnum].meter[1] = temp-1;
                    saved=0;
                   }
                  break;  
        case 4:   break;
        case 5: case MAXSEQ-MAXSEQ/2+6:   colu=2;  break;
        case 6: case 7:
                  stage_two(charnum,choice-6,0); break;
        case 8:
                  stage_two(charnum,choice-6,2); break;
        case 9: case 10:
                  stage_two(charnum,choice-6,3); break;
        default:
          if (choice<MAXSEQ/2+6) stage_two(charnum,choice-6,7);
          break;
       }
     }
    else {
      moucur(1);
      choice=vertmenu(42,5,33,MAXSEQ-MAXSEQ/2+6,choice);
      moucur(0);
      switch (choice) {
        case 0:                           clrbox(0,0,77,22,1);  return;
/*        case 1:                           charnum^=1; break; */
        case 1:                           inventinit(charnum); break;
        case 2:   str[1]=0; str[0]=' ';
                  if (qwindow(8,13,5,"Enter initial money (blank for a moneyless game): ",str))
                    {
                    temp=INF_REPEAT;
                    sscanf(str,"%d",&temp);
                    if ( (temp>=0)&&(temp<INF_REPEAT-1) )
                      p[charnum].meter[3] = temp+1;
                    else p[charnum].meter[3]=INF_REPEAT;
                    saved=0;
                    }
                  break;  
        case 3: case 4:           break;
        case 5: case MAXSEQ-MAXSEQ/2+6:   colu=1;  break;
        default:
          stage_two(charnum,choice+MAXSEQ/2-6,7);
          break;
       }
     }
   }
 }

static void inventinit(int cn)
 {
  unsigned register int  j,k,m;
  blkstruct far          *blk2;
  int                    curblk2=0,curinv=0;
  int                    mx,my,buts;

  blk2=blk;
  blk=(blkstruct far *) farmalloc(sizeof(blkstruct)*BACKBL);
  if (blk==NULL) { errorbox("NOT ENOUGH MEMORY!","  (M)enu"); blk=blk2; return;}
  for (j=0; j<sizeof(blkstruct)*BACKBL;j++)
    *( ((unsigned char far *) blk)+j)=0;
  if (!loadblks(1,(char*)blk)) { errorbox("Could not load block set!","  (M)enu");
                      farfree(blk); blk=blk2; return; }
  GraphMode();
  SetCols(colors,FaceCols);
  moucur(0);
  SetAllPalTo(&colors[BKGCOL]);
  BoxFill(0,0,SIZEX,SIZEY,BKGCOL);
  drawsurface(INVX(0,0)-6,INVY(0,0)-6,6+(5*(BLEN+5)),6+(2*(BLEN+5)),FaceCols);
  for (k=0;k<2;k++)
    for (j=0;j<5;j++)
      {
      BoxFill(INVX(j,k),INVY(j,k),INVX1(j,k),INVY1(j,k),FaceCols[GREY2]);
      Line(INVX(j,k)-1,INVY(j,k)-1,INVX1(j,k),INVY(j,k)-1,FaceCols[GREY3]);
      Line(INVX(j,k)-1,INVY(j,k)-1,INVX(j,k)-1,INVY1(j,k),FaceCols[GREY4]);
      }
  drawlistframe(MOX,MOY2,MOLEN,FaceCols);
  drawlist(MOX,MOY2,MOLEN,BACKBL,&curblk2,blocnum);
  drawsurface(MX4-2,MY4-1,40,14,FaceCols);
  doneb(MX4,MY4,FaceCols);
  drawborder(TBOX(36),TBOXY,289,9,5,2,FaceCols);
  BoxWithText(TBOX(36),TBOXY,"Select Objects for Initial Inventory",FaceCols);
  drawborder(20,174,281,9,5,2,FaceCols);
  BoxFill(20,174,300,182,FaceCols[GREY2]);
  while( (p[cn].inv[curinv]!=BACKBL)&&(curinv<9) ) curinv++;
  drawinv(cn);
  moucur(1);
  FadeTo(colors);
  mouclearbut();

  while(1)
    {
    moustats(&mx,&my,&buts);
    if (buts>0)
      {
      BoxFill(20,174,300,182,FaceCols[GREY2]);             //Turn off error msg
      if ( (mx>MX4)&&(mx<(MX4+36))&&(my>MY4)&&(my<(MY4+10)) )
        {
        for (k=4;k<20;k++)
          {
          if (p[cn].sq[k].bloc==BACKBL)
            p[cn].sq[k].on=1;
          else
            {
            p[cn].sq[k].on=0;
            for (j=0;j<10;j++)
              if (p[cn].inv[j]==p[cn].sq[k].bloc) p[cn].sq[k].on=1;
            }
          }
        farfree(blk); blk=blk2; moucur(0);
        TextMode(); return;
        }
      if ( (my>=MOY2)&&(my<MOY2+BLEN) )
        {
        if ( (mx>=MOX)&&(mx<MOX+MOLEN*BLEN) )
          {
          mouclearbut();
          if (curinv<10)
            {
            if (blk[(curblk2+(mx-MOX)/BLEN)%BACKBL].obj==1)
              {
              p[cn].inv[curinv]=(curblk2+(mx-MOX)/BLEN)%BACKBL;
              drawinv(cn);
              curinv++;
              saved=0;
              }
            else
              BoxWithText(20,174,"Only Object Blocks can be Selected!",FaceCols);
            }
          }
        moucur(0);
        if ( (mx>MOX-11)&&(mx<MOX-3) )
          { DrawArrows(MOX,MOY2,MOLEN,LEFT,FaceCols); scroll_list(2,MOX,MOY2,MOLEN,BACKBL,&curblk2,blocnum); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        if ( (mx<MOX+BLEN*MOLEN+10)&&(mx>MOX+BLEN*MOLEN+2) )
          { DrawArrows(MOX,MOY2,MOLEN,RIGHT,FaceCols); scroll_list(-2,MOX,MOY2,MOLEN,BACKBL,&curblk2,blocnum); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        moucur(1);
        }
      for(k=0;k<2;k++)
        for(j=0;j<5;j++)
          if ( (mx>INVX(j,k))&&(mx<INVX1(j,k))&&(my>INVY(j,k))&&(my<INVY1(j,k)) )
            {
            for (m=(k*5)+j;m<9;m++)
              p[cn].inv[m]=p[cn].inv[m+1];
            p[cn].inv[9]=BACKBL;
            if (curinv>(k*5+j)) { curinv--; saved=0; }
            moucur(0);  drawinv(cn);  moucur(1);
            k=2; j=5;
            mouclearbut();
            }
      }
    if (bioskey(1))
      {
      if ( (bioskey(0)&0x00FF)==27 )
        {
        for (k=4;k<20;k++)
          {
          if (p[cn].sq[k].bloc==BACKBL)
            p[cn].sq[k].on=1;
          else
            {
            p[cn].sq[k].on=0;
            for (j=0;j<10;j++)
              if (p[cn].inv[j]==p[cn].sq[k].bloc) p[cn].sq[k].on=1;
            }
          }
        farfree(blk); blk=blk2; moucur(0);
        TextMode(); return;
        }
      }
    }
  }

static void stage_two(int cn,int ch,char allow)
  {
  register int j;
  int choice=1;
  int attr=0;
  char w[900],str[30]="";
  int temp;

  do {
    attr = openmenu(10,8,26,12,w);
    if (p[cn].sq[ch].sound==LASTSND) sprintf(str," Add Sound (None)");
    else sprintf(str," Add Sound (%d)   ",p[cn].sq[ch].sound+1);
    writestr(10,8,attr+14,str);
    writestr(10,9,attr+2+(allow&1)*12," Enter Key");
    writestr(10,10,attr+2+(allow&1)*12," Enter Joystick");
    writestr(10,11,attr+2+(allow&1)*12,"   Configuration");
    writestr(10,12,attr+14," Define Sequence");
    writestr(10,13,attr+2+(allow&4)*3," Enter number of");
    if (ch<3) sprintf(str,"   executions (No Limit)");
    else switch (p[cn].meter[ch])
      {
      case INF_REPEAT: sprintf(str,"   executions (No Limit)"); break;
      case 0: sprintf(str,"   executions (None)    "); break;
      default: sprintf(str,"   executions (%d)",p[cn].meter[ch]); break;
      }
    writestr(10,14,attr+2+(allow&4)*3,str);
    if (p[cn].sq[ch].bloc!=BACKBL) sprintf(str," Tie to Block (%d)",p[cn].sq[ch].bloc);
    else sprintf(str," Tie to Block (None)");
    writestr(10,15,attr+2+(allow&4)*3,str);
/*    if (p[cn].sq[ch].ground&1) sprintf(str," Execute on a solid block");
    else sprintf(str," Execute anywhere");
    writestr(10,16,attr+2+(allow&1)*12,str);
*/
    if (p[cn].sq[ch].ground&2) sprintf(str," Always complete sequence");
    else sprintf(str," Can interrupt sequence");
    writestr(10,16,attr+2+(allow&1)*12,str);
    if (p[cn].sq[ch].norepeat) sprintf(str," Do once per keystroke");
    else sprintf(str," Auto repeat");
    writestr(10,17,attr+2+(allow&1)*12,str);
    if (p[cn].sq[ch].Momentum) sprintf(str," Momentum   ");
    else sprintf(str," No Momentum");
    writestr(10,18,attr+14,str);
    writestr(10,19,attr+15," Menu");
    choice = vertmenu(10,8,26,12,choice);
    closemenu(10,8,26,12,w);
    switch(choice) {
      case 0: choice=12; break;
      case 1:
        getsound(cn,ch);
        break;
      case 2:
        if ( !(allow&1) ) break;
        getthekey(cn,ch);
        break;
      case 3: case 4:
        if ( !(allow&1) ) break;
        setjstk(cn,ch);
        choice=4;
        break;
      case 5:
        while (enterseq(p[cn].sq,ch));
        break;
      case 6: case 7:
        if ( !(allow&4) ) break;
        while (bioskey(1)) bioskey(0);
        str[1]=0; str[0]=' ';
        if (qwindow(8,13,5,"Enter number of executions (<Enter> for Unlimited):",str)>0)
          {
          temp=INF_REPEAT;
          sscanf(str,"%d",&temp);
          if ( (temp>=0)&&(temp<INF_REPEAT) )
            {
            p[cn].meter[ch] = temp;
            saved=False;
            } else  p[cn].meter[ch]=INF_REPEAT;
          }
        choice=7; saved=0; break;
      case 8: if ( !(allow&4) ) break;
        blocattrib(&p[cn],ch);
        break;
/*      case 8: if ( !(allow&1) ) break;
        p[cn].sq[ch].ground ^= 1; saved = 0; break;
*/
      case 9: if ( !(allow&2) ) break;
        p[cn].sq[ch].ground ^= 2; saved = 0; break;
      case 10:
        p[cn].sq[ch].norepeat ^=1; saved=FALSE;
        break;
      case 11:
        p[cn].sq[ch].Momentum ^=1; saved=FALSE;
        break;
     }
    choice++;
   } while (choice!=13);
 }

static void getsound(int cn,int ch)
 {
  register int j;
  int temp=0;
  char str[5];

  str[0]=0;

  if ((DigiSnd[p[cn].sq[ch].sound][0]==0)||(DigiSnd[p[cn].sq[ch].sound][0]==' '))
    {
    if (p[cn].sq[ch].sound != LASTSND)
      {
      for (j=0;j<NUMFREQ;j++) 
         {
         if (s[p[cn].sq[ch].sound].freq[j]) 
           {
           sound(s[p[cn].sq[ch].sound].freq[j]);
           }
         else nosound();
         delay(DELAY);
         }
      }
      nosound();
    } 
  else
    {
    PlaySbVocFile((char far *) DigiSnd[p[cn].sq[ch].sound]);
    }

  if (p[cn].sq[ch].sound!=LASTSND)
    {
    sprintf(str,"%d",p[cn].sq[ch].sound+1);
    }
  else
    {
    str[0]=0;
    }

  qwindow(8,13,2,"Enter sound number (Blank for None): ",str);
  temp=LASTSND+1;
  sscanf(str,"%d",&temp); 

  if ( (temp>0)&&(temp<=LASTSND+1) )
    {
    p[cn].sq[ch].sound= temp-1;
    saved=0;
    }

  if ((DigiSnd[p[cn].sq[ch].sound][0]==0)||(DigiSnd[p[cn].sq[ch].sound][0]==' '))
    {
    if (p[cn].sq[ch].sound != LASTSND) 
      {
      for (j=0;j<NUMFREQ;j++)
         {
         if (s[p[cn].sq[ch].sound].freq[j])
           {
           sound(s[p[cn].sq[ch].sound].freq[j]);
           }
         else nosound();
         delay(DELAY);
         }
      }
    nosound();
    }
  else PlaySbVocFile((char far *) DigiSnd[p[cn].sq[ch].sound]);
  }

static void getthekey(int cn,int ch)
  {
  char w1[200];
  int attr=0,temp=0;

  while (bioskey(1)) bioskey(0);


  attr=openmenu(11,13,12,1,w1);
  writestr(11,13,attr+14,"Type Key Now");
  temp=bioskey(0);
  if ( (temp>>8 >= 59)&&(temp>>8 <= 68) )
    {
    writestr(11,13,attr+14,"  Reserved  ");
    temp=0;
    PauseTimeKey(2);
    p[cn].sq[ch].key=0;
    saved=0;
    }
  if ( (temp&0x00FF) != 27 )
    {
    p[cn].sq[ch].key=temp;
    saved=0;
    }
  closemenu(11,13,12,1,w1);
  }

static void setjstk(int cn,int ch)
 {
  char w[1000],w2[1000];
  int temp=0,attr=0,attr2=0;
 
  while (bioskey(1)) bioskey(0);
  attr=openmenu(11,13,33,3,w);
  attr2=openmenu(50,8,16,12,w2);
  writestr(11,13,attr+14,"You may select either a joystick ");
  writestr(11,14,attr+14,"position, a joystick button, or a");
  writestr(11,15,attr+14,"combination of the two.          ");
  writestr(50,8 ,attr2+15, "Pick a direction");
  writestr(50,10,attr2+14,"Up");
  writestr(50,11,attr2+14,"Up-right");
  writestr(50,12,attr2+14,"Right");
  writestr(50,13,attr2+14,"Down-right");
  writestr(50,14,attr2+14,"Down");
  writestr(50,15,attr2+14,"Down-left");
  writestr(50,16,attr2+14,"Left");
  writestr(50,17,attr2+14,"Up-Left");
  writestr(50,18,attr2+14,"Any direction");
  writestr(50,19,attr2+14,"No Joystick");
  temp = vertmenu(50,10,16,10,1);
  closemenu(50,8,16,12,w2);
  if (temp==10) { p[cn].sq[ch].jstk_mov=0; temp=0; /* force return */ }
  if (temp==0) { closemenu(11,13,33,3,w); return; }
  p[cn].sq[ch].jstk_mov=temp;
  saved=0; 
  attr2=openmenu(50,11,14,7,w2);
  writestr(50,11,attr2+14,"Pick a Button");
  writestr(50,13,attr2+15,"Button 1");
  writestr(50,14,attr2+15,"Button 2");
  writestr(50,15,attr2+15,"Both Buttons");
  writestr(50,16,attr2+15,"No Buttons");
  writestr(50,17,attr2+15,"Doesn't matter");
  temp=vertmenu(50,13,14,5,1);
  closemenu(50,11,14,7,w2);
  closemenu(11,13,33,3,w);
  if (temp==5) temp =0;
  p[cn].sq[ch].jstk_but=temp;
  saved=0;
 }

static int enterseq(sequence *cur,int f)
  {
  register int j;
  int    mx=0,my=0,oldx,oldy,buts;     // mouse coordinates
  int    x2=SL,y2=ST;                  // animated coordinates
  char   spot=0;      // Which frame out of ten is currently being defined
  frames grab;      // The current frame structure (cur[f].fr[spot]==grab)
  int    inkey;
  int    curfram,lastfram;     // Current and most recent animated frames
  char   str[20];
  char   flag=0;

  drawscrn();
  grab=cur[f].fr[0]; spot=0;
  if (grab.pic2==CHARBL+1)     //Initialize New Sequence
    {
    Box(GXC-BLEN/2,GYC+BLEN/2,GXC-1+BLEN/2,GYC-1+5*BLEN/2,FaceCols[RED2]);
    BoxWithText(TX,T1,"Frame:  1",FaceCols);
    }
  else                        //Initialize a Selected Sequence
    {
    mx=GL+BLEN; my=GT;   // set up initial cond's so rotate doesn't bomb
    getblk(mx-BLEN/2,my+BLEN/2,&blk[CHARBL].p[0][0]);
    getblk(mx-BLEN/2,my+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
    rotate(&mx,&my,&spot,cur[f],0);  //Rotate backwards to last defined frame
    grab=cur[f].fr[spot];            //Spot is now last frame number
    }
  setmoupos((mx=MX+18),(my=MY+5));
  getblk(mx-BLEN/2,my+BLEN/2,&blk[CHARBL].p[0][0]);
  getblk(mx-BLEN/2,my+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
  oldx=mx; oldy=my;
  if (spot!=MAXFRAME)
    {
    sprintf(str,"Pause Length: %3d",grab.pause);
    BoxWithText(TX,T2,str,FaceCols);
    DrawFnButs(BX,GT-30,grab.orient);
    drawchar(mx-BLEN/2,my+BLEN/2,grab,FaceCols[RED2]);
    }
  FadeTo(colors);
  moucur(1);
  mouclearbut();
  while (bioskey(1)) bioskey(0);
  while (1)
    {
    moustats(&mx,&my,&buts);
    if (cur[f].fr[0].pic2!=CHARBL+1)
      if (anichar(&cur[f],&curfram))
        {
        if ((flag&2)==2)  //If animated image is drawn, erase it
          {
          if ( (mx>x2-BLEN*2)&&(mx<x2+BLEN*2)&&(my>y2-BLEN*3)&&(my<y2+BLEN*2) )
            {
            moucur(0);
            flag|=1;  // set bit 1 if overlap of animated and current frames
            if ( (grab.pic2!=MONBL)&&(spot!=MAXFRAME) )  //(current frame erased)
              {
              drawblk(oldx-BLEN/2,oldy+BLEN/2,&blk[CHARBL].p[0][0]);
              drawblk(oldx-BLEN/2,oldy+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
              }
            }
          flag&=253;   // erase bit 2 (Animated frame erased)
          BoxFill(x2,y2,x2+BLEN-1,y2+BLEN*2-1,BKGCOL);
          if (cur[f].fr[lastfram].pic2==MONBL) y2-=cur[f].fr[lastfram].y;
          }
        if (curfram==0) { x2=SL; y2=ST; }
        if ( (curfram==spot)&&((mx-BLEN/2)>=GL)&&((my+BLEN/2)>=GT)
           &&((mx+BLEN/2)<=GL+GRIDLEN*BLEN)&&((my+BLEN*5/2)<=GT+GRIDWID*BLEN)
           &&(cur[f].fr[spot].pic2!=MONBL) )
          { x2+=mx-GXC;  y2+=my-GYC; }
        else
          {
          x2 += cur[f].fr[curfram].x;
          y2 += cur[f].fr[curfram].y;
          }
        lastfram=curfram;
        if ( (x2>=(GL+GRIDLEN*BLEN+15))&&(y2>=(BLY+BLEN+10))&&(x2+BLEN<=SIZEX-5)
           &&((y2+BLEN*2)<=(SIZEY-5)) )
          {
          if ( ((flag&1)==0)&&(mx>x2-BLEN*2)&&(mx<x2+BLEN*2)&&(my>y2-BLEN*3)
             &&(my<y2+BLEN*2) )
            {
            moucur(0);
            flag|=1;   // set bit 1 (current frame is to be erased)
            if ( (grab.pic2!=MONBL)&&(spot!=MAXFRAME) )
              {
              drawblk(oldx-BLEN/2,oldy+BLEN/2,&blk[CHARBL].p[0][0]);
              drawblk(oldx-BLEN/2,oldy+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
              }
            }
          flag|=2;   // set bit 2  (Animated frame drawn)
          if (cur[f].fr[curfram].pic2==MONBL)
            BoxFill(x2,y2,x2+BLEN-1,y2+BLEN-1,((curfram==spot) ? FaceCols[RED2] : FaceCols[RED1]));
          else
            drawchar(x2,y2,cur[f].fr[curfram],((curfram==spot) ? FaceCols[RED2] : FaceCols[RED1]));
          }
        if (flag&1)            //Redraw current frame over animation
          {
          if ( (grab.pic2!=MONBL)&&(spot!=MAXFRAME) )
            {
            getblk(oldx-BLEN/2,oldy+BLEN/2,&blk[CHARBL].p[0][0]);
            getblk(oldx-BLEN/2,oldy+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
            drawchar(oldx-BLEN/2,oldy+BLEN/2,grab,FaceCols[RED2]);
            }
          moucur(1);
          flag&=254;   // erase bit 1 (Current frame redrawn)
          }
        }
    if ( (mx!=oldx)|(my!=oldy) )
      {
      moucur(0);
      if ( (grab.pic2!=MONBL)&&(spot!=MAXFRAME) )
        {
        drawblk(oldx-BLEN/2,oldy+BLEN/2,&blk[CHARBL].p[0][0]);
        drawblk(oldx-BLEN/2,oldy+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
        getblk(mx-BLEN/2,my+BLEN/2,&blk[CHARBL].p[0][0]);
        getblk(mx-BLEN/2,my+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
        drawchar(mx-BLEN/2,my+BLEN/2,grab,FaceCols[RED2]); 
        }
      moucur(1);
      oldx=mx; oldy=my;
      }
    if (bioskey(1))
      {
      inkey=bioskey(0);
      if ( (inkey&0x00FF)!=0 )
        {
        switch (inkey&0x00FF) {
          case 'S':
            if ( (cur[f].fr[spot].pic2!=MONBL)&&(spot!=MAXFRAME) )
              {
              grab.orient^=4;
              cur[f].fr[spot].orient=grab.orient;
              saved=0;
              } break;
          case 's':
            if ( (cur[f].fr[spot].pic2!=MONBL)&&(spot!=MAXFRAME) )
              {
              grab.orient^=64;
              cur[f].fr[spot].orient=grab.orient;
              saved=0;
              } break;
          case 'R':
            if ( (cur[f].fr[spot].pic2!=MONBL)&&(spot!=MAXFRAME) )
              {
              if (grab.orient%4 == 3) grab.orient&=0xFC;
              else grab.orient++;
              cur[f].fr[spot].orient=grab.orient;
              saved=0;
              } break;
          case 'r':
            if ( (cur[f].fr[spot].pic2!=MONBL)&&(spot!=MAXFRAME) )
              {
              if ((grab.orient>>4)%4 == 3) grab.orient&=0xCF;
              else grab.orient+=16;
              cur[f].fr[spot].orient=grab.orient;
              saved=0;
              } break;
          case '+': case '=':
            if ( (cur[f].fr[spot].pic2!=MONBL)&&(spot!=MAXFRAME) )
              {
              if (grab.pause<245) grab.pause+=10;
              cur[f].fr[spot].pause=grab.pause;
              saved=0;
              } break;
          case '-': case '_':
            if ( (cur[f].fr[spot].pic2!=MONBL)&&(spot!=MAXFRAME) )
              {
              if (grab.pause>10) grab.pause-=10;
              cur[f].fr[spot].pause=grab.pause;
              saved=0;
              } break;
          case  8:
            if (spot==MAXFRAME) break; // can only remove the frame being edited
            for (j=spot;j<MAXFRAME-1;j++)  // slide all frame down one
              cur[f].fr[j]=cur[f].fr[j+1];
            grab.orient = grab.x = grab.y = 0;
            grab.pic2=CHARBL+1;
            grab.pic1=CHARBL;
            grab.pause=1;
            cur[f].fr[MAXFRAME-1]=grab;    // re-initialize last frame
            moucur(0);
            rotate(&mx,&my,&spot,cur[f],0);
            moucur(1);
            grab=cur[f].fr[spot];
            oldx=mx; oldy=my;
            saved=0;
            break;
          case 27: TextMode(); return(0);
          case ' ':
          case 13:
            buts=1;
            break;
          }
        moucur(0);
        }
      else
        {
        inkey >>= 8;
        moucur(0);
        switch (inkey) {
          case 75:                              // left arrow
            rotate(&mx,&my,&spot,cur[f],0);
            grab=cur[f].fr[spot];
            oldx=mx; oldy=my;
            break;
          case 77:                              // right arrow
            rotate(&mx,&my,&spot,cur[f],1);
            grab=cur[f].fr[spot];
            oldx=mx; oldy=my;
            break;
          case 72:                              // up arrow
            if ( (cur[f].fr[spot].pic2!=MONBL)&&(spot!=MAXFRAME) )
              {
              if (grab.pause<255) grab.pause++;
              cur[f].fr[spot].pause=grab.pause;
              saved=0;
              }
            break;
          case 80:                            // down arrow
            if ( (cur[f].fr[spot].pic2!=MONBL)&&(spot!=MAXFRAME) )
              {
              if (grab.pause>1) grab.pause--;
              cur[f].fr[spot].pause=grab.pause;
              saved=0;
              }
            break;
          }
        }
      if (grab.pic2==MONBL)
        {
        sprintf(str,"Shoot Monster: %2d",grab.pic1+1);
        BoxWithText(TX,T2,str,FaceCols);
        DrawFnButs(BX,GT-30,0);
        }
      else if (spot!=MAXFRAME)
        {
        sprintf(str,"Pause Length: %3d",grab.pause);
        BoxWithText(TX,T2,str,FaceCols);
        drawchar(mx-BLEN/2,my+BLEN/2,grab,FaceCols[RED2]);
        DrawFnButs(BX,GT-30,grab.orient);
        }
      moucur(1);
      while(bioskey(1)) bioskey(0);
      }
    if (buts>0)
      {
      moucur(0);
      if ( (grab.pic2!=MONBL)&&(spot!=MAXFRAME) )
        {
        drawblk(mx-BLEN/2,my+BLEN/2,&blk[CHARBL].p[0][0]);
        drawblk(mx-BLEN/2,my+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
        if ( (mx-BLEN/2>=GL)&&(mx+BLEN/2 <= GL+GRIDLEN*BLEN)&&(my+BLEN/2>=GT) )
          {
          if ( (grab.pic2==(CHARBL+1))&&(grab.pic1 != CHARBL) )
            {  // For single blk chars, move block to lower square if its in the higher
            grab.pic2=grab.pic1;
            grab.pic1=CHARBL;
            grab.orient<<=4;
            oldy= (my-=BLEN); oldx=mx;
            setmoupos(mx,my);
            getblk(mx-BLEN/2,my+BLEN/2,&blk[CHARBL].p[0][0]);
            getblk(mx-BLEN/2,my+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
            }
          if ( (my+BLEN/2>=GT)&&(my+BLEN*5/2 <= GT+GRIDWID*BLEN)
             &&(grab.pic2!=(CHARBL+1)) )
            {
            cur[f].fr[spot] = grab;
            saved=0;
            cur[f].fr[spot].x=mx-GXC; cur[f].fr[spot].y=my-GYC;
            if (spot<MAXFRAME) rotate(&mx,&my,&spot,cur[f],1);
            oldx=mx; oldy=my;
            if (spot!=MAXFRAME)
              {
              if (cur[f].fr[spot].pic2!=(CHARBL+1))  grab=cur[f].fr[spot];
              else                                   grab=cur[f].fr[spot-1];
              }
            }
          }
        else if ( (my>GT-30)&&(my<GT-20) )
          {
          if ( (mx>BX)&&(mx<BX+10) )              // toggle rotate top bloc
            { grab.orient^=0x01; if (grab.orient&0x04) grab.orient^=0x02; } //set rotate bit.  if either flip or rotate, switch them
          if ( (mx>BX+10)&&(mx<BX+20) )           // toggle h. flip top bloc
            grab.orient^=0x04;
          if ( (mx>BX+20)&&(mx<BX+30) )           // toggle v. flip top bloc
            grab.orient^=0x06;                    // Hey, they work. don't question it.
          DrawFnButs(BX,GT-30,grab.orient);
          getblk(mx-BLEN/2,my+BLEN/2,&blk[CHARBL].p[0][0]);
          getblk(mx-BLEN/2,my+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
          }
        else if ( (my>GT-20)&&(my<GT-10) )
          {
          if ( (mx>BX)&&(mx<BX+10) )              // toggle rotate top bloc
            { grab.orient^=0x10; if (grab.orient&0x40) grab.orient^=0x20; } //set rotate bit.  if either flip or rotate, switch them
          if ( (mx>BX+10)&&(mx<BX+20) )           // toggle h. flip top bloc
            grab.orient^=0x40;
          if ( (mx>BX+20)&&(mx<BX+30) )           // toggle v. flip top bloc
            grab.orient^=0x60;                    // Hey, they work. don't question it.
          DrawFnButs(BX,GT-30,grab.orient);
          getblk(mx-BLEN/2,my+BLEN/2,&blk[CHARBL].p[0][0]);
          getblk(mx-BLEN/2,my+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
          }
        else if ( (my>BLY)&&(my<BLY1) )
          {
          if ( (mx<BLX1)&&(mx>BLX) )
            {
            if (buts==1) grab.pic1 = (curblk+(mx-BLX)/BLEN)%CHARBL;
            if (buts==2) grab.pic2 = (curblk+(mx-BLX)/BLEN)%CHARBL;
            cur[f].fr[spot].pic1=grab.pic1;
            cur[f].fr[spot].pic2=grab.pic2;
            saved=0;
            }
          }
        else if (my>BLY1)
          {
          if ((buts&1) !=0) cur[f].fr[spot].pic1= (grab.pic1=CHARBL);
          if ((buts&2) !=0) cur[f].fr[spot].pic2= (grab.pic2=CHARBL+1);
          }
        }
      if ( (my>MY)&&(my<MY+10) )
        {
        if ( (mx>MX)&&(mx<MX+36) ) { TextMode(); return(0); }
        if ( (mx>SX)&&(mx<SX+36)&&(spot!=MAXFRAME) )
          {
          birthmon(cur[f].fr,spot);
          BoxFill(0,0,SIZEX-1,SIZEY-1,BKGCOL);
          drawscrn();
          setmoupos((mx=SX+18),(my=MY+5));
          if (cur[f].fr[spot].pic2==MONBL) setmoupos((mx=TX+BLEN),my=T1);
          getblk(mx-BLEN/2,my+BLEN/2,&blk[CHARBL].p[0][0]);
          getblk(mx-BLEN/2,my+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
          if (cur[f].fr[spot].pic2==CHARBL+1)
            rotate(&mx,&my,&spot,cur[f],0);  // rotate, then back if no monst.
          rotate(&mx,&my,&spot,cur[f],1);
          grab=cur[f].fr[spot];
          oldx=mx; oldy=my;
          FadeTo(colors);
          saved = 0;
          }
        if ( (mx>HX)&&(mx<HX+36) )
          {
          TextMode(); mouclearbut();
          DisplayHelpFile(HELPFILE);
          while ( bioskey(1) ) bioskey(0);  mouclearbut();
          drawscrn();
          rotate(&mx,&my,&spot,cur[f],1);  // to draw all frames,
          rotate(&mx,&my,&spot,cur[f],0);  // rotate, then back
          setmoupos(HX+18,MY+5);
          moustats(&mx,&my,&buts);
          oldx=mx; oldy=my;
          getblk(mx-BLEN/2,my+BLEN/2,&blk[CHARBL].p[0][0]);
          getblk(mx-BLEN/2,my+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
          FadeTo(colors);
          }
        }
      if (grab.pic2==MONBL)
        {
        sprintf(str,"Shoot Monster: %2d",grab.pic1+1);
        BoxWithText(TX,T2,str,FaceCols);
        DrawFnButs(BX,GT-30,0);
        }
      else if (spot!=MAXFRAME)
        {
        sprintf(str,"Pause Length: %3d",grab.pause);
        BoxWithText(TX,T2,str,FaceCols);
        DrawFnButs(BX,GT-30,grab.orient);
        drawchar(mx-BLEN/2,my+BLEN/2,grab,FaceCols[RED2]);
        }
      if ((my>BLY)&&(my<BLY1))
        {
        if ( (mx>BLX-11)&&(mx<BLX-3) )
          { DrawArrows(BLX,BLY,BLLIST,LEFT,FaceCols); scroll_list(2,BLX,BLY,BLLIST,CHARBL,&curblk,blocnum); DrawArrows(BLX,BLY,BLLIST,NONE,FaceCols); }
        if ( (mx<BLX1+10)&&(mx>BLX1+2) )
          { DrawArrows(BLX,BLY,BLLIST,RIGHT,FaceCols); scroll_list(-2,BLX,BLY,BLLIST,CHARBL,&curblk,blocnum); DrawArrows(BLX,BLY,BLLIST,NONE,FaceCols); }
        }
      moucur(1);
      mouclearbut();
      }
    }
  }

static void blocattrib(chrstruct *pptr,int f)
 {
  unsigned register int  j;
  blkstruct far          *blk2;
  int                    mx,my,buts,inkey;
  int                    current=0;

  blk2=blk;
  blk= (blkstruct far *) farmalloc(sizeof(blkstruct)*(BACKBL+1));
  if (blk==NULL) { errorbox("NOT ENOUGH MEMORY!","  (M)enu"); blk=blk2; return;}
  for (j=0; j<sizeof(blkstruct)*(BACKBL+1);j++)
    *( ((unsigned char far *) blk)+j)=0;
  if (!loadblks(1,(char*)blk)) { errorbox("Could not load block set!","  (M)enu");
                      farfree(blk); blk=blk2; return; }


  GraphMode();
  SetCols(colors,FaceCols);
  moucur(0);
  SetAllPalTo(&colors[BKGCOL]);
  BoxFill(0,0,SIZEX-1,SIZEY-1,BKGCOL);
  drawlistframe(MOX,MOY2,MOLEN,FaceCols);
  drawborder(SELX,SELY,BLEN,BLEN,2,2,FaceCols);
  drawsurface(MX3-2,MY3-1,40,27,FaceCols);
  noneb(MX3,MY3,FaceCols);
  doneb(MX3,MY3+13,FaceCols);
  moucurbox(1,1,SIZEX-1,SIZEY-1);
  if (pptr->sq[f].bloc==BACKBL)
    { BoxFill(SELX,SELY,SELX+BLEN-1,SELY+BLEN-1,FaceCols[GREY2]); current=0; }
  else
    { drawblk(SELX,SELY,&blk[pptr->sq[f].bloc].p[0][0]); current=pptr->sq[f].bloc; }
  drawlist(MOX,MOY2,MOLEN,BACKBL,&current,blocnum);
  drawborder(TBOX(34),TBOXY,273,9,5,2,FaceCols);
  BoxWithText(TBOX(34),TBOXY,"Select Object to Activate Sequence",FaceCols);
  drawborder(20,174,281,9,5,2,FaceCols);
  BoxFill(20,174,300,182,FaceCols[GREY2]);
  FadeTo(colors);
  moucur(1);
  while (1)
    {
    moustats(&mx,&my,&buts);
    if (buts)
      {
      if ( (mx>MX3)&&(mx<MX3+36) )
        {
        if ( (my>MY3+12)&&(my<MY3+22) )
          {
          if (pptr->sq[f].bloc==BACKBL)
            pptr->sq[f].on=1;
          else
            {
            pptr->sq[f].on=0;
            for (j=0;j<10;j++)
              if (pptr->inv[j]==pptr->sq[f].bloc) pptr->sq[f].on=1;
            }
          farfree(blk); blk=blk2; TextMode(); return;
          }
        if ( (my>MY3)&&(my<MY3+10) )
          {
          pptr->sq[f].bloc=BACKBL;
          BoxFill(SELX,SELY,SELX+BLEN-1,SELY+BLEN-1,FaceCols[GREY2]);
          BoxFill(20,174,300,182,FaceCols[GREY2]);
          saved=0;
          }
        }
      if ( (my>=MOY2)&&(my<MOY2+BLEN) )
        {
        moucur(0);
        if ( (mx>=MOX)&&(mx<MOX+MOLEN*BLEN) )
          {
          if ( blk[(current+(mx-MOX)/BLEN)%BACKBL].obj==1 )
            {
            pptr->sq[f].bloc = (current+(mx-MOX)/BLEN)%BACKBL;
            drawblk(SELX,SELY,&blk[pptr->sq[f].bloc].p[0][0]);
            BoxFill(20,174,300,182,FaceCols[GREY2]);
            saved=0;
            }
          else
            BoxWithText(20,174,"Only Object Blocks can be Selected!",FaceCols);
          }
        if ( (mx>MOX-11)&&(mx<MOX-3) )
          { DrawArrows(MOX,MOY2,MOLEN,LEFT,FaceCols); scroll_list(4,MOX,MOY2,MOLEN,BACKBL,&current,blocnum); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        if ( (mx<MOX+BLEN*MOLEN+10)&&(mx>MOX+BLEN*MOLEN+2) )
          { DrawArrows(MOX,MOY2,MOLEN,RIGHT,FaceCols); scroll_list(-4,MOX,MOY2,MOLEN,BACKBL,&current,blocnum); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        moucur(1);
        }   
      }
    }
  }

static void birthmon(frames *cur,int f)         //Have sequence create a monster
  {
  unsigned register int  j;
  blkstruct far          *blk2;
  int                    mx,my,buts,inkey;
  int                    curmon=0;

  TextMode();                                   //Load monster information
  if (!loadany("Enter name of monster set to load: ","*.mon\0",WorkDir,sizeof(monstruct)*LASTMON,(char *)&m,0)) { GraphMode(); return; }
  blk2=blk;
  blk = (blkstruct far *) farmalloc(sizeof(blkstruct)*(MONBL+1));
  if (blk==NULL)
    { errorbox("NOT ENOUGH MEMORY!","  (M)enu"); blk=blk2; GraphMode(); return;}
  for (j=0; j<sizeof(blkstruct)*(MONBL+1);j++)   // Initalize new block set
    *( ((unsigned char far *) blk)+j)=0;
  if (!loadblks(2,(char*)blk)) { errorbox("Could not load block set!","  (M)enu");
                      farfree(blk); blk=blk2; GraphMode(); return; }

  GraphMode();
  SetAllPalTo(&colors[BKGCOL]);
  BoxFill(0,0,SIZEX,SIZEY,BKGCOL);
  drawlistframe(MOX,MOY2,MOLEN,FaceCols);
  drawborder(SELX,SELY,BLEN,BLEN,2,2,FaceCols);
  drawsurface(MX3-2,MY4-1,40,14,FaceCols);
  doneb(MX3,MY4,FaceCols);
  moucurbox(1,1,SIZEX-1,SIZEY-1);
  drawlist(MOX,MOY2,MOLEN,LASTMON,&curmon,monnum);
  BoxFill(SELX,SELY,SELX+BLEN-1,SELY+BLEN-1,FaceCols[GREY2]);
  drawborder(TBOX(21),TBOXY,169,9,5,2,FaceCols);
  BoxWithText(TBOX(21),TBOXY,"Pick Monster to Shoot",FaceCols);
  FadeTo(colors);
  while (bioskey(1)) bioskey(0);
  moucur(1);

  cur[f].pic2=MONBL;
  cur[f].pic1=MONBL;
  cur[f].pause=1;
  while (j)                 //Until choice is made
    {
    moustats(&mx,&my,&buts); 
    if (animons())   // Animate all of the monsters
      {
      moucur(0);
      drawlist(MOX,MOY2,MOLEN,LASTMON,&curmon,monnum);
      if (cur[f].pic1!=MONBL)
        drawblk(SELX,SELY,&blk[m[cur[f].pic1].pic[m[cur[f].pic1].cur.pic][0]].p[0][0]);
      moucur(1);
      }
      if ( ( (buts)&&(mx>MX3)&&(mx<MX3+36)&&(my>MY4)&&(my<MY4+10) )
           ||(bioskey(1)) )
        {
        if (bioskey(1)) if ((bioskey(0)&0x00FF)==27) cur[f].pic1=MONBL;
        if (cur[f].pic1!=MONBL) j=0;            //Flag to continue
        else
          {
          mouclearbut();
          moucur(0);
          cur[f].pic2=CHARBL+1;
          cur[f].pic1=CHARBL;
          farfree(blk);
          blk=blk2;
          return;
          }
        }
    if (buts)
      {
      if ( (my>=MOY2)&&(my<MOY2+BLEN) )
        {
        moucur(0);
        if ( (mx>=MOX)&&(mx<MOX+MOLEN*BLEN) )
          {
          cur[f].pic1 = (curmon+(mx-MOX)/BLEN)%LASTMON; // Set Pic1 to the monster #
          drawblk(SELX,SELY,&blk[m[cur[f].pic1].pic[m[cur[f].pic1].cur.pic][0]].p[0][0]);
          saved=FALSE;
          }
        if ( (mx>MOX-11)&&(mx<MOX-3) )
          { DrawArrows(MOX,MOY2,MOLEN,LEFT,FaceCols); scroll_list(2,MOX,MOY2,MOLEN,LASTMON,&curmon,monnum); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        if ( (mx<MOX+BLEN*MOLEN+10)&&(mx>MOX+BLEN*MOLEN+2) )
          { DrawArrows(MOX,MOY2,MOLEN,RIGHT,FaceCols); scroll_list(-2,MOX,MOY2,MOLEN,LASTMON,&curmon,monnum); DrawArrows(MOX,MOY2,MOLEN,NONE,FaceCols); }
        moucur(1);
        }
      }
    }
  moucur(0); mouclearbut();
  while(bioskey(1)) bioskey(0);
  BoxFill(0,0,SIZEX,SIZEY,BKGCOL);
  drawsurface(MX3-2,MY4-1,40,14,FaceCols);
  doneb(MX3,MY4,FaceCols);
  drawborder(SELX,SELY-BLEN/2,BLEN,(BLEN*2),2,2,FaceCols);
  BoxFill(SELX,SELY-BLEN/2,SELX+BLEN-1,SELY+(BLEN*3)/2-1,FaceCols[GREY2]);
  drawsurface(34,15,260,20,FaceCols);
  GWrite(36, 17,FaceCols[RED1]," Choose the position from which");
  GWrite(36,25,FaceCols[RED1],"      the monster is shot.     ");
  
  cur[f].x=0; cur[f].y=0;
  drawbl(SELX,SELY-BLEN/2+cur[f].y,m[cur[f].pic1].pic[m[cur[f].pic1].cur.pic][0],cur[f].orient);
  moucur(1);
  while(1) 
    {
    moustats(&mx,&my,&buts);
    if (animons())
      {
      if ( (mx>SELX-11)&&(mx<SELX+BLEN)&&(my>SELY-BLEN/2-14)&&(my<SELY+3*BLEN/2) ) moucur(0);
      drawbl(SELX,SELY-BLEN/2+cur[f].y,m[cur[f].pic1].pic[m[cur[f].pic1].cur.pic][0],cur[f].orient);
      if ( (mx>SELX-11)&&(mx<SELX+BLEN)&&(my>SELY-BLEN/2-14)&&(my<SELY+3*BLEN/2) ) moucur(1);
      }
    if (bioskey(1))
      {
      moucur(0);
      inkey=bioskey(0);
      if ((inkey&0x00FF) == 0) 
        {
        inkey >>= 8;
        switch(inkey) 
          {
          case 72:                 //Toggle position of starting monster
            BoxFill(SELX,SELY-BLEN/2+cur[f].y,SELX-1+cur[f].x+BLEN,SELY+BLEN/2-1+cur[f].y,FaceCols[GREY2]);
            cur[f].y^=(BLEN);
            drawbl(SELX,SELY-BLEN/2+cur[f].y,m[cur[f].pic1].pic[m[cur[f].pic1].cur.pic][0],cur[f].orient);
            break;
          case 80:  
            BoxFill(SELX,SELY-BLEN/2+cur[f].y,SELX-1+cur[f].x+BLEN,SELY+BLEN/2-1+cur[f].y,FaceCols[GREY2]);
            cur[f].y^=BLEN;
            drawbl(SELX,SELY-BLEN/2+cur[f].y,m[cur[f].pic1].pic[m[cur[f].pic1].cur.pic][0],cur[f].orient);
            break;
          }
        }
      else switch(inkey&0x00FF) 
        {
        case 27:  cur[f].pic1=CHARBL; cur[f].pic2=CHARBL+1;
        case 13:                         //Free memory and return w/ choice
          moucur(0); farfree(blk); blk=blk2; return;
        case ' ':
          BoxFill(SELX,SELY-BLEN/2+cur[f].y,SELX-1+cur[f].x+BLEN,SELY+BLEN/2-1+cur[f].y,FaceCols[GREY2]);
          cur[f].y^=BLEN;                  //Toggle choice
          drawbl(SELX,SELY-BLEN/2+cur[f].y,m[cur[f].pic1].pic[m[cur[f].pic1].cur.pic][0],cur[f].orient);
          break;
        }
      moucur(1);
      }
    if (buts)     //==1)            // Save monster start location and quit
      { 
      if ( (mx>MX3)&&(mx<MX3+36)&&(my>MY4)&&(my<MY4+10) )
        {
        moucur(0); farfree(blk); blk=blk2; return;
        }
      if ( (mx>=SELX)&&(mx<SELX+BLEN) )
        {
        moucur(0);
        BoxFill(SELX,SELY-BLEN/2+cur[f].y,SELX-1+cur[f].x+BLEN,SELY+BLEN/2-1+cur[f].y,FaceCols[GREY2]);
        if ( (my>=SELY-BLEN/2)&&(my<SELY+BLEN/2) )
          cur[f].y=0;
        if ( (my>=SELY+BLEN/2)&&(my<SELY+(3*BLEN/2)) )
          cur[f].y=BLEN;
        drawbl(SELX,SELY-BLEN/2+cur[f].y,m[cur[f].pic1].pic[m[cur[f].pic1].cur.pic][0],cur[f].orient);
        mouclearbut(); moucur(1);
        }
      }
    }
  }  

static int anichar(sequence *cur,int *curfram)
 {
  if ( (Clock-oldtime) > cur->fr[*curfram].pause ) {
    if ( cur->fr[(++(*curfram))].pic2 == CHARBL+1 ) *curfram = 0;
    else if ( *curfram>=MAXFRAME ) *curfram=0;
    oldtime=Clock;
    return(1);
   }
  else return(0);
 }

static void rotate(int *x,int *y,char *spot,sequence now,int dir)
  {
  register int j;
  char str[20];
  int dx,dy;

  if ( (now.fr[*spot].pic2!=MONBL)&&(*spot != MAXFRAME) )
    {
    drawblk(*x-BLEN/2,*y+BLEN/2,&blk[CHARBL].p[0][0]);
    drawblk(*x-BLEN/2,*y+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
    }
  if (dir)
    {
    if ( (now.fr[*spot].pic2==(CHARBL+1)) ) *spot=0; else (*spot)++;
    if (*spot>MAXFRAME) *spot=0;
    }
  else
    {
    (*spot)--;
    if ((*spot) < 0)
      {
      do
        {
        (*spot)++;
        } while( ((*spot)<MAXFRAME) && (now.fr[*spot].pic2!=(CHARBL+1)) );
      }
    }
  if (*spot!=MAXFRAME) {
    if (now.fr[*spot].pic2==(CHARBL+1)) {
      now.fr[*spot].x=0; now.fr[*spot].y=0;
     }
   }
  dx=GXC; dy=GYC;
  for (j=0;j<(*spot);j++) {
    if (now.fr[j].pic2!=MONBL) {dx-=now.fr[j].x; dy-=now.fr[j].y;}
   }
  BoxFill(GL,GT,GL+BLEN*GRIDLEN-1,GT+BLEN*GRIDWID-1,BKGCOL);
  grid();
  if (  ( (dx-BLEN/2)>=GL )  &&  ( (dy+BLEN/2)>=GT )  &&
        ( (dx+BLEN/2) < (GL+BLEN*GRIDLEN) )  &&
        ( (dy+5*BLEN/2) < (GT+BLEN*GRIDWID) )  )  {
    Box(dx-BLEN/2,dy+BLEN/2,dx+BLEN/2-1,dy+(BLEN*5/2)-1,FaceCols[RED2]);
   }
  for (j=0;j<*spot;j++)
    {
    if (now.fr[j].pic2!=MONBL)
      {
      dx+=now.fr[j].x;
      dy+=now.fr[j].y;
      if (  ( (dx-BLEN/2)>=GL )  &&  ( (dy+BLEN/2)>=GT )  &&
            ( (dx+BLEN/2) < (GL+BLEN*GRIDLEN) )  &&
            ( (dy+5*BLEN/2) < (GT+BLEN*GRIDWID) )  )
        drawchar(dx-BLEN/2,dy+BLEN/2,now.fr[j],FaceCols[RED1]);
      }
    }
  if (*spot!=MAXFRAME) 
    {
    (*x)=GXC+now.fr[j].x;
    (*y)=GYC+now.fr[j].y;

    setmoupos(*x,*y);
    if (now.fr[*spot].pic2!=MONBL)
      {
      getblk((*x)-BLEN/2,(*y)+BLEN/2,&blk[CHARBL].p[0][0]);
      getblk((*x)-BLEN/2,(*y)+BLEN*3/2,&blk[CHARBL+1].p[0][0]);
      sprintf(str,"Pause Length: %3d",now.fr[*spot].pause);
      BoxWithText(TX,T2,str,FaceCols);
      DrawFnButs(BX,GT-30,now.fr[*spot].orient);
      }
    else
      {
      sprintf(str,"Shoot Monster: %2d",now.fr[*spot].pic1+1);
      BoxWithText(TX,T2,str,FaceCols);
      DrawFnButs(BX,GT-30,0);
      setmoupos((*x=TX+BLEN),*y=T1);
      }
    sprintf( str,"Frame: %2d",((*spot)+1) );
    BoxWithText(TX,T1,str,FaceCols);
    }
  else 
    {
    setmoupos((*x=TX+BLEN),*y=T1);
    BoxWithText(TX,T1,"Frame: --",FaceCols);
    BoxWithText(TX,T2,"Pause Length: ---",FaceCols);
    DrawFnButs(BX,GT-30,0);
    }
  }

static void drawbl(int x,int y,int blo,char dir)
 {
  register int j,k;

  for (j=0;j<BLEN;j++)
    for (k=0;k<BLEN;k++)
      switch(dir) {
        case 0: Point(x+j,y+k,blk[blo].p[k][j]);               break;   //straight
        case 1: Point(x+j,y+k,blk[blo].p[BLEN-j-1][k]);        break;   //rot. right.
        case 2: Point(x+j,y+k,blk[blo].p[BLEN-k-1][BLEN-j-1]); break;   //Flip and swap (2 rot.)
        case 3: Point(x+j,y+k,blk[blo].p[j][BLEN-k-1]);        break;   //rot left
        case 4: Point(x+j,y+k,blk[blo].p[k][BLEN-j-1]);        break;   //Flip
        case 5: Point(x+j,y+k,blk[blo].p[j][k]);               break;   //and rotate once
        case 6: Point(x+j,y+k,blk[blo].p[BLEN-k-1][j]);        break;   //2
        case 7: Point(x+j,y+k,blk[blo].p[BLEN-j-1][BLEN-k-1]); break;   //3
       }
 }

static void cleanup(void)
  {
  farfree(blk);
  Time.TurnOff();
  ShutSbVocDriver();
  }

static void initalldata(void)
  {
  unsigned register int lx,ly,lz;

  for (lx=0; lx<sizeof(chrstruct)*MAXPLAY;lx++) *( ((unsigned char *)p)+lx)=0;
  for (lx=0;lx<MAXPLAY;lx++) 
    {
    for (ly=0;ly<MAXSEQ;ly++) 
      {
      for (lz=0;lz<MAXFRAME;lz++) 
        {
        p[lx].sq[ly].fr[lz].pic1=CHARBL;
        p[lx].sq[ly].fr[lz].pic2=CHARBL+1;
        p[lx].sq[ly].fr[lz].pause=1;
        }
      p[lx].sq[ly].jstk_mov = -1;
      p[lx].sq[ly].jstk_but = -1;
      p[lx].sq[ly].sound    = LASTSND;
      p[lx].sq[ly].bloc     = BACKBL;
      p[lx].sq[ly].on       = 1;
      }
    for (ly=0;ly<MAXINV;ly++) p[lx].inv[ly]=BACKBL;
    for (ly=2;ly<MAXSEQ;ly++) p[lx].meter[ly]=INF_REPEAT;
    for (ly=MAXSEQ;ly<MAXSEQ+10;ly++) p[lx].meter[ly]=1;
    p[lx].meter[0]       = 1;
    p[lx].sq[0].jstk_mov = 0;   // idle should be no button, jstick center.
    p[lx].sq[0].jstk_but = 0;
    p[lx].sq[1].ground   = 2;   // fixed: cannot interrupt die sequence.
    p[lx].sq[2].ground   = 2;   // default: cannot interrupt injure sequence.
   }
  curblk=0;
  curfram=0;
 }

static void grid()
 {
  register int j,k;

//  BoxFill(GL,GT,GL+GRIDLEN*BLEN-1,GT+GRIDWID*BLEN-1,FaceCols[GREY2]);
  for (j=0; j<GRIDLEN;j++)
    for (k=0; k<GRIDWID;k++) {
      Point(GL+j*BLEN,GT+k*BLEN,FaceCols[GREY1]);
      Point(GL+(j+1)*BLEN - 1,GT+k*BLEN,FaceCols[GREY1]);
      Point(GL+(j+1)*BLEN - 1,GT+(k+1)*BLEN - 1,FaceCols[GREY1]);
      Point(GL+j*BLEN,GT+(k+1)*BLEN - 1,FaceCols[GREY1]);
     }
 }

static void drawinv(int cn)
  {
  register int j,k;

  for (k=0;k<2;k++)
    for (j=0;j<5;j++)
      if ( p[cn].inv[(5*k)+j] != BACKBL )
        drawblk(INVX(j,k),INVY(j,k),&blk[p[cn].inv[(5*k)+j]].p[0][0]);
      else BoxFill(INVX(j,k),INVY(j,k),INVX1(j,k),INVY1(j,k),FaceCols[GREY2]);
  }

static void drawchar(int x,int y,frames now,char col2)
  {
  BoxFill(x,y,x+BLEN-1,y+BLEN*2-1,BKGCOL);
  if (now.pic1 != CHARBL) drawbl(x,y,now.pic1,now.orient&0x0F);
  if (now.pic2 != (CHARBL+1)) drawbl(x,y+BLEN,now.pic2,now.orient>>4);
  Box(x,y,x+BLEN-1,y+(BLEN*2)-1,col2);
  }

static void DrawFnButs(int x,int y,char orient)
  {
  char funar[3][26]= {"   UPeAVî" "\x1A" "™AñîeAïT5U¿ˇ",
                      "   UPeA™îeAYT™AYT5U¿ˇ",
                      "   UPUAeîjA©îYAUî5U¿ˇ"};
  unsigned char col[2][4] = { { FaceCols[GREY3],FaceCols[RED1],
                       FaceCols[BLACK],FaceCols[RED2] },
                     { FaceCols[GREY3],FaceCols[RED3],
                       FaceCols[GREEN],FaceCols[GREY3] } };
  char dir[6];
  int j;

  for (j=0;j<6;j++)
    {
    dir[j]=orient&0x01;
    if (j==2) orient>>=2; else orient>>=1;
    }
    
  draw4dat(x,y+dir[0],funar[0],9,9,col[dir[0]]);
  draw4dat(x+11,y+(dir[1]^dir[2]),funar[1],9,9,col[(dir[1]^dir[2])]);
  draw4dat(x+22,y+dir[1],funar[2],9,9,col[dir[1]]);
  draw4dat(x,y+10+dir[3],funar[0],9,9,col[dir[3]]);
  draw4dat(x+11,y+10+(dir[4]^dir[5]),funar[1],9,9,col[(dir[4]^dir[5])]);
  draw4dat(x+22,y+10+dir[4],funar[2],9,9,col[dir[4]]);
  }

static void drawscrn(void)
  {
  GraphMode();
  SetCols(colors,FaceCols);
  SetAllPalTo(&colors[BKGCOL]);
  moucurbox(BLEN/2,0,(320-BLEN/2),200-(BLEN*5/2));
  drawsurface(GL-3,GT-32,GRIDLEN*BLEN+5,GRIDWID*BLEN+34,FaceCols);
  BoxFill(GL,GT,GL+BLEN*GRIDLEN-1,GT+BLEN*GRIDWID-1,BKGCOL);
  Line(GL-1,GT-1,GL-1,GT+BLEN*GRIDWID-1,FaceCols[GREY3]);
  Line(GL-1,GT-1,GL+BLEN*GRIDLEN-1,GT-1,FaceCols[GREY4]);
  grid();
  BoxFill(GL-1,GT-30,GL+GRIDLEN*BLEN-1,GT-9,FaceCols[GREY3]);
  DrawFnButs(BX,GT-30,0);
  drawlistframe(BLX,BLY,BLLIST,FaceCols);
  drawlist(BLX,BLY,BLLIST,CHARBL,&curblk,blocnum);
  drawsurface(MX-4,MY-3,124,18,FaceCols);
  menub(MX,MY,FaceCols);
  helpb(HX,MY,FaceCols);
  shootb(SX,MY,FaceCols);
  drawsurface(TX-3,T1-3,17*8+6,26,FaceCols);  // for text boxes!
  drawborder((GL+GRIDLEN*BLEN+15),(BLY1+10),(SIZEX-GL-20-GRIDLEN*BLEN),(SIZEY-15-BLY1),2,2,FaceCols);
  return;
  }

void shootb(int x,int y, unsigned char col[4])
  {
  char shootar[] = "         UUUUUUU@UUUUUUUTW˜]}_ıTU]W]◊u◊UUUW◊˝◊u◊UUUUw]◊u◊UUï_◊]}_WUVUUUUUUUT)UUUUUUUh™™™™™™™Ä";
  draw4dat(x,y,shootar,35,10,col);
  }

/*  // Now Defined in Facelift.c
static void menub(int x,int y, unsigned char col[4])
  {
  char array[]= {" UUUUUUU@Z™™™™™™ïj´´øÓªÆ™Pj™˚Ó´ÓÎ™§Z™æ˚˙˚∫Í©V™ÆÓÍªÓ∫™Q™´´∫Æ˚Æ™êZ™ÍÔ˚Ææ™îj™™™™™™T UUUUUUP "};
   draw4dat(x,y,array,36,9,col);
  }

static void helpb(int x,int y, unsigned char col[4])
  {
  char array[]= {" UUUUUUU@Z™™™™™™ïj´Æˇ∫ØÍ™Pj™Î∫Æ´Æ™§Z™øÔ´™Î™©V™Æª™Íø™™Q™´ÆÍ∫Æ™™êZ™ÎøÔ˚™™îj™™™™™™T UUUUUUP "};
   draw4dat(x,y,array,36,9,col);
  }
*/

static int animons()
 {
  int l,i=FALSE;

  for(l=0;l<LASTMON;l++)
    {
    if (m[l].used) {
      if ( (m[l].lasttime != Clock) &
           ( (Clock-m[l].lasttime) > m[l].pic[m[l].cur.pic][1] ) ) {
        if (m[l].pic[(++m[l].cur.pic)][0] == MONBL)
          m[l].cur.pic = 0;
        m[l].lasttime=Clock;
        i=TRUE;
       }
     }
   }
  return(i);
 }

int monnum(int rebmun)      /* returns the current bloc number of the */
  {                          /*         monster number 'rebnum'.       */
  int i;

  animons();
  i=m[rebmun].pic[m[rebmun].cur.pic][0];
  return(i);
  }

int blocnum(int mun)
  {
  return(mun);
  }
