// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __BRADIO_H
#define __BRADIO_H

#ifndef __RADIOBUT_H
#include <radiobut.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TBRadioButton)

/* --------------------------------------------------------
  TBRadioButton object
  -------------------------------------------------------- */

class _EXPORT TBRadioButton : public TRadioButton
{
public:
  TBRadioButton(PTWindowsObject AParent, int AnId, LPSTR ATitle,
                 int X, int Y, int W, int H, PTGroupBox AGroup,
		 PTModule AModule = NULL);
  TBRadioButton(PTWindowsObject AParent, int ResourceId, PTGroupBox AGroup,
		 PTModule AModule = NULL)
              : TRadioButton(AParent, ResourceId, AGroup, AModule){};
  static PTStreamable build();

protected:
  virtual LPSTR GetClassName()
	  {return "BORRADIO";}
  TBRadioButton(StreamableInit) : TRadioButton(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TBRadioButton"; }
};

inline Ripstream operator >> ( Ripstream is, RTBRadioButton cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTBRadioButton cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTBRadioButton cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTBRadioButton cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
