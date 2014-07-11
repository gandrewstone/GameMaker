// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  RADIOBUT.CPP
  Defines type TRadioButton.  This defines the basic behavior
  for all radio buttons.
  -------------------------------------------------------- */

#include "radiobut.h"
#include "groupbox.h"

/* Constructor for a TRadioButton object.  Initializes its data fields
  using passed parameters and default values. */
TRadioButton::TRadioButton(PTWindowsObject AParent, int AnId, LPSTR
                           ATitle, int X, int Y, int W, int H,
                           PTGroupBox AGroup, PTModule AModule)
             : TCheckBox(AParent, AnId, ATitle, X, Y, W, H, AGroup, AModule)
{
  Attr.Style = WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON;
}

/* Responds to an incoming BN_CLICKED message. Need to see if it's
   checked because Windows generates two BN_CLICKED messages on
   keyboard input such as up arrow (but only one on mouse input),
   and we should only handle the one after it's checked. */
void TRadioButton::BNClicked(TMessage& Msg)
{
  if ( GetCheck() )
    TCheckBox::BNClicked(Msg);
  else
    DefNotificationProc(Msg);	
}

TStreamable *TRadioButton::build()
{
  return new TRadioButton(streamableInit);
}

TStreamableClass RegRadioButton("TRadioButton", TRadioButton::build,
				              __DELTA(TRadioButton));
