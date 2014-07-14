/*
 * @(#)dir.c 1.4 87/11/06 Public Domain.
 *
 *  A public domain implementation of BSD directory routines for
 *  MS-DOS.  Written by Michael Rendell ({uunet,utai}michael@garfield),
 *  August 1897
 *  Ported to OS/2 by Kai Uwe Rommel
 *  December 1989, February 1990
 *  Change for HPFS support, October 1990
 */

/* does also contain EA access code for use in ZIP */


#ifdef OS2


#if defined(__EMX__) && !defined(__32BIT__)
#  define __32BIT__
#endif

#include "zip.h"

#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#ifndef __BORLANDC__
#include <malloc.h>
#endif

#define INCL_NOPM
#define INCL_DOSNLS
#define INCL_DOSERRORS
#include <os2.h>

#include "os2zip.h"


#ifndef max
#define max(a, b) ((a) < (b) ? (b) : (a))
#endif

#define EAID     0x0009


#ifdef __WATCOMC__
unsigned char __near _osmode = OS2_MODE;
#endif


#ifdef __32BIT__
#define DosFindFirst(p1, p2, p3, p4, p5, p6) \
        DosFindFirst(p1, p2, p3, p4, p5, p6, 1)
#else
#define DosQueryCurrentDisk DosQCurDisk
#define DosQueryFSAttach(p1, p2, p3, p4, p5) \
        DosQFSAttach(p1, p2, p3, p4, p5, 0)
#define DosQueryFSInfo(d, l, b, s) \
        DosQFSInfo(d, l, b, s)
#define DosQueryPathInfo(p1, p2, p3, p4) \
        DosQPathInfo(p1, p2, p3, p4, 0)
#define DosSetPathInfo(p1, p2, p3, p4, p5) \
        DosSetPathInfo(p1, p2, p3, p4, p5, 0)
#define DosEnumAttribute(p1, p2, p3, p4, p5, p6, p7) \
        DosEnumAttribute(p1, p2, p3, p4, p5, p6, p7, 0)
#define DosFindFirst(p1, p2, p3, p4, p5, p6) \
        DosFindFirst(p1, p2, p3, p4, p5, p6, 0)
#define DosMapCase DosCaseMap
#endif


#ifndef UTIL

extern int noisy;

#ifndef S_IFMT
#define S_IFMT 0xF000
#endif

static int attributes = _A_DIR | _A_HIDDEN | _A_SYSTEM;

static char *getdirent(char *);
static void free_dircontents(struct _dircontents *);

#ifdef __32BIT__
static HDIR hdir;
static ULONG count;
static FILEFINDBUF3 find;
#else
static HDIR hdir;
static USHORT count;
static FILEFINDBUF find;
#endif


DIR *opendir(char *name)
{
  struct stat statb;
  DIR *dirp;
  char c;
  char *s;
  struct _dircontents *dp;
  char nbuf[MAXPATHLEN + 1];
  int len;

  attributes = hidden_files ? (_A_DIR | _A_HIDDEN | _A_SYSTEM) : _A_DIR;

  strcpy(nbuf, name);
  len = strlen (nbuf);
  s = nbuf + len;

  if ( ((c = nbuf[strlen(nbuf) - 1]) == '\\' || c == '/') &&
       (strlen(nbuf) > 1) )
  {
    nbuf[strlen(nbuf) - 1] = 0;

    if ( nbuf[strlen(nbuf) - 1] == ':' )
      strcat(nbuf, "\\.");
  }
  else
    if ( nbuf[strlen(nbuf) - 1] == ':' )
      strcat(nbuf, ".");

#ifndef __BORLANDC__
  /* when will we ever see a Borland compiler that can properly stat !!! */
  if (stat(nbuf, &statb) < 0 || (statb.st_mode & S_IFMT) != S_IFDIR)
    return NULL;
#endif

  if ( (dirp = malloc(sizeof(DIR))) == NULL )
    return NULL;

  if ( nbuf[strlen(nbuf) - 1] == '.' )
    strcpy(nbuf + strlen(nbuf) - 1, "*.*");
  else
    if ( ((c = nbuf[strlen(nbuf) - 1]) == '\\' || c == '/') &&
         (strlen(nbuf) == 1) )
      strcat(nbuf, "*.*");
    else
      strcat(nbuf, "\\*.*");

  dirp -> dd_loc = 0;
  dirp -> dd_contents = dirp -> dd_cp = NULL;

  if ((s = getdirent(nbuf)) == NULL)
    return dirp;

  do
  {
    if (((dp = malloc(sizeof(struct _dircontents))) == NULL) ||
        ((dp -> _d_entry = malloc(strlen(s) + 1)) == NULL)      )
    {
      if (dp)
        free(dp);
      free_dircontents(dirp -> dd_contents);

      return NULL;
    }

    if (dirp -> dd_contents)
    {
      dirp -> dd_cp -> _d_next = dp;
      dirp -> dd_cp = dirp -> dd_cp -> _d_next;
    }
    else
      dirp -> dd_contents = dirp -> dd_cp = dp;

    strcpy(dp -> _d_entry, s);
    dp -> _d_next = NULL;

    dp -> _d_size = find.cbFile;
    dp -> _d_mode = find.attrFile;
    dp -> _d_time = *(unsigned *) &(find.ftimeLastWrite);
    dp -> _d_date = *(unsigned *) &(find.fdateLastWrite);
  }
  while ((s = getdirent(NULL)) != NULL);

  dirp -> dd_cp = dirp -> dd_contents;

  return dirp;
}


