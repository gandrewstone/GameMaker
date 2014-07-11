// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  BUTTON.CPP
  Defines type TButton.  This defines the basic behavior
  of all buttons.
  -------------------------------------------------------- */

#include "button.h"

/* Constructor for a TButton object.  Initializes its data members using
  parameters passed and default values. */
TButton::TButton(PTWindowsObject AParent, int AnId, LPSTR AText, int X,
                 int Y, int W, int H, BOOL IsDefault, PTModule AModule)
                      : TControl(AParent, AnId, AText, X, Y, W, H, AModule)
{
  IsCurrentDefPB = FALSE;      // not used for buttons in windows
  IsDefPB = FALSE;             // not used for buttons in windows

  if ( IsDefault )
    Attr.Style |= BS_DEFPUSHBUTTON;
  else
    Attr.Style |= BS_PUSHBUTTON;
}

/* Constructor for a TButton to be associated with a MS-Windows
   interface element created by MS-Windows from a resource definition.
   Initializes its data members using passed parameters.  Disables
   transfer of state data for the TButton. */
TButton::TButton(PTWindowsObject AParent, int ResourceId, PTModule AModule)
                 : TControl(AParent, ResourceId, AModule)
{
  DisableTransfer();
  IsDefPB = FALSE;             // needed for drawable buttons
  IsCurrentDefPB = FALSE;  // needed for drawable buttons
}

/* If this is a drawable button which is supposed to act like a
   DefPushButton, send DM_SETDEFID to Parent to make this into
   Parent's default pushbutton. This only works (and IsDefPB should
   only be TRUE) if Parent is a dialog. */
void TButton::SetupWindow()
{
  if ( IsDefPB && ( (Attr.Style & BS_OWNERDRAW) == BS_OWNERDRAW ) )
    SendMessage(Parent->HWindow, DM_SETDEFID, Attr.Id, 0);
}

/* If this is a drawable button we tell Windows whether we want to
    be treated as the current default push button or not. */
void TButton::WMGetDlgCode(RTMessage Msg)
{
  if ( (Attr.Style & BS_OWNERDRAW) == BS_OWNERDRAW )
  {
    if ( IsCurrentDefPB )
      Msg.Result = DLGC_BUTTON | DLGC_DEFPUSHBUTTON;
    else
      Msg.Result = DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
  }
  else
    DefWndProc(Msg);
}

/* A Button can't have both BS_OWNERDRAW and BS_(DEF)PUSHBUTTON
   styles so when Windows tries to make us a DEF- or non-DEFPUSHBUTTON
   we keep track of the desired style in IsCurrentDefPB. */
void TButton::BMSetStyle(RTMessage Msg)
{
   if ( (Attr.Style & BS_OWNERDRAW) == BS_OWNERDRAW )
   {
     if ( Msg.WParam == BS_DEFPUSHBUTTON )
     {
       IsCurrentDefPB = TRUE;
       InvalidateRect(HWindow, 0L, 0);
     }
     else
      if ( Msg.WParam == BS_PUSHBUTTON )
      {
        IsCurrentDefPB = FALSE;
        InvalidateRect(HWindow, 0L, 0);
      }
      else
        DefWndProc(Msg);
    }
    else
      DefWndProc(Msg);
}

void *TButton::read(ipstream& is)
{
  TControl::read(is);
  is >> IsDefPB;
  return this;
}

void TButton::write(Ropstream os)
{
  TControl::write(os);
  os << IsDefPB;
}

TStreamable *TButton::build()
{
  return new TButton(streamableInit);
}

TStreamableClass RegButton("TButton", TButton::build, __DELTA(TButton));
