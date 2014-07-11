// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  WINDOBJ.CPP
  Defines type TWindowsObject.  This defines the basic behavior
  of all Windows objects.
  -------------------------------------------------------- */

#include <string.h>
#include <alloc.h>
#include "applicat.h"
#include "windobj.h"
#include "appdict.h"

extern WNDPROC MakeObjectInstance(PTWindowsObject);
extern void FreeObjectInstance(WNDPROC);
extern PTWindowsObject GetObjectPtr(HWND);

/* Constructor for a TWindowsObject.  If a parent window is passed, adds
  the TWindowsObject to its parent's list of children.  Makes an
  instance thunk to be used in associating an MS-Windows interface
  element to the TWindowsObject. */
TWindowsObject::TWindowsObject(PTWindowsObject AParent, PTModule AModule)
{
  Status = 0;
// If this is a "DLL alias", a derived class's constructor must set HWindow
  HWindow = 0;
  Title = NULL;
  CreateOrder = 0;
  ChildList = NULL;
  Flags = 0x0;
  TransferBuffer = NULL;
  EnableAutoCreate();
  Instance = MakeObjectInstance(this);
  DefaultProc = NULL;

  Parent = AParent;
  if ( Parent )
    Parent->AddChild(this);
  else
    SiblingList = NULL;

  Application = GetApplicationObject();
  if ( AModule )
    Module = AModule;
  else
  /* by default Module = Application for all applications and for DLLs
     dynamically linked with the OWL DLL. */
    if ( Application )
      Module = (PTModule)Application;
    else  // In a DLL statically linked with OWL and AModule == 0
      Status = EM_INVALIDMODULE;
}

static void FreeChild(void *P, void *)
{
  ((PTWindowsObject)P)->ShutDownWindow();
}

/* Destructor for a TWindowsObject.  Disposes of each window in its
  ChildList and removes itself from a non-NULL parent's list of
  children. Destroys a still-associated MS-Windows interface element and
  frees the instance thunk used for association of an MS-Windows element
  to the TWindowsObject. */
TWindowsObject::~TWindowsObject()
{
  if ( !IsFlagSet(WB_ALIAS) ) //if alias only delete, don't destroy.
    Destroy();
  ForEach(FreeChild, 0);
  if (Parent)
    Parent->RemoveChild(this);
  if ( GetApplication() && this == GetApplication()->MainWindow )
    GetApplication()->MainWindow = NULL;
  if ( HIWORD(Title) )
    farfree(Title);
  FreeObjectInstance(Instance);
}

void TWindowsObject::AssignCreateOrder()
{
  HWND CurWindow;
  PTWindowsObject Wnd;
  int I;

  Wnd = (PTWindowsObject)GetClient();
  if ( !Wnd )
    CurWindow = HWindow;
  else
    CurWindow = Wnd->HWindow;
  if ( CurWindow )
  {
    CurWindow = GetWindow(CurWindow, GW_CHILD);
    if ( CurWindow )
    {
      CurWindow = GetWindow(CurWindow, GW_HWNDLAST);
      I = 1;
      while ( CurWindow )
      {
        Wnd = (PTWindowsObject)GetObjectPtr(CurWindow);
        if ( Wnd )
          Wnd->CreateOrder = I++;
        CurWindow = GetWindow(CurWindow, GW_HWNDPREV);
      }
    }
  }
}

/* The private field CreateOrder is used to ensure the create order is
  consistent through read and write of the object.  If the object is
  written, write will fill in this value.  CreateOrder ranges in value
  from 1 to N where N is the number of objects with values.  All other
  objects will have a CreateOrder of Zero, which implies the object
  will be created after the last object with a create order
*/
BOOL TWindowsObject::OrderIsI(void *P, void *I)
{
  return ((PTWindowsObject)P)->CreateOrder == *(WORD *)I;
}

/*  Returns TRUE if the child was supposed to be created but couldn't be. */
static BOOL CantCreateChild(PTWindowsObject P)
{
  BOOL Success;

  Success = (!P->IsFlagSet(WB_AUTOCREATE)) || P->Create();
  if ( P->HWindow && IsIconic(P->HWindow) )
  {
    int TextLen = GetWindowTextLength(P->HWindow);
    Pchar Text = new char[TextLen + 1];
    GetWindowText(P->HWindow, Text, TextLen + 1);
    SetWindowText(P->HWindow, Text);
    delete Text;
  }
  return !Success;
}