void closedir(DIR * dirp)
{
  free_dircontents(dirp -> dd_contents);
  free(dirp);
}


struct direct *readdir(DIR * dirp)
{
  static struct direct dp;

  if (dirp -> dd_cp == NULL)
    return NULL;

  dp.d_namlen = dp.d_reclen =
    strlen(strcpy(dp.d_name, dirp -> dd_cp -> _d_entry));

  dp.d_ino = 0;

  dp.d_size = dirp -> dd_cp -> _d_size;
  dp.d_mode = dirp -> dd_cp -> _d_mode;
  dp.d_time = dirp -> dd_cp -> _d_time;
  dp.d_date = dirp -> dd_cp -> _d_date;

  dirp -> dd_cp = dirp -> dd_cp -> _d_next;
  dirp -> dd_loc++;

  return &dp;
}


void seekdir(DIR * dirp, long off)
{
  long i = off;
  struct _dircontents *dp;

  if (off >= 0)
  {
    for (dp = dirp -> dd_contents; --i >= 0 && dp; dp = dp -> _d_next);

    dirp -> dd_loc = off - (i + 1);
    dirp -> dd_cp = dp;
  }
}


long telldir(DIR * dirp)
{
  return dirp -> dd_loc;
}


static void free_dircontents(struct _dircontents * dp)
{
  struct _dircontents *odp;

  while (dp)
  {
    if (dp -> _d_entry)
      free(dp -> _d_entry);

    dp = (odp = dp) -> _d_next;
    free(odp);
  }
}


static char *getdirent(char *dir)
{
  int done;
  static int lower;

  if (dir != NULL)
  {                                    /* get first entry */
    hdir = HDIR_SYSTEM;
    count = 1;
    done = DosFindFirst(dir, &hdir, attributes, &find, sizeof(find), &count);
    lower = IsFileSystemFAT(dir);
  }
  else                                 /* get next entry */
    done = DosFindNext(hdir, &find, sizeof(find), &count);

  if (done == 0)
  {
    if ( lower )
      StringLower(find.achName);
    return find.achName;
  }
  else
  {
    DosFindClose(hdir);
    return NULL;
  }
}


/* FAT / HPFS detection */

