/*

 Copyright (C) 1990-1993 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  zipnote.c by Mark Adler.
 */

#define UTIL
#include "revision.h"
#include "zip.h"
#include <signal.h>


/* Character to mark zip entry names in the comment file */
#define MARK '@'
#define MARKE " (comment above this line)"
#define MARKZ " (zip file comment below this line)"

/* Temporary zip file name and file pointer */
local char *tempzip;
local FILE *tempzf;


/* Local functions */
#ifdef PROTO
   local void handler(int);
   local void license(void);
   local void help(void);
   local void putclean(char *, int);
   local int catalloc(char * far *, char *);
   int main(int, char **);
#endif /* PROTO */



void err(c, h)
int c;                  /* error code from the ZE_ class */
char *h;                /* message about how it happened */
/* Issue a message for the error, clean up files and memory, and exit. */
{
  if (PERR(c))
    perror("zipnote error");
  fprintf(stderr, "zipnote error: %s (%s)\n", errors[c-1], h);
  if (tempzf != NULL)
    fclose(tempzf);
  if (tempzip != NULL)
  {
    destroy(tempzip);
    free((voidp *)tempzip);
  }
  if (zipfile != NULL)
    free((voidp *)zipfile);
#ifdef VMS
  exit(0);
#else /* !VMS */
  exit(c);
#endif /* ?VMS */
}


local void handler(s)
int s;                  /* signal number (ignored) */
/* Upon getting a user interrupt, abort cleanly using err(). */
{
#ifndef MSDOS
  putc('\n', stderr);
#endif /* !MSDOS */
  err(ZE_ABORT, "aborting");
  s++;                                  /* keep some compilers happy */
}


void warn(a, b)
char *a, *b;            /* message strings juxtaposed in output */
/* Print a warning message to stderr and return. */
{
  fprintf(stderr, "zipnote warning: %s%s\n", a, b);
}


local void license()
/* Print license information to stdout. */
{
  extent i;             /* counter for copyright array */

  for (i = 0; i < sizeof(copyright)/sizeof(char *); i++) {
    printf(copyright[i], "zipnote");
    putchar('\n');
  }
  for (i = 0; i < sizeof(disclaimer)/sizeof(char *); i++)
    puts(disclaimer[i]);
}


local void help()
/* Print help (along with license info) to stdout. */
{
  extent i;             /* counter for help array */

  /* help array */
  static char *text[] = {
"",
"ZipNote %s (%s)",
"Usage:  zipnote [-w] [-b path] zipfile",
"  the default action is to write the comments in zipfile to stdout",
"  -w   write the zipfile comments from stdin",
"  -b   use \"path\" for the temporary zip file",
"  -h   show this help               -L   show software license",
"",
"Example:",
#ifdef VMS
"     define/user sys$output foo.tmp",
"     zipnote foo.zip",
"     edit foo.tmp",
"     ... then you edit the comments, save, and exit ...",
"     define/user sys$input foo.tmp",
"     zipnote -w foo.zip"
#else /* !VMS */
"     zipnote foo.zip > foo.tmp",
"     ed foo.tmp",
"     ... then you edit the comments, save, and exit ...",
"     zipnote -w foo.zip < foo.tmp"
#endif /* ?VMS */
  };

  for (i = 0; i < sizeof(copyright)/sizeof(char *); i++) {
    printf(copyright[i], "zipnote");
    putchar('\n');
  }
  for (i = 0; i < sizeof(text)/sizeof(char *); i++)
  {
    printf(text[i], VERSION, REVDATE);
    putchar('\n');
  }
}


local void putclean(s, n)
char *s;                /* string to write to stdout */
int n;                  /* length of string */
/* Write the string s to stdout, filtering out control characters that are
   not tab or newline (mainly to remove carriage returns), and prefix MARK's
   and backslashes with a backslash.  Also, terminate with a newline if
   needed. */
{
  int c;                /* next character in string */
  int e;                /* last character written */

  e = '\n';                     /* if empty, write nothing */
  while (n--)
  {
    c = *(uch *)s++;
    if (c == MARK || c == '\\')
      putchar('\\');
    if (c >= ' ' || c == '\t' || c == '\n')
      { e=c; putchar(e); }
  }
  if (e != '\n')
    putchar('\n');
}


local int catalloc(a, s)
char * far *a;          /* pointer to a pointer to a malloc'ed string */
char *s;                /* string to concatenate on a */
/* Concatentate the string s to the malloc'ed string pointed to by a.
   Preprocess s by removing backslash escape characters. */
{
  char *p;              /* temporary pointer */
  char *q;              /* temporary pointer */

  for (p = q = s; *q; *p++ = *q++)
    if (*q == '\\' && *(q+1))
      q++;
  *p = 0;
  if ((p = malloc(strlen(*a) + strlen(s) + 3)) == NULL)
    return ZE_MEM;
  strcat(strcat(strcpy(p, *a), **a ? "\r\n" : ""), s);
  free((voidp *)*a);
  *a = p;
  return ZE_OK;
}


