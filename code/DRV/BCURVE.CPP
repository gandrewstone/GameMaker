#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#define STDVGA
#include "graph.h"
#include "math.h"
#include "pal.h"
#include "Coord2d.hpp"

#define SHOWVECTOR

#ifndef Sign(x)
  #define Sign(x) ((x) > 0 ? 1:  ((x) == 0 ? 0:  (-1)))
#endif
#define Abs(x) ((x) > 0 ? (x):(-1*(x)))

RGBdata cols[256];

typedef struct
  {
  int x,y;
  } PointStruct;

extern "C" void far Point(int x, int y, unsigned char col);
extern int xor;
void SLine(int x1,int y1,int x2,int y2,unsigned char col,int StrangeAdd(void));
void GraphMode(void);
void TextMode(void);
void Curve(Coord2d From,Coord2d To, Coord2d VMid,unsigned char col);
PointStruct e1={1,0}, e2={0,1};
void setrainbow(void);
int Curver(void);


PointStruct a={310,190}, b={10,10};


/* void t1main(void)
  {
  int l=0;
  float rad;

  GraphMode();
  bioskey(0);
  e1.y= (b.y-a.y)/10; //(50.0*sin(rad));
  e1.x= (b.x-a.x)/10; //(50.0*cos(rad));
  for(l=0;l<=360;l+=2)
    {
    //a.x=l+10;
    rad = 6.282/360 * l;
    e1.y=(50.0*sin(rad));
    e1.x=(50.0*cos(rad));
    e2.y=(50.0*sin(3.14/2+rad));
    e2.x=(50.0*cos(3.14/2+rad));

    e2.y=(50.0*sin(3.14/2-rad));
    e2.x=(50.0*cos(3.14/2-rad));

    //Palette(7,50,50,50);
    //Palette(8,31,31,31);

    gotoxy(30,1);
    printf("%4d",l);
    //b.y=l/2;

    Curve(&a,&b,&e1,&e2,l%128);
    delay(10);
    }
  bioskey(0);
  TextMode();
  }
*/

void main(void)
  {
  Coord2d From(10,10),To(10,100);
  Coord2d Mid(100,50);
//  PointStruct b = {100,101};

  GraphMode();
  RainbowCols(cols);
  SetAllPal(cols);

  Curve(From,To,Mid,7);

  From.Set(200,10);
  To.Set(300,100);
  Mid.Set(200,100);
  Curve(From,To,Mid,7);

  bioskey(0);
  TextMode();
  }


void Curve(Coord2d p1,Coord2d p2,Coord2d VMid,unsigned char col)
  {
  int l;
  Coord2d VFrom,VTo,VCur,VBres,VDelta;
  Coord2d Cur,Bres,Old,Delta;
  int Steps,MaxSteps;
  float tf,VMul;

  Line(p1.x,p1.y,VMid.x,VMid.y,22);
  Line(p2.x,p2.y,VMid.x,VMid.y,22);

  VFrom = VMid - p1;              // Create the start direction vector.
  VTo   = p2 - VMid;              // Create the end direction vector.

  Delta = p2-p1;

  // Normalize the vectors.
  tf=((float) (Abs(VFrom.x)+Abs(VTo.x)))/2.0;
  tf=Delta.x/tf;
  tf=Abs(tf);
  VMul    = tf*VTo.x;
  VTo.x   = VMul;
  VMul    = VFrom.x*tf;
  VFrom.x = VMul;

  tf=((float) (Abs(VFrom.y)+Abs(VTo.y)))/2.0;
  tf=Delta.y/tf;
  tf=Abs(tf);
  VMul    = tf*VTo.y;
  VTo.y   = VMul;
  VMul    = VFrom.y*tf;
  VFrom.y = VMul;


  // Set up Bresenham's to convert 1 vector to the other.
  VDelta = VTo-VFrom;
  VCur   = VFrom;
  Cur    = p1;
  VBres.Set(0,0);
  Bres.Set(0,0);

  MaxSteps= (Abs(Delta.x)+Abs(Delta.y))/30;

  for (Steps=0;Steps<MaxSteps;Steps++)
    {
    //bioskey(0);
    VBres += AbsC(VDelta);
    while (VBres.x>=MaxSteps) { VBres.x -= MaxSteps; VCur.x+=Sign(VDelta.x); }
    while (VBres.y>=MaxSteps) { VBres.y -= MaxSteps; VCur.y+=Sign(VDelta.y); }

    Old=Cur;

    Bres += AbsC(VCur);
    while (Bres.x>=MaxSteps) { Bres.x -= MaxSteps; Cur.x+=Sign(Delta.x)*Sign(VCur.x); }
    while (Bres.y>=MaxSteps) { Bres.y -= MaxSteps; Cur.y+=Sign(Delta.y)*Sign(VCur.y); }

    Line(Old.x,Old.y,Cur.x,Cur.y,col++);
    Line(Old.x,Old.y,Old.x+VCur.x/10,Old.y+VCur.y/10,70);
    }
  }



void SLine(int x1,int y1,int x2,int y2,unsigned char col,int StrangeAdd(void))
  {
  register int l,l1;
  int dx=0,dy=0, sdx=0,sdy=0, absdx=0, absdy=0;
  int old;
  int x=0,y=0;
  int less,great;

  dx = x2-x1;
  dy = y2-y1;
  sdx=Sign(dx);
  sdy=Sign(dy);
  absdx=sdx*dx;
  absdy=sdy*dy;

  //Point(x1,y1,col);

  if (absdx>=absdy)
    {
    old=x1;
    for (l=0; l<absdx; l++)
      {
      y += absdy;
      x1 += sdx;
      if (y>=absdx)
        {
        y-=absdx;
        y1 += sdy;
        x1 += StrangeAdd();
        if (x1<old) { less=x1;great=old; }
        else { less=old;great=x1; }
        for(l1=less;l1<=great;l1++) Point(l1,y1,col);
        old=x1;
        }
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
        x1 += (sdx+StrangeAdd());
        }
      y1 +=sdy;
      Point (x1,y1,col);
      }
    }
  }


void GraphMode(void)
  {
  union REGS r;
  int m;

  r.h.ah = 0;
  r.h.al = GMODE;
  int86(0x10,&r,&r);
  }

void TextMode(void)
  {
  union REGS r;
  int m;
  
  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);
  }

/*
int BresTime=0;
int CurTime=0;
int StartAdd;
int CurAdd=0;
int ctr,cdir;
int Curver(void)
  {
  ctr+=StartAdd;
  if (ctr>BresTime) { ctr-=BresTime; CurAdd+=cdir; }
  return(CurAdd);
  }


void Curve1(PointStruct *e1,PointStruct *mid, PointStruct *e2,unsigned char col)
  {
  BresTime=(mid->y - e1->y);

  StartAdd=41;
  CurAdd=-20;
  ctr=0;
  cdir=1;
  SLine(mid->x,mid->y,e1->x,e1->y,col,Curver);
 
  StartAdd=41;
  CurAdd=20;
  ctr=0;
  cdir=-1;
  SLine(mid->x,mid->y,e2->x,e2->y,col,Curver);
  }
*/

