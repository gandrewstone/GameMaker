/*--------------------------------------------------------------------*/
/* gramap.c             Programmer: Andy Stone   Created: 7/12/91     */
/* Module for grator.c that lets you define the links between two     */
/* maps.  Loads up the maps and allows you to graphically place the   */
/* link on the map.                                                   */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <bios.h>
#include <stdlib.h>
#include <alloc.h>
#include <string.h>

#include "gen.h"
#include "gmgen.h"

#include "scrnrout.h"
#include "mousefn.h"
#include "windio.h"
#include "palette.h"
#include "bloc.h"
#include "mon.h"
#include "map.h"
#include "graph.h"
#include "facelift.h"
#include "grator.h"

#define MAP1X           18
#define MAP1Y           5
#define MAP2X           163
#define MAP2Y           MAP1Y
#define MAPSCRNX        7
#define MAPSCRNY        9

#define NEXTBX          161
#define NEXTBY          HELPBY
#define PREVBX          (NEXTBX-25)
#define PREVBY          NEXTBY
#define HELPBX          250
#define HELPBY          186
#define DONEBX          30
#define DONEBY          HELPBY
#define BUTLEN          10
#define IN(x1,y1,x2,y2) ((mx>x1)&(mx<x2)&(my>y1)&(my<y2))
        /* IN() is true if the mouse is in the box of (x1...y2) */
#define HELPFILE        "gramap.hlp"

static int loadspecmap(char *file,mapstruct *map);
static int loadspecblk(char *file,int numblks,blkstruct *blk);
static void drawscrn(int from,int cm, int to,int tocol);
static void drawmap(int x,int y, int mx, int my, int len, int wid, mapstruct map[MWID][MLEN], blkstruct *b);
static void ColorChange(int cm,int tocol);

static int edgey(int y);
static int edgex(int x);
static int onscrn(int stx,int sty, int qx,int qy, int *retx, int *rety, int windnum);

static int edit (int *fx,int *fy,int *tx,int *ty);

static void linkb(int x,int y, unsigned char col[4]);
//static void helpb(int x,int y, unsigned char col[4]);
//static void doneb(int x,int y, unsigned char col[4]);
static void prevb(int x,int y, unsigned char col[4]);
static void nextb(int x,int y, unsigned char col[4]);
static void DrawAllButs(unsigned char col[4]);

static int HighLight(int oldbut,int bnum,int cm);
static int UnHighLight(int bnum,unsigned char col[4]);

static int FindNext(int link);
static int FindPrev(int link);

//-----------------Variables

extern GameClass  Game;
static RGBdata    colors[2][256];
static mapstruct  maps[2][MWID][MLEN];
static blkstruct *blks[2];     /* addresses - from[0],to[1] */

static mapstruct *map;
extern blkstruct *blk;    /* The variable used w/ drawblock */

extern char      *FontPtr;
extern uchar      HiCols[4];
static uchar      HiCol0[4];
static uchar      FaceCol[2][MAXFACECOL];      //Screen colors

static int from=0,to=0;
static int acol[2]={100,100},bcol[2]={50,50};

