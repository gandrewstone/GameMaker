// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  LISTBOX.CPP
  Defines type TListBox.  This defines the basic behavior
  of all listbox controls.
  -------------------------------------------------------- */

#include "listbox.h"
#include <stdlib.h>

#define MULTIPLESEL (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)

/* Constructs Strings and SelStrings, and initializes SelCount
   to zero. */
TListBoxData::TListBoxData()
{
  SelCount = 0;
  Strings = new Array(10, 0, 10);
  SelStrings = new Array(10, 0, 10);
}

/* Deletes Strings and SelStrings. */
TListBoxData::~TListBoxData()
{
  if ( Strings )
    delete Strings;
  if ( SelStrings )
    delete SelStrings ;
}

/* Adds the specified string to Strings.  If IsSelected is
   TRUE, also adds the string to SelStrings. */
void TListBoxData::AddString(Pchar AString, BOOL IsSelected)
{
  Strings->add(*new String(AString));
  if ( IsSelected )
    SelStrings->addAt(*new String(AString), SelCount++);
}

/* Adds the specified string to SelStrings. */
void TListBoxData::SelectString(LPSTR AString)
{
  Pchar TmpString = new char[_fstrlen(AString) + 1];
  _fstrcpy(TmpString, AString);
  SelStrings->addAt(*new String(TmpString), SelCount++);
  delete TmpString;
}

/* Removes all strings from SelStrings and sets SelCount to 0. */
void TListBoxData::ResetSelections()
{
    SelStrings->flush();
    SelCount = 0;
}

/* Returns the length of the string at the passed index in
   SelStrings excluding the terminating null. */
int TListBoxData::GetSelStringLength(int Index)
{
    if (Index >= 0 && Index < SelCount)
        return strlen((PCchar)(RString)(*SelStrings)[Index]);
    else
        return -1;
}

/* Copies the string at the passed index in SelStrings into
   Buffer. BufferSize includes the terminating null. */
void TListBoxData::GetSelString(LPSTR Buffer, int BufferSize, int Index)
{
  if (BufferSize > 0)
  {
      if (Index >= 0 && Index < SelCount)
      {
          _fstrncpy(Buffer, (PCchar)(RString)(*SelStrings)[Index],
              BufferSize-1);
          Buffer[BufferSize - 1] = '\0';
      }
      else
          Buffer[0] = '\0';
  }
}

/* Constructor for an instance of TListBox.  Initializes its data fields
  using parameters passed and default values.  By default, an MS-Windows
  listbox associated with the TListBox will: be visible upon creation;
  have a border and a vertical scrollbar; maintain entries in
  alphabetical order; and notify its parent when a selection is made. */
TListBox::TListBox(PTWindowsObject AParent, int AnId, int X, int Y,
                   int W, int H, PTModule AModule)
         : TControl(AParent, AnId, NULL, X, Y, W, H, AModule)
{
  Attr.Style |= LBS_STANDARD;
}

static void DoAddForLB(RObject AString, Pvoid AListBox)
{
  if (AString!=NOOBJECT)
    ((PTListBox)AListBox)->AddString((Pchar)(PCchar)(RString)AString);
}

/* Transfers state information for a TListBox. The TransferFlag passed
  specifies whether data is to be read from or written to the passed
  buffer, or whether the data element size is simply to be returned.
  The return value is the size (in bytes) of the transfer data. */
WORD TListBox::Transfer(Pvoid DataPtr, WORD TransferFlag)
{
  int I, SelIndex;
  long Style;
  int *Selections;
  int MaxStringLen = 0, TmpStringLen = 0;
  Pchar TmpString;
  PTListBoxData ListBoxData = *(PTListBoxData _FAR *)DataPtr;

  Style = GetWindowLong(HWindow, GWL_STYLE);
  if ( TransferFlag == TF_GETDATA )
  {
    // first, clear out Strings array and fill with contents of list box
    ListBoxData->Strings->flush();

    for (int i=0; i<GetCount(); i++) {
       TmpString = new char[GetStringLen(i)+1];
       GetString(TmpString, i);
       ListBoxData->AddString(TmpString, FALSE);
       delete TmpString;
    }

    // then update transfer data with new selected item(s)
    ListBoxData->ResetSelections();
    if ( (Style & MULTIPLESEL) == 0 ) // single selection
    {
      // Get selected item
      SelIndex = (int)SendMessage(HWindow, LB_GETCURSEL, 0, 0);
      if ( SelIndex != LB_ERR )
      {
         // allocate string for selected item and copy string to transfer data
	      MaxStringLen = GetStringLen(SelIndex);
	      TmpString = new char[MaxStringLen + 1];
	      GetString(TmpString, SelIndex);
	      ListBoxData->SelectString(TmpString);
	      delete TmpString;
      }
    }
    else // multiple selection
    {
      I = GetSelCount();
      if ( I > 0 )
      {
        Selections = new int[I];
        SendMessage(HWindow, LB_GETSELITEMS, I, (long)(Selections));
        // loop through all selections to find longest string
        for (int SelIndex = 0; SelIndex < I; SelIndex++)
        {
          TmpStringLen = GetStringLen(Selections[SelIndex]);
          if ( TmpStringLen > MaxStringLen )
            MaxStringLen = TmpStringLen;
        }
        // allocate temporary string large enough to hold longest selection
        TmpString = new char[MaxStringLen+1];
        for (SelIndex = 0; SelIndex < I; SelIndex++)
        {
          GetString(TmpString, Selections[SelIndex]);
          ListBoxData->SelectString(TmpString);
        }
	     delete TmpString;
	     delete Selections;
      }
    }
  }
  else if ( TransferFlag == TF_SETDATA )
  {
    ClearList();
    // add each string in ListBoxData to list box
    ListBoxData->Strings->forEach(DoAddForLB, this);

    // now update selected item(s) as per ListBoxData

    if ( (Style & MULTIPLESEL) == 0 )
    {       // single selection list box
      if ( ListBoxData->SelCount )
      {
        SelIndex = FindExactString((LPSTR)(PCchar)(RString)
                     (*ListBoxData->SelStrings)[0], -1);
        if ( SelIndex > -1 )
          SetSelIndex(SelIndex);
      }
    }
    else    // multiple selection list box
    {
      SendMessage(HWindow, LB_SETSEL, 0, -1); // Unselect all

      for (I = 0; I < ListBoxData->SelCount; ++I)
      {
        SelIndex = FindExactString((LPSTR)(PCchar)(RString)
                     (*ListBoxData->SelStrings)[I], -1);
        if ( SelIndex > -1 )
          SendMessage(HWindow, LB_SETSEL, 1, SelIndex);
      }
    }
  }
  return sizeof(PTListBoxData);
}

/* Adds a string to an associated listbox.  Returns index of the string
  in the list (the first entry is at index 0).  A negative value is
  returned if an error occurs. */
int TListBox::AddString(LPSTR AString)
{
  return (int)(SendMessage(HWindow, GetMsgID(MN_ADDSTRING),
                           0, (long)AString));
}

/* Inserts a string in the associated listbox at the passed index,
  returning the index of the string in the list.  A negative value is
  returned if an error occurs. */
int TListBox::InsertString(LPSTR AString, int Index)
{
  return (int)SendMessage(HWindow, GetMsgID(MN_INSERTSTRING),
                          Index, (long)AString);
}

/* Deletes the string at the passed index in the associated listbox.
  Returns a count of the entries remaining in the list.  A negative
  value is returned if an error occurs. */
int TListBox::DeleteString(int Index)
{
  return (int)SendMessage(HWindow, GetMsgID(MN_DELETESTRING), Index, 0);
}

/* Clears all the entries in the associated listbox. */
void TListBox::ClearList()
{
  SendMessage(HWindow, GetMsgID(MN_RESETCONTENT), 0, 0);
}

