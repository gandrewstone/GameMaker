// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  WINDOW.CPP
  Defines type TWindow.  This defines the basic behavior
  of all windows.
  -------------------------------------------------------- */

#include <string.h>
#include <alloc.h>
#include "applicat.h"
#include "window.h"

__link(RegScroller)

extern PTWindowsObject CreationWindow;
extern PTWindowsObject DlgCreationWindow;

extern PTWindowsObject GetObjectPtr(HWND);

#ifdef WIN31
extern long FAR PASCAL _export
     InitWndProc(HWND, UINT, WPARAM, LPARAM);
#endif

#ifdef WIN30
extern LONG FAR PASCAL _export
     InitWndProc(HWND_30, WORD, WORD, LONG);
#endif

/* Constructor for a TWindow.  Initializes its data fields using passed
  parameters and default values. */
TWindow::TWindow(PTWindowsObject AParent, LPSTR ATitle, PTModule AModule)
        : TWindowsObject(AParent, AModule)
{
  Title = _fstrdup(ATitle ? ATitle : "");
  DefaultProc = (WNDPROC)DefWindowProc;
  if ( !AParent )
    Attr.Style = WS_OVERLAPPEDWINDOW;
  else
    if ( AParent->IsFlagSet(WB_MDIFRAME) )
    {
      /* Set WB_MDICHILD by default, change this if you want a
         non-mdi child as a child of your TMDIFrame window. */
      SetFlags(WB_MDICHILD, TRUE);
      Attr.Style = WS_CLIPSIBLINGS;
    }
    else
      Attr.Style = WS_VISIBLE;
  Attr.ExStyle = 0;
  Attr.X = CW_USEDEFAULT;
  Attr.Y = 0;
  Attr.W = CW_USEDEFAULT;
  Attr.H = 0;
  Attr.Param = NULL;
  Attr.Menu = NULL;
  Attr.Id = 0;
  Scroller = NULL;
  FocusChildHandle = 0;
}

/* Constructor for a TWindow which is being used in a DLL as an alias
   for a non-OWL window */
TWindow::TWindow(HWND AnHWindow, PTModule AModule) :
         TWindowsObject((PTWindowsObject)NULL, AModule)
{
  HWND HParent;
  RECT WndRect;

  HWindow = AnHWindow;

  int TitleLength = GetWindowTextLength(AnHWindow);
  if (TitleLength > -1)
  {
      Title = (LPSTR)farmalloc((unsigned long)TitleLength + 1);
      Title[0] = '\0'; Title[TitleLength] = '\0';
      GetWindowText(AnHWindow, Title, TitleLength+1);
  }
  else
      Title = _fstrdup("");

  DefaultProc = (WNDPROC) SetWindowLong(AnHWindow, GWL_WNDPROC,
                                          (DWORD)GetInstance());

  // Note that the following two lines are not guaranteed to exactly
  // reconstruct the original Style and ExStyle values passed to
  // CreateWindow or CreateWindowEx.  This is because some controls
  // (e.g. EDIT) modify their style bits (e.g. WS_BORDER) when they
  // are being created.

  Attr.Style = GetWindowLong(AnHWindow, GWL_STYLE);
  Attr.ExStyle = GetWindowLong(AnHWindow, GWL_EXSTYLE);

  HParent = GetParent(AnHWindow);
  GetWindowRect(AnHWindow, &WndRect);
  Attr.H = WndRect.bottom - WndRect.top;
  Attr.W = WndRect.right - WndRect.left;
  if ( HParent && ((Attr.Style & WS_CHILD) == WS_CHILD) )
    ScreenToClient(HParent, (LPPOINT)&WndRect);
  Attr.X = WndRect.left;
  Attr.Y = WndRect.top;
  Attr.Param = NULL;
  Attr.Menu = NULL;
  Attr.Id = GetWindowWord(AnHWindow, GWW_ID);

  Scroller = NULL;
  FocusChildHandle = 0;
  SetFlags(WB_ALIAS, TRUE);
}

/* Destructor for a TWindow.  Disposes of its Scroller if the TScroller
  object was constructed. */
TWindow::~TWindow()
{
  if ( Scroller )
  {
    delete Scroller;
    Scroller = NULL;
  }
  if ( HIWORD(Attr.Menu) )
    farfree(Attr.Menu);
}

