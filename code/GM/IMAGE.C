/*----------------------------------------------------------------------*/
/* gifread.c     By Ollie Stone                                         */
/* Created : June 20,1991                                               */
/* Last edited: 7/10/91 (Andy Stone)                                    */
/*----------------------------------------------------------------------*/

#include<process.h>
#include<stdio.h>
#include<dos.h>
#include<bios.h>
#include<alloc.h>
#include<stdlib.h>
#include"gen.h"
#include"gmgen.h"
#include"palette.h"
#include"windio.h"
#include"mousefn.h"
#include"scrnrout.h"
#include"bloc.h"
#include"gif.h"
#include"jstick.h"
#include"graph.h"
#include"facelift.h"
#include"tranmous.hpp"

#define LOLEN 12
#define LOX   15
#define LOY   176
#define LOX1  (LOX+(BLEN*LOLEN)-1)
#define LOY1  (LOY+BLEN)

#define MX    (LOX1+26)
#define MY    (LOY-2)
#define HY    (MY+11)

#define GIFPATH WorkDir
#define HELPFILE "image.hlp"

typedef struct           /* Declare a typed data structure */
 {
  int left;
  int top;
  char nam[40];
 } BKDRPdata;            /* Name the data structure */

typedef struct {
  unsigned char x[320];
  } StoreStruct;

int  blocnum(int mun);      /* dummy fn for drawlist()  */
static void cleanup(void);        /* frees allocated memory */
static int  getthem(void);        /* routine to save parts of picture as blocks */
static int  goforit(FILE *fipo);
static void setscrn(char bcol);
static void initalldata();
static int smallgif(int sizx,int sizy,FILE *fipo,RGBdata *colors);
static void getbox(int mx,int my,int len,int wid);  /* saves a rectangle of graphics screen */
static void erasebox(int mx,int my,int len,int wid);  /* redraws a saved rectangle   */
static void blocselec(int seled);
static int capture(FILE *fipo);
static void Bebeep(void);

extern unsigned _stklen=8000U;  /* Set the stack to 8000 bytes, not 4096 */
RGBdata               colors[256];
RGBdata               Black(0,0,0);
StoreStruct           replace[4];
extern blkstruct far  *blk;                      // pointer to block pictures
int                   curblk=0;                  // position in list of blocks
extern int            scrx,scrx1,scry,scry1;
extern int            xstart,ystart,imagex,imagey;
int                   bwid=1,blenn=1;
char                  saved=1,fre=0;
extern char           curfil[MAXFILENAMELEN];
extern uchar          FaceCols[MAXFACECOL];      // Screen colors
extern char          *FontPtr;
extern uchar          HiCols[4];

/*
unsigned char         *FBuf=NULL,*FCur;
unsigned long int     FLen=0,BLen=0,FCurCount=0;
#define NEXTCHAR { FCur++;    FCurCount++; }
#define REWIND   { FCur=FBuf; FCurCount=0; }
*/