/* Returns the number of entries in the associated listbox. A negative
  value is returned if an error occurs. */
int TListBox::GetCount()
{
  return (int)SendMessage(HWindow, GetMsgID(MN_GETCOUNT), 0, 0);
}

/* Returns the index of the first string in the associated listbox
  which begins with the passed string.  Searches for a match beginning
  at the passed SearchIndex. If a match is not found after the last
  string has been compared, the search continues from the beginning of
  the list until a match is found or until the list has been completely
  traversed.  Searches from beginning of list when -1 is passed as the
  index.  Returns the index of the selected string.  A negative value
  is returned if an error occurs. For single or multiple-selection
  list boxes. */
int TListBox::FindString(LPSTR Prefix, int SearchIndex)
{
  return (int)SendMessage(HWindow, GetMsgID(MN_FINDSTRING),
	  SearchIndex, (long)Prefix);
}

/* Returns the index of the first string in the associated listbox
  which is the same as the passed string.  Searches for a match
  beginning at the passed SearchIndex. If a match is not found after
  the last string has been compared, the search continues from the
  beginning of the list until a match is found or until the list has
  been completely traversed.  Searches from beginning of list when -1
  is passed as the index.  Returns the index of the selected string.
  A negative value is returned if an error occurs. For single or
  multiple-selection list boxes. */
int TListBox::FindExactString(LPSTR AString, int SearchIndex)
{
  Pchar TmpString;
  int FirstMatch;
  BOOL Found = FALSE;

  FirstMatch = SearchIndex = (int)SendMessage(HWindow,
     GetMsgID(MN_FINDSTRING), SearchIndex, (long)AString);
  do
  {
    if ( SearchIndex > -1 )
    {
      TmpString = new char[GetStringLen(SearchIndex) + 1];
      GetString(TmpString, SearchIndex);
      if ( lstrcmp(TmpString, AString) == 0 )
        Found = TRUE;
      else
        SearchIndex = (int)SendMessage(HWindow,
           GetMsgID(MN_FINDSTRING), SearchIndex, (long)AString);
      delete TmpString;
    }
  } while ( !Found && SearchIndex != FirstMatch );

  if ( !Found )
    return -1;
  else
    return SearchIndex;
}

/* Retrieves the contents of the string at the passed index of the
  associated listbox, returning the length of the string (in bytes
  excluding the terminating null) as the value of the call. A
  negative value is returned if the passed index is not valid.
  The buffer must be large enough for the string and the terminating
  null. */
int TListBox::GetString(LPSTR AString, int Index)
{
  return (int)SendMessage(HWindow, GetMsgID(MN_GETTEXT),
                          Index, (long)AString);
}

/* Returns the length of the string at the passed index in the
   associated listbox excluding the terminating null. A negative
   value is returned if an error occurs. */
int TListBox::GetStringLen(int Index)
{
  return (int)SendMessage(HWindow, GetMsgID(MN_GETTEXTLEN), Index, 0);
}

/* Returns the number of selected items in the list box. For single-
   or multiple-selection list boxes (and combo boxes). */
int TListBox::GetSelCount()
{
  if ( (GetWindowLong(HWindow, GWL_STYLE) & MULTIPLESEL) == 0)
    //including if it's a combobox.
    return GetSelIndex() < 0 ? 0 : 1;
  else  // multiple-selection list box
    return (int)SendMessage(HWindow, LB_GETSELCOUNT, 0, 0);
}

/* For use with single-selection list boxes (and combo boxes).
   Retrieves the text of the string which is selected in the
   associated listbox.  Returns the number of characters copied.
   -1 is returned if no string is selected or if this is a
   multiple-selection list box.  Since the Windows function is not
   passed a size parameter, we have to allocate a string to hold
   the largest string (gotten from a query), and copy a part of it. */
