/*----------------Block specific routines------------------------------*/
/* Andy Stone + BRO      Feb 24,1991                                   */
/* Last Edited: 6/6/91                                                 */

#include <stdio.h>
#include <string.h>
#include <dos.h>
#include "gen.h"
#include "gmgen.h"
#include "graph.h"
#include "scrnrout.h"
#include "mousefn.h"
#include "windio.h"
#include "bloc.h"

extern unsigned long int far Clock;

int lastbl=BACKBL;
char wrap =1;

blkstruct far *blk=NULL;
char BlkExtensions[][7]={"*.bbl\0","*.mbl\0","*.cbl\0"};
char curfil[MAXFILENAMELEN]="";


int getblktype(void)
  {
  int attr;
  char w[1000];
  int btype=1; 

  attr=openmenu(10,10,20,4,w);
  writestr(10,10,attr+15,"Background Block");
  writestr(10,11,attr+15,"Monster Block   ");
  writestr(10,12,attr+15,"Character Block ");
  writestr(10,13,attr+15,"Menu            ");
  btype=vertmenu(10,10,20,4,btype);
  closemenu(10,10,20,4,w);
  if ((btype==4)|(btype==0)) btype = FALSE;
  return(btype);
  }

int loadblks(int btyp,char *buffer)
  {
  FILE *fp;
  char fname[MAXFILENAMELEN]="";


  switch (btyp)
    {
    case 1:
      if (!getfname(5,7,"Enter block file to load:",BlkExtensions[btyp-1],fname)) return(FALSE);
      lastbl=BACKBL;
      break;
    case 2:
      if (!getfname(5,7,"Enter block file to load:",BlkExtensions[btyp-1],fname)) return(FALSE);
      lastbl=MONBL;      
      break;
    case 3:
      if (!getfname(5,7,"Enter block file to load:",BlkExtensions[btyp-1],fname)) return(FALSE);
      lastbl=CHARBL;
      break;
    default:
      return(FALSE);
    }
  if ((fp=fopen(fname,"rb"))==NULL) return(False);
  fread((unsigned char *)buffer,sizeof(blkstruct),lastbl,fp); 
  fclose(fp);
  strcpy((char*)curfil,(char*)fname);
  return(TRUE);
  }

int saveblks(int btype,char *buffer)
  {
  FILE *fp;

#ifdef CRIPPLEWARE
  StdCrippleMsg();
#else
  if (btype==1) lastbl=BACKBL;
  if (btype==2) lastbl=MONBL;
  if (btype==3) lastbl=CHARBL;
  if ( (fp=GetSaveFile("Save blocks as:",&BlkExtensions[btype-1][1],WorkDir,curfil))==NULL) return(False);
  fwrite((unsigned char *)buffer,sizeof(blkstruct),lastbl,fp);
  fclose(fp);
#endif
  return(TRUE);
  }


void scroll_list(int dir,int x,int y,int listlen,int totalen,int *current,int (*dra)(int))
  {
  static unsigned int Rate=0;
  unsigned long int OldClock;
  unsigned int RotCnt=0;
  register int i=0;
  int l1,draw;
  int x1 = (x+ (listlen*BLEN));
  int movx,movy,button,oldx,oldy;

  do
    {
    delay(50);   /* wait 1/5 of a second so a single click will move one block */
    moustats(&movx,&movy,&button);
    } while ( (i++<4)&&(button>0) );

  while ( BLEN % (Sign(dir)*dir) != 0) dir+= Sign(dir);  // make sure dir
                                            // is an even multiple of BLEN

  moustats(&movx,&movy,&button);
  oldx=movx; oldy=movy;
  if (i<4) oldx=0;
  
  OldClock=Clock;
  do
    {
    l1 = (BLEN-dir)%BLEN;       /* l1 is + amnt of blk to left of scroll bar */
    if (dir>0)
      {
      *current = (*current-1+totalen) % totalen;
      if ( (*current==0)&&(wrap==0) ) oldx=0;
      }  /* quit scrolling if at position 0 and 'wrap' is not 1 (on)  */
    while ( (l1<=BLEN) && (l1>=0) )
      {
      draw = (*dra)(*current);
      drawblocpiece(x,y,l1,0,BLEN-l1,BLEN,draw);
      for (i=1;i<listlen;i++)
        { 
        draw = (*dra)((*current+i)%totalen);
        drawblk(x+(i*BLEN)-l1,y,&blk[draw].p[0][0]);
        }
      draw = (*dra)((*current+listlen)%totalen);
      drawblocpiece(x1-l1,y,0,0,l1,BLEN,draw);
      l1-=dir;
      delay(Rate);
      }
    if (dir < 0)
      {
      *current = (*current+1) % totalen;
      if ( (*current==(totalen-listlen)) && (wrap==0) ) oldx=0;
      }

    RotCnt++;        // Speed checker - slow down if scroll is too fast!
                     // Using 3 / 2 is OK too, but doc already written.
    if (RotCnt>10)
      {
      if (Clock-OldClock<7) Rate++;
      RotCnt=0;
      OldClock=Clock;
      }

    moustats(&movx,&movy,&button);
    } while ( (button>0)&&(movx==oldx)&&(movy==oldy) );
  }

