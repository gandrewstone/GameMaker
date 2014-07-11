// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __WINDOW_H
#define __WINDOW_H

// WINDOW.H
// Defines type TWindow. This defines the basic behavior of all windows
//

#ifndef __WINDOBJ_H
#include <windobj.h>
#endif

#ifndef __SCROLLER_H
#include <scroller.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

  /* TWindow creation attributes */
struct _CLASSTYPE TWindowAttr { // _CLASSTYPE needed by BC++ 2.0
    DWORD Style;
    DWORD ExStyle;
    int X, Y, W, H;
    LPSTR Menu;  // Menu name
    int Id ;     // Child identifier
    LPSTR Param;
 };


/* Window Class */

_CLASSDEF(TWindow)

class _EXPORT TWindow : public TWindowsObject
{
public:
    TWindowAttr  Attr;
    PTScroller Scroller;
    HWND FocusChildHandle;

    TWindow(PTWindowsObject AParent, LPSTR ATitle, PTModule AModule = NULL);
    TWindow(HWND AnHWindow, PTModule AModule = NULL);
    virtual ~TWindow();

    virtual BOOL AssignMenu(LPSTR MenuName);
    virtual BOOL AssignMenu(int MenuId);
    virtual BOOL Create();
    virtual void ActivationResponse(WORD Activated, BOOL IsIconified);

    virtual classType  	  isA() const
	{ return windowClass; }
    virtual Pchar nameOf() const
	{ return "TWindow"; }

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
        { return "OWLWindow31"; }
    virtual void GetWindowClass(WNDCLASS _FAR & AWndClass);
    virtual void SetupWindow();

    virtual void WMCreate(RTMessage Msg) = [WM_FIRST + WM_CREATE];
    virtual void WMMDIActivate(RTMessage Msg) =
                 [WM_FIRST + WM_MDIACTIVATE];
    virtual void WMHScroll(RTMessage Msg) = [WM_FIRST + WM_HSCROLL];
    virtual void WMVScroll(RTMessage Msg) = [WM_FIRST + WM_VSCROLL];
    virtual void WMPaint(RTMessage Msg) = [WM_FIRST + WM_PAINT];
#if defined(WIN31)
    // windows 3.1 interface
    virtual void Paint(HDC PaintDC, PAINTSTRUCT _FAR & PaintInfo);
#endif
#if defined(WIN30)
    // windows 3.0 interface
    virtual void Paint(HDC_30 PaintDC, PAINTSTRUCT _FAR & PaintInfo);
#endif
#if (defined(WIN30) || defined(WIN31)) && !(defined(WIN30) && defined(WIN31))
    // this function is never called. it is used to pad the vtable so that
    // exactly two Paint(...) definitions are always present.
    virtual void Paint(void *, void *) { };
#endif
    virtual void WMSize(RTMessage Msg) = [WM_FIRST + WM_SIZE];
    virtual void WMMove(RTMessage Msg) = [WM_FIRST + WM_MOVE];
    virtual void WMLButtonDown(RTMessage Msg) = [WM_FIRST + WM_LBUTTONDOWN];

    TWindow(StreamableInit) : TWindowsObject(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TWindow"; }

}; 	// end of Window class

inline Ripstream operator >> ( Ripstream is, RTWindow cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTWindow cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTWindow cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTWindow cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif // ifdef _WINDOW_H
