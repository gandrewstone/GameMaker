/*

 Copyright (C) 1990-1993 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  zip.c by Mark Adler.
 */

#include "revision.h"
#include "zip.h"
#include "crypt.h"
#ifdef VMS
#  include "VMSmunch.h"
#endif
#if (defined(MSDOS) && !defined(__GO32__)) || defined(__human68k__)
#  include <process.h>
#endif
#include <signal.h>

#ifdef MACOS
#  include <console.h>
#endif

#define MAXCOM 256      /* Maximum one-line comment size */


/* Local option flags */
#define DELETE  0
#define ADD     1
#define UPDATE  2
#define FRESHEN 3
local int action = ADD; /* one of ADD, UPDATE, FRESHEN, or DELETE */
local int comadd = 0;   /* 1=add comments for new files */
local int zipedit = 0;  /* 1=edit zip comment and all file comments */
local int latest = 0;   /* 1=set zip file time to time of latest file */
local ulg before = 0;   /* 0=ignore, else exclude files before this time */
local int test = 0;     /* 1=test zip file with unzip -t */
local int tempdir = 0;  /* 1=use temp directory (-b) */

/* Temporary zip file name and file pointer */
local char *tempzip;
local FILE *tempzf;

/* Local functions */

local void freeup  OF((void));
local void leave   OF((int));
local void handler OF((int));
local void license OF((void));
local void help    OF((void));
local void zipstdout OF((void));
local void check_zipfile OF((char *zipname));
local void get_filters OF((int argc, char **argv));
      int  main     OF((int, char **));
local int count_args OF((char *s));
local void envargs   OF((int *Pargc, char ***Pargv, char *envstr));



local void freeup()
/* Free all allocations in the found list and the zfiles list */
{
  struct flist far *f;  /* steps through found list */
  struct zlist far *z;  /* pointer to next entry in zfiles list */

  for (f = found; f != NULL; f = fexpel(f))
    ;
  while (zfiles != NULL)
  {
    z = zfiles->nxt;
    free((voidp *)(zfiles->name));
    if (zfiles->zname != zfiles->name)
      free((voidp *)(zfiles->zname));
    if (zfiles->ext)
      free((voidp *)(zfiles->extra));
    if (zfiles->cext && zfiles->cextra != zfiles->extra)
      free((voidp *)(zfiles->cextra));
    if (zfiles->com)
      free((voidp *)(zfiles->comment));
    farfree((voidp far *)zfiles);
    zfiles = z;
    zcount--;
  }
}


local void leave(e)
int e;                  /* exit code */
/* Process -o and -m options (if specified), free up malloc'ed stuff, and
   exit with the code e. */
{
  int r;                /* return value from trash() */
  ulg t;                /* latest time in zip file */
  struct zlist far *z;  /* pointer into zfile list */

  /* If latest, set time to zip file to latest file in zip file */
  if (latest && zipfile && strcmp(zipfile, "-"))
  {
    diag("changing time of zip file to time of latest file in it");
    /* find latest time in zip file */
    if (zfiles == NULL)
       warn("zip file is empty, can't make it as old as latest entry", "");
    else {
      t = 0;
      for (z = zfiles; z != NULL; z = z->nxt)
        /* Ignore directories in time comparisons */
        if (t < z->tim && z->zname[z->nam-1] != '/')
          t = z->tim;
      /* set modified time of zip file to that time */
      if (t != 0)
        stamp(zipfile, t);
      else
        warn(
         "zip file has only directories, can't make it as old as latest entry",
         "");
    }
  }
  if (tempath != NULL)
  {
    free((voidp *)tempath);
    tempath = NULL;
  }
  if (zipfile != NULL)
  {
    free((voidp *)zipfile);
    zipfile = NULL;
  }


  /* If dispose, delete all files in the zfiles list that are marked */
  if (dispose)
  {
    diag("deleting files that were added to zip file");
    if ((r = trash()) != ZE_OK)
      err(r, "was deleting moved files and directories");
  }


  /* Done! */
  freeup();
#ifdef VMS
  exit(0);
#else /* !VMS */
  exit(e);
#endif /* ?VMS */
}


void err(c, h)
int c;                  /* error code from the ZE_ class */
char *h;                /* message about how it happened */
/* Issue a message for the error, clean up files and memory, and exit. */
{
  static int error_level;
  if (error_level++ > 0) exit(0);  /* avoid recursive err() */

  if (h != NULL) {
    if (PERR(c))
      perror("zip error");
    fflush(mesg);
    fprintf(stderr, "\nzip error: %s (%s)\n", errors[c-1], h);
  }
  if (tempzip != NULL)
  {
    if (tempzip != zipfile) {
      if (tempzf != NULL)
        fclose(tempzf);
#ifndef DEBUG
      destroy(tempzip);
#endif
      free((voidp *)tempzip);
    } else {
      /* -g option, attempt to restore the old file */
      int k = 0;                        /* keep count for end header */
      ulg cb = cenbeg;                  /* get start of central */
      struct zlist far *z;  /* steps through zfiles linked list */

      fprintf(stderr, "attempting to restore %s to its previous state\n",
         zipfile);
      fseek(tempzf, cenbeg, SEEK_SET);
      tempzn = cenbeg;
      for (z = zfiles; z != NULL; z = z->nxt)
      {
        putcentral(z, tempzf);
        tempzn += 4 + CENHEAD + z->nam + z->cext + z->com;
        k++;
      }
      putend(k, tempzn - cb, cb, zcomlen, zcomment, tempzf);
      tempzf = NULL;
      fclose(tempzf);
    }
  }
  if (key != NULL)
    free((voidp *)key);
  if (tempath != NULL)
    free((voidp *)tempath);
  if (zipfile != NULL)
    free((voidp *)zipfile);
  freeup();
#ifdef VMS
  c = 0;
#endif
  exit(c);
}


