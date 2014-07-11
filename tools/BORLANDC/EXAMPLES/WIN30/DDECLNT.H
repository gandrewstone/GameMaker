// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// ddeclnt.h

# define WMU_DDE_CLOSED WM_USER + 100

extern HWND             DDEClientOpen(HWND hParentWnd, char *szApp,
                                      char *szTopic);
extern void             DDEClientClose(HWND hChannel);
extern int              DDEExecute(HWND hChannel, LPSTR lpszCommand, int nWait);
extern HANDLE           DDERead(HWND hChannel, char *szItem, int nWait);
extern int              DDEWait(HWND hChannel, int nWait);
extern void             DDESleep(int nWait);
extern LRESULT CALLBACK DDEClientWndProc(HWND hWnd, UINT message,
					WPARAM wParam, LPARAM lParam);

// end ddeclnt.h

