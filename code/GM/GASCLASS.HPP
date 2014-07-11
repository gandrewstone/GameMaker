//---------------------------------------------------------------------------
// gasclass.hpp                Programmer: Andy Stone   Created:July 10,1992-
//      This header file contains all of the graphics classes:              -
//                      1. GASClass                                         -
//                      2. PctClass                                         -
//---------------------------------------------------------------------------

#ifndef GASCLASS
#define GASCLASS
//#include <stdio.h>
#include "coord2d.hpp"

#define  GPFNEWLINE 0
#define  GPFSKIP    1
#define  GPFDRAW    2
#define  GPFDONE    255

#define INBOXRULE       1

char CursorRulesCheck(NodeClass *node);

// GASClass should be a binary tree of possible cursor changes depending
// upon certain actions - key, Mouse button, Jstick button (up,down),
// cursor in a box, not in a box, cursor movement in x,y direction (pick nearest)
// upon linking with an action, the GASnode *cur will change to the GASnode
// pointed to by the action.  This will allow the creation of a complex network
// of animation possibilities.


class GPFClass    // Graphic Picture Format
  {
  unsigned char *PicInfo;
  public:
  GPFClass::GPFClass()
    {
#ifdef DEBUG
    NewAbortMSG("GM:GPFClass Init.\n");
#endif
    PicInfo=NULL;
    }
  void          Get(int x, int y, int x1, int y1, unsigned int clear, DrawingBoard *w);
  void          Get(FILE *fp);
  void          Get(uchar *PicData) { PicInfo = PicData; }
  void          Save(FILE *fp);
  unsigned char *PicPtr(void) { return (PicInfo); }
  void          Draw(Coord2d Pos, DrawingBoard *w, GPFClass *Save);
  void          Draw(Coord2d Pos, DrawingBoard *w);
  unsigned int  Length(void);
  };

class CursorClass
  {
  public:
  DrawingBoard         *Scrn;
  GPFClass             *Behind;
  GPFClass             *CurGPF;
  LinkedListClass       Rules;
  int                   Moving;
  char                  IsOn;
  char                  Show;
  Coord2d               Hot;
  Coord2d               Pos;
  Coord2d               Old;
  Box2d                 Limits;
  Coord2d               Size;

                        CursorClass(DrawingBoard *Scrn);
                        ~CursorClass();
  void                  SetHotSpot(int x,int y) {  Hot.x=x; Hot.y=y; }
  void                  Forget(void) { Show=FALSE; }
  void                  NewGPF(GPFClass *NewOne);
  void                  On(void);
  void                  Off(void);
  void                  Goto(int x,int y);     
  void                  Move(int dx,int dy);
  void                  Limit(int x,int y,int x1,int y1);
  void                  AddRule(int x,int y,int x1,int y1, GPFClass *Pic);
  friend void           CursorClassDraw(void *d1);
  friend char           CursorRulesCheck(NodeClass *node);
  };


#endif
