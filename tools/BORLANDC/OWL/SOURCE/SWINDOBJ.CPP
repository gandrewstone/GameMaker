// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  SWINDOBJ.CPP
  Streamable object implementation for TWindowsObject.
  -------------------------------------------------------- */

// For asm in GetChildren:
#pragma inline

#include "applicat.h"
#include "windobj.h"

extern WNDPROC MakeObjectInstance(PTWindowsObject);

typedef char far *TFixupList[4096];
static TFixupList far *FixupList = NULL;

/* Reads the children of "this" which were stored on the given stream
   using PutChildren.  Used by TWindowsObject::read.  IMPORTANT: This
   method assumes that the current child list is empty! */
void TWindowsObject::GetChildren(ipstream& is)
{
  int ChildCount, I;
  TFixupList far *SaveFixup;
  TWindowsObject *W, *TmpWindowsObject;
  char * far *P;
  char * far *Q;

  SaveFixup = FixupList;
  is >> ChildCount;
  asm {
    	MOV     CX,ChildCount
	SHL     CX,1
	SHL     CX,1
	SUB     SP,CX
  }

#if defined(_ALLOW_po)	  // probably compiled with -po
    #if defined(__DLL__)    // and compiled with -WD (for a DLL)
	asm {
	    MOV AX, DGROUP
	    MOV ES, AX
	    MOV word ptr ES:FixupList, SP
	    MOV word ptr ES:FixupList+2, SS
	}
    #else   // compiled as a static library (and probably with -po)
	asm MOV word ptr SS:FixupList, SP
	asm MOV word ptr SS:FixupList+2, SS
    #endif
#else	// !defined(_ALLOW_po)
  asm	MOV word ptr DGROUP:FixupList, SP
  asm	MOV word ptr DGROUP:FixupList+2, SS
#endif	// defined(_ALLOW_po)

  asm {
	MOV     DI,SP
	PUSH    SS
	POP     ES
	XOR     AL,AL
	CLD
	REP     STOSB
  };
  for (I = 1; I <= ChildCount; ++I)
  {
    is >> TmpWindowsObject;
    AddChild(TmpWindowsObject);
    ChildList->Parent = this;
    ChildList->Application = Application;
  }
  W = (PTWindowsObject)ChildList;
  for (I = 0; I < ChildCount; ++I)
  {
    W = W->Next();
    P = (char * far *)((*FixupList)[I]);
    while ( P )
    {
      Q = P;
      if ( *P  == 0 )
        P = NULL;
      else
        P = (char * far *)*P;
      *Q = (char *)W;
    }
  }
  FixupList = SaveFixup;

  // get rid of temporary buffer on the stack
  asm {
  MOV     CX,ChildCount
  SHL     CX,1
  SHL     CX,1
  ADD     SP,CX
  };
}

/* Writes a pointer to a child window onto the passed stream */
void TWindowsObject::PutChildPtr(opstream& os, TWindowsObject *P)
{
  int Index = IndexOf(P);   // -1 if P == NULL
  os << Index;
}

/* Reads a pointer to a child window from the passed stream */
void TWindowsObject::GetChildPtr(ipstream& is, TWindowsObject *&P)
{
  int Index;

  is >> Index;
  P = At(Index);            // NULL if Index == -1
}

/* Gets a pointer to a sibling window from the passed stream.  This
   function is only valid during a read and is not valid until
   the read returns.  The pointer will not be given a valid value
   until the parent window's read  loads all of the window's
   sibling windows. */
void TWindowsObject::GetSiblingPtr(ipstream& is, TWindowsObject *&P)
{
  int Index;

  is >> Index;
  if ( (Index == -1) || (FixupList == NULL) )
    P = NULL;
  else
  {
    P = (TWindowsObject *)(*FixupList)[Index];
    (*FixupList)[Index] = (char far *)&P;
  }
}

/* Puts a pointer to a sibling window on to a stream.  The pointer can
   be read from the stream using GetSiblingPtr.  This function is only
   valid during a write procedure. */
void TWindowsObject::PutSiblingPtr(opstream& os, TWindowsObject *P)
{
  int Index = Parent->IndexOf(P);
  os << Index;
}

static void DoPutChildren(void *P, void *os)
{
  *(opstream *)os
        << (TWindowsObject *)P;
}

void TWindowsObject::PutChildren(opstream& os)
{
  int ChildCount;

  AssignCreateOrder();
  ChildCount = IndexOf(ChildList) + 1;
  os << ChildCount;
  ForEach(DoPutChildren, &os);
}

void *TWindowsObject::read(ipstream& is)
{
  BOOL TitleIsNumeric;

  HWindow = 0;
  Parent = NULL;
  SiblingList = NULL;
  ChildList = NULL;
  TransferBuffer = NULL;
  DefaultProc = NULL;

  Application = GetApplicationObject();
  /* For now, set Module to Application. This is not a general solution */
  Module = (PTModule)Application;

  Instance = MakeObjectInstance((PTWindowsObject)this);

  is >> TitleIsNumeric;
  // if TitleIsNumeric is TRUE then it's probably a dialog with Title == -1
  // i.e. unchanged from resource
  if ( TitleIsNumeric )
    is >> (long)(Title);
  else
    Title = is.freadString();

  is >> Status >> Flags >> CreateOrder;
  GetChildren(is);
  return this;
}

void TWindowsObject::write(opstream& os)
{
  WORD SavedFlags;
  BOOL TitleIsNumeric;

  TitleIsNumeric = HIWORD(Title) == NULL;
  os << TitleIsNumeric;
  // if TitleIsNumeric is TRUE then it's probably a dialog with Title == -1
  // i.e. unchanged from resource
  if ( TitleIsNumeric )
    os << (long)(Title);
  else
    os.fwriteString(Title);

  SavedFlags = Flags;
  if ( HWindow )
    SavedFlags |= WB_AUTOCREATE;

  os << Status << SavedFlags << CreateOrder;
  PutChildren(os);
}

TStreamable * TWindowsObject::build()
{
  return NULL;
}

TStreamableClass RegWindowsObject("TWindowsObject", TWindowsObject::build,
				__DELTA(TWindowsObject));
