// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __DIALOG_H
#define __DIALOG_H

#ifndef __APPLICAT_H
#include <applicat.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

const int BAD_DIALOG_STATUS = -2;

/* TDialog creation atributes */
struct _CLASSTYPE TDialogAttr { // _CLASSTYPE needed by BC++ 2.0
    LPSTR Name;
    DWORD Param;
};

  /* TDialog */

_CLASSDEF(TDialog)

class _EXPORT TDialog : public TWindowsObject
{
public:
    TDialogAttr Attr;
    BOOL IsModal;

    TDialog(PTWindowsObject AParent, LPSTR AName, PTModule AModule = NULL);
    TDialog(PTWindowsObject AParent, int ResourceId, PTModule AModule = NULL);
    virtual ~TDialog();

    virtual BOOL Create();
    virtual int Execute();

    virtual void CloseWindow(int ARetValue);
    virtual void CloseWindow();
    virtual void ShutDownWindow(int ARetValue);
    virtual void ShutDownWindow();
    virtual void Destroy(int ARetValue);
    virtual void Destroy();

    void SetCaption(LPSTR ATitle);

/* Returns the handle of the dialog's control with the passed Id.*/
    HWND GetItemHandle(int DlgItemID)
      { return GetDlgItem(HWindow, DlgItemID); }

/* Sends the passed message to the dialog's control which has
   the Id DlgItemID. */
    DWORD SendDlgItemMsg(int DlgItemID, WORD AMsg, WORD WParam, DWORD LParam)
      { return SendDlgItemMessage(HWindow, DlgItemID, AMsg, WParam, LParam); }


    virtual classType  	  isA() const
	{ return dialogClass; }
    virtual Pchar nameOf() const
        { return "TDialog"; }

    static PTStreamable build();

protected:
    virtual void GetWindowClass(WNDCLASS _FAR & AWndClass);
    virtual LPSTR GetClassName();
    virtual void SetupWindow();
    virtual void Ok(RTMessage Msg) = [ID_FIRST + IDOK];
    virtual void Cancel(RTMessage Msg) = [ID_FIRST + IDCANCEL];
    virtual void WMInitDialog(RTMessage Msg) = [WM_FIRST + WM_INITDIALOG];
    virtual void WMQueryEndSession(RTMessage Msg) =
                 [WM_FIRST + WM_QUERYENDSESSION];
    virtual void WMClose(RTMessage Msg) = [WM_FIRST + WM_CLOSE];

    TDialog(StreamableInit) : TWindowsObject(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TDialog"; }
};

inline Ripstream operator >> ( Ripstream is, RTDialog cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTDialog cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTDialog cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTDialog cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