int IsFileSystemFAT(char *dir)
{
  static USHORT nLastDrive = -1, nResult;
  ULONG lMap;
  BYTE bData[64], bName[3];
#ifdef __32BIT__
  ULONG nDrive, cbData;
  PFSQBUFFER2 pData = (PFSQBUFFER2) bData;
#else
  USHORT nDrive, cbData;
  PFSQBUFFER pData = (PFSQBUFFER) bData;
#endif

  if ( _osmode == DOS_MODE )
    return TRUE;
  else
  {
    /* We separate FAT and HPFS+other file systems here.
       at the moment I consider other systems to be similar to HPFS,
       i.e. support long file names and beeing case sensitive */

    if ( isalpha(dir[0]) && (dir[1] == ':') )
      nDrive = to_up(dir[0]) - '@';
    else
      DosQueryCurrentDisk(&nDrive, &lMap);

    if ( nDrive == nLastDrive )
      return nResult;

    bName[0] = (char) (nDrive + '@');
    bName[1] = ':';
    bName[2] = 0;

    nLastDrive = nDrive;
    cbData = sizeof(bData);

    if ( !DosQueryFSAttach(bName, 0, FSAIL_QUERYNAME, (PVOID) pData, &cbData) )
      nResult = !strcmp(pData -> szFSDName + pData -> cbName, "FAT");
    else
      nResult = FALSE;

    /* End of this ugly code */
    return nResult;
  }
}


/* access mode bits and time stamp */

int GetFileMode(char *name)
{
#ifdef __32BIT__
  FILESTATUS3 fs;
  return DosQueryPathInfo(name, 1, &fs, sizeof(fs)) ? -1 : fs.attrFile;
#else
  USHORT mode;
  return DosQFileMode(name, &mode, 0L) ? -1 : mode;
#endif
}

long GetFileTime(char *name)
{
#ifdef __32BIT__
  FILESTATUS3 fs;
#else
  FILESTATUS fs;
#endif
  USHORT nDate, nTime;

  if ( DosQueryPathInfo(name, 1, (PBYTE) &fs, sizeof(fs)) )
    return -1;

  nDate = * (USHORT *) &fs.fdateLastWrite;
  nTime = * (USHORT *) &fs.ftimeLastWrite;

  return ((ULONG) nDate) << 16 | nTime;
}

void SetFileTime(char *path, long stamp)
{
  FILESTATUS fs;
  USHORT fd, ft;
  USHORT nLength;
  char szName[CCHMAXPATH];

  if ( DosQueryPathInfo(path, FIL_STANDARD, (PBYTE) &fs, sizeof(fs)) )
    return;

  fd = (USHORT) (stamp >> 16);
  ft = (USHORT) stamp;
  fs.fdateLastWrite = fs.fdateCreation = * (FDATE *) &fd;
  fs.ftimeLastWrite = fs.ftimeCreation = * (FTIME *) &ft;

  DosSetPathInfo(path, FIL_STANDARD, (PBYTE) &fs, sizeof(fs), 0);
}

/* read volume label */

char *getVolumeLabel(int drive, unsigned long *vtime, unsigned long *vmode)
{
  static FSINFO fi;

  if ( DosQueryFSInfo(drive ? drive - 'A' + 1 : 0, 
                      FSIL_VOLSER, (PBYTE) &fi, sizeof(fi)) )
    return NULL;

  *vtime = (unsigned long) time(NULL);
  *vmode = _A_VOLID;

  return fi.vol.szVolLabel;
}

/* FAT / HPFS name conversion stuff */

int IsFileNameValid(char *name)
{
  HFILE hf;
#ifdef __32BIT__
  ULONG uAction;
#else
  USHORT uAction;
#endif

  switch( DosOpen(name, &hf, &uAction, 0, 0, FILE_OPEN,
                  OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE, 0) )
  {
  case ERROR_INVALID_NAME:
  case ERROR_FILENAME_EXCED_RANGE:
    return FALSE;
  case NO_ERROR:
    DosClose(hf);
  default:
    return TRUE;
  }
}


