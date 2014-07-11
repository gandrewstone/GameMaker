/*--------------------------------------------------------------*/
/* Graphics--basic routines                                     */
/*--------------------------------------------------------------*/

#include <bios.h>
#include <dos.h>
#include "gen.h"
#include "gmgen.h"
#include "graph.h"

char *FontPtr;
unsigned int zeroaddon=0;
unsigned int xor=0;

/*-----------------------BASIC GRAPHICS FUNCTIONS--------------------*/
void GWrite(int initx,int y,unsigned char col, char *str)
  {
  int x;
  x=initx;
  while (*str!=0)
    {
    switch (*str)
      {
      case 10:
        y+=8;
        x=initx;
        break;
      default:
        GPutChar(x,y,col,*str);
        x+=8;
        break;
      }
    *str++;
    }
  }

void GPutChar(int initx,int y,unsigned char col,char c)
  {
  int x;
  unsigned char loop,loop1;
  unsigned char mask;
  char *Let;

  Let=FontPtr+(BYTESPERCHAR*c);

  for (loop=0;loop<8;loop++,Let++)  // Do 8 lines
    {
    x=initx;
    mask=0x80;
    for (loop1=0;loop1<8;loop1++)
      {
      if ((*Let)&mask) Point(x,y,col);
      mask >>=1;
      x++;
      }
    y++;
    }
  }


int GGet(int x,int y,unsigned char col,unsigned char bcol,char *ans,int maxlen)
  {
  int  key;
  char ctr=-1;
  char done=-2;
  int  tctr=0;

  while (ans[++ctr]!=0);
  GWrite(x,y,col,ans);
  do
    {
    tctr=401;
    while(!bioskey(1))
      {
      if (tctr>400)
        {
        if (ctr<maxlen) Box(x+ctr*8,y,x+(ctr+1)*8-2,y+7,col);
        else Line(x+(ctr*8)-1,y,x+(ctr*8)-1,y+7,col);
        }
      delay(1);
      tctr++;
      if (tctr>900)
        {
        tctr=0;
        if (ctr<maxlen) Box(x+ctr*8,y,x+(ctr+1)*8-2,y+7,bcol);
        else Line(x+(ctr*8)-1,y,x+(ctr*8)-1,y+7,bcol);
        }
      }
    if (ctr<maxlen) Box(x+ctr*8,y,x+(ctr+1)*8-2,y+7,bcol);
    else Line(x+(ctr*8)-1,y,x+(ctr*8)-1,y+7,bcol);
    key=bioskey(0);
    switch (key&255)
      {
      case 0:
        switch(key>>8)
          {
          case LEFTKEY:
            if (ctr>0)
              {
              ctr--;
              BoxFill(x+ctr*8,y,x+(ctr+1)*8-1,y+7,bcol);
              ans[ctr]=0;
              }
            break;
          case RIGHTKEY:
            if (ans[ctr]!=0) ctr++;
            break;
          }
        break;
      case 27:
        done=-1;
        break;
      case 13:
        done=ctr;
        break;
      case 8:
          if (ctr>0)
            {
            ctr--;
            BoxFill(x+ctr*8,y,x+(ctr+1)*8-1,y+7,bcol);
            ans[ctr]=0;
            }
        break;
      default:
          if (ctr<maxlen)
            {
            ans[ctr] = key&255;
            ctr++;
            ans[ctr]=0;
            GWrite(x,y,col,&(ans[0]));
            }
        break;
      }
    } while (done ==-2);
  return(done);
  }

void Box(int x,int y,int x1,int y1,unsigned char col)
  {
  Line(x,y,x1,y,col);
  Line(x1,y,x1,y1,col);
  Line(x1,y1,x,y1,col);
  Line(x,y1,x,y,col);
  }

/*                       End of Function box                           */
/*---------------------------------------------------------------------*/
/*     Draws a line from (x,y) to (x1,y1).  HIGH LEVEL                  */


void Line(int x1,int y1,int x2,int y2,unsigned char col)
  {
  register int l;
  int dx=0,dy=0, sdx=0,sdy=0, absdx=0, absdy=0;
  int x=0,y=0;

  dx = x2-x1;
  dy = y2-y1;
  sdx=Sign(dx);
  sdy=Sign(dy);
  absdx=sdx*dx;
  absdy=sdy*dy;
  Point(x1,y1,col);

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
      Point(x1,y1,col);
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
      Point (x1,y1,col);
      }
    }
  }

/*                       End of Function Line                            */
/*----------------------------------------------------------------------*/


void draw4dat(int x,int y, char *pict, int len,int wid,unsigned char *col, int rotation)
  {
  int x1=0,y1=0;
  int loop;
  int temp=0;
  int pindex=0;
  int xadd,yadd;

  switch(rotation)
    {
    case 0:
      break;
    case 1:
      x1=len;
      break;
    case 2:
      x1=len;
      y1=wid;
      break;
    case 3:
      y1=wid;
      break;
    }

  do
    {
    for (loop=0;loop<4;loop++)
      {
      temp=pict[pindex];
      temp >>= (3-loop)*2;
      temp &=3;
      if (!((rotation&8)&&(temp==0)))
      if (*(col+temp) != 255) Point(x+x1,y+y1,*(col+temp));
      switch(rotation&3)
        {
        default:
        case 0:
          x1++;
          if (x1>len) { x1=0; y1++; if (y1>wid) loop=5; }
          break;
        case 1:
          y1++;
          if (y1>len) { y1=0; x1--; if (x1<0) loop=5; }
          break;
        case 2:
          x1--;
          if (x1<0) { x1=len; y1--; if (y1<0) loop=5; }
          break;
        case 3:
          y1--;
          if (y1<0) { y1=len; x1++; if (x1>wid) loop=5; }
          break;
        }
      }
    pindex++;
    } while (loop<5);
  }

