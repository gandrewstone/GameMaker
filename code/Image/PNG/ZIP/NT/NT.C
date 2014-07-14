/*
 * NT specific functions for ZIP.
 *
 * The NT version of ZIP heavily relies on the MSDOS and OS2 versions,
 * since we have to do similar things to switch between NTFS, HPFS and FAT.
 */


#include "zip.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <windows.h>
#include "ntzip.h"

#define A_RONLY    0x01
#define A_HIDDEN   0x02
#define A_SYSTEM   0x04
#define A_LABEL    0x08
#define A_DIR      0x10
#define A_ARCHIVE  0x20


#define EAID     0x0009


#ifndef UTIL

extern int noisy;


/* FAT / HPFS detection */

int IsFileSystemFAT(char *dir)
{
  char root[4];
  char vname[128];
  DWORD vnamesize = sizeof(vname);
  DWORD vserial;
  DWORD vfnsize;
  DWORD vfsflags;
  char vfsname[128];
  DWORD vfsnamesize = sizeof(vfsname);
  
    /*
     * We separate FAT and HPFS+other file systems here.
     * I consider other systems to be similar to HPFS/NTFS, i.e.
     * support for long file names and being case sensitive to some extent.
     */

    strncpy(root, dir, 3);
    if ( isalpha(root[0]) && (root[1] == ':') ) {
      root[0] = to_up(dir[0]);
      root[2] = '\\';
      root[3] = 0;
    }
    else {
      root[0] = '\\';
      root[1] = 0;
    }

    if ( !GetVolumeInformation(root, vname, vnamesize,
                         &vserial, &vfnsize, &vfsflags,
                         vfsname, vfsnamesize)) {
        fprintf(mesg, "zip diagnostic: GetVolumeInformation failed\n");
        return(FALSE);
    }

    return( strcmp(vfsname, "FAT") == 0 );
}

/* access mode bits and time stamp */

int GetFileMode(char *name)
{
DWORD dwAttr;

  dwAttr = GetFileAttributes(name);
  if ( dwAttr == -1 ) {
    fprintf(mesg, "zip diagnostic: GetFileAttributes failed\n");
    return(0x20); /* the most likely, though why the error? security? */
  }
  return(
          (dwAttr&FILE_ATTRIBUTE_READONLY  ? A_RONLY   :0)
        | (dwAttr&FILE_ATTRIBUTE_HIDDEN    ? A_HIDDEN  :0)
        | (dwAttr&FILE_ATTRIBUTE_SYSTEM    ? A_SYSTEM  :0)
        | (dwAttr&FILE_ATTRIBUTE_DIRECTORY ? A_DIR     :0)
        | (dwAttr&FILE_ATTRIBUTE_ARCHIVE   ? A_ARCHIVE :0));
}

long GetTheFileTime(char *name)
{
HANDLE h;
FILETIME ft, lft;
WORD dh, dl;

  h = CreateFile(name, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                 FILE_ATTRIBUTE_NORMAL, NULL);
  if ( h != INVALID_HANDLE_VALUE ) {
    GetFileTime(h, NULL, NULL, &ft);
    FileTimeToLocalFileTime( &ft, &lft);
    FileTimeToDosDateTime( &lft, &dh, &dl);
    CloseHandle(h);
    return(dh<<16) | dl;
  }
  else
    return 0L;
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

char *GetLongPathEA(char *name)
{
        return(NULL); /* volunteers ? */
}

int IsFileNameValid(x)
char *x;
{
        WIN32_FIND_DATA fd;
        HANDLE h = FindFirstFile(x, &fd);

        if (h == INVALID_HANDLE_VALUE) return FALSE;
        FindClose(h);
        return TRUE;
}

#endif /* UTIL */


char *StringLower(char *szArg)
{
  unsigned char *szPtr;
  for ( szPtr = szArg; *szPtr; szPtr++ )
    *szPtr = lower[*szPtr];
  return szArg;
}
