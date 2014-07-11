// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  EDIT.CPP
  Defines type TEdit.  This defines the basic behavior
  of all edit controls.
  -------------------------------------------------------- */

#include <dos.h>
#include <string.h>
#include <alloc.h>
#include "edit.h"

/* Constructor for a TEdit object. Initializes its data fields using
   passed parameters and default values. By default, an associated
   static control will have a border and its text will be left-justified.
   Also by default, an associated multiline edit control will have
   horizontal and vertical scroll bars */
TEdit::TEdit(PTWindowsObject AParent, int AnId, LPSTR ATitle, int X,
             int Y, int W, int H, WORD ATextLen, BOOL Multiline,
             PTModule AModule)
      : TStatic(AParent, AnId, ATitle, X, Y, W, H, ATextLen, AModule)
{
  // undo the styles set by TStatic
  Attr.Style = (Attr.Style & ~SS_LEFT) | WS_TABSTOP;

  Attr.Style |= ES_LEFT | ES_AUTOHSCROLL | WS_BORDER;
  if ( Multiline )
    Attr.Style |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL |
                  WS_HSCROLL;
}

/* Constructor for a TEdit to be associated with a MS-Windows
  interface element created by MS-Windows from a resource definition.
  Initializes its data fields using passed parameters.  Data transfer
  is enabled, by default, for the TEdit. */
TEdit::TEdit(PTWindowsObject AParent, int ResourceId, WORD TextLen,
             PTModule AModule)
      : TStatic(AParent, ResourceId, TextLen, AModule)
{
  EnableTransfer();
}

/* Returns a BOOL value indicating whether or not the last change
   to the text of the associated edit control can be undone.  */
BOOL TEdit::CanUndo()
{
  return SendMessage(HWindow, EM_CANUNDO, 0, 0) != 0;
}

/* Undoes the last change to the to the text of the associated
   edit control. */
void TEdit::Undo()
{
  SendMessage(HWindow, WM_UNDO, 0, 0);
}

/* Pastes the contents of the clipboard into the text of the associated
  edit control. */
void TEdit::Paste()
{
  SendMessage(HWindow, WM_PASTE, 0, 0);
}

/* Copies the text selected in the associated edit control to the
  clipboard. */
void TEdit::Copy()
{
  SendMessage(HWindow, WM_COPY, 0, 0);
}

/* Cuts the text selected in the associated edit control into the
  clipboard. */
void TEdit::Cut()
{
  SendMessage(HWindow, WM_CUT, 0, 0);
}

// virtual response functions

/* Responds to an incoming "Cut" command (with a CM_EDITCUT command
  identifier) by calling Cut. */
void TEdit::CMEditCut (TMessage&)
{
  Cut();
}

/* Responds to an incoming "Copy" command (with a CM_EDITCOPY command
  identifier) by calling Copy. */
void TEdit::CMEditCopy (TMessage&)
{
  Copy();
}

/* Responds to an incoming "Paste" command (with a CM_EDITPASTE command
  identifier) by calling Paste. */
void TEdit::CMEditPaste(TMessage&)
{
  Paste();
}

/* Responds to an incoming "Delete" command (with a CM_EDITDELETE
   command identifier) by calling DeleteSelection. */
void TEdit::CMEditDelete(TMessage&)
{
  DeleteSelection();
}

/* Responds to an incoming "Clear" command (with a CM_EDITCLEAR command
  identifier) by calling Clear. */
void TEdit::CMEditClear(TMessage&)
{
  Clear();
}

/* Responds to an incoming "Undo" command (with a CM_EDITUNDO command
  identifier) by calling Undo. */
void TEdit::CMEditUndo(TMessage&)
{
  Undo();
}

/* Beeps when edit control runs out of space */
void TEdit::ENErrSpace(TMessage& Msg)
{
  MessageBeep(0);
  DefNotificationProc(Msg);
}

/* Returns the number of lines in the associated edit control. Note that
   GetNumLines returns 1 when the edit control has no text (i.e. it has
   one line with no text in it). Returns zero if an error occurs. */
int TEdit::GetNumLines()
{
  return (WORD)SendMessage(HWindow, EM_GETLINECOUNT, 0, 0);
}

/* Returns the length of the line (whose number is passed) in the
 associated edit control.  If -1 is passed as the line number, the
 following applies: returns the length of the line upon which the caret
 is positioned; if text is selected on the line, returns the line length
 minus the number of selected characters; if selected text spans more
 than one line,  returns the length of the lines minus the number of
 selected characters. */
