
/* pngrtran.c - transforms the data in a row for png readers

   pnglib version 0.6
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995 Guy Eric Schalnat, Group 42, Inc.
   May 1, 1995
   */

#define PNG_INTERNAL
#include "png.h"

/* turn on true alpha and tRNS handling.  png_read_rows will expect
   the background image to be in the required rows */
void
png_set_alpha(png_struct *png_ptr)
{
   png_ptr->transformations |= PNG_ALPHA;
}

/* handle alpha and tRNS via a background color */
void
png_set_background(png_struct *png_ptr,
   png_color_16 *background_color)
{
   png_ptr->transformations |= PNG_BACKGROUND;
   memcpy(&(png_ptr->background), background_color,
      sizeof(png_color_16));
}

/* strip 16 bit depth files to 8 bit depth */
void
png_set_strip_16(png_struct *png_ptr)
{
   png_ptr->transformations |= PNG_16_TO_8;
}

/* dither file to 8 bit.  Supply a palette, the current number
   of elements in the palette, the maximum number of elements
   allowed, and a histogram, if possible.  If the current number
   is greater then the maximum number, the palette will be
   modified to fit in the maximum number */
void
png_set_dither(png_struct *png_ptr, png_color *palette,
   int num_palette, int maximum_colors, png_uint_16 *histogram,
   int full_dither)
{
#ifdef work_in_progress
   int true_num_palette;

   png_ptr->transformations |= PNG_DITHER;

   true_num_palette = num_palette;

   if (!full_dither)
   {
      int i;

      png_ptr->dither_index = png_large_malloc(png_ptr,
         num_palette * sizeof (png_byte));
      for (i = 0; i < num_palette; i++)
         png_ptr->dither_index[i] = i;
   }

   if (num_palette > maximum_colors)
   {
      if (histogram)
      {
         /* this is easy enough, just throw out the least used colors.
            perhaps not the best solution, but good enough */
         int i;
         int min_hist;
         int min_i;

         while (num_palette > maximum_colors)
         {
            png_color tmp_color;

            min_hist = histogram[0];
            min_i = 0;
            for (i = 1; i < num_palette; i++)
            {
               if (histogram[i] < min_hist)
               {
                  min_hist = histogram[i];
                  min_i = i;
               }
            }
            num_palette--;
            tmp_color = palette[min_i];
            palette[min_i] = palette[num_palette];
            palette[num_palette] = tmp_color;
            if (!full_dither)
            {
               int j;

               for (j = 0; j < true_num_palette; j++)
               {
                  if (png_ptr->dither_index[j] == min_i)
                     png_ptr->dither_index[j] = 0xff;
                  else if (png_ptr->dither_index[j] == num_palette)
                     png_ptr->dither_index[j] = min_i;
               }
            }
         }
      }
      else
      {
         /* this is much harder to do simply (and quickly).  Perhaps
            we need to go through a median cut routine, but those
            don't always behave themselves with only a few colors
            as input.  So we will just find the closest two colors,
            and throw out one of them (chosen somewhat randomly).
            This routine is a good canidate for rewriting. */
         int i;
         int min_dist;
         int min_i;


         while (num_palette > maximum_colors)
         {
            min_dist = PNG_COLOR_DIST(palette[0], palette[1]);
            min_i = (num_palette & 1);
            for (i = 0; i < num_palette - 1; i++)
            {
               for (j = i + 1; j < num_palette; j++)
               {
                  d = PNG_COLOR_DIST(palette[i], palette[j]);
                  if (d < min_dist)
                  {
                     min_dist = d;
                     min_i = ((num_palette & 1) ? (j) : (i));
                  }
               }
            }
            num_palette--;
            tmp_color = palette[min_i];
            palette[min_i] = palette[num_palette];
            palette[num_palette] = tmp_color;
            if (!full_dither)
            {
               int j;

               for (j = 0; j < true_num_palette; j++)
               {
                  if (png_ptr->dither_index[j] == min_i)
                     png_ptr->dither_index[j] == 0xff;
                  else if (png_ptr->dither_index[j] == num_palette)
                     png_ptr->dither_index[j] = min_i;
               }
            }
         }
      }
   }
   if (!(png_ptr->palette))
      png_ptr->palette = palette;
   png_ptr->num_palette = num_palette;

   if (full_dither)
      png_build_palette_lookup(png_ptr);
   else
   {
      for (i = num_palette; i < true_num_palette; i++)
      {
         if (png_ptr->dither_index[i] >= num_palette)
         {
            for (j = 0; j < num_palette; j++)
            {
               d = PNG_COLOR_DIST(png_ptr->palette[i],
                  png_ptr->palette[j]);

               if (d < min_d)
               {
                  min_d = d;
                  min_j = j;
               }
            }
            png_ptr->dither_index[i] = j;
         }
      }
   }
#endif
}

/* transform the image from the file_gamma to the screen_gamma */
void
png_set_gamma(png_struct *png_ptr, float screen_gamma,
   float file_gamma)
{
   png_ptr->transformations |= PNG_GAMMA;
   png_ptr->gamma = file_gamma;
   png_ptr->display_gamma = screen_gamma;
}

/* expand paletted images to rgb, expand grayscale images of
   less then 8 bit depth to 8 bit depth, and expand tRNS chunks
   to alpha channels */
void
png_set_expand(png_struct *png_ptr)
{
   png_ptr->transformations |= PNG_EXPAND;
}

void
png_set_gray_to_rgb(png_struct *png_ptr)
{
   png_ptr->transformations |= PNG_GRAY_TO_RGB;
}

/* initialize everything needed for the read.  This includes modifying
   the palette */
void
png_init_read_transformations(png_struct *png_ptr)
{
   if (png_ptr->transformations & PNG_GAMMA)
      png_build_gamma_table(png_ptr);

   if (png_ptr->bit_depth < 8 && (png_ptr->transformations & PNG_EXPAND))
   {
      if (png_ptr->color_type == PNG_COLOR_TYPE_GRAY &&
         (png_ptr->transformations & (PNG_BACKGROUND || PNG_ALPHA)))
      {
         /* expand background chunk */
         png_ptr->background.gray <<= (8 / png_ptr->bit_depth);
      }
   }
}

/* transform the row.  The order of transformations is significant,
   and is very touchy.  If you add a transformation, take care to
   decide how it fits in with the other transformations here */