int main(argc, argv)
int argc;               /* number of tokens in command line */
char **argv;            /* command line tokens */
/* Write the comments in the zipfile to stdout, or read them from stdin. */
{
  char a[FNMAX+1];      /* input line buffer */
  ulg c;                /* start of central directory */
  int k;                /* next argument type */
  char *q;              /* steps through option arguments */
  int r;                /* arg counter, temporary variable */
  ulg s;                /* length of central directory */
  int t;                /* attributes of zip file */
  int w;                /* true if updating zip file from stdin */
  FILE *x, *y;          /* input and output zip files */
  struct zlist far *z;  /* steps through zfiles linked list */


  /* If no args, show help */
  if (argc == 1)
  {
    help();
    exit(0);
  }

  init_upper();           /* build case map table */

  /* Go through args */
  zipfile = tempzip = NULL;
  tempzf = NULL;
  signal(SIGINT, handler);
#ifdef SIGTERM              /* AMIGA has no SIGTERM */
  signal(SIGTERM, handler);
#endif
  k = w = 0;
  for (r = 1; r < argc; r++)
    if (*argv[r] == '-')
      if (argv[r][1])
        for (q = argv[r]+1; *q; q++)
          switch(*q)
          {
            case 'b':   /* Specify path for temporary file */
              if (k)
                err(ZE_PARMS, "use -b before zip file name");
              else
                k = 1;          /* Next non-option is path */
              break;
            case 'h':   /* Show help */
              help();  exit(0);
            case 'l':  case 'L':  /* Show copyright and disclaimer */
              license();  exit(0);
            case 'w':
              w = 1;  break;
            default:
              err(ZE_PARMS, "unknown option");
          }
      else
        err(ZE_PARMS, "zip file cannot be stdin");
    else
      if (k == 0)
        if (zipfile == NULL)
        {
          if ((zipfile = ziptyp(argv[r])) == NULL)
            err(ZE_MEM, "was processing arguments");
        }
        else
          err(ZE_PARMS, "can only specify one zip file");
      else
      {
        tempath = argv[r];
        k = 0;
      }
  if (zipfile == NULL)
    err(ZE_PARMS, "need to specify zip file");

  /* Read zip file */
  if ((r = readzipfile()) != ZE_OK)
    err(r, zipfile);
  if (zfiles == NULL)
    err(ZE_NAME, zipfile);

  /* Put comments to stdout, if not -w */
  if (!w)
  {
    for (z = zfiles; z != NULL; z = z->nxt)
    {
      printf("%c %s\n", MARK, z->zname);
      putclean(z->comment, z->com);
      printf("%c%s\n", MARK, MARKE);
    }
    printf("%c%s\n", MARK, MARKZ);
    putclean(zcomment, zcomlen);
    exit(ZE_OK);
  }

  /* If updating comments, make sure zip file is writeable */
  if ((x = fopen(zipfile, "a")) == NULL)
    err(ZE_CREAT, zipfile);
  fclose(x);
  t = getfileattr(zipfile);

  /* Process stdin, replacing comments */
  z = zfiles;
  while (gets(a) != NULL && (a[0] != MARK || strcmp(a + 1, MARKZ)))
  {                                     /* while input and not file comment */
    if (a[0] != MARK || a[1] != ' ')    /* better be "@ name" */
      err(ZE_NOTE, "unexpected input");
    while (z != NULL && strcmp(a + 2, z->zname))
      z = z->nxt;                       /* allow missing entries in order */
    if (z == NULL)
      err(ZE_NOTE, "unknown entry name");
    if (z->com)                         /* change zip entry comment */
      free((voidp *)z->comment);
    z->comment = malloc(1);  *(z->comment) = 0;
    while (gets(a) != NULL && *a != MARK)
      if ((r = catalloc(&(z->comment), a)) != ZE_OK)
        err(r, "was building new comments");
    z->com = strlen(z->comment);
    z = z->nxt;                         /* point to next entry */
  }
  if (a != NULL)                        /* change zip file comment */
  {
    zcomment = malloc(1);  *zcomment = 0;
    while (gets(a) != NULL)
      if ((r = catalloc(&zcomment, a)) != ZE_OK)
        err(r, "was building new comments");
    zcomlen = strlen(zcomment);
  }

  /* Open output zip file for writing */
  if ((tempzf = y = fopen(tempzip = tempname(zipfile), FOPW)) == NULL)
    err(ZE_TEMP, tempzip);

  /* Open input zip file again, copy preamble if any */
  if ((x = fopen(zipfile, FOPR)) == NULL)
    err(ZE_NAME, zipfile);
  if (zipbeg && (r = fcopy(x, y, zipbeg)) != ZE_OK)
    err(r, r == ZE_TEMP ? tempzip : zipfile);

  /* Go through local entries, copying them over as is */
  for (z = zfiles; z != NULL; z = z->nxt)
    if ((r = zipcopy(z, x, y)) != ZE_OK)
      err(r, "was copying an entry");
  fclose(x);

  /* Write central directory and end of central directory with new comments */
  if ((c = ftell(y)) == -1L)    /* get start of central */
    err(ZE_TEMP, tempzip);
  for (z = zfiles; z != NULL; z = z->nxt)
    if ((r = putcentral(z, y)) != ZE_OK)
      err(r, tempzip);
  if ((s = ftell(y)) == -1L)    /* get end of central */
    err(ZE_TEMP, tempzip);
  s -= c;                       /* compute length of central */
  if ((r = putend((int)zcount, s, c, zcomlen, zcomment, y)) != ZE_OK)
    err(r, tempzip);
  tempzf = NULL;
  if (fclose(y))
    err(ZE_TEMP, tempzip);
  if ((r = replace(zipfile, tempzip)) != ZE_OK)
  {
    warn("new zip file left as: ", tempzip);
    free((voidp *)tempzip);
    tempzip = NULL;
    err(r, "was replacing the original zip file");
  }
  free((voidp *)tempzip);
  tempzip = NULL;
  setfileattr(zipfile, t);
  free((voidp *)zipfile);
  zipfile = NULL;

  /* Done! */
  exit(ZE_OK);
  return 0; /* avoid warning */
}
