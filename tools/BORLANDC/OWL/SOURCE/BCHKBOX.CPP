// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  BCHKBOX.CPP
  Defines type TBCheckBox.
  -------------------------------------------------------- */

#include "bchkbox.h"
#include "groupbox.h"

/* Constructor for a TBCheckBox object.  */

TBCheckBox::TBCheckBox(PTWindowsObject AParent, int AnId, LPSTR ATitle,
                     int X, int Y, int W, int H, PTGroupBox AGroup,
		     PTModule AModule)
               : TCheckBox(AParent, AnId, ATitle, X, Y, W, H, AGroup,
			   AModule)
{}

/* Constructor for a TBCheckBox to be associated with a MS-Windows
  interface element created by MS-Windows from a resource definition. */

TBCheckBox::TBCheckBox(PTWindowsObject AParent, int ResourceId,
			 PTGroupBox AGroup, PTModule AModule)
                 : TCheckBox(AParent, ResourceId, AGroup, AModule)
{}

PTStreamable TBCheckBox::build()
{
  return new TBCheckBox(streamableInit);
}

TStreamableClass RegBCheckBox("TBCheckBox", TBCheckBox::build,
			       __DELTA(TBCheckBox));
