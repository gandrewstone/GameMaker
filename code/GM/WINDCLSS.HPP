//---------------------------------------------------------------------------
// windclss.hpp                Programmer: Andy Stone   Created:July 10,1992-
//      This header file contains all of the windowing classes:             -
//                      1. WindowClass                                      -
//                      2. DrawingBoardClass                                -
//---------------------------------------------------------------------------
//#define sign(x) ((x) > 0 ? 1:  ((x) == 0 ? 0:  (-1)))

#ifndef WINDOWCLASS
#define WINDOWCLASS
void swap(int &x, int &y);

struct DrawingBoard
  {
  void          *WindAddr;
  int           MaxX;
  int           MaxY;
  int           XSkip;
  void          (*Point)  (unsigned int x, unsigned int y, unsigned int col, DrawingBoard *d);
  unsigned int  (*GetCol) (unsigned int x, unsigned int y,DrawingBoard *d);
  void          (*Rep1Col) (unsigned int x,unsigned int y,unsigned int len, unsigned int col,DrawingBoard *d);
  void          (*RepCols)(unsigned int x,unsigned int y,unsigned int len, unsigned char *from,DrawingBoard *d);
  void          (*GetCols)(unsigned int x,unsigned int y,unsigned int len, unsigned char *to,DrawingBoard *d);      
  void          (*SetMode)(void);
  };

void BoxFill(int x,int y,int x1,int y1,unsigned int col,DrawingBoard *d);
void Box(int x,int y,int x1,int y1,unsigned int col,DrawingBoard *d);
void Line(int x1,int y1,int x2,int y2,unsigned int col,DrawingBoard *d);
void          Low256Point  (unsigned int x,unsigned int y,unsigned int col,DrawingBoard *d);
unsigned int  Low256GetCol (unsigned int x,unsigned int y,DrawingBoard *d);
extern "C" void Low256Rep1Col(unsigned int x, unsigned int y, unsigned int len, unsigned int col, DrawingBoard *d);
extern "C" void Low256RepCols(unsigned int x, unsigned int y, unsigned int len, unsigned char *col, DrawingBoard *d);
extern "C" void Low256GetCols(unsigned int x, unsigned int y, unsigned int len, unsigned char *col, DrawingBoard *d);
#endif
