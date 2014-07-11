#if !defined( __TODOWIN_H )
#define __TODOWIN_H

//---------------------------------------------------------------------
//
//  TODOWIN.H
//
//      Copyright (c) 1991, 1992 by Borland International
//      All Rights Reserved.
//
//  Defines the following classes, which are useful for building
//  general purpose windows applications:
//
//  WinBase - provides basic data and functionality for
//            windows programming.
//
//  ModalDialog - provides for modal dialog boxes.  Inherits from
//            WinBase, which is a virtual base.
//
//  Window  - provides core functionality for a window under Windows.
//            Inherits from WinBase, which is a virtual base.
//
//---------------------------------------------------------------------

#if !defined( __WINDOWS_H )
#define STRICT
#include <Windows.h>
#endif  // __WINDOWS_H

#if !defined( __ASSERT_H )
#include <assert.h>
#endif  // __ASSERT_H

//---------------------------------------------------------------------
//
//  class WinBase
//
//      provides the basic data and functionality for all classes
//      used in the windows application.  It is an abstract base class.
//
//---------------------------------------------------------------------

class WinBase
{

public:

    virtual WORD run() = 0;     // the core function of all windows!  For
                                // the main application window, this
                                // provides the message loop.  In modal
                                // dialogs, it sets up the dialog box,
                                // calls the dialog proc, and closes down
                                // the dialog.



    static  HINSTANCE hInst;    // the handle of the current instance

    static  HINSTANCE hPrevInst;// the handle of the previous instance

    static  LPSTR cmd;          // pointer to the command line

    static  int show;           // the nCmdShow parameter of the current
                                // instance

    HWND    hWnd();             // access function

protected:

    HWND hWindow;               // the window handle of the class.  This is
                                // accessed through hWnd(), and it will
                                // provide the correct handle for any
                                // derived class.

                                // NOTE: this field is not initialized by
                                // the constructor of this class.  It must
                                // be initialized by the constructor of a
                                // class derived from this class.

};

//---------------------------------------------------------------------
//
//  class ModalDialog
//
//      provides basic functionality for modal dialog boxes.  It is an
//      abstract base class.
//
//---------------------------------------------------------------------

class ModalDialog : public virtual WinBase
{

public:

    ModalDialog( HWND );        // constructor.  Since a modal dialog
                                // needs to know its owner, the handle
                                // of the owner is passed as a parameter
                                // to the constructor.

    ~ModalDialog();

    virtual WORD run();         // the core of a modal dialog!  It sets
                                // up the dialog, executes it, and closes
                                // it down.

protected:

    virtual BOOL dispatch( HWND, UINT, WPARAM, LPARAM );
                                // the core of any window.  Whenever
                                // dlgProc receives a message it passes
                                // the message on to dispatch().  If
                                // dispatch() doesn't handle the message
                                // itself, it should call the dispatch()
                                // function in its base class.  Ultimately,
                                // messages not handled higher up are
                                // handled by this version of dispatch(),
                                // which just returns FALSE, indicating
                                // that the message wasn't handled by
                                // the dialog box at all.

            WORD result;        // this holds the value which will be
                                // returned by run().  If the dialog box
                                // needs to return a success code, that
                                // code should be stored here by the
                                // dialog handler.

private:

    virtual LPSTR getDialogName() = 0;
                                // returns a far pointer to a string
                                // that contains the name of the dialog
                                // resource for the current dialog box.
                                // This is used in run() to load the
                                // resource.

    static  BOOL CALLBACK     _export dlgProc( HWND, UINT, WPARAM, LPARAM );
                                // the dialog proc that windows calls
                                // when it has messages for the current
                                // dialog.

    static  ModalDialog *curDlg;
                                // this holds a pointer to the currenly
                                // active ModalDialog.  It is set up when
                                // the constructor is called, and is used
                                // by dlgProc() to route messages to the
                                // right dispatch() function.

};

//---------------------------------------------------------------------
//
//  class Window
//
//      provides the basic functionality for a normal window under
//      windows.
//
//---------------------------------------------------------------------

class Window : public virtual WinBase
{

public:

    virtual BOOL create();      // creates the window.  This involves
                                // registering the window class if
                                // it hasn't already been registered,
                                // creating the actual window, and
                                // inserting the window into the internal
                                // window list.

    virtual WORD run();         // provides the message loop.

protected:

    virtual LONG dispatch( UINT, WPARAM, LPARAM );
                                // dispatches messages in a class derived
                                // from Window.  When wndProc() receives
                                // a message, it determines which Window
                                // object the message should be routed
                                // to, and calls dispatch() for that object.
                                // If the dispatch() function in a derived
                                // class does not handle any particular
                                // message, it should pass that message
                                // on down to the dispatch() function in
                                // its base class.  The version of dispatch()
                                // in Window itself just calls DefWindowProc.

    virtual BOOL registerClass() = 0;
                                // the derived class should override this
                                // function and register a window class
                                // defined appropriately for the program.

    virtual BOOL createWindow() = 0;
                                // the derived class should override this
                                // function and create a window that's
                                // appropriate for the program.

    static  LRESULT CALLBACK   _export wndProc( HWND, UINT, WPARAM, LPARAM );
                                // the window proc that windows calls
                                // when it has messages for any window
                                // in the current application.  wndProc()
                                // posts the message to the appropriate
                                // window.

            void insert();      // should be called from createWindow()
                                // after successfully calling the windows
                                // function CreateWindow().  insert() adds
                                // the current object to the Window list,
                                // enabling normal dispatching of messages
                                // to the current object.

private:

    static Window *winList;     // have to maintain a list of Windows so
    Window *nextWin;            // that we can dispatch messages to the
                                // correct one.

    static Window *inCreate;    // sort-of kludgy.  But there are messages
                                // that are sent to a window during the
                                // call to CreateWindow().  We assume that
                                // any message received by wndProc() during
                                // a call to CreateWindow() is meant for
                                // the window being created, and dispatch
                                // those messages to that window.

};

inline HWND WinBase::hWnd()
{
    return hWindow;
}

inline ModalDialog::ModalDialog( HWND hOwner ) : result( 0 )
{
    assert( curDlg == 0 );
    curDlg = this;
    hWindow = hOwner;
}

inline ModalDialog::~ModalDialog()
{
    assert( curDlg != 0 );
    curDlg = 0;
}

inline void Window::insert()
{
    nextWin = winList;
    winList = this;
}

#endif  // __TODOWIN_H

