/*--------------------------------------------------------------*/
/* Graphics basic routines -- source in graphc.c and gena.asm   */
/*--------------------------------------------------------------*/

#ifndef GRAPHICS
#define GRAPHICS

#include "pixel.hpp"

#define BYTESPERCHAR 8
#ifndef Sign(x)
  #define Sign(x) ((x) > 0 ? 1:  ((x) == 0 ? 0:  (-1)))
#endif

#define SIZEX 320               // Size of graphics mode #0x13 in pixels
#define SIZEY 200               // Size of graphics mode #0x13 in pixels
#define GMODE 0x13
#define TMODE 0x3

extern void GraphMode(void);
extern void TextMode(void);
extern void SetTextColors(void);

extern void Line(int x1, int y1, int x2, int y2, unsigned char col);
extern void Box(int x, int y, int x1, int y1, unsigned char col);
extern void GPutChar(int initx,int y,unsigned char col,char c);
extern void GWrite(int initx,int y,unsigned char col, char *str);
extern int  GGet(int x,int y,Pixel col,Pixel bcol,char *ans,int maxlen);
extern void draw4dat(int x,int y, char *pict, int len,int wid,unsigned char *col, int rotation=0);

extern "C" void far BoxFill(int x,int y, int x1, int y1, unsigned char col);
extern "C" char far *GetROMFont(void);
#ifndef SVGA
extern "C" void far Point(int x, int y, unsigned char col);
extern "C" int  far GetCol(int x,int y);
#endif

#endif
