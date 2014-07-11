// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  FILEDIAL.CPP
  Defines type TFileDialog.  This defines the basic
  behavior of all file dialogs.
  -------------------------------------------------------- */

#include "filedial.h"
#include <string.h>
#include <dir.h>

LPSTR GetFileName(LPSTR FilePath)
{
  LPSTR P;

  P = _fstrrchr(FilePath, '\\');
  if ( !P )
    P = _fstrrchr(FilePath, ':');
  if ( !P )
    return FilePath;
  else
    return P + 1;
}

LPSTR GetExtension(LPSTR FilePath)
{
  LPSTR P;

  P = _fstrchr( GetFileName(FilePath), '.');
  if ( !P )
    return _fstrchr(FilePath, '\0');
  else
    return P;
}

BOOL HasWildCards(LPSTR FilePath)
{
  return _fstrchr(FilePath, '\*') || _fstrchr(FilePath, '\?');
}

/* Constructor for a TFileDialog. Uses the contents of AFilePath as
   the initial mask (for example, "*.*") for the files to be listed
   in the file list box. AFilePath is also used as a buffer in which
   the name of the file retrieved from the user is returned.  The
   resource identifier is set to SD_FILESAVE or SD_FILEOPEN, which
   correspond to dialog resources in the OWL-supplied filedial.dlg
   file.  If the resource identifier is reset a matching resource
   with similar controls must be supplied. */
TFileDialog::TFileDialog(PTWindowsObject AParent,
                         int ResourceId, LPSTR AFilePath, PTModule AModule)
               : TDialog(AParent, ResourceId, AModule)
{
  FilePath = AFilePath;
  Extension[0] = '\0';
}

/* Returns TRUE if a valid file name has been retrieved from the user to
   indicate that the file dialog can then be closed.  Retrieves the text
   of the edit control, and updates PathName.  Calls UpdateListBoxes;
   returns FALSE if UpdateListBoxes returns TRUE.  If the edit control
   contains an invalid file name, also returns FALSE. */
BOOL TFileDialog::CanClose()
{
  WORD PathLen;
  OFSTRUCT AnOfstruct;

  GetDlgItemText(HWindow, ID_FNAME, PathName, MAXPATH);
  if ( lstrcmpi(&PathName[_fstrlen(PathName) - 2], "..") == 0 )
    _fstrcat(PathName, "\\");  // otherwise OpenFile messes up
  if ( OpenFile(PathName, &AnOfstruct, OF_PARSE) == -1 )
  {
    MessageBeep(0);
    SelectFileName();
    return FALSE;
  }
  // Note that szPathName initially uses OEM char set.
  OemToAnsi((LPSTR)(AnOfstruct.szPathName), (LPSTR)(AnOfstruct.szPathName));
  _fstrncpy(PathName, (LPSTR)(AnOfstruct.szPathName), MAXPATH);
  PathName[MAXPATH-1] = '\0';
  PathLen = _fstrlen(PathName);
  if ( PathName[PathLen - 1] == '\\' ||
       HasWildCards(PathName) ||
       GetFocus() == GetDlgItem(HWindow, ID_DLIST) )
  {
    if ( PathName[PathLen - 1] == '\\' )
      _fstrncat(PathName, FileSpec, (MAXPATH-1) - _fstrlen(PathName));
    if ( !UpdateListBoxes() )
    {
      MessageBeep(0);
      SelectFileName();
    }
    return FALSE;
  }
  _fstrncat(PathName, "\\", (MAXPATH-1) - _fstrlen(PathName));
  _fstrncat(PathName, FileSpec, (MAXPATH-1) - _fstrlen(PathName));

  if ( UpdateListBoxes() )
    return FALSE;
  PathName[PathLen] = '\0';
  if ( GetExtension(PathName)[0] == '\0' )
  {
    _fstrncat(PathName, Extension, (MAXPATH-1) - _fstrlen(PathName));
    PathName[MAXPATH-1] = '\0';
  }
  AnsiLower(_fstrcpy(FilePath, PathName));
  return TRUE;
}

/* Sets up the file dialog.  Limits the number of characters which
   can be entered into the edit control to MAXPATH - 1.  Then calls
   UpdateListBoxes and SelectFileName. */
