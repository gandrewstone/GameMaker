#include <dos.h>
#include <dir.h>
#define  TMODE 3
#include "gen.h"
#include "pal.h"
#include "pixel.hpp"
#include "coord2d.hpp"
#include "viddrive.hpp"
#include "fileclss.hpp"
#include "image.h"
#include "fli.h"
#include "font.h"
#include "fontchr.h"
#include "textio.h"
#include "geninput.hpp"
#include "vidsave.cpp"

extern unsigned _stklen=20000U;  // Set the stack to 10000 bytes, not 4096

FileRead      File;
GIFImage     *Im;
VideoMode    *Vid;
Font         *StdFnt;

TimerClass    Time;
InputClass    Inp;
GPFClass      GPFArrow;
CursorClass   Cur;
int           Jbuttons=0;
Palette       CurPal;

Dispatcher    Dis;

 // Copy the Directory Name over, zero fill xtra space.
void strncpy(char *dest,char *src,int len)
  {
  for (int i=0;((i<len)&&(src[i]!=0));i++) dest[i]=src[i];
  for (;(i<len);i++)                       dest[i]=0;
  }

class ScrollMenu:public Window
  {
  Coord2d   OldScrPos;
  public:
  VideoMode *View;
  Coord2d   ScrPos;
  Coord2d   WindLen;

  ScrollMenu(VideoMode *Vid):Window(NULL,Vid) {}

  void Init(Box2d Wind,VideoMode *window)
    {
    Edges   = Wind;
    View    = window;
    WindLen = Wind.lr-Wind.ul+1;
    ScrPos.Set(0,0);
    OldScrPos.Set(10000,10000);
    }
  void ReInit(void)
    {
    ScrPos.Set(0,0);
    OldScrPos.Set(10000,10000);
    }
  virtual boolean Start  (void)        { return(False); }
  virtual boolean Draw   (void);
  virtual boolean End    (void)        { return(False); }
  virtual boolean DoMsg  (Message InMsg);
  };

class VerticalMenu:public ScrollMenu
  {
  boolean PrivateInit (uint Nitems, char *ItemList,FileRead &f);
  public:
  uint Result;
  VerticalMenu (Font *f, VideoMode *v):ScrollMenu(v)
    {
    Fnt           = f;
    View          = new VideoMode;
    *View         = *v;
    View->Address = NULL;
    Result        = 0xFFFF;
    }

  boolean Init (Box2d Wind,uint Nitems, char *ItemList,FileRead &f);
  boolean ReInit (uint Nitems, char *ItemList,FileRead &f);
  virtual boolean Start(void) { return(ScrollMenu::Start()); }
  virtual boolean DoMsg  (Message InMsg)
    {
    if (ScrollMenu::DoMsg(InMsg))
      {
      if ((Msg.Origin==COMPUTERMSG)&&(Msg.Val==DONEMSG)) Result=(Msg.Pos.y-5)/Fnt->Height.Total;
      return(True);
      }
    return(False);
    }
  };

int FindFiles(char *filespec,char *path,char *result,uint maxlen,uint *Pos);

class FileMenu:public VerticalMenu
  {
  boolean PrivateInit(char *fspec,char *Dir);
  char   *Files;
  uint    NumF;
  uint    FileIdx;

  public:
  char    Result[21];
  char    CurDir[81];
  boolean Init(Box2d Wind,char *fspec,char *Dir,FileRead &f);   // Fspec must be Double NULL terminated!
  boolean ReInit(char *fspec,char *Dir,FileRead &f);
  FileMenu(Font *f,VideoMode *v);
  ~FileMenu() { if (Files) { delete Files; Files=NULL; } }

  virtual boolean DoMsg  (Message InMsg)
    {
    if (VerticalMenu::DoMsg(InMsg))
      {
      if ((Msg.Origin==COMPUTERMSG)&&(Msg.Val==DONEMSG)) strcpy(Result,Files+(13*VerticalMenu::Result));
      return(True);
      }
    return(False);
    }
  };