int expandlink(int fro,int link)
  {
  unsigned int lx,ly;
  int done=FALSE;

  from=fro;
  to=Game.scns[from].links[link].ToScene;   // Get the to scene #
  
  if ((Game.scns[from].gametype==STARTTYPE)&&(Game.scns[to].gametype==ENDTYPE)) return(FALSE);

  blks[0]= (blkstruct  *) farmalloc(sizeof(blkstruct)*(BACKBL+1));
  if (blks[0]==NULL) { return(0);}
  blks[1]= (blkstruct  *) farmalloc(sizeof(blkstruct)*(BACKBL+1));
  if (blks[1]==NULL) { farfree(blks[0]); return(0);}
 
  for (lx=0;lx<sizeof(blkstruct)*(BACKBL+1);lx++)  // init. data struct
    {
    *(((char  *) blks[0])+lx) =0;
    *(((char  *) blks[1])+lx) =0;
    }

  for (lx=0; lx<MLEN;lx++)
    for (ly=0; ly<MWID;ly++)
      {
      maps[0][ly][lx].blk=BACKBLK; 
      maps[0][ly][lx].mon=LASTMM;
      maps[1][ly][lx].blk=BACKBLK; 
      maps[1][ly][lx].mon=LASTMM;
      }

  if (Game.scns[from].gametype!=STARTTYPE)
    {
    loadspecany(Game.File(from,0),"","",sizeof(RGBdata)*256,(char *) &(colors[0]));
    if (!loadspecblk(Game.File(from,2),BACKBL,blks[0])) done=TRUE;
    if (!loadspecmap(Game.File(from,1),*(maps[0]) )) done=TRUE;
    }
  else if(!loadspecany(Game.Files(Game.scns[to].Files[0]),"","",sizeof(RGBdata)*256,(char *) &(colors[0])))
    { GetAllPal(colors[0]); }
 if (Game.scns[to].gametype!=ENDTYPE)
    {
    loadspecany(Game.File(to,0),"","",sizeof(RGBdata)*256,(char *) &(colors[1]));
    if (!loadspecblk(Game.File(to,2),BACKBL,(blks[1]))) done=TRUE;
    if (!loadspecmap(Game.File(to,1),*(maps[1]) ))   done=TRUE;
    }
  else if (!loadspecany(Game.Files(Game.scns[from].Files[0]),"","",sizeof(RGBdata)*256,(char *) &(colors[1])))
    { GetAllPal(colors[1]); }
  mouclearbut();
  BoxFill(0,0,SIZEX,SIZEY,0);

  setmoupos(200,100);
  while (!done)
    {
    done=edit(&(Game.scns[from].links[link].From.x),&(Game.scns[from].links[link].From.y),&(Game.scns[from].links[link].To.x),&(Game.scns[from].links[link].To.y));
    if (done==3)  // Edit Prev Link
      {
      link=FindPrev(link);
      done=FALSE;
      }
    if (done==4)  // Edit next Link
      {
      link=FindNext(link);
      done=FALSE;
      }
    mouclearbut();
    }
    
  farfree(blks[0]);
  farfree(blks[1]);
  return(1);
  }
    
static int FindPrev(int link)
  {
  while(1)
    {
    link--;
    if (link<0) link+=MAXSCENELINKS;
    if (Game.scns[from].links[link].ToScene==to) return(link);
    }
  }

static int FindNext(int link)
  {
  while(1)
    {
    link++;
    if (link>=MAXSCENELINKS) link-=MAXSCENELINKS;
    if (Game.scns[from].links[link].ToScene==to) return(link);
    }
  }

