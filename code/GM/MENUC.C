#include <bios.h>
#include <string.h>
#include <conio.h>  /* Gotoxy */
#include <time.h>
//#include <ctype.h>

//#include "windio.h"
#include "scrnrout.h"
#include "mousefn.h"
#include "jstick.h"

static int butstat=0;
int windlvl=0;

int  horizmenu(int numitems, int curitem, int y,...); /* ... is x of each item and final x of end */

static void drawcursor(int,int,int,int);
static void erasecursor(void);

#define FOREATTR    14
#define BACKATTR    1
#define MENUATTR    (FOREATTR+(BACKATTR*16))
#define MENUBOX     2
#define SHADATTR    7*16
#define ERRORFORE   12
#define ERRORBACK   4
#define ERRORATTR   (ERRORFORE+(ERRORBACK*16))
#define CURATTR     (7*16)+15
#define BASE        8



#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

int horizmenu(int itemnum,int curitem,int y,...)  /* Dots are start of each item */
  {
  int selected=FALSE;
  int mx,my,mbut;
  int inkey;
  char upkey,lowkey;
  int *lens;
  register int l;        /* Loop variable */
  #ifdef USEJOYSTICK
  int joypos=0,joybut=0;
  #endif  

  lens=&y;
  lens++;    /* Lens now points to the  x coord of the first item */

  if (curitem==0) curitem++;
  curitem--;

  moucur(0);
  setmoupos( (lens[curitem]+lens[curitem+1])/2,y);
  moustats(&mx,&my,&butstat);
  drawcursor(lens[curitem],y,lens[curitem+1]-1,CURATTR);

  do
    {
    if (bioskey(1))
      {
      inkey = bioskey (0);
      lowkey = inkey&255;
      if (lowkey==0)
        {
        upkey = inkey>>8;
        switch (upkey)
          {
          case 77:   /* right arrow */
            erasecursor();
            curitem++; if (curitem>=itemnum) curitem=0;
            setmoupos(lens[curitem+1]-1,y);
            drawcursor(lens[curitem],y,lens[curitem+1]-1,CURATTR);
            break;

          case 75:   /* left arrow */
            erasecursor();
            curitem--; if (curitem<0) curitem=itemnum-1;
            setmoupos(lens[curitem]+1,y);
            drawcursor(lens[curitem],y,lens[curitem+1]-1,CURATTR);
            break;

          case 72:   /* up arrow */
            lowkey = 27;  /* escape is the same as up arrow */
            break;

          case 80:   /* down arrow */
            lowkey = 13;  /* return is the same as dn arrow */
            break;
          }
        }
      if ((lowkey==13)||(lowkey==27)) selected = TRUE;
      }
    moustats(&mx,&my,&mbut);
    for (l=0;l<itemnum;l++)
      if ((lens[l]<=mx)&(lens[l+1]>mx)) { mx=l; l=itemnum+1; }
    if (l==itemnum)
      {
      if (lens[0]>mx) mx=0;
      else if (mx>=lens[itemnum]) mx=itemnum-1;
      }
    if ((mx<itemnum) & (mx != curitem))
      {
      erasecursor();
      curitem=mx;
      drawcursor(lens[curitem],y,lens[curitem+1]-1,CURATTR);
      }
    if (mbut != butstat)
      {
      butstat = mbut;
      selected = TRUE;
      }
    } while (!selected);
  if (lowkey == 27) curitem=0; else curitem++;
  erasecursor();
  return(curitem);
  }

static char oldattr[10];
static char oldx[10];
static char oldy[10];
static char oldx1[10];
static char stackpos=0;

static void drawcursor(int x,int y,int x1,int attr)
  {
  int l=0;

  if (stackpos<10)
    {
    oldattr[stackpos]=((char) (readch(x,y)>>8));
    oldx[stackpos]   =x;
    oldy[stackpos]   =y;
    oldx1[stackpos]  =x1;
    for(l=x;l<=x1;l++) writech(l,y,attr,readch(l,y));
    stackpos++;
    }
  }

static void erasecursor(void)
  {
  int l=0;

  if (stackpos>0)
    {
    stackpos--;
    for(l=oldx[stackpos];l<=oldx1[stackpos];l++)
      writech(l,oldy[stackpos],oldattr[stackpos],readch(l,oldy[stackpos]));
    }
  }
