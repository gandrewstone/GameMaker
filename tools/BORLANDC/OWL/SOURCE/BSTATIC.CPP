// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  BSTATIC.CPP
  Defines type TBStatic.  This defines the basic behavior
  of custom static control.
  -------------------------------------------------------- */

#include "bstatic.h"

/* Constructor for a TBStatic object.  Initializes its data
   fields using passed parameters and default values.   By
   default, an associated static control will have
   left-justified text. */

TBStatic::TBStatic(PTWindowsObject AParent, int AnId, LPSTR ATitle,
		   int X, int Y, int W, int H, WORD ATextLen,
		   PTModule AModule)
     : TStatic(AParent, AnId, ATitle, X, Y, W, H, ATextLen, AModule)
{}

/* Constructor for a TBStatic to be associated with a MS-Windows
  interface element created by MS-Windows from a resource definition.
  Initializes its data fields using passed parameters.  Data transfer
  is disabled, by default, for the TBStatic. */

TBStatic::TBStatic(PTWindowsObject AParent, int ResourceId, WORD ATextLen,
		     PTModule AModule)
                 : TStatic(AParent, ResourceId, ATextLen, AModule)
{}

PTStreamable TBStatic::build()
{
  return new TBStatic(streamableInit);
}

TStreamableClass RegBStatic("TBStatic", TBStatic::build,
			     __DELTA(TBStatic));