static int edit(int *fx,int *fy,int *tx,int *ty)
  {
  int           cm=0; // (sf. curmap) whether mouse is in from(0) map or to(1)
  int           curx[2],cury[2];
  int           done=FALSE;
  int           upkey=0;
  int           mx=100,my=100,mbuts=0;
  int           ox=100,oy=100,obuts=0;
  int           changed=FALSE;
  int           tempx=0,tempy=0;  // Temp variables to hold screen coordinates
  static unsigned char linkcols[2][4];
  char          onbut=0;
  static char   redraw=1;
  
  curx[0]=edgex(*fx);
  curx[1]=edgex(*tx);
  cury[0]=edgey(*fy);
  cury[1]=edgey(*ty);
  
                        // Find colors for link  buttons
  if (redraw)
    {
    SetCols(colors[0],FaceCol[0]);
    HiCol0[0]=HiCols[0]; HiCol0[1]=HiCols[1]; HiCol0[2]=HiCols[2]; HiCol0[3]=HiCols[3];
    SetCols(colors[1],FaceCol[1]);
  /*   Set up color array so it is the closest in available */
  /* palette to desired color. */
    acol[0]=PalFindCol(63,63,0,colors[0]);    /* Find Yellow in pal#0 */
    acol[1]=PalFindCol(63,63,0,colors[1]);    /* Find Yellow in pal#1 */
    bcol[0]=PalFindCol(0,0,63,colors[0]);    /* Find blue in pal#0 */
    bcol[1]=PalFindCol(0,0,63,colors[1]);    /* Find blue in pal#1 */
    linkcols[0][0]=(linkcols[1][0]=255);
    linkcols[0][1]=FaceCol[0][RED1];
    linkcols[0][2]=FaceCol[0][RED2];
    linkcols[0][3]=FaceCol[0][GREY1];
    linkcols[1][1]=FaceCol[1][RED1];
    linkcols[1][2]=FaceCol[1][RED2];
    linkcols[1][3]=FaceCol[1][GREY1];

    SetAllPalTo(&colors[cm][FaceCol[cm][BLACK]]);
    drawscrn(from,cm,to,bcol[cm]);
    }

  moucur(0);
  drawmap(MAP1X,MAP1Y,curx[0],cury[0],MAPSCRNX,MAPSCRNY,maps[0],blks[0]);
  drawmap(MAP2X,MAP2Y,curx[1],cury[1],MAPSCRNX,MAPSCRNY,maps[1],blks[1]);
  DrawAllButs(FaceCol[cm]);
  mouclearbut();
  moucur(1);
  if (onscrn(curx[0],cury[0],*fx,*fy,&tempx,&tempy,0))
    { linkb(tempx+2,tempy+2,linkcols[0] ); }
  if (onscrn(curx[1],cury[1],*tx,*ty,&tempx,&tempy,1))
    { linkb(tempx+2,tempy+2,linkcols[0] ); }
  if (redraw) FadeTo(colors[cm]);

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

    obuts=mbuts;
    if ((changed&12)==12)                  // Button just pushed
      {
      if (onbut!=0) {changed=4; obuts=0;}
      else if (IN(DONEBX,DONEBY,DONEBX+36,DONEBY+BUTLEN))
        onbut=HighLight(onbut,1,cm); 
      else if (IN(HELPBX,HELPBY,HELPBX+36,HELPBY+BUTLEN))
        onbut=HighLight(onbut,2,cm); 
      else if (IN(PREVBX,PREVBY,PREVBX+24,PREVBY+BUTLEN))
        onbut=HighLight(onbut,3,cm); 
      else if (IN(NEXTBX,NEXTBY,NEXTBX+24,NEXTBY+BUTLEN))
        onbut=HighLight(onbut,4,cm); 
      else if (IN(MAP1X,MAP1Y,MAP1X+(MAPSCRNX*BLEN),MAP1Y+(MAPSCRNY*BLEN)))  
        {                         // Currently in from scene
        onbut=6;
        blk=blks[0];
        moucur(FALSE);
        if (onscrn(curx[0],cury[0],*fx,*fy,&tempx,&tempy,0)) // Erase old link marker
          drawblk(tempx,tempy,&blk[maps[0][*fy][*fx].blk].p[0][0]);
        moucur(TRUE);
        }
      else if (IN(MAP2X,MAP2Y,MAP2X+(MAPSCRNX*BLEN),MAP2Y+(MAPSCRNY*BLEN)))
        {                        // Currently in 'to' scene
        onbut=7;
        blk=blks[1];
        moucur(FALSE);
        if (onscrn(curx[1],cury[1],*tx,*ty,&tempx,&tempy,1))
          drawblk(tempx,tempy,&blk[maps[1][*ty][*tx].blk].p[0][0]);
        moucur(TRUE);
        }
      }
    if ((changed&2)==2)                  // Mouse moved
      {
      if ((cm==0)&&(mx>=MAP2X-3))  // Switch to left (from map) window
        {
        cm=1;
        ColorChange(cm,bcol[cm]);
        if (onscrn(curx[0],cury[0],*fx,*fy,&tempx,&tempy,0)) { linkb(tempx+2,tempy+2,linkcols[cm] ); }
        if (onscrn(curx[1],cury[1],*tx,*ty,&tempx,&tempy,1)) { linkb(tempx+2,tempy+2,linkcols[cm] ); }
        SetAllPal(colors[cm]);
        DrawAllButs(FaceCol[cm]);        
        }
      if ((cm==1)&&(mx<=MAP1X+(MAPSCRNX*BLEN)+1)) // Switch to right (to map) window */
        {
        cm=0;
        ColorChange(cm,bcol[cm]);
        if (onscrn(curx[0],cury[0],*fx,*fy,&tempx,&tempy,0)) { linkb(tempx+2,tempy+2,linkcols[cm] ); }
        if (onscrn(curx[1],cury[1],*tx,*ty,&tempx,&tempy,1)) { linkb(tempx+2,tempy+2,linkcols[cm] ); }
        SetAllPal(colors[cm]);
        DrawAllButs(FaceCol[cm]);
        }
      }
      
    if ((changed&12)==4)                    // mouse button released
      {
      switch (onbut)
        {
        case 1:                       // Done Button
          done=TRUE; 
          redraw=1;
          break;
        case 2:                       // Help Button
          TextMode();
          DisplayHelpFile(HELPFILE);
          GraphMode();
          SetAllPalTo(&colors[cm][FaceCol[cm][BLACK]]);
          drawscrn(from,cm,to,bcol[cm]);
          moucur(0);
          drawmap(MAP1X,MAP1Y,curx[0],cury[0],MAPSCRNX,MAPSCRNY,maps[0],blks[0]);
          drawmap(MAP2X,MAP2Y,curx[1],cury[1],MAPSCRNX,MAPSCRNY,maps[1],blks[1]);
          onbut=0;  //HighLight(onbut,0,cm);
          setmoupos(HELPBX+4,HELPBY+30);
          if (onscrn(curx[0],cury[0],*fx,*fy,&tempx,&tempy,0))
            { linkb(tempx+2,tempy+2,linkcols[cm] ); }
          if (onscrn(curx[1],cury[1],*tx,*ty,&tempx,&tempy,1))
            { linkb(tempx+2,tempy+2,linkcols[cm] ); }
          moucur(1);
          SetAllPal(colors[cm]);

        case 3:                       // Prev Button
          done=TRUE;
          redraw=0;
          break;
        case 4:                       // Next Button
          done=TRUE;
          redraw=0;
          break;
        case 6:                       // From link
          onbut=0;
          if (IN(MAP1X,MAP1Y,MAP1X+(MAPSCRNX*BLEN),MAP1Y+(MAPSCRNY*BLEN)))  
            {              // Currently in from scene
            mx-=MAP1X;
            my-=MAP1Y;
            my/=BLEN;
            mx/=BLEN;
            mx+=curx[0]; my+=cury[0];
            if (mx>=MLEN) mx-=MLEN;
            if (my>=MWID) my-=MWID;
            if (mx<0) mx +=MLEN;
            if (my<0) my +=MWID;

            *fx=mx;  // modify from coordinates
            *fy=my;
            }
          moucur(FALSE);
          onscrn(curx[0],cury[0],*fx,*fy,&tempx,&tempy,0); /* Calc new link */
          linkb(tempx+2,tempy+2,linkcols[0] ); /* draw link  | sceen coords.  */
          moucur(TRUE);
          break;
        case 7:                       // To link
          onbut=0;
          if (IN(MAP2X,MAP2Y,MAP2X+(MAPSCRNX*BLEN),MAP2Y+(MAPSCRNY*BLEN)))
            {                      // Currently in 'to' scene
            mx-=MAP2X;
            my-=MAP2Y;
            my/=BLEN;
            mx/=BLEN;
            mx+=curx[1]; my+=cury[1];
            if (mx>=MLEN) mx-=MLEN;
            if (my>=MWID) my-=MWID;
            if (mx<0) mx +=MLEN;
            if (my<0) my +=MWID;
            *tx=mx;                 // modify 'to' coordinates
            *ty=my;
            }
          moucur(FALSE);
          onscrn(curx[1],cury[1],*tx,*ty,&tempx,&tempy,1);
          linkb(tempx+2,tempy+2,linkcols[1] ); 
          moucur(TRUE);
          break;
        }
      }
    if ((changed&1)==1)                        // Key hit
      {
      changed=FALSE;
      upkey=bioskey(0);
      switch(upkey&255)
        {
        case 27:
          done=TRUE;
          redraw=1;
          onbut=5;
          break;
        case 0:
          switch (upkey>>8)
            {
            case 72:                // Up arrow
              cury[cm] -=1;
              if (cury[cm]<0) cury[cm] = MWID-1;
              changed=TRUE;
              break;
            case 80:               // Down arrow
	      cury[cm]+=1;
	      if (cury[cm] >=MWID) cury[cm] = 0;
	      changed=TRUE;
              break;
            case 77:
              curx[cm]+=1;
              if (curx[cm]>=MLEN) curx[cm] = 0;
              changed=TRUE;
              break;
            case 75:
              curx[cm]-=1;
	      if (curx[cm]<0) curx[cm] = MLEN-1;
	      changed=TRUE;
              break;
            }
          if (changed==TRUE)
            {
            moucur(FALSE);
            if (cm==0) 
              {
              drawmap(MAP1X,MAP1Y,curx[0],cury[0],MAPSCRNX,MAPSCRNY,maps[0],blks[0]);
              if (onscrn(curx[0],cury[0],*fx,*fy,&tempx,&tempy,0)) { linkb(tempx+2,tempy+2,linkcols[0]); }
              }
            else 
              {
              drawmap(MAP2X,MAP2Y,curx[1],cury[1],MAPSCRNX,MAPSCRNY,maps[1],blks[1]);
              if (onscrn(curx[1],cury[1],*tx,*ty,&tempx,&tempy,1)) { linkb(tempx+2,tempy+2,linkcols[1]); }
              }
            moucur(TRUE);
            }
          break;
        }
      }
    }
  return(onbut);
  }

