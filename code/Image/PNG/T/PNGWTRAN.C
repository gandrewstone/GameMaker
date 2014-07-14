
/* pngwtran.c - transforms the data in a row for png writers

   pnglib version 0.6
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995 Guy Eric Schalnat, Group 42, Inc.
   May 1, 1995
   */

#define PNG_INTERNAL
#include "png.h"

/* transform the data according to the users wishes.  The order of
   transformations is significant. */
void
png_do_write_transformations(png_struct *png_ptr)
{
   if (png_ptr->transformations & PNG_RGBA)
      png_do_write_rgbx(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_PACK)
      png_do_pack(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->bit_depth);
   if (png_ptr->transformations & PNG_SHIFT)
      png_do_shift(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->shift);
   if (png_ptr->transformations & PNG_SWAP_BYTES)
      png_do_swap(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_BGR)
      png_do_bgr(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_INVERT_MONO)
      png_do_invert(&(png_ptr->row_info), png_ptr->row_buf + 1);
}

/* pack pixels into bytes.  Pass the true bit depth in bit_depth.  The
   row_info bit depth should be 8 (one pixel per byte).  The channels
   should be 1 (this only happens on grayscale and paletted images) */
void
png_do_pack(png_row_info *row_info, png_byte *row, png_byte bit_depth)
{
   if (row_info && row && row_info->bit_depth == 8 &&
      row_info->channels == 1)
   {
      switch (bit_depth)
      {
         case 1:
         {
            png_byte *sp;
            png_byte *dp;
            int mask;
            png_int_32 i;
            int v;

            sp = row;
            dp = row;
            mask = 0x80;
            v = 0;
            for (i = 0; i < row_info->width; i++)
            {
               if (*sp)
                  v |= mask;
               sp++;
               if (mask > 1)
                  mask >>= 1;
               else
               {
                  mask = 0x80;
                  *dp = v;
                  dp++;
                  v = 0;
               }
            }
            if (mask != 0x80)
               *dp = v;
            break;
         }
         case 2:
         {
            png_byte *sp;
            png_byte *dp;
            int shift;
            png_int_32 i;
            int v;
            png_byte value;

            sp = row;
            dp = row;
            shift = 6;
            v = 0;
            for (i = 0; i < row_info->width; i++)
            {
               value = *sp & 0x3;
               v |= (value << shift);
               if (shift == 0)
               {
                  shift = 6;
                  *dp = v;
                  dp++;
                  v = 0;
               }
               else
                  shift -= 2;
               sp++;
            }
            if (shift != 6)
                   *dp = v;
            break;
         }
         case 4:
         {
            png_byte *sp;
            png_byte *dp;
            int shift;
            png_int_32 i;
            int v;
            png_byte value;

            sp = row;
            dp = row;
            shift = 4;
            v = 0;
            for (i = 0; i < row_info->width; i++)
            {
               value = *sp & 0xf;
               v |= (value << shift);

               if (shift == 0)
               {
                  shift = 4;
                  *dp = v;
                  dp++;
                  v = 0;
               }
               else
                  shift -= 4;

               sp++;
            }
            if (shift != 4)
               *dp = v;
            break;
         }
      }
      row_info->bit_depth = bit_depth;
      row_info->pixel_depth = bit_depth * row_info->channels;
      row_info->rowbytes =
         ((row_info->width * row_info->pixel_depth + 7) >> 3);
   }
}

/* shift pixel values to take advantage of whole range.  Pass the
   true number of bits in bit_depth.  The row should be packed
   according to row_info->bit_depth.  Thus, if you had a row of
   bit depth 4, but the pixels only had values from 0 to 7, you
   would pass 3 as bit_depth, and this routine would translate the
   data to 0 to 15. */
void
png_do_shift(png_row_info *row_info, png_byte *row, int bit_depth)
{
   if (row && row_info && bit_depth > 0 && bit_depth < (int)row_info->bit_depth)
   {
      int shift_start, shift_dec;

      shift_start = row_info->bit_depth - bit_depth;
      shift_dec = bit_depth;

      if (row_info->bit_depth <= 8)
      {
         png_byte *bp;
         png_uint_32 i;
         int j;

         for (bp = row, i = 0; i < row_info->rowbytes; i++, bp++)
         {
            int v;

            v = *bp;
            *bp = 0;
            for (j = shift_start; j > -shift_dec; j -= shift_dec)
            {
               if (j > 0)
                  *bp |= (png_byte)((v << j) & 0xff);
               else
                  *bp |= (png_byte)((v >> (-j)) & 0xff);
            }
         }
      }
      else
      {
         png_byte *bp;
         png_uint_32 i;
         int j;

         for (bp = row, i = 0;
            i < row_info->width * row_info->channels;
            i++, bp++)
         {
            png_uint_16 value, v;

            v = (*bp << 8) + *(bp + 1);
            value = 0;
            for (j = shift_start; j > -shift_dec; j -= shift_dec)
            {
               if (j > 0)
                  value |= (png_byte)((v << j) & 0xffffU);
               else
                  value |= (png_byte)((v >> (-j)) & 0xffffU);
            }
            *bp = value >> 8;
            *(bp + 1) = value & 0xff;
         }
      }
   }
}

/* remove filler byte after rgb */
void
png_do_write_rgbx(png_row_info *row_info, png_byte *row)
{
   if (row && row_info && row_info->color_type == PNG_COLOR_TYPE_RGB &&
      row_info->bit_depth == 8)
   {
      png_byte *sp, *dp;
      png_uint_32 i;

      for (i = 1, sp = row + 4, dp = row + 3;
         i < row_info->width;
         i++)
      {
         *dp++ = *sp++;
         *dp++ = *sp++;
         *dp++ = *sp++;
         sp++;
      }
      row_info->channels = 3;
      row_info->pixel_depth = 24;
      row_info->rowbytes = row_info->width * 3;
   }
}