void
png_do_read_transformations(png_struct *png_ptr)
{
   if ((png_ptr->transformations & PNG_EXPAND) &&
      png_ptr->row_info.color_type == PNG_COLOR_TYPE_PALETTE)
   {
      png_do_expand_palette(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->palette, png_ptr->trans, png_ptr->num_trans);
   }
   else if (png_ptr->transformations & PNG_EXPAND)
   {
      if (png_ptr->num_trans)
         png_do_expand(&(png_ptr->row_info), png_ptr->row_buf + 1,
            &(png_ptr->trans_values));
      else
         png_do_expand(&(png_ptr->row_info), png_ptr->row_buf + 1,
            NULL);
   }
   if (png_ptr->transformations & PNG_BACKGROUND)
      png_do_background(&(png_ptr->row_info), png_ptr->row_buf + 1,
         &(png_ptr->trans_values), &(png_ptr->background),
         png_ptr->num_trans, png_ptr->trans,
         png_ptr->gamma_table, png_ptr->gamma_from_1,
         png_ptr->gamma_to_1, png_ptr->gamma_16_table,
         png_ptr->gamma_16_from_1, png_ptr->gamma_16_to_1,
         png_ptr->gamma_shift);
   if (png_ptr->transformations & PNG_GAMMA)
      png_do_gamma(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->gamma_table, png_ptr->gamma_16_table,
         png_ptr->gamma_shift);
   if (png_ptr->transformations & PNG_16_TO_8)
      png_do_chop(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_DITHER)
      png_do_dither(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->palette_lookup);
   if (png_ptr->transformations & PNG_INVERT_MONO)
      png_do_invert(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_SHIFT)
      png_do_unshift(&(png_ptr->row_info), png_ptr->row_buf + 1,
         png_ptr->shift);
   if (png_ptr->transformations & PNG_PACK)
      png_do_unpack(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_BGR)
      png_do_bgr(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_GRAY_TO_RGB)
      png_do_gray_to_rgb(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_SWAP_BYTES)
      png_do_swap(&(png_ptr->row_info), png_ptr->row_buf + 1);
   if (png_ptr->transformations & PNG_RGBA)
      png_do_read_rgbx(&(png_ptr->row_info), png_ptr->row_buf + 1);
}

/* unpack pixels of 1, 2, or 4 bits per pixel into 1 byte per pixel,
   without changing the actual values.  Thus, if you had a row with
   a bit depth of 1, you would end up with bytes that only contained
   the numbers 0 or 1.  If you would rather they contain 0 and 255, use
   png_do_shift() after this. */
void
png_do_unpack(png_row_info *row_info, png_byte *row)
{
   if (row && row_info && row_info->bit_depth < 8)
   {
      switch (row_info->bit_depth)
      {
         case 1:
         {
            png_byte *sp;
            png_byte *dp;
            int shift;
            png_uint_32 i;

            sp = row + (png_size_t)((row_info->width - 1) >> 3);
            dp = row + (png_size_t)row_info->width - 1;
            shift = 7 - (int)((row_info->width + 7) & 7);
            for (i = 0; i < row_info->width; i++)
            {
               *dp = (*sp >> shift) & 0x1;
               if (shift == 7)
               {
                  shift = 0;
                  sp--;
               }
               else
                  shift++;

               dp--;
            }
            break;
         }
         case 2:
         {
            png_byte *sp;
            png_byte *dp;
            int shift;
            png_uint_32 i;

            sp = row + (png_size_t)((row_info->width - 1) >> 2);
            dp = row + (png_size_t)row_info->width - 1;
            shift = (int)((3 - ((row_info->width + 3) & 3)) << 1);
            for (i = 0; i < row_info->width; i++)
            {
               *dp = (*sp >> shift) & 0x3;
               if (shift == 6)
               {
                  shift = 0;
                  sp--;
               }
               else
                  shift += 2;

               dp--;
            }
            break;
         }
         case 4:
         {
            png_byte *sp;
            png_byte *dp;
            int shift;
            png_uint_32 i;

            sp = row + (png_size_t)((row_info->width - 1) >> 2);
            dp = row + (png_size_t)row_info->width - 1;
            shift = (int)((1 - ((row_info->width + 1) & 1)) << 2);
            for (i = 0; i < row_info->width; i++)
            {
               *dp = (*sp >> shift) & 0xf;
               if (shift == 4)
               {
                  shift = 0;
                  sp--;
               }
               else
                  shift = 4;

               dp--;
            }
            break;
         }
      }
      row_info->bit_depth = 8;
      row_info->pixel_depth = 8 * row_info->channels;
      row_info->rowbytes = row_info->width * row_info->channels;
   }
}

/* reverse the effects of png_do_shift.  This routine merely shifts the
   pixels back to their significant bits values.  Thus, if you have
   a row of bit depth 8, but only 5 are significant, this will shift
   the values back to 0 through 31 */
void
png_do_unshift(png_row_info *row_info, png_byte *row, int sig_bits)
{
   if (row && row_info && sig_bits && sig_bits < (int)row_info->bit_depth)
   {
      int shift;

      shift = row_info->bit_depth - sig_bits;

      switch (row_info->bit_depth)
      {
         case 2:
         {
            png_byte *bp;
            png_uint_32 i;

            for (bp = row, i = 0;
               i < row_info->rowbytes;
               i++, bp++)
            {
               *bp >>= 1;
               *bp &= 0x55;
            }
            break;
         }
         case 4:
         {
            png_byte *bp, mask;
            png_uint_32 i;

            mask = ((0xf0 >> shift) & 0xf0) | (0xf >> shift);
            for (bp = row, i = 0;
               i < row_info->rowbytes;
               i++, bp++)
            {
               *bp >>= shift;
               *bp &= mask;
            }
            break;
         }
         case 8:
         {
            png_byte *bp;
            png_uint_32 i;

            for (bp = row, i = 0;
               i < row_info->rowbytes;
               i++, bp++)
            {
               *bp >>= shift;
            }
            break;
         }
         case 16:
         {
            png_byte *bp;
            png_uint_16 value;
            png_uint_32 i;

            for (bp = row, i = 0;
               i < row_info->width * row_info->channels;
               i++, bp++)
            {
               value = (*bp << 8) + *(bp + 1);
               value >>= shift;
               *bp = value >> 8;
               *(bp + 1) = value & 0xff;
            }
            break;
         }
      }
   }
}

/* chop rows of bit depth 16 down to 8 */
void
png_do_chop(png_row_info *row_info, png_byte *row)
{
   if (row && row_info && row_info->bit_depth == 16)
   {
      png_byte *sp, *dp;
      png_uint_32 i;

      sp = row + 2;
      dp = row + 1;
      for (i = 1; i < row_info->width * row_info->channels; i++)
      {
         *dp = *sp;
         sp += 2;
         dp++;
      }
      row_info->bit_depth = 8;
      row_info->pixel_depth = 8 * row_info->channels;
      row_info->rowbytes = row_info->width * row_info->channels;
   }
}

