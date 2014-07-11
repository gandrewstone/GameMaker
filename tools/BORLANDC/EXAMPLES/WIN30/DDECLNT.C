// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//*******************************************************************
//
// program - DDEClnt.c
// purpose - dde client interface routines. This file is part of the
//           TSTAPP project.
//
//*******************************************************************

#define  STRICT
#include <windows.h>
#include <dde.h>
#include <string.h>
#include <time.h>

#include "ddeclnt.h"

# define WW_SERVER  0
# define ACK        0x8000
# define BUSY       0x4000

static int          nClientClassRegistered;
static char         szClientClass[] = "ddeclient";
static WORD         wLastDDEClientMsg;
static int          nAckFlg;
static int          nBusyFlg;
static HANDLE       hData;

//*******************************************************************
// DDEClientOpen - open a dde conversation with a server
//
// paramaters:
//             hParentWnd    - The handle of the window trying to start
//                             this dde conversation.
//             szApp         - The name of the application to talk to.
//             szTopic       - The name of the topic to talk about.
//
// returns:
//             The handle of the channel to use to continue the conversation
//                or
//             NULL if conversation could not be opened.
//
//*******************************************************************
HWND DDEClientOpen(HWND hParentWnd, char *szApp, char *szTopic)
{
    HINSTANCE hInst;
    HWND     hChannel;
    ATOM     aApp;
    ATOM     aTopic;
    WNDCLASS wclass;

    // Get parents instance to use in creating window to carry on
    // dde conversation.
    hInst = (HINSTANCE)GetWindowWord(hParentWnd, GWW_HINSTANCE);
    if (!hInst)
        return(0);

    // If we have not already done it, register the window class to
    // carry on this conversation.
    if (!nClientClassRegistered)
    {
        wclass.style         = CS_HREDRAW | CS_VREDRAW;
	wclass.lpfnWndProc   = DDEClientWndProc;
        wclass.cbClsExtra    = 0;
        wclass.cbWndExtra    = 2;
        wclass.hInstance     = hInst;
        wclass.hIcon         = 0;
        wclass.hCursor       = 0;
        wclass.hbrBackground = 0;
        wclass.lpszMenuName  = 0;
	wclass.lpszClassName = szClientClass;

        // Do not fail if register fails, because another app may have
        // already registered this class.
        RegisterClass(&wclass);

        // So we won't try to register again.
        nClientClassRegistered = 1;
    }

    // Create the window that will remain hidden but will carry on the
    // conversation wih the dde server.
    hChannel = CreateWindow(szClientClass,    // window class name
                            NULL,             // window title
                            WS_CHILD,         // window style
                            CW_USEDEFAULT,    // window origin
                            0,
                            CW_USEDEFAULT,    // window size
                            0,
                            hParentWnd,       // window parent
			    NULL,             // window id
                            hInst,            // window instance
                            NULL);            // no parms

    // If create of window failed we cannot continue
    if (!hChannel)
        return(0);

    // Create atoms representing application and topic we want to talk about
    aApp = GlobalAddAtom(szApp);
    if (!aApp)
        return(0);

    aTopic = GlobalAddAtom(szTopic);
    if (!aTopic)
    {
        GlobalDeleteAtom(aApp);
        return(0);
    }

    // Remember the last dde message we sent, because the window proc we
    // just created must know the last message sent by this application
    // to properly respond to a dde response.
    wLastDDEClientMsg = WM_DDE_INITIATE;

    // Broadcast a dde initiate to all apps on this system to see if any
    // will talk to us about the requested application and topic.
    SendMessage((HWND)0xFFFF, wLastDDEClientMsg, (WPARAM)hChannel, (LPARAM)MAKELONG(aApp, aTopic));

    // Get rid of the atoms because we don't need them any more.
    GlobalDeleteAtom(aApp);
    GlobalDeleteAtom(aTopic);

    // If no acknowledgement from any apps on the system to our initiate
    // message, no server will talk on the application and topic we requested.
    if (!nAckFlg)
    {
        DestroyWindow(hChannel);
        hChannel = 0;
    }

    nAckFlg = 0;

    return(hChannel);
}

//*******************************************************************
// DDEClientClose - close the dde conversation with a server
//
// paramaters:
//             hChannel      - The handle of the window conducting
//                             the dde conversation.
//
//*******************************************************************
void DDEClientClose(HWND hChannel)
{
    HWND hServerWnd;

    // If the channel is still active.
    if (IsWindow(hChannel))
    {
        wLastDDEClientMsg = WM_DDE_TERMINATE;

        // Get handle of server we were conversing with.
        hServerWnd = (HWND)GetWindowWord(hChannel, WW_SERVER);

        // If he is still active.
        if (IsWindow(hServerWnd))
            SendMessage(hServerWnd, wLastDDEClientMsg, (WPARAM)hChannel, (LPARAM)0L);
    }
}

//*******************************************************************
// DDEExecute - request an action from server
//
// paramaters:
//             hChannel      - The handle of the window conducting
//                             the dde conversation.
//             lpszCommand   - The command string to send to the
//                             dde server.
//             nWait         - The time to wait for a response to the
//                             command.  If zero, will return immediatly.
//
// returns:
//             1 - successful transmission.
//             0 - error.
//
//*******************************************************************
int DDEExecute(HWND hChannel, LPSTR lpszCommand, int nWait)
{
    int          len;
    HANDLE       hCommand;
    HWND         hServerWnd;
    LPSTR        lpCommand;
    int          nRc;

    nRc = 0;

    // Get the handle of server we were talking to.
    hServerWnd = (HWND)GetWindowWord(hChannel, WW_SERVER);

    // If he is no longer active we can't talk to him.
    if (!IsWindow(hServerWnd))
        return(nRc);

    // Get shared memory to put our request to server in.
    len = _fstrlen(lpszCommand);
    hCommand = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE,
                           (DWORD) (len + 1));
    if (hCommand)
    {
        // Lock it down to get address of memory.
        lpCommand = GlobalLock(hCommand);
        if (lpCommand)
        {
            // Copy the command into the shared memory.
            _fstrcpy(lpCommand, lpszCommand);
            GlobalUnlock(hCommand);

            // Post the message to the server.
            wLastDDEClientMsg = WM_DDE_EXECUTE;
            if (!PostMessage(hServerWnd, wLastDDEClientMsg, (WPARAM)hChannel,
                            (LPARAM)MAKELONG(0, hCommand)))
            {
                // If post failed.
                GlobalFree(hCommand);
            }
            else
            {
                nRc = 1;
		nAckFlg = 0;

                // Wait if caller told us to.
                if (nWait)
                    nRc = DDEWait(hChannel, nWait);
            }
        }
    }

    nAckFlg = 0;

    return(nRc);
}

//*******************************************************************
// DDERead - read some data from the server
//
// paramaters:
//             hChannel      - The handle of the window conducting
//                             the dde conversation.
//             szItem        - The item to get from server.
//
// returns:
//             Global Handle - of item requested if successful.
//               or
//             0 - if error.
//
//*******************************************************************
HANDLE DDERead(HWND hChannel, char *szItem, int nWait)
{
    HWND         hServerWnd;
    DDEDATA FAR *lpData;
    int          nReleaseFlg;
    HANDLE       hOurData;
    LPSTR        lpOurData;
    ATOM         aItem;
    int          len;

    hOurData = 0;

    // Get the handle of server we were talking to.
    hServerWnd = (HWND)GetWindowWord(hChannel, WW_SERVER);

    // If he is no longer active we can't talk to him.
    if (!IsWindow(hServerWnd))
        return(hOurData);

    // Create an atom representing the data item we want read.
    aItem = GlobalAddAtom(szItem);
    hData = 0;

    // Post the request to the server.
    wLastDDEClientMsg = WM_DDE_REQUEST;
    PostMessage(hServerWnd, wLastDDEClientMsg, (WPARAM)hChannel,
                (LPARAM)MAKELONG(CF_TEXT, aItem));

    // Wait for a dde message in response or time to expire.
    DDEWait(hChannel, nWait);

    // If we have a handle to returned data.
    if (hData)
    {
        // Lock it down to get address.
        lpData = (DDEDATA FAR *) GlobalLock(hData);
        if (lpData)
        {
            // Remember release flag to see if we release or server releases.
            nReleaseFlg = lpData->fRelease;

            // Get memory for our local copy of data.
            len = _fstrlen((char far *)lpData->Value);
            hOurData = GlobalAlloc(GMEM_MOVEABLE, (DWORD) (len + 1));
            if (hOurData)
            {
                // Lock it down to get address.
                lpOurData = GlobalLock(hOurData);
                if (lpOurData)
                {
                    // Copy it in.
                    _fstrcpy(lpOurData, (char far *)lpData->Value);
		    GlobalUnlock(hOurData);
                }
                else
                    hOurData = 0;
            }

            GlobalUnlock(hData);
        }

        // If server requested that we free shared memory.
        if (nReleaseFlg)
            GlobalFree(hData);
    }

    // Return handle to our copy of data.
    return(hOurData);
}

//*******************************************************************
// DDEWait - wait up to time for a dde messag
//
// paramaters:
//             hChannel      - The handle of the window conducting
//                             the dde conversation.
//             nWait         - Length of time to wait for a dde msesage
//                             in seconds.
//
// returns:
//             1 - dde message recieved while waiting.
//               or
//             0 - time expired and no dde messages recieved.
//
//*******************************************************************
int DDEWait(HWND hChannel, int nWait)
{
    time_t  t1;
    time_t  t2;
    int     nGotDDEMsg;
    MSG     msg;

    time(&t1);
    nGotDDEMsg = 0;

    do
    {
        if (PeekMessage(&msg, hChannel, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE))
        {
            DispatchMessage(&msg);
            nGotDDEMsg = 1;
        }
    }
    while (!nGotDDEMsg && ((time(&t2) - t1) < nWait));

    return(nGotDDEMsg);
}

//*******************************************************************
// DDESleep - wait for a time
//
// paramaters:
//             nWait         - Length of time to wait in seconds
//
//*******************************************************************
void DDESleep(int nWait)
{
    time_t  t1;
    time_t  t2;
    MSG     msg;

    time(&t1);

    do
    {
        PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
    }
    while ((time(&t2) - t1) < nWait);
}

//*******************************************************************
// DDEClientWndProc - window proc to manage interface with server
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
LRESULT CALLBACK DDEClientWndProc(HWND hWnd, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
    HWND   hServerWnd;
    WORD   wStatus;

    switch (message)
    {
        case WM_CREATE:              // create this window
	    // We will remember the handle of the server we are conversing
            // with in the first extra window word, but for now we do not
            // have a server.
            SetWindowWord(hWnd, WW_SERVER, 0);
            break;

        case WM_DDE_ACK:             // an ack from a server
            switch (wLastDDEClientMsg)
            {
                case WM_DDE_INITIATE:  // ack in response to initiate
                    // Get rid of application and topic atoms used in
                    // initiate request.
                    GlobalDeleteAtom(LOWORD(lParam));
                    GlobalDeleteAtom(HIWORD(lParam));

                    // Now we have a server for this conversation, so remember
                    // who it is.
                    SetWindowWord(hWnd, WW_SERVER, wParam);
                    nAckFlg = 1;
                    break;

                case WM_DDE_EXECUTE:   // ack in response to execute
                    // Server acknowledged our execute command.
                    wStatus = LOWORD(lParam);
                    nAckFlg = wStatus & ACK;
                    if (!nAckFlg)
                        nBusyFlg = wStatus & BUSY;
                    GlobalFree((HGLOBAL)HIWORD(lParam));
                    break;

                case WM_DDE_REQUEST:   // ack in response to request
                    // Get rid of item atom used in request.
                    GlobalDeleteAtom(HIWORD(lParam));
                    wStatus = LOWORD(lParam);
                    nAckFlg = wStatus & ACK;
                    if (!nAckFlg)
                        nBusyFlg = wStatus & BUSY;
                    break;
            }
            break;

        case WM_DDE_DATA:            // data from a server per request
            // Get rid of item atom used in request.
            GlobalDeleteAtom(HIWORD(lParam));

            // Save handle to global data item returned.
            hData = (HANDLE)LOWORD(lParam);
            break;

        case WM_DDE_TERMINATE:      // server says terminate conversation
            // Get handle of server we are dealing with.
            hServerWnd = (HWND)GetWindowWord(hWnd, WW_SERVER);

            // Ignore if this terminate is not from that server.
            if (wParam != (WPARAM)hServerWnd)
                return(0L);

            // Forget who server is.
            SetWindowWord(hWnd, WW_SERVER, 0);

	    // Tell our parent conversation with server is closed.
            SendMessage(GetParent(hWnd), WMU_DDE_CLOSED, (WPARAM)hWnd, (LPARAM)0L);

            // Send terminate to server to acknowledge we recieved
            // terminate,  if we did not initiate the termination ourselves.
            if ((wLastDDEClientMsg != WM_DDE_TERMINATE) && IsWindow(hServerWnd))
                PostMessage(hServerWnd, WM_DDE_TERMINATE, (WPARAM)hWnd, (LPARAM)0L);

            // Terminate this child window.
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:            // destroy this window
            // Terminate any ongoing conversation on our way out.
            DDEClientClose(hWnd);
            break;

        default:
            return(DefWindowProc(hWnd, message, wParam, lParam));
    }

    return(0L);
}

//*******************************************************************

