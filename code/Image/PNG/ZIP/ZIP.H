/*

 Copyright (C) 1990-1993 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  zip.h by Mark Adler.
 */


#define ZIP   /* for crypt.c:  include zip password functions, not unzip */

/* Set up portability */
#include "tailor.h"

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#ifndef WSIZE
#  define WSIZE  ((unsigned)32768)
#endif
/* Maximum window size = 32K. If you are really short of memory, compile
 * with a smaller WSIZE but this reduces the compression ratio for files
 * of size > WSIZE. WSIZE must be a power of two in the current implementation.
 */

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

/* Define malloc() and string functions */
#ifdef MODERN
#  ifdef __GO32__
     char *strlwr(char *); /* not in include files of djgcc */
#  endif
#  include <string.h>
#else /* !MODERN */
   voidp *malloc();
   char *getenv();
   long atol();
   char *strcpy();
   char *strcat();
   char *strchr();
   char *strrchr();
#  ifndef ZMEM
     char *memset();
     char *memcpy();
#  endif /* !ZMEM */
#endif /* ?MODERN */


/* Define fseek() commands */
#ifndef SEEK_SET
#  define SEEK_SET 0
#endif /* !SEEK_SET */

#ifndef SEEK_CUR
#  define SEEK_CUR 1
#endif /* !SEEK_CUR */


/* Forget FILENAME_MAX (incorrectly = 14 on some System V) */
#if defined(MSDOS) && !defined(WIN32) && !defined(OS2)
#  define FNMAX 256
#else
#  define FNMAX 1024
#endif


/* For setting stdout to binary */
#if defined(MSDOS) || defined(__human68k__)
#  include <io.h>
#  include <fcntl.h>
#  ifdef __GO32__
     int setmode(int, int); /* not in include files of djgcc */
#  endif
#endif /* MSDOS */


/* Types centralized here for easy modification */
#define local static            /* More meaningful outside functions */
typedef unsigned char uch;      /* unsigned 8-bit value */
typedef unsigned short ush;     /* unsigned 16-bit value */
typedef unsigned long ulg;      /* unsigned 32-bit value */


/* Lengths of headers after signatures in bytes */
#define LOCHEAD 26
#define CENHEAD 42
#define ENDHEAD 18

/* Structures for in-memory file information */
struct zlist {
  /* See central header in zipfile.c for what vem..off are */
  ush vem, ver, flg, how;
  ulg tim, crc, siz, len;
  extent nam, ext, cext, com;   /* offset of ext must be >= LOCHEAD */
  ush dsk, att, lflg;           /* offset of lflg must be >= LOCHEAD */
  ulg atx, off;
  char *name;                   /* File name in zip file */
  char *extra;                  /* Extra field (set only if ext != 0) */
  char *cextra;                 /* Extra in central (set only if cext != 0) */
  char *comment;                /* Comment (set only if com != 0) */
  char *zname;                  /* Name for new zip file header */
  int mark;                     /* Marker for files to operate on */
  int trash;                    /* Marker for files to delete */
  int dosflag;                  /* Set to force MSDOS file attributes */
  struct zlist far *nxt;        /* Pointer to next header in list */
};
struct flist {
  char *name;                   /* Pointer to zero-delimited name */
  char *zname;                  /* Name used for zip file headers */
  int dosflag;                  /* Set to force MSDOS file attributes */
  struct flist far * far *lst;  /* Pointer to link pointing here */
  struct flist far *nxt;        /* Link to next name */
};
struct plist {
  char *zname;                  /* Name used for zip file headers */
  int select;                   /* Selection flag ('i' or 'x') */
};

/* internal file attribute */
#define UNKNOWN (-1)
#define BINARY  0
#define ASCII   1

/* Error return codes and PERR macro */
#include "ziperr.h"


/* Public globals */
extern uch upper[256];          /* Country dependent case map table */
extern uch lower[256];
extern char errbuf[];           /* Handy place to build error messages */
extern int recurse;             /* Recurse into directories encountered */
extern int dispose;             /* Remove files after put in zip file */
extern int pathput;             /* Store path with name */

#define BEST -1                 /* Use best method (deflation or store) */
#define STORE 0                 /* Store method */
#define DEFLATE 8               /* Deflation method*/
extern int method;              /* Restriction on compression method */

extern int dosify;              /* Make new entries look like MSDOS */
extern char *special;           /* Don't compress special suffixes */
extern int verbose;             /* Report oddities in zip file structure */
extern int fix;                 /* Fix the zip file */
extern int level;               /* Compression level */
extern int translate_eol;       /* Translate end-of-line LF -> CR LF */
#ifdef VMS
   extern int vmsver;           /* Append VMS version number to file names */
   extern int vms_native;       /* Store in VMS formait */
#endif /* VMS */
#if defined(OS2) || defined(WIN32)
   extern int use_longname_ea;   /* use the .LONGNAME EA as the file's name */
