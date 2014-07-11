// Borland C++ - (C) Copyright 1991, 1992 by Borland International
//
//  Windows HELLO WORLD - C++ Version 

#define  STRICT
#include <windows.h>
#include <stdlib.h>
#include <string.h>

LRESULT CALLBACK _export WndProc( HWND hWnd, UINT iMessage,
                                 WPARAM wParam, LPARAM lParam );

class Main
{
public:
	static HINSTANCE hInstance;
	static HINSTANCE hPrevInstance;
	static int nCmdShow;
	static int MessageLoop( void );
};

class Window
{
protected:
    HWND hWnd;
public:
    // Provide (read) access to the window's handle in case it is needed
    // elsewhere.
    HWND GetHandle( void ) { return hWnd; }

    BOOL Show( int nCmdShow ) { return ShowWindow( hWnd, nCmdShow ); }
    void Update( void ) { UpdateWindow( hWnd ); }
    // Pure virtual function makes Window an abstract class.
    virtual long WndProc( UINT iMessage, WPARAM wParam, LPARAM lParam ) = 0;
};

class MainWindow : public Window
{
private:
    static char szClassName[14];
    // Helper function used by Paint function; it is used as a
    // callback function by LineDDA.
    static void FAR PASCAL LineFunc( int X, int Y, LPSTR lpData );
public:
    // Register the class only AFTER WinMain assigns appropriate
    // values to static members of Main and only if no previous
    // instances of the program exist (a previous instance would
    // have already performed the registration).
    static void Register( void )
    {
        WNDCLASS wndclass;   // Structure used to register Windows class.

        wndclass.style         = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc   = ::WndProc;
        wndclass.cbClsExtra    = 0;
        // Reserve extra bytes for each instance of the window;
        // we will use these bytes to store a pointer to the C++
        // (MainWindow) object corresponding to the window.
        // the size of a 'this' pointer depends on the memory model.
        wndclass.cbWndExtra    = sizeof( MainWindow * );
        wndclass.hInstance     = Main::hInstance;
        wndclass.hIcon         = LoadIcon( Main::hInstance, "whello" );
        wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW );
        wndclass.hbrBackground = (HBRUSH)GetStockObject( WHITE_BRUSH );
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = szClassName;

        if ( ! RegisterClass( &wndclass ) )
            exit( FALSE );
    }

    // Do not create unless previously registered.
    MainWindow( void )
    {
        // Pass 'this' pointer in lpParam of CreateWindow().
        hWnd = CreateWindow( szClassName,
            szClassName,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            0,
            CW_USEDEFAULT,
            0,
            NULL,
            NULL,
            Main::hInstance,
            (LPSTR) this );
        if ( ! hWnd )
            exit( FALSE );

        Show( Main::nCmdShow );
        Update();
    }
    long WndProc( UINT iMessage, WPARAM wParam, LPARAM lParam );  

    // Print a message in the client rectangle.
    void Paint( void );
    // Struct used by Paint to pass information to callback function
    // used by LineDDA
    struct LINEFUNCDATA
    {
        HDC hDC;
        char FAR *LineMessage;
        int MessageLength;
        LINEFUNCDATA( char *Message )
        {
            hDC = 0;
            MessageLength = strlen( Message );
            LineMessage = new far char [MessageLength+1];
            lstrcpy( LineMessage, Message );
        };
        ~LINEFUNCDATA( void ) { delete LineMessage; }
    };
};

HINSTANCE Main::hInstance = 0;
HINSTANCE Main::hPrevInstance = 0;
int Main::nCmdShow = 0;

