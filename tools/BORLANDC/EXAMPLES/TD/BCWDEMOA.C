/********************************************************************
 *       Copyright (c) 1992 by Borland International, Inc.          *
 *                                                                  *
 *                           BCWDEMOA.C                             *
 *                                                                  *
 *  This program is one of three demo programs provided with Turbo  *
 *  Debugger for Windows. It contains two bugs that you can find    *
 *  using TDW and the chapter in the TD User's Guide that explains  *
 *  how to debug an MS-Windows program. The original, bug-free ver- *
 *  sion, BCWDEMO.C, demonstrates how to write a simple Windows     *
 *  graphics program in Borland C++.  It's called Simple Paint and  *
 *  provides three objects the user can draw, a line, an ellipse,   *
 *  and a rectangle.  Simple Paint also provides the user with      *
 *  three choices of line thicknesses and three choices of colors,  *
 *  red, green, and black.                                          *
 ********************************************************************/

#define STRICT

#include <windows.h>
#include "bcwdemo.h"

/*****************************************************
 * FUNCTION PROTOTYPES
 *****************************************************/

/*
 * DoWMCommand processes WM_COMMAND messages.  WM_COMMAND
 * is generated when the user selects something from the menu.
 */
int DoWMCommand(WORD wParam, HWND hWnd);

/*
 * DoLButtonDown process WM_LBUTTONDOWN messages.  WM_LBUTTONDOWN
 * is generated when the user presses the left mouse button.
 */
void DoLButtonDown(HWND hWnd,LONG lParam);

/*
 * DoLButtonUp processes WM_LBUTTONUP messages.  WM_LBUTTONUP
 * is generated when the user releases the left mouse button.
 */
void DoLButtonUp(HWND hWnd,LONG lParam);

/*
 * DoMouseMove processes WM_MOUSEMOVE messages.  WM_MOUSEMOVE
 * is generated when the user moves the mouse.
 */
void DoMouseMove(HWND hWnd,LONG lParam);

/*
 * DoPaint processes WM_PAINT messages.  WM_PAINT is generated
 * whenever UpdateWindow is called or another window is moved,
 * revealing a portion of the window receiving this message.
 */
void DoPaint(HWND hWnd);

/*
 * WndProc is the callback function (window proc)
 * for the Simple Paint class of windows.
 */
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM) ;

/*
 * DrawShape draws a shape into a device context
 * at a given location.
 */
void DrawShape(HDC hdc,int x,int y,int x2, int y2,int Shape,
	       int PenWidth,COLORREF PenColor,int Slope);

/***************************************************************
 * Global Variables
 ***************************************************************/

int CurrentShape = LINE;	   /* Maintains the status of the shape
				    * the user is drawing. Default is
				    * to draw with a line.             */

int PenWidth = 3;		   /* Maintains the current pen width.
				    * Default width is medium.         */

COLORREF PenColor = RGB(255,0,0);  /* Maintains the current pen color.
				    * Default color is red.            */


/*
 * Structure definition to track
 * what shapes have been drawn.
 */
struct SSHAPE {
	RECT     Points;	   /* Location of the shape.           */
	int	 PenWidth,	   /* Pen width for the shape.         */
		 theShape;	   /* Shape this structure represents. */
	COLORREF PenColor;	   /* Color of the shape.              */
	int	 Slope;		   /* Used to determine direction lines
				    * should be drawn. If slope > 0 then
				    * draw from UpperLeft to LowerRight.
				    * Else draw from LowerLeft
				    * to UpperRight.		       */
};

typedef struct SSHAPE SHAPE;

#define nPoints 100
SHAPE thisShape[nPoints];	   /* Array that stores the shapes
				    * the user draws.		      */
int CurrentPoint = -1;		   /* Indicates the number of shapes
				    * the user has drawn.             */

/**********************************************************
 * function WinMain
 *    Standard WinMain function.
 **********************************************************/
