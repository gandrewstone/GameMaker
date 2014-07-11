// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//*******************************************************************
//
// program - Hdump.c
// purpose - a windows program to dump a file in hex.
//
//*******************************************************************

#define  STRICT
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <bios.h>
#include <ctype.h>
#include <io.h>
#include <dos.h>
#include <dir.h>

#include "hdump.h"

typedef struct scrollkeys
  {
    WORD wVirtkey;
    int  iMessage;
    WORD wRequest;
  } SCROLLKEYS;

SCROLLKEYS key2scroll [] =
  {
    { VK_HOME,  WM_COMMAND, IDM_HOME },
    { VK_END,   WM_VSCROLL, SB_BOTTOM },
    { VK_PRIOR, WM_VSCROLL, SB_PAGEUP },
    { VK_NEXT,  WM_VSCROLL, SB_PAGEDOWN },
    { VK_UP,    WM_VSCROLL, SB_LINEUP },
    { VK_DOWN,  WM_VSCROLL, SB_LINEDOWN },
    { VK_LEFT,  WM_HSCROLL, SB_PAGEUP },
    { VK_RIGHT, WM_HSCROLL, SB_PAGEDOWN }
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

HINSTANCE hInst;                              // hInstance of application
HWND      hWndMain;                           // hWnd of main window

int       xChar, yChar, yCharnl;              // character size
int       xClient, yClient;                   // client window size

LOGFONT   cursfont;                           // font structure
HFONT     holdsfont;                          // handle of original font
HFONT     hnewsfont;                          // handle of new fixed font

// window scroll/paint stuff
int       nVscrollMax, nHscrollMax;           // scroll ranges
int       nVscrollPos, nHscrollPos;           // current scroll positions
int       numlines;                           // number of lines in file
int       maxwidth;                           // width of display format
int       nVscrollInc, nHscrollInc;           // scroll increments
int       nPageMaxLines;                      // max lines on screen

// file open dialog stuff
char      szFileSpec[80];                     // file spec for initial search
char      szDefExt[5];                        // default extension
char      szFileName[80];                     // file name
char      szFilePath[80];                     // file path
WORD      wFileAttr;                          // attribute for search
WORD      wStatus;                            // status of search

// dump file stuff
char      szFName[64];                        // file to display
FILE     *fh;                                 // handle of open file
LONG      fsize;                              // size of file

// function prototypes

int      PASCAL        WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                               LPSTR lpszCmdLine, int cmdShow);

void                   InitHdump(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                 LPSTR lpszCmdLine, int cmdShow);
void                   InitHdumpFirst(HINSTANCE hInstance);
void                   InitHdumpAdded(HINSTANCE hPrevInstance);
void                   InitHdumpEvery(HINSTANCE hInstance, int cmdShow);
void                   CloseHdump(void);
BOOL CALLBACK          About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK       HdumpWndProc(HWND hWnd, UINT message,
        				    WPARAM wParam, LPARAM lParam);

void                   SetupScroll(HWND hWnd);
void                   HdumpPaint(HWND hWnd);
char                  *SnapLine(char *szBuf, LPSTR mem, int len,
                                int dwid, char *olbl);

int                    DoFileOpenDlg(HINSTANCE hInst, HWND hWnd,
                                     char *szFileSpecIn, char *szDefExtIn,
                                     WORD wFileAttrIn,
                                     char *szFilePathOut, char *szFileNameOut);
extern BOOL CALLBACK FileOpenDlgProc(HWND hDlg, UINT iMessage,
				       WPARAM wParam, LPARAM lParam);

//*******************************************************************
// WinMain - Hdump main
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

    // Go init this application.
    InitHdump(hInstance, hPrevInstance, lpszCmdLine, cmdShow);

    // Get and dispatch messages for this applicaton.
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    return(msg.wParam);
}

//*******************************************************************

