// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  CHECKBOX.CPP
  Defines type TCheckBox.  This defines the basic behavior
  for all check boxes.
  -------------------------------------------------------- */

#include "checkbox.h"
#include "groupbox.h"

/* Constructor for a TCheckBox object.  Initializes its data
   fields using passed parameters and default values. */
TCheckBox::TCheckBox(PTWindowsObject AParent, int AnId, LPSTR ATitle,
                     int X, int Y, int W, int H, PTGroupBox AGroup,
                     PTModule AModule)
               : TButton(AParent, AnId, ATitle, X, Y, W, H, FALSE, AModule)
{
  Group = AGroup;
/* Don't use TButton's inherited style - it conflicts with
   BS_AUTOCHECKBOX. */
  Attr.Style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX;
}

/* Constructor for a TCheckBox to be associated with a MS-Windows
  interface element created by MS-Windows from a resource definition.
  Initializes its data fields using passed parameters.  Data transfer
  is enabled for the TCheckBox. */
TCheckBox::TCheckBox(PTWindowsObject AParent, int ResourceId,
                     PTGroupBox AGroup, PTModule AModule)
               : TButton(AParent, ResourceId, AModule)
{
  Group = AGroup;
  EnableTransfer();
}

/* Transfers state information for the TCheckBox. The TransferFlag
  passed specifies whether data is to be read from or written to
  the passed buffer, or whether the data element size is simply to
  be returned. The return value is the size (in bytes) of the
  transfer data. */
WORD TCheckBox::Transfer(Pvoid DataPtr, WORD TransferFlag)
{
    WORD CheckFlag;

  if ( TransferFlag == TF_GETDATA )
  {
    CheckFlag = GetCheck();
    _fmemcpy(DataPtr, &CheckFlag, sizeof(CheckFlag));
  }
  else
    if ( TransferFlag == TF_SETDATA )
      SetCheck( *(WORD *)DataPtr );
  return sizeof(CheckFlag);
}

/* Returns the check state of the associated check box.  Returns
   BF_UNCHECKED (0), BF_CHECKED(1), or (if 3-state) BF_GRAYED(2). */
WORD TCheckBox::GetCheck()
{
  return (WORD)SendMessage(HWindow, BM_GETCHECK, 0, 0L);
}

/* Sets the check state of the associated check box.  Unchecks, checks,
   or grays the checkbox (if 3-state) according to the CheckFlag passed.
   (Pass BF_UNCHECKED (0), BF_CHECKED (1), or BF_GRAYED (2)). If a Group
   has been specified for the TCheckBox, notifies the Group that the
   state of the check box has changed. */
void TCheckBox::SetCheck(WORD CheckFlag)
{
  SendMessage(HWindow, BM_SETCHECK, CheckFlag, 0);
  if ( Group )
    Group->SelectionChanged(Attr.Id);
}

/* Places a checkmark in associated check box. */
void TCheckBox::Check()
{
  SetCheck(BF_CHECKED);
}

/* Removes a checkmark from the associated check box. */
void TCheckBox::Uncheck()
{
  SetCheck(BF_UNCHECKED);
}

/* Toggles the check state of the check box. */
void TCheckBox::Toggle()
{
    if ( (GetWindowLong(HWindow, GWL_STYLE) & BS_AUTO3STATE)
                                     == BS_AUTO3STATE )
	SetCheck((GetCheck() + 1) % 3);
    else
	SetCheck((GetCheck() + 1) % 2);
}

/* Responds to an incoming BN_CLICKED message.  If a Group has been
  specified for the TCheckBox, notifies the Group that the state of
  this TCheckBox has changed. */
void TCheckBox::BNClicked(TMessage& Msg)
{
  DefWndProc(Msg);
  if ( Group )
    Group->SelectionChanged(Attr.Id);
  DefNotificationProc(Msg);
}

/* Reads an instance of TCheckBox from the passed ipstream. */
void *TCheckBox::read(ipstream& is)
{
  TWindow::read(is);
  GetSiblingPtr(is, (PTWindowsObject)Group);
  return this;
}

/* Writes the TCheckBox to the passed opstream. */
void TCheckBox::write(opstream& os)
{
  TWindow::write(os);
  PutSiblingPtr(os, Group);
  }

TStreamable *TCheckBox::build()
{
  return new TCheckBox(streamableInit);
}

TStreamableClass RegCheckBox("TCheckBox", TCheckBox::build,
			     __DELTA(TCheckBox));