int FindDirs(char *dirspec,char *path,char *result,uint maxlen,uint *Idx);
class DirMenu:public VerticalMenu
  {
  boolean PrivateInit(char *fspec,char *Dir);
  char *Files;
  uint NumF;
  uint FileIdx;

  public:
  char Result[21];
  char CurDir[81];

  DirMenu(Font *f,VideoMode *v);
  ~DirMenu() { if (Files) { delete Files; Files=NULL; } }
  boolean Init(Box2d Wind,char *fspec,char *Dir,FileRead &f); // Fspec must be Double NULL terminated!
  boolean ReInit(char *fspec,char *Dir,FileRead &f);          // Fspec must be Double NULL terminated!

  virtual boolean DoMsg  (Message InMsg)
    {
    if (VerticalMenu::DoMsg(InMsg))
      {
      if ((Msg.Origin==COMPUTERMSG)&&(Msg.Val==DONEMSG))
        strcpy(Result,Files+(13*VerticalMenu::Result));
      return(True);
      }
    return(False);
    }
  };

class FileChoice:public Dispatcher
  {
  FileMenu    FileSel;
  DirMenu     DirSel;
  QueryString TextEntry;
  int         ylen;
  public:

  char Result[81];

  char *FilePic;
  char *DirPic;
  char *FileSpec;

  friend Message ChooseFileInterWindowComm(Window *Affected,Window *T);

  boolean Init(Coord2d Pos,int width,char *fspec,char *Dir);

  FileChoice(Font *Fnt,VideoMode *v):Dispatcher(Fnt,v),FileSel(Fnt,v),DirSel(Fnt,v),TextEntry(Fnt,v)
    {
    FilePic = NULL;
    DirPic  = NULL;
    }

  boolean Draw(void)
    {
    DrawFrame(1);
    DrawSurface(4);
    FileSel  .DrawFrame(0);
    FileSel  .Draw();
    DirSel   .DrawFrame(0);
    DirSel   .Draw();
    TextEntry.Start();
    return(True);
    }

  boolean DoMsg(Message InMsg) { Dispatch(Inp); return(True); }
  };

boolean FileChoice::Init(Coord2d Pos,int width,char *fspec,char *Dir)
  {
  char Fnames[51][2];

  FileSpec=fspec;
  ylen=Min(width,Vid->Size.y-Pos.y);

  if (FilePic==NULL)  FilePic="c:\\vmenu\\vmenu14.gif";
  if (DirPic ==NULL)  DirPic="c:\\vmenu\\vmenu11.gif";

  Edges.ul   = Pos;
  Edges.lr.x = Pos.x+180;
  Edges.lr.y = Pos.y+ylen;

  FileRead f(FilePic);

  strcpy(Result,Dir);
  Pos.y-=1;
  TextEntry.Init(MakeBox2d(Edges.ul.x+1,Edges.ul.y,Edges.lr.x-5,Edges.ul.y+Fnt->Height.Total),"File:",Result,80);
  TextEntry.SetColors(10,9,4);
  Pos.y+=Fnt->Height.Total+3;
  Pos.x+=6;
  FileSel.Init(MakeBox2d(Pos,MakeCoord2d(Pos.x+79,Edges.lr.y-5)),fspec,Dir,f);
  f.Open(DirPic);
  Pos.x+=89;
  DirSel.Init(MakeBox2d(Pos,MakeCoord2d(Pos.x+79,Edges.lr.y-5)),"*.*\0",Dir,f);
  f.Close();

  *this += &TextEntry;
  *this += &FileSel;           // Put the windows in the local dispatcher.
  *this += &DirSel;
  InterWindowComm = ChooseFileInterWindowComm;

  return(True);
  }



void RemoveDirectory(char *path)
  {
  for (int ctr=strlen(path);((path[ctr]!='\\')&&(ctr>=0));ctr--) path[ctr]=0;
  if (ctr>0)
    {
    path[ctr]=0;
    for (;((path[ctr]!='\\')&&(ctr>=0));ctr--) path[ctr]=0;
    }
  }

