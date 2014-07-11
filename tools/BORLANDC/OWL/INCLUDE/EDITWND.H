// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __EDITWND_H
#define __EDITWND_H

#ifndef __EDIT_H
#include <edit.h>
#endif

#ifndef __CHECKBOX_H
#include <checkbox.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

struct _CLASSTYPE TSearchStruct { // _CLASSTYPE needed for BC++ 2.0
  char SearchText[81];
  BOOL CaseSensitive;
  char ReplaceText[81];
  BOOL ReplaceAll;
  BOOL PromptOnReplace;
};

_CLASSDEF(TEditWindow)
_CLASSDEF(TSearchDialog)

/* TEditWindow declaration */

class _EXPORT TEditWindow : public TWindow
{
public:
    PTEdit Editor;
    TSearchStruct SearchStruct;
    BOOL IsReplaceOp; // True if the search is a search and replace.

    TEditWindow(PTWindowsObject AParent, LPSTR ATitle,
	    PTModule AModule = NULL);
    void DoSearch();

    static PTStreamable build();

protected:
    virtual void WMSize(RTMessage Msg) = [WM_FIRST + WM_SIZE];
    virtual void WMSetFocus(RTMessage Msg) = [WM_FIRST + WM_SETFOCUS];
    virtual void CMEditFind(RTMessage Msg) = [CM_FIRST + CM_EDITFIND];
    virtual void CMEditFindNext(RTMessage Msg) =
                               [CM_FIRST + CM_EDITFINDNEXT];
    virtual void CMEditReplace(RTMessage Msg) =
                               [CM_FIRST + CM_EDITREPLACE];

    TEditWindow(StreamableInit) : TWindow(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TEditWindow"; }
};

inline Ripstream operator >> ( Ripstream is, RTEditWindow cl )
    { return is >> (RTStreamable )cl; }
inline Ripstream operator >> ( Ripstream is, RPTEditWindow cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTEditWindow cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTEditWindow cl )
    { return os << (PTStreamable )cl; }

class _EXPORT TSearchDialog : public TDialog
{
public:
  TSearchDialog(PTWindowsObject AParent, int ResourceId,
                TSearchStruct _FAR &SearchStruct,
                PTModule AModule = NULL);
};

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
