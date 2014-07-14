/* example.c - an example of using pnglib */

/* this is an example of how to use pnglib to read and write
   png files.  The file pnglib.txt is much more verbose then
   this.  If you have not read it, do so first.  This was
   designed to be a starting point of an implementation.
   This is not officially part of pnglib, and therefore
   does not require a copyright notice.
   */

#include "png.h"

/* read a png file */
void read_png(char *file_name)
{
   FILE *fp;
   png_struct *png_ptr;
   png_info *info_ptr;

   /* open the file */
   fp = fopen(file_name, "rb");
   if (!fp) return;

   /* allocate the necessary structures */
   png_ptr = malloc(sizeof (png_struct));
   info_ptr = malloc(sizeof (png_info));

   /* set error handling */
   if (setjmp(png_ptr->jmpbuf))
   {
      /* If we get here, we had a problem reading the file */
      return;
   }

   /* initialize the structures */
   png_read_init(png_ptr);
   png_info_init(info_ptr);

   /* set up the input control */
   png_init_io(png_ptr, fp);

   /* read the file information */
   png_read_info(png_ptr, info_ptr);

   /* allocate the memory to hold the image using the fields
      of png_info. */

   /* set up the transformations you want.  Note that these are
      all optional.  Only call them if you want them */

   /* expand paletted colors into true rgb */
   if (png_info->color_type == PNG_COLOR_TYPE_PALETTE &&
      png_info->bit_depth < 8)
      png_set_expand(png_ptr);

   /* expand grayscale images to the full 8 bits */
   if (png_info->color_type == PNG_COLOR_TYPE_GRAY &&
      png_info->bit_depth < 8)
      png_set_expand(png_ptr);

   /* expand images with transparency to full alpha channels */
   if (png_info->valid & PNG_INFO_tRNS)
      png_set_expand(png_ptr);

   /* Set the background color to draw transparent and alpha
      images over */

   if (png_info->valid & PNG_INFO_bKGD)
      png_set_backgrond(png_ptr, png_info->background);
   else
   {
      png_uint_16 my_backgound[3];
      png_set_background(png_ptr, &my_background);
   }

   /* tell pnglib to handle the gamma conversion for you */
   if (png_info->valid & PNG_INFO_gAMA)
      png_set_gamma(png_ptr, screen_gamma, png_info->gamma);
   else
      png_set_gamma(png_ptr, screen_gamma, 0.45);

   /* tell pnglib to strip 16 bit depth files down to 8 bits */
   if (png_info->bit_depth == 16)
      png_set_strip_16(png_ptr);

   /* dither rgb files down to 8 bit palettes &*/
   if (png_info->color_type == PNG_COLOR_TYPE_RGB ||
      png_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
   {
      if (png_info->valid & PNG_INFO_PLTE)
         png_set_dither(png_ptr, png_info->palette,
            png_info->num_palette, 256, png_info->histogram);
      else
      {
         png_color std_color_cube[256] = { ... colors ... };

         png_set_dither(png_ptr, std_color_cube, 256, 256, NULL);
      }
   }

   /* invert monocrome files */
   if (png_info->bit_depth == 1 &&
      png_info->color_type == PNG_COLOR_GRAY)
      png_set_invert(png_ptr);

   /* shift the pixels down to their true bit depth */
   if (png_info->valid & PNG_INFO_sBIT &&
      png_info->bit_depth > png_info->sig_bit)
      png_set_shift(png_ptr, png_info->sig_bit);

   /* pack pixels into bytes */
   if (png_info->bit_depth < 8)
      png_set_packing(png_ptr);

   /* flip the rgb pixels to bgr */
   if (png_info->color_type == PNG_COLOR_TYPE_RGB ||
      png_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
      png_set_bgr(png_ptr);

   /* swap bytes of 16 bit files to least significant bit first */
   if (png_info->bit_depth == 16)
      png_set_swap(png_ptr);

   /* add a filler byte to store rgb files as rgbx */
   if (png_info->bit_depth == 8 &&
      png_info->color_type == PNG_COLOR_TYPE_RGB)
      png_set_rgbx(png_ptr);

   /* optional call to update palette with transformations */
   void png_start_read_image(png_ptr);

   /* the easiest way to read the image */
   void *row_pointers[height];
   png_read_image(png_ptr, row_pointers);

   /* the other way to read images - deal with interlacing */

   /* turn on interlace handling */
   if (png_info->interlace_type)
      number_passes = png_set_interlace_handling(png_ptr);
   else
      number_passes = 1;

   for (pass = 0; pass < number_passes; pass++)
   {
      /* Read the image using the "sparkle" effect. */
      png_read_rows(png_ptr, row_pointers, NULL, number_of_rows);

      /* If you are only reading on row at a time, this works */
      for (y = 0; y < height; y++)
      {
         char *row_pointers = row[y];
         png_read_rows(png_ptr, &row_pointers, NULL, 1);
      }

      /* to get the rectangle effect, use the third parameter */
      png_read_rows(png_ptr, NULL, row_pointers, number_of_rows);

      /* if you have called png_set_alpha(), and you want the
         rectangle effect, you will need to pass two sets of
         rows. */
      png_read_rows(png_ptr, row_pointers, display_pointers,
         number_of_rows);

      /* if you want to display the image after every pass, do
         so here */
   }

   /* read the rest of the file, getting any additional chunks
      in png_info */
   png_read_end(png_ptr, png_info);

   /* clean up after the read, and free any memory allocated */
   png_read_distroy(png_ptr, png_info);

   /* free the structures */
   free(png_ptr);
   free(png_info);

   /* close the file */
   fclose(fp);

   /* that's it */
   return;
}

/* write a png file */
void write_png(char *file_name, ... other image information ...)
{
   FILE *fp;
   png_struct *png_ptr;
   png_info *info_ptr;

   /* open the file */
   fp = fopen(file_name, "wb");
   if (!fp)
      return;

   /* allocate the necessary structures */
   png_ptr = malloc(sizeof (png_struct));
   info_ptr = malloc(sizeof (png_info));

   /* set error handling */
   if (setjmp(png_ptr->jmpbuf))
   {
      /* If we get here, we had a problem reading the file */
      return;
   }

   /* initialize the structures */
   png_write_init(png_ptr);
   png_info_init(info_ptr);

   /* set up the output control */
   png_init_io(png_ptr, fp);

   /* set the file information here */
   info_ptr->width = ;
   info_ptr->height = ;
   etc.

   /* set the palette if there is one */
   png_info->valid |= PNG_INFO_PLTE;
   png_info->palette = malloc(256 * sizeof (png_color));
   png_info->num_palette = 256;
   ... set palette colors ...

   /* optional significant bit chunk */
   png_info->valid |= PNG_INFO_sBIT;
   png_info->sig_bit = true_bit_depth;

   /* optional gamma chunk */
   png_info->valid |= PNG_INFO_gAMA;
   png_info->gamma = gamma;

   /* write the file information */
   png_write_info(png_ptr, info_ptr);

   /* set up the transformations you want.  Note that these are
      all optional.  Only call them if you want them */

   /* invert monocrome pixels */
   png_set_invert(png_ptr);

   /* shift the pixels up to a legal bit depth and fill in
      as appropriate to correctly scale the image */
   png_set_shift(png_ptr, png_info->sig_bit);

   /* pack pixels into bytes */
   png_set_packing(png_ptr);

   /* flip bgr pixels to rgb */
   png_set_bgr(png_ptr);

   /* swap bytes of 16 bit files to most significant bit first */
   png_set_swap(png_ptr);

   /* get rid of filler bytes, pack rgb into 3 bytes */
   png_set_rgbx(png_ptr);

   /* the easiest way to write the image */
   void *row_pointers[height];
   png_write_image(png_ptr, row_pointers);

   /* the other way to write the image - deal with interlacing */

   /* turn on interlace handling */
   if (interlacing)
      number_passes = png_set_interlace_handling(png_ptr);
   else
      number_passes = 1;

   for (pass = 0; pass < number_passes; pass++)
   {
      /* Write a few rows at a time. */
      png_write_rows(png_ptr, row_pointers, number_of_rows);

      /* If you are only writing one row at a time, this works */
      for (y = 0; y < height; y++)
      {
         char *row_pointers = row[y];
         png_write_rows(png_ptr, &row_pointers, 1);
      }
   }

   /* write the rest of the file */
   png_write_end(png_ptr, png_info);

   /* clean up after the write, and free any memory allocated */
   png_write_distroy(png_ptr);

   /* if you malloced the palette, free it here */
   if (png_ptr->palette)
      free(png_ptr->palette);

   /* free the structures */
   free(png_ptr);
   free(png_info);

   /* close the file */
   fclose(fp);

   /* that's it */
   return;
}