//*******************************************************************
// InitHdump - init the Hdump application
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
#pragma argsused
void InitHdump(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	       LPSTR lpszCmdLine, int cmdShow)
{
    if (! hPrevInstance)              // if no previous instance, this is first
        InitHdumpFirst(hInstance);
    else
        InitHdumpAdded(hPrevInstance);   // this is not first instance

    InitHdumpEvery(hInstance, cmdShow);  // initialization for all instances
}

//*******************************************************************
// InitHdumpFirst - done only for first instance of Hdump
//
// paramaters:
//             hInstance     - The instance of this instance of this
//                             application.
//
//*******************************************************************
void InitHdumpFirst(HINSTANCE hInstance)
{
    WNDCLASS wcHdumpClass;

    // Get string from resource with application name.
    LoadString(hInstance, IDS_NAME, (LPSTR) SetUpData.szAppName, 10);

    // Define the window class for this application.
    wcHdumpClass.lpszClassName = SetUpData.szAppName;
    wcHdumpClass.hInstance     = hInstance;
    wcHdumpClass.lpfnWndProc   = HdumpWndProc;
    wcHdumpClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcHdumpClass.hIcon         = LoadIcon(hInstance, SetUpData.szAppName);
    wcHdumpClass.lpszMenuName  = (LPSTR) SetUpData.szAppName;
    wcHdumpClass.hbrBackground = GetStockObject(WHITE_BRUSH);
    wcHdumpClass.style         = CS_HREDRAW | CS_VREDRAW;
    wcHdumpClass.cbClsExtra    = 0;
    wcHdumpClass.cbWndExtra    = 0;

    // Register the class
    RegisterClass(&wcHdumpClass);
}

//*******************************************************************
// InitHdumpAdded - done only for added instances of Hdump
//
// paramaters:
//             hPrevInstance - The instance of the previous instance
//                             of this application.
//
//*******************************************************************
void InitHdumpAdded(HINSTANCE hPrevInstance)
{
    // get the results of the initialization of first instance
    GetInstanceData(hPrevInstance, (BYTE*) &SetUpData, sizeof(SETUPDATA));
}

//*******************************************************************
// InitHdumpEvery - done for every instance of Hdump
//
// paramaters:
//             hInstance     - The instance of this instance of this
//                             application.
//             cmdShow       - Indicates how the window is to be shown
//                             initially. ie. SW_SHOWNORMAL, SW_HIDE,
//                             SW_MIMIMIZE.
//
//*******************************************************************
void InitHdumpEvery(HINSTANCE hInstance, int cmdShow)
{
    TEXTMETRIC tm;
    HDC        hDC;

    hInst = hInstance;       // save for use by window procs

    // Create applications main window.
    hWndMain = CreateWindow(
                  SetUpData.szAppName,     // window class name
                  SetUpData.szAppName,     // window title
                  WS_OVERLAPPEDWINDOW |    // type of window
		    WS_HSCROLL |
                    WS_VSCROLL,
                  CW_USEDEFAULT,           // x  window location
                  0,                       // y
                  CW_USEDEFAULT,           // cx and size
                  0,                       // cy
                  NULL,                    // no parent for this window
                  NULL,                    // use the class menu
                  hInstance,               // who created this window
                  NULL                     // no parms to pass on
                  );

    // Get the display context.
    hDC = GetDC(hWndMain);

    // Build fixed screen font. Needed to display hex formated dump.
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

    // Install the font in the current display context.
    holdsfont = SelectObject(hDC, hnewsfont);

    // get text metrics for paint
    GetTextMetrics(hDC, &tm);
    xChar = tm.tmAveCharWidth;
    yChar = tm.tmHeight + tm.tmExternalLeading;
    yCharnl = tm.tmHeight;

    // Release the display context.
    ReleaseDC(hWndMain, hDC);

    // Update display of main window.
    ShowWindow(hWndMain, cmdShow);
    UpdateWindow(hWndMain);
}

