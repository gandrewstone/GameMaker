// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __SCROLLBAR_H
#define __SCROLLBAR_H

#ifndef __CONTROL_H
#include <control.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

// TScrollBar transfer structure
struct TScrollBarData {
  int LowValue;
  int HighValue;
  int Position;
};

_CLASSDEF(TScrollBar)

/* --------------------------------------------------------
  TScrollBar object
  -------------------------------------------------------- */

class _EXPORT TScrollBar : public TControl
{
public:
    int LineMagnitude, PageMagnitude;

    TScrollBar(PTWindowsObject AParent, int AnId, int X, int Y, int W,
               int H, BOOL IsHScrollBar, PTModule AModule = NULL);
    TScrollBar(PTWindowsObject AParent, int ResourceId,
               PTModule AModule = NULL);

    void GetRange(Rint LoVal, Rint HiVal);
    int GetPosition();
    void SetRange(int LoVal, int HiVal);
    void SetPosition(int ThumbPos);
    int DeltaPos(int Delta);
    virtual WORD Transfer(Pvoid DataPtr, WORD TransferFlag);

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
        { return "SCROLLBAR"; }
    virtual void SetupWindow();

    virtual void SBLineUp(RTMessage Msg) = [NF_FIRST + SB_LINEUP];
    virtual void SBLineDown(RTMessage Msg) = [NF_FIRST + SB_LINEDOWN];
    virtual void SBPageUp(RTMessage Msg) = [NF_FIRST + SB_PAGEUP];
    virtual void SBPageDown(RTMessage Msg) = [NF_FIRST + SB_PAGEDOWN];
    virtual void SBThumbPosition(RTMessage Msg) =
                                      [NF_FIRST + SB_THUMBPOSITION];
    virtual void SBThumbTrack(RTMessage Msg) = [NF_FIRST + SB_THUMBTRACK];
    virtual void SBTop(RTMessage Msg) = [NF_FIRST + SB_TOP];
    virtual void SBBottom(RTMessage Msg) = [NF_FIRST + SB_BOTTOM];

    TScrollBar(StreamableInit) : TControl(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TScrollBar"; }
};

inline Ripstream operator >> ( Ripstream is, RTScrollBar cl )
    { return is >> (RTStreamable )cl; }
inline Ripstream operator >> ( Ripstream is, RPTScrollBar cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTScrollBar cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTScrollBar cl )
    { return os << (PTStreamable )cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
