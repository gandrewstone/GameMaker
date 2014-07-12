#ifndef BITMAPCLASS
#define BITMAPCLASS

#include "viddrive.hpp"

class BitMap
  {
  public:
  Coord2d      Size;
  uint         LineSep;
  uchar        ColDepth;
  Pixel       *Address;

  BitMap(Coord2d Siz)
    {
    Size     = Siz+1;
    LineSep  = Size.x;
    ColDepth = 8;
    Address  = new Pixel [(LineSep*Size.y)+Size.x];
    }

  void UnInit(void)
    {
    Size.Set(0,0);
    LineSep  =0;
    ColDepth =0;
    if (Address!=NULL) { delete Address; Address=NULL; }
    }

  void         Draw (Coord2d Pos,VideoMode *v);
  void         Get  (Coord2d Start,VideoMode *v);
  void         Draw (Coord2d Start,Box2d Mask,VideoMode *v);
  };
#endif

