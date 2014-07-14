/*

 Copyright (C) 1990-1993 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  zipup.c by Mark Adler and Jean-loup Gailly.
 */

#define NOCPYRT         /* this is not a main module */
#include <ctype.h>
#include "zip.h"
#include "revision.h"
#include "crypt.h"
#ifdef OS2
#  include "os2zip.h"
#endif
#if defined(MMAP)
#  include <sys/mman.h>
#  ifdef SYSV
#    include <sys/param.h>
#  else
#    define PAGESIZE getpagesize()
#  endif
#endif


/* Use the raw functions for MSDOS and Unix to save on buffer space.
   They're not used for VMS since it doesn't work (raw is weird on VMS).
   (This sort of stuff belongs in fileio.c, but oh well.) */
#ifdef VMS
#  define fhow "r"
#  define fbad NULL
   typedef void *ftype;
#  define zopen(n,p)   (vms_native?vms_open(n)    :(ftype)fopen((n),(p)))
#  define zread(f,b,n) (vms_native?vms_read(f,b,n):fread((b),1,(n),(FILE*)(f)))
#  define zclose(f)    (vms_native?vms_close(f)   :fclose((FILE*)(f)))
#  define zerr(f)      (vms_native?vms_error(f)   :ferror((FILE*)(f)))
#  define zstdin stdin
   ftype vms_open OF((char *));
   int vms_read OF((ftype, char *, int));
   int vms_close OF((ftype));
   int vms_error OF((ftype));
#else /* !VMS */
#  if (defined(MSDOS) && !defined(ATARI_ST)) || defined(__human68k__)
#    include <io.h>
#    include <fcntl.h>
#    define fhow (O_RDONLY|O_BINARY)
#  else /* !MSDOS */
#    if defined(SYSV) || defined(__386BSD__)
#      include <fcntl.h>  /* open(), read(), close(), lseek() */
#    endif
#    ifdef TOPS20
#      define O_RDONLY (0)
#      define O_UNCONVERTED     (0400) /* Forced NO conversion requested */
#      define fhow (O_RDONLY | O_UNCONVERTED)
#    else
#      ifdef AMIGA
#        include <fcntl.h>
#        define fhow (O_RDONLY | O_RAW)
#        define fbad (-1)
#        ifdef AZTEC_C
#          define O_RAW 0
#        endif
#      else
#        define fhow 0
#      endif
#    endif
#  endif /* ?MSDOS */
   typedef int ftype;
#  ifndef fbad
#    define fbad (-1)
#  endif
#  define zopen(n,p) open(n,p)
#  define zread(f,b,n) read(f,b,n)
#  define zclose(f) close(f)
#  define zerr(f) (k==(extent)(-1L))
#  define zstdin 0
#endif /* ?VMS */

#ifndef UTIL

#if defined(MMAP) || defined(BIG_MEM)
  extern uch * window;          /* Used to read all input file at once */
#endif
extern ulg window_size;         /* size of said window */

/* Local data */

  local ulg crc;       /* crc on uncompressed file data */
  local ftype ifile;   /* file to compress */
  local long remain;
  /* window bytes not yet processed. >= 0 only for BIG_MEM */
#endif
ulg isize;           /* input file size. global only for debugging */    

/* Local functions */
#if defined(PROTO) && !defined(UTIL)
   local int suffixes(char *, char *);
#endif


/* Note: a zip "entry" includes a local header (which includes the file
   name), an encryption header if encrypting, the compressed data
   and possibly an extended local header. */

int zipcopy(z, x, y)
struct zlist far *z;    /* zip entry to copy */
FILE *x, *y;            /* source and destination files */
/* Copy the zip entry described by *z from file *x to file *y.  Return an
   error code in the ZE_ class.  Also update tempzn by the number of bytes
   copied. */
{
  ulg n;                /* holds local header offset */

  Trace((stderr, "zipcopy %s\n", z->zname));
  n = 4 + LOCHEAD + (long)z->nam + (long)z->ext;

  if (fix > 1) {
    /* do not trust the old compressed size */
    if (putlocal(z, y) != ZE_OK)
      return ZE_TEMP;

    if (fseek(x, z->off + n, SEEK_SET)) /* seek to compressed data */
      return ferror(x) ? ZE_READ : ZE_EOF;

    z->off = tempzn;
    tempzn += n;
    n = z->siz;
  } else {
    if (fseek(x, z->off, SEEK_SET))     /* seek to local header */
      return ferror(x) ? ZE_READ : ZE_EOF;

    z->off = tempzn;
    n += z->siz;
  }
  /* copy the compressed data and the extended local header if there is one */
  if (z->lflg & 8) n += 16;
  tempzn += n;
  return fcopy(x, y, n);
}


#ifndef UTIL