/* If Attr.Menu currently points to a string then free it. Set Attr.Menu to
   the passed parameter. If the window has been created then load and
   set the new menu and destroy the old one. The return value is FALSE
   if SetMenu is called and fails, TRUE otherwise.  Note that this function
   is declared virtual, but may be called from constructors of classes
   derived from TWindow.  In C++, virtual functions called from constructors
   are called as though they were non-virtual.  Care must be taken when
   redefining AssignMenu in derived classes. */
BOOL TWindow::AssignMenu(LPSTR MenuName)
{
  BOOL ReturnVal = TRUE;
  HMENU NewHMenu;
  HMENU OldHMenu;
  LPSTR TempMenuName = NULL;    // handle case where MenuName==Attr.Menu

  if ( HIWORD(Attr.Menu) )
      TempMenuName = Attr.Menu;

  if ( HIWORD(MenuName) )   // Not NULL and not an int in disguise
    Attr.Menu = _fstrdup(MenuName);
  else
    Attr.Menu = MenuName;

  if (TempMenuName)
      farfree(TempMenuName);

  if ( HWindow )
  {
    OldHMenu = GetMenu(HWindow);
    NewHMenu = LoadMenu(GetModule()->hInstance, Attr.Menu);
    ReturnVal = SetMenu(HWindow, NewHMenu);
    if ( OldHMenu )
      DestroyMenu(OldHMenu);
  }
  return ReturnVal;
}

BOOL TWindow::AssignMenu(int MenuId)
{
  return AssignMenu((LPSTR)MAKEINTRESOURCE(MenuId));
}


/* Specifies registration attributes for the MS-Windows window
   class of the TWindow, allowing instances of TWindow to be
   registered.  Sets the fields of the passed WNDCLASS parameter
   to the default attributes appropriate for a TWindow. */
void TWindow::GetWindowClass(WNDCLASS& AWndClass)
{
  AWndClass.cbClsExtra		= 0;
  AWndClass.cbWndExtra		= 0;
  AWndClass.hInstance		= GetModule()->hInstance;
  AWndClass.hIcon		= LoadIcon(0, IDI_APPLICATION);
  AWndClass.hCursor		= LoadCursor(0, IDC_ARROW);
  AWndClass.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
  AWndClass.lpszMenuName	= NULL;
  AWndClass.lpszClassName	= GetClassName();
  AWndClass.style		= CS_HREDRAW | CS_VREDRAW;
  AWndClass.lpfnWndProc 	= InitWndProc;
}

/* Associates an MS-Windows interface element with the TWindow object,
  after creating the interface element if not already created.  When
  creating an element, uses the creation attributes previously set
  in the Attr data field.  (Simply associates the TWindow with an
  already-created interface element if the "FromResource" flag is set.)
  If the TWindow could be successfully associated, calls SetupWindow and
  returns True.  Association is not attempted if the TWindow's Status
  data field is non-zero. */
BOOL TWindow::Create()
{
    HWND HParent;
    PTWindow TheMDIClient; // compiler doesn't know about TMDIClients yet
    MDICREATESTRUCT CreateStruct;
    WORD MenuOrId = 0;

  if (Status == 0)
  {
    DisableAutoCreate();
    if ( Parent == NULL )
      HParent = 0;
    else
      HParent = Parent->HWindow;
    if ( !IsFlagSet(WB_FROMRESOURCE) )
    {
      if ( Register() )
      {
        CreationWindow = this;
        TWindowsObject *TmpDlgCreationWindow = DlgCreationWindow;
        /*  Set DlgCreationWindow to NULL so in InitWndProc we'll know
            we're not being created from a resource */
        DlgCreationWindow = NULL;
        /* Create as MDI child if the WB_MDICHILD flag is set and Parent
           has a Client */
        if (IsFlagSet(WB_MDICHILD))
        {
	    if ((TheMDIClient = (PTWindow)(Parent->GetClient())) != NULL)
	    {   // MDI child window
                DefaultProc = (WNDPROC)DefMDIChildProc;
                CreateStruct.szClass = GetClassName();
                CreateStruct.szTitle = Title;
                CreateStruct.hOwner = GetModule()->hInstance;
                CreateStruct.x = Attr.X; CreateStruct.y = Attr.Y;
                CreateStruct.cx = Attr.W; CreateStruct.cy = Attr.H;
                CreateStruct.style = Attr.Style;
		CreateStruct.lParam = (LONG)Attr.Param;
		
		HWindow = (HWND)LOWORD(SendMessage(TheMDIClient->HWindow,
		    WM_MDICREATE, 0, (LPARAM)(&CreateStruct)));
            }
            else    // Parent doesn't have a client, error
                Status = EM_INVALIDWINDOW;
        }
        else    // Not an MDI child window
        {
          if ( Attr.Menu)
	    MenuOrId = (WORD)LoadMenu(GetModule()->hInstance,
                                   Attr.Menu);
          else
            MenuOrId = Attr.Id;
          HWindow = CreateWindowEx(Attr.ExStyle, GetClassName(),
            Title, Attr.Style, Attr.X, Attr.Y, Attr.W,
	    Attr.H, HParent, (HMENU)MenuOrId , GetModule()->hInstance,
	    Attr.Param);
        }
        DlgCreationWindow = TmpDlgCreationWindow;
      }
    }
    else /* Windows already created window */
      HWindow = GetDlgItem(HParent, Attr.Id);
    if ( HWindow )
    {
      if ( !GetObjectPtr(HWindow) ) // predefined Windows class
      {
        // Retrieve the Title
        int TitleLength = GetWindowTextLength(HWindow);
        if (TitleLength > -1)
        {
            Title = (char far*)farmalloc((unsigned long)TitleLength + 1);
            Title[0] = '\0'; Title[TitleLength] = '\0';
            GetWindowText(HWindow, Title, TitleLength+1);
        }
        else
            Title = _fstrdup("");

        // Retrieve Attr.Style and Attr.ExStyle.

        // NOTE:  Some windows controls (e.g. EDIT) change the style bits
        // (e.g. WS_BORDER) from their original values.  If we reset
        // Attr.Style and Attr.ExStyle by extracting their values from
        // Windows, we will lose some of the style bits we supplied
        // in the CreateWindowEx call.  In the case of the ResourceId
        // constructors, of course, we must retrieve these values.

        if ( IsFlagSet(WB_FROMRESOURCE) )
        {
        Attr.Style = GetWindowLong(HWindow, GWL_STYLE);
        Attr.ExStyle = GetWindowLong(HWindow, GWL_EXSTYLE);
        }

        // Retrieve Attr.X, Attr.Y, Attr.W and Attr.H
        RECT WndRect;
        GetWindowRect(HWindow, &WndRect);
        Attr.H = WndRect.bottom - WndRect.top;
        Attr.W = WndRect.right - WndRect.left;
        if ( HParent && ((Attr.Style & WS_CHILD) == WS_CHILD) )
          ScreenToClient(HParent, (LPPOINT)&WndRect);
        Attr.X = WndRect.left;
        Attr.Y = WndRect.top;

	  SetFlags(WB_PREDEFINEDCLASS, TRUE);
	DefaultProc =  (WNDPROC)SetWindowLong(HWindow, GWL_WNDPROC,
                                          (DWORD)GetInstance());
        SetupWindow();
      }
    }
    else                              // HWindow == 0
    {
      if ( Attr.Menu )
	DestroyMenu( HMENU( MenuOrId ));
      Status = EM_INVALIDWINDOW;
    }
  }
  return (Status == 0);
}

/* Response method for an incoming WM_MDIACTIVATE message. Calls
   ActivationResponse to provide a keyboard interface for the controls
   of a window. */
void TWindow::WMMDIActivate(TMessage& Msg)
{
  DefWndProc(Msg);
  if ( Parent )
    ((PTMDIFrame)Parent)->ActiveChild = (Msg.WParam ? this : NULL);
  if ( !IsFlagSet(WB_ALIAS) )
  {
    ActivationResponse(Msg.WParam, IsIconic(HWindow));
  }
}

/* Calls TWindowsObject::ActivationResponse to enable or disable the
   "keyboard handler". If the TWindow has requested keyboard handling
   for its messages, saves the child with the focus if it is being
   deactivated and restores the focus to this child when the TWindow
   is reactivated. */
void TWindow::ActivationResponse(WORD Activated, BOOL IsIconified)
{
  HWND CurrentFocus;

  TWindowsObject::ActivationResponse(Activated, IsIconified);
  if ( IsFlagSet(WB_KBHANDLER) )
  {
    if ( Activated )
    {
      if ( FocusChildHandle && IsWindow(FocusChildHandle) &&
             !IsIconified )
      {
        SetFocus(FocusChildHandle);
        // Set to 0 so we won't reset focus if another activate comes
        FocusChildHandle = 0;
      }
    }
    else
    {
      CurrentFocus = GetFocus();
      if ( !FocusChildHandle && CurrentFocus &&
           IsChild(HWindow, CurrentFocus) )
        FocusChildHandle = CurrentFocus;
    }
  }
}

