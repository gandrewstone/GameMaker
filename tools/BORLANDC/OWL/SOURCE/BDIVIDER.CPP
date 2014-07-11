// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  BDIVIDER.CPP
  Defines type TBDivider.  This is a static control that draws
  a beveled dividing line
  -------------------------------------------------------- */

#include "bdivider.h"
#include "bwcc.h"

/* Constructor for a TBDivider object.  Initializes its data fields using
  parameters passed and default values. */

TBDivider::TBDivider(PTWindowsObject AParent, int AnId, LPSTR AText, int X,
		     int Y, int W, int H, BOOL IsVertical, BOOL IsBump,
		     PTModule AModule)
      : TControl(AParent, AnId, AText, X, Y, W, H, AModule)
{
  if ( IsVertical )
  {

    if (IsBump)
      Attr.Style |= BSS_VBUMP;
    else
      Attr.Style |= BSS_VDIP;
  }
  else
  {
    if (IsBump)
      Attr.Style |= BSS_HBUMP;
    else
      Attr.Style |= BSS_HDIP;
  }
}

/* Constructor for a TBDivider to be associated with a MS-Windows
   interface element created by MS-Windows from a resource definition.
   Initializes its data fields using passed parameters.  Disables
   transfer of state data for the TBDivider. */

TBDivider::TBDivider(PTWindowsObject AParent, int ResourceId,
		       PTModule AModule)
                 : TControl(AParent, ResourceId, AModule)
{
  DisableTransfer();
}

PTStreamable TBDivider::build()
{
  return new TBDivider(streamableInit);
}

TStreamableClass RegBDivider("TBDivider", TBDivider::build,
			      __DELTA(TBDivider));
