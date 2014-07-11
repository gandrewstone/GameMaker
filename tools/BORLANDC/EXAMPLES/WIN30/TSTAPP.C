// Borland C++ - (C) Copyright 1991 by Borland International

//*******************************************************************
//
// program - TstApp.c
// purpose - A windows program to send messages to msgwnd server.
//           This illustrates how DDE messages can be sent from
//	     one application and received by another.
//
//           To use this program, build MSGWND and TSTAPP. There
//	     are project files for this. When you run TSTAPP, MSGWND
//           will be started automatically.
//
//*******************************************************************

#define STRICT
#include <windows.h>
#include <dde.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <bios.h>
#include <ctype.h>
#include <io.h>
#include <dos.h>

#include "tstapp.h"
#include "msgproc.h"
#include "ddeclnt.h"

#define MAX_QRT_LEN 100

typedef struct scrollkeys
  {
    WORD wVirtkey;
    int  iMessage;
    WORD wRequest;
  } SCROLLKEYS;

SCROLLKEYS key2scroll [] =
  {
    {VK_HOME,  WM_COMMAND, IDM_HOME},
    {VK_END,   WM_VSCROLL, SB_BOTTOM},
    {VK_PRIOR, WM_VSCROLL, SB_PAGEUP},
    {VK_NEXT,  WM_VSCROLL, SB_PAGEDOWN},
    {VK_UP,    WM_VSCROLL, SB_LINEUP},
    {VK_DOWN,  WM_VSCROLL, SB_LINEDOWN},
    {VK_LEFT,  WM_HSCROLL, SB_PAGEUP},
    {VK_RIGHT, WM_HSCROLL, SB_PAGEDOWN}
  };

# define NUMKEYS (sizeof key2scroll / sizeof key2scroll[0])

// data initialized by first instance
typedef struct tagSETUPDATA
  {
    char   szAppName[10]; // name of application
  } SETUPDATA;

SETUPDATA SetUpData;

// Data that can be referenced throughout the
// program but not passed to other instances

HINSTANCE hInst;         // hInstance of application
HWND      hWndMain;      // hWnd of main window
//char      szAppName[10]; // name of application

int       xChar, yChar, yCharnl;
int       xClient, yClient;

LOGFONT   cursfont;
HFONT     holdsfont;
HFONT     hnewsfont;

RECT      wrect;

// window scroll/paint stuff
int       nVscrollMax, nHscrollMax;
int       nVscrollPos, nHscrollPos;
int       numlines;
int       maxwidth;
int       nVscrollInc, nHscrollInc;
int       nPaintBeg, nPaintEnd;
int       nPageMaxLines;

// for scroll print
RECT      rect;
int       blanklen;
char      blanks[256];

// to keep lines
# define  MAX_KEEP   50
HANDLE    hkeep[MAX_KEEP + 1];
int       hwm_keep;

// function prototypes

int      PASCAL        WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                               LPSTR lpszCmdLine, int cmdShow);

void                   InitTstApp(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                 LPSTR lpszCmdLine, int cmdShow);
void                   InitTstAppFirst(HINSTANCE hInstance);
void                   InitTstAppAdded(HINSTANCE hPrevInstance);
void                   InitTstAppEvery(HINSTANCE hInstance, int cmdShow);
void                   CloseTstApp(void);
BOOL FAR PASCAL        About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK       TstAppWndProc(HWND hWnd, UINT message,
				     WPARAM wParam, LPARAM lParam);

void                   SetupScroll(HWND hWnd);
void                   TstAppPaint(HWND hWnd);
void                   ScrollPrint(HWND hWnd, char *str);
void                   ClearKeep(void);

//*******************************************************************
// WinMain - TstApp main
//
// paramaters:
//             hInstance     - The instance of this instance of this
//                             application.
//             hPrevInstance - The instance of the previous instance
//                             of this application. This will be 0
//                             if this is the first instance.
//             lpszCmdLine   - A long pointer to the command line that
//                             started this application.
//             cmdShow       - Indicates how the window is to be shown
//                             initially. ie. SW_SHOWNORMAL, SW_HIDE,
//                             SW_MIMIMIZE.
//
// returns:
//             wParam from last message.
//
//*******************************************************************
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int cmdShow)
{
    MSG   msg;

    InitTstApp(hInstance, hPrevInstance, lpszCmdLine, cmdShow);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return(msg.wParam);
}