void Directory(char *path)
  {
  for (int ctr=strlen(path);((path[ctr]!='\\')&&(ctr>=0));ctr--) path[ctr]=0;
  }


static Message ChooseFileInterWindowComm(Window *Affected,Window *T)
  {
  FileChoice *This = (FileChoice *) T;
  char    NewDir[81];
  boolean NewD=False;

  if (This->DirSel.Result[0] != 0)   // See if directory has changed.
    {
    if (strcmp(This->DirSel.Result,"..\\") == 0) // Go back a directory
      {
      RemoveDirectory(This->DirSel.CurDir);
      strncpy(This->Result,This->DirSel.CurDir,80);
      }
    else                                         // Go deeper.
      {
      strcpy (This->Result,This->DirSel.CurDir);
      strcat (This->Result,This->DirSel.Result);
      }
    NewD=True;
    strcpy (NewDir,This->Result);
    }
  else  // See if the user has manually entered in a new dir name.
    {
    strcpy(NewDir,This->Result);
    Directory(NewDir);
    if (strcmp(NewDir,This->DirSel.CurDir) != 0) NewD=True;
    }

  if (NewD)
    {
    FileRead f(This->DirPic);
    This->DirSel.ReInit("*.*\0",NewDir,f);
    f.Open(This->FilePic);
    This->FileSel.ReInit(This->FileSpec,This->Result,f);

    Inp.Cursor->Off();
    This->DirSel.Draw();
    This->FileSel.Draw();
    This->TextEntry.Draw();
    Inp.Cursor->On();
    }

  if (This->FileSel.Result[0] != 0)
    {
    char temp[81];
    FileRead f(This->DirPic);
    strcpy (temp,This->DirSel.CurDir);
    strcat (temp,This->FileSel.Result);
    if (strcmp(temp,This->Result) != 0 )
      {
      strcpy(This->Result,temp);
      This->TextEntry.Draw();
      }
    This->FileSel.Result[0]=0;
    }

//DEBUG
  char str[80];
  sprintf(str,"Free Mem %lu\n",farcoreleft());
  Vid->BoxFill(MakeCoord2d(1,160),MakeCoord2d(300,175),1,Vid);
  This->Fnt->Draw(MakeCoord2d(1,170),9,str,Vid);

  return(T->Msg);
  }


Box2d   SelectArea(VideoMode *Vid);
void    PrintCoords(void);
void    TextMode(void);
boolean InputInit(void);


//=======================================MAIN=================================

//void main(int argc,char *argv[])
void main(void)
  {
  int           ModeNum=0;
//  char          FileName[81] ="c:\\pics\\water.gif";
  char          Driver[51]   ="d:\\drv\\stlth64.drv";
  char          FontFile[51] ="d:\\font\\tiny.chr";
  VideoDriver  *Drvr;
  FontChrClass *Fnt = new FontChrClass;

  for (int cnt=0;cnt<8;cnt++) CurPal(cnt).Set(cnt*8,cnt*8,cnt*8);
  CurPal(8).Set(63,63,30);
  CurPal(9).Set(63,0,63);
  CurPal(10).Set(60,60,0);
  CurPal.ColsUsed=11;


  if ((Drvr=LoadVideoDriver(Driver))==NULL)        // Get the graphics Driver.
    { printf("Error Loading Driver.\n"); return; }
  Vid=Drvr->GetMode(ModeNum);
  Vid->SetMode(Vid);

  InputInit();                                     // Set up Input Classes.
  Cur.SetVideo(Vid);
  Cur.On();

  File.Open(FontFile);                             //  Load the Font in.
  Fnt->Load(File);
  StdFnt = Fnt;

  if (Inp.m.Has) Fnt->Draw(MakeCoord2d(Vid->Size.y-Fnt->Height.Descender,100),4,"Mouse installed",Vid);

  FileRead f("c:\\pics\\backgrnd\\b.gif");
  GIFImage *g = new GIFImage;
  g->Startup(f,Vid,&CurPal);
  Vid->SetAllPal(CurPal.Cols,Vid);
  g->Draw(MakeCoord2d(0,0));
  delete g;

  f.Close();

  for (cnt=0;cnt<256;cnt++) Vid->Line(MakeCoord2d(cnt,190),MakeCoord2d(cnt,199),cnt,Vid);

  FileChoice FC(StdFnt,Vid);
  FC.Init(MakeCoord2d(19,12),100,"*.*\0","c:\\\0");
  FC.Draw();
  Vid->SetAllPal(CurPal.Cols,Vid);

  Dis +=&FC;
  Dis.Dispatch(Inp);

  Inp.GetInput();

  delete Fnt;
  Cur.Off();
  Time.TurnOff();
  Inp.UnInit();
  TextMode();
  printf("Goodbye!");
  }


