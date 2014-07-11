// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __GROUPBOX_H
#define __GROUPBOX_H

#ifndef __CONTROL_H
#include <control.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TGroupBox)

/* --------------------------------------------------------
  TGroupBox object
  -------------------------------------------------------- */
class _EXPORT TGroupBox : public TControl
{
public:
    BOOL NotifyParent;

    TGroupBox(PTWindowsObject AParent, int AnId, LPSTR AText, int X,
              int Y, int W, int H, PTModule AModule = NULL);
    TGroupBox(PTWindowsObject AParent, int ResourceId,
              PTModule AModule = NULL);
    virtual void SelectionChanged(int ControlId);

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
        { return "BUTTON"; }

    TGroupBox(StreamableInit) : TControl(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TGroupBox"; }
};

inline Ripstream operator >> ( Ripstream is, RTGroupBox cl )
    { return is >> (RTStreamable )cl; }
inline Ripstream operator >> ( Ripstream is, RPTGroupBox cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTGroupBox cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTGroupBox cl )
    { return os << (PTStreamable )cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
