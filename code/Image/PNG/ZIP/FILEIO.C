/*

 Copyright (C) 1990-1993 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  fileio.c by Mark Adler.
 *
 *  This file has become a big mess over time, and will be split in the
 *  next version, one file per operating system.
 */

#include "zip.h"

#include <time.h>

#ifdef WIN32
#  include <sys/utime.h>
#  include <windows.h> /* for findfirst/findnext */
   char *GetLongPathEA OF((char *name));
#endif

#ifdef MACOS
#  define EXDEV 1
#endif

#ifdef OSF
#  define EXDEV 18    /* avoid a bug in the DEC OSF/1 header files. */
#else
#  include <errno.h>
#endif

#ifdef MINIX
#  ifdef S_IWRITE
#    undef S_IWRITE
#  endif /* S_IWRITE */
#  define S_IWRITE S_IWUSR
#endif /* S_IWUSR */

#ifdef ATARI_ST
#  undef MSDOS
#endif

#if defined(MSDOS) && !defined(__GO32__)
#  include <io.h>
#  ifdef __TURBOC__
#    include <dir.h>
#  else /* !__TURBOC__ */
#    if !defined(__EMX__) && !defined(__WATCOMC__)
#      include <direct.h>
#    endif
#  endif /* ?__TURBOC__ */
#  define link rename
#  if defined(OS2) || defined(WIN32)
#    define MATCH shmatch
#  else
#    define MATCH dosmatch
#  endif
#else /* !MSDOS */
   extern int errno;    /* error number from system functions */
#  ifdef TOPS20
#    define PATH_START '<'
#    define PATH_END   '>'
#    define link rename
#  endif
#  ifdef VMS
#    define PATH_START '['
#    define PATH_END ']'
#    define RMDIR
#    define link rename
#    include "VMSmunch.h"
#  endif /* VMS */
#  ifdef MACOS
#    define link rename
#    define mktemp tmpnam
#  endif
#  define MATCH shmatch
#endif /* ?MSDOS */

#ifdef ATARI_ST
#  define MSDOS 1
#endif

#ifdef UTS
#  define RMDIR
#endif /* UTS */

#ifdef AMIGA
#  define link rename
#  define utime FileDate
#  ifdef __SASC_60
#    include <dos.h>               /* SAS/C 6.x , pulled by wild() */
#    include <sys/dir.h>
#    define direct dirent
#    define disk_not_mounted 0  
#  else
#    define dirent direct
#  endif
#  ifdef MODERN
#    include <clib/exec_protos.h>
#    include <clib/dos_protos.h>
#  endif
#  ifdef AZTEC_C
#    include <pragmas/exec_lib.h>
#    include <pragmas/dos_lib.h>
#  endif
#endif 

#ifdef __human68k__
#  include <jctype.h>
#  define link rename
#endif

/* Extra malloc() space in names for cutpath() */
#if (defined(VMS) || defined(TOPS20))
#  define PAD 5         /* may have to change .FOO] to ]FOO.DIR;1 */
#else
#  define PAD 0
#  define PATH_END '/'
#endif


#ifdef VMS
#  include <descrip.h>
#  include <rms.h>
#endif

/* For directory access. */

#ifndef UTIL

#ifdef __human68k__
#include <sys/dos.h>
#endif

#if defined(SYSV) || defined(__GO32__) || defined(__386BSD__) || defined(__human68k__)
/* use readdir() */
#  include <dirent.h>
#  define dstrm DIR
#  define direct dirent
#else /* !SYSV && !__GO32__ && !__386BSD__ && !__human68k__*/

#ifdef DIRENT                   /* use getdents() */
#ifndef TOPS20
#  if defined(MINIX) || defined(OSF)
#    include <dirent.h>
#  else /* !MINIX */
#    include <sys/dirent.h>
#  endif /* ?MINIX */
#  define direct dirent
#  ifdef MINIX
     int getdents OF((int, char *, unsigned));
#  else /* !MINIX */
     int getdents OF((int, char *, int));
#  endif /* ?MINIX */
#  define DBSZ 4096     /* This has to be bigger than a directory block */
   typedef struct {     /* directory stream buffer */
     int f;             /* file descriptor for the directory "file" */
     char *p;           /* pointer to next entry in buffer */
     char *q;           /* pointer after end of buffer contents */
     char b[DBSZ];              /* buffer */
   } dstrm;

#else /* TOPS20 */
#  define TRUE 1
#  define FALSE 0
#  define O_RDONLY (0)
#  define O_T20_WILD (1<<18)
#  include <monsym.h>    /* Get amazing monsym() macro */
   extern int jsys(), fstat();
   extern char *getcwd();
   extern int _gtjfn(), _rljfn();
#  define JSYS_CLASS    0070000000000
#  define FLD(val,mask) (((unsigned)(val)*((mask)&(-(mask))))&(mask))
#  define _DEFJS(name,class) (FLD(class, JSYS_CLASS) | (monsym(name)&0777777))
#  define JFNS  _DEFJS("JFNS%", 1)
#  define GNJFN _DEFJS("GNJFN%", 0)
   static int wfopen(), wfnext(), strlower(), strupper();
   static char *wfname();
   typedef struct {
     int  wfjfn;
     int  more;
   } dstrm;
#endif /* ?TOPS20 */

#else /* !DIRENT */             /* use opendir(), etc. */
#  if defined(CONVEX) || defined(ultrix)
#    include <dirent.h>
#    ifdef direct
#      undef direct /* ultrix 4.2, at least if !__POSIX */
#    endif
#    define direct dirent
#  endif /* CONVEX || ultrix */
#  ifdef NDIR
#    include "ndir.h"           /* for HPUX */
#  else /* !NDIR */
#    ifdef MSDOS
#     ifdef OS2
#      include "os2zip.h"
#     else /* !OS2 */
#      ifdef WIN32
#        include "ntzip.h"
#      endif
#      ifndef ATARI_ST
#        include <dos.h>
#      endif
#      ifdef __TURBOC__
#        define FATTR (hidden_files ? FA_HIDDEN+FA_SYSTEM+FA_DIREC : FA_DIREC)
#        define FFIRST(n,d,a)   findfirst(n,(struct ffblk *)d,a)
#        define FNEXT(d)        findnext((struct ffblk *)d)
#      else /* !__TURBOC__ */
#        define FATTR (hidden_files ? _A_HIDDEN+_A_SYSTEM+_A_SUBDIR :_A_SUBDIR)
#        define FA_LABEL _A_VOLID
#        define FFIRST(n,d,a)   _dos_findfirst(n,a,(struct find_t *)d)
#        define FNEXT(d)        _dos_findnext((struct find_t *)d)
#      endif /* ?__TURBOC__ */
       typedef struct direct {
         char   reserved [21];
         char   ff_attrib;
         short  ff_ftime;
         short  ff_fdate;
         long   size;
#      ifndef WIN32
         char d_name[13];
         int d_first;
#      else
         char d_name[MAX_PATH];
         int d_first;
         HANDLE d_hFindFile;
#      endif /* WIN32 */
       } DIR;
       typedef DIR ff_dir;
#      define ff_name d_name /* ff_name is used by DJGPP */
#     endif /* ?OS2 */
#    else /* !MSDOS */
#      ifdef VMS
#        include <ssdef.h>
         typedef struct direct {
             int d_wild;                /* flag for wildcard vs. non-wild */
             struct FAB fab;
             struct NAM nam;
             char d_qualwildname[NAM$C_MAXRSS + 1];
             char d_name[NAM$C_MAXRSS + 1];
         } DIR;
#      else /* !VMS */
#        ifdef MACOS
           typedef struct direct {
             int d_wild;                /* flag for wildcard vs. non-wild */
             char *d_name;
          } DIR;
#        endif
#        if defined(M_XENIX) || defined(SYSNDIR)
#          include <sys/ndir.h>
#        else /* !M_XENIX */
#          ifndef AZTEC_C
#            include <sys/dir.h>
#          endif
#        endif /* ?M_XENIX */
#        ifdef NODIR                    /* for AT&T 3B1 + Amiga non SAS */
#          define dirent direct
           typedef FILE DIR;
#        endif /* NODIR */
#      endif /* ?VMS */
#    endif /* ?MSDOS */
#  endif /* ?NDIR */
#  define dstrm DIR
#  if defined(MSDOS) && !defined(OS2)
#    ifndef __GO32__
       DIR *opendir OF((const char *));
       struct direct *readdir OF((DIR *));
#    endif
#    ifndef WIN32
       char *getVolumeLabel OF((int drive, ulg *vtime, ulg *vmode));
#    endif
#  endif
#endif /* ?DIRENT */
#endif /* ?SYSV */
#endif /* !UTIL */

#ifdef __GO32__
   typedef struct ffblk ff_dir;
#  define FFIRST(n,d,a)   findfirst(n,(struct ffblk *)d,a)
#endif

#define MSDOS_DIR_ATTR 0x10

/* Library functions not in (most) header files */

#ifdef TOPS20
   extern int stat(), chmod(), toupper(), tolower();
#endif

#if defined(__IBMC__) || defined(__WATCOMC__)
#  define NO_MKTEMP
#endif
char *mktemp OF((char *));

#ifdef __GO32__
#  include <dos.h>
#else
  /* int link OF((const char *, const char *)); */
  /* int unlink OF((const char *)); */
  /* int chmod OF((const char *, int)); */
  /* For many targets, chmod is already defined by sys/stat.h, and second
   * parameter is an unsigned long.
   */
#endif


#ifndef UTIL    /* the companion #endif is a bit of ways down ... */