boolean     InputInit(void)
  {
  GPFArrow.Get(GPFofArrow);
  Cur .NewGPF(&GPFArrow);
  Cur .SetHotSpot(1,0);
  Cur .Limit(0,0,319,199);
  Inp .Init();
  Inp .Cursor=&Cur; // Set the cursor that points to where input is going to.
  if (!Inp.m.Has) Inp.j.AddCursor(&Cur);
  else Inp.m.AddCursor(&Cur);
  Time.TurnOn();
  return ((boolean) (Inp.m.Has || Inp.j.Has));
  }


void TextMode(void)
  {
  union REGS r;

  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);
  }



void PrintCoords(void)
  {
  Coord2d Old(0,0);
  char    str[21];
  while (!Inp.Pending())
    {
    if (Old!=Cur.Pos)
      {
      sprintf(str,"(%d,%d)",Cur.Pos.x,Cur.Pos.y);
      Vid->BoxFill(MakeCoord2d(100,100-StdFnt->Height.Capitol),MakeCoord2d(200,100+StdFnt->Height.Descender),0,Vid);
      StdFnt->Draw(MakeCoord2d(100,100),12,str,Vid);
      Old=Cur.Pos;
      }
    }
  Inp.GetInput();
  }


Box2d SelectArea(VideoMode *Vid)
  {
  Box2d Ret;
  VideoSave *Save = new VideoSave;

  *Save     = *Vid;
  Save->Buf =  new Pixel[10000];

  while(KeyDn[MOUSEKEYDN]==0) if (Inp.Pending()) KeysPending=0;
  Ret.ul=Ret.lr=Cur.Pos;

  Cur.Off();
  Save->Box(Ret.ul,Ret.lr,4,Save);
  Cur.On();
  while(KeyDn[MOUSEKEYDN])
    {
    if (Cur.Pos!=Ret.lr)
      {
      Cur.Off();
      Save->RestoreMode();
      Save->Box(Ret.ul,Ret.lr,4,Save);
      Ret.lr=Cur.Pos;
      Save->SaveMode();
      Save->Box(Ret.ul,Ret.lr,4,Save);
      Cur.On();
      }
    }
  Cur.Off();
  Save->RestoreMode();
  Save->Box(Ret.ul,Ret.lr,4,Save);
  Cur.On();

  Inp.GetInput();
  delete Save->Buf;
  delete Save;
  return(Ret);
  }