static BOOL HasStyle(void *P, void *Style)
{
  return ((PTWindowsObject)P)->HWindow &&
    (GetWindowLong(((PTWindowsObject)P)->HWindow, GWL_STYLE) &
       *(long *)Style) == *(long *)Style;
}

/* Initializes ("sets up") the TWindow.  Called following a
   successful association between an MS-Windows interface element
   and a TWindow.  Calls TWindowsObject::SetupWindow to create windows
   in child list. If keyboard handling has been requested and
   FocusChildHandle has not been set, sets it to the first child.
   Sets the focus to TWindows created as MDI children.  If the
   TWindow has a TScroller object, calls the TScroller's
   SetSBarRange to set the range of the TWindow's window scrollbars.
   Can be redefined in descendant classes to perform additional
   initialization. */
void TWindow::SetupWindow()
{
  TWindowsObject::SetupWindow();

  if ( IsFlagSet(WB_KBHANDLER) && !FocusChildHandle )
  {
    long Style = WS_TABSTOP;
    PTWindowsObject TmpWO;
    TmpWO = FirstThat(HasStyle, &Style);
    if ( !TmpWO ) // no child has WS_TABSTOP
    {
      Style = WS_CHILD;
      TmpWO = FirstThat(HasStyle, &Style);
    }
    if ( TmpWO )
      FocusChildHandle = TmpWO->HWindow;
  }

  if ( IsFlagSet(WB_MDICHILD) )
    SetFocus(HWindow);
  if ( Scroller )
    Scroller->SetSBarRange();
}

void TWindow::WMCreate(TMessage& Msg)
{
  SetupWindow();
  DefWndProc(Msg);
}

/* Response method for an incoming WM_HSCROLL message.  If the
   message is from a scrollbar control, calls DispatchScroll directly to
   avoid calling TWindowsObject::WMHScroll so that GetWindowLong is
   called only once. Else passes the message to the TWindow's Scroller
   if it has been constructed, and calls DefWndProc. Assumes because of
   a Windows bug that if the window has the scrollbar style, it will not
   have scrollbar controls. */
void TWindow::WMHScroll(TMessage& Msg)
{
  if ( !(GetWindowLong(HWindow, GWL_STYLE) & WS_HSCROLL) )
    DispatchScroll(Msg);
  else
    if ( Scroller )
      Scroller->HScroll(Msg.WParam, LOWORD(Msg.LParam));
    else
      DefWndProc(Msg);
}

/* Response method for an incoming WM_VSCROLL message.  If the
   message is from a scrollbar control, calls DispatchScroll directly to
   avoid calling TWindowsObject::WMVScroll so that GetWindowLong is
   called only once. Else passes the message to the TWindow's Scroller
   if it has been constructed, and calls DefWndProc. Assumes because of
   a Windows bug that if the window has the scrollbar style, it will not
   have scrollbar controls. */
void TWindow::WMVScroll(TMessage& Msg)
{
  if ( !(GetWindowLong(HWindow, GWL_STYLE) & WS_VSCROLL) )
    DispatchScroll(Msg);
  else
    if ( Scroller )
      Scroller->VScroll(Msg.WParam, LOWORD(Msg.LParam));
    else
      DefWndProc(Msg);
}

/* Response method for an incoming WM_PAINT message. Calls Paint,
  performing Windows-required paint setup and cleanup before and after.
  (If the TWindow has a TScroller, also calls its BeginView and EndView
  methods before and after call to Paint. */
void TWindow::WMPaint(TMessage& Msg)
{
  PAINTSTRUCT PaintInfo;

  if ( IsFlagSet(WB_ALIAS) ) // use application-defined wndproc
    DefWndProc(Msg);
  else
  {
    BeginPaint(HWindow, &PaintInfo);
    if ( Scroller )
      Scroller->BeginView(PaintInfo.hdc, PaintInfo);
    Paint(PaintInfo.hdc, PaintInfo);
    if ( Scroller )
      Scroller->EndView();
    EndPaint(HWindow, &PaintInfo);
  }
}

/* Redraws the contents of the TWindow after a WMPaint message
   is received. Placeholder for descendant object types to redefine. */

#ifdef WIN31
void TWindow::Paint(HDC, PAINTSTRUCT _FAR &)
{ }
#endif

#ifdef WIN30
void TWindow::Paint(HDC_30, PAINTSTRUCT _FAR &)
{ }
#endif

/* Response method for an incoming WM_SIZE message.  Calls the
   SetPageSize and SetSBarRange methods of the TWindow's Scroller,
   if it has been constructed. Also saves the normal size of the
   window in Attr. */