void error(h)
  char *h;
/* Internal error, should never happen */
{
  err(ZE_LOGIC, h);
}

local void handler(s)
int s;                  /* signal number (ignored) */
/* Upon getting a user interrupt, turn echo back on for tty and abort
   cleanly using err(). */
{
#if !defined(MSDOS) && !defined(__human68k__)
  echon();
  putc('\n', stderr);
#endif /* !MSDOS */
  err(ZE_ABORT, "aborting");
  s++;                                  /* keep some compilers happy */
}


void warn(a, b)
char *a, *b;            /* message strings juxtaposed in output */
/* Print a warning message to stderr and return. */
{
  if (noisy) fprintf(stderr, "zip warning: %s%s\n", a, b);
}


local void license()
/* Print license information to stdout. */
{
  extent i;             /* counter for copyright array */

  for (i = 0; i < sizeof(copyright)/sizeof(char *); i++) {
    printf(copyright[i], "zip");
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
"Zip %s (%s). Usage:",
"zip [-options] [-b path] [-t mmddyy] [-n suffixes] [zipfile list] [-xi list]",
"  The default action is to add or replace zipfile entries from list, which",
"  can include the special name - to compress standard input.",
"  If zipfile and list are omitted, zip compresses stdin to stdout.",
"  -f   freshen: only changed files  -u   update: only changed or new files",
"  -d   delete entries in zipfile    -m   move into zipfile (delete files)",
"  -k   simulate PKZIP made zipfile  -g   allow growing existing zipfile",
"  -r   recurse into directories     -j   junk (don't record) directory names",
"  -0   store only                   -l   convert LF to CR LF (-ll CR LF to LF)",
"  -1   compress faster              -9   compress better",
"  -q   quiet operation              -v   verbose operation",
"  -c   add one-line comments        -z   add zipfile comment",
"  -b   use \"path\" for temp file     -t   only do files after \"mmddyy\"",
"  -@   read names from stdin        -o   make zipfile as old as latest entry",
"  -x   exclude the following names  -i   include only the following names",
#ifdef VMS
" \"-F\"  fix zipfile(-FF try harder) \"-D\"  do not add directory entries",
" \"-T\"  test zipfile integrity      \"-L\"  show software license",
"  -w   append the VMS version number to the name stored in the zip file",
" \"-V\"  save VMS file attributes",
#else
"  -F   fix zipfile (-FF try harder) -D   do not add directory entries",
"  -T   test zipfile integrity       -L   show software license",
#endif /* VMS */
#ifdef OS2
"  -E   use the .LONGNAME Extended attribute (if found) as filename",
#endif /* OS2 */
#ifdef S_IFLNK
"  -y   store symbolic links as the link instead of the referenced file",
#endif /* !S_IFLNK */
#if defined(MSDOS) || defined(OS2)
"  -$   include volume label         -S   include system and hidden files",
#endif
#ifdef CRYPT
"  -e   encrypt  (-ee verify key)    -n   don't compress these suffixes"
#else
"  -h   show this help               -n   don't compress these suffixes"
#endif
  };

  for (i = 0; i < sizeof(copyright)/sizeof(char *); i++)
  {
    printf(copyright[i], "zip");
    putchar('\n');
  }
  for (i = 0; i < sizeof(text)/sizeof(char *); i++)
  {
    printf(text[i], VERSION, REVDATE);
    putchar('\n');
  }
}


/* Do command line expansion for MSDOS, VMS, ATARI, and AMIGA but not OS/2 */
#if defined(MSVMS) || defined(AMIGA)
#  define PROCNAME(n) (action==ADD||action==UPDATE?wild(n):procname(n))
#else /* !MSVMS */
#  define PROCNAME(n) procname(n)
#endif /* ?MSVMS */

local void zipstdout()
/* setup for writing zip file on stdout */
{
  int r;
  mesg = stderr;
  if (isatty(1))
    err(ZE_PARMS, "cannot write zip file to terminal");
  if ((zipfile = malloc(4)) == NULL)
    err(ZE_MEM, "was processing arguments");
  strcpy(zipfile, "-");
  if ((r = readzipfile()) != ZE_OK)
    err(r, zipfile);
}


