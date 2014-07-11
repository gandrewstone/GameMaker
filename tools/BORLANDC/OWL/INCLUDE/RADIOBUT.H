// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __RADIOBUT_H
#define __RADIOBUT_H

#ifndef __CHECKBOX_H
#include <checkbox.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TRadioButton)

/* --------------------------------------------------------
  TRadioButton object
  -------------------------------------------------------- */
class _EXPORT TRadioButton : public TCheckBox
{
public:
    TRadioButton(PTWindowsObject AParent, int AnId, LPSTR ATitle,
               int X, int Y, int W, int H, PTGroupBox AGroup,
               PTModule AModule = NULL);

    TRadioButton(PTWindowsObject AParent, int ResourceId, PTGroupBox AGroup,
               PTModule AModule = NULL)
              : TCheckBox(AParent, ResourceId, AGroup, AModule){};

    static PTStreamable build();

protected:
    virtual void BNClicked(RTMessage Msg) = [NF_FIRST + BN_CLICKED];

    TRadioButton(StreamableInit) : TCheckBox(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TRadioButton"; }
};

inline Ripstream operator >> ( Ripstream is, RTRadioButton cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTRadioButton cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTRadioButton cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTRadioButton cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
