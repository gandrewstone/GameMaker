// ObjectWindows - (C) Copyright 1992 by Borland International

#ifndef __OWLDEFS_H
#define __OWLDEFS_H

#if defined(_CLASSDLL) || defined(__DLL__)
#  define _RTLDLL
#endif

#if !defined(__DEFS_H)
  #include <_defs.h>
#endif	// __DEFS_H

#if !defined(__OWLRC_H)
  #include <owlrc.h>
  #define __OWLRC_H
#endif

#if defined(__cplusplus)
typedef int	   _FAR &		Rint;
typedef int	   _FAR *		Pint;
typedef void	   _FAR *		Pvoid;
typedef void	   _FAR * _FAR &	RPvoid;
typedef char	   _FAR *		Pchar;
typedef const char _FAR *		PCchar;
typedef const void _FAR *		PCvoid;

// windows 3.0-compatibility types

#if  defined(WIN30)

#define            _WIN30_API           1

typedef long                            LONG_30;
typedef unsigned int                    WORD_30;
typedef unsigned int                    WORD_30;
typedef unsigned int                    HDC_30;
typedef unsigned int                    HWND_30;
typedef unsigned int                    HINSTANCE_30;

#endif
#endif

/* Since some resource editors strip comments from header files, some
   brief comments on the contents of owlrc.h are included here:

   The SD_ constants are used for the File Open, File Save, Input,
   Search and Replace dialogs.

   ID_FNAME, ID_FPATH, ID_FLIST and ID_DLIST are used for TFileDialog.

   ID_PROMPT, ID_INPUT, ID_SEARCHTEXT, ID_CASESENSITIVE, ID_REPLACETEXT,
   ID_REPLACEALL and ID_PROMPTONREPLACE are used by TSearchDialog.

   ID_EDITOR is used by TEditWindow.

   ID_MDICLIENT is the default id for TMDIClient and ID_FIRSTMDICHILD
   is the id of the first MDI child created by a TMDIClient.

   Note that the ID_ constants defined in OWL are all greater than or
   equal to ID_RESERVED. Any ID_ constants you use should be less than
   ID_RESERVED.

   Note that the CM_ constants defined in OWL are all greater than or
   equal to CM_RESERVED. Any CM_ constants you use should be less than
   CM_RESERVED.

*/

/* Constants for application messages */
#define WM_FIRST        0x0000  /* 0x0000- 0x7FFF window messages */
#define WM_INTERNAL     0x7F00  /* 0x7F00- 0x7FFF reserved for internal use */
#define ID_FIRST        0x8000  /* 0x8000- 0x8FFF child id messages */
#define ID_INTERNAL     0x8F00  /* 0x8F00- 0x8FFF reserved for internal use */
#define NF_FIRST        0x9000  /* 0x9000- 0x9FFF notification messages */
#define NF_INTERNAL     0x9F00  /* 0x9F00- 0x9FFF reserved for internal use */
#define CM_FIRST        0xA000  /* 0xA000- 0xFFFF command messages */
#define CM_INTERNAL     0xFF00  /* 0xFF00- 0xFFFF reserved for internal use */
#define WM_COUNT 	0x8000	/* num of window msgs */
#define ID_COUNT 	0x1000	/* num of child id msgs */
#define NF_COUNT 	0x1000	/* num of notification msgs */
#define CM_COUNT 	0x6000	/* num of command msgs */

#define WM_RESERVED             WM_INTERNAL - WM_FIRST

#define ID_RESERVED             ID_INTERNAL - ID_FIRST
#define ID_FIRSTMDICHILD        ID_RESERVED + 1
#define ID_MDICLIENT 	        ID_RESERVED + 2

#define CM_RESERVED             CM_INTERNAL - CM_FIRST


  /* Bit masks for TWindowsObject Flags */
#define WB_ALIAS              0x01
#define WB_AUTOCREATE         0x02
#define WB_FROMRESOURCE       0x04
#define WB_KBHANDLER          0x08
#define WB_MDICHILD           0x10
#define WB_MDIFRAME           0x20
#define WB_PREDEFINEDCLASS    0x40
#define WB_TRANSFER           0x80

  /* Error conditions */
#define EM_INVALIDCHILD         -1
#define EM_INVALIDCLIENT        -2
#define EM_INVALIDMAINWINDOW    -3
#define EM_INVALIDMODULE        -4
#define EM_INVALIDWINDOW        -5
#define EM_OUTOFMEMORY          -6

  /* Transfer function flags */
#define TF_GETDATA               0
#define TF_SETDATA               1
#define TF_SIZEDATA              2

/* Checkbox flags indicating check state */
#define BF_CHECKED      0x1
#define BF_GRAYED       0x2
#define BF_UNCHECKED    0x0

// define classes types for isA()

#define  moduleClass          (__firstOWLClass)
#define  applicationClass     (moduleClass + 1)
#define  scrollerClass        (applicationClass + 1)
#define  windowClass          (scrollerClass + 1)
#define  dialogClass          (windowClass + 1)


#if  defined(__DLL__)
#  define _EXPORT _export
#else
#  define _EXPORT _CLASSTYPE
#endif

#if  defined(__DLL__)
#  define _EXPFUNC _export
#else
#  define _EXPFUNC
#endif

#endif