#if !defined(__TURBOC__) && !defined(WIN32) && !defined(sgi)
   int utime OF((char *, time_t *));
#endif
#ifndef MSDOS
   /* int open OF((char *, int, ...)); */
   /* int close OF((int)); */
#  ifndef RMDIR
     /* int rmdir OF((const char *)); */
#  endif /* !RMDIR */
#endif /* !MSDOS */


/* Local globals (kinda like "military intelligence" or "broadcast quality") */
local struct stat zipstatb;
local int zipstate = -1;
/* -1 unknown, 0 old zip file exists, 1 new zip file */

local char *label = NULL;
local ulg label_time = 0;
local ulg label_mode = 0;

#ifdef VMS
  typedef int statime;
#else /* !VMS */
  typedef time_t statime;
#endif /* ?VMS */

/* Local functions */
#ifdef PROTO
#  ifdef VMS
     local void vms_wild(char *, dstrm *);
#  endif /* VMS */
#  ifdef DIRENT
     local dstrm *opend(char *);
     local void closed(dstrm *);
#  endif /* DIRENT */
   local char *readd(dstrm *);
   local int fqcmp(const voidp *, const voidp *);
   local int fqcmpz(const voidp *, const voidp *);
   local char *last(char *);
   local char *msname(char *);
#  ifdef VMS
     local char *strlower(char *);
     local char *strupper(char *);
#  endif /* VMS */
   local int newname(char *, int);
   local void inctime(struct tm *);
   local ulg unix2dostime(statime *);
#  if !defined(__TURBOC__) && !defined(OS2) && !defined(__GO32__)
     local int cmptime(struct tm *, struct tm *);
     local time_t invlocal(struct tm *);
#  endif /* !__TURBOC__ */
#endif /* PROTO */


#if defined(MSDOS) && !defined(OS2) && !defined(__GO32__)
dstrm *opendir(n)
const char *n;                /* directory to open */
/* Start searching for files in the MSDOS directory n */
{
  dstrm *d;             /* malloc'd return value */
  char *p;              /* malloc'd temporary string */

  if ((d = (dstrm *)malloc(sizeof(dstrm))) == NULL ||
      (p = malloc(strlen(n) + 5)) == NULL) {
    if (d != NULL) free((void *) d);
    return NULL;
  }
  strcat(strcpy(p, n), "/");
  strcat(p, "*.*"); /* don't use strcat in one step, TOPS20 is confused */

#ifdef WIN32
  {
    WIN32_FIND_DATA fd;
    DWORD dwAttr;

    if ( INVALID_HANDLE_VALUE == (d->d_hFindFile = FindFirstFile(p, &fd)))
    {
      free((voidp *)p);
      return NULL;
    }
    strcpy(d->d_name, fd.cFileName);

    if (-1 != (dwAttr = GetFileAttributes(fd.cFileName)))
    {
      if (!hidden_files &&
         (FILE_ATTRIBUTE_SYSTEM == (dwAttr & FILE_ATTRIBUTE_SYSTEM) ||
          FILE_ATTRIBUTE_HIDDEN == (dwAttr & FILE_ATTRIBUTE_HIDDEN)))
      {
          free ((voidp *)p);
          d->d_first = 0;
          return d;
      }
    }
  }
#else
  if (FFIRST(p, d, FATTR))
  {
    free((voidp *)p);
    return NULL;
  }
  free((voidp *)p);
#endif
  d->d_first = 1;
  return d;
}

struct direct *readdir(d)
dstrm *d;               /* directory stream to read from */
/* Return pointer to first or next directory entry, or NULL if end. */
{
  if (d->d_first)
    d->d_first = 0;
  else
#ifdef WIN32
  {
    WIN32_FIND_DATA fd;
    DWORD dwAttr;

    if (!FindNextFile(d->d_hFindFile, &fd))
        return NULL;

    strcpy(d->d_name, fd.cFileName);

    if (!hidden_files &&
       (-1 != (dwAttr = GetFileAttributes(fd.cFileName))) &&
       (FILE_ATTRIBUTE_SYSTEM == (dwAttr & FILE_ATTRIBUTE_SYSTEM) ||
        FILE_ATTRIBUTE_HIDDEN == (dwAttr & FILE_ATTRIBUTE_HIDDEN)))
    {
        return (readdir(d));
    }
  }
#else /* !WIN32 */
    if (FNEXT(d))
      return NULL;
#endif
  return (struct direct *)d;

}
#ifdef WIN32
void closedir(d)
dstrm *d;
{
        FindClose(d->d_hFindFile);
        free(d);
}
#else
#  define closedir free
#endif

#endif /* MSDOS && !OS2 && !__GO32__ */


#ifdef VMS

/*---------------------------------------------------------------------------

    _vms_findfirst() and _vms_findnext(), based on public-domain DECUS C
    fwild() and fnext() routines (originally written by Martin Minow, poss-
    ibly modified by Jerry Leichter for bintnxvms.c), were written by Greg
    Roelofs and are still in the public domain.  Routines approximate the
    behavior of MS-DOS (MSC and Turbo C) findfirst and findnext functions.

  ---------------------------------------------------------------------------*/

static char wild_version_part[10]="\0";

local void vms_wild(p, d)
char *p;
dstrm *d;
{
  /*
   * Do wildcard setup
   */
  /* set up the FAB and NAM blocks. */
  d->fab = cc$rms_fab;             /* initialize fab */
  d->nam = cc$rms_nam;             /* initialize nam */

  d->fab.fab$l_nam = &d->nam;           /* fab -> nam */
  d->fab.fab$l_fna = p;                 /* argument wild name */
  d->fab.fab$b_fns = strlen(p);         /* length */

  d->nam.nam$l_esa = d->d_qualwildname; /* qualified wild name */
  d->nam.nam$b_ess = NAM$C_MAXRSS;      /* max length */
  d->nam.nam$l_rsa = d->d_name;         /* matching file name */
  d->nam.nam$b_rss = NAM$C_MAXRSS;      /* max length */

  /* parse the file name */
  if (sys$parse(&d->fab) != RMS$_NORMAL)
    return;
  /* Does this replace d->fab.fab$l_fna with a new string in its own space?
     I sure hope so, since p is free'ed before this routine returns. */

  /* have qualified wild name (i.e., disk:[dir.subdir]*.*); null-terminate
   * and set wild-flag */
  d->d_qualwildname[d->nam.nam$b_esl] = '\0';
  d->d_wild = (d->nam.nam$l_fnb & NAM$M_WILDCARD)? 1 : 0;   /* not used... */
#ifdef DEBUG
  fprintf(mesg, "  incoming wildname:  %s\n", p);
  fprintf(mesg, "  qualified wildname:  %s\n", d->d_qualwildname);
#endif /* DEBUG */
}

dstrm *opendir(n)
char *n;                /* directory to open */
/* Start searching for files in the VMS directory n */
{
  char *c;              /* scans VMS path */
  dstrm *d;             /* malloc'd return value */
  int m;                /* length of name */
  char *p;              /* malloc'd temporary string */

  if ((d = (dstrm *)malloc(sizeof(dstrm))) == NULL ||
      (p = malloc((m = strlen(n)) + 4)) == NULL)
    return NULL;
  /* Directory may be in form "[DIR.SUB1.SUB2]" or "[DIR.SUB1]SUB2.DIR;1".
     If latter, convert to former. */
  if (m > 0  &&  *(c = strcpy(p,n)+m-1) != ']')
  {
    while (--c > p  &&  *c != ';')
      ;
    if (c-p < 5  ||  strncmp(c-4, ".DIR", 4))
    {
      free((voidp *)d);  free((voidp *)p);
      return NULL;
    }
    c -= 3;
    *c-- = '\0';        /* terminate at "DIR;#" */
    *c = ']';           /* "." --> "]" */
    while (c > p  &&  *--c != ']')
      ;
    *c = '.';           /* "]" --> "." */
  }
  strcat(p, "*.*");
  strcat(p, wild_version_part);
  vms_wild(p, d);       /* set up wildcard */
  free((voidp *)p);
  return d;
}

struct direct *readdir(d)
dstrm *d;               /* directory stream to read from */
/* Return pointer to first or next directory entry, or NULL if end. */
{
  int r;                /* return code */

  do {
    d->fab.fab$w_ifi = 0;       /* internal file index:  what does this do? */

    /* get next match to possible wildcard */
    if ((r = sys$search(&d->fab)) == RMS$_NORMAL)
    {
        d->d_name[d->nam.nam$b_rsl] = '\0';   /* null terminate */
        return (struct direct *)d;   /* OK */
    }
  } while (r == RMS$_PRV);
  return NULL;
}
#  define closedir free
#endif /* VMS */


#ifdef NODIR                    /* for AT&T 3B1 */
/*
**  Apparently originally by Rich Salz.
**  Cleaned up and modified by James W. Birdsall.
*/

#  define opendir(path) fopen(path, "r")
 
struct direct *readdir(dirp)
DIR *dirp;
{
  static struct direct entry;

  if (dirp == NULL) 
    return NULL;
  for (;;)
    if (fread (&entry, sizeof (struct direct), 1, dirp) == 0) 
      return NULL;
    else if (entry.d_ino) 
      return (&entry);
} /* end of readdir() */

#  define closedir(dirp) fclose(dirp)
#endif /* NODIR */