//*******************************************************************

//*******************************************************************
// InitTstApp - init the TstApp application
//
// paramaters:
//             hInstance     - The instance of this instance of this
//                             application.
//             hPrevInstance - The instance of the previous instance
//                             of this application. This will be 0
//                             if this is the first instance.
//             lpszCmdLine   - A long pointer to the command line that
//                             started this application.
//             cmdShow       - Indicates how the window is to be shown
//                             initially. ie. SW_SHOWNORMAL, SW_HIDE,
//                             SW_MIMIMIZE.
//
//*******************************************************************
# pragma argsused
void InitTstApp(HINSTANCE hInstance, HINSTANCE hPrevInstance,
               LPSTR lpszCmdLine, int cmdShow)
{
    if (! hPrevInstance)
        InitTstAppFirst(hInstance);
    else
        InitTstAppAdded(hPrevInstance);

    InitTstAppEvery(hInstance, cmdShow);
}

//*******************************************************************
// InitTstAppFirst - done only for first instance of TstApp
//
// paramaters:
//             hInstance     - The instance of this instance of this
//                             application.
//
//*******************************************************************
void InitTstAppFirst(HINSTANCE hInstance)
{
    WNDCLASS wcTstAppClass;

    LoadString(hInstance, IDS_NAME, (LPSTR) SetUpData.szAppName, 10);

    // fill in window class information

    wcTstAppClass.lpszClassName = SetUpData.szAppName;
    wcTstAppClass.hInstance     = hInstance;
    wcTstAppClass.lpfnWndProc   = TstAppWndProc;
    wcTstAppClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcTstAppClass.hIcon         = LoadIcon(hInstance, SetUpData.szAppName);
    wcTstAppClass.lpszMenuName  = (LPSTR) SetUpData.szAppName;
    wcTstAppClass.hbrBackground = GetStockObject(WHITE_BRUSH);
    wcTstAppClass.style         = CS_HREDRAW | CS_VREDRAW;
    wcTstAppClass.cbClsExtra    = 0;
    wcTstAppClass.cbWndExtra    = 0;

    // register the class

    RegisterClass(&wcTstAppClass);
}

//*******************************************************************
// InitTstAppAdded - done only for added instances of TstApp
//
// paramaters:
//             hPrevInstance - The instance of the previous instance
//                             of this application.
//
//*******************************************************************
void InitTstAppAdded(HINSTANCE hPrevInstance)
{
    // get the results of the initialization of first instance
    GetInstanceData(hPrevInstance, (BYTE*) &SetUpData, sizeof(SETUPDATA));
}

