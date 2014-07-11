
#include <dos.h>
#include "gen.h"
#include "timer.hpp"

void TimerClass::TurnOn(void)
  {
  Clock=0;                                      // Initialize variables.
  OldClock=0;
  In=FALSE;
  OldTimer=(unsigned long int) getvect(0x1C);    // Save the old interrupt.
  setvect(0x1C,NewTimer);  // Put mine in place.
  }

void TimerClass::TurnOff(void)
  {
  if (OldTimer!=0) setvect(0x1C,(void interrupt (*)(...)) OldTimer); // Restore his timer.
  }

char CallEm(NodeClass *CursorNode)
  {
  void *Fn,*Data;
  void (*yo)(void *);

  Fn  =  *((void **) CursorNode->GetData());
  Data=  *(((void **) CursorNode->GetData())+1);
  yo= ( (void (*)(void*)) Fn);
  (*yo)(Data);
  return(TRUE);
  }

void TimerClass::AddFn(void Function(void *ptrtodata),void *data) //,int freq)
  {                                           // parameter freq to be implemented later
  void *temp[2];
    
  temp[0]= ((void *) Function);
  temp[1]= data;
  Add( (new NodeClass(1,sizeof (void *)*2, &temp)));
  }

void RemFn(void Function(void *ptrtodata),void *data,int freq=1)
  {
  fprintf(stderr,"Function Not Supported.\n");
  }


//                            END  OF FILE
//-----------------------------------------------------------------------

