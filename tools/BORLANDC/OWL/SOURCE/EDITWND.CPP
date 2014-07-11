// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  EDITWND.CPP
  Defines type TEditWindow, a text editor (cannot perform
  file I/O)).
  -------------------------------------------------------- */

#include "editwnd.h"

__link(RegEdit)

TSearchDialog::TSearchDialog(PTWindowsObject AParent,
               int ResourceId, TSearchStruct &SearchStruct, PTModule AModule)
               : TDialog(AParent, ResourceId, AModule)
{
  new TEdit(this, ID_SEARCHTEXT, sizeof SearchStruct.SearchText,
            GetModule());
  new TCheckBox(this, ID_CASESENSITIVE, (PTGroupBox)NULL, GetModule());
  if ( ResourceId == SD_REPLACE )
  {
    new TEdit(this, ID_REPLACETEXT, sizeof SearchStruct.ReplaceText,
              GetModule());
    new TCheckBox(this, ID_REPLACEALL, (PTGroupBox)NULL, GetModule());
    new TCheckBox(this, ID_PROMPTONREPLACE, (PTGroupBox)NULL, GetModule());
  }
  TransferBuffer = &SearchStruct;
}

/* Constructor for a TEditWindow.  Initializes its data fields using
   passed parameters and default values.  Constructs its child edit
   control. */
TEditWindow::TEditWindow(PTWindowsObject AParent, LPSTR ATitle,
                         PTModule AModule)
            : TWindow(AParent, ATitle, AModule)
{
  Editor = new TEdit(this, ID_EDITOR, NULL, 0, 0, 0, 0, 0, TRUE, GetModule());
  Editor->Attr.Style |= ES_NOHIDESEL;
  IsReplaceOp = FALSE;
  memset(&SearchStruct, 0x0, sizeof SearchStruct);
}

/* Responds to an incoming WM_SIZE message by resizing the child edit
  control according to the size of the TEditWindow's client area. */
void TEditWindow::WMSize(RTMessage Msg)
{
  TWindow::WMSize(Msg);
  SetWindowPos(Editor->HWindow, 0, -1, -1, LOWORD(Msg.LParam)+2,
    HIWORD(Msg.LParam)+2, SWP_NOZORDER);
}

/* Responds to an incoming WM_SETFOCUS message by setting the focus to
   the child edit control. */
void TEditWindow::WMSetFocus(RTMessage)
{
  SetFocus(Editor->HWindow);
}

void TEditWindow::DoSearch()
{
  char S[81];
  LPSTR P;
  int Rslt = 0;
  BOOL TextFoundSinceUserInput = FALSE;

  do {
    Rslt = Editor->Search(-1, SearchStruct.SearchText,
                                   SearchStruct.CaseSensitive);
    if ( Rslt == -1 )
    {
      if ( !IsReplaceOp || !SearchStruct.ReplaceAll ||
           !TextFoundSinceUserInput )
      {
        P = SearchStruct.SearchText;
        wsprintf(S, "\"%0.60s\" not found.", P);
        MessageBox(HWindow, S, "Find error",
                                    MB_OK | MB_ICONEXCLAMATION);
      }
    }
    else
      if ( IsReplaceOp )
      {
        if ( !SearchStruct.PromptOnReplace )
        {
          TextFoundSinceUserInput = TRUE;
          Editor->Insert(SearchStruct.ReplaceText);
        }
        else
        {
          Rslt = MessageBox(HWindow, "Replace this occurrence?",
                    "Search/Replace", MB_YESNOCANCEL | MB_ICONQUESTION);
          if ( Rslt == IDYES )
            Editor->Insert(SearchStruct.ReplaceText);
          else
            if ( Rslt == IDCANCEL )
              return;
          TextFoundSinceUserInput = FALSE;
        }
      }
  }
  while ( (Rslt != -1) && SearchStruct.ReplaceAll && IsReplaceOp );
}

/*  */
void TEditWindow::CMEditFind(RTMessage)
{
  if (GetModule()->ExecDialog( new TSearchDialog(
        this, SD_SEARCH, SearchStruct, GetModule())) == IDOK )
  {
    IsReplaceOp = FALSE;
    DoSearch();
  }
}

/*  */
void TEditWindow::CMEditFindNext(RTMessage)
{
  DoSearch();
}

/*  */
void TEditWindow::CMEditReplace(RTMessage)
{
  if (GetModule()->ExecDialog( new TSearchDialog(
        this, SD_REPLACE, SearchStruct, GetModule())) == IDOK )
  {
    IsReplaceOp = TRUE;
    DoSearch();
  }
}

/* Reads an instance of TEditWindow from the passed ipstream. */
void *TEditWindow::read(ipstream& is)
{
  TWindow::read(is);

  GetChildPtr(is, (PTWindowsObject)Editor);

  is.readBytes(SearchStruct.SearchText, 81);
  is >> SearchStruct.CaseSensitive;
  is.readBytes(SearchStruct.ReplaceText, 81);
  is >> SearchStruct.ReplaceAll >> SearchStruct.PromptOnReplace;

  is >> IsReplaceOp;
  return this;
}

/* Writes the TEditWindow to the passed opstream. */
void TEditWindow::write(opstream& os)
{
  TWindow::write(os);

  PutChildPtr(os, Editor);

  os.writeBytes(SearchStruct.SearchText, 81);
  os << SearchStruct.CaseSensitive;
  os.writeBytes(SearchStruct.ReplaceText, 81);
  os << SearchStruct.ReplaceAll << SearchStruct.PromptOnReplace;

  os << IsReplaceOp;
}

TStreamable *TEditWindow::build()
{
  return new TEditWindow(streamableInit);
}

TStreamableClass RegEditWindow("TEditWindow",
			     TEditWindow::build,
		             __DELTA(TEditWindow));
