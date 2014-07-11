// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __EDIT_H
#define __EDIT_H


#ifndef __STATIC_H
#include <static.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TEdit)

  /* TEdit */
class _EXPORT TEdit : public TStatic
{
public:
    TEdit(PTWindowsObject AParent, int AnId, LPSTR AText, int X,
          int Y, int W, int H, WORD ATextLen, BOOL Multiline,
          PTModule AModule = NULL);
    TEdit(PTWindowsObject AParent, int ResourceId, WORD ATextLen,
          PTModule AModule = NULL);

    void Undo();
    BOOL CanUndo();
    void Paste();
    void Copy();
    void Cut();
    int GetNumLines();
    int GetLineLength(int LineNumber);
    BOOL GetLine(LPSTR ATextString, int StrSize, int LineNumber);
    void GetSubText(LPSTR ATextString, int StartPos, int EndPos);
    BOOL DeleteSubText(int StartPos, int EndPos);
    BOOL DeleteLine(int LineNumber);
    void GetSelection(Rint StartPos, Rint EndPos);
    BOOL DeleteSelection();
    BOOL IsModified();
    void ClearModify();
    int GetLineFromPos(int CharPos);
    int GetLineIndex(int LineNumber);
    void Scroll(int HorizontalUnit, int VerticalUnit);
    BOOL SetSelection(int StartPos, int EndPos);
    void Insert(LPSTR ATextString);
    int Search(int StartPos, LPSTR AText, BOOL CaseSensitive);

    static PTStreamable build();

protected:
    virtual LPSTR GetClassName()
	{ return "EDIT"; }
    virtual void SetupWindow();
    virtual void CMEditCut(RTMessage Msg) = [CM_FIRST + CM_EDITCUT];
    virtual void CMEditCopy(RTMessage Msg) = [CM_FIRST + CM_EDITCOPY];
    virtual void CMEditPaste(RTMessage Msg) = [CM_FIRST + CM_EDITPASTE];
    virtual void CMEditDelete(RTMessage Msg) =
                                        [CM_FIRST + CM_EDITDELETE];
    virtual void CMEditClear(RTMessage Msg) = [CM_FIRST + CM_EDITCLEAR];
    virtual void CMEditUndo(RTMessage Msg) = [CM_FIRST + CM_EDITUNDO];
    virtual void ENErrSpace(RTMessage Msg) = [NF_FIRST + EN_ERRSPACE];

    TEdit(StreamableInit) : TStatic(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TEdit"; }
};

inline Ripstream operator >> ( Ripstream is, RTEdit cl )
    { return is >> (RTStreamable )cl; }
inline Ripstream operator >> ( Ripstream is, RPTEdit cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTEdit cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTEdit cl )
    { return os << (PTStreamable )cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