BOOL TWindowsObject::CreateZeroChild(void *P, void *)
{
  return ((PTWindowsObject)P)->CreateOrder == 0 &&
         CantCreateChild((PTWindowsObject)P);
}

/* Create the children of the object.  Returns true if all the windows
   were successfully created. */
BOOL TWindowsObject::CreateChildren()
{
  int I = 1;
  PTWindowsObject P;
  BOOL Failure = FALSE;

  do {
    P = FirstThat(&TWindowsObject::OrderIsI, &I);
    if ( P )
      Failure = CantCreateChild(P);
    ++I;
  } while( !Failure && P != (PTWindowsObject)NULL );
  return !Failure && (FirstThat(&TWindowsObject::CreateZeroChild, NULL) == NULL);
}

/* Transfer window 'data' to/from the passed data buffer.  Used to
  initialize dialogs and get data out of them.  The TransferFlag passed
  specifies whether data is to be read from or written to the passed
  buffer, or whether the data element size is simply to be returned. The
  return value is the size (in bytes) of the transfer data.  This method
  simply returns zero and is redefined in TControl descendant classes. */
WORD TWindowsObject::Transfer(Pvoid, WORD)
{
    return 0;
}

/* Sets flag which indicates that the TWindowsObject has requested
  "keyboard handling" (translation of keyboard input into control
  selections). */
void TWindowsObject::EnableKBHandler()
{
  SetFlags(WB_KBHANDLER, TRUE);
}

/* Sets flag which indicates that the TWindowsObject should be
  created if a create is sent while in the parent's child list. */
void TWindowsObject::EnableAutoCreate()
{
  SetFlags(WB_AUTOCREATE, TRUE);
}

/* Sets flag which indicates that the TWindowsObject can/will
  tranfer data via the transfer mechanism. */
void TWindowsObject::EnableTransfer()
{
  SetFlags(WB_TRANSFER, TRUE);
}

/* Sets flag which indicates that the TWindowsObject should not be
  created if a create is sent while in the parent's child list. */
void TWindowsObject::DisableAutoCreate()
{
  SetFlags(WB_AUTOCREATE, FALSE);
}

/* Sets flag which indicates that the TWindowsObject cannot/
  will not tranfer data via the transfer mechanism. */
void TWindowsObject::DisableTransfer()
{
  SetFlags(WB_TRANSFER, FALSE);
}

/* Sets flag(s) for the TWindowsObject, which are stored in its Flags
   data field.  The mask of the flag(s) to be set
  (WB_KBHANDLER, etc.), and an OnOff "flag" are passed --
  On = True, Off = False. */
void TWindowsObject::SetFlags(WORD Mask, BOOL OnOff)
{
  if ( OnOff )
    Flags |= Mask;
  else
    Flags &= ~Mask;
}

/* Adds the passed pointer to a child window to the linked list
  of sibling windows which ChildList points to. */
void TWindowsObject::AddChild(PTWindowsObject AChild)
{
  if ( AChild )
    if ( !ChildList )
    {
      ChildList = AChild;
      AChild->SiblingList = AChild;
    }
    else
    {
      AChild->SiblingList = ChildList->SiblingList;
      ChildList->SiblingList = AChild;
      ChildList = AChild;
    }
}

/* Returns a pointer to the TWindowsObject's previous sibling (the
   window previous to the TWindowsObject in its parent's child window
   list). Returns the sibling which points to the TWindowsObject. If
   the TWindowsObject was the first child added to the list, returns
   a pointer to the last child added.*/
PTWindowsObject TWindowsObject::Previous()
{
  PTWindowsObject CurrentIndex;

  if ( !SiblingList )
   return NULL;
  else
  {
    CurrentIndex = this;
    while ( CurrentIndex->Next() != this)
      CurrentIndex = CurrentIndex->Next();
    return CurrentIndex;
  }
}

/* Removes the passed pointer to a child window from the linked list of
  sibling windows which the TWindowsObject's ChildList points to. */