//*******************************************************************
// InitTstAppEvery - done for every instance of TstApp
//                   Will create the window and sets a fixed font
//                   for tabular display.
//
// paramaters:
//             hInstance     - The instance of this instance of this
//                             application.
//             cmdShow       - Indicates how the window is to be shown
//                             initially. ie. SW_SHOWNORMAL, SW_HIDE,
//                             SW_MIMIMIZE.
//
//*******************************************************************
void InitTstAppEvery(HINSTANCE hInstance, int cmdShow)
{
    TEXTMETRIC tm;
    HDC        hDC;

    hInst = hInstance;       // save for use by window procs

    hWndMain = CreateWindow(
                  SetUpData.szAppName,     // window class name
                  SetUpData.szAppName,     // window title
                  WS_OVERLAPPEDWINDOW |    // type of window
                    WS_HSCROLL |
                    WS_VSCROLL,
		  CW_USEDEFAULT,           // x  window location
                  0,                       // y
                  500,                     // cx and size
                  150,                     // cy
                  NULL,                    // no parent for this window
                  NULL,                    // use the class menu
                  hInstance,               // who created this window
                  NULL                     // no parms to pass on
                  );

    hDC = GetDC(hWndMain);

    // build fied screen font
    cursfont.lfHeight         =  6;
    cursfont.lfWidth          =  6;
    cursfont.lfEscapement     =  0;
    cursfont.lfOrientation    =  0;
    cursfont.lfWeight         =  FW_NORMAL;
    cursfont.lfItalic         =  FALSE;
    cursfont.lfUnderline      =  FALSE;
    cursfont.lfStrikeOut      =  FALSE;
    cursfont.lfCharSet        =  ANSI_CHARSET;
    cursfont.lfOutPrecision   =  OUT_DEFAULT_PRECIS;
    cursfont.lfClipPrecision  =  CLIP_DEFAULT_PRECIS;
    cursfont.lfQuality        =  DEFAULT_QUALITY;
    cursfont.lfPitchAndFamily =  FIXED_PITCH | FF_DONTCARE;
    strcpy((char *)cursfont.lfFaceName, "System");

    hnewsfont = CreateFontIndirect((LPLOGFONT) &cursfont);
    holdsfont = SelectObject(hDC, hnewsfont);

    // get text metrics for paint
    GetTextMetrics(hDC, &tm);
    xChar = tm.tmAveCharWidth;
    yChar = tm.tmHeight + tm.tmExternalLeading;
    yCharnl = tm.tmHeight;

    // init blank line
    blanklen = 255;
    memset(blanks, ' ', blanklen);

    ReleaseDC(hWndMain, hDC);

    ShowWindow(hWndMain, cmdShow);
    UpdateWindow(hWndMain);
}

//*******************************************************************
// CloseTstApp - done at termination of every instance of TstApp
//               This is where wrapup code that should be run
//               at the termination of the application should go.
//
//*******************************************************************
void CloseTstApp()
{
    ClearKeep();
    CloseMsgWnd();
    DeleteObject(hnewsfont);
}

//*******************************************************************
// About - handle about dialog box messages
//
// paramaters:
//             hDlg          - The window handle for this dialog box
//             message       - The message number
//             wParam        - The WPARAM parameter for this message
//             lParam        - The LPARAM parameter for this message
//
//*******************************************************************
# pragma argsused
BOOL FAR PASCAL About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG)
        return(TRUE);

    else if (message == WM_COMMAND)
    {
        switch (wParam)
        {
            case IDOK:
		EndDialog(hDlg, TRUE);
                return(TRUE);

            default:
                return(TRUE);
        }
    }

    return(FALSE);
}

//*******************************************************************

