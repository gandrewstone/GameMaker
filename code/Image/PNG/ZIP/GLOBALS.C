/*

 Copyright (C) 1990-1993 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  globals.c by Mark Adler.
 */

#define GLOBALS         /* include definition of errors[] in zip.h */
#define UTIL            /* do not declare the read_buf variable */

#include "zip.h"


/* Handy place to build error messages */
char errbuf[FNMAX+81];

/* Argument processing globals */
int recurse = 0;        /* 1=recurse into directories encountered */
int dispose = 0;        /* 1=remove files after put in zip file */
int pathput = 1;        /* 1=store path with name */
int method = BEST;      /* one of BEST, DEFLATE (only), or STORE (only) */
int dosify = 0;         /* 1=make new entries look like MSDOS */
int verbose = 0;        /* 1=report oddities in zip file structure */
int fix = 0;            /* 1=fix the zip file */
int level = 6;          /* 0=fastest compression, 9=best compression */
int translate_eol = 0;  /* Translate end-of-line LF -> CR LF */
#ifdef VMS
   int vmsver = 0;      /* 1=append VMS version number to file names */
   int vms_native = 0;  /* 1=store in VMS format */
#endif /* VMS */
#if defined(OS2) || defined(WIN32)
   int use_longname_ea = 0; /* 1=use the .LONGNAME EA as the file's name */
#endif
int hidden_files = 0;   /* process hidden and system files */
int volume_label = 0;   /* add volume label */
int dirnames = 1;       /* include directory entries by default */
int linkput = 0;        /* 1=store symbolic links as such */
int noisy = 1;          /* 0=quiet operation */
char *special = ".Z:.zip:.zoo:.arc:.lzh:.arj"; /* List of special suffixes */
char *key = NULL;       /* Scramble password if scrambling */
char *tempath = NULL;   /* Path for temporary files */
FILE *mesg;             /* stdout by default, stderr for piping */

/* Zip file globals */
char *zipfile;          /* New or existing zip archive (zip file) */
ulg zipbeg;             /* Starting offset of zip structures */
ulg cenbeg;             /* Starting offset of central directory */
struct zlist far *zfiles = NULL;  /* Pointer to list of files in zip file */
extent zcount;          /* Number of files in zip file */
extent zcomlen;         /* Length of zip file comment */
char *zcomment;         /* Zip file comment (not zero-terminated) */
struct zlist far **zsort;       /* List of files sorted by name */
ulg tempzn;             /* Count of bytes written to output zip file */

/* Files to operate on that are not in zip file */
struct flist far *found = NULL; /* List of names found */
struct flist far * far *fnxt = &found;
                        /* Where to put next name in found list */
extent fcount;          /* Count of files in list */

/* Patterns to be matched */
struct plist *patterns = NULL; /* List of patterns to be matched */
int pcount = 0;                /* number of patterns */
int icount = 0;                /* number of include only patterns */