QuitCodes main(int argc,char *argv[])
  {
  int choice=1;
  char attr=0;
  char w[1025],filname[MAXFILENAMELEN]="";
  FILE *fipo=NULL;    // file pointers
  unsigned register int i;
  BKDRPdata backdrop;

#ifdef DEBUG
  printf("Initialization finished.\n");
  NewAbortMSG("GM:ATE in main procedure.\n");
#endif
#ifdef CHKREG
  if ((argc < 2) || (strcmp(argv[1],OKMSG)!=0)) RestorePal=True;
#endif

  strcpy (curfil,WorkDir);
  atexit(cleanup);
  FontPtr=GetROMFont();
  if (initmouse()==FALSE)
    { errorbox("This application requires a Microsoft","compatible mouse! (Q)uit");
      return(quit); }

  blk=(blkstruct far *) farmalloc(sizeof(blkstruct)*BACKBL);
  if (blk==NULL)
    { errorbox("NOT ENOUGH MEMORY FOR BLOCKS!","  (Q)uit"); return(menu); }

  initalldata();
  do
    {
    SetTextColors();
    writestr(0,0,79,GMTOPBAR);
    writestr(0,1,31,"  Palette  Block  Monster  Map  Character  Image  Sound  Integrator  Main  Help ");
    attr = openmenu(40,3,28,13,w);
    writestr(40,3,PGMTITLECOL,"    GRAPHICS IMAGE READER   ");
    writestr(40,4,PGMTITLECOL,"        Version  "GMVER"       ");
    writestr(40,5,PGMTITLECOL,"       By Oliver Stone      ");
    writestr(40,6,PGMTITLECOL,"      Copyright(C) 1994     ");
    writestr(40,8,attr+14, " Choose GIF File ");
    writestr(40,9,attr+14, " Choose Set of Blocks");
    writestr(40,10,attr+14," New Set of Blocks");
    writestr(40,11,attr+14," Cut Images to Block Set");
    writestr(40,12,attr+14," Save Palette and Block Set");
    writestr(40,13,attr+14," Get Backdrop");
    writestr(40,14,attr+14," Delete a Backdrop");
    writestr(40,15,attr+14," Quit");
    choice=vertmenu(40,8,28,8,choice);
    closemenu(40,3,28,13,w);
    switch(choice)
      {
      case 1:
        if (fipo!=NULL) fclose(fipo);
        i=30;
        while ( (i!=0)&(filname[i]!='\\') ) filname[i--]=0;
        if (getfname(5,7,"Enter the GIF file name: ","*.gif\0",filname))
          {
          if ((fipo=fopen(filname,"rb"))==NULL)
            errorbox("Error Opening File!","(R)eturn to Main Menu");
          /*
          else
            {
            if (FBuf!=NULL) farfree(FBuf);
            FBuf=(unsigned char*) farmalloc(BLen=farcoreleft());
            FLen=fread(FBuf,1,BLen,fipo);  // Get in as much as possible
            if (FLen<BLen)                 // File is smaller then the room
              {                            // We allocated for it.
              FBuf=(unsigned char*) farrealloc(FBuf,FLen+1);
                                            // FLen set to be < Blen so that
              BLen=FLen+1;                  // we can test if whole file in.
              }
            REWIND;
            }
          */
          }
        break;
      case 2:
        if (!saved)
          {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice = 4; break; }
          }
        choice=3; fre=0;
        loadblks(1,(char *) blk);
        break;
      case 3: 
        if (!saved)
          {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice=4; break; }
          }
        initalldata();
        strcpy(curfil,WorkDir);
        saved=1; choice=3; fre=1;
        errorbox("New block set created!","(C)ontinue",2);
        break;
      case 4: 
        if (!goforit(fipo)) choice = 0; 
        break;

      case 5:
              #ifdef CRIPPLEWARE
              StdCrippleMsg();
              #else
              if ( (saveany("Enter the palette file name: ",".pal",WorkDir,sizeof(RGBdata)*256,(char *) &colors))&(saveblks(1,(char *)blk)) ) saved=TRUE;
              #endif
              choice=7; break;
      case 6: {
              char Name[MAXFILENAMELEN]="";
              char Path[MAXFILENAMELEN]="";
              if (!capture(fipo)) { choice = 0; break; }
              choice=errorbox("Save this Backdrop?","  (Y)es  (N)o")&0x00FF;
              if ( (choice=='n') || (choice=='N') ) { choice=5; break; }
              ParseFile(filname,Name,Path);
              i = strlen(Name);
              Name[i]   = 10;
              Name[i+1] = 0;
              strcpy(backdrop.nam,Name);
              backdrop.left=xstart; backdrop.top=ystart;
              saveany("Enter the backdrop name: ",".bkd",WorkDir,sizeof(BKDRPdata),(char *)&backdrop);
              choice=7;
              }
              break;
      case 7: delany("Delete which backdrop: ","*.bkd\0",WorkDir); choice = 5; break;
      case 0: choice=8;
      case 8: if (!saved) {
        choice = (errorbox("Unsaved changes--Really Quit?","  (Y)es  (N)o")&0x00FF);
        if ( (choice!='Y') && (choice!='y') ) { choice = 4; break; }
        }
      choice=8; break;
      }
    choice++;
    }
  while (choice != 9);
  fclose(fipo);
  return(menu);
  }


static int goforit(FILE *fipo)
 {
  int bgcol;
  int redo;

  scrx=0; scrx1=320; scry=0; scry1=170; xstart=0; ystart=0;
  if (intro(fipo,colors)==-1) { rewind(fipo); return(0); }
  SetCols(colors,FaceCols);
  rewind(fipo);
  if ( (imagex>(scrx1-scrx))|(imagey>(scry1-scry)) ) {
    if (smallgif(scrx1-scrx,scry1-scry,fipo,colors) == -1) { rewind(fipo); return(0); }
   }
  do {
    rewind(fipo);
    TextMode();
    if ( (bgcol=intro(fipo,colors)) == -1) { rewind(fipo); return(0); }
    if (bgcol!=-1) {
      GraphMode();
      SetAllPalTo(&colors[BKGCOL]);
      scrx=0; scrx1=SIZEX; scry=0; scry1=170;
      setscrn(bgcol);
      decode(fipo,1);
      FadeTo(colors);
      redo=getthem();   // this returns a 1 if returning from help,etc.
     }                  // this was it acts as a redraw screen.
   } while (redo);
  rewind(fipo);
  TextMode();
  return(TRUE);
  }

static int capture(FILE *fipo)
 {
  int x,y,buts=0;

  if (smallgif(SIZEX,SIZEY,fipo,colors) == -1) { rewind(fipo); return(0); }
  scrx=0;  scry=0;  scrx1=SIZEX; scry1=SIZEY;
  rewind(fipo);
  TextMode();
  if (intro(fipo,colors) == -1) { rewind(fipo);  return(0); }
  SetCols(colors,FaceCols);
  GraphMode();
  SetAllPalTo(&colors[BKGCOL]);
  decode(fipo,1);
  FadeTo(colors);
  while (bioskey(1)) bioskey(0);
  while ( (!buts)&&(!bioskey(1)) ) moustats(&x,&y,&buts);  
  mouclearbut();
  rewind(fipo);
  while (bioskey(1)) bioskey(0);
  TextMode();
  return(1);
  }