static int UnHighLight(int bnum,unsigned char col[4])
  {
  moucur(FALSE);
  switch(bnum)
    {
    case 1:
      doneb(DONEBX,DONEBY,col);
      break;
    case 2:
      helpb(HELPBX,HELPBY,col);
      break;
    case 3:
      prevb(PREVBX,PREVBY,col);
      break;
    case 4:
      nextb(NEXTBX,NEXTBY,col);
      break;
    }
  moucur(TRUE);
  return(0);  /* Nothing highlighted */
  }

static int HighLight(int oldbut,int bnum,int cm)
  {
  unsigned char *col;
  
  if (oldbut==bnum) return(bnum);
  UnHighLight(oldbut,FaceCol[cm]);   

  if (cm==0) col=HiCol0;
  else       col=HiCols;

  moucur(FALSE);
  switch(bnum)
    {
    case 1:
      doneb(DONEBX,DONEBY+1,col);
      break;
    case 2:
      helpb(HELPBX,HELPBY+1,col);
      break;
    case 3:
      prevb(PREVBX,PREVBY+1,col);
      break;
    case 4:
      nextb(NEXTBX,NEXTBY+1,col);
      break;
    }
  moucur(TRUE);

  return(bnum);
  }


static int onscrn(int stx,int sty, int qx,int qy, int *retx, int *rety, int windnum)
  {
  int xin=FALSE,yin=FALSE;
  
  if ((qx-stx<MAPSCRNX)&(qx-stx>=0))  /* Test to see if qx is in screen */
    {
    if (windnum==0) *retx=MAP1X+((qx-stx)*BLEN);
    if (windnum==1) *retx=MAP2X+((qx-stx)*BLEN);
    xin=TRUE;
    }  
  if ((qy-sty<MAPSCRNY)&(qy-sty>=0))
    {
    if (windnum==0) *rety=MAP1Y+((qy-sty)*BLEN);
    if (windnum==1) *rety=MAP2Y+((qy-sty)*BLEN);
    yin=TRUE;
    }  
  if ((!xin)&(stx>=MLEN-MAPSCRNX)&((MLEN-stx)+qx<MAPSCRNX)) /* Test if on a screen wrap */
    {
    if (windnum==0) *retx=MAP1X+(((MLEN-stx)+qx)*BLEN);
    if (windnum==1) *retx=MAP2X+(((MLEN-stx)+qx)*BLEN);
    xin=TRUE;
    }
  if ((!yin)&(sty>=MWID-MAPSCRNY)&( (MWID-sty)+qy<MAPSCRNY)) /* Test if on a screen wrap */
    {
    if (windnum==0) *rety=MAP1Y+(((MLEN-sty)+qy)*BLEN);
    if (windnum==1) *rety=MAP2Y+(((MLEN-sty)+qy)*BLEN);
    yin=TRUE;
    }
  return(xin&yin);  
  }