int TListBox::GetSelString(LPSTR AString, int MaxChars)
{
  int Index, Length;
  Pchar TempString;

  if ( (GetWindowLong(HWindow, GWL_STYLE) & MULTIPLESEL) == 0)
  //including if it's a combobox.
  {
    Index = GetSelIndex();
    if ( Index > -1 )
    {
      Length = GetStringLen(Index);
      if ( MaxChars >= Length )
        return GetString(AString,Index);
      else
      {
        TempString = new char[Length+1];
        if ( TempString )
        {
          GetString(TempString, Index);
          _fstrncpy(AString, TempString, MaxChars);
          delete TempString;
          return MaxChars;
        }
      }
    }
  }
  return -1;
}

/* Retrieves the text of the strings which are selected in the
   associated listbox.  Returns the number of items put into Strings.  -1 is
   returned if this is not a multiple-selection list box.  Since the Windows
   function is not  passed a size parameter, we have to allocate a string
   to hold the largest string (gotten from a query), and copy a part of it.
   Only for use with multiple-selection list boxes. */
int TListBox::GetSelStrings(LPSTR *Strings, int MaxCount, int MaxChars)
{
  int I, TmpStringLen, *Selections;
  Pchar TmpString;

  if ( (GetWindowLong(HWindow, GWL_STYLE) & MULTIPLESEL) == 0)
    //including if it's a combobox.
    return -1;

  I = GetSelCount();
  if ( I < MaxCount )
    MaxCount = I;

  if ( MaxCount > 0 )
  {
    Selections = new int[MaxCount];
    SendMessage(HWindow, LB_GETSELITEMS, MaxCount, (long)(Selections));
    for (int SelIndex = 0; SelIndex < MaxCount; SelIndex++)
    {
      TmpStringLen = GetStringLen(Selections[SelIndex]);
      if ( MaxChars >= TmpStringLen )
        GetString(Strings[SelIndex], Selections[SelIndex]);
      else
      {
        TmpString = new char [TmpStringLen+1];
        if ( TmpString )
        {
          GetString(TmpString, Selections[SelIndex]);
          _fstrncpy(Strings[SelIndex], TmpString, MaxChars);
          delete TmpString;
        }
      }
    }
    delete Selections;
  }
  return MaxCount;
}

/* Selects the first string in the associated listbox following the
  passed index which begins with the passed string.  Searches for a
  match beginning at the passed Index.  If a match is not found after
  the last string has been compared, the search continues from the
  beginning of the list until a match is found or until the list has
  been completely traversed.  Searches from beginning of list when
  -1 is passed as the index.  Returns the index of the selected
  string.  A negative value is returned if an error occurs. Only for
  single-selection list boxes (and combo boxes). */
int TListBox::SetSelString(LPSTR Prefix, int SearchIndex)
{
  if ( (GetWindowLong(HWindow, GWL_STYLE) & MULTIPLESEL) == 0 )
    //including if it's a combobox.
    return (int)SendMessage(HWindow, GetMsgID(MN_SELECTSTRING),
                            SearchIndex, (long)Prefix);
  else
    return -1;
}

/* Selects the strings in the associated list box which begin with
   the passed prefixes.  For each string the search begins at the
   beginning of the list and continues until a match is found or
   until the list has been completely traversed. If ShouldSet is
   TRUE, the matched strings are selected and highlighted, if
   ShouldSet is FALSE the highlight is removed from the matched
   strings and they are no longer selected. Returns the number of
   strings successfully selected or deselected. If NumSelections
   is less than zero, all strings are selected or deselected, and a
   negative value is returned on failure. Only for multiple-
   selection list boxes.  (-1 is returned if this is not a
   multiple-selectionlist box). */
