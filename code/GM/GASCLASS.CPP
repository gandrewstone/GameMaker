#include "gen.h"
#include "coord2d.hpp"
#include "genclass.hpp"
#include "timer.hpp"
#include "windclss.hpp"
#include "gasclass.hpp"
extern TimerClass Time;
extern int CurMode;

unsigned int GPFClass::Length(void)
  {
  unsigned char *cur;
  char done=False;
  int len;  

  if (PicInfo==NULL) return(ERROR);
  cur=PicInfo;
  
  while(!done)
    {
    switch(*cur)
      {
      case GPFDONE:
        done=True;
        break;
      case GPFDRAW:
        cur+=2;
        len=( (int) *cur);
        cur+=len+1;
        break;
      case GPFNEWLINE:
        cur++;
        break;        
      case GPFSKIP:
        cur +=3;
        break;
      default:
        return(0);
      }
    }
  return( (unsigned int) ( ((unsigned long int) cur)-((unsigned long int) PicInfo)) );
  }



void GPFClass::Draw(Coord2d Pos,DrawingBoard *w)
  {
  uchar *cur;
  char   done=False;
  int    xt,len;
  int    initx;


  if (PicInfo==NULL) return;

  initx=Pos.x;
  cur  =PicInfo;
  
  while(!done)
    {
    switch(*cur)
      {
      case GPFDONE:
        done=True;
        break;
      case GPFDRAW:
        cur+=2;
        len=( (int) *cur);
        cur++;
        Pos.x++;
        (*(w->RepCols))(Pos.x,Pos.y,len,cur,w);
        cur+=len;
        Pos.x+=len;
        break;
      case GPFNEWLINE:
        Pos.y++;
        Pos.x=initx;
        cur++;
        break;        
      case GPFSKIP:
        cur +=2;
        len=( (int) *cur);
        Pos.x+=len-1;
        cur++;
        break;
      }
    }
  }


void GPFClass::Draw(Coord2d Pos, DrawingBoard *w, GPFClass *Save)
  {
  uchar *cur;
  uchar *sav;
  char   done=False;
  int    xt,len;
  int    initx;

  if (PicInfo==NULL)
    {
    *Save->PicInfo=GPFDONE;
    return;
    }

  initx= Pos.x;
  cur  = PicInfo;
  sav  = (Save->PicInfo)-1;
  
  while(!done)
    {
    switch(*cur)
      {
      case GPFDONE:
        done=True;
        sav++;
        *sav=GPFDONE;
        break;
      case GPFDRAW:
        sav++;
        *sav=GPFDRAW;
        cur+=2;
        len=( (int) *cur);
        sav++;
        *sav=len>>8;
        sav++;
        *sav=len;

        cur++;
        sav++;
        Pos.x++;
        w->GetCols(Pos.x,Pos.y,len,sav,w);
        w->RepCols(Pos.x,Pos.y,len,cur,w);
        cur   += len;
        sav   += len-1;
        Pos.x += len;
        break;
      case GPFNEWLINE:
        sav++;
        *sav=GPFNEWLINE;
        Pos.y++;
        Pos.x=initx;
        cur++;
        break;        
      case GPFSKIP:
        sav++;
        *sav=GPFSKIP;
        sav++;
        *sav=*(cur+1);
        sav++;
        *sav=*(cur+2);
        cur +=2;
        len=( (int) *cur);
        Pos.x+=len-1;
        cur++;
        break;
      }
    }
  }

void CursorClass::Limit(int x,int y,int x1,int y1)
  {
/*  if (CurMode==TMODE)
    {
    x <<=2;
    y <<=2;
    x1<<=2;
    y1<<=2;
    }
*/
  Limits.ul.x=x;  Limits.ul.y=y;
  Limits.lr.x=x1; Limits.lr.y=y1;
  if (Limits.ul.x>Limits.lr.x) Swap(Limits.ul.x,Limits.lr.x);
  if (Limits.ul.y>Limits.lr.y) Swap(Limits.ul.y,Limits.lr.y);
  if (Pos.x<Limits.ul.x) Pos.x=Limits.ul.x;
  if (Pos.y<Limits.ul.y) Pos.y=Limits.ul.y;
  if (Pos.y>Limits.lr.y) Pos.y=Limits.lr.y;
  if (Pos.x>Limits.lr.x) Pos.x=Limits.lr.x;
  }