void TWindowsObject::RemoveChild(PTWindowsObject AChild)
{
  PTWindowsObject LastChild , NextChild;

  if ( ChildList )
  {
    LastChild = ChildList;
    NextChild = LastChild;
    while ( (NextChild->SiblingList != LastChild) &&
            (NextChild->SiblingList != AChild) )
      NextChild = NextChild->SiblingList;
    if ( NextChild->SiblingList == AChild )
      if ( NextChild->SiblingList == NextChild )
        ChildList = NULL;
      else
      {
        if ( NextChild->SiblingList == ChildList )
          ChildList = NextChild;
        NextChild->SiblingList = NextChild->SiblingList->SiblingList;
      }
  }
}

/* Reparents "this" to a new parent */
void TWindowsObject::SetParent(PTWindowsObject NewParent)
{
  if ( Parent )
    Parent->RemoveChild(this);
  SiblingList = (PTWindowsObject)NULL;
  ::SetParent(HWindow, NewParent->HWindow);  // tell windows about it
  Parent = NewParent;
  if ( Parent )
    Parent->AddChild(this);
}

/* Returns a pointer to the first TWindowsObject in the
   ChildList that meets some specified criteria.If no child in the
   list meets the criteria, NULL is returned. The Test
   parameter which defines the criteria, is a pointer to a function
   that takes two pointers to void. The first void* will be used as
   the pointer to the child window and the second as a pointer to
   the Test function's additional parameters.  The Test function must
   return a Boolean value indicating whether the child passed meets
   the criteria.  */
PTWindowsObject TWindowsObject::FirstThat(TCondFunc Test,
                                          void *PParamList)
{
  PTWindowsObject NextChild;
  PTWindowsObject CurChild;
  BOOL Found = FALSE;

  if ( ChildList )
  {
    NextChild = ChildList->Next();
    do {
      CurChild = NextChild;
      NextChild = NextChild->Next();
      /* Test CurChild for ok */
      if ( (*Test)( CurChild, PParamList) )
        Found = TRUE;
    }
    while ( !Found && (CurChild != ChildList) && ChildList);

    if ( Found )
      return CurChild;
  }
  return NULL;

}

/* Iterates over each child window in the TWindowsObject's ChildList,
   calling the procedure whose pointer is passed as the Action to be
   performed for each child.  A pointer to a child is passed as the
   first parameter to the iteration procedure. */
void TWindowsObject::ForEach(TActionFunc Action, void *PParamList)
{
  PTWindowsObject CurChild;
  PTWindowsObject NextChild; // allows ForEach to delete children

  if ( ChildList )
  {
    NextChild = ChildList->Next();
    do
    {
      CurChild = NextChild;
      NextChild = NextChild->Next();
      ( *Action )( CurChild, PParamList );
    }
    while ( (CurChild != ChildList) && (ChildList) );
  }
}

/* Returns a pointer to the first TWindowsObject in the
   ChildList that meets some specified criteria.If no child in the
   list meets the criteria, NULL is returned. The Test
   parameter which defines the criteria, is a pointer to a member
   function (this is how it's different from FirstThat above).
   that takes two pointers to void. The first void* will be used as
   the pointer to the child window and the second as a pointer to
   the Test function's additional parameters.  The Test function must
   return a Boolean value indicating whether the child passed meets
   the criteria.  */
PTWindowsObject TWindowsObject::FirstThat(TCondMemFunc Test,
                                          void *PParamList)
{
  PTWindowsObject NextChild;
  PTWindowsObject CurChild;
  BOOL Found = FALSE;

  if ( ChildList )
  {
    NextChild = ChildList->Next();
    do {
      CurChild = NextChild;
      NextChild = NextChild->Next();
      /* Test CurChild for ok */
      if ( (this->*Test)( CurChild, PParamList) )
        Found = TRUE;
    }
    while ( !Found && (CurChild != ChildList) && ChildList);

    if ( Found )
      return CurChild;
  }
  return NULL;

}

/* Iterates over each child window in the TWindowsObject's ChildList,
   calling the member function (unlike ForEach above which takes pointer
   to non-member function) whose pointer is passed as the Action to
   be performed for each child.  A pointer to a child is passed as the
   first parameter to the iteration procedure. */
