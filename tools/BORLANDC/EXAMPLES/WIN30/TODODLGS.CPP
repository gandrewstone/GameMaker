//---------------------------------------------------------------------
//
//  TODODLGS.CPP - part of TODO example program
//
//      Copyright (c) 1991, 1992 by Borland International
//      All Rights Reserved.
//
//---------------------------------------------------------------------

#define STRICT

#if !defined( __WINDOWS_H )
#include <Windows.h>
#endif  // __WINDOWS_H

#if !defined( __STRING_H )
#include <String.h>
#endif  // __STRING_H

#if !defined( __TODODLGS_H )
#include "TodoDlgs.h"
#endif  // __TODODLGS_H

#if !defined( __TODODEFS_H )
#include "TodoDefs.h"
#endif  // __TODODEFS_H

//---------------------------------------------------------------------
//
//  member functions for class AboutBox.
//
//  not much needed here...
//
//---------------------------------------------------------------------

LPSTR AboutBox::getDialogName()
{
    return "AboutBox";
}

BOOL AboutBox::dispatch( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
        {

        case WM_INITDIALOG:

            return TRUE;        // no initialization.

        case WM_COMMAND:

            if( wParam == IDOK || wParam == IDCANCEL )
                {
                EndDialog( hDlg, TRUE );  // selecting OK or Cancel
                return TRUE;              // terminates the dialog
                }

        default:                // if we don't handle it, pass it
                                // to our parent.

            return ModalDialog::dispatch( hDlg, msg, wParam, lParam );

        }
}

//---------------------------------------------------------------------
//
//  class SaveDir is only used locally, although it makes a handy
//  addition to a toolbox.  When created it saves the current drive
//  and directory, and when destroyed it restores the drive and path
//  to the saved drive and directory.
//
//---------------------------------------------------------------------

class SaveDir
{
public:
    SaveDir();
    ~SaveDir();
private:
    int drive;
    char path[MAXPATH];
};

SaveDir::SaveDir()
{
    drive = getdisk();
    path[0] = '\\';
    getcurdir( 0, path+1 );
}

SaveDir::~SaveDir()
{
    setdisk( drive );
    chdir( path );
}

//---------------------------------------------------------------------
//
//  member functions for class FileBox.
//
//---------------------------------------------------------------------

const fileAttr = 0x4010;

LPSTR FileBox::getDialogName()
{
    return "OpenFile";
}

//---------------------------------------------------------------------
//
//  FileBox::FileBox( HWND owner,
//                    const char *c,
//                    const char *p,
//                    const char *s,
//                    BOOL me
//                  );
//
//  constructor.  Massages initial data.
//
//---------------------------------------------------------------------

FileBox::FileBox( HWND owner,
                  const char *c,
                  const char *p,
                  const char *s,
                  BOOL me
                ) :
    ModalDialog( owner ),
    WinBase( ),
    caption( c ),
    iPath( p ),
    iSpec( s ),
    mustExist( me )
{
    strcpy( path, p );

    if( path[ strlen( path ) - 1 ] != '\\' )
        strcat( path, "\\" );

    strcat( path, s );
}

WORD FileBox::run()
{
    SaveDir sd;

    if( ModalDialog::run() != 0 )   // run() returns non-zero if
        return 1;                   // the user selected Cancel
    else
        {
        getcwd( res, sizeof res );  // otherwise, set up the selected
        int l = strlen( res );      // path and file name in res.
        if( res[ l-1 ] != '\\' )
            strcat( res, "\\" );
        strcat( res, path );
        return 0;
        }
}

//---------------------------------------------------------------------
//
//  handy function for internal use
//
//---------------------------------------------------------------------


void FileBox::resetDlg( HWND hDlg )
{
    DlgDirList( hDlg, path, IDD_FLIST, IDD_FPATH, fileAttr );
                                    // set the directory list to the
                                    // specified path.

    SetDlgItemText( hDlg, IDD_FNAME, path );
                                    // set the file name to the
                                    // specified name.
}

//---------------------------------------------------------------------
//
//  void FileBox::initDlg( HWND hDlg );
//
//  initializes the FileBox dialog.
//
//---------------------------------------------------------------------


