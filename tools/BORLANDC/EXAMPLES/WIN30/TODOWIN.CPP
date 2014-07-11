//---------------------------------------------------------------------
//
//  TODOWIN.CPP - part of TODO example program
//
//      Copyright (c) 1991, 1992 by Borland International
//      All Rights Reserved.
//
//---------------------------------------------------------------------

#define STRICT

#if !defined( __WINDOWS_H )
#include <Windows.h>
#endif  // __WINDOWS_H

#if !defined( __ASSERT_H )
#include <assert.h>
#endif  // __ASSERT_H

#if !defined( __TODOWIN_H )
#include "TodoWin.h"
#endif  // __TODOWIN_H

//---------------------------------------------------------------------
//
//  initialize static data members of class WinBase.
//  NOTE: these are compiler dependent.  If you are not using
//  Borland C++ you will have to replace this code with something
//  that works with your compiler.
//
//---------------------------------------------------------------------

extern HINSTANCE _hInstance, _hPrev;
extern LPSTR _pszCmdline;
extern int _cmdShow;

HINSTANCE WinBase::hInst = _hInstance;

HINSTANCE WinBase::hPrevInst = _hPrev;

LPSTR WinBase::cmd = _pszCmdline;

int WinBase::show = _cmdShow;

//---------------------------------------------------------------------
//
//  members and data for class ModalDialog
//
//---------------------------------------------------------------------

ModalDialog *ModalDialog::curDlg = 0;

WORD ModalDialog::run()
{
    FARPROC dlgProc =
	MakeProcInstance( (FARPROC)ModalDialog::dlgProc, (HINSTANCE)hInst );
    DialogBox( (HINSTANCE)hInst, (LPCSTR)getDialogName(), (HWND)hWnd(), (DLGPROC)dlgProc );
    FreeProcInstance( dlgProc );
    return result;
}

BOOL CALLBACK _export ModalDialog::dlgProc( HWND hDlg,
					      UINT msg,
					      WPARAM wParam,
					      LPARAM lParam
                                            )
{
    return curDlg->dispatch( hDlg, msg, wParam, lParam );
}

BOOL ModalDialog::dispatch( HWND, UINT, WPARAM, LPARAM )
{
    return FALSE;
}

//---------------------------------------------------------------------
//
//  members and data for class Window
//
//---------------------------------------------------------------------

Window *Window::inCreate = 0;
Window *Window::winList = 0;

BOOL Window::create()
{
    if( hPrevInst == 0 && registerClass() == FALSE )
        {
        return FALSE;
        }

    inCreate = this;            // flag that we're inside CreateWindow()

    createWindow();

    nextWin = winList;          // insert this object into the Window list
    winList = this;

    inCreate = 0;               // now it's OK to use normal dispatching

    return TRUE;
}

WORD Window::run()
{
    assert( hWnd() != 0 );      // check that we really exist

    MSG msg;
    while( GetMessage( &msg, NULL, NULL, NULL ) != 0 )
        {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
        }
    return msg.wParam;
}

LONG Window::dispatch( UINT msg, WPARAM wParam, LPARAM lParam )
{
    return DefWindowProc( (HWND)hWnd(), msg, wParam, lParam );
}

LRESULT CALLBACK Window::wndProc( HWND hWnd,
                                 UINT msg,
                                 WPARAM wParam,
                                 LPARAM lParam
                               )
{

    Window *cur = Window::winList;

    //  look up the handle in our Window list
    while( cur != 0 && cur->hWnd() != hWnd )
        cur = cur->nextWin;

    //  normal dispatching
    if( cur != 0 )
        return cur->dispatch( msg, wParam, lParam );

    //  if we're inside CreateWindow(), assume that the message is for us
    if( inCreate != 0 )
        {
        inCreate->hWindow = hWnd;
        return inCreate->dispatch( msg, wParam, lParam );
        }

    //  otherwise, pass it on to windows
    return DefWindowProc( hWnd, msg, wParam, lParam );
}