local void check_zipfile(zipname)
  char *zipname;
  /* Invoke unzip -t on the given zip file */
{
#if (defined(MSDOS) && !defined(__GO32__)) || defined(__human68k__)
   if (spawnlp(P_WAIT, "unzip", "unzip", verbose ? "-t" : "-tqq",
               zipname, NULL)) {
#else
   char cmd[128];
   strcpy(cmd, "unzip -t ");
   if (!verbose) strcat(cmd, "-qq ");
   strcat(cmd, zipname);
# ifdef VMS
   if (!system(cmd)) {
# else
   if (system(cmd)) {
# endif
#endif
     fprintf(mesg, "test of %s FAILED\n", zipfile);
     err(ZE_TEST, "original files unmodified");
   }
   if (noisy)
     fprintf(mesg, "test of %s OK\n", zipfile);
}

local void get_filters(argc, argv)
  int argc;               /* number of tokens in command line */
  char **argv;            /* command line tokens */
/* Counts number of -i or -x patterns, sets patterns and pcount */
{
  int i;
  int flag = 0;

  pcount = 0;
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strrchr(argv[i], 'i') != NULL) {
        flag = 'i';
      } else if (strrchr(argv[i], 'x') != NULL) {
        flag = 'x';
      } else {
        flag = 0;
      }
    } else if (flag) {
      if (patterns != NULL) {
        patterns[pcount].zname = ex2in(argv[i], 0, (int *)NULL);
        patterns[pcount].select = flag;
        if (flag == 'i') icount++;
      }
      pcount++;
    }
  }
  if (pcount == 0 || patterns != NULL) return;
  patterns = (struct plist*) malloc(pcount * sizeof(struct plist));
  if (patterns == NULL)
    err(ZE_MEM, "was creating pattern list");
  get_filters(argc, argv);
}