void FileBox::initDlg( HWND hDlg )
{
    SendDlgItemMessage( hDlg, IDD_FNAME, EM_LIMITTEXT, MAXPATH, 0 );
                                    // no more than MAXPATH characters
                                    // in the file name.
    resetDlg( hDlg );
    SendMessage( hDlg, WM_SETTEXT, 0, (LONG)(LPSTR)caption );
                                    // display the caption as the
                                    // dialog title.
}

//---------------------------------------------------------------------
//
//  BOOL FileBox::flistCmd( HWND hDlg, LPARAM lParam )
//
//  called when the user does something to the directory list box.
//
//---------------------------------------------------------------------

BOOL FileBox::flistCmd( HWND hDlg, LPARAM lParam )
{

    switch( HIWORD( lParam ) )
        {
        case LBN_SELCHANGE:     // user changed the selection in the
                                // list box.

            if( DlgDirSelect( hDlg, path, IDD_FLIST ) != 0 )
                strcat( path, iSpec );
                                // current selection is a
                                // path, so append the file spec.

            SetDlgItemText( hDlg, IDD_FNAME, path );
                                // display the new path

            SendDlgItemMessage( hDlg,
                                IDD_FNAME,
                                EM_SETSEL,
                                0,
                                MAKELONG( 0, 0x7FFF )
                                );
                                // select all characters in the file
                                // name edit box.
            return TRUE;

        case LBN_DBLCLK:        // user has double-clicked a selection
                                // in the list box.

            if( DlgDirSelect( hDlg, path, IDD_FLIST ) != 0 )
                strcat( path, iSpec );
                                // current selection is a
                                // path, so append the file spec.

            resetDlg( hDlg );
            return TRUE;

        default:

            return FALSE;

        }
}

//---------------------------------------------------------------------
//
//  BOOL FileBox::fnameCmd( HWND hDlg, LPARAM lParam );
//
//  called when the user does something in the edit box containing the
//  file name.
//
//---------------------------------------------------------------------

BOOL FileBox::fnameCmd( HWND hDlg, LPARAM lParam )
{
    if( HIWORD( lParam ) == EN_CHANGE )
        {                       // enables or disables the OK button
                                // depending on whether there are any
                                // characters in the edit box.
        BOOL active =
	    (BOOL)SendMessage( (HWND)LOWORD( lParam ), WM_GETTEXTLENGTH, 0, 0 );
        EnableWindow( GetDlgItem( hDlg, IDOK ), active );
        return TRUE;
        }
    else
        return FALSE;
}

//---------------------------------------------------------------------
//
//  void FileBox::okCmd( HWND hDlg );
//
//  called when the user selects the OK button.  Validates the file
//  name.  Resets the dialog if the name is contains wildcards or is
//  invalid, and terminates the dialog if it's a valid file name.
//
//---------------------------------------------------------------------

void FileBox::okCmd( HWND hDlg )
{
    GetDlgItemText( hDlg, IDD_FNAME, path, 80 );
                                // get the specified path.

    int len = strlen( path );
    char lastChar = path[ len - 1 ];

    if( lastChar == '\\' || lastChar == ':' )
        strcat( path, iSpec );  // if the path ends in '\' or ':' it's not
                                // a full spec, so append the default spec.

    if( strpbrk( path, "*?" ) != 0 )
        {                       // if the path contains a wildcard,
        resetDlg( hDlg );       // reset the dialog and continue.
        return;
        }

    if( mustExist == TRUE )     // if we require the file to exist, check it.
        {
        ffblk fileInfo;
        if( findfirst( path, &fileInfo, 0 ) )
            {
            MessageBeep(0);     // file doesn't exist.
            return;
            }
        }

    EndDialog( hDlg, TRUE );    // we're done!
}

//---------------------------------------------------------------------
//
//  void FileBox::cancelCmd( HWND hDlg );
//
//  called when the user selects the Cancel button.  Terminates the
//  dialog.
//
//---------------------------------------------------------------------

void FileBox::cancelCmd( HWND hDlg )
{
    result = 1;
    EndDialog( hDlg, TRUE );
}

//---------------------------------------------------------------------
//
//  BOOL FileBox::dispatch( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
//
//  dispatches commands.
//
//---------------------------------------------------------------------

