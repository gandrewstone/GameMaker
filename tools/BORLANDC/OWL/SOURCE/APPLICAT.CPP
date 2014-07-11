// ObjectWindows - (C) Copyright 1992 by Borland International

/* ---------------------------------------------------------
  APPLICAT.CPP
 Defines type TApplication. This defines the basic behavior
 for OWL applications.
----------------------------------------------------------- */

#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include "applicat.h"
#include "appdict.h"

#if defined(__DLL__)
extern TAppDictionary *AppDictionary;
#else
extern PTApplication pTApplication;
#endif

/*  Implementation of Constructor for a TApplication object.
    Inserts "this" into AppDictionary and sets members from
    passed parameters. */

inline void TApplication::__TApplication(

            LPSTR, HINSTANCE, HINSTANCE APrevInstance, LPSTR, int ACmdShow
		
		    )
{
  hPrevInstance = APrevInstance;
  nCmdShow = ACmdShow;
  MainWindow = NULL;
  HAccTable = 0;
  KBHandlerWnd = NULL;

#if defined (__DLL__)
  AppDictionary->Add(this);
#else
  pTApplication = this;
#endif	
}

#ifdef WIN30
TApplication::TApplication(

           LPSTR AName, HINSTANCE_30 AnInstance, HINSTANCE_30 APrevInstance,
	   LPSTR ACmdLine, int ACmdShow
		
		   ) : TModule( AName, HINSTANCE( AnInstance ), ACmdLine)
{
  __TApplication(AName, HINSTANCE( AnInstance ), HINSTANCE( APrevInstance ),
	  ACmdLine, ACmdShow );
}
#endif

#ifdef WIN31
TApplication::TApplication(

           LPSTR AName, HINSTANCE AnInstance, HINSTANCE APrevInstance,
	   LPSTR ACmdLine, int ACmdShow
		
		   ) : TModule( AName, AnInstance, ACmdLine)
{
  __TApplication(AName, AnInstance, APrevInstance, ACmdLine, ACmdShow );
}
#endif

TApplication::~TApplication()
{
  if ( MainWindow ) // true only if MainWindow was destroyed but not deleted.
    delete MainWindow;

#if defined(__DLL__)
  AppDictionary->Delete();
#endif
}

/* Handles initialization for the first executing instance
  of the OWL application. */
void TApplication::InitApplication()
{
}

/* Handles initialization for each executing instance of the OWL
   application.  Creates and displays the main window. */
void TApplication::InitInstance()
{
  InitMainWindow();
  MainWindow = MakeWindow(MainWindow);
  if ( MainWindow )
    MainWindow->Show(nCmdShow);
  else
    Status = EM_INVALIDMAINWINDOW;
}

/* Initialize the application's MainWindow object. By default
   MainWindow's title is the same as the Application's */
void TApplication::InitMainWindow()
{
  MainWindow = new TWindow(NULL, Name);
}

/* Initializes instances, creating and displaying their main window
   (calls InitApplication for the first executing instance; calls
   InitInstance for all instances).Runs the application. Enters message
   loop if initialization was successful. */
void TApplication::Run()
{
  if ( !hPrevInstance )
    InitApplication();
  if (Status == 0 )
    InitInstance();
  if (Status == 0)
    MessageLoop();
  else
    Error(Status);
}

/* General message loop.  Retrieves and processes a message from the
   OWL application's message queue.  Calls ProcessAppMsg to allow
   special handling of the message.  If not specially handled,
   performs default processing of the message, dispatching the message
   to the TWindowsObject's window procedure.  All unusual processing
   can be accomplished by redefining ProcessAppMsg or any of the
   Process... functions. */
void TApplication::MessageLoop()
{
  MSG Message;

  while ( TRUE )
  {
    if ( PeekMessage(&Message, 0, 0, 0, PM_REMOVE) )
    {
      if ( Message.message == WM_QUIT )
        break;
      if ( !ProcessAppMsg(&Message) )
      {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      }
    }
    else   // No message waiting.
      IdleAction();
  }
  Status = Message.wParam;
}

/* Determines whether the application can be closed, returning a BOOL
  indicator.  The default behavior specified here is to return the
  result of a call to the CanClose function of the TApplication's
  MainWindow.  */
BOOL TApplication::CanClose()
{
  if ( MainWindow )
    return MainWindow->CanClose();
  else
    return TRUE;
}

