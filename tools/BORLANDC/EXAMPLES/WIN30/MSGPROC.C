// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//*******************************************************************
//
// program - MsgProc.c
// purpose - A windows program to send messages to MsgWnd application.
//           This file is part of the TSTAPP project.
//
//*******************************************************************

#define  STRICT
#include <windows.h>
#include <dde.h>
#include <string.h>

# include "msgproc.h"
# include "ddeclnt.h"

static HWND hChannel;
static HWND hMsgWnd;

//*******************************************************************
// SendMsgWndMsg - send a message to the MsgWnd application
//
// paramaters:
//             hWnd          - The window handle of caller
//             command       - The command to send to msgwnd application
//             msg           - The message to send to msgwnd application
//
//*******************************************************************
void SendMsgWndMsg(HWND hWnd, char *command, char *msg)
{
    HANDLE      hmsg;
    LPSTR       lpmsg;

    // If the msgwnd application is running and we have a conversation
    // channel established with it, lets send our text message to it.
    if (GetMsgLink(hWnd))
    {
        // Allocate some shared memory to put message into.
        hmsg = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                           (DWORD) (strlen(command) + strlen(msg) + 8));
        if (hmsg)
        {
            // Lock it down to get addres of memory.
            lpmsg = GlobalLock(hmsg);
            if (lpmsg)
            {
                // Build print command to send to msgwnd application.
                _fstrcpy(lpmsg, "[");
                _fstrcat(lpmsg, command);
                if (msg)
                {
                    _fstrcat(lpmsg, " (\"");
		    _fstrcat(lpmsg, msg);
                    _fstrcat(lpmsg, "\")");
                }
                _fstrcat(lpmsg, "]");

                // Send the command.
                DDEExecute(hChannel, lpmsg, 0);

                GlobalUnlock(hmsg);
            }

            GlobalFree(hmsg);
        }
    }
}

//*******************************************************************
// GetLines - get number of lines printed by msgwnd
//
// paramaters:
//             hWnd          - The window handle of caller
//
// returns:
//          handle to 'lines' data item.
//            or
//          NULL if it was not returned.
//
//*******************************************************************
HANDLE GetLines(HWND hWnd)
{
    HANDLE hTmp;

    hTmp = 0;

    // If the msgwnd application is running and we have a conversation
    // channel established with it, lets send our text message to it.
    if (GetMsgLink(hWnd))
    {
        // Go ask for the 'lines' data item from msgwnd application.
        hTmp = DDERead(hChannel, "lines", 5);
    }

    // Return handle to 'lines' data item or NULL if it was not returned.
    return(hTmp);
}

//*******************************************************************
// CloseMsgWnd - close dde message link to the MsgWnd application
//
//*******************************************************************
void CloseMsgWnd()
{
    // Tell msgwnd application we want to terminate our conversation.
    DDEClientClose(hChannel);
    hChannel = 0;
}

//*******************************************************************
// GetMsgLink - establish link to msgwnd application
//
//*******************************************************************
int GetMsgLink(HWND hWnd)
{
    char msgbuf[80];

    // See if msgwnd application is already running.
    hMsgWnd = FindWindow("msgwnd", NULL);
    if (!hMsgWnd)
    {
	// If not, try to activate it.
        strcpy(msgbuf, "MSGWND.EXE");
        WinExec(msgbuf, SW_SHOWNORMAL);
	hMsgWnd = FindWindow("msgwnd", NULL);
        hChannel = 0;
    }

    // Have we started a conversation with the msgwnd application yet?
    if (!hChannel)
    {
        // No, try to start one.
        hChannel = DDEClientOpen(hWnd, "msgwnd", "screen");
    }

    return((int) hChannel);
}

//*******************************************************************