void TWindowsObject::ForEach(TActionMemFunc Action, void *PParamList)
{
  PTWindowsObject CurChild;
  PTWindowsObject NextChild; // allows ForEach to delete children

  if ( ChildList )
  {
    NextChild = ChildList->Next();
    do
    {
      CurChild = NextChild;
      NextChild = NextChild->Next();
      ( this->*Action )( CurChild, PParamList );
    }
    while ( (CurChild != ChildList) && (ChildList) );
  }
}

static int Position;

static BOOL IsItThisChild1(void *P, void *AChild)
{
  ++Position;
  return ((PTWindowsObject)P == (PTWindowsObject)AChild);
}

/* Returns the position at which the passed child window appears
   in the TWindowsObject's ChildList. If the child does not appear in
   the list, -1 is returned. Zero'th position is ChildList->Next */
int TWindowsObject::IndexOf(PTWindowsObject AChild)
{
  Position = -1;
  if ( FirstThat(IsItThisChild1, AChild) )
    return Position;
  else
    return -1;
}

/* Returns the child at the passed position in the TWindowsObject's
   ChildList.  The ChildList is circularly-referent so that passing a
   position larger than the number of children will cause the traversal
   of the list to wrap. Zero'th position is ChildList->Next */
PTWindowsObject TWindowsObject::At(int APosition)
{
  PTWindowsObject CurrentChild;

  if ( APosition == -1 )
    return NULL;
  CurrentChild = ChildList;
  if ( CurrentChild )
  {
    CurrentChild = ChildList->Next();
    while (APosition > 0)
    {
      CurrentChild = CurrentChild->Next();
      --APosition;
    }
  }
  return CurrentChild;
}

static BOOL IsItThisChild2(void *P, void *Id)
{
  return ( ((PTWindowsObject)P)->GetId() == *(int *)Id);
}

/* Returns a pointer to the window in the ChildList with the passed Id.
   If no child in the list has the passed Id, NULL is returned.*/
PTWindowsObject TWindowsObject::ChildWithId(int Id)
{
  return FirstThat(IsItThisChild2, &Id);
}

/* Specifies default processing for an incoming message. Invokes default
   processing, defined by MS-Windows. Calls the default window procedure
   if this is a window; sets Msg.Result to 0 if it's a dialog. Both cases
   must be handled here instead of in TWindow or TDialog because DefWndProc
   can be called while an object is being destructed, making it effectively
   non-virtual */
void TWindowsObject::DefWndProc(TMessage& Msg)
{
  if ( IsFlagSet(WB_MDIFRAME) )
  {
    HWND ClientHWnd;

    if ( GetClient() )
      ClientHWnd = GetClient()->HWindow;
    else
      ClientHWnd = 0;
    Msg.Result = DefFrameProc(HWindow, ClientHWnd, Msg.Message ,
                              Msg.WParam, Msg.LParam);
  }
  else
    if ( DefaultProc ) // this is a Window
      Msg.Result =
#ifdef STRICT
      CallWindowProc(DefaultProc, HWindow, Msg.Message,
				  Msg.WParam, Msg.LParam);
#else
      CallWindowProc((FARPROC)DefaultProc, HWindow, Msg.Message,
				  Msg.WParam, Msg.LParam);
#endif
    else // this is a dialog
      Msg.Result = 0;
}

// --------------------------------------------------------
// DispatchAMessage gets called by StdWndProc and
// Dispatches the Windows messages to the proper WM routine
// Calls _DDVTdispatch to return a pointer to the virtual function
// to call. If a virtual function is not defined calls the
// DefProc argument instead.
// ---------------------------------------------------------

typedef void (TWindowsObject::* MRMFP)(TMessage&);
typedef void (* MRFP)(TMessage&);

#if defined(_CLASSDLL) || defined(__DLL__)
#define getVptr(thisPtr)	(*(void far **)(thisPtr))
#else
#define getVptr(thisPtr)	(*(void near **)(thisPtr))
#endif

MRFP far * _DDVTdispatchNULL(void near *, int);
MRFP far * _DDVTdispatchNULL(void far  *, int);

typedef void FAR PASCAL (* DISPATCHHOOK)(HANDLE hWnd, WORD wMessage,
                                             WORD wParam, LONG lParam,
                                             void far *Handler,
                                             void far *ObjectPtr);
DISPATCHHOOK __OWL_DISPATCH_HOOK__ = NULL;

