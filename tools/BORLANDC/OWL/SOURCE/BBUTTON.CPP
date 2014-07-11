// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  BBUTTON.CPP
  Defines type TBButton.  This defines custom control push button
  -------------------------------------------------------- */

#include "bbutton.h"
#include "bwcc.h"

/* Constructor for a TBButton object.  Initializes its data fields using
  supplied parameters and default values. */
TBButton::TBButton(PTWindowsObject AParent, int AnId, LPSTR AText, int X,
                 int Y, int W, int H, BOOL IsDefault, PTModule AModule)
                      : TButton(AParent, AnId, AText, X, Y, W, H, IsDefault,
			        AModule)
{
}

/* Constructor for a TBButton to be associated with a MS-Windows
   interface element created by MS-Windows from a resource definition.
   Initializes its data fields using supplied parameters. */
TBButton::TBButton(PTWindowsObject AParent, int ResourceId, PTModule AModule)
                 : TButton(AParent, ResourceId, AModule)
{
}

PTStreamable TBButton::build()
{
  return new TBButton(streamableInit);
}

TStreamableClass RegBButton("TBButton", TBButton::build, __DELTA(TBButton));
