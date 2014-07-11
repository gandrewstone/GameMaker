// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __BSTATIC_H
#define __BSTATIC_H

#ifndef __STATIC_H
#include <static.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TBStatic)

/* TBStatic */

class _EXPORT TBStatic : public TStatic
{
public:
    TBStatic(PTWindowsObject AParent, int AnId, LPSTR ATitle, int X,
            int Y, int W, int H, WORD ATextLen, PTModule AModule = NULL);
    TBStatic(PTWindowsObject AParent, int ResourceId, WORD ATextLen,
	      PTModule AModule = NULL);
    virtual Pchar nameOf() const
	    { return "TBStatic"; }

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
	    {return "BORSTATIC";}
    TBStatic(StreamableInit) : TStatic(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TBStatic"; }
};

inline Ripstream operator >> ( Ripstream is, RTBStatic cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTBStatic cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTBStatic cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTBStatic cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
