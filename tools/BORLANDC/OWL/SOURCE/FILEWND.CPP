// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  FILEWND.CPP
  Defines type TFileWindow, a text editor which can read
  and write to a file.
  -------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
#include <alloc.h>
#include "filewnd.h"

#define Min(a, b)     ((a) < (b) ? (a) : (b))

/* Constructor for a TFileWindow.  Initializes its data members using
  passed parameters and default values. */
TFileWindow::TFileWindow(PTWindowsObject AParent,LPSTR ATitle,
                         LPSTR AFileName, PTModule AModule)
             :   TEditWindow(AParent, ATitle, AModule)
{
  IsNewFile = TRUE;
  FileName = _fstrdup(AFileName ? AFileName : "");
}

/* Dispose of the file name. */
TFileWindow::~TFileWindow()
{
  if ( FileName )
    farfree(FileName);
}

/* Performs setup for a TFileWindow, appending 'Untitled' to its caption */
void TFileWindow::SetupWindow()
{
  TEditWindow::SetupWindow();
  SetFileName(FileName);
  if ( FileName[0] != '\0')
    if ( !Read() )
      SetFileName("");
}

/* Sets the file name of the window and updates the caption. */
void TFileWindow::SetFileName(LPSTR AFileName)
{
  char NewCaption[81];
  LPSTR P[2];

  if ( FileName != AFileName )
  {
    farfree(FileName);
    FileName = _fstrdup(AFileName ? AFileName : "");
  }
  P[0] = Title;
  if ( FileName[0] == '\0' )
    P[1] = "(Untitled)";
  else
    P[1] = AFileName;
  if ( Title == NULL || Title[0] == '\0' )
    SetWindowText(HWindow, P[1]);
  else
  {
    wvsprintf(NewCaption, "%s - %s", (LPSTR)P);
    SetWindowText(HWindow, NewCaption);
  }
}

/* Begins the edit of a new file, after determining that it is Ok to
  clear the TEdit's text. */
void TFileWindow::NewFile()
{
  if ( CanClear() )
  {
    Editor->Clear();
    InvalidateRect(Editor->HWindow, NULL, FALSE);
    Editor->ClearModify();
    IsNewFile = TRUE;
    SetFileName(NULL);
  }
}

/* Replaces the current file with the given file. */
void TFileWindow::ReplaceWith(LPSTR AFileName)
{
  char OldName[MAXPATH];

  _fstrcpy(OldName, FileName);
  SetFileName(AFileName);
  if ( Read() )
    InvalidateRect(Editor->HWindow, NULL, FALSE);
  else
    SetFileName(OldName);
}

/* Brings up a dialog allowing the user to open a file into this
  window.  Same as selecting File|Open from the menus. */
void TFileWindow::Open()
{
  char TmpName[MAXPATH];

  if ( CanClear() && (GetModule()->ExecDialog(
         new TFileDialog(this, SD_FILEOPEN,
                         _fstrcpy(TmpName, "*.*"), GetModule())) == IDOK) )
    ReplaceWith(TmpName);
}

static HANDLE LocalReAllocDS(HANDLE hMem, WORD wBytes, WORD wFlags,
                                                                 WORD TheDS)
{
  WORD SavedDS;
  HANDLE ReturnValue;

  SavedDS = _DS;
  _DS = TheDS;
  ReturnValue = LocalReAlloc(hMem, wBytes, wFlags);
  _DS = SavedDS;
  return ReturnValue;
}

static LPSTR LocalLockDS(HANDLE hMem, WORD TheDS)
{
  WORD SavedDS;
  LPSTR ReturnValue;

  SavedDS = _DS;
  _DS = TheDS;
  ReturnValue = (LPSTR)LocalLock(hMem);
  _DS = SavedDS;
  return ReturnValue;
}

static BOOL LocalUnlockDS(HANDLE hMem, WORD TheDS)
{
  WORD SavedDS;
  BOOL ReturnValue;

  SavedDS = _DS;
  _DS = TheDS;
  ReturnValue = LocalUnlock(hMem);
  _DS = SavedDS;
  return ReturnValue;
}

