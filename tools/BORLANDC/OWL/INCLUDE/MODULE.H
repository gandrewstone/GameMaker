// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __MODULE_H
#define __MODULE_H

#ifndef __OBJECT_H
#include <object.h>
#endif

#ifndef __WINDOWS_H
#undef NULL
#include <windows.h>
#endif

#ifndef __OWLDEFS_H
#include <owldefs.h>
#endif

#ifndef __OWL_H
#include <owl.h>
#endif

#ifndef __WINDOW_H
#include <window.h>
#endif

#ifndef __MDI_H
#include <mdi.h>
#endif

#ifndef __DIALOG_H
#include <dialog.h>
#endif

#pragma option -Vo-
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po-
#endif

_CLASSDEF(TModule)

// Module Class

class _EXPORT TModule : public Object {
public:

    // Lib and WinMain args
    HINSTANCE		hInstance;
    LPSTR		lpCmdLine;

    int Status;
    LPSTR Name;

#if defined(WIN31)
    TModule(LPSTR AName, HINSTANCE AnInstance, LPSTR ACmdLine);
#endif
#if defined(WIN30)
    TModule(LPSTR AName, HINSTANCE_30 AnInstance, LPSTR ACmdLine);
#endif
    virtual ~TModule();

    BOOL LowMemory();
    void RestoreMemory();
    virtual PTWindowsObject ValidWindow(PTWindowsObject AWindowsObject);
    virtual PTWindowsObject MakeWindow(PTWindowsObject AWindowsObject);
    virtual int ExecDialog(PTWindowsObject ADialog);
    HWND GetClientHandle(HWND AnHWindow);

#if defined(WIN31)
    // windows 3.1 interface
    virtual PTWindowsObject GetParentObject(HWND ParentHandle);
#endif
#if defined(WIN30)
    // windows 3.0 interface
    virtual PTWindowsObject GetParentObject(HWND_30 ParentHandle);
#endif
#if (defined(WIN30) || defined(WIN31)) && !(defined(WIN30) && defined(WIN31))
    // this function is never called. it is used to pad the vtable so that
    // exactly two GetParentObject(...) definitions are always present.
    virtual PTWindowsObject GetParentObject(void *)
            { return NULL; }
#endif
    virtual void Error(int ErrorCode);

    // define pure virtual functions derived from Object class
    virtual classType  	  isA() const
	    { return moduleClass; }
    virtual Pchar nameOf() const
	    { return "TModule"; }
    virtual hashValueType hashValue() const
	    {  return hashValueType(hInstance); }
    virtual int 	  isEqual(RCObject module)  const
            { return (hInstance == ((RTModule)module).hInstance); }
    virtual void	  printOn(Rostream outputStream) const
            { outputStream << nameOf() << "{ hInstance = "
              << (void _FAR *)hInstance << " }\n"; }
private:

    PTWindowsObject __GetParentObject(HWND ParentHandle);
    void __TModule(LPSTR AName, HINSTANCE AnInstance, LPSTR ACmdLine);

};	// end of Module class

extern PTModule Module;

#pragma option -Vo.
#if     defined(__BCOPT__) && !defined(_ALLOW_po)
#pragma option -po.
#endif

#endif // ifndef __MODULE_H
