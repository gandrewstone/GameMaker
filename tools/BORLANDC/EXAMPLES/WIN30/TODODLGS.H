#if !defined( __TODODLGS_H )
#define __TODODLGS_H

//---------------------------------------------------------------------
//
//  TODODLGS.H
//
//      Copyright (c) 1991, 1992 by Borland International
//      All Rights Reserved.
//
//  defines the following classes, which handle all the dialog boxes for
//  the Todo program.
//
//      AboutBox
//
//      FileBox     - provides a basic dialog for selecting a file.
//
//      EditBox     - provides a dialog box for editing an entry in the
//                    Todo list.
//
//---------------------------------------------------------------------

#if !defined( __WINDOWS_H )
#define STRICT
#include <Windows.h>
#endif  // __WINDOWS_H

#if !defined( __DIR_H )
#include <Dir.h>
#endif  // __DIR_H

#if !defined( __STRSTREAM_H )
#include <strstream.h>
#endif  // __STRSTREAM_H

#if !defined( __TODOWIN_H )
#include "TodoWin.h"
#endif  // __TODOWIN_H

#if !defined( __TODOLIST_H )
#include "TodoList.h"
#endif  // __TODOLIST_H

//---------------------------------------------------------------------
//
//  class AboutBox
//
//      draws and manages the About dialog.
//
//---------------------------------------------------------------------

class AboutBox : public ModalDialog
{

public:

    AboutBox( HWND );

private:

    virtual LPSTR getDialogName();

    virtual BOOL dispatch( HWND, UINT, WPARAM, LPARAM );

};

//---------------------------------------------------------------------
//
//  class FileBox
//
//      draws and manages a dialog box for selecting a file.
//
//---------------------------------------------------------------------

class FileBox : public ModalDialog
{

public:

    FileBox( HWND, const char *, const char *, const char *, BOOL = TRUE );

                                // constructor for the FileBox.
                                //
                                // Arguments:
                                //
                                // HWND owner - handle of the owner of
                                //              this dialog
                                // const char *c - caption for this dialog
                                // const char *p - initial path
                                // const char *s - file spec (wildcards
                                //                 accepted)
                                // BOOL me       - indicates whether the
                                //                 selected file must exist
                                //                 in order to be valid.
                                //                 For an input file, this
                                //                 should be TRUE.  For an
                                //                 output file it can
                                //                 be FALSE.


    const char *getPath();      // after executing run(), this function
                                // can be called to get the full path
                                // to the selected file.

    virtual WORD run();         // draws the dialog box and handles
                                // user input

protected:

    virtual BOOL dispatch( HWND, UINT, WPARAM, LPARAM );

private:

    virtual LPSTR getDialogName();

    const char *caption;        // store initial parameter
    const char *iPath;          // store initial parameter
    const char *iSpec;          // store initial parameter
    BOOL mustExist;             // store initial parameter

    char path[MAXPATH];         // used internally to keep track of
                                // current selection

    char res[MAXPATH];          // holds the selected path after run()
                                // has been executed.

    void resetDlg( HWND );      // used internally
    void initDlg( HWND );       // used internally
    BOOL flistCmd( HWND, LONG );  // used internally
    BOOL fnameCmd( HWND, LONG );  // used internally
    void okCmd( HWND );         // used internally
    void cancelCmd( HWND );     // used internally

};

class TodoEntry;                // forward reference

//---------------------------------------------------------------------
//
//  class EditBox
//
//      draws and manages a dialog box to edit an entry in the Todo list.
//
//---------------------------------------------------------------------

class EditBox : public ModalDialog
{

public:

    EditBox( HWND, TodoEntry& );
                                // constructor for the EditBox.
                                //
                                // Arguments:
                                //
                                // HWND owner - handle of the owner
                                //              of this dialog
                                // TodoEntry& td - the entry to be edited

private:

    virtual LPSTR getDialogName();

    virtual BOOL dispatch( HWND, UINT, WPARAM, LPARAM );

    TodoEntry& current;         // the entry to be edited

    int button;                 // current selection among the radio
                                // buttons that set the priority

    void initDlg( HWND );       // used internally
    void checkButton( HWND, WPARAM );  // used internally
    void okCmd( HWND );         // used internally
    void cancelCmd( HWND );     // used internally
};

//---------------------------------------------------------------------
//
//  inline functions
//
//---------------------------------------------------------------------

inline AboutBox::AboutBox( HWND hOwner ) : ModalDialog( hOwner )
{
}

inline const char *FileBox::getPath()
{
    return res;
}

inline EditBox::EditBox( HWND hOwner, TodoEntry& td ) :
    ModalDialog( hOwner ), current( td )
{
}

#endif  // __TODODLGS_H