static int edgex(int x)  /* if x is supposed to be shown on the middle of the scrn, edge finds the upper left coords */ 
  {
  x-= MAPSCRNX/2;
  if (x<0) x+=MLEN;
  return(x);
  }

static int edgey(int y)
  {
  y-= MAPSCRNY/2;
  if (y<0) y+=MWID;
  return(y);
  }


static void drawmap(int x,int y, int mx, int my, int len, int wid, mapstruct map[MWID][MLEN], blkstruct  *b)
  {
  register int lx,ly;

  blk=b;
  for (ly=my;ly<my+wid;ly++)
    for (lx=mx;lx<mx+len;lx++)
      {
      drawblk(x+((lx-mx)*BLEN),y+((ly-my)*BLEN),&blk[map[ly%MWID][lx%MLEN].blk].p[0][0]);
      }
  }

static void ColorChange(int cm,int tocol)
  {
  unsigned char colone;

  moucur(0);
  if (cm==0) { colone=acol[0]; tocol=bcol[1]; }
  else       { colone=bcol[0]; tocol=acol[1]; }
  GWrite(6,MAP1Y+12,colone,"F\nr\no\nm");
  GWrite(308,MAP2Y+20,tocol,"T\no");
  Box(MAP1X-1,MAP1Y-1,MAP1X+(MAPSCRNX*BLEN),MAP1Y+(MAPSCRNY*BLEN),colone);
  Box(MAP2X-1,MAP2Y-1,MAP2X+(MAPSCRNX*BLEN),MAP2Y+(MAPSCRNY*BLEN),tocol);
  moucur(1);
  }
    
