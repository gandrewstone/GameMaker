// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  INPUTDIA.CPP
  Defines type TInputDialog.  This defines the basic
  behavior of all input dialogs.
  -------------------------------------------------------- */

#include "inputdia.h"
#include <string.h>

TInputDialog::TInputDialog(PTWindowsObject AParent, LPSTR ATitle,
                           LPSTR APrompt, LPSTR ABuffer, WORD ABufferSize,
                           PTModule AModule)
             : TDialog(AParent, SD_INPUTDIALOG, AModule)
{
  SetCaption(ATitle);
  Prompt = _fstrdup(APrompt ? APrompt : "");
  Buffer = ABuffer;
  BufferSize = ABufferSize;
}

/* Sets and gets the values of the items (controls) of the input
   dialog. */
void TInputDialog::TransferData(WORD Direction)
{
  if ( Direction == TF_SETDATA )
  {
    SetDlgItemText(HWindow, ID_PROMPT, Prompt);
    SetDlgItemText(HWindow, ID_INPUT, Buffer);
  }
  else
    if ( Direction == TF_GETDATA )
      GetDlgItemText(HWindow, ID_INPUT, Buffer, BufferSize);
}

/* Sets the values of the items (controls) of the input dialog. */
void TInputDialog::SetupWindow()
{
  TDialog::SetupWindow();
  SendDlgItemMessage(HWindow, ID_INPUT, EM_LIMITTEXT,
                       BufferSize - 1, 0);
}

/* Reads an instance of TInputDialog from the passed ipstream. */
void *TInputDialog::read(ipstream& is)
{
  TDialog::read(is);

  Prompt = is.freadString();
  return this;
}

/* Writes the TInputDialog to the passed opstream. */
void TInputDialog::write(opstream& os)
{
  TDialog::write(os);

  os.fwriteString(Prompt);
}

TStreamable *TInputDialog::build()
{
  return new TInputDialog(streamableInit);
}

TStreamableClass RegInputDialog("TInputDialog", TInputDialog::build,
					      __DELTA(TInputDialog));
