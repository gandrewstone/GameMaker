#include "gen.h"
#include "geninput.hpp"
extern TimerClass Time;

extern void KeyUpdate(int KeyInfo);
extern "C" void SetMouseRoutine(void Routine(void));

extern unsigned int far MyDS;
static int dx=0,dy=0;
int    MouseX=0,MouseY=0,MouseButs=0,MickeyX=0,MickeyY=0;

char JoyStickClass::Init(void)  // Peter Savage and Andy Stone
  {  
#ifdef DEBUG
  if (!NewAbortMSG("GM:Joystick Initialization.\n")) printf("Abort Initialization Error!\n");
#endif
  ReadJoyStick(&CenterX,&CenterY,&OldButs); // Read joystick, see if gameport
  if ((CenterX!=0xFFFF)&&(CenterY!=0xFFFF)) //  or joystick exists on machine
    {
    Has=TRUE;                               // Set joystick is installed flag
    On =TRUE;                               // Initially set jstick to active
    RangeX=10;                              // Whatever the true range of the 
    RangeY=10;                              //  stick, I xlate it into this.
    LeastX=CenterX-50; LeastY=CenterY-50;   // Set range to the initial value
    MostX =CenterX+50; MostY =CenterY+50;   //  for lack of other data.
    Time.AddFn(CheckJoyStick ,this);        // Link the polling to the timer
    return(TRUE);                           // Return Joystick installed 
    } 
  Has=FALSE;                                // Set joystick not installed flag 
  On =FALSE;
  return(FALSE);                            // Return not installed
  }

void CheckJoyStick(void *t1)
  {
  JoyStickClass *t;
  int temp;
  int sign=1;
  unsigned int x,y,tx,ty,tb;

  t= (JoyStickClass *) t1;
  ReadJoyStick(&tx,&ty,&tb);                // Read new joystick values
  if (tx>t->MostX)  t->MostX=tx;            // Re-range the stick dynamically
  if (tx<t->LeastX) t->LeastX=tx;
  if (ty>t->MostY)  t->MostY=ty;
  if (ty<t->LeastY) t->LeastY=ty;
  if (tx<t->CenterX-50)
    {
    temp=t->CenterX-(t->LeastX);            // Find total range
    tx= t->CenterX-tx;                      // Find current deviation from center
    sign=-1;
    }
  else if (tx>t->CenterX+50)
    {
    temp=t->MostX-(t->CenterX);             // Find total range
    tx -=t->CenterX;                        // Find current deviation from center
    sign=1;
    }
  else { temp=0; x=0; }
  if (t->RangeX!=0) temp /= t->RangeX;
  else temp=10000;
  if (temp!=0) x= tx/temp;
  else x=0;
  x*=sign;

  if (ty<t->CenterY-50)
    {
    temp=t->CenterY-(t->LeastY);                 // Find total range
    ty  =t->CenterY-ty;                     // Find current deviation from center
    sign= -1;
    }
  else if (ty>t->CenterY+50)
    {
    temp=t->MostY-(t->CenterY);                  // Find total range
    ty -=t->CenterY;                        // Find current deviation from center
    sign=1;
    }
  else { temp=0; ty=0; }
  if (t->RangeY!=0)   temp /= t->RangeY;    
  else             temp  = 10000;
  if (temp!=0)     y     = ty/temp;
  else             y     = 0;
  y*=sign;

  dx=x;
  dy=y;
  
  t->Cursors.Traverse(MoveCursors,t->Cursors.First);  // Move all cursors
  if (tb!=t->OldButs)                   // Call keydn routine if joystick
    {                                   // button status has changed.
    KeyUpdate( (2*256)+(tb&255));
    t->OldButs=tb;
    }
  } 

void MouseClass::AddCursor(CursorClass *Cur)
  {
  Cursors.Add( (new NodeClass(1,sizeof (CursorClass *), &Cur)));
  }

void JoyStickClass::AddCursor(CursorClass *Cur)
  {
  Cursors.Add( (new NodeClass(1,sizeof (CursorClass *), &Cur)));
  }