int main(argc, argv)
int argc;               /* number of tokens in command line */
char **argv;            /* command line tokens */
/* Add, update, freshen, or delete zip entries in a zip file.  See the
   command help in help() above. */
{
  int a;                /* attributes of zip file */
  ulg c;                /* start of central directory */
  int d;                /* true if just adding to a zip file */
  char *e;              /* malloc'd comment buffer */
  struct flist far *f;  /* steps through found linked list */
  int i;                /* arg counter, root directory flag */
  int k;                /* next argument type, marked counter,
                           comment size, entry count */
  ulg n;                /* total of entry len's */
  int o;                /* true if there were any ZE_OPEN errors */
  char *p;              /* steps through option arguments */
  char *pp;             /* temporary pointer */
  int r;                /* temporary variable */
  int s;                /* flag to read names from stdin */
  ulg t;                /* file time, length of central directory */
  struct zlist far *v;  /* temporary variable */
  struct zlist far * far *w;    /* pointer to last link in zfiles list */
  FILE *x, *y;          /* input and output zip files */
  struct zlist far *z;  /* steps through zfiles linked list */
  char *zipbuf;         /* stdio buffer for the zip file */
  FILE *comment_stream; /* set to stderr if anything is read from stdin */

#if defined(__IBMC__) && defined(__DEBUG_ALLOC__)
  {
    extern void DebugMalloc(void);
    atexit(DebugMalloc);
  }
#endif

  mesg = (FILE *) stdout; /* cannot be made at link time for VMS */
  comment_stream = (FILE *)stdin;

  init_upper();           /* build case map table */

#ifdef MACOS
   argc = ccommand(&argv);
#endif

  /* Process arguments */
  diag("processing arguments");
  if (argc == 1 && isatty(1))
  {
    help();
    exit(0);
  }
  envargs(&argc, &argv, "ZIPOPT"); /* get options from environment */

  zipfile = tempzip = NULL;
  tempzf = NULL;
  d = 0;                        /* disallow adding to a zip file */
  signal(SIGINT, handler);
#ifdef SIGTERM                  /* AMIGADOS and others have no SIGTERM */
  signal(SIGTERM, handler);
#endif
  k = 0;                        /* Next non-option argument type */
  s = 0;                        /* set by -@ if -@ is early */

  get_filters(argc, argv);      /* scan first the -x and -i patterns */

  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
      if (argv[i][1])
        for (p = argv[i]+1; *p; p++)
          switch(*p)
          {
            case '0':
              method = STORE; level = 0; break;
            case '1':  case '2':  case '3':  case '4':
            case '5':  case '6':  case '7':  case '8':  case '9':
                        /* Set the compression efficacy */
              level = *p - '0';  break;
            case 'b':   /* Specify path for temporary file */
              tempdir = 1;
              if (k != 0)
                err(ZE_PARMS, "use -b before zip file name");
              else
                k = 1;          /* Next non-option is path */
              break;
            case 'c':   /* Add comments for new files in zip file */
              comadd = 1;  break;
            case 'd':   /* Delete files from zip file */
              if (action != ADD)
                err(ZE_PARMS, "specify just one action");
              action = DELETE;
              break;
            case 'D':   /* Do not add directory entries */
              dirnames = 0; break;
            case 'e':   /* Encrypt */
#ifndef CRYPT
              err(ZE_PARMS, "encryption not supported");
#else /* CRYPT */
              e = key == NULL ? (char *)NULL : key;
              if ((key = malloc(PWLEN+1)) == NULL)
                err(ZE_MEM, "was getting encryption password");
              if (getp(e == NULL ? "Enter password: " : "Verify password: ",
                       key, PWLEN+1) == NULL)
                err(ZE_PARMS, "stderr is not a tty");
              if (e != NULL)
              {
                r = strcmp(key, e);
                free((voidp *)e);
                if (r)
                  err(ZE_PARMS, "password not verified");
              }
#endif /* ?CRYPT */
              break;
            case 'F':   /* fix the zip file */
              fix++; break;
            case 'f':   /* Freshen zip file--overwrite only */
              if (action != ADD)
                err(ZE_PARMS, "specify just one action");
              action = FRESHEN;
              break;
            case 'g':   /* Allow appending to a zip file */
              d = 1;  break;
            case 'h': case 'H': case '?':  /* Help */
              help();
              leave(ZE_OK);
            case 'j':   /* Junk directory names */
              pathput = 0;  break;
            case 'k':   /* Make entries using DOS names (k for Katz) */
              dosify = 1;  break;
            case 'l':   /* Translate end-of-line */
              translate_eol++; break;
            case 'L':   /* Show license, version */
              license();
              leave(ZE_OK);
            case 'm':   /* Delete files added or updated in zip file */
              dispose = 1;  break;
            case 'n':   /* Don't compress files with a special suffix */
              special = NULL; /* will be set at next argument */
              break;
            case 'o':   /* Set zip file time to time of latest file in it */
              latest = 1;  break;
            case 'p':   /* Store path with name */
              break;            /* (do nothing as annoyance avoidance) */
            case 'q':   /* Quiet operation */
              noisy = 0;
              if (verbose) verbose--;
              break;
            case 'r':   /* Recurse into subdirectories */
              recurse = 1;  break;
            case 'S':
              hidden_files = 1;
              break;
            case 't':   /* Exclude files earlier than specified date */
              if (before)
                err(ZE_PARMS, "can only have one -t");
              k = 2;  break;
            case 'T':   /* test zip file */
              test = 1; break;
            case 'u':   /* Update zip file--overwrite only if newer */
              if (action != ADD)
                err(ZE_PARMS, "specify just one action");
              action = UPDATE;
              break;
            case 'v':   /* Mention oddities in zip file structure */
              noisy = 1;
              verbose++;
              break;
#ifdef VMS
            case 'V':   /* Store in VMS format */
              vms_native = 1; break;
            case 'w':   /* Append the VMS version number */
              vmsver = 1;  break;
#endif /* VMS */
            case 'i':   /* Include only the following files */
            case 'x':   /* Exclude following files */
              if (k != 4 &&
                  (k != 3 || (action != UPDATE && action != FRESHEN)))
                err(ZE_PARMS, "nothing to select from");
              k = 5;
              break;
#ifdef S_IFLNK
            case 'y':   /* Store symbolic links as such */
              linkput = 1;  break;
#endif /* S_IFLNK */
            case 'z':   /* Edit zip file comment */
              zipedit = 1;  break;
#if defined(MSDOS) || defined(OS2)
            case '$':   /* Include volume label */
              volume_label = 1; break;
#endif
            case '@':   /* read file names from stdin */
              comment_stream = NULL;
              if (k < 3)        /* zip file not read yet */
                s = 1;          /* defer -@ until after zipfile read */
              else if (strcmp(zipfile, "-") == 0)
                err(ZE_PARMS, "can't use - and -@ together");
              else              /* zip file read--do it now */
                while ((pp = getnam(errbuf)) != NULL)
                {
                  if ((r = PROCNAME(pp)) != ZE_OK)
                    if (r == ZE_MISS)
                      warn("name not matched: ", pp);
                    else
                      err(r, pp);
                }
              break;
#ifdef OS2
            case 'E':
              /* use the .LONGNAME EA (if any) as the file's name. */
              use_longname_ea = 1;
              break;
#endif
            default:
            {
              sprintf(errbuf, "no such option: %c", *p);
              err(ZE_PARMS, errbuf);
            }
          }
      else              /* just a dash */
        switch (k)
        {
        case 0:
          zipstdout();
          k = 3;
          if (s)
            err(ZE_PARMS, "can't use - and -@ together");
          break;
        case 1:
          err(ZE_PARMS, "invalid path");
          break;
        case 2:
          err(ZE_PARMS, "invalid time");
          break;
        case 3:  case 4:
          comment_stream = NULL;
          if ((r = PROCNAME(argv[i])) != ZE_OK)
            if (r == ZE_MISS)
              warn("name not matched: ", argv[i]);
            else
              err(r, argv[i]);
          if (k == 3)
            k = 4;
        }
    else                /* not an option */
    {
      if (special == NULL)
        special = argv[i];
      else if (k == 5)
        break; /* -i and -x arguments already scanned */
      else switch (k)
      {
        case 0:
          if ((zipfile = ziptyp(argv[i])) == NULL)
            err(ZE_MEM, "was processing arguments");
          if ((r = readzipfile()) != ZE_OK)
            err(r, zipfile);
          k = 3;
          if (s)
          {
            while ((pp = getnam(errbuf)) != NULL)
            {
              if ((r = PROCNAME(pp)) != ZE_OK)
                if (r == ZE_MISS)
                  warn("name not matched: ", pp);
                else
                  err(r, pp);
            }
            s = 0;
          }
          break;
        case 1:
          if ((tempath = malloc(strlen(argv[i]) + 1)) == NULL)
            err(ZE_MEM, "was processing arguments");
          strcpy(tempath, argv[i]);
          k = 0;
          break;
        case 2:
        {
          int yy, mm, dd;       /* results of sscanf() */

          if (sscanf(argv[i], "%2d%2d%2d", &mm, &dd, &yy) != 3 ||
              mm < 1 || mm > 12 || dd < 1 || dd > 31)
            err(ZE_PARMS, "invalid date entered for -t option");
          before = dostime(yy + (yy < 80 ? 2000 : 1900), mm, dd, 0, 0, 0);
          k = 0;
          break;
        }
        case 3:  case 4:
          if ((r = PROCNAME(argv[i])) != ZE_OK)
            if (r == ZE_MISS)
              warn("name not matched: ", argv[i]);
            else
              err(r, argv[i]);
          if (k == 3)
            k = 4;
      }
    }
  }
  if (k < 3) {               /* zip used as filter */
    zipstdout();
    comment_stream = NULL;
    if ((r = procname("-")) != ZE_OK)
      if (r == ZE_MISS)
        warn("name not matched: ", "-");
      else
        err(r, "-");
    k = 4;
    if (s)
      err(ZE_PARMS, "can't use - and -@ together");
  }

  /* Clean up selections */
  if (k == 3 && (action == UPDATE || action == FRESHEN)) {
    for (z = zfiles; z != NULL; z = z->nxt) {
      /* if -u or -f with no args, do all */
      z->mark = pcount ? filter(z->zname) : 1;
    }
  }
  if ((r = check_dup()) != ZE_OK)     /* remove duplicates in found list */
    if (r == ZE_PARMS)
      err(r, "cannot repeat names in zip file");
    else
      err(r, "was processing list of files");

  if (zcount)
    free((voidp *)zsort);

  /* Check option combinations */
  if (special == NULL) 
    err(ZE_PARMS, "missing suffix list");
  if (level == 9 || !strcmp(special, ";") || !strcmp(special, ":"))
    special = NULL; /* compress everything */

  if (action == DELETE && (method != BEST || dispose || recurse ||
      key != NULL || comadd || zipedit))
    err(ZE_PARMS, "invalid option(s) used with -d");
  if (linkput && dosify)
    {
      warn("can't use -y with -k, -y ignored", "");
      linkput = 0;
    }
  if (test && !strcmp(zipfile, "-")) {
    warn("can't use -T on stdout, -T ignored", "");
  }
  if ((action != ADD || d) && !strcmp(zipfile, "-"))
    err(ZE_PARMS, "can't use -d,-f,-u or -g on stdout\n");
