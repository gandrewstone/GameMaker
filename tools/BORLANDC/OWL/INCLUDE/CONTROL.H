// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __CONTROL_H
#define __CONTROL_H

#ifndef __APPLICAT_H
#include <applicat.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

  /* TControl */

_CLASSDEF(TControl)

class _EXPORT TControl : public TWindow {
public:
    TControl(PTWindowsObject AParent, int AnId, LPSTR ATitle, int X,
             int Y, int W, int H, PTModule AModule = NULL);
    TControl(PTWindowsObject AParent, int ResourceId,
	     PTModule AModule = NULL);
    virtual int GetId()
        { return Attr.Id; }

protected:
    TControl(StreamableInit) : TWindow(streamableInit) {};

    virtual void WMPaint(RTMessage Msg) = [WM_FIRST + WM_PAINT];
    virtual void WMDrawItem(RTMessage Msg) = [WM_FIRST + WM_DRAWITEM];
    virtual void ODADrawEntire(DRAWITEMSTRUCT far & DrawInfo);
    virtual void ODAFocus(DRAWITEMSTRUCT far & DrawInfo);
    virtual void ODASelect(DRAWITEMSTRUCT far & DrawInfo);
};

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