BOOL FileBox::dispatch( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
        {
        case WM_INITDIALOG:

            initDlg( hDlg );
            return TRUE;

        case WM_COMMAND:

            switch( wParam )
                {
                case IDD_FLIST:

                    if( flistCmd( hDlg, lParam ) == TRUE )
                        return TRUE;
                    break;

                case IDD_FNAME:

                    if( fnameCmd( hDlg, lParam ) == TRUE )
                        return TRUE;
                    break;

                case IDOK:

                    okCmd( hDlg );
                    return TRUE;

                case IDCANCEL:

                    cancelCmd( hDlg );
                    return TRUE;

                default:

                    break;
                }
        default:

            break;
        }

    return ModalDialog::dispatch( hDlg, msg, wParam, lParam );

}

//---------------------------------------------------------------------
//
//  member functions for class EditBox.
//
//---------------------------------------------------------------------

LPSTR EditBox::getDialogName()
{
    return "TodoEdit";
}

//---------------------------------------------------------------------
//
//  void EditBox::initDlg( HWND hDlg );
//
//  initializes the dialog box.
//
//---------------------------------------------------------------------

void EditBox::initDlg( HWND hDlg )
{
    char temp[100];

    // set up the current date edit field.
    ostrstream( temp, sizeof( temp ) ) << current.dateCreated << ends;
    SetDlgItemText( hDlg, IDE_DATEENT, temp );

    // set up the date due edit field.
    ostrstream( temp, sizeof( temp ) ) << current.dateDue << ends;
    SetDlgItemText( hDlg, IDE_DATEDUE, temp );

    // set up the text edit field
    SetDlgItemText( hDlg, IDE_TEXT, current.text );

    // set up the correct radio button
    button = IDE_HIGH + 1 - current.priority;
    CheckRadioButton( hDlg, IDE_LOW, IDE_HIGH, button );
}

//---------------------------------------------------------------------
//
//  void EditBox::checkButton( HWND hDlg, WPARAM wParam );
//
//  called when the user selects one of the radio buttons.
//
//---------------------------------------------------------------------

void EditBox::checkButton( HWND hDlg, WPARAM wParam )
{
    button = wParam;
    CheckRadioButton( hDlg, IDE_LOW, IDE_HIGH, button );
}

//---------------------------------------------------------------------
//
//  void EditBox::okCmd( HWND hDlg );
//
//  called when the user selects the OK button.  Copies data from the
//  dialog into the Todo entry and terminates the dialog.
//
//---------------------------------------------------------------------

void EditBox::okCmd( HWND hDlg )
{
    char temp[100 ];

    //  copy date created from dialog.
    GetDlgItemText( hDlg, IDE_DATEENT, temp, sizeof( temp ) );
    current.dateCreated = temp;

    //  copy date due from dialog.
    GetDlgItemText( hDlg, IDE_DATEDUE, temp, sizeof( temp ) );
    current.dateDue = temp;

    //  copy text from dialog
    int len = (int)SendDlgItemMessage(hDlg,IDE_TEXT,EM_LINELENGTH,0,0) + 1;
    delete current.text;
    current.text = new char[ len ];
    GetDlgItemText( hDlg, IDE_TEXT, current.text, len );

    //  copy priority from dialog.
    current.priority = IDE_HIGH + 1 - button;

    //  mark this entry as modified.
    current.dirty = TRUE;

    EndDialog( hDlg, TRUE );
}

//---------------------------------------------------------------------
//
//  void EditBox::cancelCmd( HWND hDlg );
//
//  called when the user selects the Cancel button.  Terminates the
//  dialog without changing any fields in the entry.
//
//---------------------------------------------------------------------

void EditBox::cancelCmd( HWND hDlg )
{
    result = 1;
    EndDialog( hDlg, TRUE );
}

//---------------------------------------------------------------------
//
//  BOOL EditBox::dispatch( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
//
//  dispatches commands.
//
//---------------------------------------------------------------------

BOOL EditBox::dispatch( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
        {
        case WM_INITDIALOG:

            initDlg( hDlg );
            return TRUE;

        case WM_COMMAND:

            switch( wParam )
                {
                case IDE_LOW:
                case IDE_MEDIUM:
                case IDE_HIGH:

                    checkButton( hDlg, wParam );
                    return TRUE;

                case IDOK:

                    okCmd( hDlg );
                    return TRUE;

                case IDCANCEL:

                    cancelCmd( hDlg );
                    return TRUE;
                }
        }

    return ModalDialog::dispatch( hDlg, msg, wParam, lParam );
}