void ChangeNameForFAT(char *name)
{
  char *src, *dst, *next, *ptr, *dot, *start;
  static char invalid[] = ":;,=+\"[]<>| \t";

  if ( isalpha(name[0]) && (name[1] == ':') )
    start = name + 2;
  else
    start = name;

  src = dst = start;
  if ( (*src == '/') || (*src == '\\') )
    src++, dst++;

  while ( *src )
  {
    for ( next = src; *next && (*next != '/') && (*next != '\\'); next++ );

    for ( ptr = src, dot = NULL; ptr < next; ptr++ )
      if ( *ptr == '.' )
      {
        dot = ptr; /* remember last dot */
        *ptr = '_';
      }

    if ( dot == NULL )
      for ( ptr = src; ptr < next; ptr++ )
        if ( *ptr == '_' )
          dot = ptr; /* remember last _ as if it were a dot */

    if ( dot && (dot > src) &&
         ((next - dot <= 4) ||
          ((next - src > 8) && (dot - src > 3))) )
    {
      if ( dot )
        *dot = '.';

      for ( ptr = src; (ptr < dot) && ((ptr - src) < 8); ptr++ )
        *dst++ = *ptr;

      for ( ptr = dot; (ptr < next) && ((ptr - dot) < 4); ptr++ )
        *dst++ = *ptr;
    }
    else
    {
      if ( dot && (next - src == 1) )
        *dot = '.';           /* special case: "." as a path component */

      for ( ptr = src; (ptr < next) && ((ptr - src) < 8); ptr++ )
        *dst++ = *ptr;
    }

    *dst++ = *next; /* either '/' or 0 */

    if ( *next )
    {
      src = next + 1;

      if ( *src == 0 ) /* handle trailing '/' on dirs ! */
        *dst = 0;
    }
    else
      break;
  }

  for ( src = start; *src != 0; ++src )
    if ( (strchr(invalid, *src) != NULL) || (*src == ' ') )
      *src = '_';
}


/* .LONGNAME EA code */

typedef struct
{
  ULONG cbList;               /* length of value + 22 */
#ifdef __32BIT__
  ULONG oNext;
#endif
  BYTE fEA;                   /* 0 */
  BYTE cbName;                /* length of ".LONGNAME" = 9 */
  USHORT cbValue;             /* length of value + 4 */
  BYTE szName[10];            /* ".LONGNAME" */
  USHORT eaType;              /* 0xFFFD for length-preceded ASCII */
  USHORT eaSize;              /* length of value */
  BYTE szValue[CCHMAXPATH];
}
FEALST;

typedef struct
{
  ULONG cbList;
#ifdef __32BIT__
  ULONG oNext;
#endif
  BYTE cbName;
  BYTE szName[10];            /* ".LONGNAME" */
}
GEALST;


char *GetLongNameEA(char *name)
{
  EAOP eaop;
  GEALST gealst;
  static FEALST fealst;

  if ( _osmode == DOS_MODE )
    return NULL;

  eaop.fpGEAList = (PGEALIST) &gealst;
  eaop.fpFEAList = (PFEALIST) &fealst;
  eaop.oError = 0;

  strcpy(gealst.szName, ".LONGNAME");
  gealst.cbName  = (BYTE) strlen(gealst.szName);
#ifdef __32BIT__
  gealst.oNext   = 0;
#endif

  gealst.cbList  = sizeof(gealst);
  fealst.cbList  = sizeof(fealst);

  if ( DosQueryPathInfo(name, FIL_QUERYEASFROMLIST,
                        (PBYTE) &eaop, sizeof(eaop)) )
    return NULL;

  if ( fealst.cbValue > 4 && fealst.eaType == 0xFFFD )
  {
    fealst.szValue[fealst.eaSize] = 0;
    return fealst.szValue;
  }

  return NULL;
}


char *GetLongPathEA(char *name)
{
  static char nbuf[CCHMAXPATH + 1];
  char *comp, *next, *ea, sep;
  BOOL bFound = FALSE;

  nbuf[0] = 0;
  next = name;

  while ( *next )
  {
    comp = next;

    while ( *next != '\\' && *next != '/' && *next != 0 )
      next++;

    sep = *next;
    *next = 0;

    ea = GetLongNameEA(name);
    strcat(nbuf, ea ? ea : comp);
    bFound = bFound || (ea != NULL);

    *next = sep;

    if ( *next )
    {
      strcat(nbuf, "\\");
      next++;
    }
  }

  return (nbuf[0] != 0) && bFound ? nbuf : NULL;
}


/* general EA code */

typedef struct
{
  USHORT nID;
  USHORT nSize;
  ULONG lSize;
}
EAHEADER, *PEAHEADER;


#ifdef __32BIT__