/* add filler byte after rgb */
void
png_do_read_rgbx(png_row_info *row_info, png_byte *row)
{
   if (row && row_info && row_info->color_type == 2 &&
      row_info->bit_depth == 8)
   {
      png_byte *sp, *dp;
      png_uint_32 i;

      for (i = 1, sp = row + (png_size_t)row_info->width * 3,
         dp = row + (png_size_t)row_info->width * 4;
         i < row_info->width;
         i++)
      {
         *(--dp) = 0xff;
         *(--dp) = *(--sp);
         *(--dp) = *(--sp);
         *(--dp) = *(--sp);
      }
      *(--dp) = 0xff;
      row_info->channels = 4;
      row_info->pixel_depth = 32;
      row_info->rowbytes = row_info->width * 4;
   }
}

/* expand grayscale files to rgb, with or without alpha */
void
png_do_gray_to_rgb(png_row_info *row_info, png_byte *row)
{
   if (row && row_info && row_info->bit_depth >= 8 &&
      !(row_info->color_type & PNG_COLOR_MASK_COLOR))
   {
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (row_info->bit_depth == 8)
         {
            png_byte *sp, *dp;
            png_uint_32 i;

            for (i = 0, sp = row + (png_size_t)row_info->width - 1,
               dp = row + (png_size_t)row_info->width * 3 - 1;
               i < row_info->width;
               i++)
            {
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *sp;
               sp--;
            }
         }
         else
         {
            png_byte *sp, *dp;
            png_uint_32 i;

            for (i = 0, sp = row + (png_size_t)row_info->width * 2 - 1,
               dp = row + (png_size_t)row_info->width * 6 - 1;
               i < row_info->width;
               i++)
            {
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               sp--;
               sp--;
            }
         }
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         if (row_info->bit_depth == 8)
         {
            png_byte *sp, *dp;
            png_uint_32 i;

            for (i = 0, sp = row + (png_size_t)row_info->width * 2 - 1,
               dp = row + (png_size_t)row_info->width * 4 - 1;
               i < row_info->width;
               i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *sp;
               sp--;
            }
         }
         else
         {
            png_byte *sp, *dp;
            png_uint_32 i;

            for (i = 0, sp = row + (png_size_t)row_info->width * 4 - 1,
               dp = row + (png_size_t)row_info->width * 8 - 1;
               i < row_info->width;
               i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               sp--;
               sp--;
            }
         }
      }
      row_info->channels += 2;
      row_info->color_type |= PNG_COLOR_MASK_COLOR;
      row_info->pixel_depth = row_info->channels * row_info->bit_depth;
      row_info->rowbytes = ((row_info->width *
         row_info->pixel_depth + 7) >> 3);
   }
}

/* build a grayscale palette.  Palette is assumed to be 1 << bit_depth
   large of png_color.  This lets grayscale images be treated as
   paletted.  Most useful for gamma correction and simplification
   of code. */
void
png_build_grayscale_palette(int bit_depth, png_color *palette)
{
   int num_palette;
   int color_inc;
   int i;
   int v;

   if (!palette)
      return;

   switch (bit_depth)
   {
      case 1:
         num_palette = 2;
         color_inc = 0xff;
         break;
      case 2:
         num_palette = 4;
         color_inc = 0x55;
         break;
      case 4:
         num_palette = 16;
         color_inc = 0x11;
         break;
      case 8:
         num_palette = 256;
         color_inc = 1;
         break;
      default:
         num_palette = 0;
         break;
   }

   for (i = 0, v = 0; i < num_palette; i++, v += color_inc)
   {
      palette[i].red = v;
      palette[i].green = v;
      palette[i].blue = v;
   }
}

