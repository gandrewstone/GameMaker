// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  BSTATBMP.CPP
  Defines type TBStaticBmp.  This is a static control that draws
  a bitmap
  -------------------------------------------------------- */

#include "bstatbmp.h"
#include "bwcc.h"

/* Constructor for a TBStaticBmp object.  Initializes its data fields using
  parameters passed and default values. */

TBStaticBmp::TBStaticBmp(PTWindowsObject AParent, int AnId, LPSTR AText,
			   int X, int Y, int W, int H, PTModule AModule)
                      : TControl(AParent, AnId, AText, X, Y, W, H, AModule)
{
  Attr.Style |= BBS_BITMAP;
  DisableTransfer();
}

/* Constructor for a TBStaticBmp to be associated with a MS-Windows
   interface element created by MS-Windows from a resource definition.
   Initializes its data fields using passed parameters.  Disables
   transfer of state data for the TBStaticBmp. */

TBStaticBmp::TBStaticBmp(PTWindowsObject AParent, int ResourceId,
			   PTModule AModule)
                 : TControl(AParent, ResourceId, AModule)
{
  DisableTransfer();
}

PTStreamable TBStaticBmp::build()
{
  return new TBStaticBmp(streamableInit);
}

TStreamableClass RegBStaticBmp("TBStaticBmp", TBStaticBmp::build,
				__DELTA(TBStaticBmp));
