// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __BSTATBMP_H
#define __BSTATBMP_H

#ifndef __CONTROL_H
#include <control.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TBStaticBmp)

/* --------------------------------------------------------
  TBStaticBmp object
  -------------------------------------------------------- */
class _EXPORT TBStaticBmp : public TControl
{
public:
    TBStaticBmp(PTWindowsObject AParent, int AnId, LPSTR AText,
            int X, int Y, int W, int H, PTModule AModule = NULL);
    TBStaticBmp(PTWindowsObject AParent, int ResourceId,
		 PTModule AModule = NULL);

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
	    {return "BORBTN";}
    TBStaticBmp(StreamableInit) : TControl(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TBStaticBmp"; }
};

inline Ripstream operator >> ( Ripstream is, RTBStaticBmp cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTBStaticBmp cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTBStaticBmp cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTBStaticBmp cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