void TWindowsObject::DispatchAMessage(WORD AMsg, TMessage& Msg,
                                      MRMFP DefProc)
{
    MRFP FFP;
    MRMFP MFP;

    union
    {
        MRMFP mfp;
        MRFP fp;
    }   Temp;

    Temp.mfp = NULL;

    HWND TheHWindow = HWindow;

    BeforeDispatchHandler();

    FFP = *_DDVTdispatchNULL(getVptr(this), (int)AMsg);

    if (FFP)                        // found a DDVT function
    {
        Temp.fp = FFP;              // convert DDVT function pointer
        MFP = Temp.mfp;             //   to member function pointer
    }
    else                            // no DDVT function, use DefProc
        MFP = DefProc;

    if (__OWL_DISPATCH_HOOK__)
    {
        if (!FFP)                   // if don't have normal function ptr
        {                           //   convert from member function ptr
            Temp.mfp = MFP;         //   to normal function pointer
            FFP = Temp.fp;
        }

        (*__OWL_DISPATCH_HOOK__)(TheHWindow, Msg.Message, Msg.WParam,
                    Msg.LParam, (void far *)FFP, this);
    }

    (this->*MFP)(Msg);           // invoke handler

    if ( this == GetObjectPtr(TheHWindow) ) // this hasn't been deleted
      AfterDispatchHandler();
}

/* Responds to an incoming WM_COMMAND message.  If a child window had
  the focus when the message was sent, the message is sent to the child
  window.  If the message cannot be given to a child window, it is
  given to "this"  */
void TWindowsObject::WMCommand(TMessage& Msg)
{
  HWND CurrentWindow, Control;
  PTWindowsObject Child;

  if ( IsFlagSet(WB_KBHANDLER) && (Msg.LParam == 0L) )
  {
    Control = GetDlgItem(HWindow, Msg.WParam);
    if ( Control && ( SendMessage(Control, WM_GETDLGCODE, 0, 0) &
         (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON) ) )
    {
      Msg.LP.Lo = (WORD)Control;
      Msg.LP.Hi = BN_CLICKED;
    }
  }
  if ( !IsFlagSet(WB_ALIAS) &&
     (LOWORD(Msg.LParam) == 0) )        // it's a command message and...
  {
    if ( Msg.WParam < CM_COUNT )        // ... we can route it
    {
      // Find the object closest to the focus window
      CurrentWindow = GetFocus(); // window with focus when command was sent
      Child = (PTWindowsObject)GetObjectPtr(CurrentWindow);
      while ( !Child && CurrentWindow && CurrentWindow != HWindow )
      {
        CurrentWindow = GetParent(CurrentWindow);
        Child = (PTWindowsObject)GetObjectPtr(CurrentWindow);
      }

 // If the object is found, route to the object, else handle it yourself
      if ( !Child )
        Child = this;
      Child->DispatchAMessage(CM_FIRST + Msg.WParam, Msg, &TWindowsObject::DefCommandProc);
    }
    else
      DefWndProc(Msg);
  }
  else
  {
    // Find the child that generated the notification
    Child = (PTWindowsObject)GetObjectPtr(GetDlgItem(HWindow, Msg.WParam));

    /* If the child is found, give the notification to the child,
      else give it to "this" as an "id" notification. */
    if ( Child && HIWORD(Msg.LParam) < NF_COUNT )
      Child->DispatchAMessage(NF_FIRST + HIWORD(Msg.LParam),
               Msg, &TWindowsObject::DefNotificationProc);
    else
      if ( !IsFlagSet(WB_ALIAS) )
      {
        if ( Msg.WParam < ID_COUNT )
          DispatchAMessage(ID_FIRST + Msg.WParam,
                 Msg, &TWindowsObject::DefChildProc);
        else
          DefChildProc(Msg);
      }
      else  // if I'm an alias don't route ID messages to me
        DefWndProc(Msg);
  }
}

/* Function called when a window must redraw one of its "owner-draw"
   controls. Having the control redraw itself is usually preferable.
   Also called when the window's "owner draw" menu must be redrawn */
void TWindowsObject::DrawItem(DRAWITEMSTRUCT far &)
{
}

/* Dispatches WM_DRAWITEM messages (for owner draw controls) to the
   appropriate control's appropriate member function */
void TWindowsObject::WMDrawItem(TMessage& Msg)
{
  PTWindowsObject Child = (PTWindowsObject)GetObjectPtr(
                          ((LPDRAWITEMSTRUCT)(Msg.LParam))->hwndItem);
  if ( Child )
    Child->DispatchAMessage(WM_FIRST + WM_DRAWITEM, Msg,
                                    &TWindowsObject::DefWndProc);
  else  // no child object (includes case where item is a menu)
  {
    if ( IsFlagSet(WB_ALIAS) )
      DefWndProc(Msg);
    else
      DrawItem(*((LPDRAWITEMSTRUCT)(Msg.LParam)));
  }
}

/* Dispatches scroll messages as if they where WMCommand messages,
   that is by routing them to the scroll bar control as a notification
   and to "this" as an "id" notification.  */
void TWindowsObject::DispatchScroll(TMessage& Msg)
{
    PTWindowsObject Child;
    WORD ChildId;

  if ( HIWORD(Msg.LParam) )
  {
    Child = (PTWindowsObject)GetObjectPtr((HWND)HIWORD(Msg.LParam));
    if ( Child )
      Child->DispatchAMessage(NF_FIRST + Msg.WParam, Msg, &TWindowsObject::DefNotificationProc);
    else
    {
      ChildId = GetWindowWord((HWND)HIWORD(Msg.LParam), GWW_ID);
      if ( ChildId < ID_COUNT )
        DispatchAMessage(ID_FIRST + ChildId, Msg, &TWindowsObject::DefChildProc);
      else
        DefChildProc(Msg);
    }
  }
  else
    DefWndProc(Msg);
}

/* Responds to an incoming WM_VSCROLL message by calling DispatchScroll. If
   message is not handled, calls DefWndProc. */
void TWindowsObject::WMVScroll(TMessage& Msg)
{
  if ( !(GetWindowLong(HWindow, GWL_STYLE) & WS_VSCROLL) )
    DispatchScroll(Msg);
  else
    DefWndProc(Msg);
}

/* Responds to an incoming WM_HSCROLL message by calling DispatchScroll.
   If message is not handled, calls DefWndProc. */
void TWindowsObject::WMHScroll(TMessage& Msg)
{
  if ( !(GetWindowLong(HWindow, GWL_STYLE) & WS_HSCROLL) )
    DispatchScroll(Msg);
  else
    DefWndProc(Msg);
}

/* Performs default processing for a command message (menu selection or
  accelerator). If the original message receiver was this object, give
  the message to DefWndProc, else if the object has a parent, give the
  message to the parent, else give the message to the original receiver. */
void TWindowsObject::DefCommandProc(TMessage& Msg)
{
  PTWindowsObject Target;

  if ( Msg.Receiver == HWindow )
    Target = NULL;
  else
    if ( Parent )
      Target = Parent;
    else
      Target = (PTWindowsObject)GetObjectPtr(Msg.Receiver);
  if ( Target == NULL )
    DefWndProc(Msg);
  else
    Target->DispatchAMessage(CM_FIRST + Msg.WParam, Msg, &TWindowsObject::DefCommandProc);
}

/* Performs default processing for an incoming notification message from
  a child of the TWindowsObject. Nothing can be done by default of a
  child notification (or "id" message). The user can override this
  function if it is more convenient to handle "id" messages in a case
  statement. */
void TWindowsObject::DefChildProc(TMessage& Msg)
{
  DefWndProc(Msg);
}

/* Performs default processing for a notification message generated by
   the TWindowsObject. (The TWindowsObject has the option to perform
   processing in response to its own notification messages. )  It passes
   the message to the parent as an "id" message.  It is assumed that the
   object giving this message to this object is the parent of this
   object.  This is done in WMCommand, WMHScroll, or WMVScroll of the
   parent. Notifications are translated into "id" messages so that the
   parent does not confuse child notification with its own
   notifications. If its an WMHScroll or WMVScroll the ID is looked up
   explicitly. */
