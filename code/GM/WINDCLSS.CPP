#include "gen.h"
#include "genclass.hpp"
#include "timer.hpp"
#include "windclss.hpp"

void Low256Point  (unsigned int x,unsigned int y,unsigned int col,DrawingBoard *d)
  {
  if ((x<d->MaxX)&&(y<d->MaxY))
    *( ((unsigned char *) d->WindAddr)+(y*d->XSkip)+x)=((unsigned char) col);
  }

unsigned int Low256GetCol (unsigned int x,unsigned int y,DrawingBoard *d)
  {
  if ((x<d->MaxX)&&(y<d->MaxY))
    return (*( ((unsigned char *) d->WindAddr)+(y*d->XSkip)+x));
  else return(0xFFFF);
  }

void Box(int x,int y,int x1,int y1,unsigned int col,DrawingBoard *d)
  {
  if (x>x1) swap (x,x1);
  if (y>y1) swap (y,y1);
  Line(x,y,x1,y,col,d);
  Line(x1,y,x1,y1,col,d);
  Line(x1,y1,x,y1,col,d);
  Line(x,y1,x,y,col,d);
  }

void BoxFill(int x,int y,int x1,int y1,unsigned int col,DrawingBoard *d)
  {
  register int loop,temp;
  if (x>x1) swap (x,x1);
  if (y>y1) swap (y,y1);
  temp=x1-x+1;
  for(loop=y;loop<=y1;loop++) (*(d->Rep1Col))(x,loop,temp,col,d);
  }

void Line(int x1,int y1,int x2,int y2,unsigned int col,DrawingBoard *d)
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
  (*(d->Point)) (x1,y1,col,d);

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
      (*(d->Point)) (x1,y1,col,d);
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
      (*(d->Point)) (x1,y1,col,d);
      }
    }
  }

/*                       End of Function Line                            */
/*----------------------------------------------------------------------*/


void swap(int &a, int &b)
  {
  int temp;
  temp = a;
  a = b;
  b = temp;
  }

void Low256GraphMode() //DrawingBoard *d)
  {
  asm   mov ax,13h
  asm   int 10h
//  CurMode=GMODE
  }

/* void TextMode(void)
  {
  asm   mov ax,TMODE
  asm   int 10h
  CurMode=TMODE;
  }
*/