void
png_correct_palette(png_struct *png_ptr, png_color *palette,
   int num_palette)
{
   if ((png_ptr->transformations & (PNG_GAMMA)) &&
      (png_ptr->transformations & (PNG_BACKGROUND)))
   {
      if (png_ptr->color_type == 3)
      {
         int i;
         png_color back, back_1;

         back.red = png_ptr->gamma_table[png_ptr->palette[
            png_ptr->background.index].red];
         back.green = png_ptr->gamma_table[png_ptr->palette[
            png_ptr->background.index].green];
         back.blue = png_ptr->gamma_table[png_ptr->palette[
            png_ptr->background.index].blue];

         back_1.red = png_ptr->gamma_to_1[png_ptr->palette[
            png_ptr->background.index].red];
         back_1.green = png_ptr->gamma_to_1[png_ptr->palette[
            png_ptr->background.index].green];
         back_1.blue = png_ptr->gamma_to_1[png_ptr->palette[
            png_ptr->background.index].blue];

         for (i = 0; i < num_palette; i++)
         {
            if (i < (int)png_ptr->num_trans &&
               png_ptr->trans[i] == 0)
            {
               palette[i] = back;
            }
            else if (i < (int)png_ptr->num_trans &&
               png_ptr->trans[i] != 0xff)
            {
               int v;

               v = png_ptr->gamma_to_1[png_ptr->palette[i].red];
               v = (int)(((png_uint_32)(v) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(back_1.red) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].red = png_ptr->gamma_from_1[v];

               v = png_ptr->gamma_to_1[png_ptr->palette[i].green];
               v = (int)(((png_uint_32)(v) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(back_1.green) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].green = png_ptr->gamma_from_1[v];

               v = png_ptr->gamma_to_1[png_ptr->palette[i].blue];
               v = (int)(((png_uint_32)(v) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(back_1.blue) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].blue = png_ptr->gamma_from_1[v];
            }
            else
            {
               palette[i].red = png_ptr->gamma_table[palette[i].red];
               palette[i].green = png_ptr->gamma_table[palette[i].green];
               palette[i].blue = png_ptr->gamma_table[palette[i].blue];
            }
         }
      }
      else
      {
         int i, back;

         back = png_ptr->gamma_table[png_ptr->background.gray];

         for (i = 0; i < num_palette; i++)
         {
            if (palette[i].red == png_ptr->trans_values.gray)
            {
               palette[i].red = back;
               palette[i].green = back;
               palette[i].blue = back;
            }
            else
            {
               palette[i].red = png_ptr->gamma_table[palette[i].red];
               palette[i].green = png_ptr->gamma_table[palette[i].green];
               palette[i].blue = png_ptr->gamma_table[palette[i].blue];
            }
         }
      }
   }
   else if (png_ptr->transformations & (PNG_GAMMA))
   {
      int i;

      for (i = 0; i < num_palette; i++)
      {
         palette[i].red = png_ptr->gamma_table[palette[i].red];
         palette[i].green = png_ptr->gamma_table[palette[i].green];
         palette[i].blue = png_ptr->gamma_table[palette[i].blue];
      }
   }
   else if (png_ptr->transformations & (PNG_BACKGROUND))
   {
      if (png_ptr->color_type == 3)
      {
         int i;
         png_byte br, bg, bb;

         br = palette[png_ptr->background.index].red;
         bg = palette[png_ptr->background.index].green;
         bb = palette[png_ptr->background.index].blue;

         for (i = 0; i < num_palette; i++)
         {
            if (i >= (int)png_ptr->num_trans ||
               png_ptr->trans[i] == 0)
            {
               palette[i].red = br;
               palette[i].green = bg;
               palette[i].blue = bb;
            }
            else if (i < (int)png_ptr->num_trans ||
               png_ptr->trans[i] != 0xff)
            {
               palette[i].red = (png_byte)((
                  (png_uint_32)(png_ptr->palette[i].red) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(br) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].green = (png_byte)((
                  (png_uint_32)(png_ptr->palette[i].green) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(bg) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
               palette[i].blue = (png_byte)((
                  (png_uint_32)(png_ptr->palette[i].blue) *
                  (png_uint_32)(png_ptr->trans[i]) +
                  (png_uint_32)(bb) *
                  (png_uint_32)(255 - png_ptr->trans[i]) +
                  127) / 255);
            }
         }
      }
      else /* assume grayscale palette (what else could it be?) */
      {
         int i;

         for (i = 0; i < num_palette; i++)
         {
            if (i == (int)png_ptr->trans_values.gray)
            {
               palette[i].red = (png_byte)png_ptr->background.gray;
               palette[i].green = (png_byte)png_ptr->background.gray;
               palette[i].blue = (png_byte)png_ptr->background.gray;
            }
         }
      }
   }
}

/* replace any alpha or transparency with the supplied background color.
   If color type is 3 (palette), use trans and trans_num,
   otherwise, use trans_values.  background is the color (in rgb or
   grey or palette index, as appropriate) */
void
png_do_background(png_row_info *row_info, png_byte *row,
   png_color_16 *trans_values, png_color_16 *background,
   int trans_num, png_byte *trans,
   png_byte *gamma_table, png_byte *gamma_from_1, png_byte *gamma_to_1,
   png_uint_16 **gamma_16, png_uint_16 **gamma_16_from_1,
   png_uint_16 **gamma_16_to_1, int png_shift)
{
#ifdef work_in_progress
   if (row && row_info && background &&
      ((row_info->color_type & PNG_COLOR_MASK_ALPHA) ||
      (row_info->color_type == PNG_COLOR_TYPE_PALETTE &&
      trans_num && trans) ||
      (row_info->color_type != PNG_COLOR_TYPE_PALETTE &&
      trans_values))
   {
      switch (row_info->color_type)
      {
         case PNG_COLOR_TYPE_GRAY:
         {
            switch (row_info->bit_depth)
            {
               case 1:
               {
                  png_byte *sp;
                  int shift;
                  png_uint_32 i;

                  sp = row;
                  shift = 7;
                  for (i = 0; i < row_info->width; i++)
                  {
                     if (((*sp >> shift) & 0x1) ==
                        trans_values->gray)
                     {
                        *sp &= ((0x7f7f >> (7 - shift)) & 0xff);
                        *sp |= (background->gray << shift);
                     }
                     if (!shift)
                     {
                        shift = 7;
                        sp++;
                     }
                     else
                        shift--;
                  }
                  break;
               }
               case 2:
               {
                  png_byte *sp;
                  int shift;
                  png_uint_32 i;

                  sp = row;
                  shift = 6;
                  for (i = 0; i < row_info->width; i++)
                  {
                     if (((*sp >> shift) & 0x3) ==
                        trans_values->gray)
                     {
                        *sp &= ((0x3f3f >> (6 - shift)) & 0xff);
                        *sp |= (background->gray << shift);
                     }
                     if (!shift)
                     {
                        shift = 6;
                        sp++;
                     }
                     else
                        shift -= 2;
                  }
                  break;
               }
               case 4:
               {
                  png_byte *sp;
                  int shift;
                  png_uint_32 i;

                  sp = row + 1;
                  shift = 4;
                  for (i = 0; i < row_info->width; i++)
                  {
                     if (((*sp >> shift) & 0xf) ==
                        trans_values->gray)
                     {
                        *sp &= ((0xf0f >> (4 - shift)) & 0xff);
                        *sp |= (background->gray << shift);
                     }
                     if (!shift)
                     {
                        shift = 4;
                        sp++;
                     }
                     else
                        shift -= 4;
                  }
                  break;
               }
               case 8:
               {
                  png_byte *sp;
                  png_uint_32 i;

                  for (i = 0, sp = row;
                     i < row_info->width; i++, sp++)
                  {
                     if (*sp == trans_values->gray)
                     {
                        *sp = background->gray;
                     }
                  }
                  break;
               }
               case 16:
               {
                  png_byte *sp;
                  png_uint_32 i;

                  for (i = 0, sp = row;
                     i < row_info->width; i++, sp += 2)
                  {
                     png_uint_16 v;

                     v = ((png_uint_16)(*sp) << 8) +
                        (png_uint_16)(*(sp + 1));
                     if (v == trans_values->gray)
                     {
                        *sp = (background->gray >> 8) & 0xff;
                        *(sp + 1) = background->gray & 0xff;
                     }
                  }
                  break;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_RGB:
         {
            if (row_info->bit_depth == 8)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++, sp += 3)
               {
                  if (*sp == trans_values->red &&
                     *(sp + 1) == trans_values->green &&
                     *(sp + 2) == trans_values->blue)
                  {
                     *sp = background->red;
                     *(sp + 1) = background->green;
                     *(sp + 2) = background->blue;
                  }
               }
            }
            else if (row_info->bit_depth == 16)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++, sp += 6)
               {
                  png_uint_16 r, g, b;

                  r = ((png_uint_16)(*sp) << 8) +
                     (png_uint_16)(*(sp + 1));
                  g = ((png_uint_16)(*(sp + 2)) << 8) +
                     (png_uint_16)(*(sp + 3));
                  b = ((png_uint_16)(*(sp + 4)) << 8) +
                     (png_uint_16)(*(sp + 5));
                  if (r == trans_values->red &&
                     g == trans_values->green &&
                     b == trans_values->blue)
                  {
                     *sp = (background->red >> 8) & 0xff;
                     *(sp + 1) = background->red & 0xff;
                     *(sp + 2) = (background->green >> 8) & 0xff;
                     *(sp + 3) = background->green & 0xff;
                     *(sp + 4) = (background->blue >> 8) & 0xff;
                     *(sp + 5) = background->blue & 0xff;
                  }
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_PALETTE:
         {
            switch (row_info->bit_depth)
            {
               case 1:
               {
                  png_byte *sp;
                  int shift;
                  png_uint_32 i;

                  sp = row;
                  shift = 7;
                  for (i = 0; i < row_info->width; i++)
                  {
                     png_byte v;

                     v = ((*sp >> shift) & 0x1);
                     if (v < trans_num &&
                        trans[v] < 0xff)
                     {
                        *sp &= ((0x7f7f >> (7 - shift)) & 0xff);
                        *sp |= (background->index << shift);
                     }
                     if (!shift)
                     {
                        shift = 7;
                        sp++;
                     }
                     else
                        shift--;
                  }
                  break;
               }
               case 2:
               {
                  png_byte *sp;
                  int shift;
                  png_uint_32 i;

                  sp = row;
                  shift = 6;
                  for (i = 0; i < row_info->width; i++)
                  {
                     png_byte v;

                     v = ((*sp >> shift) & 0x3);
                     if (v < trans_num &&
                        trans[v] < 0xff)
                     {
                        *sp &= ((0x3f3f >> (6 - shift)) & 0xff);
                        *sp |= (background->index << shift);
                     }
                     if (!shift)
                     {
                        shift = 6;
                        sp++;
                     }
                     else
                        shift -= 2;
                  }
                  break;
               }
               case 4:
               {
                  png_byte *sp;
                  int shift;
                  png_uint_32 i;

                  sp = row;
                  shift = 4;
                  for (i = 0; i < row_info->width; i++)
                  {
                     png_byte v;

                     v = ((*sp >> shift) & 0xf);
                     if (v < trans_num &&
                        trans[v] < 0xff)
                     {
                        *sp &= ((0xf0f >> (4 - shift)) & 0xff);
                        *sp |= (background->index << shift);
                     }
                     if (!shift)
                     {
                        shift = 4;
                        sp++;
                     }
                     else
                        shift -= 4;
                  }
                  break;
               }
               case 8:
               {
                  png_byte *sp;
                  png_uint_32 i;

                  for (i = 0, sp = row;
                     i < row_info->width; i++, sp++)
                  {
                     if (*sp < trans_num &&
                        trans[*sp] < 0xff)
                     {
                        *sp = background->index;
                     }
                  }
                  break;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_GRAY_ALPHA:
         {
            switch (row_info->bit_depth)
            {
               case 8:
               {
                  png_byte *sp, *dp;
                  png_uint_32 i;
                  png_byte back, back_1;

                  if (gamma_table && gamma_to_1)
                  {
                     back = gamma_table[background.gray];
                     back_1 = gamma_to_1[background.gray];
                  }
                  else
                  {
                     back = background.gray;
                     back_1 = background.gray;
                  }

                  for (i = 0, sp = row,
                     dp = row;
                     i < row_info->width; i++, sp += 2, dp++)
                  {
                     png_uint_16 a;

                     a = *(sp + 1);
                     if (a == 0xff)
                     {
                        *dp = *sp;
                     }
                     else if (a == 0)
                     {
                        *dp = back;
                     }
                     else
                     {
                        png_uint_16 v;

                        if (gamma_to_1)
                           v = gamma_to_1[*sp];
                        else
                           v = *sp;
                        v = ((png_uint_16)(v) * a +
                           (png_uint_16)back_1 *
                           (255U - a) + 127U) / 255U;
                        if (gamma_from_1)
                           *dp = gamma_from_1[v];
                        else
                           *dp = v;
                     }
                  }
                  break;
               }
               case 16:
               {
                  png_byte *sp, *dp;
                  png_uint_32 i;
                  png_uint_16 back, back_1;

                  if (gamma_16 && gamma_16_to_1)
                  {
                     back = gamma_16[background.gray];
                     back_1 = gamma_16_to_1[
                        (background.gray & 0xff) >> gamma_shift][
                        background.gray > 8];
                  }
                  else
                  {
                     back = background.gray;
                     back_1 = background.gray;
                  }

                  for (i = 0, sp = row,
                     dp = row;
                     i < row_info->width; i++, sp += 4, dp += 2)
                  {
                     png_uint_16 a;

                     a = ((png_uint_16)(*(sp + 2)) << 8) +
                        (png_uint_16)(*(sp + 3));
                     if (a == 0xffffU)
                     {
                        memcpy(dp, sp, 2);
                     }
                     else if (a == 0)
                     {
                        *dp = (back >> 8) & 0xff;
                        *(dp + 1) = back & 0xff;
                     }
                     else
                     {
                        png_uint_32 g, v;

                        if (gamma_16_to_1)
                           g = gamma_16_to_1[
                              *(sp + 1) >> gamma_shift][*sp];
                        else
                           g = ((png_uint_32)(*sp) << 8) +
                              (png_uint_32)(*(sp + 1));
                        v = ((png_uint_32)(*(sp + 2)) << 8) +
                           (png_uint_16)(*(sp + 3));
                        v = (g * (png_uint_32)a +
                           (png_uint_32)back_1 *
                           (png_uint_32)(65535U - a) +
                           32767U) / 65535U;
                        if (gamma_16_from_1)
                        {
                           v = gamma_16_from_1[
                              (v & 0xff) >> gamma_shift][v >> 8];
                        }
                        *sp = (png_byte)((v >> 8) & 0xff);
                        *(sp + 1) = (png_byte)(v & 0xff);
                     }
                  }
                  break;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_RGB_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               png_byte *sp, *dp;
               png_uint_32 i;

               for (i = 0, sp = row,
                  dp = row;
                  i < row_info->width; i++, sp += 4, dp += 3)
               {
                  png_uint_16 a;

                  a = *(sp + 3);
                  if (a == 0xff)
                  {
                     *dp = *sp;
                     *(dp + 1) = *(sp + 1);
                     *(dp + 2) = *(sp + 2);
                  }
                  else if (a == 0)
                  {
                     *dp = background->red;
                     *(dp + 1) = background->green;
                     *(dp + 2) = background->blue;
                  }
                  else
                  {
                     *dp = ((png_uint_16)(*sp) * a +
                        (png_uint_16)background->red *
                        (255U - a) + 127U) / 255U;
                     *(dp + 1) = ((png_uint_16)(*(sp + 1)) * a +
                        (png_uint_16)background->green *
                        (255U - a) + 127U) / 255U;
                     *(dp + 2) = ((png_uint_16)(*(sp + 2)) * a +
                        (png_uint_16)background->blue *
                        (255U - a) + 127U) / 255U;
                  }
               }
            }
            else if (row_info->bit_depth == 16)
            {
               png_byte *sp, *dp;
               png_uint_32 i;

               for (i = 0, sp = row,
                  dp = row;
                  i < row_info->width; i++, sp += 8, dp += 6)
               {
                  png_uint_16 a;

                  a = ((png_uint_16)(*(sp + 6)) << 8) +
                     (png_uint_16)(*(sp + 7));
                  if (a == 0xffffU)
                  {
                     memcpy(dp, sp, 6);
                  }
                  else if (a == 0)
                  {
                     *dp = (background->red >> 8) & 0xff;
                     *(dp + 1) = background->red & 0xff;
                     *(dp + 2) = (background->green >> 8) & 0xff;
                     *(dp + 3) = background->green & 0xff;
                     *(dp + 4) = (background->blue >> 8) & 0xff;
                     *(dp + 5) = background->blue & 0xff;
                  }
                  else
                  {
                     png_uint_32 r, g, b, v;

                     r = ((png_uint_32)(*sp) << 8) +
                        (png_uint_32)(*(sp + 1));
                     g = ((png_uint_32)(*(sp + 2)) << 8) +
                        (png_uint_16)(*(sp + 3));
                     b = ((png_uint_16)(*(sp + 4)) << 8) +
                        (png_uint_16)(*(sp + 5));
                     v = (r * (png_uint_32)a +
                        (png_uint_32)background->red *
                        (png_uint_32)(65535U - a) +
                        32767U) / 65535U;
                     *sp = (png_byte)((v >> 8) & 0xff);
                     *(sp + 1) = (png_byte)(v & 0xff);
                     v = (g * (png_uint_32)a +
                        (png_uint_32)background->green *
                        (png_uint_32)(65535U - a) +
                        32767U) / 65535U;
                     *(sp + 2) = (png_byte)((v >> 8) & 0xff);
                     *(sp + 3) = (png_byte)(v & 0xff);
                     v = (b * (png_uint_32)a +
                        (png_uint_32)background->blue *
                        (png_uint_32)(65535U - a) +
                        32767U) / 65535U;
                     *(sp + 4) = (png_byte)((v >> 8) & 0xff);
                     *(sp + 5) = (png_byte)(v & 0xff);
                  }
               }
            }
            break;
         }
      }
      if (row_info->color_type & PNG_COLOR_MASK_ALPHA)
      {
         row_info->color_type &= ~PNG_COLOR_MASK_ALPHA;
         row_info->channels -= 1;
         row_info->pixel_depth = row_info->channels *
            row_info->bit_depth;
         row_info->rowbytes = ((row_info->width *
            row_info->bit_depth + 7) >> 3);
      }
   }
#endif
}

/* gamma correct the image, avoiding the alpha channel.  Make sure
   you do this after you deal with the trasparency issue on grayscale
   or rgb images. If your bit depth is 8, use gamma_table, if it is 16,
   use gamma_16_table and gamma_shift.  Build these with
   build_gamma_table().  If your bit depth < 8, gamma correct a
   palette, not the data.  */
void
png_do_gamma(png_row_info *row_info, png_byte *row,
   png_byte *gamma_table, png_uint_16 **gamma_16_table,
   int gamma_shift)
{
   if (row && row_info && ((row_info->bit_depth <= 8 && gamma_table) ||
      (row_info->bit_depth == 16 && gamma_16_table)))
   {
      switch (row_info->color_type)
      {
         case PNG_COLOR_TYPE_RGB:
         {
            if (row_info->bit_depth == 8)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  png_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (v >> 8) & 0xff;
                  *(sp + 1) = v & 0xff;
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (v >> 8) & 0xff;
                  *(sp + 1) = v & 0xff;
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (v >> 8) & 0xff;
                  *(sp + 1) = v & 0xff;
                  sp += 2;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_RGB_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  sp++;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  png_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (v >> 8) & 0xff;
                  *(sp + 1) = v & 0xff;
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (v >> 8) & 0xff;
                  *(sp + 1) = v & 0xff;
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (v >> 8) & 0xff;
                  *(sp + 1) = v & 0xff;
                  sp += 4;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_GRAY_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  sp++;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  png_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (v >> 8) & 0xff;
                  *(sp + 1) = v & 0xff;
                  sp += 4;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_GRAY:
         {
            if (row_info->bit_depth == 8)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               png_byte *sp;
               png_uint_32 i;

               for (i = 0, sp = row;
                  i < row_info->width; i++)
               {
                  png_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >>
                     gamma_shift][*sp];
                  *sp = (v >> 8) & 0xff;
                  *(sp + 1) = v & 0xff;
                  sp += 2;
               }
            }
            break;
         }
      }
   }
}

/* expands a palette row to an rgb or rgba row depending
   upon whether you supply trans and num_trans */
void
png_do_expand_palette(png_row_info *row_info, png_byte *row,
   png_color *palette,
   png_byte *trans, int num_trans)
{
   if (row && row_info && row_info->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (row_info->bit_depth < 8)
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               png_byte *sp;
               png_byte *dp;
               int shift;
               png_uint_32 i;

               sp = row + (png_size_t)((row_info->width - 1) >> 3);
               dp = row + (png_size_t)row_info->width - 1;
               shift = 7 - (int)((row_info->width + 7) & 7);
               for (i = 0; i < row_info->width; i++)
               {
                  if ((*sp >> shift) & 0x1)
                     *dp = 1;
                  else
                     *dp = 0;
                  if (shift == 7)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift++;

                  dp--;
               }
               break;
            }
            case 2:
            {
               png_byte *sp;
               png_byte *dp;
               int shift, value;
               png_uint_32 i;

               sp = row + (png_size_t)((row_info->width - 1) >> 2);
               dp = row + (png_size_t)row_info->width - 1;
               shift = (int)((3 - ((row_info->width + 3) & 3)) << 1);
               for (i = 0; i < row_info->width; i++)
               {
                  value = (*sp >> shift) & 0x3;
                  *dp = value;
                  if (shift == 6)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift += 2;

                  dp--;
               }
               break;
            }
            case 4:
            {
               png_byte *sp;
               png_byte *dp;
               int shift, value;
               png_uint_32 i;

               sp = row + (png_size_t)((row_info->width - 1) >> 1);
               dp = row + (png_size_t)row_info->width - 1;
               shift = (int)((row_info->width & 1) << 2);
               for (i = 0; i < row_info->width; i++)
               {
                  value = (*sp >> shift) & 0xf;
                  *dp = value;
                  if (shift == 4)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift += 4;

                  dp--;
               }
               break;
            }
         }
         row_info->bit_depth = 8;
         row_info->pixel_depth = 8;
         row_info->rowbytes = row_info->width;
      }
      switch (row_info->bit_depth)
      {
         case 8:
         {
            if (trans)
            {
               png_byte *sp, *dp;
               png_uint_32 i;

               sp = row + (png_size_t)row_info->width;
               dp = row + (png_size_t)(row_info->width << 2) - 3;

               for (i = 0; i < row_info->width; i++)
               {
                  if (*sp >= (png_byte)num_trans)
                     *dp-- = 0xff;
                  else
                     *dp-- = trans[*sp];
                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }
               row_info->bit_depth = 8;
               row_info->pixel_depth = 32;
               row_info->rowbytes = row_info->width * 4;
               row_info->color_type = 6;
               row_info->channels = 4;
            }
            else
            {
               png_byte *sp, *dp;
               png_uint_32 i;

               sp = row + (png_size_t)row_info->width - 1;
               dp = row + (png_size_t)(row_info->width * 3) - 1;

               for (i = 0; i < row_info->width; i++)
               {
                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }
               row_info->bit_depth = 8;
               row_info->pixel_depth = 24;
               row_info->rowbytes = row_info->width * 3;
               row_info->color_type = 2;
               row_info->channels = 3;
            }
            break;
         }
      }
   }
}

/* if the bit depth < 8, it is expanded to 8.  Also, if the
   transparency value is supplied, an alpha channel is built. */
void
png_do_expand(png_row_info *row_info, png_byte *row,
   png_color_16 *trans_value)
{
   if (row && row_info)
   {
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY &&
         row_info->bit_depth < 8)
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               png_byte *sp;
               png_byte *dp;
               int shift;
               png_uint_32 i;

               sp = row + (png_size_t)((row_info->width - 1) >> 3);
               dp = row + (png_size_t)row_info->width - 1;
               shift = 7 - (int)((row_info->width + 7) & 7);
               for (i = 0; i < row_info->width; i++)
               {
                  if ((*sp >> shift) & 0x1)
                     *dp = 0xff;
                  else
                     *dp = 0;
                  if (shift == 7)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift++;

                  dp--;
               }
               break;
            }
            case 2:
            {
               png_byte *sp;
               png_byte *dp;
               int shift, value;
               png_uint_32 i;

               sp = row + (png_size_t)((row_info->width - 1) >> 2);
               dp = row + (png_size_t)row_info->width - 1;
               shift = (int)((3 - ((row_info->width + 3) & 3)) << 1);
               for (i = 0; i < row_info->width; i++)
               {
                  value = (*sp >> shift) & 0x3;
                  *dp = (value | (value << 2) | (value << 4) |
                     (value << 6));
                  if (shift == 6)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift += 2;

                  dp--;
               }
               break;
            }
            case 4:
            {
               png_byte *sp;
               png_byte *dp;
               int shift, value;
               png_uint_32 i;

               sp = row + (png_size_t)((row_info->width - 1) >> 1);
               dp = row + (png_size_t)row_info->width - 1;
               shift = (int)((1 - ((row_info->width + 1) & 1)) << 2);
               for (i = 0; i < row_info->width; i++)
               {
                  value = (*sp >> shift) & 0xf;
                  *dp = (value | (value << 4));
                  if (shift == 4)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift = 4;

                  dp--;
               }
               break;
            }
         }
         row_info->bit_depth = 8;
         row_info->pixel_depth = 8;
         row_info->rowbytes = row_info->width;
      }
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY && trans_value)
      {
         if (row_info->bit_depth == 8)
         {
            png_byte *sp, *dp;
            png_uint_32 i;

            sp = row + (png_size_t)row_info->width - 1;
            dp = row + (png_size_t)(row_info->width << 1) - 1;
            for (i = 0; i < row_info->width; i++)
            {
               if (*sp == trans_value->gray)
                  *dp-- = 0;
               else
                  *dp-- = 0xff;
               *dp-- = *sp--;
            }
         }
         else if (row_info->bit_depth == 16)
         {
            png_byte *sp, *dp;
            png_uint_32 i;

            sp = row + (png_size_t)row_info->rowbytes - 1;
            dp = row + (png_size_t)(row_info->rowbytes << 1) - 1;
            for (i = 0; i < row_info->width; i++)
            {
               if (((png_uint_16)*(sp + 1) |
                  ((png_uint_16)*sp << 8)) == trans_value->gray)
               {
                  *dp-- = 0;
                  *dp-- = 0;
               }
               else
               {
                  *dp-- = 0xff;
                  *dp-- = 0xff;
               }
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         row_info->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
         row_info->channels = 2;
         row_info->pixel_depth = (row_info->bit_depth << 1);
         row_info->rowbytes =
            ((row_info->width * row_info->pixel_depth) >> 3);
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_RGB && trans_value)
      {
         if (row_info->bit_depth == 8)
         {
            png_byte *sp, *dp;
            png_uint_32 i;

            sp = row + (png_size_t)row_info->rowbytes - 1;
            dp = row + (png_size_t)(row_info->width << 2) - 1;
            for (i = 0; i < row_info->width; i++)
            {
               if (*sp == trans_value->red &&
                  *(sp + 1) == trans_value->green &&
                  *(sp + 2) == trans_value->blue)
                  *dp-- = 0;
               else
                  *dp-- = 0xff;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         else if (row_info->bit_depth == 16)
         {
            png_byte *sp, *dp;
            png_uint_32 i;

            sp = row + (png_size_t)row_info->rowbytes - 1;
            dp = row + (png_size_t)(row_info->width << 3) - 1;
            for (i = 0; i < row_info->width; i++)
            {
               if ((((png_uint_16)*(sp + 1) |
                  ((png_uint_16)*sp << 8)) == trans_value->red) &&
                  (((png_uint_16)*(sp + 3) |
                  ((png_uint_16)*(sp + 2) << 8)) == trans_value->green) &&
                  (((png_uint_16)*(sp + 5) |
                  ((png_uint_16)*(sp + 4) << 8)) == trans_value->blue))
               {
                  *dp-- = 0;
                  *dp-- = 0;
               }
               else
               {
                  *dp-- = 0xff;
                  *dp-- = 0xff;
               }
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         row_info->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
         row_info->channels = 4;
         row_info->pixel_depth = (row_info->bit_depth << 2);
         row_info->rowbytes =
            ((row_info->width * row_info->pixel_depth) >> 3);
      }
   }
}

void
png_do_dither(png_row_info *row_info, png_byte *row,
   png_byte *palette_lookup)
{
#ifdef work_in_progress
   if (row && row_info && palette_lookup)
   {
      if (row_info->color_type == PNG_COLOR_TYPE_RGB)
      {
         int r, g, b, p;
         png_byte *sp, *dp;
         png_uint_32 i;

         sp = row;
         dp = row;
         for (i = 0; i < row_info->width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;

            p = ((r & 0xf8) << 7) + ((g & 0xf8) << 2) + ((b & 0xf8) >> 3);
            *dp++ = palette_lookup[p];
         }
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
      {
         int r, g, b, p;
         png_byte *sp, *dp;
         png_uint_32 i;

         sp = row;
         dp = row;
         for (i = 0; i < row_info->width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;
            sp++;

            p = ((r & 0xf8) << 7) + ((g & 0xf8) << 2) + ((b & 0xf8) >> 3);
            *dp++ = palette_lookup[p];
         }
      }
   }
#endif
}

/* build dither table */
void png_build_palette_lookup(png_struct *png_ptr)
{
#ifdef work_in_progress
   int i, r, g, b, ir, ig, ib;
   int total_bits, num_red, num_green, num_blue;
   png_uint_32 num_entries;

   total_bits = PNG_DITHER_RED_BITS + PNG_DITHER_GREEN_BITS +
      PNG_DITHER_BLUE_BITS;

   num_red = (1 << PNG_DITHER_RED_BITS);
   num_green = (1 << PNG_DITHER_GREEN_BITS);
   num_blue = (1 << PNG_DITHER_BLUE_BITS);
   num_entries = (1 << total_bits);

   png_ptr->palette_lookup = (png_byte *)png_large_malloc(png_ptr,
      num_entries);

   i = 0;
   for (ir = 0; ir < num_red; ir++)
   {
      r = (ir * 255) / (num_red - 1);
      for (ig = 0; ig < num_green; ig++)
      {
         g = (ig * 255) / (num_green - 1);
         for (ib = 0; ib < num_blue; ib++)
         {
            int min_d, j, k, d;

            b = (ib * 255) / (num_blue - 1);
            min_d = 1024;
            k = 0;
            for (j = 0; j < png_ptr->num_palette; j++)
            {
               d = abs(r - (int)png_ptr->palette[j].red) +
                  abs(g - (int)png_ptr->palette[j].green) +
                  abs(b - (int)png_ptr->palette[j].blue);
               if (d < min_d)
               {
                  min_d = d;
                  k = j;
               }
            }
            png_ptr->palette_lookup[i++] = k;
         }
      }
   }
#endif
}

static int png_gamma_shift[] =
   {0x10, 0x21, 0x42, 0x84, 0x110, 0x248, 0x550, 0xff0};

void
png_build_gamma_table(png_struct *png_ptr)
{
   if (png_ptr->bit_depth <= 8)
   {
      int i;
      double g;

      g = 1.0 / (png_ptr->gamma * png_ptr->display_gamma);

      png_ptr->gamma_table = (png_byte *)png_malloc(png_ptr,
         (png_uint_32)256);

      for (i = 0; i < 256; i++)
      {
         png_ptr->gamma_table[i] = (png_byte)(pow((double)i / 255.0,
            g) * 255.0 + .5);
      }

      if (png_ptr->transformations & (PNG_BACKGROUND || PNG_ALPHA))
      {
         g = 1.0 / (png_ptr->gamma);

         png_ptr->gamma_to_1 = (png_byte *)png_malloc(png_ptr,
            (png_uint_32)256);

         for (i = 0; i < 256; i++)
         {
            png_ptr->gamma_to_1[i] = (png_byte)(pow((double)i / 255.0,
               g) * 255.0 + .5);
         }

         g = 1.0 / (png_ptr->display_gamma);

         png_ptr->gamma_from_1 = (png_byte *)png_malloc(png_ptr,
            (png_uint_32)256);

         for (i = 0; i < 256; i++)
         {
            png_ptr->gamma_from_1[i] = (png_byte)(pow((double)i / 255.0,
               g) * 255.0 + .5);
         }
      }
   }
   else
   {
      double g;
      int i, j, shift, num;
      int sig_bit;
      png_uint_32 ig;

      sig_bit = 0;
      if (png_ptr->color_type & PNG_COLOR_MASK_COLOR)
      {
         sig_bit = (int)png_ptr->sig_bit.red;
         if ((int)png_ptr->sig_bit.green > sig_bit)
            sig_bit = png_ptr->sig_bit.green;
         if ((int)png_ptr->sig_bit.blue > sig_bit)
            sig_bit = png_ptr->sig_bit.blue;
      }
      else
      {
         sig_bit = (int)png_ptr->sig_bit.gray;
      }

      if (sig_bit > 0)
         shift = 16 - sig_bit;
      else
         shift = 0;

      if (shift > 8)
         shift = 8;

      png_ptr->gamma_shift = shift;

      num = (1 << (8 - shift));

      g = 1.0 / (png_ptr->gamma * png_ptr->display_gamma);

      png_ptr->gamma_16_table = (png_uint_16 **)png_malloc(png_ptr,
         num * sizeof (png_uint_16 *));

      for (i = 0; i < num; i++)
      {
         png_ptr->gamma_16_table[i] = (png_uint_16 *)png_malloc(png_ptr,
            256 * sizeof (png_uint_16));

         ig = (((png_uint_32)i *
            (png_uint_32)png_gamma_shift[shift]) >> 4);
         for (j = 0; j < 256; j++)
         {
            png_ptr->gamma_16_table[i][j] =
               (png_uint_16)(pow((double)(ig + ((png_uint_32)j << 8)) /
                  65535.0, g) * 65535.0 + .5);
         }
      }
      if (png_ptr->transformations & (PNG_BACKGROUND || PNG_ALPHA))
      {
         g = 1.0 / (png_ptr->gamma);

         png_ptr->gamma_16_to_1 = (png_uint_16 **)png_malloc(png_ptr,
            num * sizeof (png_uint_16 *));

         for (i = 0; i < num; i++)
         {
            png_ptr->gamma_16_to_1[i] = (png_uint_16 *)png_malloc(png_ptr,
               256 * sizeof (png_uint_16));

            ig = (((png_uint_32)i *
               (png_uint_32)png_gamma_shift[shift]) >> 4);
            for (j = 0; j < 256; j++)
            {
               png_ptr->gamma_16_to_1[i][j] =
                  (png_uint_16)(pow((double)(ig + ((png_uint_32)j << 8)) /
                     65535.0, g) * 65535.0 + .5);
            }
         }
         g = 1.0 / (png_ptr->display_gamma);

         png_ptr->gamma_16_from_1 = (png_uint_16 **)png_malloc(png_ptr,
            num * sizeof (png_uint_16 *));

         for (i = 0; i < num; i++)
         {
            png_ptr->gamma_16_from_1[i] = (png_uint_16 *)png_malloc(png_ptr,
               256 * sizeof (png_uint_16));

            ig = (((png_uint_32)i *
               (png_uint_32)png_gamma_shift[shift]) >> 4);
            for (j = 0; j < 256; j++)
            {
               png_ptr->gamma_16_from_1[i][j] =
                  (png_uint_16)(pow((double)(ig + ((png_uint_32)j << 8)) /
                     65535.0, g) * 65535.0 + .5);
            }
         }
      }
   }
}
