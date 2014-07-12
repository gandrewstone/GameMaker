// Copyright 1994 G. Andrew Stone
// Recreational Software Designs
// NOT PUBLIC DOMAIN!
//----------------------------------------------------------------------

#include <stdio.h>
#include "Viddrive.hpp"
#include "xmsvid.hpp"

//inline ulongi CalcAddress (int &x,int &y,VideoMode &v)

#define CalcAddress(p,v) ((p.y*((v)->LineSep))+p.x)

static void  XMSPoint  (Coord2d Pos, Pixel col,VideoMode *v);
static Pixel XMSGetCol (Coord2d Pos,VideoMode *v);
static void  XMSRepCols(Coord2d Pos,uint Num,Pixel *cols,VideoMode *v);
static int   XMSGetCols(Coord2d Pos,uint Num,Pixel *cols,VideoMode *v);
static void  XMSSetMode(VideoMode *Vid);

boolean XMSVideoInit(XMSVideoMode *x)
  {
  ulongi Siz;
  uchar error;

  Siz = ((ulongi)x->LineSep)*((ulongi)x->Size.y);
  printf("BufferSize:%lu ",Siz);
  error=x->Get.Src.Alloc(Siz);
  x->Get.Src.Offset=0;
  x->Set.Dst=x->Get.Src;
  x->Address = (void far *) x->Set.Dst.Handle;   // All I really need.
  printf("Alloc: %x Error:%x",x->Set.Dst.Handle,error);

  x->SetMode= XMSSetMode;
  x->Point  = XMSPoint;
  x->GetCol = XMSGetCol;
  x->RepCols= XMSRepCols;
  x->GetCols= XMSGetCols;
  return(True);
  }

void XMSVideoUnInit(XMSVideoMode *x)
  {
  x->Set.Dst.Free();
  }


static void XMSSetMode(VideoMode *Vid)
  {
  }

static void XMSPoint(Coord2d Pos, Pixel col,VideoMode *v)
  {
  uint Buf;
  uchar error;
  XMSVideoMode   *x = ((XMSVideoMode*)v);
  x->Set.Dst.Offset = CalcAddress(Pos,v);
  x->Set.Src.Handle = 0;
  x->Set.Src.Offset = (ulongi) &Buf;
  x->Set.XferLen    = 2;
  error=x->Set.Go();
  printf("Point:%x ",x->Set.error);
  }

static Pixel XMSGetCol(Coord2d Pos,VideoMode *v)
  {
  Pixel col[2];
  uchar error;
  XMSVideoMode   *x = ((XMSVideoMode*)v);
  x->Get.Src.Offset = (ulongi) CalcAddress(Pos,v);
  printf("Src %x:%x",x->Get.Src.Handle,x->Get.Src.Offset);
  x->Get.Dst.Handle = 0;
  x->Get.Dst.Offset = (ulongi) col;
  x->Get.XferLen    = 2;
  error=x->Get.Go();
  printf("GetCol:%x\n",x->Get.error);
  return(col[0]);
  }



static void XMSRepCols(Coord2d Pos,uint Num,Pixel *cols,VideoMode *v)
  {
  XMSVideoMode   *x = ((XMSVideoMode*)v);
  x->Set.Dst.Offset = CalcAddress(Pos,v);
  x->Set.Src.Handle = 0;
  x->Set.Src.Offset = (ulongi) cols;
  x->Set.XferLen    = Num;
  x->Set.Go();
  }

static int XMSGetCols(Coord2d Pos,uint Num,Pixel *cols,VideoMode *v)
  {
  uchar error;
  XMSVideoMode   *x = ((XMSVideoMode*)v);

  x->Get.Src.Offset = CalcAddress(Pos,v);
  x->Get.Dst.Handle = 0;
  x->Get.Dst.Offset = (ulongi) cols;
  x->Get.XferLen    = Num;
  error=x->Get.Go();
  printf("Error:%x\n",error);
  return(Num);
  }

