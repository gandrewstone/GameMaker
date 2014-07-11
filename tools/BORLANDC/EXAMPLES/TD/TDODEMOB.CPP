/***********************************************************************
 *  Copyright (c) 1992 - Borland International.
 *
 *  File: TDODEMO.CPP
 *
 *  Buggy version of TDODEMO.CPP that shows how to use TDW to
 *  debug an Object Windows application.
 *
 *  The Color Scribble program lets the user draw on the screen in
 *  any of four colors: red, green, blue, and black. This version
 *  contains several bugs that you can use TDW to find and Turbo C++
 *  for Windows to correct. For more information, see the Turbo Debugger
 *  User's Guide and read the section on debugging an Object Windows
 *  application.
 ***********************************************************************/

#define STRICT

#include <stdio.h>
#include <stdlib.h>
#include <owl.h>
#include "tdodemob.h"

// CScribbleApplication Declarations

class CScribbleApplication : public TApplication
{
public:
  CScribbleApplication(LPSTR AName, HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
    : TApplication(AName, hInstance, hPrevInstance, lpCmdLine, nCmdShow) {};
  virtual void InitMainWindow();
};

// ScribbleWindow Declarations

class ScribbleWindow : public TWindow
{
public:
  HDC HandleDC;              // Display context for drawing.
  BOOL ButtonDown;           // left-button-down flag
  HPEN ThePen;               // Pen that is used for drawing in color
  ScribbleWindow(PTWindowsObject AParent, LPSTR ATitle);
  ~ScribbleWindow();
  void GetWindowClass(WNDCLASS &AWndClass);


  // Dynamic virtual member function that gets called when the left mouse
  // button is clicked in the window.  This method sets up the window for
  // scribbling by creating a display context.

  virtual void WMLButtonDown(RTMessage Msg)  = [WM_FIRST + WM_LBUTTONDOWN];


  // Dynamic virtual member function that gets called when the left mouse
  // button is released in the window.  This method releases the display 
  // context that is used for drawing.

  virtual void WMLButtonUp(RTMessage Msg)    = [WM_FIRST + WM_LBUTTONUP];


  // Dynamic virtual member function that gets called when the mouse is
  // moved anywhere in the window. If the left mouse button is pressed,
  // the window will be scribbled in.

  virtual void WMMouseMove(RTMessage Msg)    = [WM_FIRST + WM_MOUSEMOVE];


  // Dynamic virtual member function that gets called when the right
  // mouse button is clicked in the window.  It clears the window by
  // invalidating the window, causing a WM_PAINT message to be sent.

  virtual void WMRButtonDown(RTMessage Msg)  = [WM_FIRST + WM_RBUTTONDOWN];


  // Dynamic virtual member function that gets called when user selects
  // Pen.Red from the menu bar. Disposes of the current pen and creates
  // a red pen.

  virtual void SelectRedPen(RTMessage Msg)   = [CM_FIRST + CM_RED];


  // Dynamic virtual member function that gets called when user selects
  // Pen.Green from the menu bar. Disposes of the current pen and creates
  // a green pen.

  virtual void SelectGreenPen(RTMessage Msg) = [CM_FIRST + CM_GREEN];


  // Dynamic virtual member function that gets called when user selects
  // Pen.Blue from the menu bar. Disposes of the current pen and creates
  // a blue pen.

  virtual void SelectBluePen(RTMessage Msg)  = [CM_FIRST + CM_BLUE];


  // Dynamic virtual member function that gets called when user selects
  // Pen.Black from the menu bar. Disposes of the current pen and creates
  // a black pen.

  virtual void SelectBlackPen(RTMessage Msg) = [CM_FIRST + CM_BLACK];


 // Redeclares the ObjectWindows function SetupWindow so you
 // can set a breakpoint and obtain the window handle.