/* Perhaps due to bugs in the current OS/2 2.0 kernel, the success or
   failure of the DosEnumAttribute() and DosQueryPathInfo() system calls
   depends on the area where the return buffers are allocated. This
   differs for the various compilers, for some alloca() works, for some
   malloc() works, for some, both work. We'll have to live with that. */

/* The use of malloc() is not very convenient, because it requires
   backtracking (i.e. free()) at error returns. We do that for system
   calls that may fail, but not for malloc() calls, because they are VERY
   unlikely to fail. If ever, we just leave some memory allocated
   over the usually short lifetime of a zip process ... */

#ifdef __GNUC__
#define alloc(x) alloca(x)
#define unalloc(x)
#else
#define alloc(x) malloc(x)
#define unalloc(x) free(x)
#endif

void GetEAs(char *path, char **bufptr, size_t *size,
                        char **cbufptr, size_t *csize)
{
  FILESTATUS4 fs;
  PDENA2 pDENA, pFound;
  EAOP2 eaop;
  PGEA2 pGEA;
  PGEA2LIST pGEAlist;
  PFEA2LIST pFEAlist;
  PEAHEADER pEAblock;
  ULONG ulAttributes, ulMemoryBlock;
  ULONG nLength;
  ULONG nBlock;
  char szName[CCHMAXPATH];

  *size = *csize = 0;

  if ( _osmode == DOS_MODE )
    return;

  strcpy(szName, path);
  nLength = strlen(szName);
  if ( szName[nLength - 1] == '/' )
    szName[nLength - 1] = 0;

  if ( DosQueryPathInfo(szName, FIL_QUERYEASIZE, (PBYTE) &fs, sizeof(fs)) )
    return;
  nBlock = max(fs.cbList, 65535);
  if ( (pDENA = alloc((size_t) nBlock)) == NULL )
    return;

  ulAttributes = -1;

  if ( DosEnumAttribute(ENUMEA_REFTYPE_PATH, szName, 1, pDENA, nBlock,
                        &ulAttributes, ENUMEA_LEVEL_NO_VALUE)
    || ulAttributes == 0
    || (pGEAlist = alloc((size_t) nBlock)) == NULL )
  {
    unalloc(pDENA);
    return;
  }

  pGEA = pGEAlist -> list;
  pFound = pDENA;

  while ( ulAttributes-- )
  {
    if ( !(strcmp(pFound -> szName, ".LONGNAME") == 0 && use_longname_ea) )
    {
      pGEA -> cbName = pFound -> cbName;
      strcpy(pGEA -> szName, pFound -> szName);

      nLength = sizeof(GEA2) + strlen(pGEA -> szName);
      nLength = ((nLength - 1) / sizeof(ULONG) + 1) * sizeof(ULONG);

      pGEA -> oNextEntryOffset = ulAttributes ? nLength : 0;
      pGEA   = (PGEA2)  ((PCH) pGEA + nLength);
    }

    pFound = (PDENA2) ((PCH) pFound + pFound -> oNextEntryOffset);
  }

  if ( pGEA == pGEAlist -> list ) /* no attributes to save */
  {
    unalloc(pDENA);
    unalloc(pGEAlist);
    return;
  }

  pGEAlist -> cbList = (PCH) pGEA - (PCH) pGEAlist;

  pFEAlist = (PVOID) pDENA;  /* reuse buffer */
  pFEAlist -> cbList = nBlock;

  eaop.fpGEA2List = pGEAlist;
  eaop.fpFEA2List = pFEAlist;
  eaop.oError = 0;

  if ( DosQueryPathInfo(szName, FIL_QUERYEASFROMLIST,
                        (PBYTE) &eaop, sizeof(eaop)) )
  {
    unalloc(pDENA);
    unalloc(pGEAlist);
    return;
  }

  /* The maximum compressed size is (in case of STORE type) the
     uncompressed size plus the size of the compression type field
     plus the size of the CRC field. */

  ulAttributes = pFEAlist -> cbList;
  ulMemoryBlock = ulAttributes + sizeof(USHORT) + sizeof(ULONG);
  pEAblock = (PEAHEADER) malloc(sizeof(EAHEADER) + ulMemoryBlock);

  if ( pEAblock == NULL )
  {
    unalloc(pDENA);
    unalloc(pGEAlist);
    return;
  }

  *bufptr = (char *) pEAblock;
  *size = sizeof(EAHEADER);

  pEAblock -> nID = EAID;
  pEAblock -> nSize = sizeof(pEAblock -> lSize);
  pEAblock -> lSize = ulAttributes; /* uncompressed size */

  nLength = memcompress((char *) (pEAblock + 1), ulMemoryBlock,
                        (char *) pFEAlist, ulAttributes);
  *size += nLength;
  pEAblock -> nSize += nLength;

  if ( (pEAblock = (PEAHEADER) malloc(sizeof(EAHEADER))) == NULL )
  {
    unalloc(pDENA);
    unalloc(pGEAlist);
    return;
  }

  *cbufptr = (char *) pEAblock;
  *csize = sizeof(EAHEADER);

  pEAblock -> nID = EAID;
  pEAblock -> nSize = sizeof(pEAblock -> lSize);
  pEAblock -> lSize = ulAttributes;

  if ( noisy )
    printf(" (%ld bytes EA's)", ulAttributes);

  unalloc(pDENA);
  unalloc(pGEAlist);
}