void drawblocpiece(int x,int y, int bx,int by,int len, int wid, int bloc)
  {
  register int lx,ly;

  for (ly=0;ly<wid;ly++)
    for (lx=0;lx<len;lx++)
      Point (lx+x,ly+y,blk[bloc].p[(ly+by)%BLEN][(lx+bx)%BLEN]);
      
  }

void drawlist(int x,int y,int listlen,int totalen,int *current,int (*dra)(int))
 {
  register int i;
  int draw;
 
  for (i=0;i<listlen;i++)
   { 
    draw = (*dra)((*current+i)%totalen);
    drawblk(x+(i*BLEN),y,&blk[draw].p[0][0]);
   }
 }

void vscroll_list(int dir,int x,int y,int listlen,int totalen,int *current,int (*dra)(int))
 {
  static unsigned int Rate=0;
  unsigned long int OldClock;
  unsigned int RotCnt=0;
  register int i=0;
  int l1,draw;
  int y1 = (y+ (listlen*BLEN));
  int movx,movy,button,oldx,oldy;

  do {
    delay(50);   /* wait 1/5 of a second so a single click will move one block */
    moustats(&movx,&movy,&button);
  } while ( (i++<4)&&(button>0) );

    if ( (dir>BLEN)|(dir<-BLEN) ) dir = Sign(dir)*BLEN;
    while ( BLEN % (Sign(dir)*dir) != 0) dir+= Sign(dir);  // make sure dir
                                              // is an even multiple of BLEN
  moustats(&movx,&movy,&button);
  oldx=movx; oldy=movy;
  if (i<4) oldx=0;
  
  OldClock=Clock;
  do
    {
    l1 = (BLEN-dir)%BLEN;       /* l1 is + amnt of blk above of scroll bar */
    if (dir>0)
      {
      *current = (*current-1+totalen) % totalen;
      if ( (*current==0)&&(wrap==0) ) oldx=0;
      }                                 /* quit scrolling if at position 0 */
    while ( (l1<=BLEN) && (l1>=0) )
      {
      draw = (*dra)(*current);
      drawblocpiece(x,y,0,l1,BLEN,BLEN-l1,draw);
      for (i=1;i<listlen;i++)
        {
        draw = (*dra)((*current+i)%totalen);
        drawblk(x,y+(i*BLEN)-l1,&blk[draw].p[0][0]);
        }
      draw = (*dra)((*current+listlen)%totalen);
      drawblocpiece(x,y1-l1,0,0,BLEN,l1,draw);
      l1-=dir;
      delay(Rate);
      }
    if (dir < 0)
      {
      *current = (*current+1) % totalen;
      if ( (*current==(totalen-listlen)) && (wrap==0) ) oldx=0;
      }

    RotCnt++;        // Speed checker - slow down if scroll is too fast!
    if (RotCnt>5)
      {
      if (Clock-OldClock<3) Rate++;
      RotCnt=0;
      OldClock=Clock;
      }
    moustats(&movx,&movy,&button);
    } while ( (button>0)&&(movx==oldx)&&(movy==oldy) );
  }

void vdrawlist(int x,int y,int listlen,int totalen,int *current,int (*dra)(int))
 {
  register int i;
  int draw;
 
  for (i=0;i<listlen;i++)
   { 
    draw = (*dra)((*current+i)%totalen);
    drawblk(x,y+(i*BLEN),&blk[draw].p[0][0]);
   }
 }
