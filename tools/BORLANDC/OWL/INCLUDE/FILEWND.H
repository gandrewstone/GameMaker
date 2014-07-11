// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __FILEWND_H
#define __FILEWND_H

#ifndef __EDITWND_H
#include <editwnd.h>
#endif

#ifndef __FILEDIAL_H
#include <filedial.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

 /* TFileWindow */

_CLASSDEF(TFileWindow)

class _EXPORT TFileWindow : public TEditWindow
{
public:
    LPSTR FileName;
    BOOL IsNewFile;

    TFileWindow(PTWindowsObject AParent, LPSTR ATitle, LPSTR AFileName,
                PTModule AModule = NULL);
    virtual ~TFileWindow();

    virtual BOOL CanClear();
    virtual BOOL CanClose();
    void NewFile();
    void Open();
    BOOL Read();
    void ReplaceWith(LPSTR AFileName);
    BOOL Save();
    BOOL SaveAs();
    void SetFileName(LPSTR AFileName);
    BOOL Write();

    static PTStreamable build();

protected:
    virtual void SetupWindow();
    virtual void CMFileNew(RTMessage Msg) = [CM_FIRST + CM_FILENEW];
    virtual void CMFileOpen(RTMessage Msg) = [CM_FIRST + CM_FILEOPEN];
    virtual void CMFileSave(RTMessage Msg) = [CM_FIRST + CM_FILESAVE];
    virtual void CMFileSaveAs(RTMessage Msg) =
                                         [CM_FIRST + CM_FILESAVEAS];

    TFileWindow(StreamableInit) : TEditWindow(streamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:
    virtual const Pchar streamableName() const
        { return "TFileWindow"; }
};

inline Ripstream operator >> ( Ripstream is, RTFileWindow cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTFileWindow cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTFileWindow cl )
    { return os << (RTStreamable)cl; }
inline Ropstream operator << ( Ropstream os, PTFileWindow cl )
    { return os << (PTStreamable)cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
