// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __CHECKBOX_H
#define __CHECKBOX_H

#ifndef __BUTTON_H
#include <button.h>
#endif

#ifndef __GROUPBOX_H
#include <groupbox.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

/* --------------------------------------------------------
  TCheckBox object
  -------------------------------------------------------- */

_CLASSDEF(TCheckBox)

class _EXPORT TCheckBox : public TButton
{
public:
    PTGroupBox Group;

    TCheckBox(PTWindowsObject AParent,int AnId, LPSTR ATitle, int X,
              int Y ,int W, int H, PTGroupBox AGroup,
	      PTModule AModule = NULL);
    TCheckBox(PTWindowsObject AParent, int ResourceId,
              PTGroupBox AGroup, PTModule AModule = NULL);
    void Check();
    void Uncheck();
    void Toggle();
    WORD GetCheck();
    void SetCheck(WORD CheckFlag);
    virtual WORD Transfer(Pvoid DataPtr, WORD TransferFlag);

    static PTStreamable build();

protected:
    virtual void BNClicked(RTMessage Msg) = [NF_FIRST + BN_CLICKED];

    /* Override TButton's processing so drawable check boxes and radio
       buttons work properly. */
    virtual void WMGetDlgCode(RTMessage Msg) =
                         [WM_FIRST + WM_GETDLGCODE]
        { DefWndProc(Msg); }

    TCheckBox(StreamableInit) : TButton(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TCheckBox"; }
};

inline Ripstream operator >> ( Ripstream is, RTCheckBox cl )
    { return is >> (RTStreamable )cl; }
inline Ripstream operator >> ( Ripstream is, RPTCheckBox cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTCheckBox cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTCheckBox cl )
    { return os << (PTStreamable )cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif


