// Palette manipulation class.
// July 15,94                             Copyright(C) 1994 G. Andrew Stone

#ifndef PALETTEHDR
#define PALETTEHDR
#include "Pixel.hpp"
#include <stdio.h>
#include <bios.h>

class RGBdata
  {
  public:
  uchar red,green,blue;

  void Set(unsigned char r, unsigned char g, unsigned char b)
    { red=r; green=g; blue=b; }

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
  Pixel Match(RGBdata *colors)
    {
    int        min=32000;  // counter to keep track of best fit
    int        sum;        // number to compare to min
    int        r,g,b;      // red green and blue components of the color
    int        newcolor;   // where the number for the closest color is stored.

    for (int k=0;k<256;k++)
      {
      r  =  ((int)(red&63))  - ((int) (colors[k].red&63));
      g  =  ((int)(green&63))- ((int) (colors[k].green&63));
      b  =  ((int)(blue&63)) - ((int) (colors[k].blue&63));
      r *= r;
      g *= g;
      b *= b;
      sum= r+g+b;
      if (sum<min)  { min=sum; newcolor=k; }
      }
    return(newcolor);
    }
  };

class Palette
  {
  public:
  uint    ColsUsed;
  RGBdata Cols[256];
  RGBdata &operator () (int i) { return(Cols[i]); }

  Palette() { ColsUsed=0; }
  Pixel Match(RGBdata Col,int *Error=NULL)
    {
    int        min=32000;  // counter to keep track of best fit
    int        sum;        // number to compare to min
    int        r,g,b;      // red green and blue components of the color
    int        newcolor;   // where the number for the closest color is stored.

    for (int k=0;k<ColsUsed;k++)
      {
      r  =  ((int)(Col.red&63))  - ((int) (Cols[k].red&63));
      g  =  ((int)(Col.green&63))- ((int) (Cols[k].green&63));
      b  =  ((int)(Col.blue&63)) - ((int) (Cols[k].blue&63));
      r *= r;
      g *= g;
      b *= b;
      sum= r+g+b;
      if (sum<min)  { min=sum; newcolor=k; }
      }
    if (Error != NULL) *Error = min;
    return(newcolor);
    }
  };
#endif
