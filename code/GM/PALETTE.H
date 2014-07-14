#ifndef PAL_H
#define PAL_H
// Palette manipulation routines - source in palc.c, pala.asm
#include "gen.h"
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


#define COL0 33,33,33                           //light grey
#define COL1 43, 0, 0                           //med. red
#define COL2 28, 0, 0                           //dark red
#define COL3  0, 0, 0                           //black
#define COL4 28,28,28                           //mezzo light grey
#define COL5 23,23,23                           //mezzo dark grey
#define COL6 18,18,18                           //dark grey
#define COL7 48, 0, 0                           //bright red
#define COL8  8,48,24                           //green

#ifndef Sign(x)
  #define Sign(x) ((x) > 0 ? 1:  ((x) == 0 ? 0:  (-1)))
#endif

extern "C"
  {
  void GetAllPal(RGBdata *pal);
  void Palette(unsigned char col,unsigned char red,unsigned char green,unsigned char blue);
  void SetAllPal(RGBdata *pal);
  void SetAllPalTo(RGBdata *pal);
  }

char FindForCol(RGBdata *colors, char bkcol,char choice);   //Phase out!!!
void FadeTo(RGBdata *dest,int FadeTime=65);
void FadeAllTo(RGBdata dest,int FadeTime=65);  // Fade completely to one color.
unsigned char PalFindCol(unsigned char red, unsigned char green, unsigned char blue,RGBdata *colors);
void RainbowCols(RGBdata *colors);
void RandCols(RGBdata *colors);
void insertcol(int number,unsigned char red,unsigned char green,unsigned char blue,RGBdata *colors);
void InitStartCols(RGBdata *colors);

#endif