void CursorClass::AddRule(int x,int y,int x1,int y1,GPFClass *Pic)
  {
  NodeClass *n;
  unsigned char temp[81];
  unsigned char *t;

  if (x>x1) Swap (x,x1);
  if (y>y1) Swap (y,y1);

  t=temp;
  *( (unsigned char *)  t)=INBOXRULE;
  t++;
  *( (unsigned int *)t)=x;
  t+=2;
  *( (unsigned int *)t)=y;
  t+=2;
  *( (unsigned int *)t)=x1;
  t+=2;
  *( (unsigned int *)t)=y1;
  t+=2;
  *( (GPFClass**) t) =Pic;
  
  n=new NodeClass(2,20,temp);
  if (n!=NULL) Rules.Add(n);
  }

CursorClass::~CursorClass()
  {
  if (Behind->PicPtr()!=NULL) free(Behind->PicPtr());
  delete Behind;
  }

CursorClass::CursorClass(DrawingBoard *s)
  {
  Old         = MakeCoord2d(5,5);
  Pos=Hot     = MakeCoord2d(0,0);
//  IsOn        = True;
  Show        = False;
  Moving      = False;
  Scrn        = s;             // Set up the window to put the cursor in
  Limits.lr   = Limits.ul = MakeCoord2d(0,0);
  Behind      = new GPFClass;
  CurGPF      = NULL;
  Time.AddFn(CursorClassDraw ,((void (far*)(void*)) this));        // Link the polling to the timer
  }

void CursorClass::NewGPF(GPFClass *NewOne)
  {
  int CurOn=0;
  unsigned int t;
  unsigned char *temp;

  CurOn=Show;
  if (Show) Off();

  if ((t=NewOne->Length())!=ERROR)
    {
    if (Behind->PicPtr()!=NULL) free(Behind->PicPtr());
    if ((temp=(unsigned char*)malloc(t))!=NULL) 
      {
      *temp=GPFDONE;
      Behind->Get(temp);
      CurGPF=NewOne;      
      }
    }
  if (CurOn) On();  
  }

CursorClass *d;
void CursorClassDraw(void *d1)
  {
  d= (CursorClass *) d1;

//  if ((d->IsOn)&&(d->Old!=d->Pos))
  if (d->Old!=d->Pos)
    {
    if ((d->Show)&&(d->CurGPF!=NULL)) d->Behind->Draw(d->Old-d->Hot,d->Scrn);
    int tshow = d->Show;
    d->Show   = False;
    d->Rules.Traverse(CursorRulesCheck);
    d->Show   = tshow;
    d->Moving = True;
    if ((d->Show)&&(d->CurGPF!=NULL)) d->CurGPF->Draw(d->Pos-d->Hot,d->Scrn,(d->Behind));
    d->Old    = d->Pos;
    d->Moving = False;
    }
  }

char CursorRulesCheck(NodeClass *node)
  {
  char retval=True;
  unsigned char *info;

  info = (unsigned char *) node->GetData();
  switch (*info)
    {
    case INBOXRULE:
      if (InBox(d->Pos.x,d->Pos.y,*((int *)(info+1)),*((int *)(info+3)),*((int *)(info+5)),*( (int *)(info+7)) ))
        {
        retval=False;
        
        if (d->CurGPF != *((GPFClass **) (info+9)) )
          {
          d->NewGPF(*((GPFClass **) (info+9)));
          }
        }
      break;
    }
  return(retval);
  }

void CursorClass::On(void)
  { 
//  if ((IsOn)&&(!Show)&&(CurGPF!=NULL))
  if ((!Show)&&(CurGPF!=NULL))
    {
    Show=True; Old=Pos;
    CurGPF->Draw(Pos-Hot,Scrn,Behind);
    }
  }  

void CursorClass::Off(void)
  { 
//  if ((IsOn)&&(Show))
  if (Show)
    {
    Show=False;
    Behind->Draw(Old-Hot,Scrn);
    }
  }

void CursorClass::Goto(int x,int y)
  { 
  Pos.x=x; Pos.y=y;
  }

void CursorClass::Move(int dx,int dy)
  { 
  if (!Moving)
    {
    Pos.x+=dx; Pos.y+=dy;
    if (Pos.x<Limits.ul.x) Pos.x=Limits.ul.x;
    if (Pos.y<Limits.ul.y) Pos.y=Limits.ul.y;
    if (Pos.x>Limits.lr.x) Pos.x=Limits.lr.x;
    if (Pos.y>Limits.lr.y) Pos.y=Limits.lr.y;
    }
  }