#ifdef VMS
  if (vms_native && translate_eol)
    err(ZE_PARMS, "can't use -V with -l");
#endif
  if (zcount == 0 && (action != ADD || d))
    warn(zipfile, " not found or empty");


  /* If -b not specified, make temporary path the same as the zip file */
#if defined(MSDOS) || defined(__human68k__) || defined(AMIGA)
  if (tempath == NULL && ((p = strrchr(zipfile, '/')) != NULL ||
#  ifdef MSDOS
                          (p = strrchr(zipfile, '\\')) != NULL ||
#  endif
                          (p = strrchr(zipfile, ':')) != NULL))
  {
    if (*p == ':')
      p++;
#else
  if (tempath == NULL && (p = strrchr(zipfile, '/')) != NULL)
  {
#endif
    if ((tempath = malloc((int)(p - zipfile) + 1)) == NULL)
      err(ZE_MEM, "was processing arguments");
    r = *p;  *p = 0;
    strcpy(tempath, zipfile);
    *p = (char)r;
  }

  /* For each marked entry, if not deleting, check if it exists, and if
     updating or freshening, compare date with entry in old zip file.
     Unmark if it doesn't exist or is too old, else update marked count. */
  diag("stating marked entries");
  k = 0;                        /* Initialize marked count */
  for (z = zfiles; z != NULL; z = z->nxt)
    if (z->mark) {
      if (action != DELETE &&
                ((t = filetime(z->name, (ulg *)NULL, (long *)NULL)) == 0 ||
                 t < before ||
                 ((action == UPDATE || action == FRESHEN) && t <= z->tim)))
      {
        z->mark = 0;
        z->trash = t && t >= before;    /* delete if -um or -fm */
        if (verbose) {
          fprintf(mesg, "zip diagnostic: %s %s\n", z->name,
                 z->trash ? "up to date" : "missing or early");
        }
      }
      else
        k++;
#if 0
      fprintf(mesg, "t %ld, z->tim %ld, t-tim %ld\n", t, z->tim,
              t-z->tim); /* ??? */
#endif
    }

  /* Remove entries from found list that do not exist or are too old */
  diag("stating new entries");
  for (f = found; f != NULL;)
    if (action == DELETE || action == FRESHEN ||
        (t = filetime(f->name, (ulg *)NULL, (long *)NULL)) == 0 ||
        t < before || (namecmp(f->name, zipfile) == 0 && strcmp(zipfile, "-")))
      f = fexpel(f);
    else
      f = f->nxt;

  /* Make sure there's something left to do */
  if (k == 0 && found == NULL &&
      !(zfiles != NULL && (latest || fix || zipedit))) {
    if (test && (zfiles != NULL || zipbeg != 0)) {
      check_zipfile(zipfile);
      leave(ZE_OK);
    }
    if (action == UPDATE || action == FRESHEN)
      leave(ZE_OK);
    else if (zfiles == NULL && (latest || fix))
      err(ZE_NAME, zipfile);
    else
      err(ZE_NONE, zipfile);
  }
  d = (d && k == 0 && (zipbeg || zfiles != NULL)); /* d true if appending */


  /* Before we get carried away, make sure zip file is writeable. This
   * has the undesired side effect of leaving one empty junk file on a WORM,
   * so when the zipfile does not exist already and when -b is specified,
   * the writability check is made in replace().
   */
  if (strcmp(zipfile, "-"))
  {
    if (tempdir && zfiles == NULL && zipbeg == 0) {
      a = 0;
    } else {
       x = zfiles == NULL && zipbeg == 0 ? fopen(zipfile, FOPW) : 
                                           fopen(zipfile, FOPM);
      /* Note: FOPW and FOPM expand to several parameters for VMS */
      if (x == NULL)
        err(ZE_CREAT, zipfile);
      fclose(x);
      a = getfileattr(zipfile);
      if (zfiles == NULL && zipbeg == 0)
        destroy(zipfile);
    }
  }
  else
    a = 0;

  /* Throw away the garbage in front of the zip file for -F */
  if (fix) zipbeg = 0;

  /* Open zip file and temporary output file */
  diag("opening zip file and creating temporary zip file");
  x = NULL;
  tempzn = 0;
  if (strcmp(zipfile, "-") == 0)
  {
#if defined(MSDOS) || defined(__human68k__)
    /* Set stdout mode to binary for MSDOS systems */
    setmode(fileno(stdout), O_BINARY);
    tempzf = y = fdopen(fileno(stdout), FOPW);
#else
    tempzf = y = stdout;
#endif
    /* tempzip must be malloced so a later free won't barf */
    tempzip = malloc(4);
    if (tempzip == NULL)
      err(ZE_MEM, "allocating temp filename");
    strcpy(tempzip, "-");
  }
  else if (d) /* d true if just appending (-g) */
  {
    if ((y = fopen(zipfile, FOPM)) == NULL)
      err(ZE_NAME, zipfile);
    tempzip = zipfile;
    tempzf = y;
    if (fseek(y, cenbeg, SEEK_SET))
      err(ferror(y) ? ZE_READ : ZE_EOF, zipfile);
    tempzn = cenbeg;
  }
  else
  {
    if ((zfiles != NULL || zipbeg) && (x = fopen(zipfile, FOPR_EX)) == NULL)
      err(ZE_NAME, zipfile);
    if ((tempzip = tempname(zipfile)) == NULL)
      err(ZE_MEM, "allocating temp filename");
    if ((tempzf = y = fopen(tempzip, FOPW)) == NULL)
      err(ZE_TEMP, tempzip);
    if (zipbeg && (r = fcopy(x, y, zipbeg)) != ZE_OK)
      err(r, r == ZE_TEMP ? tempzip : zipfile);
    tempzn = zipbeg;
  }
#ifndef VMS
  /* Use large buffer to speed up stdio: */
  zipbuf = (char *)malloc(ZBSZ);
  if (zipbuf == NULL)
    err(ZE_MEM, tempzip);
# ifdef _IOFBF
  setvbuf(y, zipbuf, _IOFBF, ZBSZ);
# else
  setbuf(y, zipbuf);
# endif /* _IOBUF */
#endif /* VMS */
  o = 0;                                /* no ZE_OPEN errors yet */


  /* Process zip file, updating marked files */
  if (zfiles != NULL)
    diag("going through old zip file");
  w = &zfiles;
  while ((z = *w) != NULL)
    if (z->mark)
    {
      /* if not deleting, zip it up */
      if (action != DELETE)
      {
        if (noisy)
        {
          fprintf(mesg, "updating: %s", z->zname);
          fflush(mesg);
        }
        if ((r = zipup(z, y)) != ZE_OK && r != ZE_OPEN)
        {
          if (noisy)
          {
            putc('\n', mesg);
            fflush(mesg);
          }
          sprintf(errbuf, "was zipping %s", z->name);
          err(r, errbuf);
        }
        if (r == ZE_OPEN)
        {
          o = 1;
          if (noisy)
          {
            putc('\n', mesg);
            fflush(mesg);
          }
          perror("zip warning");
          warn("could not open for reading: ", z->name);
          warn("will just copy entry over: ", z->zname);
          if ((r = zipcopy(z, x, y)) != ZE_OK)
          {
            sprintf(errbuf, "was copying %s", z->zname);
            err(r, errbuf);
          }
          z->mark = 0;
        }
        w = &z->nxt;
      }
      else
      {
        if (noisy)
        {
          fprintf(mesg, "deleting: %s\n", z->zname);
          fflush(mesg);
        }
        v = z->nxt;                     /* delete entry from list */
        free((voidp *)(z->name));
        free((voidp *)(z->zname));
        if (z->ext)
          free((voidp *)(z->extra));
        if (z->cext && z->cextra != z->extra)
          free((voidp *)(z->cextra));
        if (z->com)
          free((voidp *)(z->comment));
        farfree((voidp far *)z);
        *w = v;
        zcount--;
      }
    }
    else
    {
      /* copy the original entry verbatim */
      if (!d && (r = zipcopy(z, x, y)) != ZE_OK)
      {
        sprintf(errbuf, "was copying %s", z->zname);
        err(r, errbuf);
      }
      w = &z->nxt;
    }


  /* Process the edited found list, adding them to the zip file */
  diag("zipping up new entries, if any");
  for (f = found; f != NULL; f = fexpel(f))
  {
    /* add a new zfiles entry and set the name */
    if ((z = (struct zlist far *)farmalloc(sizeof(struct zlist))) == NULL)
      err(ZE_MEM, "was adding files to zip file");
    z->nxt = NULL;
    z->name = f->name;
    f->name = NULL;
    z->zname = f->zname;
    f->zname = NULL;
    z->ext = z->cext = z->com = 0;
    z->mark = 1;
    z->dosflag = f->dosflag;
    /* zip it up */
    if (noisy)
    {
      fprintf(mesg, "  adding: %s", z->zname);
      fflush(mesg);
    }
    if ((r = zipup(z, y)) != ZE_OK  && r != ZE_OPEN)
    {
      if (noisy)
      {
        putc('\n', mesg);
        fflush(mesg);
      }
      sprintf(errbuf, "was zipping %s", z->name);
      err(r, errbuf);
    }
    if (r == ZE_OPEN)
    {
      o = 1;
      if (noisy)
      {
        putc('\n', mesg);
        fflush(mesg);
      }
      perror("zip warning");
      warn("could not open for reading: ", z->name);
      free((voidp *)(z->name));
      free((voidp *)(z->zname));
      farfree((voidp far *)z);
    }
    else
    {
      *w = z;
      w = &z->nxt;
      zcount++;
    }
  }
  if (key != NULL)
  {
    free((voidp *)key);
    key = NULL;
  }


  /* Get one line comment for each new entry */
  if (comadd)
  {
    if (comment_stream == NULL) {
      comment_stream = fdopen(fileno(stderr), "r");
    }
    if ((e = malloc(MAXCOM + 1)) == NULL)
      err(ZE_MEM, "was reading comment lines");
    for (z = zfiles; z != NULL; z = z->nxt)
      if (z->mark)
      {
        if (noisy)
          fprintf(mesg, "Enter comment for %s:\n", z->name);
        if (fgets(e, MAXCOM+1, comment_stream) != NULL)
        {
          if ((p = malloc((k = strlen(e))+1)) == NULL)
          {
            free((voidp *)e);
            err(ZE_MEM, "was reading comment lines");
          }
          strcpy(p, e);
          if (p[k-1] == '\n')
            p[--k] = 0;
          z->comment = p;
          z->com = k;
        }
      }
    free((voidp *)e);
  }

  /* Get multi-line comment for the zip file */
  if (zipedit)
  {
    if (comment_stream == NULL) {
      comment_stream = fdopen(fileno(stderr), "r");
    }
    if ((e = malloc(MAXCOM + 1)) == NULL)
      err(ZE_MEM, "was reading comment lines");
    if (noisy && zcomlen)
    {
      fputs("current zip file comment is:\n", mesg);
      fwrite(zcomment, 1, zcomlen, mesg);
      if (zcomment[zcomlen-1] != '\n')
        putc('\n', mesg);
      free((voidp *)zcomment);
    }
    zcomment = malloc(1);
    *zcomment = 0;
    if (noisy)
      fputs("enter new zip file comment (end with .):\n", mesg);
#if (defined(AMIGA) && (defined(LATTICE)||defined(__SASC)))
      flushall();  /* tty input/output is out of sync here */
#endif
    while (fgets(e, MAXCOM+1, comment_stream) != NULL && strcmp(e, ".\n"))
    {
      if (e[(r = strlen(e)) - 1] == '\n')
        e[--r] = 0;
      if ((p = malloc((*zcomment ? strlen(zcomment) + 3 : 1) + r)) == NULL)
      {
        free((voidp *)e);
        err(ZE_MEM, "was reading comment lines");
      }
      if (*zcomment)
        strcat(strcat(strcpy(p, zcomment), "\r\n"), e);
      else
        strcpy(p, *e ? e : "\r\n");
      free((voidp *)zcomment);
      zcomment = p;
    }
    zcomlen = strlen(zcomment);
    free((voidp *)e);
  }


  /* Write central directory and end header to temporary zip */
  diag("writing central directory");
  k = 0;                        /* keep count for end header */
  c = tempzn;                   /* get start of central */
  n = t = 0;
  for (z = zfiles; z != NULL; z = z->nxt)
  {
    if ((r = putcentral(z, y)) != ZE_OK)
      err(r, tempzip);
    tempzn += 4 + CENHEAD + z->nam + z->cext + z->com;
    n += z->len;
    t += z->siz;
    k++;
  }
  if (k == 0)
    warn("zip file empty", "");
  if (verbose)
    fprintf(mesg, "total bytes=%lu, compressed=%lu -> %d%% savings\n",
           n, t, percent(n, t));
  t = tempzn - c;               /* compute length of central */
  diag("writing end of central directory");
  if ((r = putend(k, t, c, zcomlen, zcomment, y)) != ZE_OK)
    err(r, tempzip);
  tempzf = NULL;
  if (fclose(y))
    err(d ? ZE_WRITE : ZE_TEMP, tempzip);
  if (x != NULL)
    fclose(x);

  /* Free some memory before spawning unzip */
  lm_free();

  /* Test new zip file before overwriting old one or removing input files */
  if (test) {
    check_zipfile(tempzip);
  }
  /* Replace old zip file with new zip file, leaving only the new one */
  if (strcmp(zipfile, "-") && !d)
  {
    diag("replacing old zip file with new zip file");
    if ((r = replace(zipfile, tempzip)) != ZE_OK)
    {
      warn("new zip file left as: ", tempzip);
      free((voidp *)tempzip);
      tempzip = NULL;
      err(r, "was replacing the original zip file");
    }
    free((voidp *)tempzip);
  }
  tempzip = NULL;
  if (a && strcmp(zipfile, "-")) {
    setfileattr(zipfile, a);
#ifdef VMS
    /* If the zip file existed previously, restore its record format: */
    if (x != NULL)
      VMSmunch(zipfile, RESTORE_RTYPE, NULL);
#endif
  }

  /* Finish up (process -o, -m, clean up).  Exit code depends on o. */
  leave(o ? ZE_OPEN : ZE_OK);
  return 0; /* just to avoid compiler warning */
}

