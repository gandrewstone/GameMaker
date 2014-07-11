// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  STATIC.CPP
  Defines type TStatic.  This defines the basic behavior
  of all static controls.
  -------------------------------------------------------- */

#include "static.h"

/* Constructor for a TStatic object.  Initializes its data
   fields using passed parameters and default values.   By
   default, an associated static control will have
   left-justified text. */
TStatic::TStatic(PTWindowsObject AParent, int AnId, LPSTR ATitle,
                 int X, int Y, int W, int H, WORD ATextLen, PTModule AModule)
        : TControl(AParent, AnId, ATitle, X, Y, W, H, AModule)
{
  TextLen = ATextLen;
  Attr.Style = (Attr.Style | SS_LEFT) & ~WS_TABSTOP;
}

/* Constructor for a TStatic to be associated with a MS-Windows
  interface element created by MS-Windows from a resource definition.
  Initializes its data fields using passed parameters.  Data transfer
  is disabled, by default, for the TStatic. */
TStatic::TStatic(PTWindowsObject AParent, int ResourceId, WORD ATextLen,
                 PTModule AModule)
        : TControl(AParent, ResourceId, AModule)
{
  TextLen = ATextLen;
  DisableTransfer();
}

/* Transfers state information for TStatic controls. The TransferFlag passed
  specifies whether data is to be read from or written to the passed
  buffer, or whether the data element size is simply to be returned. The
  return value is the size (in bytes) of the transfer data. */
WORD TStatic::Transfer(Pvoid DataPtr,WORD TransferFlag)
{
  if ( TransferFlag == TF_GETDATA )
    GetText((LPSTR)DataPtr, TextLen);
  else
    if ( TransferFlag == TF_SETDATA )
      SetText((LPSTR)DataPtr);
  return TextLen;
}

/* Reads an instance of TStatic from the passed ipstream. */
void *TStatic::read(ipstream& is)
{
  TWindow::read(is);
  is >> TextLen;
  return this;
}

/* Writes the TStatic to the passed opstream. */
void TStatic::write(opstream& os)
{
  TWindow::write(os);
  os << TextLen;
}

TStreamable *TStatic::build()
{
  return new TStatic(streamableInit);
}

TStreamableClass RegStatic("TStatic", TStatic::build, __DELTA(TStatic));
