// --------------------------------
//  Copyright 1994 G. Andrew Stone
//  Not public domain!
// --------------------------------

#include "gen.h"
#include <fileclss.hpp>   // <> so the DRV version is found first, as with the Borland include rules
#include "viddrive.hpp"

enum ChunkTypes GM_ENUM16
  {
  ColorMap=11,LineCompress=12,AllBlack=13,RunLength=15,Copy=16
  };

class FLIHeader
  {
  public:
  long FileSize;
  uint     FileID;
  uint     NumFrames;
  uint     Width;
  uint     Height;
  uint     PixDepth;
  int      Flags;   // = 0
  uint     FrameSpd;
  long Next;    // = 0
  long Frit;   // = 0
  uchar    Xtra[102];
  boolean  Validate(void)
    {
    if ((FileID==0xAF11)&&(PixDepth==8)) return(True);
    return(False);
    }
  void Print(void)
    {
    char s[81];
    sprintf(s,"Size:%lu Frames:%d Dim:(%d,%d) ColorDepth:%d Spd:%d\n",FileSize,NumFrames,Width,Height,PixDepth,FrameSpd);
    DEBUG(s);
    }
  };


class FrameHeader
  {
  public:
  long Size;  // In bytes.
  uint     ID;
  uint     NumChunks;
  char     Xtra[8];
  boolean Validate(void);
  boolean View(FLIHeader&,FileRead&,VideoMode *,Coord2d);
  };


class FLI
  {
  public:
  Coord2d      Pos;
  FileRead     File;
  FLIHeader    Head;
  VideoMode    *Vid;
  FrameHeader  CurFrame;
  uint         FrameIdx;
  boolean View(void);
  boolean StepInit(void);
  boolean Step(void);
  };