  virtual void SetupWindow();
};

ScribbleWindow::ScribbleWindow(PTWindowsObject AParent, LPSTR ATitle)
               : TWindow(AParent, ATitle)
{
  AssignMenu((LPSTR)MAKEINTRESOURCE(MenuID));
                         // Attach menu from resource file to window.

  ThePen = CreatePen(PS_SOLID, PenWidth, 0);
                         // Initialize pen to black.
  ButtonDown = FALSE;
}

ScribbleWindow::~ScribbleWindow()
{
  DeleteObject(ThePen);  // Dispose of pen
}

/**************************************************** 
 * member function ScribbleWindow::GetWindowClass
 *
 * Changes the window icon to a custom icon
 ****************************************************/

void ScribbleWindow::GetWindowClass(WNDCLASS &AWndClass)
{
  TWindow::GetWindowClass(AWndClass);
  AWndClass.hIcon = LoadIcon(GetApplication()->hInstance, 
                    MAKEINTRESOURCE(IconID));
}

/********************************************************************** 
 * member function ScribbleWindow::WMLButtonDown
 *
 * Process WM_LBUTTONDOWN messages by marking mouse as being pressed,
 * telling Windows to send all mouse messages to window, and selecting
 * a colored pen into the display context.
 **********************************************************************/

void ScribbleWindow::WMLButtonDown(RTMessage Msg)
{
  if ( !ButtonDown )
  {
    ButtonDown = TRUE;           // Mark mouse button as being pressed so
                                 // when mouse movement occurs, a line
                                 // will be drawn.

    MoveTo(HandleDC, Msg.LP.Lo, Msg.LP.Hi);
                                 // Move drawing point to location
                                 // where mouse was pressed.

    SelectObject(HandleDC, ThePen);
                                 // Select pen into display context.
  }
}

/*********************************************************** 
 * member function ScribbleWindow::WMLButtonUp
 *
 * Process WM_LBUTTONUP messages by releasing the display
 * context and marking the mouse button as not being pressed.
 ************************************************************/

void ScribbleWindow::WMLButtonUp(RTMessage)
{
  if ( ButtonDown )
  {
    ReleaseDC(HWindow, HandleDC);
    ButtonDown = FALSE;
  }
}

/*********************************************************************
 * member function ScribbleWindow::WMRButtonDown
 *
 * Process WM_RBUTTONDOWN messages by erasing the window.
 *********************************************************************/

void ScribbleWindow::WMRButtonDown(RTMessage)
{
  UpdateWindow(HWindow);
}

/********************************************************
 * member function ScribbleWindow::WM_Mousemove
 *
 * Process WM_MOUSEMOVE messages by drawing a line if the
 * mouse button is marked as being pressed.
 ********************************************************/

void ScribbleWindow::WMMouseMove(RTMessage Msg)
{
  if ( ButtonDown )
    LineTo(HandleDC, Msg.LP.Lo, Msg.LP.Hi);
                                   // Draw a line to where the mouse is presently
}

/******************************************************************
 * member function ScribbleWindow::SelectRedPen
 *
 * Create a red pen in response to a "Red" selection from Pen menu.
 ******************************************************************/
#pragma argsused
void ScribbleWindow::SelectRedPen(RTMessage Msg)
{
  DeleteObject(ThePen);
  ThePen = CreatePen(PS_SOLID, PenWidth, RGB(255, 0, 0));
}

/********************************************************************** 
 * member function ScribbleWindow::SelectGreenPen
 *
 * Create a green pen in response to a "Green" selection from Pen menu.
 **********************************************************************/
#pragma argsused
void ScribbleWindow::SelectGreenPen(RTMessage Msg)
{
  DeleteObject(ThePen);
  ThePen = CreatePen(PS_SOLID, PenWidth, RGB(0, 255, 0));
}

/******************************************************************** 
 * member function ScribbleWindow::SelectBluePen
 *
 * Create a blue pen in response to a "Blue" selection from Pen menu.
 ********************************************************************/
#pragma argsused
void ScribbleWindow::SelectBluePen(RTMessage Msg)
{
  DeleteObject(ThePen);
  ThePen = CreatePen(PS_SOLID, PenWidth, RGB(0, 0, 255));
}

/**********************************************************************
 * member function ScribbleWindow::SelectBlackPen
 *
 * Create a black pen in response to a "Black" selection from Pen menu.
 **********************************************************************/
#pragma argsused
void ScribbleWindow::SelectBlackPen(RTMessage Msg)
{
  DeleteObject(ThePen);
  ThePen = CreatePen(PS_SOLID, PenWidth, RGB(0, 0, 0));
}

/*************************************************************
 * member function ScribbleWindow::SetupWindow
 *
 * Redeclare the ObjectWindows function SetupWindow so you
 * can set a breakpoint on it and obtain the window handle.
 *************************************************************/

void ScribbleWindow::SetupWindow()
{
    TWindow::SetupWindow();
}


/*************************************************************
 * member function CScribbleApplication::InitMainWindow
 *
 * Initialize a Color Scribble window for the main window.
 *************************************************************/

void CScribbleApplication::InitMainWindow()
{
  MainWindow = new ScribbleWindow(NULL, Name);
}


int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPSTR lpCmdLine, int nCmdShow)
{
  CScribbleApplication MyApp("Color Scribble", hInstance, hPrevInstance,
               lpCmdLine, nCmdShow);
  MyApp.Run();
  return MyApp.Status;
}