#ifdef DIRENT
# ifdef TOPS20
local dstrm *opend(n)
char *n;                /* directory name to open */
/* Open the directory *n, returning a pointer to an allocated dstrm, or
   NULL if error. */
{
    dstrm *d;             /* pointer to malloc'ed directory stream */
    char    *c;                         /* scans TOPS20 path */
    int     m;                          /* length of name */
    char    *p;                         /* temp string */

    if (((d = (dstrm *)malloc(sizeof(dstrm))) == NULL) ||
        ((p = (char *)malloc((m = strlen(n)) + 4)) == NULL)) {
            return NULL;
    }

/* Directory may be in form "<DIR.SUB1.SUB2>" or "<DIR.SUB1>SUB2.DIRECTORY".
** If latter, convert to former. */

    if ((m > 0)  &&  (*(c = strcpy(p,n) + m-1) != '>')) {
        c -= 10;
        *c-- = '\0';        /* terminate at "DIRECTORY.1" */
        *c = '>';           /* "." --> ">" */
        while ((c > p)  &&  (*--c != '>'));
        *c = '.';           /* ">" --> "." */
    }
    strcat(p, "*.*");
    if ((d->wfjfn = wfopen(p)) == 0) {
        free((voidp *)d);
        free((voidp *)p);
        return NULL;
    }
    free((voidp *)p);
    d->more = TRUE;
    return (d);
}
#else /* !TOPS20 */

local dstrm *opend(n)
char *n;                /* directory name to open */
/* Open the directory *n, returning a pointer to an allocated dstrm, or
   NULL if error. */
{
    dstrm *d;             /* pointer to malloc'ed directory stream */

  if ((d = (dstrm *)malloc(sizeof(dstrm))) == NULL)
    return NULL;
  if ((d->f = open(n, 0, 0)) < 0)               /* open directory */
    return NULL;
  d->p = d->q = d->b;                           /* buffer is empty */
  return d;
}
# endif /* ?TOPS20 */
#else /* !DIRENT */
#  define opend opendir                         /* just use opendir() */
#endif /* ?DIRENT */


local char *readd(d)
dstrm *d;               /* directory stream to read from */
/* Return a pointer to the next name in the directory stream d, or NULL if
   no more entries or an error occurs. */
{
#ifdef TOPS20
  char    *p;
  if ((d->more == FALSE) || ((p = wfname(d->wfjfn)) == NULL)) {
      return NULL;
  }
  if (wfnext(d->wfjfn) == 0) {
      d->more = FALSE;
  }
  return p;

#else /* !TOPS20 */
  struct direct *e;     /* directory entry read */

# ifdef DIRENT
  int n;                /* number of entries read by getdents */

  if (d->p >= d->q)                             /* if empty, fill buffer */
    if ((n = getdents(d->f, d->b, DBSZ)) <= 0)
      return NULL;
    else
      d->q = n + (d->p = d->b);
  e = (struct direct *)(d->p);                  /* point to entry */
  d->p += ((struct direct *)(d->p))->d_reclen;  /* advance */
  return e->d_name;                             /* return name */
# else /* !DIRENT */
  e = readdir(d);
  return e == NULL ? (char *)NULL : e->d_name;
# endif /* ?DIRENT */
#endif /* ?TOPS20 */
}


#ifdef DIRENT
local void closed(d)
dstrm *d;               /* directory stream to close */
/* Close the directory stream */
{
#ifndef TOPS20
  close(d->f);
#endif
  free((voidp *)d);
}
#else /* !DIRENT */
#  define closed closedir
#endif /* ?DIRENT */

#ifdef TOPS20
/* Wildcard filename routines */

/* WFOPEN - open wild card filename
**      Returns wild JFN for filespec, 0 if failure.
*/
static int
wfopen(name)
char *name;
{
    return (_gtjfn(name, (O_RDONLY | O_T20_WILD)));
}

/* WFNAME - Return filename for wild JFN
**      Returns pointer to dynamically allocated filename string
*/
static char *
wfname(jfn)
int jfn;
{
    char *fp, fname[200];
    int ablock[5];

    ablock[1] = (int) (fname - 1);
    ablock[2] = jfn & 0777777;  /* jfn, no flags */
    ablock[3] = 0111110000001;  /* DEV+DIR+NAME+TYPE+GEN, punctuate */
    if (!jsys(JFNS, ablock))
        return NULL;            /* something bad happened */
    if ((fp = (char *)malloc(strlen(fname) + 1)) == NULL) {
        return NULL;
    }
    strcpy(fp, fname);          /* copy the file name here */
    return fp;
}

/* WFNEXT - Make wild JFN point to next real file
**      Returns success or failure (not JFN)
*/
static int
wfnext(jfn)
int jfn;
{
    int ablock[5];

    ablock[1] = jfn;            /* save jfn and flags */
    return jsys(GNJFN, ablock);
}
#endif /* TOPS20 */
  

#ifdef __human68k__
int wild(w)
char *w;
{
  struct _filbuf inf;
  int r;
  extern int _hupair;
  char name[FNMAX];
  char *p;

  if (_hupair)
    return procname(w);         /* if argv's passed hupair, don't glob */
  strcpy(name, w);
  _toslash(name);
  if ((p = strrchr(name, '/')) == NULL && (p = strrchr(name, ':')) == NULL)
    p = name;
  else
    p++;
  if (_dos_files (&inf, w, 0xff) < 0)
    return ZE_MISS;
  do {
    strcpy(p, inf.name);
    r = procname(name);
    if (r != ZE_OK)
      return r;
  } while (_dos_nfiles(&inf) >= 0);

  return ZE_OK;
}
#endif

#if defined(MSDOS) && !defined(OS2) && !defined(WIN32)

char *getVolumeLabel(drive, vtime, vmode)
  int drive;  /* drive name: 'A' .. 'Z' or '\0' for current drive */
  ulg *vtime; /* volume label creation time (DOS format) */
  ulg *vmode; /* volume label file mode */

/* If a volume label exists for the given drive, return its name and
   set its time and mode. The returned name must be static data. */
{
  static char vol[14];
  ff_dir d;

  if (drive) {
    vol[0] = (char)drive;
    strcpy(vol+1, ":/");
  } else {
    strcpy(vol, "/");
  }
  strcat(vol, "*.*");
  if (FFIRST(vol, &d, FA_LABEL) == 0) {
    strncpy(vol, d.ff_name, sizeof(vol)-1);
    *vtime = ((ulg)d.ff_fdate << 16) | ((ulg)d.ff_ftime & 0xffff);
    *vmode = (ulg)d.ff_attrib;
    return vol;
  }
  return NULL;
}

#endif /*  MSDOS && !OS2 && !WIN32 */

#if defined(MSDOS) || defined(OS2)

int wild(w)
char *w;                /* path/pattern to match */
/* If not in exclude mode, expand the pattern based on the contents of the
   file system.  Return an error code in the ZE_ class. */
{
# ifndef __GO32__
  dstrm *d;             /* stream for reading directory */
  char *e;              /* name found in directory */
  int r;                /* temporary variable */
  char *n;              /* constructed name from directory */
# endif /* __GO32__ */
  int f;                /* true if there was a match */
  char *a;              /* alloc'ed space for name */
  char *p;              /* path */
  char *q;              /* name */
  char v[5];            /* space for device current directory */

# ifndef WIN32
  if (volume_label == 1) {
    volume_label = 2;
    label = getVolumeLabel(w[1] == ':' ? to_up(w[0]) : '\0',
                           &label_time, &label_mode);
    if (label != NULL) {
       newname(label, 0);
    }
    if (w[1] == ':' && w[2] == '\0') return ZE_OK;
    /* "zip -$ foo a:" can be used to force drive name */
  }
# endif
  /* Allocate and copy pattern */
  if ((p = a = malloc(strlen(w) + 1)) == NULL)
    return ZE_MEM;
  strcpy(p, w);

  /* Normalize path delimiter as '/'. */
  for (q = p; *q; q++)                  /* use / consistently */
    if (*q == '\\')
      *q = '/';

  /* Only name can have special matching characters */
  if ((q = isshexp(p)) != NULL &&
      (strrchr(q, '/') != NULL || strrchr(q, ':') != NULL))
  {
    free((voidp *)a);
    return ZE_PARMS;
  }

  /* Separate path and name into p and q */
  if ((q = strrchr(p, '/')) != NULL && (q == p || q[-1] != ':'))
  {
    *q++ = 0;                           /* path/name -> path, name */
    if (*p == 0)                        /* path is just / */
      p = strcpy(v, "/.");
  }
  else if ((q = strrchr(p, ':')) != NULL)
  {                                     /* has device and no or root path */
    *q++ = 0;
    p = strcat(strcpy(v, p), ":");      /* copy device as path */
    if (*q == '/')                      /* -> device:/., name */
    {
      strcat(p, "/");
      q++;
    }
    strcat(p, ".");
  }
  else                                  /* no path or device */
  {
    q = p;
    p = strcpy(v, ".");
  }
  /* I can't understand Mark's code so I am adding a hack here to get
   * "zip -r foo ." to work. Allow the dubious "zip -r foo .." but
   * reject "zip -rm foo ..".
   */
  if (recurse && (strcmp(q, ".") == 0 ||  strcmp(q, "..") == 0)) {
     if (dispose && strcmp(q, "..") == 0)
        err(ZE_PARMS, "cannot remove parent directory");
     return procname(p);
  }
# ifdef __GO32__
  /* expansion already done by DJGPP */
  f = 1;
  if (strcmp(q, ".") != 0 && strcmp(q, "..") != 0 && procname(w) != ZE_OK) {
     f = 0;
  }
# else
  /* Search that level for matching names */
  if ((d = opend(p)) == NULL)
  {
    free((voidp *)a);
    return ZE_MISS;
  }
  if ((r = strlen(p)) > 1 &&
      (strcmp(p + r - 2, ":.") == 0 || strcmp(p + r - 2, "/.") == 0))
    *(p + r - 1) = 0;
  f = 0;
  while ((e = readd(d)) != NULL) {
    if (strcmp(e, ".") && strcmp(e, "..") && MATCH(q, e))
    {
      f = 1;
      if (strcmp(p, ".") == 0) {                /* path is . */
        r = procname(e);                        /* name is name */
        if (r) {
           f = 0;
           break;
        }
      } else
      {
        if ((n = malloc(strlen(p) + strlen(e) + 2)) == NULL)
        {
          free((voidp *)a);
          closed(d);
          return ZE_MEM;
        }
        n = strcpy(n, p);
        if (n[r = strlen(n) - 1] != '/' && n[r] != ':')
          strcat(n, "/");
        r = procname(strcat(n, e));             /* name is path/name */
        free((voidp *)n);
        if (r) {
          f = 0;
          break;
        }
      }
    }
  }
  closed(d);
# endif /* __GO32__ */

  /* Done */
  free((voidp *)a);
  return f ? ZE_OK : ZE_MISS;
}
#endif /* MSDOS || OS2 */