#define VERTMENUCOL 8
boolean VerticalMenu::PrivateInit (uint Nitems, char *ItemList,FileRead &f)
  {
  if (!f.EOFflag)
    {
    if (View->Address!=NULL) { delete View->Address; View->Address=NULL; }
    GIFImage *Gif = new GIFImage;
    Gif->Startup(f,View,&CurPal);
    View->Size    = Gif->Size;
    View->LineSep = View->Size.x;
    View->Size.y  = Min(View->Size.y,Max(Nitems*Fnt->Height.Total,WindLen.y));
    View->Size.y  = Min(View->Size.y,65500/View->LineSep);
    View->Address = new Pixel [View->LineSep*View->Size.y];
    if (View->Address) Gif->Draw(MakeCoord2d(0,0));
    delete Gif;
    f.Close();
    }
  else return(False);

  if (Fnt!=NULL)
    {
    Coord2d Pos(5,5+Fnt->Height.Capitol);
    for (uint Cnt=0;((Pos.y<View->Size.y)&&(Cnt<Nitems));Cnt++,Pos.y+=Fnt->Height.Total)
      {
      uint Next=0;
      while (ItemList[Next]!=0) Next++;
      Fnt->Draw(Pos+1,0,View->Size.x,ItemList,Next,View);
      Fnt->Draw(Pos,VERTMENUCOL,View->Size.x,ItemList,Next,View);
      while (ItemList[Next]==0) Next++;
      ItemList+=Next;
      }
    }
  return(True);
  }


boolean VerticalMenu::Init (Box2d Wind,uint Nitems, char *ItemList,FileRead &f)
  {
  ScrollMenu::Init(Wind,View);
  return(PrivateInit(Nitems,ItemList,f));
  }

boolean VerticalMenu::ReInit (uint Nitems, char *ItemList,FileRead &f)
  {
  ScrollMenu::ReInit();
  return(PrivateInit(Nitems,ItemList,f));
  }

boolean ScrollMenu::DoMsg  (Message InMsg)
  {
  Msg.Clear();
  if (InMsg.Origin==KEYBOARDMSG)
    {
    switch(Ascii(InMsg.Val))
      {
      case 13:
        Msg.Origin = COMPUTERMSG;
        Msg.Val    = DONEMSG;
        Msg.Pos    = InMsg.Pos-Edges.ul+ScrPos;
        return(True);
      }
    }
  if (KeyDn[MOUSEKEYDN])
    {
    boolean Moved=False;
    Coord2d &CurPos = Inp.Cursor->Pos;
    Coord2d  OldPos = CurPos;
    Inp.Cursor->Limit(Edges);
    while(KeyDn[MOUSEKEYDN])
      {
      if (OldPos != CurPos)       // Time to Scroll.
        {
        Moved=True;
        ScrPos+=(CurPos-OldPos);  // Set New starting Pos.
        Coord2d Old=OldScrPos;
        Inp.Cursor->Off();
        Draw();
        Inp.Cursor->Move(Old-ScrPos);
        Inp.Cursor->On();
        OldPos=CurPos;
        }
      }
    if (!Moved)
      {
      Msg.Origin = COMPUTERMSG;
      Msg.Val    = DONEMSG;
      Msg.Pos    = InMsg.Pos-Edges.ul+ScrPos;
      }
    Inp.Cursor->Limit(0,0,Vid->Size.x,Vid->Size.y);
    return(True);
    }
  return(False);
  }


boolean ScrollMenu::Draw(void)
    {
    if (ScrPos.y<0) ScrPos.y=0;
    if (ScrPos.x<0) ScrPos.x=0;
    if (ScrPos.y+WindLen.y>View->Size.y) ScrPos.y=Max(View->Size.y-WindLen.y,0);
    if (ScrPos.x+WindLen.x>View->Size.x) ScrPos.x=Max(View->Size.x-WindLen.x,0);

    if (ScrPos!=OldScrPos)
      {
      OldScrPos=ScrPos;
      Pixel *StAddr = View->Address+((ScrPos.y*View->LineSep)+ScrPos.x);
      for (Coord2d Pos=Edges.ul;Pos.y<WindLen.y+Edges.ul.y;Pos.y++,StAddr+=View->LineSep)
        Vid->RepCols(Pos,WindLen.x,StAddr,Vid);
      return(True);
      }
    return(False);
    }

FileMenu::FileMenu(Font *f,VideoMode *v):VerticalMenu(f,v)
  {
  Files=NULL;
  NumF=FileIdx=0;
  }