int Main::MessageLoop( void )
{
    MSG msg;

    while( GetMessage( &msg, NULL, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    return msg.wParam;
}

char MainWindow::szClassName[] = "Hello, World!";

void MainWindow::Paint( void )
{
    PAINTSTRUCT ps;
    RECT rect;
    FARPROC lpLineFunc;
    LINEFUNCDATA LineFuncData( "Borland C++ welcomes you to the Wonderful World of Windows Programming!" );

    lpLineFunc = MakeProcInstance( (FARPROC) MainWindow::LineFunc, (HINSTANCE)Main::hInstance );
    BeginPaint( hWnd, &ps );
    GetClientRect( hWnd, (LPRECT) &rect );
    LineFuncData.hDC = ps.hdc;
    SetTextAlign( ps.hdc, TA_BOTTOM );
    LineDDA( 0, 0, 0,
	 rect.bottom / 2, (LINEDDAPROC)lpLineFunc, (LPARAM) &LineFuncData );
    EndPaint( hWnd, &ps );
    FreeProcInstance( lpLineFunc );
}

void FAR PASCAL _export MainWindow::LineFunc( int X, int Y, LPSTR lpData )
{
    LINEFUNCDATA FAR * lpLineFuncData = (LINEFUNCDATA FAR *) lpData;
    TextOut( lpLineFuncData->hDC, X, Y,
             lpLineFuncData->LineMessage, lpLineFuncData->MessageLength );
}

long MainWindow::WndProc( UINT iMessage, WPARAM wParam, LPARAM lParam )
{
    switch (iMessage)
    {
        case WM_CREATE:
	    break;
        case WM_PAINT:
            Paint();
            break;
        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;
        default:
            return DefWindowProc( hWnd, iMessage, wParam, lParam );
    }
    return 0;
}

// If data pointers are near pointers
#if defined(__SMALL__) || defined(__MEDIUM__)
inline Window *GetPointer( HWND hWnd )
{
    return (Window *) GetWindowWord( hWnd, 0 );
}
inline void SetPointer( HWND hWnd, Window *pWindow )
{
    SetWindowWord( hWnd, 0, (WORD) pWindow );
}

// else pointers are far
#elif defined(__LARGE__) || defined(__COMPACT__)
inline Window *GetPointer( HWND hWnd )
{
    return (Window *) GetWindowLong( hWnd, 0 );
}
inline void SetPointer( HWND hWnd, Window *pWindow )
{
    SetWindowLong( hWnd, 0, (LONG) pWindow );
}

#else
    #error Choose another memory model!
#endif

LRESULT  CALLBACK _export WndProc( HWND hWnd, UINT iMessage, WPARAM wParam,
                                 LPARAM lParam )
{
    // Pointer to the (C++ object that is the) window.
    Window *pWindow = GetPointer( hWnd );

    // The pointer pWindow will have an invalid value if the WM_CREATE
    // message has not yet been processed (we respond to the WM_CREATE
    // message by setting the extra bytes to be a pointer to the
    // (C++) object corresponding to the Window identified
    // by hWnd).  The messages that
    // precede WM_CREATE must be processed without using pWindow so we
    // pass them to DefWindowProc.
    // How do we know in general if the pointer pWindow is invalid?
    // Simple: Windows allocates the window extra bytes using LocalAlloc
    // which zero initializes memory; thus, pWindow will have a value of
    // zero before we set the window extra bytes to the 'this' pointer.
    // Caveat emptor: the fact that LocalAlloc will zero initialize the
    // window extra bytes is not documented; therefore, it could change
    // in the future.

    if ( pWindow == 0 )
    {
        if ( iMessage == WM_CREATE )
        {
            LPCREATESTRUCT lpcs;

            lpcs = (LPCREATESTRUCT) lParam;
            pWindow = (Window *) lpcs->lpCreateParams;

            // Store a pointer to this object in the window's extra bytes;
            // this will enable us to access this object (and its member
            // functions) in WndProc where we are
            // given only a handle to identify the window.
            SetPointer( hWnd, pWindow );
            // Now let the object perform whatever
            // initialization it needs for WM_CREATE in its own
            // WndProc.
	    return pWindow->WndProc( iMessage, wParam, lParam );
        }
        else
            return DefWindowProc( hWnd, iMessage, wParam, lParam );
    }
    else
        return pWindow->WndProc( iMessage, wParam, lParam );
}

// Turn off warning: Parameter 'lpszCmdLine' is never used in function WinMain(unsigned int,unsigned int,char far*,int)
#pragma argsused

// Turn off warning: 'MainWnd' is assigned a value that is never used in function WinMain(unsigned int,unsigned int,char far*,int)
#pragma option -w-aus

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine,
                    int nCmdShow )
{
    Main::hInstance = hInstance;
    Main::hPrevInstance = hPrevInstance;
    Main::nCmdShow = nCmdShow;

    // A Windows class should be registered with Windows before any windows
    // of that type are created.
    // Register here all Windows classes that will be used in the program.
    // Windows classes should not be registered if an instance of
    // the program is already running.
    if ( ! Main::hPrevInstance ) {
        MainWindow::Register();
    }

    MainWindow MainWnd;

    return Main::MessageLoop();
}