#if defined(MSDOS) && !defined(OS2) && !defined(WIN32)
#  if defined(__TURBOC__) || defined(__GO32__) || defined(__BORLANDC__)
#   define GetFileMode(name) bdosptr(0x43, (name), 0)
#  else
    int GetFileMode(char *name)
    {
       unsigned int attr = 0;
       _dos_getfileattr(name, &attr);
       return attr;
    }
#  endif/* __TURBOC__  || __GO32__ */
# endif /* MSDOS && !OS2 */

#ifdef __human68k__
#  define GetFileMode(name) (_dos_chmod((name), -1) & 0x3f)
#endif

#ifdef AMIGA
 
/* What we have here is a mostly-generic routine using opend()/readd() and */
/* isshexp()/MATCH() to find all the files matching a multi-part filespec  */
/* using the portable pattern syntax.  It shouldn't take too much fiddling */
/* to make it usable for any other platform that has directory hierarchies */
/* but no shell-level pattern matching.  It works for patterns throughout  */
/* the pathname, such as "foo:*.?/source/x*.[ch]".                         */

/* whole is a pathname with wildcards, wildtail points somewhere in the  */
/* middle of it.  All wildcards to be expanded must come AFTER wildtail. */

local int wild_recurse(whole, wildtail) char *whole; char *wildtail;
{
    dstrm *dir;
    char *subwild, *name, *newwhole = NULL, *glue = NULL, plug = 0, plug2;
    ush newlen, amatch = 0;
    BPTR lok;
    int e = ZE_MISS;

    if (!isshexp(wildtail))
        if (lok = Lock(whole, ACCESS_READ)) {       /* p exists? */
            UnLock(lok);
            return procname(whole);
        } else
            return ZE_MISS;                     /* woops, no wildcards! */

    /* back up thru path components till existing dir found */
    do {
        name = wildtail + strlen(wildtail) - 1;
        for (;;)
            if (name-- <= wildtail || *name == '/') {
                subwild = name + 1;
                plug2 = *subwild;
                *subwild = 0;
                break;
            }
        if (glue)
            *glue = plug;
        glue = subwild;
        plug = plug2;
        dir = opend(whole);
    } while (!dir && !disk_not_mounted && subwild > wildtail);
    wildtail = subwild;                 /* skip past non-wild components */

    if (subwild = strchr(wildtail + 1, '/')) {
        /* this "+ 1" dodges the  ^^^ hole left by *glue == 0 */
        *(subwild++) = 0;               /* wildtail = one component pattern */
        newlen = strlen(whole) + strlen(subwild) + 32;
    } else
        newlen = strlen(whole) + 31;
    if (!dir || !(newwhole = malloc(newlen))) {
        if (glue)
            *glue = plug;
        e = dir ? ZE_MEM : ZE_MISS;
        goto ohforgetit;
    }
    strcpy(newwhole, whole);
    newlen = strlen(newwhole);
    if (glue)
        *glue = plug;                           /* repair damage to whole */
    if (!isshexp(wildtail)) {
        e = ZE_MISS;                            /* non-wild name not found */
        goto ohforgetit;
    }

    while (name = readd(dir)) {
        if (MATCH(wildtail, name)) {
            strcpy(newwhole + newlen, name);
            if (subwild) {
                name = newwhole + strlen(newwhole);
                *(name++) = '/';
                strcpy(name, subwild);
                e = wild_recurse(newwhole, name);
            } else
                e = procname(newwhole);
            newwhole[newlen] = 0;
            if (e == ZE_OK)
                amatch = 1;
            else if (e != ZE_MISS)
                break;
        }
    }

  ohforgetit:
    if (dir) closed(dir);
    if (subwild) *--subwild = '/';
    if (newwhole) free(newwhole);
    if (e == ZE_MISS && amatch)
        e = ZE_OK;
    return e;
}

int wild(p) char *p;
{
    char *use;

    /* wild_recurse() can't handle colons in wildcard part: */
    if (use = strchr(p, ':')) {
        if (strchr(++use, ':'))
            return ZE_PARMS;
    } else
        use = p;

    return wild_recurse(p, use);
}
#endif /* AMIGA */
                                          

#ifdef VMS
int wild(p)
char *p;                /* path/pattern to match */
/* Expand the pattern based on the contents of the file system.  Return an
   error code in the ZE_ class. */
{
  dstrm *d;             /* stream for reading directory */
  char *e;              /* name found in directory */
  int f;                /* true if there was a match */

  /* Search given pattern for matching names */
  if ((d = (dstrm *)malloc(sizeof(dstrm))) == NULL)
    return ZE_MEM;
  vms_wild(p, d);       /* pattern may be more than just directory name */

  /*
   * Save version specified by user to use in recursive drops into
   * subdirectories.
   */
  strncpy(wild_version_part,d->nam.nam$l_ver,d->nam.nam$b_ver);
  wild_version_part[d->nam.nam$b_ver] = 0;

  f = 0;
  while ((e = readd(d)) != NULL)        /* "dosmatch" is already built in */
    if (procname(e) == ZE_OK)
      f = 1;
  closed(d);

  /* Done */
  return f ? ZE_OK : ZE_MISS;
}
#endif /* VMS */


char *getnam(n)
char *n;                /* where to put name (must have >=FNMAX+1 bytes) */
/* Read a space, \n, \r, or \t delimited name from stdin into n, and return
   n.  If EOF, then return NULL.  Also, if the name read is too big, return
   NULL. */
{
  int c;                /* last character read */
  char *p;              /* pointer into name area */

  p = n;
  while ((c = getchar()) == ' ' || c == '\n' || c == '\r' || c == '\t')
    ;
  if (c == EOF)
    return NULL;
  do {
    if (p - n >= FNMAX)
      return NULL;
    *p++ = (char)c;
    c = getchar();
  } while (c != EOF && c != ' ' && c != '\n' && c != '\r' && c != '\t');
  *p = 0;
  return n;
}


struct flist far *fexpel(f)
struct flist far *f;    /* entry to delete */
/* Delete the entry *f in the doubly-linked found list.  Return pointer to
   next entry to allow stepping through list. */
{
  struct flist far *t;  /* temporary variable */

  t = f->nxt;
  *(f->lst) = t;                        /* point last to next, */
  if (t != NULL)
    t->lst = f->lst;                    /* and next to last */
  if (f->name != NULL)                  /* free memory used */
    free((voidp *)(f->name));
  if (f->zname != NULL)
    free((voidp *)(f->zname));
  farfree((voidp far *)f);
  fcount--;                             /* decrement count */
  return t;                             /* return pointer to next */
}


local int fqcmp(a, b)
const voidp *a, *b;           /* pointers to pointers to found entries */
/* Used by qsort() to compare entries in the found list by name. */
{
  return strcmp((*(struct flist far **)a)->name,
                (*(struct flist far **)b)->name);
}


local int fqcmpz(a, b)
const voidp *a, *b;           /* pointers to pointers to found entries */
/* Used by qsort() to compare entries in the found list by zname. */
{
  return strcmp((*(struct flist far **)a)->zname,
                (*(struct flist far **)b)->zname);
}


local char *last(p)
char *p;                /* sequence of / delimited path components */
/* Return a pointer to the start of the last path component. For a
 * directory name terminated by /, the return value is an empty string.
 */
{
  char *t;              /* temporary variable */

  if ((t = strrchr(p, PATH_END)) != NULL)
    return t + 1;
  else
    return p;
}


local char *msname(n)
char *n;
/* Reduce all path components to MSDOS upper case 8.3 style names.  Probably
   should also check for invalid characters, but I don't know which ones
   those are. */
{
  int c;                /* current character */
  int f;                /* characters in current component */
  char *p;              /* source pointer */
  char *q;              /* destination pointer */

  p = q = n;
  f = 0;
  while ((c = (unsigned char)*p++) != 0)
    if (c == '/')
    {
      *q++ = (char)c;
      f = 0;                            /* new component */
    }
#ifdef __human68k__
    else if (iskanji(c))
    {
      if (f == 7 || f == 11)
        f++;
      else if (*p != '\0' && f < 12 && f != 8)
      {
        *q++ = c;
        *q++ = *p++;
        f += 2;
      }
    }
#endif
    else if (c == '.')
      if (f < 9)
      {
        *q++ = (char)c;
        f = 9;                          /* now in file type */
      }
      else
        f = 12;                         /* now just excess characters */
    else
      if (f < 12 && f != 8)
      {
        *q++ = (char)(to_up(c));
        f++;                            /* do until end of name or type */
      }
  *q = 0;
  return n;
}


