// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  GROUPBOX.CPP
  Defines type TGroupBox.  This defines the basic behavior
  for all group boxes.
  -------------------------------------------------------- */

#include "groupbox.h"

/* Constructor for a TGroupBox object.  Initializes its data fields
   using supplied parameters and default values.  By default, the
   parent window is notified when the state of the group box's selection
   boxes has changed.  */
TGroupBox::TGroupBox(PTWindowsObject AParent, int AnId, LPSTR AText,
                     int X, int Y, int W, int H, PTModule AModule)
			: TControl(AParent, AnId, AText, X, Y, W, H, AModule)
{
  NotifyParent = TRUE;
  Attr.Style = (Attr.Style | BS_GROUPBOX) & ~WS_TABSTOP;
}

/* Constructor for a TGroupBox to be associated with a MS-Windows interface
   element created by MS-Windows from a resource definition. Initializes
   its data fields using supplied parameters.  By default, the
   parent window is notified when the state of the group box's selection
   boxes has changed.Disables transfer of state data for the TGroupBox. */
TGroupBox::TGroupBox(PTWindowsObject AParent, int ResourceId, PTModule AModule)
                     : TControl(AParent, ResourceId, AModule)
{
  NotifyParent = TRUE;
  DisableTransfer();
}

/* Notifies parent that the selection in the associated groupbox has
  changed.  This method is called by TCheckBoxes grouped in the groupbox
  when their state changes. */
void TGroupBox::SelectionChanged(int ControlId)
{
  if ( NotifyParent )
    SendMessage(Parent->HWindow, WM_COMMAND, Attr.Id,
      MAKELONG(HWindow, ControlId));
}

/* Reads an instance of TGroupBox from the passed ipstream. */
void *TGroupBox::read(ipstream& is)
{
  TWindow::read(is);
  is >> NotifyParent;
  return this;
}

/* Writes the TGroupBox to the passed opstream. */
void TGroupBox::write(opstream& os)
{
  TWindow::write(os);
  os << NotifyParent;
  }

TStreamable *TGroupBox::build()
{
  return new TGroupBox(streamableInit);
}

TStreamableClass RegGroupBox("TGroupBox", TGroupBox::build,
			     __DELTA(TGroupBox));
