//-------------------------------
// viddrive.hpp                          Generic video driver class.
// Copyright 1994 G. Andrew Stone
// Recreational Software Designs
// NOT PUBLIC DOMAIN!
//-------------------------------

#ifndef VIDEO_DRIVER
#define VIDEO_DRIVER
#include "gen.h"
#include "pal.h"
#include "Pixel.hpp"
#include "Coord2d.hpp"

class VideoMode
  {
  public:
  Coord2d      Size;
  uint         LineSep;
  uchar        ColDepth;
  Pixel       *Address;
  long int     ViewPageOff;
  uint         BIOSMode;
  void         (*SetMode)    (VideoMode *v);
  void         (*Clear)      (Pixel Col,VideoMode *v);
  void         (*Point)      (Coord2d Pos, Pixel col,VideoMode *v);
  Pixel        (*GetCol)     (Coord2d Pos,VideoMode *v);
  void         (*Rep1Col)    (Coord2d Pos,uint Num,Pixel col,VideoMode *v);

  void         (*Rep1ColRev) (Coord2d Pos,uint Num,Pixel col,VideoMode *v);
  void         (*RepCols)    (Coord2d Pos,uint Num,Pixel *cols,VideoMode *v);
  void         (*RepColsRev) (Coord2d Pos,uint Num,Pixel *cols,VideoMode *v);
  int          (*GetCols)    (Coord2d Pos,uint Num,Pixel *cols,VideoMode *v);
  int          (*GetColsRev) (Coord2d Pos,uint Num,Pixel *cols,VideoMode *v);

  void         (*Line)       (Coord2d Start,Coord2d End,Pixel col,VideoMode *v);
  void         (*Box)        (Coord2d Start,Coord2d End,Pixel col,VideoMode *v);
  void         (*BoxFill)    (Coord2d Start,Coord2d End,Pixel col,VideoMode *v);
  void         (*DrawBlock)  (Coord2d Pos,int b,VideoMode *v);
  void         (*BCurve)  (Coord2d p1,Coord2d p2,Coord2d VMid,unsigned char col,VideoMode *v);

  void         (*Spline)  (Coord2d p1,Coord2d p2,Coord2d VMid,unsigned char col,VideoMode *v);
  void         (*SetViewPos) (unsigned long int pos,VideoMode *v);
  void         (*SetWritePage) (unsigned long int pos,VideoMode *v);
  void         (*SetAllPal)  (RGBdata *pal,VideoMode *v);
  void         (*GetAllPal)  (RGBdata *pal,VideoMode *v);

  void         (*SetAllPalTo)(RGBdata *pal,VideoMode *v);
  void         (*SetPal)     (int num,RGBdata  pal,VideoMode *v);

  void         SetStdRoutines(void);
  };

class VideoDriver
  {
  char Ret;                    // The Retf which precedes all drivers
  char ID[3];
  char Object[13];
  int  Version;
  char Date[9];
  char xtra[1];                // Blank so first stuff takes 32 bytes.
  char VGAType[32];
  public:
  char NumModes;
  VideoMode FirstDriver;       // First Video Driver data structure is here.
  VideoMode*GetMode(int num)
    {
    if (num>NumModes) return(False);
    return( (&FirstDriver+num));
    }
  boolean Validate(int MinVersion);
  };

VideoDriver* LoadVideoDriver(char *FileName);
extern "C" void TextMode(void);

#endif