int TEdit::GetLineLength(int LineNumber)
{
  int StartPos = -1;

  if ( LineNumber > -1 )
    StartPos = GetLineIndex(LineNumber);
  return (WORD)SendMessage(HWindow, EM_LINELENGTH, StartPos, 0);
}

/* Retrieves the text of the line of the associated edit control
   with the passed line number.  Return FALSE if an error occurs
   or if the text did not completely fit in the passed buffer.
   The returned string is null-terminated. */
BOOL TEdit::GetLine(LPSTR ATextString, int StrSize, int LineNumber)
{
  int BytesCopied;
  BOOL Success;

  if ( StrSize <= 0 )
    return FALSE;

  if ( (StrSize >= GetLineLength(LineNumber) + 1) )
    Success = TRUE;
  else
    Success = FALSE;

  if ( StrSize == 1 )
  {
    ATextString[0] = '\0';
    return Success;
  }

  ((WORD FAR *)ATextString)[0] = StrSize;

  BytesCopied = (WORD)(SendMessage(HWindow, EM_GETLINE,
                                   LineNumber, long(ATextString)));
  if ( BytesCopied != 0 )
  {
    ATextString[BytesCopied] = '\0'; // Windows returns non-null terminated string
    return Success;
  }
  return FALSE;
}

/* Selects the text in the associated edit control which begins
   and ends at the passed positions. */
BOOL TEdit::SetSelection(int StartPos, int EndPos)
{
  return (SendMessage(HWindow, EM_SETSEL, 0, MAKELONG(StartPos,EndPos)) != 0);
}

/* Returns, in the passed var parameters, the starting and
   ending positions of the text selected in the associated
   edit control. */
void TEdit::GetSelection(int& StartPos, int& EndPos)
{
    long RetValue;

    RetValue = SendMessage(HWindow, EM_GETSEL, 0, 0);
    StartPos = LOWORD(RetValue);
    EndPos   = HIWORD(RetValue);
}

/* Returns a BOOL value indicating whether or not the user has
   changed the text in the associated edit control. */
BOOL TEdit::IsModified()
{
  return (SendMessage(HWindow, EM_GETMODIFY, 0, 0) != 0);
}

/* Clears the change flag for the associated edit control. */
void TEdit::ClearModify()
{
  SendMessage(HWindow, EM_SETMODIFY, 0, 0);
}

/* Returns the number of the line of the associated edit control which
  contains the character whose position is passed.  If the position
  passed is greater than the position of the last character, the number
  of the last line is returned. If -1 is passed, the number of the line
  which contains the first selected character is returned. */
int TEdit::GetLineFromPos(int CharPos)
{
  return (int)SendMessage(HWindow, EM_LINEFROMCHAR, CharPos, 0L);
}

/* Returns the number of characters in the associated edit control that
  occur before the line whose number is passed.  If -1 is passed, the
  line number of the line upon which the caret is positioned is used. */
int TEdit::GetLineIndex(int LineNumber)
{
    return  (WORD)SendMessage(HWindow, EM_LINEINDEX, LineNumber, 0);
}

/* Scrolls the text of the associated edit control by the
  specified horizontal and vertical amounts. */
void TEdit::Scroll(int HorizontalUnit, int VerticalUnit)
{
    SendMessage(HWindow, EM_LINESCROLL, 0,
                MAKELONG(VerticalUnit, HorizontalUnit));
}

/* Sets the selection of the associated edit control to
   the passed string. (Does a "paste" type of action
   without affecting the clipboard). */
void TEdit::Insert(LPSTR ATextString)
{
  SendMessage(HWindow, EM_REPLACESEL, 0, (long)ATextString);
}

/* Searchs for and selects the given text in the edit control and
  returns the offset of the text or -1 if the text is not found.
  If the StartPos = -1 then it is assumed that the start pos is
  the end of the current selection. */
int TEdit::Search(int StartPos, LPSTR AText, BOOL CaseSensitive)
{
  LPSTR SText, Line = NULL, Pos = NULL;
  int LineSize = 0, LineLen, NumLines, CurLine, Offset, SBeg;

  if ( AText[0] == '\0' )
    return -1;
  if ( StartPos == -1 )
    GetSelection(SBeg, StartPos);
  if ( !CaseSensitive )
    SText = AnsiLower(_fstrdup(AText));
  else
    SText = AText;
  CurLine = GetLineFromPos(StartPos);
  Offset = StartPos - GetLineIndex(CurLine);
  NumLines = GetNumLines();
  while ( CurLine < NumLines )
  {
    LineLen = GetLineLength(CurLine);
    if ( LineLen >= LineSize )
    {
      LineSize = LineLen + 1;
      Line = (LPSTR)farrealloc(Line, LineSize);
    }
    if ( Line == NULL )
      return -1;
    GetLine(Line, LineSize, CurLine);
    if ( !CaseSensitive )
      AnsiLower(Line);
    Pos = _fstrstr(&Line[Offset], SText);
    if ( Pos )
    {
      SBeg = (int)(GetLineIndex(CurLine) + (Pos - Line));
      SetSelection(SBeg, SBeg + _fstrlen(SText));
      break;
    }
    Offset = 0;
    CurLine++;
  }
  if ( Line )
    farfree(Line);
  if ( !CaseSensitive )
    farfree(SText);
  if ( Pos )
    return SBeg;
  else
    return -1;
}