/* Reads the contents of a previously-specified file into the TEdit
  child control. */
BOOL TFileWindow::Read()
{
  long CharsToRead;
  UINT HEditorBuffer;
  LPSTR EditorBuffer;
  WORD EditorsDS;
  char S[MAXPATH + 33];
  int AFile;
  BOOL Success = FALSE;

  AFile = _lopen(FileName, OF_READ);
  if ( AFile != -1 )
  {
    CharsToRead = _llseek(AFile, 0L, 2);
    _llseek(AFile, 0L, 0);
    if ( CharsToRead < MaxInt && CharsToRead > 0 )
    {
      Editor->Clear();

      // attempt to reallocate Editor's buffer to the size of the file
      HEditorBuffer = SendMessage(Editor->HWindow, EM_GETHANDLE, 0, 0L);
      EditorsDS = FP_SEG(GlobalLock((Editor->GetModule())->hInstance));
      if ( LocalReAllocDS((HANDLE)HEditorBuffer, (WORD)(CharsToRead+1),
                          LHND, EditorsDS) != NULL )
      {
        // read the file into EditorBuffer
	EditorBuffer = (LPSTR)LocalLockDS((HANDLE)HEditorBuffer, EditorsDS);
        if ( _lread(AFile, EditorBuffer, (WORD)CharsToRead) == CharsToRead )
        {
          // NULL terminate Editor's buffer
          EditorBuffer[(WORD)CharsToRead] = '\0';
	  LocalUnlockDS((HANDLE)HEditorBuffer, EditorsDS);

	  SendMessage(Editor->HWindow, EM_SETHANDLE, HEditorBuffer, 0L);
          Success = TRUE;

          IsNewFile = FALSE;
          Editor->ClearModify();
          Editor->SetSelection(0,0);
        }
      }
      GlobalUnlock((Editor->GetModule())->hInstance);
    }
    _lclose(AFile);
  }
  if ( !Success )
  {
    wsprintf(S, "Unable to read file \"%s\" from disk", FileName);
    MessageBox(HWindow, S, GetModule()->Name, MB_ICONEXCLAMATION | MB_OK);
  }
  return Success;
}

/* Saves the contents of the TEdit child control into the file currently
  being editted.  Returns true if the file was saved or
  Editor->IsModified returns FALSE (contents already saved). */
BOOL TFileWindow::Save()
{
  if ( Editor->IsModified() )
  {
    if ( IsNewFile )
      return SaveAs();
    else
    {
      if ( Write() )
        return TRUE;
      else
        return FALSE;
    }
  }
  else       // Editor's contents haven't been changed. No need to write.
    return TRUE;
}

/* Saves the contents of the TEdit child control into a file whose name
  is retrieved from the user, through execution of a "Save" file
  dialog. Returns true if the file was saved. */
BOOL TFileWindow::SaveAs()
{
  char TmpName[MAXPATH];
  char OldName[MAXPATH];
  OFSTRUCT TmpOfStruct;
  char S[MAXPATH+20];

  _fstrcpy(OldName, FileName);
  if ( FileName )
    _fstrcpy(TmpName, FileName);
  else
    TmpName[0] = '\0';
  if ( GetModule()->ExecDialog( new TFileDialog
            (this, SD_FILESAVE, TmpName, GetModule())) == IDOK )
  {
    if ( OpenFile(TmpName, &TmpOfStruct, OF_EXIST) != -1 )
    {
      wsprintf(S, "Replace Current \"%s\"?", (LPSTR)TmpName);
      if ( MessageBox(HWindow, S, "File Changed",
                          MB_YESNO | MB_ICONQUESTION) == IDNO )
      {
        SetFileName(OldName);
        return FALSE;
      }
    }
    SetFileName(TmpName);
    if ( Write() )
      return TRUE;
    else
    {
      SetFileName(OldName);
      return FALSE;
    }
  }
  return FALSE;
}

/* Writes the contents of the TEdit child control to a
   previously-specified file. */