static void drawscrn(int from,int cm, int to,int tocol)
  {
  int temp=0,lp;
  char letter[2]={0,0};
  
  moucur(0);

  drawsurface(2,2,316,196,FaceCol[cm]);
  Line(4,MAP1Y+50,4,MAP1Y+163,FaceCol[cm][GREY3]);
  Line(306,MAP2Y+50,306,MAP2Y+163,FaceCol[cm][GREY3]);
  Line(4,MAP1Y+50,13,MAP1Y+50,FaceCol[cm][GREY4]);
  Line(306,MAP2Y+50,315,MAP2Y+50,FaceCol[cm][GREY4]);
  BoxFill(307,MAP2Y+51,315,MAP2Y+163,FaceCol[cm][GREEN]);
  BoxFill(5,MAP1Y+51,13,MAP1Y+163,FaceCol[cm][GREEN]);
  Line(NEXTBX-1,NEXTBY+1,NEXTBX-1,NEXTBY+9,FaceCol[cm][GREY2]);
  Point(NEXTBX-1,NEXTBY+10,FaceCol[cm][GREY4]);
  DrawAllButs(FaceCol[0]);
  setmoupos(DONEBX+18,DONEBY+5);
  temp=strlen(Game.scns[from].desc);
  for (lp=0;lp<temp;lp++)
    {
    letter[0]=Game.scns[from].desc[lp];
    GWrite(6,MAP1Y+52+lp*8,FaceCol[cm][BLACK],letter);
    }
  temp=strlen(Game.scns[to].desc);
  for (lp=0;lp<temp;lp++)
    {
    letter[0]=Game.scns[to].desc[lp];
    GWrite(308,MAP2Y+52+lp*8,FaceCol[cm][BLACK],letter);
    }
  Line(MAP1X-2,MAP2Y-2,MAP2X+MAPSCRNX*BLEN,MAP2Y-2,FaceCol[cm][GREY4]);
  BoxFill(MAP1X+MAPSCRNX*BLEN+1,MAP1Y-2,MAP2X-3,MAP1Y+MAPSCRNY*BLEN,FaceCol[cm][GREY1]);
  Line(MAP1X-2,MAP1Y-1,MAP1X-2,MAP1Y+MAPSCRNY*BLEN,FaceCol[cm][GREY3]);
  Line(MAP2X-2,MAP2Y-1,MAP2X-2,MAP2Y+MAPSCRNY*BLEN,FaceCol[cm][GREY3]);
  moucur(1);
  ColorChange(cm,tocol);
  }