#ifdef VMS
local char *strlower(s)
char *s;                /* string to convert */
/* Convert all uppercase letters to lowercase in string s */
{
  char *p;              /* scans string */

  for (p = s; *p; p++)
    if (*p >= 'A' && *p <= 'Z')
      *p += 'a' - 'A';
  return s;
}

local char *strupper(s)
char *s;                /* string to convert */
/* Convert all lowercase letters to uppercase in string s */
{
  char *p;              /* scans string */

  for (p = s; *p; p++)
    if (*p >= 'a' && *p <= 'z')
      *p -= 'a' - 'A';
  return s;
}
#endif /* VMS */

char *ex2in(x, isdir, pdosflag)
char *x;                /* external file name */
int isdir;              /* input: x is a directory */
int *pdosflag;          /* output: force MSDOS file attributes? */
/* Convert the external file name to a zip file name, returning the malloc'ed
   string or NULL if not enough memory. */
{
  char *n;              /* internal file name (malloc'ed) */
  char *t;              /* shortened name */
  int dosflag;

#ifdef TOPS20
  int jfn;
  char *fp, fname[200];
  int ablock[5];

  jfn = _gtjfn(x, (O_RDONLY));
  ablock[1] = (int) (fname - 1);
  ablock[2] = jfn & 0777777;    /* jfn, no flags */
  ablock[3] = 0111100000001;    /* DEV+DIR+NAME+TYPE, punctuate */
  if (!jsys(JFNS, ablock)) {
      _rljfn(jfn);
      return NULL;              /* something bad happened */
  }
  _rljfn(jfn);
  if ((fp = (char *)malloc(strlen(fname) + 1)) == NULL) {
      return NULL;
  }
  strcpy(fp, fname);            /* copy the file name here */
  x = fp;
#endif /* TOPS20 */

#if defined(OS2) || defined(WIN32)
  dosflag = dosify || IsFileSystemFAT(x);
  if (!dosify && use_longname_ea && (t = GetLongPathEA(x)) != NULL)
  {
    x = t;
    dosflag = 0;
  }
#else /* !OS2 */
# ifdef MSDOS
  dosflag = 1;
# else /* !MSDOS */
  dosflag = dosify; /* default for non-DOS and non-OS/2 */
# endif /* ?MSDOS */
#endif /* ?OS2 */

  /* Find starting point in name before doing malloc */
#if defined(MSDOS) || defined(__human68k__) /* msdos */
  t = *x && *(x + 1) == ':' ? x + 2 : x;
  while (*t == '/' || *t == '\\')
    t++;
#else /* !MSDOS */
#  if (defined(VMS) || defined(TOPS20))
  t = x;
  if ((n = strrchr(t, ':')) != NULL)
    t = n + 1;
  if (*t == PATH_START && (n = strrchr(t, PATH_END)) != NULL)
    if ((x = strchr(t, '.')) != NULL && x < n)
      t = x + 1;
    else
      t = n + 1;
#  else /* !(VMS || TOPS20) */
#    ifdef AMIGA
  if ((t = strrchr(x, ':')) != NULL)    /* reject ":" */
    t++;
  else
    t = x;
  {                                     /* reject "//" */
    char *tt = t;
    while (tt = strchr(tt, '/'))
      while (*++tt == '/')
        t = tt;
  }
  while (*t == '/')             /* reject leading "/" on what's left */
    t++;
#    else /* !AMIGA */                      /* unix */
  for (t = x; *t == '/'; t++)
    ;
#    endif /* ?AMIGA */
#  endif /* ?(VMS || TOPS20) */
#endif /* ?MSDOS */

  /* Make changes, if any, to the copied name (leave original intact) */
#ifdef __human68k__
  _toslash(t);
#endif /* !__human68k__ */
#ifdef MSDOS
  for (n = t; *n; n++)
    if (*n == '\\')
      *n = '/';
#endif /* MSDOS */

  if (!pathput)
    t = last(t);

  /* Discard directory names with zip -rj */
  if (*t == '\0')
    return t;

  /* Malloc space for internal name and copy it */
  if ((n = malloc(strlen(t) + 1)) == NULL)
    return NULL;
  strcpy(n, t);

#if (defined(VMS) || defined(TOPS20))
  if ((t = strrchr(n, PATH_END)) != NULL)
  {
    *t = '/';
    while (--t > n)
      if (*t == '.')
        *t = '/';
  }

  /* Fix from Greg Roelofs: */
  /* Get current working directory and strip from n (t now = n) */
  {
    char cwd[256], *p, *q;
    int c;

    if (getcwd(cwd, 256) && ((p = strchr(cwd, '.')) != NULL))
    {
      ++p;
      if ((q = strrchr(p, PATH_END)) != NULL)
      {
        *q = '/';
        while (--q > p)
          if (*q == '.')
            *q = '/';

        /* strip bogus path parts from n */
        if (strncmp(n, p, (c=strlen(p))) == 0)
        {
          q = n + c;
          while (*t++ = *q++)
            ;
        }
      }
    }
  }
  strlower(n);

  if (isdir)
  {
    if (strcmp((t=n+strlen(n)-6), ".dir;1"))
      error("directory not version 1");
    else
      strcpy(t, "/");
  }
#ifdef VMS
  else if (!vmsver)
    if ((t = strrchr(n, ';')) != NULL)
      *t = 0;
#endif /* VMS */

  if ((t = strrchr(n, '.')) != NULL)
  {
    if ( t[1] == 0)                /* "filename." -> "filename" */
      *t = 0;
#ifdef VMS
    else if (t[1] == ';')         /* "filename.;vvv" -> "filename;vvv" */
    {
      char *f = t+1;
      while (*t++ = *f++) ;
    }
#endif /* VMS */
  }
#else
  if (isdir == 42) return n;      /* avoid warning on unused variable */
#endif /* ?(VMS || TOPS20) */

  if (dosify)
    msname(n);
#if defined(MSDOS) && !defined(OS2) && !defined(WIN32)
  else
    strlwr(n);
#endif
  /* Returned malloc'ed name */
  if (pdosflag) 
    *pdosflag = dosflag;
  return n;
}


char *in2ex(n)
char *n;                /* internal file name */
/* Convert the zip file name to an external file name, returning the malloc'ed
   string or NULL if not enough memory. */
{
  char *x;              /* external file name */
#if (defined(VMS) || defined(TOPS20))
  char *t;              /* scans name */

  if ((t = strrchr(n, '/')) == NULL)
#endif
  {
    if ((x = malloc(strlen(n) + 1 + PAD)) == NULL)
      return NULL;
    strcpy(x, n);
  }
#if (defined(VMS) || defined(TOPS20))
  else
  {
    if ((x = malloc(strlen(n) + 3 + PAD)) == NULL)
      return NULL;
    x[0] = PATH_START;
    x[1] = '.';
    strcpy(x + 2, n);
    *(t = x + 2 + (t - n)) = PATH_END;
    while (--t > x)
      if (*t == '/')
        *t = '.';
  }
  strupper(x);
#endif /* ?(VMS || TOPS20) */

#if defined(OS2) || defined(WIN32)
  if ( !IsFileNameValid(x) )
    ChangeNameForFAT(x);
#endif /* !OS2 */
  return x;
}


int check_dup()
/* Sort the found list and remove duplicates.
   Return an error code in the ZE_ class. */
{
  struct flist far *f;          /* steps through found linked list */
  extent j;                     /* index for s */
  struct flist far **s;         /* sorted table */

  /* sort found list, remove duplicates */
  if (fcount)
  {
    if ((s = (struct flist far **)malloc(
         fcount * sizeof(struct flist far *))) == NULL)
      return ZE_MEM;
    for (j = 0, f = found; f != NULL; f = f->nxt)
      s[j++] = f;
    qsort((char *)s, fcount, sizeof(struct flist far *), fqcmp);
    for (j = fcount - 1; j > 0; j--)
      if (strcmp(s[j - 1]->name, s[j]->name) == 0)
        fexpel(s[j]);           /* fexpel() changes fcount */
    qsort((char *)s, fcount, sizeof(struct flist far *), fqcmpz);
    for (j = 1; j < fcount; j++)
      if (strcmp(s[j - 1]->zname, s[j]->zname) == 0)
      {
        warn("name in zip file repeated: ", s[j]->zname);
        warn("  first full name: ", s[j - 1]->name);
        warn(" second full name: ", s[j]->name);
        return ZE_PARMS;
      }
    free((voidp *)s);
  }
  return ZE_OK;
}

int filter(name)
  char *name;
  /* Scan the -i and -x lists for matches to the given name.
     Return true if the name must be included, false otherwise.
     Give precedence to -x over -i.
   */
{
   int n;
   int include = icount ? 0 : 1;
#ifdef MATCH_LASTNAME_ONLY
   char *p = last(name);
   if (*p) name = p;
#endif

   if (pcount == 0) return 1;

   for (n = 0; n < pcount; n++) {
      if (MATCH(patterns[n].zname, name)) {
         if (patterns[n].select == 'x') return 0;
         include = 1;
      }
   }
   return include;
}

