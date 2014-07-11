// ObjectWindows - (C) Copyright 1992 by Borland International

/* ------------------------------------------------------------
  Defines type TDialog.  This defines the basic behavior
  of all dialogs.
  ------------------------------------------------------------ */

#include "dialog.h"
#include <string.h>
#include <alloc.h>

extern PTWindowsObject DlgCreationWindow;

/* Constructor for a TDialog object. Takes an LPSTR for its template name.
   TWindowsObject's constructor creates an instance thunk for the TDialog. */
TDialog::TDialog(PTWindowsObject AParent, LPSTR AName, PTModule AModule)
                                : TWindowsObject(AParent, AModule)
{
  DisableAutoCreate();
  Title = (LPSTR)MAKEINTRESOURCE(-1);
  if ( HIWORD(AName) )   // Not NULL and not an int in disguise
    Attr.Name = _fstrdup(AName);
  else
    Attr.Name = AName;
  Attr.Param = 0L;
  IsModal = FALSE;
}

/* Constructor for a TDialog object. Takes an int for its template name.
   TWindowsObject's constructor creates an instance thunk for the TDialog. */
TDialog::TDialog(PTWindowsObject AParent, int ResourceId, PTModule AModule)
                                : TWindowsObject(AParent, AModule)
{
  DisableAutoCreate();
  Title = (LPSTR)MAKEINTRESOURCE(-1);
  Attr.Name = (LPSTR)MAKEINTRESOURCE(ResourceId);
  Attr.Param = 0L;
  IsModal = FALSE;
}

/* Destructor for a TDialog. ~TWindowsObject frees the instance
   thunk. */
TDialog::~TDialog()
{
  if ( HIWORD(Attr.Name) )
    farfree(Attr.Name);
}

static BOOL RegisterFails(void *AWindowsObject, void *)
{
  return !((PTWindowsObject)AWindowsObject)->Register();
}

/* Creates an MS-Windows modeless dialog, and associates the
   modeless dialog interface element with the TDialog.  Creation
   and association are not attempted if the Status data member
   is non-zero. */
BOOL TDialog::Create()
{
  HWND HParent;

  IsModal = FALSE; // GetClassName called from Register needs to know this.
  if ( Status == 0 && Register() )
  {
    DisableAutoCreate();
    EnableKBHandler();
    if ( !Parent )
      HParent = 0;
    else
      HParent = Parent->HWindow;
    DlgCreationWindow = this;
    /* Register all the dialog's child objects (for custom control
       support) */
    if ( FirstThat(RegisterFails, NULL) == NULL )
    {
      HWindow = CreateDialogParam(GetModule()->hInstance,
               Attr.Name, HParent, (DLGPROC)GetInstance(), Attr.Param);
      if ( !HWindow )
        Status = EM_INVALIDWINDOW;
    }
    else
      Status = EM_INVALIDCHILD;
    DlgCreationWindow = NULL;
  }
  return (Status == 0);
}

/* Creates an MS-Windows modal dialog.  Associates the modal dialog
  interface element with the TDialog.  Creation and association is not
  attempted if the Status data member is non-zero. */
int TDialog::Execute()
{
  HWND HParent;
  int ReturnValue = -1;
  PTWindowsObject OldKBHandler;

  IsModal = TRUE;
  if ( Status == 0  && Register() )
  {
    DisableAutoCreate();

    /* Enable the keyboard handler. Although modal dialogs do
        their own keyboard handling, we use the WB_KBHANDLER
        flag for WM_COMMAND processing. */
    EnableKBHandler();
    if ( GetApplication() )
      OldKBHandler = GetApplication()->KBHandlerWnd;

    if ( !Parent )
      HParent = 0;
    else
      HParent = Parent->HWindow;
    DlgCreationWindow = this;
    /* Register all the dialog's child objects (for custom control
       support) */
    if ( FirstThat(RegisterFails, NULL) == NULL )
    {
      ReturnValue = DialogBoxParam(GetModule()->hInstance, Attr.Name,
                                HParent, (DLGPROC)GetInstance(), Attr.Param);
      // -1 if the function cannot create the dialog box
      if ( ReturnValue == -1)
        Status = EM_INVALIDWINDOW;
    }
    else
      Status = EM_INVALIDCHILD;
    DlgCreationWindow = NULL;
    if ( GetApplication() )
      GetApplication()->SetKBHandler(OldKBHandler);
  }
  if ( Status == 0 )
    delete this;
  else
      if (ReturnValue != -1)
          ReturnValue = BAD_DIALOG_STATUS;  // dialog ran, but status != 0
  return ReturnValue;
}

/* Responds to an incoming WM_INITDIALOG message.  This message is
  sent after an MS-Windows dialog is created and before the dialog
  is displayed. Calls SetupWindow to perform set up for the dialog. */
void TDialog::WMInitDialog(TMessage&)
{
  SetupWindow();
}

/* Sets up the dialog box by calling SetCaption and then
   TWindowsObject::SetupWindow. Calling SetCaption here allows one
   to override the dialog caption (specified in the dialog resource)
   by setting Title prior to this point. */
void TDialog::SetupWindow()
{
  SetCaption(Title);
  TWindowsObject::SetupWindow();
}

/*Respond to Windows attempt to close down. Note: a dialog needs
  to invert the test because Windows expects the opposite response from
  that of a normal window. */