static int loadspecblk(char *file,int numblks,blkstruct  *blk)
  {
  FILE *fp;
  fp=fopen(file,"rb"); /* must exist because getfname searches for it */
  if (fp==NULL) return(FALSE);
  fread((unsigned char  *)blk,sizeof(blkstruct),numblks,fp); 
  fclose(fp);
  return(TRUE);
  }

static int loadspecmap(char *file,mapstruct *map)
  {
  FILE *fp;
  if  ((fp=fopen(file,"rb"))==NULL) return(FALSE);
  fread( (char *) map,sizeof(mapstruct),MLEN*MWID,fp);
  fclose(fp);
  return(TRUE);
  }

static void DrawAllButs(unsigned char col[4])
  {
  moucur(FALSE);
  prevb(PREVBX,PREVBY,col);
  nextb(NEXTBX,NEXTBY,col);
  helpb(HELPBX,HELPBY,col);
  doneb(DONEBX,DONEBY,col);
  moucur(TRUE);
  }

/*
void prevb(int x,int y, unsigned char col[4])
  {
  char array[]= {" n ¥U	U%U%U–¥™Y™Y™Yš¥™U™U™U•U–¥™Y™Y™Yš¥™•™e™Y•U–¥™Y™U–•™U™U™Y–¥•U™Y™Y™Y–e–e–e••%•%U	UU ¥ n"};
  array[1]=10;
  array[91]=10;  
   draw4dat(x,y,array,7,45,col);
  }

void doneb(int x,int y, unsigned char col[4])
  {
  char array[]= {" n ¥U	U%U%Uš•™e™Y™Y™Y™Y™eš••U–¥™Y™Y™Y™Y™Y™Y–¥•U™¥šY™Y™Y™Y™Y™Y™Y•U–¥™Y™U–•™U™U™Y&¥%U	UU ¥ n"};
  array[1]=10;
  array[91]=10;  

   draw4dat(x,y,array,7,45,col);
  }

void nextb(int x,int y, unsigned char col[4])
  {
  char array[]= {"  Z U€U`UXUXf–ifefefefefefefUVZ–efeVZVeVeVefZ–UVefefY–VVVVY–efefUVj¦VVVVVVVVVVVVVXUXU`U€Z   "};
  draw4dat(x,y,array,7,45,col);

  }

void helpb(int x,int y, unsigned char col[4])
  {
  char array[]= {"  Z U€U`UXUXefefefj¦efefefefUVZ–efeVZVeVeVefZ–UVeVeVeVeVeVeVeVZ¦UVZ–efefefj–eVeVeXUXU`U€Z   "};
  draw4dat(x,y,array,7,45,col);

  }
*/

void linkb(int x,int y, unsigned char col[4])
  {
  static char array[] = " UU UUPiUT©UTU©UUU©UUU©UUU©UUU©UUU©UUU©UUUªªUªªTj©TUUP UU ";
  draw4dat(x,y,array,15,15,col);
  }

static void prevb(int x,int y, unsigned char col[4])
  {
  static char prevar[] = "      UUUUUUUUUUõýuuW]×uuuWõý}uuWU×u]Õ—U×WUUUUUU)UUUUUªªªªª";
  draw4dat(x,y,prevar,23,10,col);
  }

static void nextb(int x,int y, unsigned char col[4])
  {
  static char nextar[] = "      UUUUU@UUUUUT]wý×ô_wU×WU_÷õ}WU]÷U×WU]wý×WVUUUUUTUUUUUhªªªªª€";
  draw4dat(x,y,nextar,23,10,col);
  }