void TWindowsObject::DefNotificationProc(TMessage& Msg)
{
  if ( Parent )
  {
    if ( Parent->IsFlagSet(WB_ALIAS) )
      DefWndProc(Msg);
    else
    {
      if (Msg.Message == WM_COMMAND )
        Parent->DispatchAMessage(ID_FIRST + Msg.WParam, Msg, &TWindowsObject::DefChildProc);
      else
        Parent->DispatchAMessage(
             ID_FIRST + GetWindowWord(HWindow, GWW_ID), Msg, &TWindowsObject::DefChildProc);
    }
  }
}


static void DoEnableAutoCreate(void *P, void *)
{
  if ( ((PTWindowsObject)P)->HWindow )
    ((PTWindowsObject)P)->EnableAutoCreate();
}

/* Destroys an MS-Windows element associated with the TWindowsObject
   after setting the WB_AUTOCREATE flag to ON for each of the windows
   in the TWindowsObject's ChildList. */
void TWindowsObject::Destroy()
{
  if ( HWindow )
  {
    ForEach(DoEnableAutoCreate, 0);
    if ( IsFlagSet(WB_MDICHILD) && (Parent->GetClient() != NULL) )
      SendMessage(((PTWindowsObject)(Parent->GetClient()))->HWindow,
		WM_MDIDESTROY, (WORD)HWindow, 0);
    else
      DestroyWindow(HWindow);
  }
}

/* Initializes the passed parameter with the registration attributes for
  the TWindowsObject. This function serves as a placeholder for
  descendant classes to redefine to specify registration
  attributes for the MS-Windows class of a window object. */
void TWindowsObject::GetWindowClass(WNDCLASS&)
{
}

/* Performs setup following creation of an associated MS-Windows window.
  Iterates through the TWindowsObject's ChildList, attempting to create
  an associated MS-Windows interface element for each child window
  object in the list. (A child's Create method is not called if its
  WB_AUTOCREATE flag is not set). Calls TransferData to transfer data
  for its children for whom data transfer is enabled.  Can be redefined
  in derived classes to perform additional special initialization.
*/
void TWindowsObject::SetupWindow()
{
  if ( ! CreateChildren() )
    Status = EM_INVALIDCHILD;
  else
    TransferData(TF_SETDATA);
}

static void _FAR * DataPtr;

static void TransferDataChild(void *AChild, void *Direction)
{
    if ( ((PTWindowsObject)AChild)->IsFlagSet(WB_TRANSFER) )
    {
      *(LPSTR _FAR *)&DataPtr += ((PTWindowsObject)AChild)->Transfer(
                     DataPtr, *(WORD *)Direction);
    }
}

/* Transfers data between the TWindowsObject's data buffer and the child
   windows in its ChildList. (Data is not transfered between any child
   windows whose WB_TRANSFER flag is not set). */
void TWindowsObject::TransferData(WORD Direction)
{
  if ( TransferBuffer )
  {
    DataPtr = TransferBuffer;
    ForEach(TransferDataChild, &Direction);
  }
}

/* Registers the TWindowsObject's MS-Windows, if not already registered. */
BOOL TWindowsObject::Register()
{
  WNDCLASS WindowClass;

  if ( !GetClassInfo( 0, GetClassName(), &WindowClass) &&
       !GetClassInfo(GetModule()->hInstance, GetClassName(),
        &WindowClass) )
  {
    GetWindowClass(WindowClass);
    return RegisterClass(&WindowClass);
  }
    return TRUE;
}

/* Displays the TWindowsObject, after checking that it has a valid
   (non-NULL) handle. */
void TWindowsObject::Show(int ShowCmd)
{
  if ( HWindow )
    ShowWindow(HWindow, ShowCmd);
}

/* Sets the Title and caption of the TWindowsObject. */
void TWindowsObject::SetCaption(LPSTR ATitle)
{
    if (Title != ATitle)
    {
        if ( HIWORD(Title) )
            farfree(Title);

        Title = _fstrdup(ATitle ? ATitle : "");
    }
    if ( HWindow )
        SetWindowText(HWindow, Title);
}

static BOOL CannotCloseChild(void *P, void *)
{
  return ( ((PTWindowsObject)P)->HWindow && !( ((PTWindowsObject)P)->CanClose()) );
}


/* Returns a BOOL indicating whether or not it is Ok to close
  the TWindowsObject.  Iterates through the TWindowsObject's
  ChildList, calling the CanClose method of each.  Returns False if
  any of the child windows return False. */
