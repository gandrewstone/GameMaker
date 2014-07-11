// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __SCROLLER_H
#define __SCROLLER_H

// SCROLLER.H
// Defines type TScroller.
//

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TWindow)
_CLASSDEF(TScroller)

#define MaxInt 32767
long LongMulDiv(long Mult1, long Mult2, long Div1);

  /* TScroller object */

class _EXPORT TScroller : public Object, public TStreamable
{
public:
    PTWindow Window;
    long XPos, YPos;      // current pos in horz/vert scroll units
    long XRange, YRange;  // # of scrollable horz/vert scroll units
    int XUnit, YUnit;     //logical device units per horz/vert scroll unit
    int XLine, YLine;        // # of horz/vert scroll units per line
    int XPage, YPage;        // # of horz/vert scroll units per page
    BOOL AutoMode;           // auto scrolling mode
    BOOL TrackMode;          // track scroll mode
    BOOL AutoOrg;            // indicates Scroller offsets origin
    BOOL HasHScrollBar;
    BOOL HasVScrollBar;

    TScroller(PTWindow TheWindow, int TheXUnit, int TheYUnit,
              long TheXRange, long TheYRange);
    ~TScroller();
    void SetUnits(int TheXUnit, int TheYUnit);
    virtual void SetPageSize();
    virtual void SetSBarRange();
    void SetRange(long TheXRange, long TheYRange);
#if defined(WIN31)
    // windows 3.1 interface
    virtual void BeginView(HDC PaintDC, PAINTSTRUCT _FAR & PaintInfo);
#endif
#if defined(WIN30)
    // windows 3.0 interface
    virtual void BeginView(HDC_30 PaintDC, PAINTSTRUCT _FAR & PaintInfo);
#endif
#if (defined(WIN30) || defined(WIN31)) && !(defined(WIN30) && defined(WIN31))
    // this function is never called. it is used to pad the vtable so that
    // exactly two BeginView(...) definitions are always present.
    virtual void BeginView(void *, void *) { };
#endif
    virtual void EndView();
    virtual void VScroll(WORD ScrollEvent, int ThumbPos);
    virtual void HScroll(WORD ScrollEvent, int ThumbPos);
    virtual void ScrollTo(long X, long Y);

    /* Scrolls to a position calculated using the passed
       "Delta" values. */
    void ScrollBy(long Dx, long Dy)
        { ScrollTo(XPos + Dx, YPos + Dy); }

    virtual void AutoScroll();

    /* Returns a BOOL value indicating whether or not the passed
       area (in units) is currently visible. */
    BOOL IsVisibleRect(long X, long Y, int XExt, int YExt)
        { return (X + XExt >= XPos) && (Y + YExt >= YPos) &&
             (X < XPos + XPage) && (Y < YPos + YPage); }

    /* Converts a horizontal range value from the scrollbar to a
       horizontal scroll value. */
    int XScrollValue(long ARangeUnit)
        { return (int)LongMulDiv(ARangeUnit, MaxInt, XRange); }

    /* Converts a vertical range value from the scrollbar to a
       vertical scroll value. */
    int YScrollValue(long ARangeUnit)
        { return (int)LongMulDiv(ARangeUnit, MaxInt, YRange); }

    /* Converts a horizontal scroll value from the scrollbar to
       a horizontal range value. */
    long XRangeValue(int AScrollUnit)
        { return LongMulDiv(AScrollUnit, XRange, MaxInt); }

    /* Converts a vertical scroll value from the scrollbar to
       a vertical range value. */
    long YRangeValue(int AScrollUnit)
        { return LongMulDiv(AScrollUnit, YRange, MaxInt); }

    // define pure virtual functions derived from Object class
    virtual classType  	isA() const
	    { return scrollerClass; }
    virtual Pchar nameOf() const
	    { return "TScroller"; }
    virtual hashValueType hashValue() const
	    { return InstanceHashValue; }
    virtual int   	isEqual(RCObject testobj)  const
        { return (this ==  &(RTScroller)testobj); }
    virtual void     	printOn(Rostream outputStream) const
        { outputStream << nameOf() << "{ Window = " << Window <<" }\n";}

    static PTStreamable build();

protected:
    TScroller(StreamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);
    hashValueType InstanceHashValue;
    static hashValueType ClassHashValue;

private:
    virtual const Pchar streamableName() const
	    { return "TScroller"; }
	
    void __BeginView(HDC PaintDC, PAINTSTRUCT _FAR & PaintInfo);	
};

inline Ripstream operator >> ( Ripstream is, RTScroller cl )
    { return is >> (RTStreamable )cl; }
inline Ripstream operator >> ( Ripstream is, RPTScroller cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTScroller cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTScroller cl )
    { return os << (PTStreamable )cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif  // ifdef __SCROLLER_H
