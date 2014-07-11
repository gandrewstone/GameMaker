// ObjectWindows - (C) Copyright 1992 by Borland International

/* -------------------------------------------------------
  COMBOBOX.CPP
  Defines type TComboBox.  This defines the basic behavior
  of all combo box controls.
  -------------------------------------------------------- */

#include "combobox.h"

/* Constructor for a TComboBoxData object. Initializes Strings and
   Selection. */
TComboBoxData::TComboBoxData()
{
  Strings = new Array(10, 0, 10);
  Selection = NULL;
}

/* Destructor for TComboBoxData. Deletes Strings and Selection. */
TComboBoxData::~TComboBoxData()
{
  if ( Strings )
    delete Strings;
  if ( Selection )
    delete Selection;
}

/* Adds the supplied string to the Strings Array and copies it into
   Selection if IsSelected is TRUE. */
void TComboBoxData::AddString(Pchar AString, BOOL IsSelected)
{
  Strings->add(*new String(AString));
  if ( IsSelected )
  {
      if (AString != Selection)
      {
          if (Selection)
            delete Selection;
	  Selection = strdup(AString);
      }
  }
}

/* Constructor for a TComboBox object.  Initializes its data fields
   using supplied parameters and default values.  By default, an
   MS-Windows combobox associated with the TComboBox will have a
   vertical scrollbar and will maintain its entries in alphabetical
   order. */
TComboBox::TComboBox(PTWindowsObject AParent, int AnId, int X, int Y,
                     int W, int H, DWORD AStyle, WORD ATextLen,
                     PTModule AModule)
           : TListBox(AParent, AnId, X, Y, W, H, AModule)
{
  TextLen = ATextLen;
  Attr.Style = WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | CBS_SORT
               | CBS_AUTOHSCROLL | WS_VSCROLL | AStyle;
}

TComboBox::TComboBox(PTWindowsObject AParent, int ResourceId,
                     WORD ATextLen, PTModule AModule)
           : TListBox(AParent, ResourceId, AModule)
{
  TextLen = ATextLen;
}

/* Sets and selects the contents of the associated edit
   control to the supplied string. */
void TComboBox::SetText(LPSTR AString)
{
  if (SetSelString(AString, -1) < 0 )  // AString not in list box
  {
    SetWindowText(HWindow, AString);
    SetEditSel(0, _fstrlen(AString));
  }
}

/* Returns, in the supplied reference parameters, the starting and
   ending positions of the text selected in the associated
   edit control. Returns CB_ERR if the combo box has no edit
   control */
int TComboBox::GetEditSel(int& StartPos, int& EndPos)
{
    long RetValue;

    RetValue = SendMessage(HWindow, CB_GETEDITSEL, 0, 0);
    StartPos = LOWORD(RetValue);
    EndPos   = HIWORD(RetValue);
    return (int)RetValue;
}

/* Shows the list of an associated drop-down combobox. */
void TComboBox::ShowList()
{
    if ( (GetWindowLong(HWindow,GWL_STYLE) & CBS_DROPDOWN) == CBS_DROPDOWN )
      SendMessage(HWindow, CB_SHOWDROPDOWN, 1, 0);
}

/* Hides the list of an associated drop-down combobox. */
void TComboBox::HideList()
{
    if ( (GetWindowLong(HWindow,GWL_STYLE) & CBS_DROPDOWN) == CBS_DROPDOWN )
        SendMessage(HWindow, CB_SHOWDROPDOWN, 0, 0);
}


static void DoAddForCB(RObject AString, Pvoid AComboBox)
{
  if (AString!=NOOBJECT)
     ((PTListBox)AComboBox)->AddString((Pchar)(PCchar)(RString)AString);
}