#pragma argsused
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   LPSTR lpszCmdLine, int nCmdShow )
{
    WNDCLASS	wndClass;
    MSG		msg;
    HWND	hWnd;

    /*
     * Register window class style if first
     * instance of this program.
     */
    if ( !hPrevInstance )
    {
	wndClass.style		= CS_HREDRAW | CS_VREDRAW ;
	wndClass.lpfnWndProc	= WndProc;
	wndClass.cbClsExtra 	= 0;
	wndClass.cbWndExtra	= 0;
	wndClass.hInstance 	= hInstance;
	wndClass.hIcon	 	= LoadIcon(hInstance, "IDI_SIMPLEPAINT");
	wndClass.hCursor	= LoadCursor(NULL, IDC_ARROW );
	wndClass.hbrBackground	= GetStockObject(WHITE_BRUSH );
	wndClass.lpszMenuName	= szAppName;
	wndClass.lpszClassName	= szAppName;

    if (!RegisterClass(&wndClass))
	return FALSE;
    }

    /*
     * Create and display the window.
     */
    hWnd = CreateWindow(szAppName,"Simple Paint",
			WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,0,
			CW_USEDEFAULT,0,NULL,NULL,hInstance,NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
	TranslateMessage(&msg );
	DispatchMessage(&msg );
    }
    return 0;
}

/*******************************************************
 * function WndProc
 *    Handles all messages received by the window
 *******************************************************/

LRESULT CALLBACK WndProc (HWND hWnd, UINT Message,
			 WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
	case WM_COMMAND:
	    return DoWMCommand(wParam, hWnd);

	case WM_LBUTTONDOWN:
	    DoLButtonDown(hWnd,lParam);
	    break;

	case WM_LBUTTONUP:
	    DoLButtonUp(hWnd,lParam);
	    break;

	case WM_MOUSEMOVE:
	    DoMouseMove(hWnd,lParam);
	    break;

	case WM_PAINT:
	    DoPaint(hWnd);
	    break;

	case WM_DESTROY:
	    PostQuitMessage(0);
	    break;

	default:
	    return DefWindowProc(hWnd,Message,wParam,lParam);
    }
    return 0;
}

/*****************************************************************
 * function DrawShape
 *
 *    Draws the shape given by Shape parameter using PenWidth
 *    and PenColor in the rectangle bounded by x,y,x2,y2.  The
 *    Slope parameter is used with line shapes to determine if
 *    lines should be drawn with a negative or positive slope.
 *****************************************************************/

void DrawShape(HDC hdc, int x, int y, int x2, int y2, int Shape,
	       int PenWidth, COLORREF PenColor, int Slope)
{
    HANDLE	saveObject;

    /*
     * Create the proper pen for this shape.  Save the
     * previously selected object from this DC.
     */
    saveObject = SelectObject(hdc, CreatePen(PS_SOLID,
					     PenWidth,
					     PenColor));
    switch(Shape)
    {
	case LINE:
	    /*
	     *  Rectangles that save a shape's position must be stored
	     *  as upper-left and lower-right.  To draw a line from
	     *  upper-right to lower-left, the line must have a
	     *  negative slope.
	     */
	    if (Slope > 0)
	    {
		MoveTo(hdc,x,y);
		LineTo(hdc,x2,y2);
	    }
	    else
	    {
		MoveTo(hdc,x,y2);
		LineTo(hdc,x2,y);
	    }
	    break;

	case ELLIPSE:
	    Ellipse(hdc,x,y,x2,y2);
	    break;

	case RECTANGLE:
	    Rectangle(hdc,x,y,x2,y2);
	    break;
    }
    /*
     *  Delete the currently selected object (the pen) and select
     *	whatever object was currently selected when we entered
     *	this routine.
     */
    SelectObject(hdc,saveObject);
}

/****************************************************************
 * function DoPaint
 *    Processes WM_PAINT messages.  WM_PAINT is generated
 *    whenever UpdateWindow is called or another window is moved,
 *    revealing a portion of the window receiving this message.
 ****************************************************************/