static int getthem(void)
  {
  int           mx,my,oldx=-1,oldy=0,buts=0,oldbuts=0;
  register int  j,k;
  static int    selected=0;
  int           pos,blbl,bwbl,inkey;
  char          flag=0;

  blbl=blenn*BLEN; bwbl=bwid*BLEN;
  mx=(scrx1+scrx-blbl)/2;
  my=(scry1+scry-bwbl)/2;
  moucurbox(scrx+1,scry+1,scrx1-blbl-1,scry1-bwbl+1);
  setmoupos(mx,my);
  blocselec(selected);
  mouclearbut();
  while(1) 
    {
    while(bioskey(1)) bioskey(0);
    while(my<=(scry1-bwbl)) 
      {
      if (bioskey(1))
        {
        inkey = bioskey(0);
        if ( (inkey&0x00FF) == 27 ) { selected=0; return(0); }
        inkey >>= 8;
        switch (inkey)
          {
          case 75:             // Left Arrow
            if (blenn>1)
              {
              erasebox(oldx-1,oldy-1,blbl+1,bwbl+1);
              blenn--;
              blbl=blenn*BLEN;
              moucurbox(scrx+1,scry+1,scrx1-blbl-1,scry1-bwbl+1);
              oldx=-1;
              }
            break;
          case 77:             // Right Arrow
            if ( ((blenn+1)*BLEN < scrx1-scrx)&&((blenn+1)*bwid < BACKBL)
               &&(oldx+blbl+BLEN<scrx1) )
              {
              erasebox(oldx-1,oldy-1,blbl+1,bwbl+1);
              blenn++;
              blbl=blenn*BLEN;
              moucurbox(scrx+1,scry+1,scrx1-blbl-1,scry1-bwbl+1);
              oldx=-1;
              }
            break;
          case 72:            // Up arrow
            if (bwid>1)
              {
              erasebox(oldx-1,oldy-1,blbl+1,bwbl+1);
              bwid--;
              bwbl=bwid*BLEN;
              moucurbox(scrx+1,scry+1,scrx1-blbl-1,scry1-bwbl+1);
              oldx=-1;
              }
            break;
          case 80:            // Down arrow
            if ( ((bwid+1)*BLEN < scry1-scry)&&((bwid+1)*blenn < BACKBL)
               &&(oldy+bwbl+BLEN<scry1) )
              {
              erasebox(oldx-1,oldy-1,blbl+1,bwbl+1);
              bwid++;
              bwbl=bwid*BLEN;
              moucurbox(scrx+1,scry+1,scrx1-blbl-1,scry1-bwbl+1);
              oldx=-1;
              }
            break;
           } // End of switch
         }
      moustats(&mx,&my,&buts);
      if ( (oldx!=mx)|(oldy!=my) )
        {
        if (oldx!=-1) erasebox(oldx-1,oldy-1,blbl+1,bwbl+1);
        getbox(mx-1,my-1,blbl+1,bwbl+1);
        Box(mx-1,my-1,mx+blbl,my+bwbl,(fre) ? FaceCols[RED1] : FaceCols[RED2]);
        oldx=mx; oldy=my;
        }
      if (!buts)
        {
        if (oldbuts==1)
          {
          fre=0; oldbuts=0;
          blocselec(selected);
          Box(mx-1,my-1,mx+blbl,my+bwbl,FaceCols[RED2]);
          }
        }
      else 
        {
        if (buts==1)
          {
          oldbuts=1;  pos=0;
          if (fre)
            {
            for (k=0;k<bwid;k++)
              {
              for (j=0;j<blenn;j++)
                {
                if ((selected+pos)>=BACKBL) { k=bwid; j=blenn; Bebeep(); break; }
                else getblk(mx+(j*BLEN),my+(k*BLEN),&blk[(selected+pos)%BACKBL].p[0][0]);
                saved=0;
                pos++;
                }
              }
            }
          else Bebeep();
          drawlist(LOX,LOY,LOLEN,BACKBL,&curblk,blocnum);
          }
        if (buts==2) 
          {
          fre=1;
          if (selected==BACKBL) selected=1;
          do {
            flag=0; selected++;
            for(j=0;j<BLEN;j++)
              for(k=0;k<BLEN;k++)
                if ( blk[selected].p[j][k] != 0 ) flag=1;
            } while ( (flag)&&(selected<BACKBL) );
          if ( (flag)&&(selected==BACKBL) ) { selected=0; curblk=0; }
          else curblk = (selected+BACKBL-(LOLEN/2))%BACKBL;
          drawlist(LOX,LOY,LOLEN,BACKBL,&curblk,blocnum);
          blocselec(selected);
          Box(mx-1,my-1,mx+blbl,my+bwbl,FaceCols[RED1]);
          while (buts==2) moustats(&mx,&my,&buts);        
          }
        }
      }
    erasebox(oldx-1,oldy-1,blbl+1,bwbl+1);
    moucurbox(0,0,MX+36,LOY1);
    setmoupos(mx,(LOY+1) ); my=LOY+1;
    moucur(1);
    while( my>=(LOY-4) )
      {
      moustats(&mx,&my,&buts);
      if ( (mx>LOX)&(mx<LOX1)  ) 
        {
        if (buts==1)
          {
          if ( (selected != (curblk+(mx-LOX)/BLEN)%BACKBL)||(fre==0) )
            {
            fre=1;
            moucur(0);
            drawlist(LOX,LOY,LOLEN,BACKBL,&curblk,blocnum);            
            selected = (curblk+(mx-LOX)/BLEN)%BACKBL;
            blocselec(selected);
            moucur(1);
            }
          }
        if (buts==2) 
          {
          flag=0;
          if ( (selected!=(curblk+(mx-LOX)/BLEN)%BACKBL)||(fre==0) )
            {
            fre=1;
            moucur(0);
            drawlist(LOX,LOY,LOLEN,BACKBL,&curblk,blocnum);
            selected=(curblk+(mx-LOX)/BLEN)%BACKBL;
            blocselec(selected);
            moucur(1);
            }
          for(j=0;j<BLEN;j++)
            for(k=0;k<BLEN;k++)
              if ( blk[selected].p[j][k] != 0 ) { flag=1; j=(k=BLEN); }
          if (flag)
            {
            for(j=0;j<sizeof(blkstruct);j++)
              *(((unsigned char far *)&blk[selected])+j)=0;
            blk[selected].solid=15;
            blk[selected].grav=15;
            blk[selected].touchbl=selected;
            blk[selected].nextbl=selected;
            blk[selected].effect=1;
            saved=0;
            moucur(0);
            drawlist(LOX,LOY,LOLEN,BACKBL,&curblk,blocnum);
            blocselec(selected);
            moucur(1);
            }
          }
        }
      if (buts!=0) 
        {
        if ( (mx>LOX-11)&&(mx<LOX-3) )   { moucur(0); DrawArrows(LOX,LOY,LOLEN,LEFT,FaceCols); scroll_list(2,LOX,LOY,LOLEN,BACKBL,&curblk,blocnum); DrawArrows(LOX,LOY,LOLEN,NONE,FaceCols); blocselec(selected); moucur(1); }
        if ( (mx>LOX1+2)&&(mx<LOX1+10) ) { moucur(0); DrawArrows(LOX,LOY,LOLEN,RIGHT,FaceCols); scroll_list(-2,LOX,LOY,LOLEN,BACKBL,&curblk,blocnum); DrawArrows(LOX,LOY,LOLEN,NONE,FaceCols); blocselec(selected); moucur(1); } 
        if ( (mx>MX)&(my>=MY)&(my<MY+9) ) 
          { mouclearbut(); selected=0; return(0); }
        if ( (mx>MX)&(my>=HY)&(my<HY+9) )
          { 
          mouclearbut();
          moucur(0);
          TextMode();
          DisplayHelpFile(HELPFILE);
          while (bioskey(1)) bioskey(0);  mouclearbut();
          return(1); // Re-enter's fn to redraw screen
          }
        }
      }
    moucur(0);
    moucurbox(scrx+1,scry+1,scrx1-blbl-1,scry1-bwbl+1);
    if (mx>(scrx1-blbl-1)) mx=(scrx1-blbl-1);
    if (mx<(scrx+1)) mx=scrx+1;
    oldy=(my=scry1-bwbl);
    setmoupos(mx,my);
    oldbuts=0; oldx=-1;
    }
  }