int percent(n, m)
ulg n;
ulg m;               /* n is the original size, m is the new size */
/* Return the percentage compression from n to m using only integer
   operations */
{
  if (n > 0xffffffL)            /* If n >= 16M */
  {                             /*  then divide n and m by 256 */
    n += 0x80;  n >>= 8;
    m += 0x80;  m >>= 8;
  }
  return n > m ? (int)(1 + (200 * (n - m)/n)) / 2 : 0;
}

local int suffixes(a, s)
char *a;                /* name to check suffix of */
char *s;                /* list of suffixes separated by : or ; */
/* Return true if a ends in any of the suffixes in the list s. */
{
  int m;                /* true if suffix matches so far */
  char *p;              /* pointer into special */
  char *q;              /* pointer into name a */

  m = 1;
#ifdef VMS
  if( (q = strrchr(a,';')) != NULL )    /* Cut out VMS file version */
    --q;
  else
    q = a + strlen(a) - 1;
#else
  q = a + strlen(a) - 1;
#endif
  for (p = s + strlen(s) - 1; p >= s; p--)
    if (*p == ':' || *p == ';')
      if (m)
        return 1;
      else
      {
        m = 1;
#ifdef VMS
        if( (q = strrchr(a,';')) != NULL )      /* Cut out VMS file version */
          --q;
        else
          q = a + strlen(a) - 1;
#else
        q = a + strlen(a) - 1;
#endif
      }
    else
    {
      m = m && q >= a && case_map(*p) == case_map(*q);
      q--;
    }
  return m;
}