void DoPaint(HWND hWnd)
{
    int         i;

    HDC		hdc,
		hMemDC;

    RECT	theRect,
		destRect;

    HBITMAP	theBitmap;
    PAINTSTRUCT	ps;

    if (CurrentPoint >= 0)
    {
	hdc = BeginPaint(hWnd,&ps);
	/*
	 * Determine which rectangle on the window is invalid.
	 * If no rectangle is marked invalid, it will be a full
	 * window repaint.
	 */
	GetUpdateRect(hWnd,&theRect,0);
	if (IsRectEmpty(&theRect))
	    GetClientRect(hWnd,&theRect);

	/*
	 * Create a memory DC and bitmap the same
	 * size as the update rectangle.
	 */
	hMemDC = CreateCompatibleDC(hdc);
	theBitmap = CreateCompatibleBitmap(hdc,
					   theRect.right-theRect.left,
					   theRect.bottom-theRect.top);
	SelectObject(hMemDC,theBitmap);

	/*
	 * Erase the memBitmap.
	 */
	BitBlt(hMemDC, 0, 0,
	       theRect.right-theRect.left,
	       theRect.bottom-theRect.top,
	       hdc, 0, 0, SRCCOPY);

	/*
	 * Draw only those shapes that lie
	 * within the update rectangle.
	 */
	for (i = 0; i <= CurrentPoint; ++i)
	{
	    IntersectRect(&destRect, &thisShape[i].Points, &theRect);
	    if (!IsRectEmpty(&destRect))
		DrawShape(hMemDC,
			  thisShape[i].Points.left-theRect.left,
			  thisShape[i].Points.top-theRect.top,
			  thisShape[i].Points.right-theRect.left,
			  thisShape[i].Points.bottom-theRect.top,
			  thisShape[i].theShape,thisShape[i].PenWidth,
			  thisShape[i].PenColor,thisShape[i].Slope);
		/*
		 * Note that when drawing the shape, the shape's
		 * position was transformed so that the origin was
		 * at the upper-left corner of the update rectangle.
		 * This is the point (0,0) on the bitmap that will
		 * map onto (theRect.left,theRect.right).
		 */
	}

	/*
	 * Finally, copy the bitmap onto the update rectangle.
	 */
	BitBlt(hdc, theRect.left, theRect.top,
	       theRect.right-theRect.left,
	       theRect.bottom-theRect.top,
	       hMemDC, 0, 0, SRCCOPY);

	DeleteDC(hMemDC);
	DeleteObject(theBitmap);
	EndPaint(hWnd,&ps);
     }

}


/**********************************************************
 * static variables oldx, oldy, mouseDown
 *    Used to maintain both the state of the mouse position
 *    and the button status between mouse messages.
 **********************************************************/

static	oldx = -1, oldy = -1, mouseDown = 0;

/******************************************************************
 * function DoLButtonDown
 *    When the left button on the mouse is pressed, this routine
 *    saves the origin of this shape, the current pen parameters,
 *    and the current shape into the shapes array.  The mouse
 *    button is also marked as pressed.
 ******************************************************************/

void DoLButtonDown(HWND hWnd, LONG lParam)
{
    /*
     * Redirect all subsequent mouse movements to this
     * window until the mouse button is released.
     */
    SetCapture(hWnd);
    oldy = thisShape[++CurrentPoint].Points.top = HIWORD(lParam);
    oldx = thisShape[CurrentPoint].Points.left = LOWORD(lParam);
    thisShape[CurrentPoint].theShape = CurrentShape;
    thisShape[CurrentPoint].PenWidth = PenWidth;
    thisShape[CurrentPoint].PenColor = PenColor;

    mouseDown = 1;

}

/******************************************************************
 * function DoLButtonUp
 *    When the Left mouse button is released, this routine
 *    allows other windows to receive mouse messages and saves
 *    the position of the mouse as the other corner of a bounding
 *    rectangle for the shape.
 ******************************************************************/