/* Transfers the items and selection of the combo box to or from a
   transfer buffer if TF_SETDATA or TF_GETDATA, repectively, is passed
   as the TransferFlag. DataPtr should point to a PTComboBoxData
   (i.e. it should be a pointer to a pointer to a TComboBoxData)
   which points to the data to be transferred.
   Transfer returns the size of PTComboBox (the pointer not the
   object). To retrieve the size without transferring data, pass
   TF_SIZEDATA as the TransferFlag. */
WORD TComboBox::Transfer(Pvoid DataPtr, WORD TransferFlag)
{
   Pchar TmpString;

  PTComboBoxData ComboBoxData = *(PTComboBoxData _FAR *)DataPtr;

  if ( TransferFlag == TF_GETDATA )
  {
#if 0	
    int StringSize = GetWindowTextLength(HWindow) + 1;
    if ( ComboBoxData->Selection )
      delete ComboBoxData->Selection;
    ComboBoxData->Selection= new char[StringSize];
    GetWindowText(HWindow, ComboBoxData->Selection, StringSize);
#endif

    // first, clear out Strings array and fill with contents of list box
    ComboBoxData->Strings->flush();

    for (int i=0; i<GetCount(); i++) {
       TmpString = new char[GetStringLen(i)+1];
       GetString(TmpString, i);
       ComboBoxData->AddString(TmpString, FALSE);
       delete TmpString;
    }

   LONG nCBIndex = SendMessage(HWindow, CB_GETCURSEL, (WORD) NULL, (DWORD) NULL);
   if (ComboBoxData->Selection)
	   delete ComboBoxData->Selection;

   if (nCBIndex != CB_ERR)   //item is selected
      {  int StringSize = SendMessage (HWindow, CB_GETLBTEXTLEN, nCBIndex, (DWORD) NULL);
	      ComboBoxData->Selection = new char[StringSize];
	      if (SendMessage (HWindow, CB_GETLBTEXT, nCBIndex, (DWORD) ComboBoxData->Selection) == CB_ERR)
	         delete ComboBoxData->Selection;    //delete if not successfully filled
      }
  }
  else
    if ( TransferFlag == TF_SETDATA )
    {
      ClearList();
      ComboBoxData->Strings->forEach(DoAddForCB, this);

      int SelIndex = FindExactString(ComboBoxData->Selection, -1);
      if ( SelIndex > -1 )
        SetSelIndex(SelIndex);

      SetWindowText(HWindow, ComboBoxData->Selection);
    }
  return sizeof(PTComboBoxData);
}

/* Returns the appropriate Windows message int identifier for the
   function identified by the supplied AnId. Allows instances
   of TComboBox to inherit many TListBox methods */
WORD TComboBox::GetMsgID(WORD AnId)
{
  WORD MsgXlat[] = {CB_ADDSTRING,    CB_INSERTSTRING, CB_DELETESTRING,
                    CB_RESETCONTENT, CB_GETCOUNT,     CB_GETLBTEXT,
                    CB_GETLBTEXTLEN, CB_SELECTSTRING, CB_SETCURSEL,
                    CB_GETCURSEL,    CB_FINDSTRING };

  return MsgXlat[AnId];
}

/* Limits the amount of text that the user can enter in the combo box's
   edit control to the value of TextLen minus 1. */
void TComboBox::SetupWindow()
{
  TListBox::SetupWindow();
  if ( TextLen != 0 )
    SendMessage(HWindow, CB_LIMITTEXT, TextLen - 1, 0);
}

/* Reads an instance of TComboBox from the supplied ipstream. */
void *TComboBox::read(Ripstream is)
{
  TListBox::read(is);
  is >> TextLen;
  return this;
}

/* Writes the TComboBox to the supplied opstream. */
void TComboBox::write(Ropstream os)
{
  TListBox::write(os);
  os << TextLen;
}

PTStreamable TComboBox::build()
{
  return new TComboBox(streamableInit);
}

TStreamableClass RegComboBox("TComboBox", TComboBox::build,
			     __DELTA(TComboBox));
