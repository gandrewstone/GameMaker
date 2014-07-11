// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __LISTBOX_H
#define __LISTBOX_H

#ifndef __CONTROL_H
#include <control.h>
#endif

#ifndef __ARRAY_H
#include <array.h>
#endif

#ifndef __STRNG_H
#include <strng.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TListBox)
_CLASSDEF(TListBoxData)

/* --------------------------------------------------------
  TListBox object
  -------------------------------------------------------- */

class _EXPORT TListBox : public TControl
{
public:
    TListBox(PTWindowsObject AParent, int AnId, int X, int Y,
             int W, int H, PTModule AModule = NULL);
    TListBox(PTWindowsObject AParent, int ResourceId, PTModule AModule = NULL)
             : TControl(AParent, ResourceId, AModule){};

    int AddString(LPSTR AString);
    int InsertString(LPSTR AString, int Index);
    int DeleteString(int Index);
    void ClearList();
    virtual WORD Transfer(Pvoid DataPtr, WORD TransferFlag);
    int GetCount();
    int FindString(LPSTR AString, int SearchIndex);
    int FindExactString(LPSTR AString, int SearchIndex);
    int GetString(LPSTR AString, int Index);
    int GetStringLen(int Index);

    // next four functions only for single-selection
    // list boxes (and combo boxes).
    int GetSelString(LPSTR AString, int MaxChars);
    int SetSelString(LPSTR AString, int SearchIndex);
    int GetSelIndex();
    int SetSelIndex(int Index);

    int GetSelCount();

    // next four functions only for multiple-selection list boxes.
    int GetSelStrings(LPSTR *Strings, int MaxCount, int MaxChars);
    int SetSelStrings(LPSTR *Prefixes, int NumSelections,
                      BOOL ShouldSet);
    int GetSelIndexes(Pint Indexes, int MaxCount);
    int SetSelIndexes(Pint Indexes, int NumSelections,
                      BOOL ShouldSet);

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
        { return "LISTBOX"; }
    virtual WORD GetMsgID(WORD AMsg);

    TListBox(StreamableInit) : TControl(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TListBox"; }
};

inline Ripstream operator >> ( Ripstream is, RTListBox cl )
    { return is >> (RTStreamable )cl; }
inline Ripstream operator >> ( Ripstream is, RPTListBox cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTListBox cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTListBox cl )
    { return os << (PTStreamable )cl; }

enum msgname {MN_ADDSTRING,    MN_INSERTSTRING, MN_DELETESTRING,
              MN_RESETCONTENT, MN_GETCOUNT,     MN_GETTEXT,
              MN_GETTEXTLEN,   MN_SELECTSTRING, MN_SETCURSEL,
              MN_GETCURSEL,    MN_FINDSTRING };

class _EXPORT TListBoxData
{
public:
    PArray Strings;
    PArray SelStrings;
    int SelCount;

    TListBoxData();
    ~TListBoxData();
    void AddString(Pchar AString, BOOL IsSelected = FALSE);
    void SelectString(LPSTR AString);
    void ResetSelections();
    int GetSelStringLength(int Index = 0);
    void GetSelString(LPSTR Buffer, int BufferSize, int Index = 0);
};

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
