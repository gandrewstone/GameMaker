// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  MDIFRAME.CPP
  Defines type TMDIFrame.  This defines the basic behavior
  of all MDI frame windows.
  -------------------------------------------------------- */

#include "mdi.h"

__link(RegMDIClient)

/* Constructor for a TMDIFrame.  Initializes its data fields using
   passed parameters and default values. */
TMDIFrame::TMDIFrame(LPSTR ATitle, LPSTR MenuName, PTModule AModule)
          : TWindow(NULL, ATitle, AModule)
{
  AssignMenu(MenuName);
  ClientWnd = NULL;
  ChildMenuPos = 0;
  ActiveChild = NULL;
  SetFlags(WB_MDIFRAME, TRUE);
}

/* Constructor for a TMDIFrame.  Initializes its data fields using
   passed parameters and default values. */
TMDIFrame::TMDIFrame(LPSTR ATitle, int MenuId, PTModule AModule)
          : TWindow(NULL, ATitle, AModule)
{
  AssignMenu(MenuId);
  ClientWnd = NULL;
  ChildMenuPos = 0;
  ActiveChild = NULL;
  SetFlags(WB_MDIFRAME, TRUE);
}

/* Constructor for a TMDIFrame which is being used in a DLL as an alias
   for a non-OWL window */
TMDIFrame::TMDIFrame(HWND AnHWindow, HWND ClientHWnd, PTModule AModule)
          : TWindow(AnHWindow, AModule)
{
// Attr.Menu set in TWindow's constructor
  ChildMenuPos = 0;
  ActiveChild = NULL;
  ClientWnd = new TMDIClient(this, ClientHWnd);
  RemoveClient();   // remove ClientWnd from OWL child list
  SetFlags(WB_MDIFRAME | WB_ALIAS, TRUE);
}

/* Destructor for a TMDIFrame.  Disposes of the TMDIFrame's MDI client
  window. */
TMDIFrame::~TMDIFrame()
{
  if ( ClientWnd )
  {
    delete ClientWnd;
    ClientWnd = NULL;
  }
}

/* Sets up the TMDIFrame by constructing and creating its TMDIClient. */
void TMDIFrame::SetupWindow()
{
  HMENU FrameMenu;
  RECT R;

  InitClientWindow();
  RemoveClient();   // remove ClientWnd from OWL child list
  FrameMenu = GetMenu(HWindow);
  ClientWnd->ClientAttr->hWindowMenu = GetSubMenu(FrameMenu, ChildMenuPos);
  GetClientRect(HWindow, &R);
  if ( ClientWnd->Attr.X == CW_USEDEFAULT )
  {
    ClientWnd->Attr.X = R.left;
    ClientWnd->Attr.Y = R.top;
  }
  if ( ClientWnd->Attr.W == CW_USEDEFAULT )
  {
    ClientWnd->Attr.W = R.right - R.left;
    ClientWnd->Attr.H = R.bottom - R.top;
  }

  // allow client area to grow scroll bars if necessary.
  ClientWnd->Attr.Style |= WS_VSCROLL + WS_HSCROLL;

  if ( ClientWnd->Create() )
    TWindow::SetupWindow();
  else
    Status = EM_INVALIDCLIENT;
}

/* Specifies registration attributes for the MS-Windows window class
   of the TMDIFrame.  Sets the fields of the passed WNDCLASS parameter
   to the default attributes appropriate for a TMDIFrame. */
void TMDIFrame::GetWindowClass(WNDCLASS& AWndClass)
{
  TWindow::GetWindowClass(AWndClass);
  AWndClass.style = 0;
}

/* Creates a valid new MDI child window after calling InitChild
   to construct a new MDI child window object. */
PTWindowsObject TMDIFrame::CreateChild()
{
  return GetModule()->MakeWindow(InitChild());
}

static BOOL CannotClose(void *P, void *)
{
  if ( ((PTWindowsObject)P)->IsFlagSet(WB_MDICHILD) )
    return !((PTWindowsObject)P)->CanClose();
  else
    return FALSE;
}

static void CloseChild(void *AChild, void *)
{
  if ( ((PTWindowsObject)AChild)->IsFlagSet(WB_MDICHILD) )
    ((PTWindowsObject)AChild)->ShutDownWindow();
}

/* Closes each MDI child, after calling the child's CanClose method to
  ensure that it is Ok to do so. Returns TRUE if all children are closed
  (or there were no children), FALSE if any child can't be closed */
BOOL TMDIFrame::CloseChildren()
{
  if ( !FirstThat(CannotClose, NULL) ) // All children can be closed
  {
    ForEach(CloseChild, NULL);
    return TRUE;
  }
  return FALSE;
}

/* Reads an instance of TMDIFrame from the passed ipstream. */
void *TMDIFrame::read(ipstream& is)
{
  TWindow::read(is);

  GetChildPtr(is, (PTWindowsObject)ActiveChild);

  is >> ClientWnd;
  if ( ClientWnd )
    ClientWnd->Parent = this;
  is >> ChildMenuPos;
  return this;
}

/* Writes the TMDIFrame to the passed opstream. */
void TMDIFrame::write(opstream& os)
{
  TWindow::write(os);

  PutChildPtr(os, ActiveChild);

  os << ClientWnd;
  os << ChildMenuPos;
}

TStreamable *TMDIFrame::build()
{
  return new TMDIFrame(streamableInit);
}

TStreamableClass RegMDIFrame("TMDIFrame", TMDIFrame::build,
					  __DELTA(TMDIFrame));