// Fspec must be Double NULL terminated!
boolean FileMenu::PrivateInit(char *fspec,char *Dir)
  {
  for (uint i=0;i<21;i++) Result[i]=0;
  for (i=0;i<81;i++) CurDir[i]=0;
  if (Files==NULL) Files = new char [1300];
  if (Files==NULL) return(False);
  FileIdx   = 0;
  Result[0] = 0;
  NumF      = 0;
  while ((*fspec)!=0)             // Get a list of all the files.
    {
    NumF+=FindFiles(fspec,Dir,Files+FileIdx,1300,&FileIdx);
    while ((*fspec)!=0) fspec++;  // Go to the next file spec.
    }
  return(True);
  }

boolean FileMenu::Init(Box2d Wind,char *fspec,char *Dir,FileRead &f)
  {
  if (PrivateInit(fspec,Dir)) return(VerticalMenu::Init(Wind,NumF,Files,f));
  return(False);
  }

boolean FileMenu::ReInit(char *fspec,char *Dir,FileRead &f)
  {
  if (PrivateInit(fspec,Dir)) return(VerticalMenu::ReInit(NumF,Files,f));
  return(False);
  }

DirMenu::DirMenu(Font *f,VideoMode *v):VerticalMenu(f,v)
  {
  Files=NULL;
  NumF=FileIdx=0;
  }

// Fspec must be Double NULL terminated!
boolean DirMenu::PrivateInit(char *fspec,char *Dir)
  {
  for (uint i=0;i<21;i++) Result[i]=0;
  for (i=0;i<81;i++)      CurDir[i]=0;
  if (Files==NULL) Files = new char [1300];
  if (Files==NULL) return(False);
  FileIdx   = 0;
  Result[0] = 0;
  NumF      = 0;
  strcpy(CurDir,Dir);
  while ((*fspec)!=0)   // Get a list of all the files.
    {
    NumF+=FindDirs(fspec,Dir,Files+FileIdx,1300,&FileIdx);
    while ((*fspec)!=0) fspec++;  // Go to the next file spec.
    }
  return(True);
  }

boolean DirMenu::Init(Box2d Wind,char *fspec,char *Dir,FileRead &f)
  {
  if (PrivateInit(fspec,Dir)) return(VerticalMenu::Init(Wind,NumF,Files,f));
  return(False);
  }

boolean DirMenu::ReInit(char *fspec,char *Dir,FileRead &f)
  {
  if (PrivateInit(fspec,Dir)) return(VerticalMenu::ReInit(NumF,Files,f));
  return(False);
  }


//MouseMoved
//            is called whenever the mouse is moved by the user.
// Friend function to MouseClass.
// Andy Stone                   7/22/92

void MouseMoved(void)
  {
  Inp.m.Change();
  }

// KeyUpdate
//           is called whenever an input button is pressed or released.
// This can be a key hit on the keyboard, a mouse button, or a joystick button.
// in upper byte, 1=keyboard, 2=mouse, 3=jstk #1, 4=jstk#2.
// Andy Stone   7/22/92
extern uint MouseButs;
void KeyUpdate(uint KeyI)
  {
  uchar upkey  =KeyI>>8;
  uchar KeyInfo=KeyI&255;
  switch (upkey)
    {
    case 1:
      break;
    case 2:
      KeyInfo>>=1;
      KeyDn[MOUSEKEYDN]=((MouseButs&7)>0);

      for (uint j=0;j<3;j++,KeyInfo>>=2)
        {
        if (KeyInfo&1)
          {
          KeyDn[MOUSEKEYDN+j+1] = True;
          if (KeysPending<MAXKEYSTK)
            {
            KeyStk[KeysPending]=0x200+j;
            KeysPending++;
            }
          }
        if (KeyInfo&2) KeyDn[MOUSEKEYDN+j+1] = False;
        }
      break;
    }
  }



