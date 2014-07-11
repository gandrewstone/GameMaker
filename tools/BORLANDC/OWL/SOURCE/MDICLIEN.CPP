// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  MDICLIEN.CPP
  Defines type TMDIClient.  This defines the basic behavior
  of all MDI client windows.
  -------------------------------------------------------- */

#include <alloc.h>
#include "mdi.h"

/* Constructor for a TMDIClient.  Initializes its data fields using
   passed parameter and default values. If this is not done, the default
   size of the window would be zero.  Allocates space for the
   CLIENTCREATESTRUCT on the heap and sets ClientAttr to point to this
   space. */
TMDIClient::TMDIClient(TMDIFrame *AParent, PTModule AModule)
           : TWindow(AParent, NULL, AModule)
{
  Attr.Id = ID_MDICLIENT;
  Attr.Style = WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | WS_CLIPCHILDREN;
  Attr.X = CW_USEDEFAULT;    Attr.Y = 0;
  Attr.W = CW_USEDEFAULT;    Attr.H = 0;

  ClientAttr = (LPCLIENTCREATESTRUCT)farmalloc(sizeof(CLIENTCREATESTRUCT));
  ClientAttr->hWindowMenu = (HMENU)0;
  ClientAttr->idFirstChild = ID_FIRSTMDICHILD;
  Attr.Param = (LPSTR)ClientAttr;
  SetFlags(WB_MDICHILD, FALSE);
}

/* Constructor for a TMDIClient which is being used in a DLL as an alias
   for a non-OWL window */
TMDIClient::TMDIClient(PTMDIFrame AParent, HWND AnHWindow, PTModule AModule)
           : TWindow(AnHWindow, AModule)
{
  Parent = AParent;
  SetFlags(WB_MDICHILD, FALSE);
}

/* Frees the memory associated with ClientAttr. */
TMDIClient::~TMDIClient()
{
	if (ClientAttr)
		farfree(ClientAttr);
}

/* Reads an instance of TMDIClient from the passed ipstream. */
void *TMDIClient::read(ipstream& is)
{
  TWindow::read(is);
  ClientAttr = (LPCLIENTCREATESTRUCT)farmalloc(sizeof(CLIENTCREATESTRUCT));

  WORD idFirstChild = ClientAttr->idFirstChild; // prevent compiler warning
  is >> idFirstChild;
  ClientAttr->hWindowMenu = (HMENU)0;
  Attr.Param = (LPSTR)ClientAttr;
  return this;
}

/* Writes the TMDIClient to the passed opstream. */
void TMDIClient::write(opstream& os)
{
  TWindow::write(os);
  os << ClientAttr->idFirstChild;
}

TStreamable *TMDIClient::build()
{
  return new TMDIClient(streamableInit);
}

TStreamableClass RegMDIClient("TMDIClient", TMDIClient::build,
					  __DELTA(TMDIClient));
