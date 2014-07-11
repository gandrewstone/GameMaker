// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __BUTTON_H
#define __BUTTON_H

#ifndef __CONTROL_H
#include <control.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

/* --------------------------------------------------------
  TButton object
  -------------------------------------------------------- */

_CLASSDEF(TButton)

class _EXPORT TButton : public TControl
{
public:
    BOOL IsDefPB;

	TButton(PTWindowsObject AParent, int AnId, LPSTR AText,
            int X, int Y, int W, int H, BOOL IsDefault,
	    PTModule AModule = NULL);
	TButton(PTWindowsObject AParent, int ResourceId,
	    PTModule AModule = NULL);

    static PTStreamable build();
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

protected:
    BOOL IsCurrentDefPB;

    virtual LPSTR GetClassName()
        { return "BUTTON"; }
    virtual void SetupWindow();

    virtual void WMGetDlgCode(RTMessage Msg) =
                         [WM_FIRST + WM_GETDLGCODE];
    virtual void BMSetStyle(RTMessage Msg) = [WM_FIRST + BM_SETSTYLE];

    TButton(StreamableInit) : TControl(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TButton"; }
};

inline Ripstream operator >> ( Ripstream is, RTButton cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTButton cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTButton cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTButton cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