#else /* !__32BIT__ */

typedef struct
{
  ULONG oNextEntryOffset;
  BYTE fEA;
  BYTE cbName;
  USHORT cbValue;
  CHAR szName[1];
}
FEA2, *PFEA2;

typedef struct
{
  ULONG cbList;
  FEA2 list[1];
}
FEA2LIST, *PFEA2LIST;

void GetEAs(char *path, char **bufptr, size_t *size,
                        char **cbufptr, size_t *csize)
{
  FILESTATUS2 fs;
  PDENA1 pDENA, pFound;
  EAOP eaop;
  PGEALIST pGEAlist;
  PGEA pGEA;
  PFEALIST pFEAlist;
  PFEA pFEA;
  PFEA2LIST pFEA2list;
  PFEA2 pFEA2;
  EAHEADER *pEAblock;
  ULONG ulAttributes;
  USHORT nLength, nMaxSize;
  char szName[CCHMAXPATH];

  *size = *csize = 0;

  if ( _osmode == DOS_MODE )
    return;

  strcpy(szName, path);
  nLength = strlen(szName);
  if ( szName[nLength - 1] == '/' )
    szName[nLength - 1] = 0;

  if ( DosQueryPathInfo(szName, FIL_QUERYEASIZE, (PBYTE) &fs, sizeof(fs))
    || fs.cbList <= 2 * sizeof(ULONG) )
    return;

  ulAttributes = -1;
  nMaxSize = (USHORT) min(fs.cbList * 2, 65520L);

  if ( (pDENA = malloc((size_t) nMaxSize)) == NULL )
    return;

  if ( DosEnumAttribute(ENUMEA_REFTYPE_PATH, szName, 1, pDENA, fs.cbList,
                        &ulAttributes, ENUMEA_LEVEL_NO_VALUE)
    || ulAttributes == 0
    || (pGEAlist = malloc(nMaxSize)) == NULL )
  {
    free(pDENA);
    return;
  }

  pGEA = pGEAlist -> list;
  pFound = pDENA;

  while ( ulAttributes-- )
  {
    nLength = strlen(pFound -> szName);

    if ( !(strcmp(pFound -> szName, ".LONGNAME") == 0 && use_longname_ea) )
    {
      pGEA -> cbName = pFound -> cbName;
      strcpy(pGEA -> szName, pFound -> szName);

      pGEA = (PGEA) ((PCH) (pGEA++) + nLength);
    }

    pFound = (PDENA1) ((PCH) (pFound++) + nLength);
  }

  if ( pGEA == pGEAlist -> list )
  {
    free(pDENA);
    free(pGEAlist);
    return;
  }

  pGEAlist -> cbList = (PCH) pGEA - (PCH) pGEAlist;

  pFEAlist = (PFEALIST) pDENA; /* reuse buffer */
  pFEAlist -> cbList = fs.cbList;
  pFEA = pFEAlist -> list;

  eaop.fpGEAList = pGEAlist;
  eaop.fpFEAList = pFEAlist;
  eaop.oError = 0;

  if ( DosQueryPathInfo(szName, FIL_QUERYEASFROMLIST,
                    (PBYTE) &eaop, sizeof(eaop)) )
  {
    free(pDENA);
    free(pGEAlist);
    return;
  }

  /* now convert into new OS/2 2.0 32-bit format */

  pFEA2list = (PFEA2LIST) pGEAlist;  /* reuse buffer */
  pFEA2 = pFEA2list -> list;

  while ( (PCH) pFEA - (PCH) pFEAlist < pFEAlist -> cbList )
  {
    nLength = sizeof(FEA) + pFEA -> cbName + 1 + pFEA -> cbValue;
    memcpy((PCH) pFEA2 + sizeof(pFEA2 -> oNextEntryOffset), pFEA, nLength);
    memset((PCH) pFEA2 + sizeof(pFEA2 -> oNextEntryOffset) + nLength, 0, 3);
    pFEA = (PFEA) ((PCH) pFEA + nLength);

    nLength = sizeof(FEA2) + pFEA2 -> cbName + 1 + pFEA2 -> cbValue;
    nLength = ((nLength - 1) / sizeof(ULONG) + 1) * sizeof(ULONG);
    /* rounded up to 4-byte boundary */
    pFEA2 -> oNextEntryOffset =
      ((PCH) pFEA - (PCH) pFEAlist < pFEAlist -> cbList) ? nLength : 0;
    pFEA2 = (PFEA2) ((PCH) pFEA2 + nLength);
  }

  pFEA2list -> cbList = (PCH) pFEA2 - (PCH) pFEA2list;
  ulAttributes = pFEA2list -> cbList;

  pEAblock = (PEAHEADER) pDENA; /* reuse buffer */

  *bufptr = (char *) pEAblock;
  *size = sizeof(EAHEADER);

  pEAblock -> nID = EAID;
  pEAblock -> nSize = sizeof(pEAblock -> lSize);
  pEAblock -> lSize = ulAttributes; /* uncompressed size */

  nLength = (USHORT) memcompress((char *) (pEAblock + 1),
    nMaxSize - sizeof(EAHEADER), (char *) pFEA2list, ulAttributes);

  *size += nLength;
  pEAblock -> nSize += nLength;

  pEAblock = (PEAHEADER) pGEAlist;

  *cbufptr = (char *) pEAblock;
  *csize = sizeof(EAHEADER);

  pEAblock -> nID = EAID;
  pEAblock -> nSize = sizeof(pEAblock -> lSize);
  pEAblock -> lSize = ulAttributes;

  if ( noisy )
    printf(" (%ld bytes EA's)", ulAttributes);
}