//*******************************************************************
// TstAppWndProc - every message for this instance will come here
//
//   Handle the messages for this application.
//
// paramaters:
//             hWnd          - The window handle for this message
//             message       - The message number
//             wParam        - The WPARAM parameter for this message
//             lParam        - The LPARAM parameter for this message
//
// returns:
//             depends on message.
//
//*******************************************************************
LRESULT CALLBACK TstAppWndProc(HWND hWnd, UINT message,
			       WPARAM wParam, LPARAM lParam)
{
    static   int msgno;

    FARPROC  lpproc;
    char     buf[128];
    int      i;
    HANDLE   hTmp;
    LPSTR    lpTmp;

    switch (message)
    {
        case WMU_DDE_CLOSED:
            // This message is sent by the ddeclnt module when the dde
            // conversation is terminated.
            CloseMsgWnd();
            break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDM_QUIT:
                    // User has selected QUIT from menu.
                    PostMessage(hWnd, WM_CLOSE, 0, 0L);
                    break;

                case IDM_HOME:
                    // Home key was hit.
                    SendMessage(hWnd, WM_HSCROLL, SB_TOP, 0L); /* not set up */
                    SendMessage(hWnd, WM_VSCROLL, SB_TOP, 0L);
                    break;

                case IDM_ABOUT:
                    // User has selected ABOUT from menu.
                    lpproc = MakeProcInstance((FARPROC)About, hInst);
                    DialogBox(hInst,
                              MAKEINTRESOURCE(ABOUT),
                              hWnd,
                              (DLGPROC)lpproc);
                    FreeProcInstance(lpproc);
                    break;

                case IDM_MESSAGE:
                    // User has selected MSG from menu.

                    // Format the message to send.
                    sprintf(buf, "TstApp hWnd %04X Msg # %04d: ", hWnd, ++msgno);

                    // Show it in our window.
                    ScrollPrint(hWnd, buf);

		    // Send it via dde to msgwnd application.
                    SendMsgWndMsg(hWnd, "print", buf);
                    break;

                case IDM_CLEAR:
                    // User has selected CLEAR from menu.

                    // Tell msgwnd applicaton to clear its window.
                    SendMsgWndMsg(hWnd, "clear", NULL);
                    break;

                case IDM_LINES:
                    // User has selected LINES from menu.

                    // Ask msgwnd applicaton how many lines it has recieved.
                    hTmp = GetLines(hWnd);
                    if (hTmp)
                    {
                        lpTmp = GlobalLock(hTmp);
                        if (lpTmp)
			{
                            _fstrcpy(buf, lpTmp);
                            ScrollPrint(hWnd, buf);
                            GlobalUnlock(hTmp);
                        }

                        GlobalFree(hTmp);
                    }
                    break;

                default:
                    break;
            }
            break;

        case WM_SIZE:
            // Save size of window client area.

            yClient = HIWORD(lParam);
            xClient = LOWORD(lParam);

            // Go setup scroll ranges and file display area based upon
            // client area size.

            SetupScroll(hWnd);
            break;

        case WM_VSCROLL:
            // React to the various vertical scroll related actions.
            switch (wParam)
            {
                case SB_TOP:
                    nVscrollInc = -nVscrollPos;
                    break;

                case SB_BOTTOM:
                    nVscrollInc = nVscrollMax - nVscrollPos;
                    break;

                case SB_LINEUP:
		    nVscrollInc = -1;
                    break;

                case SB_LINEDOWN:
                    nVscrollInc = 1;
                    break;

                case SB_PAGEUP:
                    nVscrollInc = -max(1, yClient / yChar);
                    break;

                case SB_PAGEDOWN:
                    nVscrollInc = max(1, yClient / yChar);
                    break;

                case SB_THUMBPOSITION:
                    nVscrollInc = LOWORD(lParam) - nVscrollPos;
                    break;

                case SB_THUMBTRACK:
		    nVscrollInc = LOWORD(lParam) - nVscrollPos;
                    break;

                default:
                    nVscrollInc = 0;
            }

            nVscrollInc = max(-nVscrollPos,
                              min(nVscrollInc, nVscrollMax - nVscrollPos));
            if (nVscrollInc)
            {
                nVscrollPos += nVscrollInc;
                ScrollWindow(hWnd, 0, -yChar * nVscrollInc, NULL, NULL);
                SetScrollPos(hWnd, SB_VERT, nVscrollPos, TRUE);
                UpdateWindow(hWnd);
            }
            break;

        case WM_HSCROLL:
            // React to the various horizontal scroll related actions.
	    switch (wParam)
            {
                case SB_LINEUP:
                    nHscrollInc = -1;
                    break;

                case SB_LINEDOWN:
                    nHscrollInc = 1;
                    break;

                case SB_PAGEUP:
                    nHscrollInc = -8;
                    break;

                case SB_PAGEDOWN:
                    nHscrollInc = 8;
                    break;

                case SB_THUMBPOSITION:
                    nHscrollInc = LOWORD(lParam) - nHscrollPos;
		    break;

                case SB_THUMBTRACK:
                    nHscrollInc = LOWORD(lParam) - nHscrollPos;
                    break;

                default:
                    nHscrollInc = 0;
            }

            nHscrollInc = max(-nHscrollPos,
                              min(nHscrollInc, nHscrollMax - nHscrollPos));
            if (nHscrollInc)
            {
                nHscrollPos += nHscrollInc;
                ScrollWindow(hWnd, -xChar * nHscrollInc, 0, NULL, NULL);
                SetScrollPos(hWnd, SB_HORZ, nHscrollPos, TRUE);
                UpdateWindow(hWnd);
            }
            break;

        case WM_KEYDOWN:
            // Translate various keydown messages to appropriate horizontal
            // and vertical scroll actions.

            for (i = 0; i < NUMKEYS; i++)
            {
                if (wParam == key2scroll[i].wVirtkey)
                {
                    SendMessage(hWnd, key2scroll[i].iMessage,
                                key2scroll[i].wRequest, 0L);
                    break;
                }
            }
            break;

        case WM_PAINT:
            // Go paint the client area of the window with the appropriate
            // part of the selected file.

	    TstAppPaint(hWnd);
            break;

        case WM_DESTROY:
            // This is the end if we were closed by a DestroyWindow call.
            CloseTstApp();         // take any necessary wrapup action.
            PostQuitMessage(0);    // this is the end...
            break;

        case WM_QUERYENDSESSION:
            // If we return TRUE we are saying it's ok with us to end the
            // windows session.
            CloseTstApp();         // take any necessary wrapup action.
            return((long) TRUE);   // we agree to end session.

        case WM_CLOSE:
            // Tell windows to terminate us.
            DestroyWindow(hWnd);
            break;

	default:
            // Let windows handle all messages we choose to ignore.
            return(DefWindowProc(hWnd, message, wParam, lParam));
    }

    return(0L);
}

