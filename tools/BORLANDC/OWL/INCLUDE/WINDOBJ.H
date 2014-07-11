// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __WINDOBJ_H
#define __WINDOBJ_H

#ifndef __WINDOWS_H
#undef NULL
#include <windows.h>
#endif

#ifndef __OBJECT_H
#include <object.h>
#endif

#ifndef __APPLICAT_H
#include <applicat.h>
#endif

#ifndef __OWLDEFS_H
#include <owldefs.h>
#endif

#ifndef __OBJSTRM_H
#include <objstrm.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

//  windows 3.1 name confict with TWindowsObject::GetFirstChild()
#if defined(GetFirstChild)
#undef GetFirstChild(hwnd)
#endif

struct TMessage {
  HWND Receiver;
  WORD Message;
  union {
    WORD WParam;
        struct tagWP {
          BYTE Lo;
          BYTE Hi;
        } WP;
  };
  union {
        DWORD LParam;
        struct tagLP {
          WORD Lo;
          WORD Hi;
        } LP;
  };
  long Result;
};

_CLASSDEF(TMDIClient)
_CLASSDEF(TApplication)
_CLASSDEF(TModule)
_CLASSDEF(TWindowsObject)


typedef TMessage _FAR &RTMessage;

typedef void ( _FAR *TActionFunc )(Pvoid Child, Pvoid ParamList);
typedef BOOL ( _FAR *TCondFunc )(Pvoid Child, Pvoid ParamList);

/* TWindowsObject */
class _EXPORT TWindowsObject : public Object, public TStreamable
{
public:
    int Status;
    HWND HWindow;	// handle to associated MS-Windows window
    LPSTR Title;
    PTWindowsObject Parent;

    TWindowsObject(PTWindowsObject AParent, PTModule AModule = NULL);
    virtual ~TWindowsObject();
    void SetFlags(WORD Mask, BOOL OnOff);

    /* Determines whether the flag whose mask is passed has been set,
       returning a BOOL indicator --  True = On, False = Off. */
    BOOL IsFlagSet(WORD Mask)
        { return( (Flags & Mask) == Mask); }

    PTWindowsObject FirstThat(TCondFunc Test, Pvoid PParamList);
    void ForEach(TActionFunc Action, Pvoid PParamList);
    PTWindowsObject FirstThat(
	    BOOL (TWindowsObject::* _FAR Test)(Pvoid, Pvoid), Pvoid PParamList);
    void ForEach(
	    void (TWindowsObject::* _FAR Action)(Pvoid, Pvoid), Pvoid PParamList);

    PTWindowsObject Next();
    PTWindowsObject GetFirstChild()
	    { return ChildList->SiblingList; }
    PTWindowsObject GetLastChild()
	    { return ChildList; }
    PTWindowsObject Previous();
    void EnableKBHandler();
    void EnableAutoCreate();
    void DisableAutoCreate();
    void EnableTransfer();
    void DisableTransfer();

    PTModule GetModule()
        { return Module; }
    PTApplication GetApplication()
        { return Application; }
    WNDPROC GetInstance()
        { return Instance; }
    virtual BOOL Register();

/* Pure virtual function, placeholder for derived classes to redefine to
   create an MS_Windows element to be associated with an OWL window
   object */
    virtual BOOL Create() = 0;

    virtual void Destroy();

    virtual int GetId();
    PTWindowsObject ChildWithId(int Id);
    virtual PTMDIClient GetClient();
    virtual void SetParent(PTWindowsObject NewParent);
    void Show(int ShowCmd);
    void SetCaption(LPSTR ATitle);
    virtual BOOL CanClose();
    void SetTransferBuffer(Pvoid ATransferBuffer)
        { TransferBuffer = ATransferBuffer; }
    virtual WORD Transfer(Pvoid DataPtr, WORD TransferFlag);
    virtual void TransferData(WORD Direction);
    virtual void DefWndProc(RTMessage Msg);
    virtual void BeforeDispatchHandler() {}
    virtual void AfterDispatchHandler() {}
    virtual void DispatchAMessage(WORD AMsg, RTMessage AMessage,
	    void (TWindowsObject::* _FAR)(RTMessage ));
    void CloseWindow();
    void GetChildren(Ripstream is);
    void PutChildren(Ropstream os);
    BOOL CreateChildren();
    virtual void ShutDownWindow();
    virtual void DrawItem(DRAWITEMSTRUCT far & DrawInfo);
    virtual void ActivationResponse(WORD Activated, BOOL IsIconified);