local int newname(n, isdir)
char *n;                /* name to add (or exclude) */
int  isdir;             /* true for a directory */
/* Add (or exclude) the name of an existing disk file.  Return an error
   code in the ZE_ class. */
{
  char *m;
  struct flist far *f;  /* where in found, or new found entry */
  struct zlist far *z;  /* where in zfiles (if found) */
  int dosflag;

  /* Search for name in zip file.  If there, mark it, else add to
     list of new names to do (or remove from that list). */
  if ((m = ex2in(n, isdir, &dosflag)) == NULL)
    return ZE_MEM;

  /* Discard directory names with zip -rj */
  if (*m == '\0') {
#ifndef AMIGA
/* A null string is a legitimate external directory name in AmigaDOS; also,
 * a command like "zip -r zipfile FOO:" produces an empty internal name.
 */
    if (pathput) error("empty name without -j");
#endif
    free((voidp *)m);
    return ZE_OK;
  }
  if ((z = zsearch(m)) != NULL) {
    z->mark = pcount ? filter(m) : 1;
    if (z->mark == 0) {
      free((voidp *)m);
      if (verbose)
        fprintf(mesg, "zip diagnostic: excluding %s\n", z->name);
    } else {
      free((voidp *)(z->name));
      if ((z->name = malloc(strlen(n) + 1 + PAD)) == NULL)
        return ZE_MEM;
      strcpy(z->name, n);
#ifdef FORCE_NEWNAME
      free((voidp *)(z->zname));
      z->zname = m;
#else
      /* Better keep the old name. Useful when updating on MSDOS a zip file
       * made on Unix.
       */
      free((voidp *)m);
#endif
      z->dosflag = dosflag;
      if (verbose)
        fprintf(mesg, "zip diagnostic: including %s\n", z->name);
    }
    if (n == label) {
       label = z->name;
    }
  } else if (pcount == 0 || filter(m)) {

    /* Check that we are not adding the zip file to itself. This
     * catches cases like "zip -m foo ../dir/foo.zip".
     */
    struct stat statb;
    if (zipstate == -1)
       zipstate = strcmp(zipfile, "-") != 0 &&
                   stat(zipfile, &zipstatb) == 0;
    if (zipstate == 1 && (statb = zipstatb, stat(n, &statb) == 0
      && zipstatb.st_mode  == statb.st_mode
      && zipstatb.st_ino   == statb.st_ino
      && zipstatb.st_dev   == statb.st_dev
      && zipstatb.st_uid   == statb.st_uid
      && zipstatb.st_gid   == statb.st_gid
      && zipstatb.st_size  == statb.st_size
      && zipstatb.st_mtime == statb.st_mtime
      && zipstatb.st_ctime == statb.st_ctime))
      /* Don't compare a_time since we are reading the file */
         return ZE_OK;

    /* allocate space and add to list */
    if ((f = (struct flist far *)farmalloc(sizeof(struct flist))) == NULL ||
        (f->name = malloc(strlen(n) + 1 + PAD)) == NULL)
    {
      if (f != NULL)
        farfree((voidp far *)f);
      return ZE_MEM;
    }
    strcpy(f->name, n);
    f->zname = m;
    f->dosflag = dosflag;
    *fnxt = f;
    f->lst = fnxt;
    f->nxt = NULL;
    fnxt = &f->nxt;
    fcount++;
    if (n == label) {
      label = f->name;
    }
  }
  return ZE_OK;
}


int procname(n)
char *n;                /* name to process */
/* Process a name or sh expression to operate on (or exclude).  Return
   an error code in the ZE_ class. */
{
#if (!defined(VMS) && !defined(TOPS20))
  char *a;              /* path and name for recursion */
#endif
  dstrm *d;             /* directory stream from opend() */
  char *e;              /* pointer to name from readd() */
  int m;                /* matched flag */
  char *p;              /* path for recursion */
  struct stat s;        /* result of stat() */
  struct zlist far *z;  /* steps through zfiles list */

  if (strcmp(n, "-") == 0)   /* if compressing stdin */
    return newname(n, 0);
  else if (
#ifdef S_IFLNK          /* if symbolic links exist ... */
      linkput ? lstat(n, &s) :
#endif /* S_IFLNK */
      SSTAT(n, &s)
#if defined(__TURBOC__) || defined(VMS) || defined(__WATCOMC__)
       /* For these 3 compilers, stat() succeeds on wild card names! */
      || isshexp(n)
#endif
     )
  {
    /* Not a file or directory--search for shell expression in zip file */
    p = ex2in(n, 0, (int *)NULL);       /* shouldn't affect matching chars */
    m = 1;
    for (z = zfiles; z != NULL; z = z->nxt) {
      if (MATCH(p, z->zname))
      {
        z->mark = pcount ? filter(z->zname) : 1;
        if (verbose)
            fprintf(mesg, "zip diagnostic: %scluding %s\n",
               z->mark ? "in" : "ex", z->name);
        m = 0;
      }
    }
    free((voidp *)p);
    return m ? ZE_MISS : ZE_OK;
  }

  /* Live name--use if file, recurse if directory */
#ifdef __human68k__
  _toslash(n);
#endif
#ifdef MSDOS
  for (p = n; *p; p++)          /* use / consistently */
    if (*p == '\\')
      *p = '/';
#endif /* MSDOS */
  if ((s.st_mode & S_IFDIR) == 0)
  {
    /* add or remove name of file */
    if ((m = newname(n, 0)) != ZE_OK)
      return m;
  } else {
#if defined(VMS) || defined(TOPS20)
    if (dirnames && (m = newname(n, 1)) != ZE_OK) {
      return m;
    }
    /* recurse into directory */
    if (recurse && (d = opend(n)) != NULL)
    {
      while ((e = readd(d)) != NULL) {
        if ((m = procname(e)) != ZE_OK)     /* recurse on name */
        {
          closed(d);
          return m;
        }
      }
      closed(d);
    }
#else /* (VMS || TOPS20) */
    /* Add trailing / to the directory name */
    if ((p = malloc(strlen(n)+2)) == NULL)
      return ZE_MEM;
    if (strcmp(n, ".") == 0) {
      *p = 0;  /* avoid "./" prefix and do not create zip entry */
    } else {
      strcpy(p, n);
      a = p + strlen(p);
#ifdef AMIGA
      if (*p && a[-1] != '/' && a[-1] != ':')
#else
      if (a[-1] != '/')
#endif
        strcpy(a, "/");
      if (dirnames && (m = newname(p, 1)) != ZE_OK) {
        free((voidp *)p);
        return m;
      }
    }
    /* recurse into directory */
    if (recurse && (d = opend(n)) != NULL)
    {
      while ((e = readd(d)) != NULL) {
        if (strcmp(e, ".") && strcmp(e, ".."))
        {
          if ((a = malloc(strlen(p) + strlen(e) + 1)) == NULL)
          {
            free((voidp *)p);
            closed(d);
            return ZE_MEM;
          }
          strcat(strcpy(a, p), e);
          if ((m = procname(a)) != ZE_OK)   /* recurse on name */
          {
            if (m == ZE_MISS)
              warn("name not matched: ", a);
            else
              err(m, a);
          }
          free((voidp *)a);
        }
      }
      free((voidp *)p);
      closed(d);
    }
#endif /* ? (VMS || TOPS20) */
  } /* (s.st_mode & S_IFDIR) == 0) */
  return ZE_OK;
}


#if !defined(CRAY) && !defined(__TURBOC__) && !defined(OS2) /* and ... */
#if !defined( __GO32__)

local int cmptime(p, q)
struct tm *p, *q;       /* times to compare */
/* Return negative if time p is before time q, positive if after, and
   zero if the same */
{
  int r;                /* temporary variable */

  if (p == NULL)
    return -1;
  else if ((r = p->tm_year - q->tm_year) != 0)
    return r;
  else if ((r = p->tm_mon - q->tm_mon) != 0)
    return r;
  else if ((r = p->tm_mday - q->tm_mday) != 0)
    return r;
  else if ((r = p->tm_hour - q->tm_hour) != 0)
    return r;
  else if ((r = p->tm_min - q->tm_min) != 0)
    return r;
  else
    return p->tm_sec - q->tm_sec;
}


local time_t invlocal(t)
struct tm *t;           /* time to convert */
/* Find inverse of localtime() using bisection.  This routine assumes that
   time_t is an integer type, either signed or unsigned.  The expectation
   is that sometime before the year 2038, time_t will be made a 64-bit
   integer, and this routine will still work. */
{
  time_t i;             /* midpoint of current root range */
  time_t l;             /* lower end of root range */
  time_t u;             /* upper end of root range */

  /* Bracket the root [0,largest time_t].  Note: if time_t is a 32-bit signed
     integer, then the upper bound is GMT 1/19/2038 03:14:07, after which all
     the Unix systems in the world come to a grinding halt.  Either that, or
     all those systems will suddenly find themselves transported to December
     of 1901 ... */
  l = 0;
  u = 1;
  while (u < (u << 1))
    u = (u << 1) + 1;

  /* Find the root */
  while (u - l > 1)
  {
    i = l + ((u - l) >> 1);
    if (cmptime(localtime(&i), t) <= 0)
      l = i;
    else
      u = i;
  }
  return l;
}
#endif
#endif


