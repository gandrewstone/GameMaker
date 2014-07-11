// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  BWINDOW.CPP
  Defines type TBWindow.
  -------------------------------------------------------- */

#include "applicat.h"
#include "bwindow.h"

/* Constructor for a TBWindow object. */
TBWindow::TBWindow(PTWindowsObject AParent, LPSTR ATitle,
		   PTModule AModule)
     : TWindow(AParent, ATitle, AModule)
{
}

void TBWindow::GetWindowClass(WNDCLASS& AWndClass)
{
      TWindow::GetWindowClass(AWndClass);
      AWndClass.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
}

PTStreamable TBWindow::build()
{
  return new TBWindow(streamableInit);
}

TStreamableClass RegBWindow("TBWindow", TBWindow::build,
			    __DELTA(TBWindow));
