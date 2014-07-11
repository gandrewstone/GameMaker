//-----------------------------------------------------------------------
// geninput.hpp      GAME-MAKER  V2.0                By Andy Stone      |
// Created: June 1,1992                                                 |
//                                                                      |
// This file deals with all keyboard,mouse, and joystick input, creating|
// a unified input class that is easy to deal with.                     |
//-----------------------------------------------------------------------

#ifndef GENINPUT
#define GENINPUT

#define ERROR 2                 // Define error constant

#include <dos.h>
#include <stdio.h>
#include "gen.h"
#include "coord2d.hpp"
#include "jstick.h"
#include "genclass.hpp"
#include "timer.hpp"
#include "windclss.hpp"
#include "gasclass.hpp"

extern "C" void MouseInterrupt(void);

char MoveCursors(NodeClass *CursorNode);


// Hierarchy    Cursor Cursor                     
//                |   X   |       \
//              Mouse    Joystick keyboard
// Mouse Jstick, kbd has a linked list of all the cursors it needs to change.
// Calls each cursor fn - either with Offsets or Screen locations.
// Traditional mouse stuff, like moucur, and stats, should be handled
// through the cursor.

class JoyStickClass
  {
  unsigned int CenterX,CenterY;
  unsigned int LeastX,LeastY;
  unsigned int MostX,MostY;
  unsigned int RangeX,RangeY;
  unsigned int OldButs;
  char On;
  friend void CheckJoyStick(void *t);
  LinkedListClass Cursors;

  public:
  void AddCursor(CursorClass *Cur);
  char Has;
  char Init(void);
  void TurnOn(void)  { if (!On) On=TRUE; }
  void TurnOff(void) { if (On) On=FALSE; }
  };

/*  
class KeyBoardClass
  {
  Init(void);
  char   
  }
*/

class MouseClass
  {
  int MickeyRatioX,MickeyRatioY;
  int OldMickeyX,OldMickeyY;
  void far *oldinterrupt;
  void Change(void);
  LinkedListClass Cursors;

  public:

  char          Has;
                ~MouseClass(void);   // Destructor  
  friend void   MouseMoved(void);
  char          Init(void);
  void          AddCursor(CursorClass *Cur);
//  char          RemCursor(CursorClass *Cur);
  void          SetMickeyRatio(int NewX,int NewY);
  };

class InputClass
  {
  public:
  MouseClass    m;
  JoyStickClass j;
//  KeyBoardClass k;
  void Init(void);
  };

extern unsigned char GPFofArrow[];
extern unsigned char GPFofNullCursor[];

#endif