/*****************************************************************
 | envargs - add default options from environment to command line
 |----------------------------------------------------------------
 | Author: Bill Davidsen, original 10/13/91, revised 23 Oct 1991.
 | This program is in the public domain.
 |----------------------------------------------------------------
 | Minor program notes:
 |  1. Yes, the indirection is a tad complex
 |  2. Parenthesis were added where not needed in some cases
 |     to make the action of the code less obscure.
 ****************************************************************/

local void
envargs(Pargc, Pargv, envstr)
int *Pargc;
char ***Pargv;
char *envstr;
{
    char *getenv();
    char *envptr;                               /* value returned by getenv */
    char *bufptr;                               /* copy of env info */
    int argc = 0;                               /* internal arg count */
    int ch;                                             /* spare temp value */
    char **argv;                                /* internal arg vector */
    char **argvect;                             /* copy of vector address */

    /* see if anything in the environment */
    envptr = getenv(envstr);
    if (envptr == NULL || *envptr == 0) return;

    /* count the args so we can allocate room for them */
    argc = count_args(envptr);
    bufptr = malloc(1+strlen(envptr));
    if (bufptr == NULL)
        err(ZE_MEM, "Can't get memory for arguments");

    strcpy(bufptr, envptr);

    /* allocate a vector large enough for all args */
    argv = (char **)malloc((argc+*Pargc+1)*sizeof(char *));
    if (argv == NULL)
        err(ZE_MEM, "Can't get memory for arguments");
    argvect = argv;

    /* copy the program name first, that's always true */
    *(argv++) = *((*Pargv)++);

    /* copy the environment args first, may be changed */
    do {
        *(argv++) = bufptr;
        /* skip the arg and any trailing blanks */
        while ((ch = *bufptr) != '\0' && ch != ' ') ++bufptr;
        if (ch == ' ') *(bufptr++) = '\0';
        while ((ch = *bufptr) != '\0' && ch == ' ') ++bufptr;
    } while (ch);

    /* now save old argc and copy in the old args */
    argc += *Pargc;
    while (--(*Pargc)) *(argv++) = *((*Pargv)++);

    /* finally, add a NULL after the last arg, like UNIX */
    *argv = NULL;

    /* save the values and return */
    *Pargv = argvect;
    *Pargc = argc;
}

static int
count_args(s)
char *s;
{
    int count = 0;
    int ch;

    do {
        /* count and skip args */
        ++count;
        while ((ch = *s) != '\0' && ch != ' ') ++s;
        while ((ch = *s) != '\0' && ch == ' ') ++s;
    } while (ch);

    return count;
}