//*******************************************************************
// SetupScroll - setup scroll ranges
//
//   Setup the vertical and horizontal scroll ranges and positions
//   of the applicatons main window based on:
//
//       numlines - The maximum number of lines to display.
//       maxwidth - The maximum width of any line to display.
//
//   The resulting variables, nVscrollPos and nPageMaxLines, are used
//   by the function TstAppPaint to determine what part of the selected
//   file to display in the window.
//
// paramaters:
//             hWnd          - The callers window handle
//
//*******************************************************************
void SetupScroll(HWND hWnd)
{
    // numlines established during open
    nVscrollMax = max(0, numlines - yClient / yChar);
    nVscrollPos = min(nVscrollPos, nVscrollMax);

    nHscrollMax = max(0, maxwidth - xClient / xChar);
    nHscrollPos = min(nHscrollPos, nHscrollMax);

    SetScrollRange (hWnd, SB_VERT, 0, nVscrollMax, FALSE);
    SetScrollPos   (hWnd, SB_VERT, nVscrollPos, TRUE);

    SetScrollRange (hWnd, SB_HORZ, 0, nHscrollMax, FALSE);
    SetScrollPos   (hWnd, SB_HORZ, nHscrollPos, TRUE);

    nPageMaxLines = min(numlines, yClient / yChar);

    rect.left = 0;
    rect.top = 0;
    rect.right = xClient;
    rect.bottom = yClient;

    blanklen = rect.right / xChar + 1;
}

//*******************************************************************
// TstAppPaint - paint the main window
//
// This function is responsible for redisplaying a portion of the saved
// strings.  Which strings it displays depends on the current scroll
// position.
//
// paramaters:
//             hWnd          - The callers window handle
//
//*******************************************************************
void TstAppPaint(HWND hWnd)
{
    PAINTSTRUCT  ps;
    HDC          hDC;
    char         currec[256];
    int          ypos;
    LPSTR        lcp;
    int          i;
    int          ndone;

    // Get display context.
    BeginPaint(hWnd, (LPPAINTSTRUCT) &ps);
    hDC = ps.hdc;

    // Select fixed font.
    SelectObject(hDC, hnewsfont);

    // Setup scroll ranges.
    SetupScroll(hWnd);

    // See if we have any lines to show.
    if (hwm_keep)
      {
        // Y position of bottom line in client area.
        ypos = rect.bottom - yChar;

	// Index into keep list of first line (from bottom) to show.
        i = nVscrollMax - nVscrollPos;

        ndone = 1;
        while (ndone)
          {
            lcp = GlobalLock(hkeep[i]);
            if (lcp)
              {
                // We must fill line with blanks to width of window
                // or else some previous longet text might show through.
                _fstrcpy(currec, blanks);

                // Line to show.
                _fstrncpy(currec, lcp, _fstrlen(lcp));

                // Send to window.
                TextOut(hDC,
                        xChar * (-nHscrollPos + 0),
                        ypos,
                        currec,
                        blanklen);

                // New Y is one character height higher.
                ypos -= yChar;

                GlobalUnlock(hkeep[i]);
              }

            // Index of next keep string to show.
            i++;

            // No use drawing lines beyond top of client area.
            // They would not show, so don't wast the energy.
            if (ypos < -yChar)
              ndone = 0;

            // Have we done all of the lines?
            if (i > (hwm_keep - 1))
              ndone = 0;
	  }
      }

    // Release the display context.
    EndPaint(hWnd, (LPPAINTSTRUCT) &ps);
}