int TListBox::SetSelStrings(LPSTR *Prefixes, int NumSelections,
                            BOOL ShouldSet)
{
  int SelIndex, Successes = 0;

  if ( (GetWindowLong(HWindow, GWL_STYLE) & MULTIPLESEL) == 0)
    //including if it's a combobox.
    return -1;

  if ( NumSelections > -1 )
  {
    for ( int I = 0; I < NumSelections; I++ )
      if ((SelIndex = FindString(Prefixes[I], -1)) > -1 )
        if ( (int)SendMessage(HWindow, LB_SETSEL, ShouldSet, SelIndex)
             > -1 )
          Successes++;
  }
  else
    return (int)SendMessage(HWindow, LB_SETSEL, ShouldSet, -1);
  return Successes;
}

/* Returns the index of the selected string in the associated listbox.
   A negative value is returned if no string is selected or this
   is a multiple-selection list box. Only for single-selection
   flist boxes (and combo boxes). */
int TListBox::GetSelIndex()
{
  if ( (GetWindowLong(HWindow, GWL_STYLE) & MULTIPLESEL) == 0)
    //including if it's a combobox.
    return (int)SendMessage(HWindow, GetMsgID(MN_GETCURSEL), 0, 0);
  else
    return -1;
}

/* Fills Indexes with the indexes of up to Count selected strings.
   Returns number of items put in the array (-1 for single-selection
   list boxes and combo boxes). */
int TListBox::GetSelIndexes(Pint Indexes, int MaxCount)
{
  if ( (GetWindowLong(HWindow, GWL_STYLE) & MULTIPLESEL) == 0)
    //including if it's a combobox.
    return -1;
  return (int)SendMessage(HWindow, LB_GETSELITEMS, MaxCount,
                          (long)(Indexes));
}

/* Selects the string at passed index in the associated listbox and
   forces the string into view.  Clears selection when -1 is passed
   as the index. A negative value is returned if an error occurs.
   Only for single-selection list boxes (and combo boxes). */
int TListBox::SetSelIndex(int Index)
{
  if ( (GetWindowLong(HWindow, GWL_STYLE) & MULTIPLESEL) == 0)
    //including if it's a combobox.
    return (int)SendMessage(HWindow, GetMsgID(MN_SETCURSEL), Index, 0);
  else
    return -1;
}

/* Selects/deselects the strings in the associated list box at the
   passed indexes. If ShouldSet is TRUE, the indexed strings are
   selected and highlighted, if ShouldSet is FALSE the highlight is
   removed and they are no longer selected. Returns the number of
   strings successfully selected or deselected (-1 if not a
   multiple-selection list box).  If NumSelections is less than zero,
   all strings are selected or deselected, and a negative value is
   returned on failure. Only for multiple-selection list boxes. */
int TListBox::SetSelIndexes(Pint Indexes, int NumSelections,
                            BOOL ShouldSet)
{
  int Successes = 0;

  if ( (GetWindowLong(HWindow, GWL_STYLE) & MULTIPLESEL) == 0)
    //including if it's a combobox.
    return -1;

  if ( NumSelections > -1 )
  {
    for ( int I = 0; I < NumSelections; I++ )
      if ( (int)SendMessage(HWindow, LB_SETSEL, ShouldSet, Indexes[I])
             > -1 )
        Successes++;
  }
  else
    return (int)SendMessage(HWindow, LB_SETSEL, ShouldSet, -1);
  return Successes;
}

/* Returns the appropriate MS-Windows message WORD identifier for
   the function identified by the passed MsgName string. Allows
   instances of TComboBox (which redefines GetMsgID) to inherit
   many TListBox methods. */
WORD TListBox::GetMsgID(WORD AMsg)
{
  WORD MsgXlat[] = { LB_ADDSTRING,    LB_INSERTSTRING, LB_DELETESTRING,
                     LB_RESETCONTENT, LB_GETCOUNT,     LB_GETTEXT,
                     LB_GETTEXTLEN,   LB_SELECTSTRING, LB_SETCURSEL,
                     LB_GETCURSEL,    LB_FINDSTRING };
  return MsgXlat[AMsg];
}

PTStreamable TListBox::build()
{
  return new TListBox(streamableInit);
}

TStreamableClass RegListBox("TListBox", TListBox::build, __DELTA(TListBox));
