// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  SCROLLER.CPP
  Defines type TScroller.
  -------------------------------------------------------- */

#include "applicat.h"
#include "window.h"

#define max(a,b)(((a) >(b)) ?(a) :(b))
#define min(a,b)(((a) <(b)) ?(a) :(b))
#define abs(a)  (((a) >= 0)  ?(a) :-(a))

hashValueType TScroller::ClassHashValue = 0;

/* Constructs a TScroller object, initializing its data fields to
   default values. */
TScroller::TScroller(PTWindow TheWindow,int TheXUnit, int TheYUnit, long TheXRange, long TheYRange)
{
  Window = TheWindow;
  XPos = YPos = 0;
  XUnit = TheXUnit;
  YUnit = TheYUnit;
  XRange = TheXRange;
  YRange = TheYRange;
  XLine = 1;  YLine = 1;
  XPage = 1;  YPage = 1;
  AutoMode = TRUE;
  TrackMode = TRUE;
  AutoOrg = TRUE;
  HasHScrollBar = (Window &&
          ((Window->Attr.Style & WS_HSCROLL) == WS_HSCROLL));
  HasVScrollBar = (Window &&
          ((Window->Attr.Style & WS_VSCROLL) == WS_VSCROLL));
  InstanceHashValue = ClassHashValue++;
}

/* Destructs a TScroller object. Sets Window's Scroller member to NULL */
TScroller::~TScroller()
{
  if ( Window && Window->Scroller )
    Window->Scroller = NULL;
}

/* Sets the number of units per page (amount by which to scroll on
   a page scroll request) according to the current size of the
   Window's client area. */
