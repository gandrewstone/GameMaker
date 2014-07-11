// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  CONTROL.CPP
  Defines type TControl.  This defines the basic behavior
  of all controls.
  -------------------------------------------------------- */

#include "control.h"
#include <alloc.h>   // for farfree

/* Constructor for a TControl.  Sets creation attributes using
   the parameters passed and default values. */
TControl::TControl(PTWindowsObject AParent, int AnId, LPSTR ATitle,
                   int X, int Y, int W, int H, PTModule AModule)
                      : TWindow(AParent, ATitle, AModule)
{
  Attr.Id = AnId;
  Attr.X = X;
  Attr.Y = Y;
  Attr.W = W;
  Attr.H = H;
  Attr.Style = WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP;
  SetFlags(WB_MDICHILD, FALSE);   // in case flag was set in TWindow()
}

/* Constructor for a TControl to be associated with a MS-Windows
  interface element created by MS-Windows from a resource definition.
  Initializes its data fields using passed parameters.  Data transfer
  is enabled for the TControl. */
TControl::TControl(PTWindowsObject AParent, int ResourceId, PTModule AModule)
         : TWindow(AParent, (LPSTR)NULL, AModule)
{
  if ( HIWORD(Title) )
    farfree(Title); // Free memory allocated by TWindow's constructor
  Title = NULL;
  SetFlags(WB_FROMRESOURCE, TRUE);
  memset(&Attr, 0x0, sizeof Attr);
  Attr.Id = ResourceId;
  Scroller = NULL;
  FocusChildHandle = 0;
  EnableTransfer();
}

/* Response method for an incoming WM_PAINT message. If this is a regular
   control call DefWndProc. If this is a custom control call
   TWindow::WMPaint */
void TControl::WMPaint(TMessage& Msg)
{
  if ( IsFlagSet(WB_PREDEFINEDCLASS) )   // predefined Windows class
    DefWndProc(Msg);
  else
    TWindow::WMPaint(Msg);
}

void TControl::WMDrawItem(TMessage& Msg)
{
  switch ( ((LPDRAWITEMSTRUCT)(Msg.LParam))->itemAction )
  {
    case ODA_DRAWENTIRE:
         ODADrawEntire(*((LPDRAWITEMSTRUCT)(Msg.LParam)));
         break;
    case ODA_FOCUS:
         ODAFocus(*((LPDRAWITEMSTRUCT)(Msg.LParam)));
         break;
    case ODA_SELECT:
         ODASelect(*((LPDRAWITEMSTRUCT)(Msg.LParam)));
         break;
  }
}

/* Function called when an "owner-draw" control needs to be redrawn
   Will usually be redefined by descendants of TControl which use owner draw
   style */
void TControl::ODADrawEntire(DRAWITEMSTRUCT far & DrawInfo)
{
  if (Parent)
    Parent->DrawItem(DrawInfo);
}

/* Function called when an "owner-draw" control gains or loses focus
   Will usually be redefined by descendants of TControl which use owner draw
   style */
void TControl::ODAFocus(DRAWITEMSTRUCT far & DrawInfo)
{
  if (Parent)
    Parent->DrawItem(DrawInfo);
}

/* Function called when an "owner-draw" control's selection status changes
   Will usually be redefined by descendants of TControl which use owner draw
   style */
void TControl::ODASelect(DRAWITEMSTRUCT far & DrawInfo)
{
  if (Parent)
    Parent->DrawItem(DrawInfo);
}
