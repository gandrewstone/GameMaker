
/* pngstub.c - stub functions for i/o and memory allocation

   pnglib version 0.6
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995 Guy Eric Schalnat, Group 42, Inc.
   May 1, 1995

   This file provides a location for all input/output, memory location,
   and error handling.  Users which need special handling in these areas
   are expected to modify the code in this file to meet their needs.  See
   the instructions at each function. */

#define PNG_INTERNAL
#include "png.h"

/* Write the data to whatever output you are using.  The default
   routine writes to a file pointer.  If you need to write to something
   else, this is the place to do it.  We suggest saving the old code
   for future use, possibly in a #define.  Note that this routine sometimes
   gets called with very small lengths, so you should implement some kind
   of simple buffering if you are using unbuffered writes.  This should
   never be asked to write more then 64K on a 16 bit machine.  The cast
   to size_t is there for insurance, but If you are having problmes with
   it, you can take it out.  Just be sure to cast length to whatever
   fwrite needs in that spot if you don't have a function prototype for
   it. */
void
png_write_data(png_struct *png_ptr, png_byte *data, png_uint_32 length)
{
   png_uint_32 check;

   check = fwrite(data, 1, (size_t)length, png_ptr->fp);
   if (check != length)
   {
      png_error(png_ptr, "Write Error");
   }
}

/* Read the data from whatever input you are using.  The default
   routine reads from a file pointer.  If you need to read from something
   else, this is the place to do it.  We suggest saving the old code
   for future use.  Note that this routine sometimes gets called with
   very small lengths, so you should implement some kind of simple
   buffering if you are using unbuffered reads.  This should
   never be asked to read more then 64K on a 16 bit machine.  The cast
   to size_t is there for insurance, but if you are having problmes with
   it, you can take it out.  Just be sure to cast length to whatever
   fread needs in that spot if you don't have a function prototype for
   it. */
void
png_read_data(png_struct *png_ptr, png_byte *data, png_uint_32 length)
{
   png_uint_32 check;

   check = fread(data, 1, (size_t)length, png_ptr->fp);
   if (check != length)
   {
      png_error(png_ptr, "Read Error");
   }
}

/* Initialize the input/output for the png file.  If you change
   the read and write routines, you will probably need to change
   this routine (or write your own).  If you change the parameters
   of this routine, remember to change png.h also. */
void
png_init_io(png_struct *png_ptr, FILE *fp)
{
   png_ptr->fp = fp;
}

/* Allocate memory.  For reasonable files, size should never exceed
   64K.  However, zlib may allocate more then 64K if you don't tell
   it not to.  See zconf.h and png.h for more information. zlib does
   need to allocate exactly 64K, so whatever you call here must
   have the ability to do that. */
void *
png_large_malloc(png_struct *png_ptr, png_uint_32 size)
{
   void *ret;

#ifdef PNG_MAX_MALLOC_64K
   if (size > 65536)
      png_error(png_ptr, "Cannot Allocate > 64K");
#endif

#ifdef __TURBOC__
   ret = farmalloc(size);
#else
#  ifdef _MSC_VER
   ret = halloc(size, 1);
#  else
   ret = malloc(size);
#  endif
#endif

   if (!ret)
   {
      png_error(png_ptr, "Out of Memory");
   }

   return ret;
}

/* free a pointer allocated by png_large_malloc().  In the default
  configuration, png_ptr is not used, but is passed in case it
  is needed.  If ptr is NULL, return without taking any action. */
void
png_large_free(png_struct *png_ptr, void *ptr)
{
   if (!png_ptr)
      return;

   if (ptr != (void *)0)
   {
#ifdef __TURBOC__
      farfree(ptr);
#else
#  ifdef _MSC_VER
      hfree(ptr);
#  else
      free(ptr);
#  endif
#endif
   }
}

/* Allocate memory.  This is called for smallish blocks only  It
   should not get anywhere near 64K. */
void *
png_malloc(png_struct *png_ptr, png_uint_32 size)
{
   void *ret;

   if (!png_ptr)
      return ((void *)0);

#ifdef PNG_MAX_MALLOC_64K
   if (size > 65536)
      png_error(png_ptr, "Cannot Allocate > 64K");
#endif

   ret = malloc((png_size_t)size);

   if (!ret)
   {
      png_error(png_ptr, "Out of Memory");
   }

   return ret;
}

/* Reallocate memory.  This will not get near 64K on a
   even marginally reasonable file. */
void *
png_realloc(png_struct *png_ptr, void *ptr, png_uint_32 size)
{
   void *ret;

   if (!png_ptr)
      return ((void *)0);

#ifdef PNG_MAX_MALLOC_64K
   if (size > 65536)
      png_error(png_ptr, "Cannot Allocate > 64K");
#endif

   ret = realloc(ptr, (png_size_t)size);

   if (!ret)
   {
      png_error(png_ptr, "Out of Memory");
   }

   return ret;
}

/* free a pointer allocated by png_malloc().  In the default
  configuration, png_ptr is not used, but is passed incase it
  is needed.  If ptr is NULL, return without taking any action. */
void
png_free(png_struct *png_ptr, void *ptr)
{
   if (!png_ptr)
      return;

   if (ptr != (void *)0)
      free(ptr);
}

/* This function is called whenever there is an error.  Replace with
   however you wish to handle the error.  Note that this function
   MUST NOT return, or the program will crash */
void
png_error(png_struct *png_ptr, char *message)
{
   fprintf(stderr, "pnglib error: %s\n", message);

   longjmp(png_ptr->jmpbuf, 1);
}

/* This function is called when there is a warning, but the library
   thinks it can continue anyway.  You don't have to do anything here
   if you don't want to.  In the default configuration, png_ptr is
   not used, but it is passed in case it may be useful. */
void
png_warning(png_struct *png_ptr, char *message)
{
   if (!png_ptr)
      return;

   fprintf(stderr, "pnglib warning: %s\n", message);
}