void TScroller::SetPageSize()
{
  RECT ClientRect;
  int Width, Height;

  if ( Window )
  {
    GetClientRect(Window->HWindow, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
    if ( (Width != 0) && (XUnit > 0) )
      XPage = max(1, ((Width+1)/XUnit) -1);
    if ( (Height != 0) && (YUnit > 0) )
      YPage = max(1, ((Height+1)/YUnit) -1);
  }
}

/* Sets the range of the TScroller and also sets the range of
   its Window's scrollbars. */
void TScroller::SetRange(long TheXRange, long TheYRange)
{
  XRange = TheXRange;
  YRange = TheYRange;
  SetSBarRange();
  ScrollTo(XPos, YPos);
}

/* Resets the X and Y scroll unit size (in device units) to the passed
  parameters.  Calls SetPageSize to update the X and Y page size, which
  are specified in scroll units. */
void TScroller::SetUnits(int TheXUnit, int TheYUnit)
{
  XUnit = TheXUnit;
  YUnit = TheYUnit;
  SetPageSize();
}

/* Sets the range of the Window's scrollbars. */
void TScroller::SetSBarRange()
{
  if ( Window )
  {
    if ( HasHScrollBar )
        SetScrollRange(Window->HWindow, SB_HORZ, 0,
                       max(0, (int)min(XRange, MaxInt)), FALSE);
    if ( HasVScrollBar )
        SetScrollRange(Window->HWindow, SB_VERT, 0,
                       max(0, (int)min(YRange, MaxInt)), FALSE);
  }
}

/* Sets the origin for the paint display context according to
   XPos, YPos. */

inline void TScroller::__BeginView(HDC PaintDC, PAINTSTRUCT&)
{
  long XOrg, YOrg;

  XOrg = XPos * XUnit;
  YOrg = YPos * YUnit;
  if ( AutoOrg && (XOrg <= MaxInt) && (YOrg <= MaxInt) )
    SetViewportOrg(PaintDC, (int)-XOrg, (int)-YOrg);
}

#if  defined(WIN31)
void TScroller::BeginView(HDC PaintDC, PAINTSTRUCT&)
{
  PAINTSTRUCT ps_tmp;
  __BeginView(PaintDC, ps_tmp);
}
#endif
#if  defined(WIN30)
void TScroller::BeginView(HDC_30 PaintDC, PAINTSTRUCT&)
{
  PAINTSTRUCT ps_tmp;	
  __BeginView( HDC( PaintDC ), ps_tmp);
}
#endif

/* Updates the position of the Window's scrollbar(s). */
void TScroller::EndView()
{
  int TempPos;

  if ( Window )
  {
    if ( HasHScrollBar )
    {
      if ( XRange > MaxInt )
        TempPos = XScrollValue(XPos);
      else
        TempPos = (int)XPos;
      if ( GetScrollPos(Window->HWindow, SB_HORZ) != TempPos )
        SetScrollPos(Window->HWindow, SB_HORZ, TempPos, TRUE);
    }
    if ( HasVScrollBar )
    {
      if ( YRange > MaxInt )
        TempPos = YScrollValue(YPos);
      else
        TempPos = (int)YPos;
      if ( GetScrollPos(Window->HWindow, SB_VERT) != TempPos )
        SetScrollPos(Window->HWindow, SB_VERT, TempPos, TRUE);
    }
  }
}

/* Scrolls vertically according to scroll action and thumb position. */
void TScroller::VScroll(WORD ScrollEvent, int ThumbPos)
{
  switch (ScrollEvent) {
    case SB_LINEDOWN : ScrollBy(0, YLine);
                       break;
    case SB_LINEUP   : ScrollBy(0, -YLine);
                       break;
    case SB_PAGEDOWN : ScrollBy(0, YPage);
                       break;
    case SB_PAGEUP   : ScrollBy(0, -YPage);
                       break;
    case SB_TOP      : ScrollTo(XPos, 0);
                       break;
    case SB_BOTTOM   : ScrollTo(XPos, YRange);
                       break;

    case SB_THUMBPOSITION :
      if ( YRange > MaxInt )
        ScrollTo(XPos, YRangeValue(ThumbPos));
      else
        ScrollTo(XPos, ThumbPos);
      break;
    case SB_THUMBTRACK :
      if ( TrackMode )
        if ( YRange > MaxInt )
          ScrollTo(XPos, YRangeValue(ThumbPos));
        else
          ScrollTo(XPos, ThumbPos);
      if ( Window && HasVScrollBar )
        SetScrollPos(Window->HWindow, SB_VERT, ThumbPos, TRUE);
      break;
  }  // switch
}

/* Scrolls horizontally according to scroll action and thumb
   position. */
void TScroller::HScroll(WORD ScrollEvent, int ThumbPos)
{
  switch (ScrollEvent) {
    case SB_LINEDOWN : ScrollBy(XLine, 0);
                       break;
    case SB_LINEUP : ScrollBy(-XLine, 0);
                     break;
    case SB_PAGEDOWN : ScrollBy(XPage, 0);
                       break;
    case SB_PAGEUP : ScrollBy(-XPage, 0);
                     break;

    case SB_TOP      : ScrollTo(0, YPos);
                       break;
    case SB_BOTTOM   : ScrollTo(XRange, YPos);
                       break;

    case SB_THUMBPOSITION :
      if ( XRange > MaxInt )
        ScrollTo(XRangeValue(ThumbPos), YPos);
      else
        ScrollTo(ThumbPos, YPos);
      break;
    case SB_THUMBTRACK :
        if ( TrackMode )
          if ( XRange > MaxInt )
            ScrollTo(XRangeValue(ThumbPos), YPos);
          else
            ScrollTo(ThumbPos, YPos);
        if ( (Window) && HasHScrollBar )
          SetScrollPos(Window->HWindow, SB_HORZ, ThumbPos, TRUE);
        break;
  }  // switch
}

/* Scrolls to an (X,Y) position, after checking boundary conditions.
   Causes a WMPaint message to be sent.  First scrolls the contents
   of the client area, if a portion of the client area will
   remain visible. */
void TScroller::ScrollTo(long X, long Y)
{
  long NewXPos , NewYPos;

  if ( Window )
  {
    NewXPos = max(0, min(X, XRange));
    NewYPos = max(0, min(Y, YRange));
    if ( (NewXPos != XPos) || (NewYPos != YPos) )
    {
    /* Scaling is not needed here. If condition is met, ScrollWindow()
       will succeed since XPage and YPage are ints. If condition is not
       met, ScrollWindow() is called in EndScroll() as a result of calling
       UpdateWindow(). EndScroll() performs the necessary scaling.
    */
      if ( AutoOrg || (abs(YPos - NewYPos) < YPage) &&
                      (abs(XPos - NewXPos) < XPage) )
        ScrollWindow(Window->HWindow, (int)((XPos-NewXPos)*XUnit),
                          (int)((YPos-NewYPos)*YUnit), NULL, NULL);
      else
        InvalidateRect(Window->HWindow, NULL, TRUE);
      XPos = NewXPos;
      YPos = NewYPos;
      UpdateWindow(Window->HWindow);
    }
  }
}

/* Performs "auto-scrolling".  (Dragging the mouse from within the
   client area of the Window to without results in auto-scrolling
   when the AutoMode data member of the Scroller is TRUE). */
void TScroller::AutoScroll()
{
  POINT CursorPos;
  RECT ClientRect;
  long Dx , Dy;

  if ( AutoMode && Window )
  {
    GetCursorPos(&CursorPos);
    ScreenToClient(Window->HWindow, &CursorPos);
    GetClientRect(Window->HWindow, &ClientRect);
    Dx = 0; Dy = 0;
    if ( CursorPos.y < 0 )
      Dy = min(-YLine, max(-YPage, (CursorPos.y/10) * YLine));
    else
      if ( CursorPos.y > ClientRect.bottom )
        Dy = max(YLine, min(YPage,
                     ((CursorPos.y - ClientRect.bottom)/10) * YLine));
    if ( CursorPos.x < 0 )
      Dx = min(-XLine, max(-XPage, (CursorPos.x/10) * XLine));
    else
      if ( CursorPos.x > ClientRect.right )
        Dx = max(XLine, min(XPage, ((CursorPos.x - ClientRect.right)/10) * XLine));
    ScrollBy(Dx, Dy);
  }
}

/* Reads an instance of TScroller from the passed ipstream. */
void *TScroller::read(ipstream& is)
{
  is >> XPos >> YPos
     >> XUnit >> YUnit
     >> XRange >> YRange
     >> XLine >> YLine
     >> XPage >> YPage
     >> AutoMode >> TrackMode
     >> AutoOrg
     >> HasHScrollBar >> HasVScrollBar;
  Window = NULL;
  InstanceHashValue = ClassHashValue++;
  return this;
}

/* Writes the TScroller to the passed opstream. */
void TScroller::write(opstream& os)
{
  os << XPos << YPos
     << XUnit << YUnit
     << XRange << YRange
     << XLine << YLine
     << XPage << YPage
     << AutoMode << TrackMode
     << AutoOrg
     << HasHScrollBar << HasVScrollBar;
}

TStreamable *TScroller::build()
{
  return new TScroller(streamableInit);
}

TStreamableClass RegScroller("TScroller", TScroller::build,
			     __DELTA(TScroller));