void stamp(f, d)
char *f;                /* name of file to change */
ulg d;                  /* dos-style time to change it to */
/* Set last updated and accessed time of file f to the DOS time d. */
{
#if defined(MACOS)
  warn("timestamp not implemented yet", "");
#else
#ifdef __TURBOC__
  int h;                /* file handle */

  if ((h = open(f, 0)) != -1)
  {
#ifdef ATARI_ST
    d = ( d >> 16 ) | ( d << 16 );
#endif
    setftime(h, (struct ftime *)&d);
    close(h);
  }
#else /* !__TURBOC__ */
#ifdef VMS
  int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year;
  char timbuf[24];
  static char *month[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                          "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  struct VMStimbuf {
      char *actime;           /* VMS revision date, ASCII format */
      char *modtime;          /* VMS creation date, ASCII format */
  } ascii_times = {timbuf, timbuf};

  /* Convert DOS time to ASCII format for VMSmunch */
  tm_sec = (int)(d << 1) & 0x3e;
  tm_min = (int)(d >> 5) & 0x3f;
  tm_hour = (int)(d >> 11) & 0x1f;
  tm_mday = (int)(d >> 16) & 0x1f;
  tm_mon = ((int)(d >> 21) & 0xf) - 1;
  tm_year = ((int)(d >> 25) & 0x7f) + 1980;
  sprintf(timbuf, "%02d-%3s-%04d %02d:%02d:%02d.00", tm_mday, month[tm_mon],
    tm_year, tm_hour, tm_min, tm_sec);

  /* Set updated and accessed times of f */
  if (VMSmunch(f, SET_TIMES, &ascii_times) != RMS$_NMF)
    warn("can't set zipfile time: ", f);

#else /* !VMS */
#ifdef OS2
  SetFileTime(f, d);
#else /* !OS2 */
  struct tm t;          /* argument for mktime() or invlocal() */
#if defined(WIN32) || defined(sgi)
  struct utimbuf u;     /* argument for utime() */
#else
  time_t u[2];          /* argument for utime() */
#endif
#ifndef __GO32__
  extern time_t mktime OF((struct tm *));
#endif

  /* Convert DOS time to time_t format in u[0] and u[1] */
  t.tm_sec = (int)(d << 1) & 0x3e;
  t.tm_min = (int)(d >> 5) & 0x3f;
  t.tm_hour = (int)(d >> 11) & 0x1f;
  t.tm_mday = (int)(d >> 16) & 0x1f;
  t.tm_mon = ((int)(d >> 21) & 0xf) - 1;
  t.tm_year = ((int)(d >> 25) & 0x7f) + 80;
#if defined(WIN32) || defined (sgi)
  u.actime = u.modtime = mktime(&t);
#else
# if defined(MSDOS) || defined(OS2) || defined(CRAY)
  /* mktime() is more reliable than invlocal() because the time range is
   * wider on MSDOS than on Unix; required for Cray because invlocal assumes
   * 32-bit ints
   */
  u[0] = u[1] = mktime(&t);
# else
  u[0] = u[1] = invlocal(&t);
# endif
#endif

  /* Set updated and accessed times of f */
#if defined(WIN32) || defined(sgi)
  utime(f, &u);
#else
  utime(f, u);
#endif
#endif /* ?OS2 */
#endif /* ?VMS */
#endif /* ?__TURBOC__ */
#endif /* ?MACOS */
}


local void inctime(s)
struct tm *s;           /* time to increment in place */
/* Increment the time structure *s by one second, return the result in
   place. */
{
  int y;                /* temporary variable */

  /* days in each month, except for February */
  static int days[] = {31,0,31,30,31,30,31,31,30,31,30,31};

  /* Set days in February from year (1900 is a leap year, 2000 is not) */
  y = s->tm_year + 1900;
  days[1] = y % 4 == 0 && (y % 100 != 0 || y % 400 == 0) ? 29 : 28;

  /* Increment time with carry */
  if (s->tm_sec != 59)
    s->tm_sec++;
  else if (s->tm_sec = 0, s->tm_min != 59)
    s->tm_min++;
  else if (s->tm_min = 0, s->tm_hour != 23)
    s->tm_hour++;
  else if (s->tm_hour = 0, s->tm_mday != days[s->tm_mon])
    s->tm_mday++;
  else if (s->tm_mday = 1, s->tm_mon != 11)
    s->tm_mon++;
  else
  {
    s->tm_mon = 0;
    s->tm_year++;
  }
}


ulg dostime(y, n, d, h, m, s)
int y;                  /* year */
int n;                  /* month */
int d;                  /* day */
int h;                  /* hour */
int m;                  /* minute */
int s;                  /* second */
/* Convert the date y/n/d and time h:m:s to a four byte DOS date and
   time (date in high two bytes, time in low two bytes allowing magnitude
   comparison). */
{
  return y < 1980 ? dostime(1980, 1, 1, 0, 0, 0) :
        (((ulg)y - 1980) << 25) | ((ulg)n << 21) | ((ulg)d << 16) |
        ((ulg)h << 11) | ((ulg)m << 5) | ((ulg)s >> 1);
}


local ulg unix2dostime(t)
statime *t;             /* unix time to convert */
/* Return the Unix time t in DOS format, rounded up to the next two
   second boundary. */
{
  struct tm *s;         /* result of localtime() */

  s = localtime(t);             /* Use local time since MSDOS does */
  inctime(s);                   /* Add one second to round up */
  return dostime(s->tm_year + 1900, s->tm_mon + 1, s->tm_mday,
                 s->tm_hour, s->tm_min, s->tm_sec);
}


ulg filetime(f, a, n)
char *f;                /* name of file to get info on */
ulg *a;                 /* return value: file attributes */
long *n;                /* return value: file size */
/* If file *f does not exist, return 0.  Else, return the file's last
   modified date and time as an MSDOS date and time.  The date and
   time is returned in a long with the date most significant to allow
   unsigned integer comparison of absolute times.  Also, if a is not
   a NULL pointer, store the file attributes there, with the high two
   bytes being the Unix attributes, and the low byte being a mapping
   of that to DOS attributes.  If n is not NULL, store the file size
   there.
   If f is "-", use standard input as the file. If f is a device, return
   a file size of -1 */
{
  struct stat s;        /* results of stat() */
  char name[FNMAX];
  int len = strlen(f);

  if (f == label) {
    if (a != NULL)
      *a = label_mode;
    if (n != NULL)
      *n = -2L; /* convention for a label name */
    return label_time;  /* does not work for unknown reason */
  }
  strcpy(name, f);
  if (name[len - 1] == '/')
    name[len - 1] = 0; 
  /* not all systems allow stat'ing a file with / appended */

  if (strcmp(f, "-") == 0) {
#if defined(AMIGA) && !defined(__SASC_60)
  /* forge stat values for stdin since Amiga has no fstat() */
    s.st_mode = (S_IREAD|S_IWRITE|S_IFREG); 
    s.st_size = -1;
    s.st_mtime = time(&s.st_mtime);
#else /* !AMIGA */
    if (fstat(fileno(stdin), &s) != 0)
      error("fstat(stdin)");
#endif /* ?AMIGA */
  } else if ((
#ifdef S_IFLNK
             linkput ? lstat(name, &s) :
#endif
             SSTAT(name, &s)) != 0)
             /* Accept about any file kind including directories
              * (stored with trailing / with -r option)
              */
    return 0;

  if (a != NULL) {
#if defined(MSDOS) || defined(OS2) || defined(__human68k__)
    *a = ((ulg)s.st_mode << 16) | (ulg)GetFileMode(name);
#else
    *a = ((ulg)s.st_mode << 16) | !(s.st_mode & S_IWRITE);
    if ((s.st_mode & S_IFDIR) != 0) {
      *a |= MSDOS_DIR_ATTR;
    }
#endif
  }
  if (n != NULL)
    *n = (s.st_mode & S_IFMT) != S_IFREG ? -1L : s.st_size;

#ifdef OS2
  return GetFileTime(name);
#else /* !OS2 */
#  ifdef VMS
     return unix2dostime(&s.st_ctime);   /* Use creation time in VMS */
#  else /* !VMS */
#    ifdef ATARI_ST
       return s.st_mtime; /* Turbo C doesn't use UNIX times */
#    else
#      ifdef WIN32
         return GetTheFileTime(name);
#      else
         return unix2dostime((statime*)&s.st_mtime);
#      endif /* WIN32 */
#    endif
#  endif /* ?VMS */
#endif /* ?OS2 */
}

#ifdef TOPS20
#  include <monsym.h>   /* Get amazing monsym() macro */
#  define       _FBBYV  monsym(".FBBYV")
#  define         FBBSZ_S       -24     /* Obsolete, replace by FLDGET! */
#  define         FBBSZ_M       077     /* ditto */

extern int _gtjfn(), _rljfn(), _gtfdb(), stat();

int set_extra_field(z)
  struct zlist *z;
  /* create extra field and change z->att if desired */
{
  int jfn;

  translate_eol = 0;
  jfn = _gtjfn(z->name, O_RDONLY);
  z->att = (((_gtfdb (jfn, _FBBYV) << FBBSZ_S) & FBBSZ_M) != 8) ?
           ASCII :BINARY;
  _rljfn(jfn);
  return 0;
}
#else /* !TOPS20 */
# if !defined(OS2) && !defined(VMS)

int set_extra_field(z)
  struct zlist *z;
  /* create extra field and change z->att if desired */
{
  return (int)(z-z);
}
# endif /* !OS2 && !VMS */
#endif /* TOPS20 */



int issymlnk(a)
ulg a;                  /* Attributes returned by filetime() */
/* Return true if the attributes are those of a symbolic link */
{
#ifdef S_IFLNK
  return ((a >> 16) & S_IFMT) == S_IFLNK;
#else /* !S_IFLNK */
  return (int)a & 0;    /* avoid warning on unused parameter */
#endif /* ?S_IFLNK */
}