#endif
extern int hidden_files;        /* process hidden and system files */
extern int volume_label;        /* add volume label */
extern int dirnames;            /* include directory names */
extern int linkput;             /* Store symbolic links as such */
extern int noisy;               /* False for quiet operation */
extern char *key;               /* Scramble password or NULL */
extern char *tempath;           /* Path for temporary files */
extern FILE *mesg;              /* Where informational output goes */
extern char *zipfile;           /* New or existing zip archive (zip file) */
extern ulg zipbeg;              /* Starting offset of zip structures */
extern ulg cenbeg;              /* Starting offset of central directory */
extern struct zlist far *zfiles;/* Pointer to list of files in zip file */
extern extent zcount;           /* Number of files in zip file */
extern extent zcomlen;          /* Length of zip file comment */
extern char *zcomment;          /* Zip file comment (not zero-terminated) */
extern struct zlist far **zsort;/* List of files sorted by name */
extern ulg tempzn;              /* Count of bytes written to output zip file */
extern struct flist far *found; /* List of names found */
extern struct flist far * far *fnxt;    /* Where to put next in found list */
extern extent fcount;           /* Count of names in found list */

extern struct plist *patterns;  /* List of patterns to be matched */
extern int pcount;              /* number of patterns */
extern int icount;              /* number of include only patterns */

/* Diagnostic functions */
#ifdef DEBUG
# ifdef MSDOS
#  undef  stderr
#  define stderr stdout
# endif
#  define diag(where) fprintf(stderr, "zip diagnostic: %s\n", where)
#  define Assert(cond,msg) {if(!(cond)) error(msg);}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#else
#  define diag(where)
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif


/* Public function prototypes */

        /* in zip.c, zipcloak.c, or zipsplit.c */
void warn  OF((char *, char *));
void err   OF((int c, char *h));
void error OF((char *h));

        /* in zipup.c */
int zipcopy OF((struct zlist far *, FILE *, FILE *));
#ifndef UTIL
   int percent OF((ulg, ulg));
   int zipup OF((struct zlist far *, FILE *));
   int file_read OF((char *buf, unsigned size));
#endif /* !UTIL */

        /* in zipfile.c */
#ifndef UTIL
   struct zlist far *zsearch OF((char *));
   int trash OF((void));
#endif /* !UTIL */
char *ziptyp OF((char *));
int readzipfile OF((void));
int putlocal OF((struct zlist far *, FILE *));
int putextended OF((struct zlist far *, FILE *));
int putcentral OF((struct zlist far *, FILE *));
int putend OF((int, ulg, ulg, extent, char *, FILE *));

        /* in fileio.c */
#ifndef UTIL
#  if defined(MSDOS) || defined(VMS) || defined(__human68k__)
     int wild OF((char *));
#  endif
   char *getnam OF((char *));
   struct flist far *fexpel OF((struct flist far *));
   char *in2ex OF((char *));
   char *ex2in OF((char *, int, int *));
   int check_dup OF((void));
   int filter OF((char *name));
   int procname OF((char *));
   void stamp OF((char *, ulg));
   ulg dostime OF((int, int, int, int, int, int));
   ulg filetime OF((char *, ulg *, long *));
   int set_extra_field OF((struct zlist *z));
   int issymlnk OF((ulg a));
#  ifdef S_IFLNK
#    define rdsymlnk(p,b,n) readlink(p,b,n)
/*   extern int readlink OF((char *, char *, int)); */
#  else /* !S_IFLNK */
#    define rdsymlnk(p,b,n) (0)
#  endif /* !S_IFLNK */
   int deletedir OF((char *));
#endif /* !UTIL */
int destroy OF((char *));
int replace OF((char *, char *));
int getfileattr OF((char *));
int setfileattr OF((char *, int));
char *tempname OF((char *));
int fcopy OF((FILE *, FILE *, ulg));
#ifdef ZMEM
   char *memset OF((char *, int, unsigned int));
   char *memcpy OF((char *, char *, unsigned int));
   int memcmp OF((char *, char *, unsigned int));
#endif /* ZMEM */

        /* in util.c */
char *isshexp OF((char *));
int   shmatch OF((char *, char *));
#ifdef MSDOS
   int dosmatch OF((char *, char *));
#endif /* MSDOS */
void     init_upper OF((void));
int      namecmp    OF((char *string1, char *string2));
voidp far **search  OF((voidp *, voidp far **, extent,
                       int (*)(const voidp *, const voidp far *)));
ulg updcrc OF((char *, extent));

extern ulg crc_32_tab[];
#define CRC32(c, b) (crc_32_tab[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))

#ifndef UTIL
        /* in deflate.c */
void lm_init OF((int pack_level, ush *flags));
void lm_free OF((void));
ulg  deflate OF((void));

        /* in trees.c */
void ct_init     OF((ush *attr, int *method));
int  ct_tally    OF((int dist, int lc));
ulg  flush_block OF((char far *buf, ulg stored_len, int eof));

        /* in bits.c */
void     bi_init    OF((FILE *zipfile));
void     send_bits  OF((int value, int length));
unsigned bi_reverse OF((unsigned value, int length));
void     bi_windup  OF((void));
void     copy_block OF((char far *buf, unsigned len, int header));
int      seekable   OF((void));
extern   int (*read_buf) OF((char *buf, unsigned size));
ulg     memcompress OF((char *tgt, ulg tgtsize, char *src, ulg srcsize));

#endif /* !UTIL */

/* end of zip.h */
