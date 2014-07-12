// Palette manipulation routines - source in palc.c, pala.asm

#include "Pixel.hpp"

#ifndef PALETTEHDR
class RGBdata
  {
  public:
  uchar red,green,blue;

  void Set(unsigned char r, unsigned char g, unsigned char b)
    { red=r&63; green=g&63; blue=b&63; }

  RGBdata() { Set(0,0,0); }
  RGBdata(uchar r, uchar g, uchar b) { Set(r,g,b); }

  RGBdata operator >>= (uchar shift)
    {
    red>>=shift; green>>=shift; blue>>=shift;
    red&=63; green&=63; blue&=63;
    return (*this);
    }
  RGBdata operator + (RGBdata add)
    {
    RGBdata ret;
    ret.Set(red+add.red,green+add.green,blue+add.blue);
    return ret;
    }
  friend RGBdata operator / (RGBdata col,int amt)
    {
    RGBdata ret;
    ret.Set(col.red/amt,col.green/amt,col.blue/amt);
    return ret;
    }
  Pixel Match(RGBdata *colors);
  };
#endif

