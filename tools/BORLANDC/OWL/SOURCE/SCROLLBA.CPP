// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  SCROLLBA.CPP
  Defines type TScrollBar.  This defines the basic
  behavior of all scrollbar controls.
  -------------------------------------------------------- */

#include "scrollba.h"

/* Constructor for a TScrollBar object.  Initializes its data members
  (including its creation attributes) using parameters passed and
  default values. If the size attribute (H for horizontal scrollbars,
  W for vertical) is zero, the attribute is set to the appropriate
  system metric. */
TScrollBar::TScrollBar(PTWindowsObject AParent, int AnId, int X, int Y,
                       int W, int H, BOOL IsHScrollBar, PTModule AModule)
           : TControl(AParent, AnId, NULL, X, Y, W, H, AModule)
{
  LineMagnitude = 1;
  PageMagnitude = 10;
  if ( IsHScrollBar )
  {
    Attr.Style |= SBS_HORZ;
    if ( Attr.H == 0 )
      Attr.H = GetSystemMetrics(SM_CYHSCROLL);
  }
  else
  {
    Attr.Style |= SBS_VERT;
    if ( Attr.W == 0 )
      Attr.W = GetSystemMetrics(SM_CXVSCROLL);
  }
}

TScrollBar::TScrollBar(PTWindowsObject AParent, int ResourceId, PTModule AModule)
                       : TControl(AParent, ResourceId, AModule)
{
  LineMagnitude = 1;
  PageMagnitude = 10;
}

/* Transfers state information for a TScrollbar. The TransferFlag passed
   specifies whether data is to be read from or written to the passed
  buffer, or whether the data element size is simply to be returned.
  The return value is the size (in bytes) of the transfer data. */
WORD TScrollBar::Transfer(Pvoid DataPtr, WORD TransferFlag)
{
  TScrollBarData *NewPtr = (TScrollBarData *)DataPtr;

  if ( TransferFlag == TF_GETDATA )
  {
    GetRange(NewPtr->LowValue, NewPtr->HighValue);
    NewPtr->Position = GetPosition();
  }
  else if ( TransferFlag == TF_SETDATA )
  {
    SetRange(NewPtr->LowValue, NewPtr->HighValue);
    SetPosition(NewPtr->Position);
  }
  return sizeof(TScrollBarData);
}

/* Sets up an associated scrollbar by setting its range to 0..100. */
void TScrollBar::SetupWindow()
{
  SetRange(0, 100);
  TControl::SetupWindow();
}

/* Retrieves the range of values that the associated scrollbar can
  return. */
void TScrollBar::GetRange(int& LoVal, int& HiVal)
{
  GetScrollRange(HWindow, SB_CTL, &LoVal, &HiVal);
}

/* Returns the position of the thumb of the associated scrollbar. */
int TScrollBar::GetPosition()
{
  return GetScrollPos(HWindow, SB_CTL);
}

/* Sets the range of values that the associated scrollbar can return. */
void TScrollBar::SetRange(int LoVal, int HiVal)
{
  SetScrollRange(HWindow, SB_CTL, LoVal, HiVal, FALSE);
}

/* Sets the position of the thumb of the associated scrollbar. */
void TScrollBar::SetPosition(int ThumbPos)
{
  int LoVal , HiVal;

  GetRange(LoVal, HiVal);
  if ( ThumbPos > HiVal )
    ThumbPos = HiVal;
  else if ( ThumbPos < LoVal )
    ThumbPos = LoVal;
  if ( ThumbPos != GetPosition() )
    SetScrollPos(HWindow, SB_CTL, ThumbPos, TRUE);
}

/* Changes the position (by Delta) of the thumb of the associated
  scrollbar.  Returns the new position. */
int TScrollBar::DeltaPos(int Delta)
{
    if ( Delta != 0 )
	SetPosition(GetPosition() + Delta);
    return GetPosition();
}

/* Responds to an SB_LINEUP notification message which the associated
  scrollbar sent to its parent.  Changes the position (by LineMagnitude)
  of the thumb of the associated scrollbar. */
void TScrollBar::SBLineUp(TMessage&)
{
    DeltaPos(0 - LineMagnitude);
}

/* Responds to an SB_LINEDOWN notification message which the associated
  scrollbar sent to its parent.  Changes the position (by LineMagnitude)
  of the thumb. */
void TScrollBar::SBLineDown(TMessage&)
{
    DeltaPos(LineMagnitude);
}

/* Responds to an SB_PAGEUP notification message which the associated
  scrollbar sent to its parent.  Changes the position (by PageMagnitude)
  of the thumb. */
void TScrollBar::SBPageUp(TMessage&)
{
    DeltaPos(0 - PageMagnitude);
}

/* Responds to an SB_PAGEDOWN notification message which the associated
  scrollbar sent to its parent.  Changes the position (by PageMagnitude)
  of the thumb. */
void TScrollBar::SBPageDown(TMessage&)
{
    DeltaPos(PageMagnitude);
}

/* Responds to an SB_THUMBPOSITION notification message which the
  associated scrollbar sent to its parent.  Moves the thumb of the
  scrollbar to the new position. */
void TScrollBar::SBThumbPosition(TMessage& Msg)
{
    SetPosition(LOWORD(Msg.LParam));
}

/* Responds to an SB_THUMBTRACK notification message which the
   associated scrollbar sent to its parent.  Draws the thumb in the
   current position on the track. */
void TScrollBar::SBThumbTrack(TMessage& Msg)
{
    SetPosition(LOWORD(Msg.LParam));
}

/* Responds to an SB_TOP notification message which the associated
  scrollbar sent to its parent.  Moves the thumb to the top of the
  scrollbar. */
void TScrollBar::SBTop(TMessage&)
{
    int Lo, Hi;

    GetRange(Lo, Hi);
    SetPosition(Lo);
}

/* Responds to an SB_BOTTOM notification message which the associated
  scrollbar sent to its parent.  Moves the thumb to the bottom of the
  scrollbar. */
void TScrollBar::SBBottom(TMessage&)
{
    int Lo , Hi;

    GetRange(Lo, Hi);
    SetPosition(Hi);
}

/* Reads an instance of TScrollBar from the passed ipstream. */
void *TScrollBar::read(ipstream& is)
{
  TWindow::read(is);
  is >> LineMagnitude
     >> PageMagnitude;
  return this;
}

/* Writes the TScrollBar to the passed opstream. */
void TScrollBar::write(opstream& os)
{
  TWindow::write(os);
  os << LineMagnitude
     << PageMagnitude;
}

TStreamable *TScrollBar::build()
{
  return new TScrollBar(streamableInit);
}

TStreamableClass RegScrollBar("TScrollBar", TScrollBar::build,
					  __DELTA(TScrollBar));
