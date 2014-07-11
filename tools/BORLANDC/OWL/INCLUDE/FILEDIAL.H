// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __FILEDIAL_H
#define __FILEDIAL_H

#ifndef __DIALOG_H
#include <dialog.h>
#endif

#ifndef __DIR_H
#include <dir.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

#define FILESPEC	MAXPATH + MAXEXT

_CLASSDEF(TFileDialog)

  /* TFileDialog */
class _EXPORT TFileDialog : public TDialog
{
public:
    LPSTR FilePath;
    char PathName[MAXPATH];
    char Extension[MAXEXT];
    char FileSpec[FILESPEC];

    TFileDialog(PTWindowsObject AParent, int ResourceId, LPSTR AFilePath,
                PTModule AModule = NULL);
    virtual BOOL CanClose();
    void SelectFileName();
    void UpdateFileName();
    BOOL UpdateListBoxes();

    static PTStreamable build();

protected:
    virtual void SetupWindow();
    virtual void HandleFName(RTMessage Msg)
          = [ID_FIRST + ID_FNAME];
    virtual void HandleFList(RTMessage Msg)
          = [ID_FIRST + ID_FLIST];
    virtual void HandleDList(RTMessage Msg)
          = [ID_FIRST + ID_DLIST];

    TFileDialog(StreamableInit) : TDialog(streamableInit) {};

private:
    virtual const Pchar streamableName() const
        { return "TFileDialog"; }
};

inline Ripstream operator >> ( Ripstream is, RTFileDialog cl )
    { return is >> (RTStreamable )cl; }
inline Ripstream operator >> ( Ripstream is, RPTFileDialog cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTFileDialog cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTFileDialog cl )
    { return os << (PTStreamable )cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif
