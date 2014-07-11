// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//*******************************************************************
//
// program - DDESrvr.c
// purpose - DDE server interface routines. This file is part of the
//           MSGWND project.
//
//*******************************************************************

#define  STRICT
#include <windows.h>
#include <dde.h>
#include <stdio.h>
#include <string.h>

# include "ddesrvr.h"

# define WW_CLIENT 0
# define ACK       0x8000
# define BUSY      0x4000

static int  nServerClassRegistered;
static char szServerClass[] = "ddeserver";
static WORD wLastDDEServerMsg;

//*******************************************************************
// DDEServerInit - called in response to WM_DDE_INITIATE
//
// paramaters:
//             hParentWnd    - The handle of the server window trying to
//                             respond to this dde conversation.
//             hClientWnd    - The handle of the client window trying to start
//                             this dde conversation.
//             lParam        - lParam send by client window.
//                             Contains atoms of application and topic client
//                             wants to talk about.
//             szApp         - The name of the application server represents.
//             szTopic       - The name of the topic server knows about.
//
// returns:
//             The handle of the channel to use to continue the conversation
//                or
//             NULL if conversation could not be opened.
//
//*******************************************************************
HWND DDEServerInit(HWND hParentWnd, HWND hClientWnd, LPARAM lParam,
		  char *szApp, char *szTopic)
{
    HINSTANCE hInst;
    HWND     hChannel;
    ATOM     aApp;
    ATOM     aTopic;
    WNDCLASS wclass;
    char     szClassName[sizeof(szServerClass) + 10];

    // Get atoms representing application and topic we will talk about.
    aApp = GlobalAddAtom(szApp);
    if (!aApp)
        return(0);

    aTopic = GlobalAddAtom(szTopic);
    if (!aTopic)
    {
        GlobalDeleteAtom(aApp);
        return(0);
    }

    // If request is not for application and topic we will talk about
    // do not open conversaton.
    if ((aApp != LOWORD(lParam)) || (aTopic != HIWORD(lParam)))
    {
        GlobalDeleteAtom(aApp);
        GlobalDeleteAtom(aTopic);
        return(0);
    }

    // Get parents instance to use in creating window to carry on
    // dde conversation.
    hInst = (HANDLE)GetWindowWord(hParentWnd, GWW_HINSTANCE);
    if (!hInst)
        return(0);

    // Create a unique class name for this instance of the server.
    sprintf(szClassName, "%s%04X", szServerClass, hInst);

    // If we have not already done it, register the window class to
    // carry on this conversation.
    if (!nServerClassRegistered)
    {
        wclass.style         = CS_HREDRAW | CS_VREDRAW;
        wclass.lpfnWndProc   = DDEServerWndProc;
        wclass.cbClsExtra    = 0;
        wclass.cbWndExtra    = 2;
        wclass.hInstance     = hInst;
        wclass.hIcon         = 0;
        wclass.hCursor       = 0;
        wclass.hbrBackground = 0;
        wclass.lpszMenuName  = 0;
        wclass.lpszClassName = szClassName;

        // Do not fail if register fails, because another app may have
        // already registered this class.
        RegisterClass(&wclass);

	// So we won't try to register again.
        nServerClassRegistered = 1;
    }

    // Create the window that will remain hidden but will carry on the
    // conversation wih the dde client.
    hChannel = CreateWindow(szClassName,      // window class name
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

    // If create of window failed we cannot continue.
    if (!hChannel)
        return(0);

    // Remember the handle of our client.
    SetWindowWord(hChannel, WW_CLIENT, (WORD)hClientWnd);

    // Send acknowledgement to client.
    wLastDDEServerMsg = WM_DDE_ACK;
    SendMessage(hClientWnd, (UINT)wLastDDEServerMsg, (WPARAM)hChannel, (LPARAM)MAKELONG(aApp, aTopic));

    // Return handle of window that will carry on the conversation for
    // this server.
    return(hChannel);
}

//*******************************************************************
// DDEServerClose - close the dde conversation with a client
//
// paramaters:
//             hChannel      - The handle of the window conducting
//                             the dde conversation.
//
//*******************************************************************
int DDEServerClose(HWND hChannel)
{
    HWND hClientWnd;

    // If the channel is still active.
    if (IsWindow(hChannel))
    {
        wLastDDEServerMsg = WM_DDE_TERMINATE;

        // Get handle of client we were conversing with.
        hClientWnd = (HWND)GetWindowWord(hChannel, WW_CLIENT);

        // If he is still active.
        if (IsWindow(hClientWnd))
            SendMessage(hClientWnd, (UINT)wLastDDEServerMsg, (WPARAM)hChannel, (LPARAM)0L);
    }

    return(1);
}

//*******************************************************************
// DDEServerWndProc - window proc to manage interface with client
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
LRESULT CALLBACK DDEServerWndProc(HWND hWnd, UINT message,
				 WPARAM wParam, LPARAM lParam)
{
    WORD         wStatus;
    HWND         hClientWnd;
    HANDLE       hCommand;
    HANDLE       hData;
    LPSTR        lpData;
    HANDLE       hShData;
    DDEDATA FAR *lpShData;
    char         szItem[30];
    ATOM         aItem;
    LPSTR        lpCommand;
    int          nRc;
    int          nSize;

    switch (message)
    {
        case WM_DDE_EXECUTE:         // execute a command for client
            // Get handle of client.
	    hClientWnd = (HWND)wParam;

            // Init status.
            wStatus = 0;

            // Get handle of command client is sending us.
            hCommand = (HANDLE)HIWORD(lParam);

            // Lock it down to get address.
            lpCommand = GlobalLock(hCommand);
            if (lpCommand)
            {
                // Call our function to execute the command.
                if (DDEExecuteCommand(lpCommand))
                    wStatus = ACK;

                GlobalUnlock(hCommand);
            }

	    // Send acknowledgement back to client.
            PostMessage(hClientWnd, WM_DDE_ACK, (WPARAM)hWnd,
                       (LPARAM)MAKELONG(wStatus, hCommand));
            break;

        case WM_DDE_REQUEST:         // data request from a client
            // Get handle of client.
            hClientWnd = (HWND)wParam;

            // Get atom representing data request item.
            aItem = HIWORD(lParam);
            if (!GlobalGetAtomName(aItem, szItem, sizeof(szItem)))
            {
                // Error if atom not found.
                PostMessage(hClientWnd, WM_DDE_ACK, (WPARAM)hWnd,
                           (LPARAM)MAKELONG(0, aItem));
                return(0L);
            }

	    // Init return code.
            nRc = 0;

            // Don't care about case.
            strupr(szItem);

            // Call our function to get the data item.
            nSize = DDEData(szItem, &hData);

            // If any data found.
            if (nSize)
            {
                // Get shared memory to send item back in.
                hShData = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE,
                                      (DWORD) (nSize + sizeof(DDEDATA) + 1));
                if (hShData)
                {
                    // Lock it down to get address.
                    lpShData = (DDEDATA FAR *) GlobalLock(hShData);
		    if (lpShData)
                    {
                        // Setup data header.
                        lpShData->fResponse = 1;
                        lpShData->fRelease = 1;
                        lpShData->fAckReq = 0;
                        lpShData->cfFormat = CF_TEXT;

                        // Lock out local copy of data to get address.
                        lpData = GlobalLock(hData);
                        if (lpData)
                        {
                            // Copy data to shared memory.
                            _fstrcpy((LPSTR)lpShData->Value, lpData);
                            GlobalUnlock(hData);

                            nRc = 1;
                        }

			GlobalUnlock(hShData);
                    }
                }

                GlobalFree(hData);
            }

            if (nRc)
            {
                // If ok send data back.
                PostMessage(hClientWnd, WM_DDE_DATA, (WPARAM)hWnd,
                           (LPARAM)MAKELONG(hShData, aItem));
            }
            else
            {
                // If errors just send negative acknowlegement back.
                PostMessage(hClientWnd, WM_DDE_ACK, (WPARAM)hWnd,
                           (LPARAM)MAKELONG(0, aItem));
            }
	    break;

        case WM_DDE_TERMINATE:      // server says terminate conversation
            // Get handle of client we are dealing with.
            hClientWnd = (HWND)GetWindowWord(hWnd, WW_CLIENT);

            // Ignore if not from our client.
            if (wParam != (WPARAM)hClientWnd)
                return(0L);

            // Forget who client is.
            SetWindowWord(hWnd, WW_CLIENT, 0);

            // Tell our parent conversation closed.
            SendMessage(GetParent(hWnd), WMU_DDE_CLOSED, (WPARAM)hWnd, 0L);

            // Send terminate to server to acknowledge we recieved
            // terminate,  if we did not initiate the termination ourselves.
            if ((wLastDDEServerMsg != WM_DDE_TERMINATE) && IsWindow(hClientWnd))
		PostMessage(hClientWnd, WM_DDE_TERMINATE, (WPARAM)hWnd, 0L);

            // Terminate this child window.
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:            // destroy this window
            // Terminate any ongoing conversation on our way out.
            DDEServerClose(hWnd);
            break;

        default:
            return(DefWindowProc(hWnd, message, wParam, lParam));
    }

    return(0L);
}

//*******************************************************************
