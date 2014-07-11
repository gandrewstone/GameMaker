// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __APPLICAT_H
#define __APPLICAT_H

#ifndef __OWL_H
#include <owl.h>
#endif

#ifndef __MODULE_H
#include <module.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

// Application Class

_CLASSDEF(TApplication)

class _EXPORT TApplication : public TModule {
public:

    // WinMain arguments
    HINSTANCE 		hPrevInstance;
    int    		nCmdShow;

    PTWindowsObject MainWindow;
    HACCEL HAccTable;
    PTWindowsObject KBHandlerWnd;

#if defined(WIN31)
    // windows 3.1 interface
    TApplication(LPSTR AName, HINSTANCE AnInstance,
              HINSTANCE APrevInstance, LPSTR ACmdLine, int ACmdShow);
#endif
#if defined(WIN30)
    // windows 3.0 interface
    TApplication(LPSTR AName, HINSTANCE_30 AnInstance,
              HINSTANCE_30 APrevInstance, LPSTR ACmdLine, int ACmdShow);
#endif

    ~TApplication();
    virtual void Run();
    virtual BOOL CanClose();
    void SetKBHandler(PTWindowsObject AWindowsObject);

    // define pure virtual functions derived from Object class
    virtual classType  	  isA() const
         { return applicationClass; }
    virtual Pchar nameOf() const
         {return "TApplication";}
protected:
    virtual void InitApplication();  // "first"-instance initialization
    virtual void InitInstance();     // each-instance initialization
    virtual void InitMainWindow();   // init application main window

    virtual void MessageLoop();
    /* IdleAction may be redefined in derived classes to do some action when
       there are no messages pending. */
    virtual void IdleAction() {}
    virtual BOOL ProcessAppMsg(LPMSG PMessage);
    virtual BOOL ProcessDlgMsg(LPMSG PMessage);
    virtual BOOL ProcessAccels(LPMSG PMessage);
    virtual BOOL ProcessMDIAccels(LPMSG PMessage);

private:

    void __TApplication(LPSTR AName, HINSTANCE AnInstance,
                    HINSTANCE APrevInstance, LPSTR ACmdLine, int ACmdShow);

};	// end of Application class

/* Performs special handling for the message last retrieved.
   Translates keyboard input messages into control selections or
   command messages, when appropriate.  Dispatches message, if
   translated. */
inline BOOL TApplication::ProcessAppMsg(LPMSG PMessage)
       { if ( KBHandlerWnd )
           if ( KBHandlerWnd->IsFlagSet(WB_MDICHILD) )
             return ProcessMDIAccels(PMessage) ||
                    ProcessDlgMsg(PMessage) ||
                    ProcessAccels(PMessage);
           else
            return ProcessDlgMsg(PMessage) ||
                   ProcessMDIAccels(PMessage) ||
                   ProcessAccels(PMessage);
         else
           return ProcessMDIAccels(PMessage) ||
                  ProcessAccels(PMessage); }

/* Attempts to translate a message into a control selection if the
   currently active OWL window has requested "keyboard handling".
   (Some keyboard input messages are translated into control
   selection messages). Dispatches message, if translated. */
inline BOOL TApplication::ProcessDlgMsg(LPMSG PMessage)
    { if (KBHandlerWnd && KBHandlerWnd->HWindow)
           return IsDialogMessage(KBHandlerWnd->HWindow, PMessage);
         else
           return FALSE; }

/* Attempts to translate a message into a command message if the
   TApplication has loaded an accelerator table. (Keyboard input
   messages for which an entry exists in the accelerator table are
   translated into command messages.)  Dispatches message, if
   translated.  (Translation of MDI accelerator messages is performed
   in ProcessMDIAccels function.) */
inline BOOL TApplication::ProcessAccels(LPMSG PMessage)
       { return HAccTable &&
         TranslateAccelerator(
                 MainWindow->HWindow, HAccTable, PMessage); }

/* Attempts to translate a message into a system command message
   for MDI TApplications (whose main window is a TMDIFrame). (Some
   keyboard input messages are translated into system commands for
   MDI applications). Dispatches message, if translated. */
inline BOOL TApplication::ProcessMDIAccels(LPMSG PMessage)
       { return (PTWindowsObject)(MainWindow->GetClient()) &&
           TranslateMDISysAccel(
            ((PTWindowsObject)(MainWindow->GetClient()))->HWindow, PMessage); }

/* Activates and deactivates "keyboard handling" (translation of
   keyboard input into control selections) for the currently active
   TWindowsObject by setting the KBHandlerWnd to the parameter passed.
   This function is called internally by the OWL whenever a OWL window
   is activated.  If "keyboard handling" has been requested for the
   TWindowsObject, the parameter passed is non-NULL, else NULL is
   passed.  "Keyboard handling" is requested, by default, for all
   modeless dialogs and may be requested for a TWindow via a call to
   its EnableKBHandler function. */
inline void TApplication::SetKBHandler(PTWindowsObject AWindowsObject)
       { KBHandlerWnd = AWindowsObject; }

extern PTApplication _EXPFUNC GetApplicationObject();

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif // ifndef _APPLICAT_H
