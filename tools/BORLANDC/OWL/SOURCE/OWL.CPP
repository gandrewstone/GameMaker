// ObjectWindows - (C) Copyright 1992 by Borland International

//
// OWL.CPP
//

#include <dos.h>
#include <mem.h>
#include "owl.h"

// AllocCSToDSAlias was not declared in windows.h

extern "C" WORD FAR PASCAL AllocCSToDSAlias(WORD);

long FAR PASCAL _export StdWndProc(HWND HWindow, WORD Message, WORD WParam,
				   LONG LParam);

typedef struct TObjectInstance {
    char Code;
    WORD  Offset;
    union Ptr {
	struct TObjectInstance FAR *Next;
	Object FAR *ObjectPtr;
    }ptr;
}FAR *PObjectInstance;

typedef struct TInstanceBlock {
    WORD Next;
    char Code[5];
    void FAR *WndProcPtr;
    struct TObjectInstance Instances[34];
}FAR *PInstanceBlock;


WORD InstBlockList = 0;
PObjectInstance InstFreeList = NULL;
FARPROC StdWndProcInstance = NULL;

//Creation window pointers for InitWndProc
PTWindowsObject CreationWindow = NULL;
PTWindowsObject DlgCreationWindow = NULL;

// Return pointer to TWindowsObject given a window handle

PTWindowsObject GetObjectPtr(HWND HWindow)
{
  PObjectInstance AnInstance;

  if ( IsWindow(HWindow) )
  {
    AnInstance = (PObjectInstance)GetWindowLong(HWindow, GWL_WNDPROC);
    if ((BYTE)(AnInstance->Code) == 0xE8)
      if (AnInstance->Offset == (2-3) - FP_OFF(AnInstance) )
        if ( *(WORD FAR *)MK_FP(FP_SEG(AnInstance),2)  == 0x2E5B )
          return (PTWindowsObject)(AnInstance->ptr.ObjectPtr);

    if ( (FARPROC)GetClassLong(HWindow, GCL_WNDPROC) ==
         (FARPROC)DefDlgProc )
    {  // If it's a dialog get Instance from where it's stored.
      AnInstance = (PObjectInstance)GetWindowLong(HWindow, 4);
      if ( AnInstance )
        if ((BYTE)(AnInstance->Code) == 0xE8)
          if (AnInstance->Offset == (2-3) - FP_OFF(AnInstance) )
            if ( *(WORD FAR *)MK_FP(FP_SEG(AnInstance),2)  == 0x2E5B )
              return (PTWindowsObject)(AnInstance->ptr.ObjectPtr);
    }
  }
  return NULL;
}

long FAR PASCAL _export StdWndProc(HWND HWindow, WORD Message, WORD WParam,
				   LONG LParam)
{
    PTWindowsObject myself;
    TMessage Msg;

    Msg.Receiver = HWindow;  Msg.Message = Message;
    Msg.WParam = WParam;     Msg.LParam = LParam;
    Msg.Result = 1L;
    myself = (PTWindowsObject)MK_FP(_ES,_BX);  // ES:BX set from the thunk
    myself->HWindow = HWindow;
    if ( Message >= 0x8000 )
      myself->DefWndProc(Msg);
    else
      myself->DispatchAMessage(Message, Msg, &TWindowsObject::DefWndProc);
  return Msg.Result;
}

// Implementation of Initialization window procedure.
inline LRESULT __InitWndProc(
             HWND HWindow, UINT Message, WPARAM WParam, LPARAM LParam
		     )
{
  PTWindowsObject myself;

  if ( DlgCreationWindow )
  {      // DlgCreationWindow is my parent dialog, I'm being created by it
    WORD MyId = GetWindowWord(HWindow, GWW_ID);
    myself = DlgCreationWindow->ChildWithId(MyId);
  }
  else
  {
     // I'm being created by TWindow::Create()
    myself = CreationWindow;
  }

  SetWindowLong(HWindow, GWL_WNDPROC, (long)(myself->GetInstance()));
  return (* (long (FAR PASCAL *)(HWND, WORD, WORD, DWORD))
            (myself->GetInstance()))(HWindow, Message, WParam, LParam);
}

// Win31 Initialization window procedure.
#ifdef WIN31
LRESULT FAR PASCAL _export
  InitWndProc(HWND HWindow, UINT Message, WPARAM WParam, LPARAM LParam)
{
  return __InitWndProc(HWindow, Message, WParam, LParam);
}
#endif

// Win30 Initialization window procedure.
#ifdef WIN30
LONG FAR PASCAL _export
  InitWndProc(HWND_30 HWindow, WORD Message, WORD WParam, LONG LParam)
{
  return __InitWndProc(
	     HWND(HWindow), UINT(Message), WPARAM(WParam), LPARAM(LParam));
}
#endif


WNDPROC MakeObjectInstance(PTWindowsObject P)
{
    char BlockCode[5] ;
    PInstanceBlock Block;
    PObjectInstance Instance;
    WNDPROC ObjInstance;

    // POP BX
    BlockCode[0]=0x5b;
    // LES BX, CS:[BX]
    BlockCode[1]=0x2e;
    BlockCode[2]=0xc4;
    BlockCode[3]=0x1f;
    // JMP FAR StdWndProc
    BlockCode[4]=0xea;

    if (InstFreeList == NULL)
    {
        Block = (PInstanceBlock)GlobalLock(GlobalAlloc(
                 GMEM_FIXED | GMEM_DDESHARE | GMEM_NOT_BANKED, sizeof(TInstanceBlock)));
        Block->Next = InstBlockList;
	_fmemcpy(Block->Code, BlockCode, 5);
        Block->WndProcPtr = StdWndProcInstance;
        Instance = Block->Instances;
        do {
            Instance->Code = 0xE8;
            Instance->Offset = (2 - 3) - FP_OFF(Instance);
      	    Instance->ptr.Next = InstFreeList;
      	    InstFreeList = Instance;
	    Instance = (PObjectInstance)MK_FP(FP_SEG(Instance),
					      FP_OFF(Instance)+sizeof(TObjectInstance));
	} while (FP_OFF(Instance) != sizeof(TInstanceBlock));
        InstBlockList = FP_SEG(Block);
        PrestoChangoSelector(FP_SEG(Block), FP_SEG(Block));
    }
    ObjInstance = (WNDPROC)InstFreeList;
    Instance = (PObjectInstance)MK_FP(
		    AllocCSToDSAlias(FP_SEG(InstFreeList)),
		    FP_OFF(InstFreeList));
    InstFreeList = Instance->ptr.Next;
    Instance->ptr.ObjectPtr = (Object FAR *)P;
    FreeSelector(FP_SEG(Instance));
    return (ObjInstance);
}

void FreeObjectInstance(WNDPROC P)
{
    PObjectInstance Instance;

    Instance = (PObjectInstance) MK_FP(
		    AllocCSToDSAlias(FP_SEG(P)),
		    FP_OFF(P) );

    Instance->ptr.Next = InstFreeList;
    FreeSelector(FP_SEG(Instance));
    InstFreeList = (PObjectInstance)P;
}

unsigned short far _EXPFUNC OWLGetVersion()
{
    return OWLVersion;
}
