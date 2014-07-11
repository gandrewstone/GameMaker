// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __COMBOBOX_H
#define __COMBOBOX_H

#ifndef __LISTBOX_H
#include <listbox.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TComboBox)
_CLASSDEF(TComboBoxData)

/* --------------------------------------------------------
  TComboBox object
  -------------------------------------------------------- */

class _EXPORT TComboBox : public TListBox
{
public:
    WORD TextLen;

    TComboBox(PTWindowsObject AParent, int AnId, int X, int Y, int W,
              int H, DWORD AStyle, WORD ATextLen, PTModule AModule = NULL);
    TComboBox(PTWindowsObject AParent, int ResourceId, WORD ATextLen,
              PTModule AModule = NULL);

    /* Returns the length of the associated edit control's text. */
    int GetTextLen()
        { return GetWindowTextLength(HWindow); }

    /* Fills the supplied string with the text of the associated
       edit control. Returns the number of characters copied. */
    int GetText(LPSTR AString, int MaxChars)
        { return GetWindowText(HWindow, AString, MaxChars); }

    void SetText(LPSTR AString);

    /* Selects characters in the edit control of the combo box
       which are between StartPos and EndPos. Returns CB_ERR if
       the combo box does not have an edit control. */
    int SetEditSel(int StartPos, int EndPos)
        { return (int)SendMessage(HWindow, CB_SETEDITSEL,
                      0, MAKELONG(StartPos, EndPos)); }

    int GetEditSel(Rint StartPos, Rint EndPos);

    /* Clears the text of the associated edit control. */
    void Clear()
        { SetText(""); }

    void ShowList();
    void HideList();
    virtual WORD Transfer(Pvoid DataPtr, WORD TransferFlag);
    virtual Pchar nameOf() const
	    { return "TComboBox"; }

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
	    {return "COMBOBOX";}
    virtual WORD GetMsgID(WORD AnId);
    virtual void SetupWindow();

    TComboBox(StreamableInit) : TListBox(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TComboBox"; }
};

inline Ripstream operator >> ( Ripstream is, RTComboBox cl )
    { return is >> (RTStreamable )cl; }
inline Ripstream operator >> ( Ripstream is, RPTComboBox cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTComboBox cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTComboBox cl )
    { return os << (PTStreamable )cl; }

class _EXPORT TComboBoxData
{
public:
    PArray Strings;
    Pchar Selection;

    TComboBoxData();
    ~TComboBoxData();
    void AddString(Pchar AString, BOOL IsSelected = FALSE);
};

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
