// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  BRADIO.CPP
  Defines type TBRadioButton.  This defines the basic behavior
  for all Borland custom radio buttons.
  -------------------------------------------------------- */

#include "bradio.h"
#include "groupbox.h"

/* Constructor for a TBRadioButton object.  */

TBRadioButton::TBRadioButton(PTWindowsObject AParent, int AnId,
			       LPSTR ATitle, int X, int Y, int W,
			       int H, PTGroupBox AGroup, PTModule AModule)
          : TRadioButton(AParent, AnId, ATitle, X, Y, W, H, AGroup, AModule)
{}

PTStreamable TBRadioButton::build()
{
  return new TBRadioButton(streamableInit);
}

TStreamableClass RegBRadioButton("TBRadioButton", TBRadioButton::build,
				              __DELTA(TBRadioButton));