BOOL TWindowsObject::CanClose()
{
  return (FirstThat(CannotCloseChild, 0) == NULL);
}

/* Destroys the associated MS-Windows interface element and frees "this"
  after determining that it is Ok to do so. If the TWindowsObject is the
  main window of the application, calls the CanClose method of the
  application, else calls this->CanClose, before calling ShutDownWindow. */
void TWindowsObject::CloseWindow()
{
  BOOL WillClose;

  if ( GetApplication() && this == GetApplication()->MainWindow )
    WillClose = GetApplication()->CanClose();
  else
    WillClose = CanClose();
  if ( WillClose )
    ShutDownWindow();
}

/* Destroys the associated MS-Windows interface element and frees "this"
   without calling CanClose. */
void TWindowsObject::ShutDownWindow()
{
  Destroy();
  delete this;
}

/* The default response to a WMClose message is to send a CloseWindow
  message.  CloseWindow sends a CanClose to determine if the window
  can be closed. */
void TWindowsObject::WMClose(TMessage& Msg)
{
  if ( IsFlagSet(WB_ALIAS) )
    DefWndProc(Msg);
  else
    CloseWindow();
}

/* Responds to an incoming WM_DESTROY message.  If the TWindowsObject
   is the application's main window posts a 'quit' message to end
   the application. */
void TWindowsObject::WMDestroy(TMessage& Msg)
{
  if ( !IsFlagSet(WB_ALIAS) )
  {
      PTApplication app = GetApplication();
      if (app && (this == app->MainWindow))
          PostQuitMessage(app->Status);
  }
  DefWndProc(Msg);
}

/* Responds to an incoming WM_NCDESTROY message, the last message
   sent to an MS-Windows interface element.  Sets the HWindow data
   member of the TWindowsObject to zero to indicate that an interface
   element is no longer associated with the object.  */
void TWindowsObject::WMNCDestroy(TMessage& Msg)
{
  DefWndProc(Msg);
  HWindow = 0;
  if ( GetApplication() && this == GetApplication()->KBHandlerWnd )
    GetApplication()->SetKBHandler(NULL);
  if ( IsFlagSet(WB_ALIAS) )
    delete this;
}

/* Responds to an incoming WM_ACTIVATE message by calling
   DefWndProc followed by ActivationResponse. DefWndProc must be
   called first since TWindow::ActivationResponse does a SetFocus
   which would otherwise be overridden when DefWindowProc does its
   SetFocus. */
void TWindowsObject::WMActivate(TMessage& Msg)
{
  DefWndProc(Msg);
  if ( !IsFlagSet(WB_ALIAS) )
    ActivationResponse(Msg.WParam, Msg.LP.Hi);
}

/* Responds to an incoming WM_ACTIVATE or WM_MDIACTIVATE message. If the
   TWindowsObject is being activated and if it has requested
   keyboard handling for its messages, enables the "keyboard handler"
   by calling the SetKBHandler function of the application. Otherwise
   disables the "keyboard handler". */
void TWindowsObject::ActivationResponse(WORD Activated, BOOL /*IsIconified*/ )
{
  if ( GetApplication() )
  {
    if ( Activated && IsFlagSet(WB_KBHANDLER) )
      GetApplication()->SetKBHandler(this);
    else
      GetApplication()->SetKBHandler(NULL);
  }
}

/* Respond to Windows attempt to close close down. */
void TWindowsObject::WMQueryEndSession(TMessage& Msg)
{
  if ( IsFlagSet(WB_ALIAS) )
    DefWndProc(Msg);
  else
  {
    if ( GetApplication() && this == GetApplication()->MainWindow )
      Msg.Result = (int)GetApplication()->CanClose();
    else
      Msg.Result = (int)CanClose();
  }
}

/* If the window receives an Exit menu choice, it will attempt
  to close down the window. */
void TWindowsObject::CMExit(TMessage& Msg)
{
  if ( GetApplication() && this == GetApplication()->MainWindow )
    CloseWindow();
  else
    DefCommandProc(Msg);
}

/* Returns a NULL pointer to indicate that the TWindowsObject is not
   a TMDIFrame.  Is redefined for TMDIFrame to return a pointer to
   their TMDIClient window. */
TMDIClient *TWindowsObject::GetClient()
{
  return NULL;
}