#endif /* __32BIT__ */

int set_extra_field(struct zlist *z)
{
  GetEAs(z->name, &z->extra, &z->ext, &z->cextra, &z->cext);
  /* store data in local header, and size only in central headers */
  return 0;
}

#endif /* UTIL */


/* Initialize the table of uppercase characters including handling of
   country dependent characters. */

void init_upper()
{
  COUNTRYCODE cc;
  unsigned nCnt, nU;

  for ( nCnt = 0; nCnt < sizeof(upper); nCnt++ )
    upper[nCnt] = lower[nCnt] = (unsigned char) nCnt;

  cc.country = cc.codepage = 0;
  DosMapCase(sizeof(upper), &cc, (PCHAR) upper);

  for ( nCnt = 0; nCnt < 256; nCnt++ )
  {
    nU = upper[nCnt];
    if (nU != nCnt && lower[nU] == (unsigned char) nU)
      lower[nU] = (unsigned char) nCnt;
  }

  for ( nCnt = 'A'; nCnt <= 'Z'; nCnt++ )
    lower[nCnt] = (unsigned char) (nCnt - 'A' + 'a');
}


char *StringLower(char *szArg)
{
  unsigned char *szPtr;
  for ( szPtr = szArg; *szPtr; szPtr++ )
    *szPtr = lower[*szPtr];
  return szArg;
}


#if defined(__IBMC__) && defined(__DEBUG_ALLOC__)
void DebugMalloc(void)
{
  _dump_allocated(0); /* print out debug malloc memory statistics */
}
#endif


#endif /* OS2 */
