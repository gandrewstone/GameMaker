//-----------------------------------------------------------------------
// timer.hpp      GAME-MAKER  V2.0                By Andy Stone         |
// Created: June 1,1992                                                 |
//                                                                      |
// This file steals the timer, and allows the user to set time based    |
// function calls.                                                      |
//-----------------------------------------------------------------------

#ifndef TIMERCLASS
#define TIMERCLASS
#include "gen.h"
#include "genclass.hpp"
extern char CallEm(NodeClass *CursorNode);

#define TIMERSTKLEN     2000
extern unsigned int far MyDS; 
extern unsigned long int far Clock;
extern unsigned long int far OldTimer;

class TimerClass:public LinkedListClass
  {
  public:
  char In;
  unsigned long int OldClock;
  friend void interrupt NewTimer(...);
  void AddFn(void Function(void *ptrtodata),void *data); //,int freq=1); //add in freq functality later
  void RemFn(void Function(void *ptrtodata),void *data,int freq=1);
  void TurnOff(void);
  void TurnOn(void);
  };
#endif