BOOL TFileWindow::Write()
{
  const BufferSize = 1024;

  long CharsToWrite, CharsWritten = 0;
  int BlockSize;
  Pchar ABuffer;
  char S[MAXPATH + 33];
  int NumLines;
  int AFile;

  AFile = _lcreat(FileName, 0);
  if ( AFile == -1 )
  {
    wsprintf(S, "Unable to write file \"%s\" to disk", FileName);
    MessageBox(HWindow, S, GetModule()->Name, MB_ICONEXCLAMATION | MB_OK);
    return FALSE;
  }
  else
  {
    NumLines = Editor->GetNumLines();
    CharsToWrite = Editor->GetLineIndex(NumLines-1) +
                   Editor->GetLineLength(NumLines-1);
    ABuffer = new char[BufferSize+1 + 2*NumLines];
    if ( !ABuffer )
      return FALSE;
    else
    {
      while ( CharsWritten < CharsToWrite )
      {
        BlockSize = (int)Min((CharsToWrite - CharsWritten), BufferSize);
        Editor->GetSubText(ABuffer, (int)CharsWritten,
          (int)(CharsWritten + BlockSize));
        if (_lwrite(AFile, ABuffer, strlen(ABuffer)) == (WORD)-1)
        {
	    delete ABuffer;
            _lclose(AFile);
            return FALSE;
        }
        CharsWritten += BlockSize;
      }
      IsNewFile = FALSE;
      Editor->ClearModify();
      delete ABuffer;
    }
    _lclose(AFile);
  }
  return TRUE;
}

/* Returns a BOOL value indicating whether or not it is Ok to clear
  the TEdit's text.  Returns TRUE if the text has not been changed, or
  if the user Oks the clearing of the text. */
BOOL TFileWindow::CanClear()
{
  char S[MAXPATH+28];
  int Rslt;

  if ( Editor->IsModified() )
  {
    if ( !FileName || (FileName[0]=='\0'))
      _fstrcpy(S, "Untitled file has changed.  Save?");
    else
      wsprintf(S, "File \"%s\" has changed.  Save?", FileName);
    Rslt = MessageBox(HWindow, S, "File Changed",
                          MB_YESNOCANCEL | MB_ICONQUESTION);
    if ( Rslt == IDYES )
      return Save();
    else
      return Rslt != IDCANCEL;
  }
  return TRUE;
}

/* Returns a BOOL value indicating whether or not it is Ok to close
  the TEdit's text.  Returns the result of a call to CanClear. */
BOOL TFileWindow::CanClose()
{
  return CanClear();
}

/* Responds to an incoming "New" command (with a CM_FILENEW command
  identifier) by calling NewFile. */
void TFileWindow::CMFileNew(TMessage&)
{
  NewFile();
}
	
/* Responds to an incoming "Open" command (with a CM_FILEOPEN command
  identifier) by calling Open(). */
void TFileWindow::CMFileOpen(TMessage&)
{
  Open();
}

/* Responds to an incoming "Save" command (with a CM_FILESAVE
   command identifier) by calling Save. */
void TFileWindow::CMFileSave(TMessage&)
{
  Save();
}
/* Responds to an incoming "SaveAs" command (with a CM_FILESAVEAS
   command identifier) by calling SaveAs. */
void TFileWindow::CMFileSaveAs(TMessage&)
{
  SaveAs();
}

/* Reads an instance of TFileWindow from the passed ipstream. */
void *TFileWindow::read(ipstream& is)
{
  TEditWindow::read(is);
  FileName = is.freadString();
  IsNewFile = ( FileName[0] == '\0' );
  return this;
}

/* Writes the TFileWindow to the passed opstream. */
void TFileWindow::write(opstream& os)
{
  TEditWindow::write(os);
  os.fwriteString(FileName);
}

TStreamable *TFileWindow::build()
{
  return new TFileWindow(streamableInit);
}

TStreamableClass RegFileWindow("TFileWindow", TFileWindow::build,
					    __DELTA(TFileWindow));
