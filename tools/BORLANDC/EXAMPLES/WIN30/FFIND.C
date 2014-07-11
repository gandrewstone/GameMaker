// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//*******************************************************************
//
// program - Ffind.c
// purpose - a Windows program to find files on system drives.
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

#include "ffind.h"

#define MAX_QRT_LEN 100

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

char      qrtxt[MAX_QRT_LEN];                 // dialog input string

char      file_name_pattern[MAX_QRT_LEN];     // file name search pattern
int       file_count;                         // count of files found

int       xChar, yChar, yCharnl;              // character size
int       xClient, yClient;                   // client window size

LOGFONT   cursfont;                           // font structure
HFONT     holdsfont;                          // handle of original font
HFONT     hnewsfont;                          // handle of new fixed font

HANDLE    hdlgr;                              // handle of dialog resource

int       ndrives;                            // number of drives in system
char      fdrives[10];                        // letters of floppy drives
char      hdrives[30];                        // letters of hard drives

FARPROC   lpModelessProc;                     // pointer to proc for modeless box
HWND      hQryDlgBox;                         // handle of modeless dialog box

// function prototypes

int      PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPSTR lpszCmdLine, int cmdShow);

void            InitFfind(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                          LPSTR lpszCmdLine, int cmdShow);
void            InitFfindFirst(HINSTANCE hInstance);
void            InitFfindAdded(HINSTANCE hPrevInstance);
void            InitFfindEvery(HINSTANCE hInstance, int cmdShow);
void            CloseFfind(void);

LRESULT CALLBACK   FfindWndProc(HWND hWnd, UINT message,
                                WPARAM wParam, LPARAM lParam);

void            InitMainDlg(HWND hWnd);
BOOL CALLBACK   MainDlgBoxProc(HWND hDlg, UINT message,
			   WPARAM wParam, LPARAM lParam);

int             FindFile(char *drives, char *pattern);
int             DoADir(char *patternp, char *patternn, char *include);
int             FnMatch(char *pat, char *name);
void            FindDrives(char *fdrives, char *hdrives);
char           *FmtEntry(char *buf, char *name, char *patternp,
                         unsigned time, unsigned date, long size);

