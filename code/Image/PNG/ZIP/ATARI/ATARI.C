/*
 * ATARI.C
 *
 * Necessary interface functions, mostly for conversion
 * of path names. (Is this *really* necessary? The C library should
 * do this -- Jean-loup.)
 */
#ifdef ATARI_ST

#include <stdio.h>
#include <stdlib.h>
#include <tos.h>
#include <ext.h>

#define FNMAX 256
#define OF(sig) sig

char *st_fn OF((char *));

char *st_fn(s)
char *s;
{
static char tosname [ FNMAX ];
char *t = tosname;

  while ( *t=*s++ ) {
    if ( *t == '/' )
      *t = '\\';
    t++;
  }
  
  return(tosname);
}

int st_unlink(f)
char *f;
{
  return(unlink(st_fn(f)));
}

/* Fake chmod with minimalistic functionality.
 * [ anyway people will be in trouble with the readonly files
 *   produces by this, since 'normal' users don't own the
 *   'tools' to manipulate these. ]
 */
int st_chmod(f, a)
char *f;                /* file path */
int a;                  /* attributes returned by getfileattr() */
/* Give the file f the attributes a, return non-zero on failure */
{
  if ( ! ( a & S_IWRITE ) )
    if (Fattrib(st_fn(f), 1, FA_READONLY) < 0 )
      return(-1);
  return 0;
}

/*
 * mktemp is not part of the Turbo C library.
 */ 
char *st_mktemp(s)
char *s;
{
char *t;
long i;
  for(t=s; *t; t++)
    if ( *t == '/' )
      *t = '\\';
  t -= 6;
  i = (unsigned long)s % 1000000L;
  do {
    sprintf(t, "%06ld", i++);
  } while ( Fsfirst(s, 0x21) == 0 );
  return(s);
}

FILE *st_fopen(f,m)
char *f;
char *m;
{
  return(fopen(st_fn(f),m));
}

int st_open(f,m)
char *f;
int m;
{
  return(open(st_fn(f),m));
}

int st_stat(f, b)
char *f;
struct stat *b;
{
  return(stat(st_fn(f),b));
}

int st_findfirst(n,d,a)
char *n;
struct ffblk *d;
int a;
{
  return(findfirst( st_fn(n),(struct ffblk *)d,a));
}


int st_rename(s, d)
char *s, *d;
{
char tosname [ FNMAX ];
char *t = tosname;

  while ( *t=*s++ ) {
    if ( *t == '/' )
      *t = '\\';
    t++;
  }
  return(rename(tosname, st_fn(d)));
}  

int st_rmdir(d)
char *d;
{
  return(Ddelete(st_fn(d)));
}

#endif /* ?ATARI_ST */
