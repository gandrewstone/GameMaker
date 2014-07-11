// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __OWL_H
#define __OWL_H

// ---------------------------------------------------
// This include file causes all the include files
// necessary for basic owl functionality to be included.
//----------------------------------------------------

#if defined(_CLASSDLL) || defined(__DLL__)
#  define _RTLDLL
#endif

#if         defined(BUILD_OWL)

#if         !defined(WIN30) && !defined(WIN31)
#error   OWL must be built with WIN30 or WIN31 (or both) defined.
#endif

#if         !defined(WIN31) && defined(STRICT)
#error   OWL must be built with WIN31 defined if STRICT is defined.
#endif

#if         defined(WIN30)  && defined(WIN31) && !defined(STRICT)
#error   OWL must be built with STRICT if both WIN30 and WIN31 are defined.
#endif

#if         defined(__COMPACT__)
#error   OWL cannot be built in Compact model.
#endif

#if         defined(__TINY__)
#error   OWL cannot be built in Tiny model.
#endif

#else   //  !defined(BUILD_OWL)

#if         !defined(WIN30) && !defined(WIN31)
#error   OWL applications must be built with either WIN30 or WIN31 defined.
#endif

#if         defined(WIN30) && defined(WIN31)
#error   OWL applications cannot be built with both WIN30 and WIN31 defined.
#endif

#if         !defined(WIN31) && defined(STRICT)
#error   OWL applications must be built with WIN31 defined if STRICT is defined.
#endif

#if         defined(__COMPACT__)
#error   OWL applications cannot be built in Compact model.
#endif

#if         defined(__TINY__)
#error   OWL applications cannot be built in Tiny model.
#endif

#endif  //  !defined(BUILD_OWL)

#if !defined(__DEFS_H)
#include <_defs.h>
#endif

#if !defined(__OWLDEFS_H)
#include <owldefs.h>
#endif

#if !defined(__APPLICAT_H)
#include <applicat.h>
#endif

extern unsigned short far  _EXPFUNC OWLGetVersion();

const unsigned short OWLVersion = 0x0110; // Version 1.1

#endif