void TFileDialog::SetupWindow()
{
  TDialog::SetupWindow();
  SendDlgItemMessage(HWindow, ID_FNAME, EM_LIMITTEXT, MAXPATH-1, 0);
  _fstrncpy(PathName, FilePath, MAXPATH);
  PathName[MAXPATH-1] = '\0';

  if ( !UpdateListBoxes() )
  {
    _fstrcpy(PathName, "*.*");
    UpdateListBoxes();
  }
  SelectFileName();
}

/* Responds to messages from the edit control.  Enables the OK button
   if the edit control contains text. */
void TFileDialog::HandleFName(TMessage& Msg)
{
  if ( HIWORD(Msg.LParam) == EN_CHANGE )
    EnableWindow(GetDlgItem(HWindow, IDOK),
      SendMessage((HWND)LOWORD(Msg.LParam), WM_GETTEXTLENGTH, 0, 0) != 0 );
}

/* Responds to messages from the file list box.  Updates PathName with
   the name of the selected file and calls UpdateFileName, when the
   selection in the list box changes.  Attempts to close the dialog if
   the selected entry was double-clicked.  Clears the selection in the
   list box when it loses the focus. */
void TFileDialog::HandleFList(TMessage& Msg)
{
  switch (HIWORD(Msg.LParam)) {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
         DlgDirSelect(HWindow, PathName, ID_FLIST);
         UpdateFileName();
         if ( HIWORD(Msg.LParam) == LBN_DBLCLK )
           CloseWindow(IDOK);
         break;
    case LBN_KILLFOCUS:
	 SendMessage((HWND)LOWORD(Msg.LParam), LB_SETCURSEL, (WORD)-1, 0);
         break;
  }
}

/* Responds to messages from the directory list box.  Updates PathName
   when the selection in the list box changes.  Calls UpdateListBoxes if
   the selected entry was double-clicked, else calls UpdateFileName.
   Clears the selection in the list box when it loses the focus.  */
void TFileDialog::HandleDList(TMessage& Msg)
{
  switch (HIWORD(Msg.LParam)) {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
         DlgDirSelect(HWindow, PathName, ID_DLIST);
         _fstrcat(PathName, FileSpec);
         if ( HIWORD(Msg.LParam) == LBN_DBLCLK )
           UpdateListBoxes();
         else
           UpdateFileName();
         break;
    case LBN_KILLFOCUS:
	 SendMessage((HWND)LOWORD(Msg.LParam), LB_SETCURSEL, (WORD)-1, 0);
         break;
  }
}

/* Selects the text in the edit control, and sets the focus to the
   edit control. */
void TFileDialog::SelectFileName()
{
  SendDlgItemMessage(HWindow, ID_FNAME, EM_SETSEL, 0, 0x7FFF000L);
  SetFocus(GetDlgItem(HWindow, ID_FNAME));
}

/* Sets the text of the edit control to PathName, and selects the
   text. */
void TFileDialog::UpdateFileName()
{
  SetDlgItemText(HWindow, ID_FNAME, AnsiLower(PathName));
  SendDlgItemMessage(HWindow, ID_FNAME, EM_SETSEL, 0, 0x7FFF0000L);
}

/* Attempts to update the file and/or directory list boxes.  If updated,
   calls UpdateFileName and returns TRUE.  Else returns FALSE. */
BOOL TFileDialog::UpdateListBoxes()
{
  int Result;
  char Path[MAXPATH];

  Path[0] = '\0';
  if ( GetDlgItem(HWindow, ID_FLIST) )
  {
    _fstrcpy(Path, PathName);
    Result = DlgDirList(HWindow, Path, ID_FLIST, ID_FPATH, 0);
    if ( Result )
      DlgDirList(HWindow, "*.*", ID_DLIST, 0, 0xC010);
  }
  else
  {
      _fstrncpy(Path, PathName, MAXPATH);
      Path[MAXPATH-1] = '\0';
      _fstrncat(Path, "*.*", (MAXPATH-1) - strlen(Path));
      Result = DlgDirList(HWindow, Path, ID_DLIST, ID_FPATH, 0xC010);
  }
  if ( Result )
  {
    _fstrncpy(FileSpec, GetFileName(PathName), FILESPEC);
    FileSpec[FILESPEC-1] = '\0';
    _fstrcpy(PathName, FileSpec);
    UpdateFileName();
    return TRUE;
  }
  return FALSE;
}

TStreamable *TFileDialog::build()
{
  return new TFileDialog(streamableInit);
}

TStreamableClass RegFileDialog("TFileDialog", TFileDialog::build,
					    __DELTA(TFileDialog));