void DoLButtonUp(HWND hWnd, LONG lParam)
{
    if (mouseDown == 0) return;
    ReleaseCapture();

    /*
     * For rectangles to work with the IntersectRect function,
     * they must be stored as left, top, right, bottom.
     */
    SetRect(&thisShape[CurrentPoint].Points,
	    min(thisShape[CurrentPoint].Points.left, LOWORD(lParam)),
	    min(thisShape[CurrentPoint].Points.top, HIWORD(lParam)),
	    max(thisShape[CurrentPoint].Points.left, LOWORD(lParam)),
	    max(thisShape[CurrentPoint].Points.top, HIWORD(lParam)));

    /*
     * if the origin of the line has changed, it should be drawn
     * from upper-right to lower left and therefore has negative
     * slope.  Otherwise it will have positive slope.
     */
    if (CurrentShape == LINE)
	thisShape[CurrentPoint].Slope =
	    thisShape[CurrentPoint].Points.left ==
	    LOWORD(lParam) ||
	    thisShape[CurrentPoint].Points.top ==
	    HIWORD(lParam) ?
	    -1 : 1;

    /*
     * Mark this region on the window as
     * needing redrawing and force an update.
     */
    InvalidateRect(hWnd, &thisShape[CurrentPoint].Points, 0);
    UpdateWindow(hWnd);
    mouseDown = 0;
    oldx = -1;
    oldy = -1;
}

static int SaveROP;

/**********************************************************************
 * function DoMouseMove
 *    When the mouse is moved and the button is down, this function
 *    draws the current shape.  It uses the Raster Operation NOTXORPEN
 *    to draw the shape.  When this mode is used, drawing the
 *    same image twice returns the image to its original state.
 *    NOTXORPEN turns black on black white, black on white black
 *    and white on white white.
 **********************************************************************/

void DoMouseMove(HWND hWnd,LONG lParam)
{
    HDC hdc;

    if (mouseDown)
    {
	hdc = GetDC(hWnd);
	/*
	 * Erase the old shape.
	 */
	SaveROP = SetROP2(hdc,R2_NOTXORPEN);
	DrawShape(hdc, thisShape[CurrentPoint].Points.left,
		  thisShape[CurrentPoint].Points.top, oldx, oldy,
		  thisShape[CurrentPoint].theShape,
		  thisShape[CurrentPoint].PenWidth,
		  thisShape[CurrentPoint].PenColor, 1);
	/*
	 * At this point, the slope must be positive because
	 * the coordinates could not have been switched.
	 * The next step is to draw the new shape.
	 */

	oldx = LOWORD(lParam);
	oldy = HIWORD(lParam);
	DrawShape(hdc, thisShape[CurrentPoint].Points.left,
		  thisShape[CurrentPoint].Points.top, oldx, oldy,
		  thisShape[CurrentPoint].theShape,
		  thisShape[CurrentPoint].PenWidth,
		  thisShape[CurrentPoint].PenColor, 1);
	SetROP2(hdc,SaveROP);
	ReleaseDC(hWnd,hdc);
    }

}

/*********************************************************************
 * function DoWMCommand
 *    When a menu item is selected, this function changes the current
 *    state of shape selections to match the user's menu selection.
 *******************************************************************/

int DoWMCommand(WORD wParam, HWND hWnd)
{
    switch(wParam)
    {
	case MID_QUIT:
       DestroyWindow(hWnd);
	    break;

	case MID_LINE:
	    CurrentShape = LINE;
	    break;

	case MID_ELLIPSE:
	    CurrentShape = ELLIPSE;
	    break;

	case MID_RECTANGLE:
	    CurrentShape = RECTANGLE;
	    break;

	case MID_THIN:
	    PenWidth = 1;
	    break;

	case MID_REGULAR:
	    PenWidth = 3;
	    break;

	case MID_THICK:
	    PenWidth = 5;
	    break;

	case MID_RED:
	    PenColor = RGB(255,0,0);
	    break;

	case MID_GREEN:
	    PenColor = RGB(0,255,0);
	    break;

	case MID_BLACK:
	    PenColor = RGB(0,0,0);
	    break;

	default:
	    return 0;
    }

    return 1;
}