void TWindow::WMSize(TMessage& Msg)
{
  RECT WndRect;

  if ( Scroller && (Msg.WParam != SIZEICONIC) )
  {
    Scroller->SetPageSize();
	Scroller->SetSBarRange();
  }
  if ( Msg.WParam == SIZENORMAL )
  {
    GetWindowRect(HWindow, &WndRect);
    Attr.H = WndRect.bottom - WndRect.top;
    Attr.W = WndRect.right - WndRect.left;
  }
  DefWndProc(Msg);
}

/* Save the normal position of the window.  If IsIconic and IsZoomed
   ignore the position since it does not reflect the normal state. */
void TWindow::WMMove(TMessage& Msg)
{
  RECT WndRect;

  if ( !(IsIconic(HWindow) || IsZoomed(HWindow)) )
  {
    GetWindowRect(HWindow, &WndRect);
    if ( ((GetWindowLong(HWindow, GWL_STYLE) & WS_CHILD) == WS_CHILD)
            && Parent && Parent->HWindow )
      ScreenToClient(Parent->HWindow, (LPPOINT)&WndRect);
    Attr.X = WndRect.left;
    Attr.Y = WndRect.top;
  }
  DefWndProc(Msg);
}

/* Response method for an incoming WM_LBUTTONDOWN message.  If
   the TWindow's Scroller has been constructed and if auto-scrolling
   has been requested, captures mouse input, loops until a
   WM_LBUTTONUP message comes in calling the Scroller's AutoScroll
   method, and then releases capture on mouse input. */
void TWindow::WMLButtonDown(TMessage& Msg)
{
  MSG LoopMsg;

  LoopMsg.message = 0;
  if ( Scroller && Scroller->AutoMode )
  {
    SetCapture(HWindow);
    while ( LoopMsg.message != WM_LBUTTONUP )
    {
      if ( PeekMessage(&LoopMsg, 0, 0, 0, PM_REMOVE) )
      {
        TranslateMessage(&LoopMsg);
        DispatchMessage(&LoopMsg);
      }
      Scroller->AutoScroll();
    }
    ReleaseCapture();
  }
  DefWndProc(Msg);
}

void *TWindow::read(ipstream& is)
{
  BOOL NameIsNumeric;
  TWindowsObject::read(is);

  if ( IsFlagSet(WB_FROMRESOURCE) )
  {
    DefaultProc = (WNDPROC)DefWindowProc;
    memset(&Attr, 0x0, sizeof (Attr));
  }
  else
  {
    is >> Attr.Style >> Attr.ExStyle
       >> Attr.X >> Attr.Y >> Attr.W
       >> Attr.H >> (long)(Attr.Param);

    if ( IsFlagSet(WB_MDICHILD) )
      DefaultProc = (WNDPROC)DefMDIChildProc;
    else
      DefaultProc = (WNDPROC)DefWindowProc;
  }
  is >> Attr.Id;

  is >> NameIsNumeric;
  if ( NameIsNumeric )
    is >> (long)(Attr.Menu);
  else
    Attr.Menu= is.freadString();

  is >> Scroller;
  if ( Scroller )
    Scroller->Window = this;
  FocusChildHandle = 0;
  return this;
}

/* Writes data of the TWindow to the passed opstream. Writes its
   Scroller if constructed. */
void TWindow::write(Ropstream os)
{
  long SaveStyle;
  BOOL NameIsNumeric;
  TWindowsObject::write(os);
  if ( !IsFlagSet(WB_FROMRESOURCE) )
  {
    SaveStyle = Attr.Style & ~(WS_MINIMIZE | WS_MAXIMIZE);
    if ( HWindow )
      if ( IsIconic(HWindow) )
        SaveStyle |= WS_MINIMIZE;
      else
        if ( IsZoomed(HWindow) )
          SaveStyle |= WS_MAXIMIZE;
    os << SaveStyle << Attr.ExStyle << Attr.X
       << Attr.Y << Attr.W << Attr.H
       << (long)(Attr.Param);
  }
  os << Attr.Id;

  NameIsNumeric = HIWORD(Attr.Menu) == NULL;
  os << NameIsNumeric;
  if ( NameIsNumeric )
    os << (long)(Attr.Menu);
  else
    os.fwriteString(Attr.Menu);

  os << Scroller;
}

PTStreamable TWindow::build()
{
  return new TWindow(streamableInit);
}

TStreamableClass RegWindow("TWindow", TWindow::build, __DELTA(TWindow));