void TDialog::WMQueryEndSession(TMessage& Msg)
{
  if ( GetApplication() && this == GetApplication()->MainWindow )
    Msg.Result = (long)!(GetApplication()->CanClose());
  else
    Msg.Result = (long)!CanClose();
}

/* Conditionally shuts down the dialog box. If this is a modal
   dialog calls CanClose and, if CanClose returns true, transfers
   its data and shuts down, passing ARetValue. Calls
   TWindowsObject::CloseWindow if this is a modeless dialog. */
void TDialog::CloseWindow(int ARetValue)
{
  if ( IsModal )
  {
    if ( CanClose() )
    {
      TransferData(TF_GETDATA);
      ShutDownWindow(ARetValue);
    }
  }
  else   // !IsModal
    TWindowsObject::CloseWindow();
}

/* Conditionally shuts down the dialog box. Calls CloseWindow
   passing IDCANCEL if this is a modal dialog. If this is a
   modeless dialog calls TWindowsObject::CloseWindow. */
void TDialog::CloseWindow()
{
  if ( IsModal )
    CloseWindow(IDCANCEL);
  else   // !IsModal
    TWindowsObject::CloseWindow();
}

/* Unconditionally shuts down a dialog box, returning ARetValue if this
   is a modal dialog. */
void TDialog::ShutDownWindow(int ARetValue)
{
  if ( IsModal )
    // Note that we can't delete a modal dialog here because we're still in
    // its Execute function.
    Destroy(ARetValue);
  else
    TWindowsObject::ShutDownWindow();
}

/* Unconditionally shuts down a dialog box, returning IDCANCEL if this
   is a modal dialog. */
void TDialog::ShutDownWindow()
{
  if ( IsModal )
    // Note that we can't delete a modal dialog here because we're still in
    // its Execute function.
    Destroy(IDCANCEL);
  else
  TWindowsObject::ShutDownWindow();
}

static void DoEnableAutoCreate(void *AWindowsObject, void *)
{
  if ( ((PTWindowsObject)AWindowsObject)->HWindow )
    ((PTWindowsObject)AWindowsObject)->EnableAutoCreate();
}

/* Destroys the MS-Windows dialog associated with the TDialog. */
void TDialog::Destroy(int ARetValue)
{
  if ( IsModal && HWindow)
  {
    ForEach(DoEnableAutoCreate, NULL);
    EndDialog(HWindow, ARetValue);
  }
  else
    TWindowsObject::Destroy();
}

/* Destroys the MS-Windows dialog associated with the TDialog. */
void TDialog::Destroy()
{
  if ( IsModal )
    Destroy(IDCANCEL);
  else
    TWindowsObject::Destroy();
}

/* Set Dialog's Title data member and Caption */
void TDialog::SetCaption(LPSTR ATitle)
{
  if ( (int)ATitle != -1 )
    TWindowsObject::SetCaption(ATitle);
}

/* Responds to an incoming notification message from a button with
   an Id equal to IDOK.  Calls CanClose.  If the call returns True,
   calls TransferData and then ends the dialog, returning IDOK. */
void TDialog::Ok(TMessage&)
{
  CloseWindow(IDOK);
}

/* Responds to an incoming notification message from a button with
   an Id equal to IDCANCEL.  Ends the dialog, returning IDCANCEL
   if it's a modal dialog. */
void TDialog::Cancel(TMessage&)
{
  ShutDownWindow();
}

/* Ends the dialog, returning IDCANCEL if it's a modal dialog. */
void TDialog::WMClose(TMessage&)
{
  ShutDownWindow();
}

LPSTR TDialog::GetClassName()
{
  if ( IsModal )
    return (LPSTR)32770L;
  else
    return "OWLDialog31";
}

/* Specifies registration attributes for the MS-Windows window
   class of the TDialog, allowing instances of TDialog to
   be registered.  Sets the fields of the passed WNDCLASS
   parameter to the default attributes appropriate for a
   TDialog. */
void TDialog::GetWindowClass(WNDCLASS& AWndClass)
{
  AWndClass.style = CS_HREDRAW | CS_VREDRAW;
  AWndClass.lpfnWndProc = DefDlgProc;
  AWndClass.cbClsExtra = 0;
  AWndClass.cbWndExtra = DLGWINDOWEXTRA;
  AWndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  AWndClass.lpszMenuName = NULL;
  AWndClass.hInstance = GetModule()->hInstance;
  AWndClass.hIcon = LoadIcon(0, IDI_APPLICATION);
  AWndClass.hCursor = LoadCursor(0, IDC_ARROW);
  AWndClass.lpszClassName = GetClassName();
}

/* Reads an instance of TDialog from the passed ipstream. */
void *TDialog::read(ipstream& is)
{
  BOOL NameIsNumeric;

  TWindowsObject::read(is);

  is >> NameIsNumeric;
  if ( NameIsNumeric )
    is >> (long)(Attr.Name);
  else
    Attr.Name = is.freadString();

  is >> IsModal;
  return this;
}

/* Writes the TDialog to the passed opstream. */
void TDialog::write(opstream& os)
{
  BOOL NameIsNumeric;

  TWindowsObject::write(os);
  NameIsNumeric = HIWORD(Attr.Name) == NULL;
  os << NameIsNumeric;
  if ( NameIsNumeric )
    os << (long)(Attr.Name);
  else
    os.fwriteString(Attr.Name);
  os << IsModal;
}

TStreamable *TDialog::build()
{
  return new TDialog(streamableInit);
}

TStreamableClass RegDialog("TDialog", TDialog::build, __DELTA(TDialog));