int deletedir(d)
char *d;                /* directory to delete */
/* Delete the directory *d if it is empty, do nothing otherwise.
   Return the result of rmdir(), delete(), or system().
   For VMS, d must be in format [x.y]z.dir;1  (not [x.y.z]).
 */
{
#if (defined(MACOS) || defined(TOPS20))
    warn("deletedir not implemented yet", "");
    return 127;
#else
# ifdef RMDIR
    /* code from Greg Roelofs, who horked it from Mark Edwards (unzip) */
    int r, len;
    char *s;              /* malloc'd string for system command */

    len = strlen(d);
    if ((s = malloc(len + 34)) == NULL)
      return 127;

#  ifdef VMS
    system(strcat(strcpy(s, "set prot=(o:rwed) "), d));
    r = delete(d);
#  else /* !VMS */
    sprintf(s, "IFS=\" \t\n\" /bin/rmdir %s 2>/dev/null", d);
    r = system(s);
#  endif /* ?VMS */
    free(s);
    return r;
# else /* !RMDIR */
    return rmdir(d);
# endif /* ?RMDIR */
#endif /* ? MACOS || TOPS20 */
}


#endif /* !UTIL */

int destroy(f)
char *f;                /* file to delete */
/* Delete the file *f, returning non-zero on failure. */
{
  return unlink(f);
}


int replace(d, s)
char *d, *s;            /* destination and source file names */
/* Replace file *d by file *s, removing the old *s.  Return an error code
   in the ZE_ class. This function need not preserve the file attributes,
   this will be done by setfileattr() later.
 */
{
  struct stat t;        /* results of stat() */
  int copy = 0;
  int d_exists;

#ifdef VMS
  /* stat() is broken on VMS remote files (accessed through Decnet).
   * This patch allows creation of remote zip files, but is not sufficient
   * to update them or compress remote files */
  unlink(d);
#else
  d_exists = (LSTAT(d, &t) == 0);
  if (d_exists)
  {
    /*
     * respect existing soft and hard links!
     */
    if (t.st_nlink > 1
# ifdef S_IFLNK
        || (t.st_mode & S_IFMT) == S_IFLNK
# endif
        )
       copy = 1;
    else if (unlink(d))
       return ZE_CREAT;                 /* Can't erase zip file--give up */
  }
#endif /* VMS */
  if (!copy) {
      if (link(s, d)) {               /* Just move s on top of d */
          copy = 1;                     /* failed ? */
#if !defined(VMS) && !defined(ATARI_ST) && !defined(AMIGA)
    /* For VMS & ATARI & AMIGA assume failure is EXDEV */
          if (errno != EXDEV
#  ifdef ENOTSAM
           && errno != ENOTSAM /* Used at least on Turbo C */
#  endif
              ) return ZE_CREAT;
#endif
      }
#ifndef link    /* UNIX link() */
      /*
       * Assuming a UNIX link(2), we still have to remove s.
       * If link has been #defined to rename(), nothing to do.
       */
      else {
# ifdef KEEP_OWNER
          if (d_exists)
              /* this will fail if the user isn't priviledged */
              chown(d, t.st_uid, t.st_gid);
# endif
          unlink(s);
      }
#endif          /* ?UNIX link() */
  }

  if (copy) {
    FILE *f, *g;      /* source and destination files */
    int r;            /* temporary variable */

    if ((f = fopen(s, FOPR)) == NULL) {
      fprintf(stderr," replace: can't open %s\n", s);
      return ZE_TEMP;
    }
    if ((g = fopen(d, FOPW)) == NULL)
    {
      fclose(f);
      return ZE_CREAT;
    }
    r = fcopy(f, g, (ulg)-1L);
    fclose(f);
    if (fclose(g) || r != ZE_OK)
    {
      unlink(d);
      return r ? (r == ZE_TEMP ? ZE_WRITE : r) : ZE_WRITE;
    }
    unlink(s);
  }
  return ZE_OK;
}


int getfileattr(f)
char *f;                /* file path */
/* Return the file attributes for file f or 0 if failure */
{
  struct stat s;

  return SSTAT(f, &s) == 0 ? s.st_mode : 0;
}


int setfileattr(f, a)
char *f;                /* file path */
int a;                  /* attributes returned by getfileattr() */
/* Give the file f the attributes a, return non-zero on failure */
{
#if defined(MACOS) || defined(TOPS20)
  return 0;
#else
  return chmod(f, a);
#endif
}


char *tempname(zip)
  char *zip;              /* path name of zip file to generate temp name for */

/* Return a temporary file name in its own malloc'ed space, using tempath. */
{
  char *t = zip;   /* malloc'ed space for name (use zip to avoid warning) */

  if (tempath != NULL)
  {
    if ((t = malloc(strlen(tempath)+12)) == NULL)
      return NULL;
    strcpy(t, tempath);
#if (!defined(VMS) && !defined(TOPS20))
#  ifdef AMIGA
    {
          char c = t[strlen(t)-1];
          if (c != '/' && c != ':')
            strcat(t, "/");
    }
#  else /* !AMIGA */
          
    if (t[strlen(t)-1] != '/')
      strcat(t, "/");
#  endif  /* ?AMIGA */            
#endif
  }
  else
  {
    if ((t = malloc(12)) == NULL)
      return NULL;
    *t = 0;
  }
#ifdef NO_MKTEMP
  {
    char *p = t + strlen(t);
    sprintf(p, "%08lx", (ulg)time(NULL));
    return t;
  }
#else
  strcat(t, "_ZXXXXXX");
  return mktemp(t);
#endif
}


int fcopy(f, g, n)
FILE *f, *g;            /* source and destination files */
ulg n;                  /* number of bytes to copy or -1 for all */
/* Copy n bytes from file *f to file *g, or until EOF if n == -1.  Return
   an error code in the ZE_ class. */
{
  char *b;              /* malloc'ed buffer for copying */
  extent k;             /* result of fread() */
  ulg m;                /* bytes copied so far */

  if ((b = malloc(CBSZ)) == NULL)
    return ZE_MEM;
  m = 0;
  while (n == -1L || m < n)
  {
    if ((k = fread(b, 1, n == -1 ?
                   CBSZ : (n - m < CBSZ ? (extent)(n - m) : CBSZ), f)) == 0)
      if (ferror(f))
      {
        free((voidp *)b);
        return ZE_READ;
      }
      else
        break;
    if (fwrite(b, 1, k, g) != k)
    {
      free((voidp *)b);
      fprintf(stderr," fcopy: write error\n");
      return ZE_TEMP;
    }
    m += k;
  }
  free((voidp *)b);
  return ZE_OK;
}


#ifdef ZMEM

/************************/
/*  Function memset()  */
/************************/

/*
 * memset - for systems without it
 *  bill davidsen - March 1990
 */

char *
memset(buf, init, len)
register char *buf;     /* buffer loc */
register int init;      /* initializer */
register unsigned int len;   /* length of the buffer */
{
    char *start;

    start = buf;
    while (len--) *(buf++) = init;
    return(start);
}


/************************/
/*  Function memcpy()  */
/************************/

char *
memcpy(dst,src,len)           /* v2.0f */
register char *dst, *src;
register unsigned int len;
{
    char *start;

    start = dst;
    while (len--)
        *dst++ = *src++;
    return(start);
}


/************************/
/*  Function memcmp()  */
/************************/

int
memcmp(b1,b2,len)                     /* jpd@usl.edu -- 11/16/90 */
register char *b1, *b2;
register unsigned int len;
{

    if (len) do {             /* examine each byte (if any) */
      if (*b1++ != *b2++)
        return (*((uch *)b1-1) - *((uch *)b2-1));  /* exit when miscompare */
       } while (--len);

    return(0);        /* no miscompares, yield 0 result */
}

#endif  /* ZMEM */

#ifdef TOPS20

int
strupper(s)     /* Returns s in uppercase */
char *s;        /* String to be uppercased */
{
    char    *p;

    p = s;
    for (; *p; p++)
        *p = toupper (*p);
}

int
strlower(s)     /* Returns s in lowercase. */
char *s;        /* String to be lowercased */
{
    char    *p;

    p = s;
    for (; *p; p++)
        *p = tolower (*p);
}
#endif /* TOPS20 */

#if defined(__TURBOC__) && !defined(OS2)

/************************/
/*  Function fcalloc()  */
/************************/

/* Turbo C malloc() does not allow dynamic allocation of 64K bytes
 * and farmalloc(64K) returns a pointer with an offset of 8, so we
 * must fix the pointer. Warning: the pointer must be put back to its
 * original form in order to free it, use fcfree().
 * For MSC, use halloc instead of this function (see tailor.h).
 */
static ush ptr_offset = 0;

void * fcalloc(items, size)
    unsigned items; /* number of items */
    unsigned size;  /* item size */
{
    void * buf = farmalloc((ulg)items*size + 16L);
    if (buf == NULL) return NULL;
    /* Normalize the pointer to seg:0 */
    if (ptr_offset == 0) {
        ptr_offset = (ush)((uch*)buf-0);
    } else if (ptr_offset != (ush)((uch*)buf-0)) {
        err(ZE_LOGIC, "inconsistent ptr_offset");
    }
    *((ush*)&buf+1) += (ptr_offset + 15) >> 4;
    *(ush*)&buf = 0;
    return buf;
}

void fcfree(ptr)
    void *ptr; /* region allocated with fcalloc() */
{
    /* Put the pointer back to its original form: */
    *((ush*)&ptr+1) -= (ptr_offset + 15) >> 4;
    *(ush*)&ptr = ptr_offset;
    farfree(ptr);
 }

#endif /* __TURBOC__ && !OS2 */
