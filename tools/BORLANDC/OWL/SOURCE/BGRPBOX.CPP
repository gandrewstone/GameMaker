// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  BGRPBOX.CPP
  Defines type TBGroupBox.  This defines the basic behavior
  for all custom control group boxes.
  -------------------------------------------------------- */

#include "bgrpbox.h"
#include "bwcc.h"

/* Constructor for a TBGroupBox object.  Initializes its data fields
   using parameters passed and default values. */

TBGroupBox::TBGroupBox(PTWindowsObject AParent, int AnId, LPSTR AText,
		       int X, int Y, int W, int H, PTModule AModule)
	: TGroupBox(AParent, AnId, AText, X, Y, W, H, AModule)
{
  Attr.Style = (Attr.Style  & ~BS_GROUPBOX) | BSS_GROUP;
}

/* Constructor for a TBGroupBox to be associated with a MS-Windows interface
  element created by MS-Windows from a resource definition. Initializes
  its data fields using passed parameters.  */

TBGroupBox::TBGroupBox(PTWindowsObject AParent, int ResourceId,
		       PTModule AModule)
        : TGroupBox(AParent, ResourceId, AModule)
{}

PTStreamable TBGroupBox::build()
{
  return new TBGroupBox(streamableInit);
}

TStreamableClass RegBGroupBox("TBGroupBox", TBGroupBox::build,
			      __DELTA(TBGroupBox));