//*******************************************************************
// CloseHdump - done at termination of every instance of Hdump
//
//*******************************************************************
void CloseHdump()
{
    DeleteObject(hnewsfont);
}

//*******************************************************************
// About - handle about dialog messages
//
// paramaters:
//             hDlg          - The window handle for this message
//             message       - The message number
//             wParam        - The WPARAM parameter for this message
//             lParam        - The LPARAM parameter for this message
//
//*******************************************************************
#pragma argsused
BOOL CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
// HdumpWndProc - handles messages for this application
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
LRESULT CALLBACK HdumpWndProc(HWND hWnd, UINT message,
			     WPARAM wParam, LPARAM lParam)
{
    static char     sztFilePath[64];  // we remember last directory searched
    static char     sztFileExt[5];
    static char     sztFileSpec[64];
    static char     sztFileName[64];

    DLGPROC  lpproc;                  // pointer to thunk for dialog box
    char     buf[128];                // temp buffer
    int      i;
    FILE    *fh;

    switch (message)
    {
        case WM_CREATE:
            // Start file search at current directory
            getcwd(sztFilePath, sizeof(sztFilePath));
            strcat(sztFilePath, "\\");
            strcpy(sztFileExt, ".*");
            strcpy(sztFileName, "");

            return(DefWindowProc(hWnd, message, wParam, lParam));

        case WM_COMMAND:
	    switch (wParam)
            {
                case IDM_QUIT:
                    // User selected Quit on menu
                    PostMessage(hWnd, WM_CLOSE, 0, 0L);
                    break;

                case IDM_HOME:
                    // Used to implement home to topleft from keyboard.
                    SendMessage(hWnd, WM_HSCROLL, SB_TOP, 0L); /* not set up */
                    SendMessage(hWnd, WM_VSCROLL, SB_TOP, 0L);
                    break;

		case IDM_ABOUT:
                    // Display about box.
                    lpproc = (DLGPROC)MakeProcInstance((FARPROC)About, hInst);
                    DialogBox(hInst,
                              MAKEINTRESOURCE(ABOUT),
                              hWnd,
                              lpproc);
                    FreeProcInstance((FARPROC)lpproc);
                    break;

                case IDM_OPEN:
                    // Setup initial search path for open dialog box.
                    strcpy(sztFileSpec, sztFilePath);
		    strcat(sztFileSpec, "*");
                    strcat(sztFileSpec, sztFileExt);

                    // Go ask user to select file to display.
                    if (DoFileOpenDlg(hInst,
                                      hWnd,
                                      sztFileSpec,
                                      sztFileExt,
                                      0x4010, // drives & subdirectories
                                      sztFilePath,
                                      sztFileName))
                    {
                        // If user said OK construct file path/file name
			// to open.
                        strcpy(szFName, sztFilePath);
                        strcat(szFName, sztFileName);

                        // Show file name in window caption.
                        sprintf(buf, "hdump - %s", szFName);
                        SetWindowText(hWnd, buf);

                        // Determine file size and some display paramaters.
                        fh = fopen(szFName, "r+b");
                        if (fh)
                        {
                            fsize = filelength(fileno(fh));
			    numlines = (int) ((fsize + 16L - 1L) / 16L);
                            SnapLine(buf, szFName, 16, 16,NULL); // display width
                            maxwidth = strlen(buf);
                            nVscrollPos = 0;
                            nHscrollPos = 0;

                            fclose(fh);
                        }

                        else  // if file open failed.
                        {
                            SetWindowText(hWnd, "hdump");
                            numlines = 0;
			    maxwidth = 0;
                        }

                        // Go setup scroll ranges for this file.
                        SetupScroll(hWnd);

                        // Show first part of file.
                        InvalidateRect(hWnd, NULL, TRUE);
                        UpdateWindow(hWnd);
                    }
                    break;

                default:
		    break;
          }
          break;

        case WM_SIZE:
            // Save size of window client area.
            if (lParam)
              {
                yClient = HIWORD(lParam);
                xClient = LOWORD(lParam);

                yClient = (yClient / yCharnl + 1) * yCharnl;

		lParam = MAKELONG(xClient, yClient);

                // Go setup scroll ranges and file display area based upon
                // client area size.
                SetupScroll(hWnd);

                return(DefWindowProc(hWnd, message, wParam, lParam));
              }
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
            HdumpPaint(hWnd);
            break;

        case WM_DESTROY:
            // This is the end if we were closed by a DestroyWindow call.
            CloseHdump();         // take any necessary wrapup action.
            PostQuitMessage(0);   // this is the end...
            break;

	case WM_QUERYENDSESSION:
            // If we return TRUE we are saying it's ok with us to end the
            // windows session.
            CloseHdump();         // take any necessary wrapup action.
            return((long) TRUE);  // we agree to end session.

        case WM_CLOSE:
            // Tell windows to destroy our window.
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
// Setup the vertical and horizontal scroll ranges and positions
// of the applicatons main window based on:
//
//     numlines - The maximum number of lines to display in a hexdump
//                of the selected file.
//     maxwidth - The width of each line to display as formated for
//                the hexdump.
//
// The resulting variables, nVscrollPos and nPageMaxLines, are used
// by the function HdumpPaint to determine what part of the selected
// file to display in the window.
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
}

//*******************************************************************
// HdumpPaint - paint the main window
//
// If a file has been selected, as indicated by numlines being
// greater than 0, this module will read and display a portion
// of the file as determined by the current size and scroll
// position of the window.
//
// paramaters:
//             hWnd          - The callers window handle
//
//*******************************************************************
void HdumpPaint(HWND hWnd)
{
    PAINTSTRUCT  ps;
    HDC          hDC;
    LONG         offset;
    int          tlen;
    int          lpos;
    char         buf[128];
    int          i;
    int          len;
    int          hfile;
    HANDLE       hbuff;
    LPSTR        lpbuff;
    LPSTR        lcp;

    BeginPaint(hWnd, (LPPAINTSTRUCT) &ps);
    hDC = ps.hdc;

    // Establish fixed font in display context.
    SelectObject(hDC, hnewsfont);

    if (numlines)
    {
	// Open the file to display
        // (files should not stay open over multiple windows messages)
        if ((hfile = _lopen(szFName, OF_READ)) != -1)
        {
            // Get a buffer to hold data to fill screen.
            len = nPageMaxLines * 16;
            hbuff = GlobalAlloc(GMEM_MOVEABLE, (DWORD) len);
            if (hbuff)
            {
                // Lock the buffer to get a pointer to it.
                lpbuff = GlobalLock(hbuff);
                if (lpbuff)
                {
		    // Get offset into file to start display.
                    offset = (LONG) nVscrollPos * 16L;

                    if (_llseek(hfile, offset, 0) != -1L)
                    {
                        tlen = _lread(hfile, lpbuff, len);
                        if (tlen != -1)
                        {
                            lpos = 0;
                            lcp = lpbuff;

                            for (i = nVscrollPos; i < (nVscrollPos + nPageMaxLines); i++)
                            {
				if (tlen > 16)
                                    len = 16;
                                else
                                    len = tlen;
                                tlen -= len;

                                SnapLine(buf, lcp, len, 16, (char *) (i * 16));

                                TextOut(hDC,
                                        xChar * (-nHscrollPos + 0),
                                        yChar * lpos++,
                                        buf,
                                        strlen(buf));

                                lcp += 16;
                            }
                        }
                    }

                    GlobalUnlock(hbuff);
                }

                GlobalFree(hbuff);
            }

            _lclose(hfile);
	}
    }

    EndPaint(hWnd, (LPPAINTSTRUCT) &ps);
}

//*******************************************************************
// SnapLine - dump a line of memory
//
// paramaters:
//             szBuf         - buffer to hold result
//             mem           - pointer to memory to format
//             len           - length to format
//             dwid          - max display width (8 or 16 recomended)
//             olbl          - label to put at start of line
//
// returns:
//             A pointer to szBbuf.
//
//*******************************************************************
char *SnapLine(char *szBuf, LPSTR mem, int len, int dwid, char *olbl)
{
    int            i;
    int            j;
    unsigned char  c;
    unsigned char  buff[80];
    unsigned char  tbuf[80];

    if (len > dwid)
        len = dwid;

    *szBuf = 0;
    i = 0;
    j = 0;

    // Show offset for this line.
    sprintf((char *)tbuf, "%04X  ", olbl);
    strcpy(szBuf, (char *)tbuf);

    // Format hex portion of line and save chars for ascii portion
    for (i = 0; i < len; i++)
    {
        c = *mem++;

        sprintf((char *)tbuf, "%02X ", c);
        strcat(szBuf, (char *)tbuf);

        if (c >= 32 && c < 127)
            buff[i] = c;
        else
            buff[i] = 46;
    }

    j = dwid - i;

    buff[i] = 0;

    // Fill out hex portion of short lines.
    for (i = j; i > 0; i--)
        strcat(szBuf, "   ");

    // Add ascii portion to line.
    sprintf((char *)tbuf, " |%s|", (char *)buff);
    strcat(szBuf, (char *)tbuf);

    // Fill out end of short lines.
    for (i = j; i > 0; i--)
        strcat(szBuf, " ");

    return(szBuf);
}

//*******************************************************************
// DoFileOpenDlg - invoke FileOpenDlgProc to get name of file to open
//
// Setup call to FileOpenDlgProc to let user select path/filename
//
// paramaters:
//             hInst         - instance of caller
//             hWnd          - window of caller
//             szFileSpecIn  - file path to search initially
//             szDefExtIn    - file extension search initially
//             wFileAttrIn   - attribute for 'DlgDirList' to use in search
//             szFilePathOut - path of selected file
//             szFileNameOut - name of selected file
//
// returns:
//             1 - if a file selected
//             0 - if file not selected
//
//*******************************************************************
DoFileOpenDlg(HINSTANCE hInst, HWND hWnd,
              char *szFileSpecIn, char *szDefExtIn,
              WORD wFileAttrIn,
              char *szFilePathOut, char *szFileNameOut)
{
    DLGPROC   lpfnFileOpenDlgProc;
    int       iReturn;

    // Save starting file spec.
    strcpy(szFileSpec, szFileSpecIn);
    strcpy(szDefExt, szDefExtIn);
    wFileAttr = wFileAttrIn;

    lpfnFileOpenDlgProc = (DLGPROC)MakeProcInstance((FARPROC)FileOpenDlgProc, hInst);

    iReturn = DialogBox(hInst,
                        MAKEINTRESOURCE(OPENFILE),
                        hWnd,
                        lpfnFileOpenDlgProc);

    FreeProcInstance((FARPROC)lpfnFileOpenDlgProc);

    // Return selected file spec.
    strcpy(szFilePathOut, szFilePath);
    strcpy(szFileNameOut, szFileName);

    return(iReturn);
}

//*******************************************************************
// FileOpenDlgProc - get the name of a file to open
//
// paramaters:
//             hDlg          - The window handle of this dialog box
//             message       - The message number
//             wParam        - The WPARAM parameter for this message
//             lParam        - The LPARAM parameter for this message
//
// returns:
//             depends on message.
//
//*******************************************************************
BOOL CALLBACK FileOpenDlgProc(HWND hDlg, UINT iMessage,
				WPARAM wParam, LPARAM lParam)
{
    static char   curpath[64];

    char          cLastChar;
    int           nLen;
    struct ffblk  fileinfo;

    switch (iMessage)
    {
        case WM_INITDIALOG:
            // Save current working directory (will be restored on exit)
            getcwd(curpath, sizeof(curpath));

            SendDlgItemMessage(hDlg, IDD_FNAME, EM_LIMITTEXT, 80, 0L);

            // Fill list box with files from starting file spec.
            DlgDirList(hDlg,
                       szFileSpec,
                       IDD_FLIST,
                       IDD_FPATH,
                       wFileAttr);

            // Save starting file spec.
            SetDlgItemText(hDlg, IDD_FNAME, szFileSpec);
            return(TRUE);

        case WM_COMMAND:
            switch (wParam)
            {
                case IDD_FLIST:
                    switch (HIWORD(lParam))
                    {
                        case LBN_SELCHANGE:
                            if (DlgDirSelect(hDlg, szFileName, IDD_FLIST))
				strcat(szFileName, szFileSpec);
                            SetDlgItemText(hDlg, IDD_FNAME, szFileName);
                            break;

                        case LBN_DBLCLK:
                            if (DlgDirSelect(hDlg, szFileName, IDD_FLIST))
                            {
                                strcat(szFileName, szFileSpec);
                                DlgDirList(hDlg, szFileName, IDD_FLIST, IDD_FPATH, wFileAttr);
                                SetDlgItemText(hDlg, IDD_FNAME, szFileSpec);
                            }
                            else
                            {
				SetDlgItemText(hDlg, IDD_FNAME, szFileName);
                                SendMessage(hDlg, WM_COMMAND, IDOK, 0L);
                            }
                            break;
                    }
                    break;

                case IDD_FNAME:
                    if (HIWORD(lParam) == EN_CHANGE)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDOK),
                                     (BOOL) SendMessage((HWND)LOWORD(lParam),
                                                        WM_GETTEXTLENGTH, 0, 0L));
		    }
                    break;

                case IDOK:
                    GetDlgItemText(hDlg, IDD_FNAME, szFileName, 80);

                    nLen  = strlen(szFileName);
                    cLastChar = *AnsiPrev(szFileName, szFileName + nLen);

                    if (cLastChar == '\\' || cLastChar == ':')
                        strcat(szFileName, szFileSpec);

                    if (strchr(szFileName, '*') || strchr(szFileName, '?'))
		    {
                        if (DlgDirList(hDlg, szFileName, IDD_FLIST, IDD_FPATH, wFileAttr))
                        {
                            strcpy(szFileSpec, szFileName);
                            SetDlgItemText(hDlg, IDD_FNAME, szFileSpec);
                        }
                        else
                            MessageBeep(0);
                        break;
                    }

                    // Fill list with new spec.
                    if (DlgDirList(hDlg, szFileName, IDD_FLIST, IDD_FPATH, wFileAttr))
		    {
                        // Save new spec.
                        strcpy(szFileSpec, szFileName);
                        SetDlgItemText(hDlg, IDD_FNAME, szFileSpec);
                        break;
                    }

                    // Since we fell through, szFileName was not a search path.
                    szFileName[nLen] = '\0';

                    // Make sure file exists.
                    if (findfirst(szFileName, &fileinfo, 0))
                    {
			strcat(szFileName, szDefExt);

                        // Make sure file exists.
                        if (findfirst(szFileName, &fileinfo, 0))
                        {
                            // No, beep and break.
                            MessageBeep(0);
                            break;
                        }
                    }

                    // Get current path to return.
                    GetDlgItemText(hDlg, IDD_FPATH, szFilePath, 80);
		    strupr(szFilePath);
                    if (szFilePath[strlen(szFilePath) - 1] != '\\')
                        strcat(szFilePath, "\\");

                    // Return selected file name.
                    strcpy(szFileName, fileinfo.ff_name);

                    // Restore current working directory.
                    chdir(curpath);

                    // Terminate this dialog box.
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    // Restore current working directory
                    chdir(curpath);

                    // Terminate this dialog box.
                    EndDialog(hDlg, FALSE);
                    break;

		default:
                    return(FALSE);
            }
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}

//*******************************************************************