static int smallgif(int sizx,int sizy,FILE *fipo,RGBdata *colors)
  {
  int buts=0;
  int isok,border=7;
  int oldsx,oldsy;

  isok=intro(fipo,colors);
  if (isok!=-1)
    {
    GraphMode();
    moucur(0);
    SetCols(colors,FaceCols);
    SetAllPalTo(&colors[BKGCOL]);
    xstart=0; ystart=0;
    if ( (sizx>=imagex)&&(sizy>=imagey) )  return(isok);
    scrx=(320-imagex/4)/2; scry=(200-imagey/4)/2;
    if (scrx<border) scrx=border;
    if (scry<border) scry=border;
    scrx1=scrx+imagex/4; scry1=scry+imagey/4;    //imagex/2?  (original code)
    if (scrx1>SIZEX-border+1) scrx1=SIZEX-border+1;
    if (scry1>SIZEY-border+1) scry1=SIZEY-border+1;
    drawborder(scrx-2,scry-2,scrx1-scrx+4,scry1-scry+4,2,2,FaceCols);
    BoxFill(scrx-2,scry-2,scrx1+1,scry1+1,isok);
    decode(fipo,4);
    scrx--;  scry--;
    if (sizx>imagex) sizx=imagex;
    if (sizy>imagey) sizy=imagey;
    moucurbox(scrx,scry,scrx1-sizx/4-1,scry1-sizy/4-1);
    mouclearbut();
    while (bioskey(1)) bioskey(0);
    setmoupos(scrx,scry);
    xstart=scrx; ystart = scry;
    getbox(scrx,scry,sizx/4+1,sizy/4+1);
    Box(scrx,scry,scrx+sizx/4+1,scry+sizy/4+1,FaceCols[RED1]);
    FadeTo(colors);
    while( (!buts)&&(!bioskey(1)) )
      {
      oldsx=xstart;  oldsy = ystart;
      while( (xstart==oldsx)&&(ystart==oldsy)&&(buts==0)&&(!bioskey(1)) )
        moustats(&xstart,&ystart,&buts);
      erasebox(oldsx,oldsy,sizx/4+1,sizy/4+1);
      getbox(xstart,ystart,sizx/4+1,sizy/4+1);
      Box(xstart,ystart,xstart+sizx/4+1,ystart+sizy/4+1,FaceCols[RED1]);
      }
    xstart -= scrx;  xstart*=4;
    ystart -= scry;  ystart*=4;
    }
  return(isok);
  }