    // define pure virtual functions derived from Object class
    virtual classType      isA() const = 0;
    virtual Pchar nameOf() const = 0;
    virtual hashValueType  hashValue() const
        { return hashValueType(HWindow); }
    virtual int       	   isEqual(RCObject testwin)  const
        { return this ==  &(RTWindowsObject)testwin; }
    virtual void      	   printOn(Rostream outputStream) const
	{ outputStream << nameOf() << "{ HWindow = "
		 << (void _FAR *)HWindow << " }\n"; }

    static PTStreamable build();

protected:

    WNDPROC DefaultProc;
    Pvoid TransferBuffer;
    virtual void GetWindowClass(WNDCLASS _FAR & AWndClass);
    virtual LPSTR GetClassName() = 0;
    void RemoveClient()
		{ RemoveChild((PTWindowsObject)GetClient()); }
	void GetChildPtr(Ripstream is, RPTWindowsObject P);
	void PutChildPtr(Ropstream os, PTWindowsObject P);
	void GetSiblingPtr(Ripstream is, RPTWindowsObject P);
	void PutSiblingPtr(Ropstream os, PTWindowsObject P);
    virtual void DefCommandProc(RTMessage Msg);
    virtual void DefChildProc(RTMessage Msg);
    virtual void DefNotificationProc(RTMessage Msg);
    virtual void SetupWindow();
    virtual void WMVScroll(RTMessage Msg) =
                 [WM_FIRST + WM_VSCROLL];
    virtual void WMHScroll(RTMessage Msg) =
                 [WM_FIRST + WM_HSCROLL];
    void DispatchScroll(RTMessage Msg);
    virtual void WMCommand(RTMessage Msg) =
                 [WM_FIRST + WM_COMMAND];
    virtual void WMDrawItem(RTMessage Msg) =
                 [WM_FIRST + WM_DRAWITEM];
    virtual void WMClose(RTMessage Msg) =
                 [WM_FIRST + WM_CLOSE];
    virtual void WMDestroy(RTMessage Msg) =
                 [WM_FIRST + WM_DESTROY];
    virtual void WMNCDestroy(RTMessage Msg) =
                 [WM_FIRST + WM_NCDESTROY];
    virtual void WMActivate(RTMessage Msg) =
                 [WM_FIRST + WM_ACTIVATE];
    virtual void WMQueryEndSession(RTMessage Msg) =
                 [WM_FIRST + WM_QUERYENDSESSION];
    virtual void CMExit(RTMessage Msg) =
                 [CM_FIRST + CM_EXIT];

    TWindowsObject(StreamableInit) {};
    virtual void write (Ropstream os);
    virtual Pvoid read (Ripstream is);

private:

    WNDPROC Instance;
    PTApplication Application;
    PTModule Module;
    WORD Flags;
    WORD CreateOrder;
    BOOL OrderIsI(Pvoid P, Pvoid I);
    BOOL CreateZeroChild(Pvoid P, Pvoid I);
    void AssignCreateOrder();
    PTWindowsObject ChildList, SiblingList;
    void AddChild(PTWindowsObject AChild);
    void RemoveChild(PTWindowsObject AChild);
    int IndexOf(PTWindowsObject P);
    PTWindowsObject At(int APosition);

    virtual const Pchar streamableName() const
        { return "TWindowsObject"; }

};	// end of WindowsObject class

/* Returns the Id of the TWindowsObject, used to identify the window
   in a specified parent's ChildList.  Redefined by TControl
   descendants to return their identifier from their attributes
   structure.  0 is returned here as the default identifier. This
   precludes any window with a 0 Id from being easily found. If you
   need to address individual windows, redefine GetId to return
   something other than 0.*/
inline int TWindowsObject::GetId()
        { return 0; }

/* Returns a pointer to the TWindowsObject's next sibling (the next
   window in its parent's child window list.  If this was the last child
   added to the list, returns a pointer to the first child added.*/
inline PTWindowsObject TWindowsObject::Next()
        { return SiblingList; }

typedef void ( TWindowsObject::* _FAR TActionMemFunc )(Pvoid Child, Pvoid ParamList);
typedef BOOL ( TWindowsObject::* _FAR TCondMemFunc )(Pvoid Child, Pvoid ParamList);

inline Ripstream operator >> ( Ripstream is, RTWindowsObject cl )
    { return is >> (RTStreamable)cl; }
inline Ripstream operator >> ( Ripstream is, RPTWindowsObject cl )
    { return is >> (RPvoid)cl; }

inline Ropstream operator << ( Ropstream os, RTWindowsObject cl )
    { return os << (RTStreamable )cl; }
inline Ropstream operator << ( Ropstream os, PTWindowsObject cl )
    { return os << (PTStreamable )cl; }

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif  // ifndef _WINDOBJ_H