int zipup(z, y)
struct zlist far *z;    /* zip entry to compress */
FILE *y;                /* output file */
/* Compress the file z->name into the zip entry described by *z and write
   it to the file *y. Encrypt if requested.  Return an error code in the
   ZE_ class.  Also, update tempzn by the number of bytes written. */
{
  ulg a = 0L;           /* attributes returned by filetime() */
  char *b;              /* malloc'ed file buffer */
  extent k = 0;         /* result of zread */
  int l = 0;            /* true if this file is a symbolic link */
  int m;                /* method for this entry */
  ulg o, p;             /* offsets in zip file */
  long q = -3L;         /* size returned by filetime */
  int r;                /* temporary variable */
  ulg s = 0L;           /* size of compressed data */
  int isdir;            /* set for a directory name */
  int set_type = 0;     /* set if file type (ascii/binary) unknown */

  z->nam = strlen(z->zname);
  isdir = z->zname[z->nam-1] == '/';
  z->att = (ush)UNKNOWN; /* will be changed later */

  if ((z->tim = filetime(z->name, &a, &q)) == 0 || q < -2L)
    return ZE_OPEN;
  /* q is set to -1 if the input file is a device, -2 for a volume label */
  if (q == -2L) {
     isdir = 1;
     q = 0;
  }
  remain = -1L; /* changed only for BIG_MEM */
  window_size = 0L;

  /* Select method based on the suffix and the global method */
  m = special != NULL && suffixes(z->name, special) ? STORE : method;

  /* Open file to zip up unless it is stdin */
  if (strcmp(z->name, "-") == 0)
  {
    ifile = (ftype)zstdin;
#if defined(MSDOS) || defined(__human68k__)
    setmode(zstdin, O_BINARY);
#endif
  }
  else
  {
    set_extra_field(z); /* create extra field and change z->att if desired */
    l = issymlnk(a);
    if (l)
      ifile = fbad;
    else if (isdir) { /* directory */
      ifile = fbad;
      m = STORE;
      q = 0;
    }
    else if ((ifile = zopen(z->name, fhow)) == fbad)
      return ZE_OPEN;

#ifdef MMAP
    /* Map ordinary files but not devices. This code should go in fileio.c */
    if (q > 0 && !translate_eol) {
      if (window != NULL)
        free(window);  /* window can't be a mapped file here */
      window_size = q + MIN_LOOKAHEAD;
      window = (uch*)mmap(0, window_size, PROT_READ, MAP_SHARED, ifile, 0);
      if (window == (uch*)(-1)) {
        window = NULL;
        return ZE_OPEN;
      }
      /* If we can't touch the page beyond the end of file, remap
       * using a garbage page:
       */
      remain = window_size & (PAGESIZE-1);
      if (remain > 0 && remain <= MIN_LOOKAHEAD &&
        mmap(window+window_size-remain, MIN_LOOKAHEAD, PROT_READ,
             MAP_SHARED | MAP_FIXED, ifile, 0) == (char*)(-1)) {
        /* The input file has at least MIN_LOOKAHEAD bytes, otherwise
         * we would have enough space in the first page (assuming a page
         * size of at least 512 bytes)
         */
        fprintf(mesg, " cannot extend mmap on %s", z->name);
        window = NULL;
        return ZE_OPEN;
      }
      remain = q;
    }
#else
# ifdef BIG_MEM
    /* Read the whole input file at once */
    if (q > 0 && !translate_eol) {
      window_size = q + MIN_LOOKAHEAD;
      window = window ? (uch*) realloc(window, (unsigned)window_size)
                      : (uch*) malloc((unsigned)window_size);
      /* Just use normal code if big malloc or realloc fails: */
      if (window != NULL) {
        remain = zread(ifile, (char*)window, q+1);
        if (remain != q) {
          fprintf(mesg, " q=%ld, remain=%ld ", q, remain);
          error("can't read whole file at once");
        }
      } else {
        window_size = 0L;
      }
    }
# endif /* BIG_MEM */
#endif /* MMAP */

  } /* strcmp(z->name, "-") == 0 */

  if (l || q == 0)
    m = STORE;
  if (m == BEST)
    m = DEFLATE;

  /* Do not create STORED files with extended local headers if the
   * input size is not known, because such files could not be extracted.
   * So if the zip file is not seekable and the input file is not
   * on disk, obey the -0 option by forcing deflation with stored block.
   * Note however that using "zip -0" as filter is not very useful...
   * ??? to be done.
   */

  /* Fill in header information and write local header to zip file.
   * This header will later be re-written since compressed length and
   * crc are not yet known.
   */

  /* (Assume ext, cext, com, and zname already filled in.) */
#if defined(OS2) || defined(WIN32)
  z->vem = z->dosflag ? 20       /* Made under MSDOS by PKZIP 2.0 */
         : OS_CODE + REVISION;
  /* For a FAT file system, we cheat and pretend that the file
   * was not made on OS2 but under DOS. unzip is confused otherwise.
   */
#else
  z->vem = dosify ? 20 : OS_CODE + REVISION;
#endif

  z->ver = m == STORE ? 10 : 20;     /* Need PKUNZIP 2.0 except for store */
  z->crc = 0;  /* to be updated later */
  /* Assume first that we will need an extended local header: */
  z->flg = 8;  /* to be updated later */
#ifdef CRYPT
  if (key != NULL) {
    z->flg |= 1;
    /* Since we do not yet know the crc here, we pretend that the crc
     * is the modification time:
     */
    z->crc = z->tim << 16;
  }
#endif
  z->lflg = z->flg;
  z->how = m;                             /* may be changed later  */
  z->siz = m == STORE && q >= 0 ? q : 0;  /* will be changed later  */
  z->len = q >= 0 ? q : 0;                /* may be changed later  */
  z->dsk = 0;
  if (z->att == (ush)UNKNOWN) {
      z->att = BINARY;                    /* set sensible value in header */
      set_type = 1;
  }
  z->atx = z->dosflag ? a & 0xff : a;     /* Attributes from filetime() */
  z->off = tempzn;
  if ((r = putlocal(z, y)) != ZE_OK)
    return r;
  tempzn += 4 + LOCHEAD + z->nam + z->ext;

#ifdef CRYPT
  if (key != NULL) {
    crypthead(key, z->crc, y);
    z->siz += RAND_HEAD_LEN;  /* to be updated later */
    tempzn += RAND_HEAD_LEN;
  }
#endif
  if (ferror(y))
    err(ZE_WRITE, "unexpected error on zip file");
  o = ftell(y); /* for debugging only, ftell can fail on pipes */
  if (ferror(y))
    clearerr(y); 

  /* Write stored or deflated file to zip file */
  isize = 0L;
  crc = updcrc((char *)NULL, 0);

  if (m == DEFLATE) {
     bi_init(y);
     if (set_type) z->att = (ush)UNKNOWN; /* will be changed in deflate() */
     ct_init(&z->att, &m);
     lm_init(level, &z->flg);
     s = deflate();
  }
  else
  {
    if ((b = malloc(CBSZ)) == NULL)
       return ZE_MEM;

    if (!isdir) /* no read for directories */
    while ((k = l ? rdsymlnk(z->name, b, CBSZ) : file_read(b, CBSZ)) > 0)
    {
      if (zfwrite(b, 1, k, y) != k)
      {
        free((voidp *)b);
        return ZE_TEMP;
      }
      if (verbose) putc('.', stderr);
#ifdef MINIX
      if (l)
        q = k;
#endif /* MINIX */
      if (l)
        break;
    }
    free((voidp *)b);
    s = isize;
  }
  if (ifile != fbad && zerr(ifile))
    return ZE_READ;
  if (ifile != fbad)
    zclose(ifile);
#ifdef MMAP
  if (remain >= 0L) {
    munmap(window, window_size);
    window = NULL;
  }
#endif

  tempzn += s;
  p = tempzn; /* save for future fseek() */

#if (!defined(MSDOS) || defined(OS2)) && !defined(VMS)
  /* Check input size (but not in VMS -- variable record lengths mess it up)
   * and not on MSDOS -- diet in TSR mode reports an incorrect file size)
   */
  if (q >= 0 && isize != (ulg)q && !translate_eol)
  {
    Trace((mesg, " i=%ld, q=%ld ", isize, q));
    warn(" file size changed while zipping ", z->name);
  }
#endif

  /* Try to rewrite the local header with correct information */
  z->crc = crc;
  z->siz = s;
#ifdef CRYPT
  if (key != NULL)
    z->siz += RAND_HEAD_LEN;
#endif
  z->len = isize;
  if (fseek(y, z->off, SEEK_SET)) {
    if (z->how != (ush) m)
       error("can't rewrite method");
    if (m == STORE && q < 0)
       err(ZE_PARMS, "zip -0 not supported for I/O on pipes or devices");
    if ((r = putextended(z, y)) != ZE_OK)
      return r;
    tempzn += 16L;
    z->flg = z->lflg; /* if flg modified by inflate */
  } else {
     /* seek ok, ftell() should work, check compressed size */
#ifndef VMS
    if (p - o != s) {
      fprintf(mesg, " s=%ld, actual=%ld ", s, p-o);
      error("incorrect compressed size");
    }
#endif
    z->how = m;
    z->ver = m == STORE ? 10 : 20;     /* Need PKUNZIP 2.0 except for store */
    if ((z->flg & 1) == 0)
      z->flg &= ~8; /* clear the extended local header flag */
    z->lflg = z->flg;
    /* rewrite the local header: */
    if ((r = putlocal(z, y)) != ZE_OK)
      return r;
    if (fseek(y, p, SEEK_SET))
      return ZE_READ;
    if ((z->flg & 1) != 0) {
      /* encrypted file, extended header still required */
      if ((r = putextended(z, y)) != ZE_OK)
        return r;
      tempzn += 16L;
    }
  }
  /* Free the local extra field which is no longer needed */
  if (z->ext) {
    if (z->extra != z->cextra)
      free((voidp *)(z->extra));
    z->ext = 0;
  }

  /* Display statistics */
  if (noisy)
  {
    if (verbose)
      fprintf(mesg, "\t(in=%lu) (out=%lu)", isize, s);
    if (m == DEFLATE)
      fprintf(mesg, " (deflated %d%%)\n", percent(isize, s));
    else
      fprintf(mesg, " (stored 0%%)\n");
    fflush(mesg);
  }
  return ZE_OK;
}


