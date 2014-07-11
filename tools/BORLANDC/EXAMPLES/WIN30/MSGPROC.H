// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// msgproc.h

// message defines
//#define IDM_MESSAGE   199

// function prototypes
extern void   SendMsgWndMsg(HWND hWnd, char *command, char *msg);
extern HANDLE GetLines(HWND hWnd);
extern void   CloseMsgWnd(void);
extern int    GetMsgLink(HWND hWnd);

// end msgproc.h

