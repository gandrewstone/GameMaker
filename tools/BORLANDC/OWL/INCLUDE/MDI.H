// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __MDI_H
#define __MDI_H

#ifndef __APPLICAT_H
#include <applicat.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TMDIFrame)
_CLASSDEF(TMDIClient)

/* --------------------------------------------------------
  TMDIClient object
  -------------------------------------------------------- */

class _EXPORT TMDIClient : public TWindow
{
public:
    LPCLIENTCREATESTRUCT ClientAttr;

    TMDIClient(PTMDIFrame AParent, PTModule AModule = NULL);
    TMDIClient(PTMDIFrame AParent, HWND AnHWindow, PTModule AModule = NULL);
    virtual ~TMDIClient();

    /* Arranges iconized MDI child windows. */
    virtual void ArrangeIcons()
    { SendMessage(HWindow, WM_MDIICONARRANGE, 0, 0); }

    /* Cascades the MDI child windows. */
    virtual void CascadeChildren()
    { SendMessage(HWindow, WM_MDICASCADE, 0, 0); }

    /* Tiles the MDI child windows. */
    virtual void TileChildren()
    { SendMessage(HWindow, WM_MDITILE, 0, 0); }

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
        { return "MDICLIENT"; }

    /* Override TWindow::WMPaint and instead call DefWndProc */
    virtual void WMPaint(RTMessage Msg) = [WM_FIRST + WM_PAINT]
        { DefWndProc(Msg); }

    /* Override TWindow::WMMDIActivate, instead just call DefWndProc. */
    virtual void WMMDIActivate(RTMessage Msg) =
                 [WM_FIRST + WM_MDIACTIVATE]
        { DefWndProc(Msg); }

    TMDIClient(StreamableInit) : TWindow(streamableInit) {}
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TMDIClient"; }
};

inline Ripstream operator >> ( Ripstream is, RTMDIClient cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTMDIClient cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTMDIClient cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTMDIClient cl )
    { return os << (PTStreamable)cl; }

/* --------------------------------------------------------
  TMDIFrame object
  -------------------------------------------------------- */

class _EXPORT TMDIFrame : public TWindow
{
public:
    PTMDIClient ClientWnd;     // MDI client window
    int ChildMenuPos;          // menu position for child menu
    PTWindow ActiveChild;

    TMDIFrame(LPSTR ATitle, LPSTR MenuName, PTModule AModule = NULL);
    TMDIFrame(LPSTR ATitle, int MenuId, PTModule AModule = NULL);
    TMDIFrame(HWND AnHWindow, HWND ClientHWnd, PTModule AModule = NULL);
    virtual ~TMDIFrame();

    /* Constructs the TMDIFrame's MDI client window. */
    virtual void InitClientWindow()
        { ClientWnd = new TMDIClient(this); }

    /* Returns a pointer to the TMDIFrame's MDI client window. */
    virtual PTMDIClient GetClient()
        { return ClientWnd; }

    /* Constructs a new MDI child window object.  By default,
      constructs an instance of TWindow as an MDI child window object.
      Will almost always be redefined by descendants to construct an
      instance of a user-defined TWindow descendant as an MDI child
      window object. */
    virtual PTWindowsObject InitChild()
        { return new TWindow(this, ""); }

    virtual PTWindowsObject CreateChild();

    /* Tiles the MDI child windows by calling the TileChildren method
      of the MDI client window object. */
    virtual void TileChildren()
        { ClientWnd->TileChildren(); }

    /* Cascades the MDI child windows by calling the CascadeChildren
      method of the MDI client window object. */
    virtual void CascadeChildren()
        { ClientWnd->CascadeChildren(); }

    /* Arranges iconized MDI child windows by calling the
      ArrangeIcons method of the MDI client window object. */
    virtual void ArrangeIcons()
        { ClientWnd->ArrangeIcons(); }

    virtual BOOL CloseChildren();
    static PTStreamable build();

protected:
    virtual void GetWindowClass(WNDCLASS _FAR & AWndClass);
    virtual LPSTR GetClassName()
        { return "OWLMDIFrame31"; }
    virtual void SetupWindow();

    /* Since an MDI child doesn't get MDIACTIVATE messages when the
       frame gets (de)activated, call its ActivationResponse here. */
    virtual void WMActivate(RTMessage Msg)
        { TWindow::WMActivate(Msg);
          if ( ActiveChild )
            ActiveChild->ActivationResponse(
                   Msg.WParam, IsIconic(ActiveChild->HWindow)); }

    /* Responds to an incoming "CreateChild" command (with a
      CM_CREATECHILD command identifier) by calling CreateChild to
      construct and create a new MDI child. */
    virtual void CMCreateChild(RTMessage) =
                 [CM_FIRST + CM_CREATECHILD]
        { CreateChild(); }

    /* Responds to an incoming "Tile" command (with a CM_TILECHILDREN
      command identifier) by calling TileChildren to tile the MDI
      child windows. */
    virtual void CMTileChildren(RTMessage) =
                 [CM_FIRST + CM_TILECHILDREN]
        { TileChildren(); }

    /* Responds to an incoming "Cascade" command (with a
      CM_CASCADECHILDREN command identifier) by calling
      CascadeChildren to cascade the MDI  child windows. */
    virtual void CMCascadeChildren(RTMessage) =
                 [CM_FIRST + CM_CASCADECHILDREN]
        { CascadeChildren(); }

    /* Responds to an incoming "Arrange" command (with a
      CM_ARRANGEICONS command identifier) by calling ArrangeIcons
      to arrange the icons of the MDI child windows. */
    virtual void CMArrangeIcons(RTMessage) =
                 [CM_FIRST + CM_ARRANGEICONS]
        { ArrangeIcons(); }

    /* Responds to an incoming "CloseAll" command (with a
      CM_CLOSECHILDREN command identifier) by calling CloseChildren
      to close the MDI child windows. */
    virtual void CMCloseChildren(RTMessage) =
                 [CM_FIRST + CM_CLOSECHILDREN]
        { CloseChildren(); }

    TMDIFrame(StreamableInit) : TWindow(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TMDIFrame"; }
};

inline Ripstream operator >> ( Ripstream is, RTMDIFrame cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTMDIFrame cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTMDIFrame cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTMDIFrame cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