int file_read(buf, size)
  char *buf;
  unsigned size;
/* Read a new buffer from the current input file, perform end-of-line
 * translation, and update the crc and input file size.
 * IN assertion: size >= 2 (for end-of-line translation)
 */
{
  unsigned len;
  char far *b;

#if defined(MMAP) || defined(BIG_MEM)
  if (remain == 0L) {
    return 0;
  } else if (remain > 0L) {
    /* The window data is already in place. We still compute the crc
     * by 32K blocks instead of once on whole file to keep a certain
     * locality of reference.
     */
    Assert (buf == (char*)window + isize, "are you lost?");
    if (size > remain) size = remain;
    if (size > WSIZE) size = WSIZE; /* don't touch all pages at once */
    remain -= (long) size;
    len = size;
  } else
#endif
  if (translate_eol == 0) {
    len = zread(ifile, buf, size);
    if (len == (unsigned)EOF || len == 0) return (int)len;

  } else if (translate_eol == 1) {
    /* Transform LF to CR LF */
    size >>= 1;
    b = buf+size;
    size = len = zread(ifile, b, size);
    if (len == (unsigned)EOF || len == 0) return (int)len;
    do {
       if ((*buf++ = *b++) == '\n') *(buf-1) = '\r', *buf++ = '\n', len++;
    } while (--size != 0);
    buf -= len;

  } else {
    /* Transform CR LF to LF and suppress final ^Z */
    b = buf;
    size = len = zread(ifile, buf, size-1);
    if (len == (unsigned)EOF || len == 0) return (int)len;
    buf[len] = '\n'; /* I should check if next char is really a \n */
    do {
       if ((*buf++ = *b++) == '\r' && *b == '\n') buf--, len--;
    } while (--size != 0);
    if (len == 0) {
       zread(ifile, buf, 1); len = 1; /* keep single \r if EOF */
    } else {
       buf -= len;
       if (buf[len-1] == ('Z' & 0x1f)) len--; /* suppress final ^Z */
    }
  }
  crc = updcrc(buf, len);
  isize += (ulg)len;
  return (int)len;
}
#endif /* !UTIL */
