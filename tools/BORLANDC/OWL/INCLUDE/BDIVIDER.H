// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __BDIVIDER_H
#define __BDIVIDER_H

#ifndef __CONTROL_H
#include <control.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TBDivider)

/* --------------------------------------------------------
  TBDivider object
  -------------------------------------------------------- */
class _EXPORT TBDivider : public TControl
{
public:
	TBDivider(PTWindowsObject AParent, int AnId, LPSTR AText,
            int X, int Y, int W, int H, BOOL IsVertical,
	    BOOL IsBump, PTModule AModule = NULL);
	TBDivider(PTWindowsObject AParent, int ResourceId,
	    PTModule AModule = NULL);
    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
	    {return "BORSHADE";}
    TBDivider(StreamableInit) : TControl(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TBDivider"; }
};

inline Ripstream operator >> ( Ripstream is, RTBDivider cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTBDivider cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTBDivider cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTBDivider cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
