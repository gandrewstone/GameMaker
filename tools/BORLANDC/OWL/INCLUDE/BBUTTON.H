// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __BBUTTON_H
#define __BBUTTON_H

#ifndef __BUTTON_H
#include <button.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

/* --------------------------------------------------------
  TBButton object
  -------------------------------------------------------- */

_CLASSDEF(TBButton)

class _EXPORT TBButton : public TButton
{
public:
    TBButton(PTWindowsObject AParent, int AnId, LPSTR AText,
            int X, int Y, int W, int H, BOOL IsDefault,
	    PTModule AModule = NULL);
    TBButton(PTWindowsObject AParent, int ResourceId,
	    PTModule AModule = NULL);
    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
	    {return "BORBTN";}
    TBButton(StreamableInit) : TButton(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TBButton"; }
};

inline Ripstream operator >> ( Ripstream is, RTBButton cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTBButton cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTBButton cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTBButton cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