//*******************************************************************
// WinMain - ffind main
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
    InitFfind(hInstance, hPrevInstance, lpszCmdLine, cmdShow);

    // Get and dispatch messages for this applicaton.
    while (GetMessage(&msg, NULL, 0, 0))
    {
      if(!hQryDlgBox || !IsWindow(hQryDlgBox) ||
         !IsDialogMessage( hQryDlgBox, &msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    return(msg.wParam);
}

//*******************************************************************

//*******************************************************************
// InitFfind - init the ffind application
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
void InitFfind(HINSTANCE hInstance, HINSTANCE hPrevInstance,
               LPSTR lpszCmdLine, int cmdShow)
{
    if (!hPrevInstance)              // if no previous instance, this is first
        InitFfindFirst(hInstance);
    else
        InitFfindAdded(hPrevInstance);   // this is not first instance

    InitFfindEvery(hInstance, cmdShow);  // initialization for all instances
}

//*******************************************************************
// InitFfindFirst - done only for first instance of ffind
//
// paramaters:
//             hInstance     - The instance of this instance of this
//                             application.
//
//*******************************************************************
void InitFfindFirst(HINSTANCE hInstance)
{
    WNDCLASS wcFfindClass;

    // Get string from resource with application name.
    LoadString(hInstance, IDS_NAME, (LPSTR) SetUpData.szAppName, 10);

    // Define the window class for this application.
    wcFfindClass.lpszClassName = SetUpData.szAppName;
    wcFfindClass.hInstance     = hInstance;
    wcFfindClass.lpfnWndProc   = FfindWndProc;
    wcFfindClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcFfindClass.hIcon         = LoadIcon(hInstance, SetUpData.szAppName);
    wcFfindClass.lpszMenuName  = (LPSTR) NULL;
    wcFfindClass.hbrBackground = GetStockObject(WHITE_BRUSH);
    wcFfindClass.style         = CS_HREDRAW | CS_VREDRAW;
    wcFfindClass.cbClsExtra    = 0;
    wcFfindClass.cbWndExtra    = 0;

    // Register the class
    RegisterClass(&wcFfindClass);
}

//*******************************************************************
// InitFfindAdded - done only for added instances of ffind
//
// paramaters:
//             hPrevInstance - The instance of the previous instance
//                             of this application.
//
//*******************************************************************
#pragma argsused
void InitFfindAdded(HINSTANCE hPrevInstance)
{
    // get the results of the initialization of first instance
    GetInstanceData(hPrevInstance, (BYTE*) &SetUpData, sizeof(SETUPDATA));
}

//*******************************************************************
// InitFfindEvery - done for every instance of ffind
//
// paramaters:
//             hInstance     - The instance of this instance of this
//                             application.
//             cmdShow       - Indicates how the window is to be shown
//                             initially. ie. SW_SHOWNORMAL, SW_HIDE,
//                             SW_MIMIMIZE.
//
//*******************************************************************
#pragma argsused
void InitFfindEvery(HINSTANCE hInstance, int cmdShow)
{
    TEXTMETRIC tm;
    HDC        hDC;

    hInst = hInstance;       // save for use by window procs

    // Create applications main window.
    hWndMain = CreateWindow(
                  SetUpData.szAppName,     // window class name
                  SetUpData.szAppName,     // window title
                  WS_BORDER |              // type of window
                    WS_CAPTION |
                    WS_SYSMENU |
                    WS_MINIMIZEBOX,
                  10,                      // x - same as dialog box
                  19,                      // y
                  256,                     // cx
                  123,                     // cy
                  NULL,                    // no parent for this window
                  NULL,                    // use the class menu
                  hInstance,               // who created this window
                  NULL                     // no parms to pass on
                  );

    // Get the display context.
    hDC = GetDC(hWndMain);

    // Build fixed screen font. Needed to display columinar report.
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

    hnewsfont = CreateFontIndirect( &cursfont);

    // Install the font in the current display context.
    holdsfont = SelectObject(hDC, hnewsfont);

    // get text metrics for paint
    GetTextMetrics(hDC, &tm);
    xChar = tm.tmAveCharWidth;
    yChar = tm.tmHeight + tm.tmExternalLeading;
    yCharnl = tm.tmHeight;

    // Release the display context.
    ReleaseDC(hWndMain, hDC);

    // Create a thunk for the main dialog box proc function.
    lpModelessProc = MakeProcInstance((FARPROC)MainDlgBoxProc, hInst);

    // Find drive letters for all drives in system.
    // We will only use the hard drives.
    FindDrives(fdrives, hdrives);
    hdrives[9] = 0; // limit drives to max of 9
    ndrives = strlen(hdrives);

    InitMainDlg(hWndMain);
}

//*******************************************************************
// CloseFfind - done at termination of every instance of ffind
//
//*******************************************************************
void CloseFfind()
{
}

//*******************************************************************

//*******************************************************************
// FfindWndProc - handles messages for this application
//
// paramaters:
//             hWnd          - The window handle for this message
//             message       - The message number
//             wParam        - The WPARAM parmater for this message
//             lParam        - The LPARAM parmater for this message
//
// returns:
//             depends on message.
//
//*******************************************************************
LRESULT CALLBACK FfindWndProc(HWND hWnd, UINT message,
			     WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_MOVE:
            // Move the dialog box on top of our main window every
            // time the main window moves.
            if (IsWindow(hQryDlgBox))
                SendMessage(hQryDlgBox, message, wParam, lParam);
            break;

        case WM_SETFOCUS:
            // Always set the input focus to the dialog box.
            if (IsWindow(hQryDlgBox))
                SendMessage(hQryDlgBox, message, wParam, lParam);
            break;

        case WM_CLOSE:
            // Tell windows to destroy our window.
            DestroyWindow(hWnd);
            break;

        case WM_QUERYENDSESSION:
            // If we return TRUE we are saying it's ok with us to end the
            // windows session.
            return((long) TRUE);  // we agree to end session.

        case WM_ENDSESSION:
            // If wParam is not zero, it meany every application said ok
            // to WM_QUERYENDSESSION messages, so we are really ending.
            if (wParam)           // if all apps aggreed to end session.
                CloseFfind();     // This is the end. We will not get a
                                   // WM_DESTROY message on end session.
            break;

        case WM_DESTROY:
	    // This is the end if we were closed by a DestroyWindow call.

	    // Remove font
	    DeleteObject(hnewsfont);

	    CloseFfind();
            PostQuitMessage(0);
            break;

        default:
            return(DefWindowProc(hWnd, message, wParam, lParam));
    }

    return(0L);
}

//*******************************************************************

//*******************************************************************
// InitMainDlg - put up modeless dialog box
//
// paramaters:
//             hWnd          - The window handle of the caller
//
//*******************************************************************
void InitMainDlg(HWND hWnd)
{
    // This structure describes the fixed portion at the begining of a
    // dialog template. The part we are going to modify is dtItemCount
    // to cause the proper number of drive selector boxes to display.
    typedef struct tagDLGTEMPLATE
    {
        long dtStyle;
        BYTE dtItemCount;
        int  dtX;
        int  dtY;
        int  dtCX;
        int  dtCY;
        char dtResourceName[1];
    }   DLGTEMPLATE;
    DLGTEMPLATE FAR *dltp;

    // Modifying dialog box parms on the fly.

    // Load the dialog box resource.
    hdlgr = LoadResource(hInst,
                         FindResource(hInst,
                                      MAKEINTRESOURCE(MAIN),
                                      RT_DIALOG));

    if (hdlgr)
    {
        // Lock the resource so we have a pointer to it.
        dltp = (DLGTEMPLATE FAR *) LockResource(hdlgr);
        if (dltp)
        {
            // Change the number of items to display. The drive boxes are
            // defined last so we can truncate the ones not needed.
            dltp->dtItemCount = (BYTE) (ndrives + QX_DRV1 - QX_1);

            // Create the dialog box. Its proc is the next function down.
            if (!CreateDialogIndirect(hInst,
                                      (LPSTR) dltp,
                                      hWnd,
                                      (DLGPROC)lpModelessProc))
            {
                // Unlock dialog resource we locked above.
                UnlockResource(hdlgr);

                // Free it.
                FreeResource(hdlgr);

                // Zero handle to it.
                hdlgr = 0;
            }
        }
    }
}

//*******************************************************************
// MainDlgBoxProc - handle Main dialog messages (modeless)
//
// This is a modeless dialog box procedure that controls this
// entire application.
//
// paramaters:
//             hDlg          - The window handle for this message
//             message       - The message number
//             wParam        - The WPARAM parameter for this message 
//             lParam        - The LPARAM parameter for this message
//
//*******************************************************************
#pragma argsused
BOOL CALLBACK MainDlgBoxProc(HWND hDlg, UINT message,
			   WPARAM wParam, LPARAM lParam)
{
    static HWND   hlistwnd;
    static RECT   wrect;

    int           x, y, w, h, i, rc;
    char         *cp, *cpd, tmp[30], sdrives[30];
    HANDLE        hCursor;

    switch (message)
    {
        case WM_INITDIALOG:
            // Save the handle of this proc for use by main window proc.
            hQryDlgBox = hDlg;

            // Set names of drives in drive box text.
            cp = hdrives;
            for (i = 0; i < ndrives; i++)
            {
                sprintf(tmp, "%c:", *cp++);
                SetDlgItemText(hDlg, QX_DRV1 + i, tmp);
            }

            // Select the first drive.
            SendMessage(GetDlgItem(hDlg, QX_DRV1), BM_SETCHECK, TRUE, 0L);

            // Get position of dialog box window.
            GetWindowRect(hDlg, (LPRECT) &wrect);
            w = wrect.right - wrect.left;
            h = wrect.bottom - wrect.top;

            // Move main application window to same position.
            SetWindowPos(hWndMain, hDlg,
                         wrect.left, wrect.top, w, h,
                         0);

            // Establish initial value of file name pattern box.
            strcpy(file_name_pattern, "*.*");
            SetDlgItemText(hDlg, QX_PATTERN, file_name_pattern);
            SetFocus(GetDlgItem(hDlg, QX_PATTERN));

            // Save handle of list box control because we use it a lot.
            hlistwnd = GetDlgItem(hDlg, QX_LIST);

            // Install fixed fonts for those controls that need it.
            SendMessage(GetDlgItem(hDlg, QX_3), WM_SETFONT, (WPARAM)hnewsfont, FALSE);
            SendMessage(GetDlgItem(hDlg, QX_LIST), WM_SETFONT, (WPARAM)hnewsfont, FALSE);
            SendMessage(GetDlgItem(hDlg, QX_COUNT), WM_SETFONT, (WPARAM)hnewsfont, FALSE);
            break;

        case WM_MOVE:
            // Always keep this dialog box on top of main window.
            GetWindowRect(hWndMain, (LPRECT) &wrect);
            x = wrect.left;
            y = wrect.top;
            w = wrect.right - wrect.left;
            h = wrect.bottom - wrect.top;
            MoveWindow(hDlg, x, y, w, h, 1);
            break;

        case WM_SYSCOMMAND:
            // Pass WM_SYSCOMMAND messages on to main window so both
            // main window and dialog box get iconized, minimized etc.
            // in parallel.
            SendMessage(hWndMain, message, wParam, lParam);
            break;

        case WM_COMMAND:
            switch (wParam)
            {
                case QX_PATTERN:                 // Pattern string
                    GetDlgItemText(hDlg, wParam, qrtxt, MAX_QRT_LEN);
                    break;

                case QX_SEARCH:                  // File button
                    strcpy(file_name_pattern, qrtxt);

                    // Build list of selected drives.
                    cp = hdrives;
                    cpd = sdrives;
                    for ( i = 0; i < ndrives; i++)
                    {
                        if (SendMessage(GetDlgItem(hDlg, QX_DRV1 + i), BM_GETCHECK, 0, 0L))
                            *cpd++ = *cp;
                        cp++;
                    }
                    *cpd = 0;

                    // Clear any previous list and count.
                    SendMessage(hDlg, WM_COMMAND, QX_CLEARW, 0L);

                    // Tell list control not to display till we are done
                    SendDlgItemMessage(hDlg, QX_LIST, WM_SETREDRAW, FALSE, 0L);

                    // Load hour-glass cursor.
                    hCursor = LoadCursor(NULL, IDC_WAIT);
                    SetCursor(hCursor);

                    // Go find files matching pattern on selected drives.
                    rc = FindFile(sdrives, file_name_pattern);

                    // Tell list control ok to redraw itself.
                    SendDlgItemMessage(hDlg, QX_LIST, WM_SETREDRAW, TRUE, 0L);

                    // Reload arrow cursor.
                    SetCursor(hCursor);

                    // See if out of memory error.
                    if (rc < 0)
                    {
                        MessageBox(hDlg,
                                   "Out of memory",
                                   "--ERROR--",
                                   MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
                    }

                    // Show count of files fount.
                    sprintf(tmp, "# %5d", file_count);
                    SetDlgItemText(hDlg, QX_COUNT, tmp);

                    // Show list window.
                    InvalidateRect(hlistwnd, NULL, TRUE);
                    UpdateWindow(hlistwnd);
                    break;

                case QX_CLEARW:
                    // Clear list window.
                    SendMessage(hlistwnd, LB_RESETCONTENT, 0L, 0L);
                                        InvalidateRect(hlistwnd, NULL, TRUE);
                    UpdateWindow(hlistwnd);

                    // Set count to 0.
                    file_count = 0;
                    sprintf(tmp, "# %5d", file_count);
                    SetDlgItemText(hDlg, QX_COUNT, tmp);
                    break;

                case QX_CANCEL:                  // Cancel button
                    // Tell main application window we want to quit.
                    SendMessage(hWndMain, WM_CLOSE, 0, 0L);
                    break;

                default:
                    break;
            }
            break;

        case WM_CLOSE:
            // Unlock dialog resource we locked above.
            UnlockResource(hdlgr);

            // Free it.
            FreeResource(hdlgr);

            // Zero handle to it.
            hdlgr = 0;

            // Zero handle to this dialog window.
            hQryDlgBox = 0;

            // Tell main window to close.
            PostMessage(hWndMain, WM_CLOSE, 0, 0L);

            // Destroy ourseleves.
            DestroyWindow(hDlg);
            break;

        default:
            return FALSE;
    }

    return(TRUE);
}

//*******************************************************************

//*******************************************************************
// FindFile - find files matching a pattern files on specified drives
//
// paramaters:
//             drives   - An array of drive letters to search.
//             pattern  - A file name pattern.
//
// returns:
//             Number of files found matching pattern.
//             If unable to add a name to listbox, number is returned as
//             a negative.
//
//*******************************************************************
FindFile(char *drives, char *pattern)
{
    char     tree[3];
    int      nfiles;
    int      rc;

    rc = 0;
    nfiles = 0;
    file_count = 0;
    tree[1] = ':';
    tree[2] = 0;
    while (*drives)
    {
        tree[0] = *drives++;
        nfiles = DoADir(tree, "*.*", pattern);
        if (nfiles >= 0)
            file_count += nfiles;
        else
        {
            file_count -= nfiles;
            rc = -file_count;
            break;
        }
    }

    return(rc);
}

//*******************************************************************
// DoADir - search a directory for files matching a file name pattern
//          recursivly search sub directories
//
// paramaters:
//             patternp  - A path to search.
//             patternn  - A file name pattern to use to find directories.
//             include   - A file name pattern to use to select files.
//
// returns:
//             Number of files found matching include.
//             If unable to add a name to listbox, number is returned as
//             a negative.
//
//*******************************************************************
int DoADir(char *patternp, char *patternn, char *include)
{
    char          patternw[80];
    char          npatternp[64];
    char          buf[128];
    int           files;
    int           mfiles;
    int           have_subs;
    int           nfiles;
    LONG          lrc;
    struct ffblk  fileinfo;

    strcpy(patternw, patternp);
    strcat(patternw, "\\");

    strcat(patternw, patternn);

    files = 0;
    mfiles = 0;
    have_subs = 0;

    if (!findfirst(patternw, &fileinfo, FA_DIREC))
    {
        do
        {
            if (fileinfo.ff_attrib & FA_DIREC)  // subdirectory
            {
                if (fileinfo.ff_name[0] != '.')  // ignore . and ..
                    have_subs = 1;
            }
            else                              // file
            {
                files++;

                if (FnMatch(include, fileinfo.ff_name))
                {
                    mfiles++;

                    FmtEntry(buf, fileinfo.ff_name, patternp,
                             fileinfo.ff_ftime, fileinfo.ff_fdate, fileinfo.ff_fsize);

                    lrc = SendDlgItemMessage(hQryDlgBox, QX_LIST, LB_ADDSTRING, 0,
					    (LONG) ((LPSTR) buf));

                    if (lrc == LB_ERR || lrc == LB_ERRSPACE)
                        return(-mfiles); // error return
                }
            }
        }
        while (!findnext(&fileinfo));
    }

    if (have_subs)
    {
        if (!findfirst(patternw, &fileinfo, FA_DIREC))
        {
            do
            {
                if (fileinfo.ff_attrib & FA_DIREC)  // subdirectory
                {
                    if (fileinfo.ff_name[0] != '.')  // ignore . and ..
                    {
                        strcpy(npatternp, patternp);
                        strcat(npatternp, "\\");
                        strcat(npatternp, fileinfo.ff_name);

                        nfiles = DoADir(npatternp, patternn, include);

                        if (nfiles >= 0)
                            mfiles += nfiles;
                        else
                        {
                            mfiles -= nfiles;
                            return(-mfiles); // error return
                        }
                    }
                }
            }
            while (!findnext(&fileinfo));
        }
    }

    return(mfiles);
}

//*******************************************************************
// FindDrives - find floppy and hard drives in system
//
// paramaters:
//             fdrives - An array to hold floppy drive letters.
//             hdrives - An array to hold hard drive letters.
//
//*******************************************************************
void FindDrives(char *fdrives, char *hdrives)
{
    unsigned int  equip;
    unsigned int  maxflop;
    unsigned int  savedriveno;
    unsigned int  maxdriveno;
    unsigned int  ui;
    unsigned int  driveno;
    int           nfdrives;
    int           nhdrives;

    savedriveno = getdisk() + 1;               // save current drive

    equip = biosequip();                       // find how many floppys
    maxflop = ((equip >> 6) & 0x03) + 1;

    maxdriveno = setdisk(savedriveno - 1);     // find max drive in system

    nfdrives = 0;
    nhdrives = 0;
    for (ui = 1; ui <= maxdriveno; ui++)       // for all potential drives
    {
        if ((ui <= maxflop) || (ui >= 3))
        {
            setdisk(ui - 1);                   // set to drive
            driveno = getdisk() + 1;           // get current drive

            if (driveno == ui)                 // if set successful
            {
                if (ui >= 3)                   // C: assumed to be first hard ?
                    hdrives[nhdrives++] = (char) (ui + '@');
                else if (ui <= maxflop)        // because set to B: successful even if no B:
                    fdrives[nfdrives++] = (char) (ui + '@');
            }
        }
    }

    fdrives[nfdrives] = 0;
    hdrives[nhdrives] = 0;

    setdisk(savedriveno - 1);                  // reset current drive
}

//*******************************************************************

//*******************************************************************
// FnMatch - test if a file name matches a file name pattern.
//           handles * and ? wildcard characters.
//
// paramaters:
//             pat   - A file name pattern (ie. xyz?.*)
//             name  - A file name to test against pat (ie. xyz1.c)
//
// returns:
//             1 - if match
//             0 - if not a match
//
//*******************************************************************
FnMatch(char *pat, char *name)
{
    int   match;
    int   ndone;
    char *cpp;
    char *cpn;

    cpp = pat;
    cpn = name;
    match = 1;
    ndone = 1;
    while (ndone)
    {
        switch (*cpp)
        {
            case '*':
                // skip to . or end of pat
                while (*cpp && *cpp != '.')
                    cpp++;

                // skip to . or end of name
                while (*cpn && *cpn != '.')
                    cpn++;
                break;

            case '?':
                cpp++;
                cpn++;
                break;

            case 0:
                if (*cpn != 0)
                    match = 0;
                ndone = 0;
                break;

            default:
                if (tolower(*cpp) == tolower(*cpn))
                {
                    cpp++;
                    cpn++;
                }
                else
                {
                    match = 0;
                    ndone = 0;
                }
                break;
        }
    }

    return(match);
}

//*******************************************************************
// FmtEntry - format directory entry for report
//
// paramaters:
//             buf      - A buffer to hold formated directory entry.
//             name     - File name.
//             patternp - Path on which file was found.
//             time     - File time from directory.
//             date     - File data from directory.
//             szie     - File size from directory.
//
// returns:
//             A pointer to buf.
//
//*******************************************************************
char *FmtEntry(char *buf, char *name, char *patternp,
               unsigned time, unsigned date, long size)
{
    char   *cp;
    char    xname[10];
    char    xext[10];
    int     mo, dd, yy, hh, mi, ss;

    cp = strchr(name, '.');
    if (cp)
    {
        *cp = 0;
        strcpy(xname, name);
        strcpy(xext, cp + 1);
    }
    else
    {
        strcpy(xname, name);
        strcpy(xext, "");
    }

    mo = (date >> 5) & 0x0f;
    dd = date & 0x1f;
    yy = ((date >> 9) & 0x7f) + 80;
    hh = (time >> 11) & 0x1f;
    mi = (time >> 5) & 0x3f;
    ss = (time & 0x1f) * 2;

    sprintf(buf, "%-8s.%-3s %7ld %02d/%02d/%02d %02d:%02d:%02d %s",
                  xname,
                  xext,
                  size,
                  mo, dd, yy,
                  hh, mi, ss,
                  patternp);

    return(buf);
}

//*******************************************************************
