// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __STATIC_H
#define __STATIC_H

#ifndef __CONTROL_H
#include <control.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

  /* TStatic */

_CLASSDEF(TStatic)

class _EXPORT TStatic : public TControl
{
public:
    WORD TextLen;

    TStatic(PTWindowsObject AParent, int AnId, LPSTR ATitle, int X,
            int Y, int W, int H, WORD ATextLen, PTModule AModule = NULL);
    TStatic(PTWindowsObject AParent, int ResourceId, WORD ATextLen,
            PTModule AModule = NULL);

    /* Returns the length of the control's text. */
    int GetTextLen()
        { return GetWindowTextLength(HWindow); }

    /* Fills the passed string with the text of the associated text
       control.  Returns the number of characters copied. */
    int GetText(LPSTR ATextString, int MaxChars)
        { return GetWindowText(HWindow, ATextString, MaxChars); }

    /* Sets the contents of the associated static text control to
       the passed string. */
    void SetText(LPSTR ATextString)
        { SetWindowText(HWindow, ATextString); }

    /* Clears the text of the associated static text control. */
    void Clear()
        { SetText(""); }

    virtual WORD Transfer(Pvoid DataPtr, WORD TransferFlag);

    virtual Pchar nameOf() const
	    { return "TStatic"; }

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
        { return "STATIC"; }

    TStatic(StreamableInit) : TControl(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);
private:
    virtual const Pchar streamableName() const
        { return "TStatic"; }
};

inline Ripstream operator >> ( Ripstream is, RTStatic cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTStatic cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTStatic cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTStatic cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