//*******************************************************************
// ScrollPrint - saves string sent to it and cause it to display
//
// This function gets a string and saves it in a list of strings.
// The oldest string is deleted when the list reaches its maximum
// size.
//
// Depending on the scroll position at the time the string is recieved
// one of two methods of displaying the string will be used:
//
// If the scroll position was at the bottom, the window is scrolled up
// one line, the bottom line is made invalid, and windows is told to
// repaint. This will cause only the new string to be output, resulting
// in minimum redraw on the screen.
//
// If the scroll position was not at the bottom of the window because
// the user had been looking at previous messages, windows will be told
// to redraw the entire window starting with the new string.
//
// paramaters:
//             hWnd          - The window to put the message in.
//             str           - the string to print in window.
//
//*******************************************************************
void ScrollPrint(HWND hWnd, char *str)
{
    int   i;
    int   lstr;
    LPSTR lcp;
    RECT  trect;

    // If our keep stack is full free oldest member.
    if (hwm_keep >= MAX_KEEP)
      GlobalFree(hkeep[hwm_keep]);

    // Move all handles to make room for new one.
    for (i = hwm_keep; i > 0; i--)
      hkeep[i] = hkeep[i - 1];

    // If keep stack not yet full add one to high watter mark.
    if (hwm_keep < MAX_KEEP)
      hwm_keep++;

    // Make sure we know how many saved lines there are.
    numlines = hwm_keep;

    // Length of new string.
    lstr = strlen(str);

    // Is it longer than any previous string.
    if (lstr > maxwidth)
      maxwidth = lstr;

    // Get storage to save it.
    hkeep[0] = GlobalAlloc(GMEM_MOVEABLE, (DWORD) (lstr + 1));
    if (hkeep[0])
      {
        // Lock it down to get address.
        lcp = GlobalLock(hkeep[0]);
        if (lcp)
          {
	    // Save string.
            _fstrcpy(lcp, str);
            GlobalUnlock(hkeep[0]);
          }
      }

    // See what we have to do to display it efficently.
    if (!(nVscrollMax - nVscrollPos))
      {
        // We are scrolled to bottom of list.

        // Scroll contents of window up one character hehght.
        ScrollWindow(hWnd, 0, -yChar, &rect, &rect);

        // Set scroll position to last line
        nVscrollPos = numlines - yChar / yClient;

        // Tell windows to repaint only the bottom line of window.
        GetClientRect(hWnd, &trect);
        trect.top = trect.bottom - yChar;
	InvalidateRect(hWnd, &trect, TRUE);
      }
    else
      {
        // We are not scrolled to bottom of list.

        // Set scroll position to last line.
        nVscrollPos = numlines - yChar / yClient;

        // Tell windows to repaint the entire window.
        InvalidateRect(hWnd, NULL, TRUE);
      }
}

//*******************************************************************
// ClearKeep - free all saved strings
//
//
//*******************************************************************
void ClearKeep()
  {
    int i;

    for (i = 0; i < hwm_keep; i++)
      {
        GlobalFree(hkeep[i]);
        hkeep[i] = 0;
      }

    // Reset counters.
    numlines = hwm_keep = 0;
  }

//*******************************************************************