static void erasebox(int mx,int my,int len,int wid)
 {
  register int j;

  for (j=1;j<len;j++)
    Point(mx+j,my,replace[0].x[j]);
  for (j=1;j<len;j++)
    Point(mx+j,my+wid,replace[1].x[j]);
  for (j=0;j<=wid;j++)
    Point(mx,my+j,replace[2].x[j]);
  for (j=0;j<=wid;j++)
    Point(mx+len,my+j,replace[3].x[j]);
 }

static void getbox(int mx,int my,int len,int wid)
  {
  register int j;
//  unsigned char far *gmem = (unsigned char far *)0xA0000000;

  if ( (mx<0)||(my<0)||(mx+len>=320)||(my+wid>=200) ) return;
  for (j=1;j<len;j++)
    replace[0].x[j]=(unsigned char)(GetCol(mx+j,my));     // =*( gmem+mx+j+(my*320) );
  for (j=1;j<len;j++)
    replace[1].x[j]=(unsigned char)(GetCol(mx+j,my+wid)); // =*( gmem+mx+j+((my+wid)*320) );
  for (j=0;j<=wid;j++)
    replace[2].x[j]=(unsigned char)(GetCol(mx,my+j));     // =*( gmem+mx+((my+j)*320) );
  for (j=0;j<=wid;j++)
    replace[3].x[j]=(unsigned char)(GetCol(mx+len,my+j)); // =*( gmem+mx+len+((my+j)*320) );
  }

static void blocselec(int seled)
  {
  int mx;

  if ( ( (seled>=curblk)&(seled<curblk+LOLEN) ) |
       ( (seled<(curblk+LOLEN)%BACKBL)&
         (seled>=((curblk+LOLEN)%BACKBL)-LOLEN) ) )
    {
    mx = LOX + ((BACKBL+seled-curblk)%BACKBL)*BLEN;
    Box(mx+2,LOY+2,mx+BLEN-2,LOY1-2,(fre) ? FaceCols[RED1] : FaceCols[RED2]);
    }
  }

static void initalldata(void)
  {
  unsigned register int l1;

  for (l1=0;l1<sizeof(blkstruct)*BACKBL;l1++)  // Zero Whole Block Structure
    *( ((char far *)blk)+l1)=0;
  for (l1=0; l1<BACKBL;l1++) 
    { 
    blk[l1].solid=15;
    blk[l1].grav=15;
    blk[l1].touchbl=l1;
    blk[l1].nextbl=l1;
    blk[l1].effect=1;
    }
  fre=1;
  }

static void setscrn(char bcol)
  {
  moucur(0);
  BoxFill(scrx,scry,scrx1,scry1,bcol);
//  Box(LOX-5,LOY-1,LOX1+5,LOY1,col);  /* draw list window */
//  Line(LOX-1,LOY-1,LOX-1,LOY1,col);
//  Line(LOX1+1,LOY-1,LOX1+1,LOY1,col);
  drawlistframe(LOX,LOY,LOLEN,FaceCols);
  drawlist(LOX,LOY,LOLEN,BACKBL,&curblk,blocnum);
  drawsurface(MX-2,MY-1,40,25,FaceCols);
  menub(MX,MY,FaceCols);
  helpb(MX,HY,FaceCols);
  }

static void cleanup(void)
  {
  Time.TurnOff();
  farfree(blk);
  }

static void Bebeep(void)
  {
  sound(440);
  delay(50);
  nosound();
  delay(25);
  sound(440);
  delay(100);
  nosound();
  }

int blocnum(int mun)
  { 
  return(mun);
  }