void MouseClass::SetMickeyRatio(int NewX,int NewY)
  {
  MickeyRatioX=NewX;
  MickeyRatioY=NewY;
  }

void MouseClass::Change(void)
  {
  dx=0; dy=0;

  while (OldMickeyX-MickeyX >= MickeyRatioX) { OldMickeyX+= MickeyRatioX; dx++; }
  while (OldMickeyY-MickeyY >= MickeyRatioY) { OldMickeyY+= MickeyRatioY; dy++; }
  while (MickeyX-OldMickeyX > MickeyRatioX) { OldMickeyX-= MickeyRatioX; dx--; }
  while (MickeyY-OldMickeyY > MickeyRatioY) { OldMickeyY-= MickeyRatioY; dy--; }
  if (dx>1000) dx=0;
  if (dy>1000) dy=0;
  Cursors.Traverse(MoveCursors,Cursors.First);
  }

char MoveCursors(NodeClass *CursorNode)
  {
  CursorClass *Cursor;

  Cursor=  *((CursorClass **) CursorNode->GetData());
  Cursor->Move(dx,dy);
  return(TRUE);
  }

MouseClass::~MouseClass(void)  //Destructor
  {
  if (Has)
    {
    asm mov ax,0                // If he has a mouse, I've put in my handler.
    asm int 33h                 // this fn disables my handler.
    }
  }

char MouseClass::Init(void)
  {
  unsigned int temp;
#ifdef DEBUG
  if (!NewAbortMSG("GM:Mouse Initialization.\n")) printf("Abort Initialization Error!\n");
#endif
  Has=ERROR;

  asm               mov        ax,0
  asm               int        33h
  asm               mov        [temp],ax
  if (temp==0) Has=FALSE;
  else if (temp==0xFFFF)
    {
    MickeyRatioX=2;
    MickeyRatioY=2;
    MyDS=FP_SEG(&MickeyX);   // Remember DS, 'cause in my int. it's destroyed
    SetMouseRoutine(MouseInterrupt);
    Has=TRUE;                // Set the has-a-mouse flag
    }
  return(Has);
  }

void InputClass::Init(void)
  {
#ifdef DEBUG
  if (!NewAbortMSG("GM:Input Initialization.\n")) printf("Abort Initialization Error!\n");
#endif
//  if (m.Init()==1) printf("HE's GOT A MOUSEE!\n");
//  if (j.Init()) printf("He's ALSO got a joystick!");
//  k.Init();
  m.Init();
  j.Init();
  }


/*
void KeyBoardClass::Init(void)
  {
  }
*/

unsigned char GPFofArrow[] =
  { 
  GPFDRAW,0,3,15,0,0,GPFNEWLINE,
  GPFDRAW,0,4,15,15,0,0,GPFNEWLINE,
  GPFDRAW,0,5,15,15,15,0,0,GPFNEWLINE,
  GPFDRAW,0,6,15,15,15,15,0,0,GPFNEWLINE,
  GPFDRAW,0,7,15,15,15,15,15,0,0,GPFNEWLINE,
  GPFDRAW,0,8,15,15,15,15,15,15,0,0,GPFNEWLINE,
  GPFDRAW,0,9,15,15,15,15,15,15,15,0,0,GPFNEWLINE,
  GPFDRAW,0,10,15,15,15,15,15,15,15,15,0,0,GPFNEWLINE,
  GPFDRAW,0,11,15,15,15,15,15,15,15,15,15,0,0,GPFNEWLINE,
  GPFDRAW,0,11,15,15,0,0,15,15,0,0,0,0,0,GPFNEWLINE,
  GPFDRAW,0,3,0,0,0,GPFSKIP,0,1,GPFDRAW,0,4,0,15,15,0,GPFNEWLINE,
  GPFSKIP,0,6,GPFDRAW,0,3,15,15,0,GPFNEWLINE,
  GPFSKIP,0,7,GPFDRAW,0,3,15,15,0,GPFNEWLINE,
  GPFSKIP,0,7,GPFDRAW,0,3,0,0,0,
  GPFDONE
  };

unsigned char GPFofNullCursor[]= { GPFDONE };