/* Deletes the selected text in the associated edit control.  Returns
  FALSE if no text is selected. */
BOOL TEdit::DeleteSelection()
{
    int StartPos , EndPos;

    GetSelection(StartPos, EndPos);
    if ( StartPos != EndPos )
    {
	SendMessage(HWindow, WM_CLEAR, 0, 0);
	return TRUE;
    }
    return FALSE;
}

/* Deletes the text of the associated edit control between
   the passed positions.  Returns FALSE if an error occurs. */
BOOL TEdit::DeleteSubText(int StartPos, int EndPos)
{
    if ( SetSelection(StartPos, EndPos) )
        return DeleteSelection();
    else
      return FALSE;
}

/* Deletes the text at the passed line number in the associated edit
  control.  If -1 is passed, deletes the current line.  Returns False
  if the line passed is out of range (and not -1) or if an error occurs. */
BOOL TEdit::DeleteLine(int LineNumber)
{
  int FirstPos , LastPos;

  if ( LineNumber == -1 )
    LineNumber = GetLineFromPos(GetLineIndex(-1));
  FirstPos = GetLineIndex(LineNumber);
  if ( FirstPos != -1 )
  {
    LastPos = GetLineIndex(LineNumber + 1);
    if ( LastPos == -1 )
      LastPos = FirstPos + GetLineLength(LineNumber);
    if ( FirstPos == 0  && FirstPos == LastPos )
    {
      SetText("");
	return TRUE;
    }
    else
	return DeleteSubText(FirstPos, LastPos);
  }
  return FALSE;
}

/* Retrieves the text of the associated edit control between the passed
  positions. */
void TEdit::GetSubText(LPSTR ATextString,int StartPos,int EndPos)
{
  int StartLine , EndLine , StartChar , EndChar;
  int TempSize , TempIndex = 0 , TempStart , TempEnd;
  int TempLine , TempLineLength;
  BOOL OkToContinue = TRUE;
  Pchar PLine;

  if ( EndPos >= StartPos )
  {
    StartLine = GetLineFromPos(StartPos);
    EndLine = GetLineFromPos(EndPos);
    StartChar = StartPos - GetLineIndex(StartLine);
    EndChar = EndPos - GetLineIndex(EndLine);
    for (TempLine = StartLine; TempLine <= EndLine; ++TempLine)
      if ( OkToContinue )
      {
        TempLineLength = GetLineLength(TempLine);
	  PLine = new char [TempLineLength+1];
        if ( TempLine == StartLine )
          TempStart = StartChar;
        else
          TempStart = 0;
        if ( TempLine == EndLine )
          TempEnd = (EndChar>TempLineLength) ? TempLineLength : EndChar;
        else
          TempEnd = TempLineLength;
        TempSize = TempEnd - TempStart;
        if ( GetLine(PLine, TempLineLength + 1, TempLine) )
        {
            // Can happen if we're indexing the CR/LF
            if (TempSize>0)
            {
                movedata(FP_SEG(&PLine[TempStart]),
                         FP_OFF(&PLine[TempStart]),
                         FP_SEG(&ATextString[TempIndex]),
                         FP_OFF(&ATextString[TempIndex]), TempSize );
                TempIndex += TempSize;
            }
            if ( TempLine != EndLine )
            {
                ATextString[TempIndex++] = 0x0d;  // CR
                ATextString[TempIndex++] = 0x0a;  // LF
            }
        }
        else
          OkToContinue = FALSE;
	delete PLine;
      } // if OkToContinue (and for loop)
  }
  ATextString[TempIndex] = '\0';
}

/* Limits the amount of text that an edit control can have to the
  value of TextLen */
void TEdit::SetupWindow()
{
  TStatic::SetupWindow();
  if ( TextLen != 0 )
    SendMessage(HWindow, EM_LIMITTEXT, TextLen - 1, 0);
}

TStreamable *TEdit::build()
{
  return new TEdit(streamableInit);
}

TStreamableClass RegEdit("TEdit", TEdit::build, __DELTA(TEdit));
