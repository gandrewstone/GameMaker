// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __BCHKBOX_H
#define __BCHKBOX_H

#ifndef __CHECKBOX_H
#include <checkbox.h>
#endif

#ifndef __GROUPBOX_H
#include <groupbox.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TBCheckBox)

/* --------------------------------------------------------
  TBCheckBox object
  -------------------------------------------------------- */

class _EXPORT TBCheckBox : public TCheckBox
{
public:
    TBCheckBox(PTWindowsObject AParent,int AnId, LPSTR ATitle, int X,
	    int Y ,int W, int H, PTGroupBox AGroup,
	    PTModule AModule = NULL);
    TBCheckBox(PTWindowsObject AParent, int ResourceId,
	        PTGroupBox AGroup, PTModule AModule = NULL);
    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
	    {return "BORCHECK";}
    TBCheckBox(StreamableInit) : TCheckBox(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TBCheckBox"; }
};

inline Ripstream operator >> ( Ripstream is, RTBCheckBox cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTBCheckBox cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTBCheckBox cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTBCheckBox cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
