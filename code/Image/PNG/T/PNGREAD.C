
/* pngread.c - read a png file

   pnglib version 0.6
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995 Guy Eric Schalnat, Group 42, Inc.
   May 1, 1995
   */

#define PNG_INTERNAL
#include "png.h"

/* initialize png structure for reading, and allocate any memory needed */
void
png_read_init(png_struct *png_ptr)
{
   jmp_buf tmp_jmp;

   memcpy(tmp_jmp, png_ptr->jmpbuf, sizeof (jmp_buf));
   memset(png_ptr, 0, sizeof (png_struct));
   memcpy(png_ptr->jmpbuf, tmp_jmp, sizeof (jmp_buf));

   png_ptr->zbuf_size = PNG_ZBUF_SIZE;
   png_ptr->zbuf = png_large_malloc(png_ptr, png_ptr->zbuf_size);
   png_ptr->zstream = &(png_ptr->zstream_struct);
   png_ptr->zstream->zalloc = png_zalloc;
   png_ptr->zstream->zfree = png_zfree;
   png_ptr->zstream->opaque = png_ptr;
   inflateInit(png_ptr->zstream);
   png_ptr->zstream->next_out = png_ptr->zbuf;
   png_ptr->zstream->avail_out = (uInt)png_ptr->zbuf_size;
}

/* read the information before the actual image data. */
void
png_read_info(png_struct *png_ptr, png_info *info)
{
   png_byte chunk_start[8];
   png_uint_32 length;

   png_read_data(png_ptr, chunk_start, 8);
   if (memcmp(chunk_start, png_sig, 8))
      png_error(png_ptr, "Not a Png File");

   while (1)
   {
      png_uint_32 crc;

      png_read_data(png_ptr, chunk_start, 8);
      length = png_get_uint_32(chunk_start);
      png_reset_crc(png_ptr);
      png_calculate_crc(png_ptr, chunk_start + 4, 4);
      if (!memcmp(chunk_start + 4, png_IHDR, 4))
      {
         if (png_ptr->mode != PNG_BEFORE_IHDR)
            png_error(png_ptr, "Out of Place IHDR");

         png_handle_IHDR(png_ptr, info, length);
         png_ptr->mode = PNG_HAVE_IHDR;
      }
      else if (!memcmp(chunk_start + 4, png_PLTE, 4))
      {
         if (png_ptr->mode != PNG_HAVE_IHDR)
            png_error(png_ptr, "Missing IHDR");

         png_handle_PLTE(png_ptr, info, length);
         png_ptr->mode = PNG_HAVE_PLTE;
      }
      else if (!memcmp(chunk_start + 4, png_gAMA, 4))
      {
         if (png_ptr->mode != PNG_HAVE_IHDR)
            png_error(png_ptr, "Out of Place PLTE");

         png_handle_gAMA(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_sBIT, 4))
      {
         if (png_ptr->mode != PNG_HAVE_IHDR)
            png_error(png_ptr, "Out of Place sBIT");

         png_handle_sBIT(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_cHRM, 4))
      {
         if (png_ptr->mode != PNG_HAVE_IHDR)
            png_error(png_ptr, "Out of Place cHRM");

         png_handle_cHRM(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_tRNS, 4))
      {
         if (png_ptr->mode != PNG_HAVE_IHDR &&
            png_ptr->mode != PNG_HAVE_PLTE)
            png_error(png_ptr, "Out of Place tRNS");

         png_handle_tRNS(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_bKGD, 4))
      {
         if (png_ptr->mode != PNG_HAVE_IHDR &&
            png_ptr->mode != PNG_HAVE_PLTE)
            png_error(png_ptr, "Out of Place bKGD");

         png_handle_bKGD(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_hIST, 4))
      {
         if (png_ptr->mode != PNG_HAVE_PLTE)
            png_error(png_ptr, "Out of Place hIST");

         png_handle_hIST(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_IDAT, 4))
      {
         png_ptr->idat_size = length;
         png_ptr->mode = PNG_HAVE_IDAT;
         break;
      }
      else if (!memcmp(chunk_start + 4, png_pHYs, 4))
      {
         if (png_ptr->mode != PNG_HAVE_IHDR &&
            png_ptr->mode != PNG_HAVE_PLTE)
            png_error(png_ptr, "Out of Place pHYs");

         png_handle_pHYs(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_oFFs, 4))
      {
         if (png_ptr->mode != PNG_HAVE_IHDR &&
            png_ptr->mode != PNG_HAVE_PLTE)
            png_error(png_ptr, "Out of Place oFFs");

         png_handle_oFFs(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_tIME, 4))
      {
         if (png_ptr->mode == PNG_BEFORE_IHDR ||
            png_ptr->mode == PNG_AFTER_IEND)
            png_error(png_ptr, "Out of Place tIME");

         png_handle_tIME(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_tEXt, 4))
      {
         if (png_ptr->mode == PNG_BEFORE_IHDR ||
            png_ptr->mode == PNG_AFTER_IEND)
            png_error(png_ptr, "Out of Place tEXt");

         png_handle_tEXt(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_zTXt, 4))
      {
         if (png_ptr->mode == PNG_BEFORE_IHDR ||
            png_ptr->mode == PNG_AFTER_IEND)
            png_error(png_ptr, "Out of Place zTXt");

         png_handle_zTXt(png_ptr, info, length);
      }
      else if (!memcmp(chunk_start + 4, png_IEND, 4))
      {
         png_error(png_ptr, "No Image in File");
      }
      else
      {
         if (isupper(chunk_start[4]))
            png_error(png_ptr, "Unknown Critical Chunk");

         png_crc_skip(png_ptr, length);
      }
      png_read_data(png_ptr, chunk_start, 4);
      crc = png_get_uint_32(chunk_start);
      if (((crc ^ 0xffffffffL) & 0xffffffffL) !=
         (png_ptr->crc & 0xffffffffL))
         png_error(png_ptr, "Bad CRC value");
   }
}

/* initialize palette, background, etc, after transformations
   are set, but before any reading takes place.  This allows
   the user to obtail a gamma corrected palette, for example.
   If the user doesn't call this, we will do it ourselves. */
void
png_start_read_image(png_struct *png_ptr)
{
   png_read_start_row(png_ptr);
}

void
png_read_row(png_struct *png_ptr, png_byte *row, png_byte *dsp_row)
{
   int ret;

   if (!(png_ptr->row_init))
      png_read_start_row(png_ptr);

   /* if interlaced and we do not need a new row, combine row and return */
   if (png_ptr->interlaced && (png_ptr->transformations & PNG_INTERLACE))
   {
      switch (png_ptr->pass)
      {
         case 0:
            if (png_ptr->row_number & 7)
            {
               if (dsp_row)
                  png_combine_row(png_ptr, dsp_row, row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 1:
            if ((png_ptr->row_number & 7) || png_ptr->width < 5)
            {
               if (dsp_row)
                  png_combine_row(png_ptr, dsp_row, row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 2:
            if ((png_ptr->row_number & 7) != 4)
            {
               if (dsp_row && (png_ptr->row_number & 4))
                  png_combine_row(png_ptr, dsp_row, row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 3:
            if ((png_ptr->row_number & 3) || png_ptr->width < 3)
            {
               if (dsp_row)
                  png_combine_row(png_ptr, dsp_row, row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 4:
            if ((png_ptr->row_number & 3) != 2)
            {
               if (dsp_row && (png_ptr->row_number & 2))
                  png_combine_row(png_ptr, dsp_row, row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 5:
            if ((png_ptr->row_number & 1) || png_ptr->width < 2)
            {
               if (dsp_row)
                  png_combine_row(png_ptr, dsp_row, row,
                     png_pass_dsp_mask[png_ptr->pass]);
               png_read_finish_row(png_ptr);
               return;
            }
            break;
         case 6:
            if (!(png_ptr->row_number & 1))
            {
               png_read_finish_row(png_ptr);
               return;
            }
            break;
      }
   }

   if (png_ptr->mode != PNG_HAVE_IDAT)
      png_error(png_ptr, "invalid attempt to read row data");

   png_ptr->zstream->next_out = png_ptr->row_buf;
   png_ptr->zstream->avail_out = (uInt)png_ptr->irowbytes;
   do
   {
      if (!(png_ptr->zstream->avail_in))
      {
         while (!png_ptr->idat_size)
         {
            png_byte buf[4];
            png_uint_32 crc;

            png_read_data(png_ptr, buf, 4);
            crc = png_get_uint_32(buf);
            if (((crc ^ 0xffffffffL) & 0xffffffffL) !=
               (png_ptr->crc & 0xffffffffL))
               png_error(png_ptr, "Bad CRC value");

            png_read_data(png_ptr, buf, 4);
            png_ptr->idat_size = png_get_uint_32(buf);
            png_reset_crc(png_ptr);

            png_crc_read(png_ptr, buf, 4);
            if (memcmp(buf, png_IDAT, 4))
               png_error(png_ptr, "Not enough image data");

         }
         png_ptr->zstream->avail_in = (uInt)png_ptr->zbuf_size;
         png_ptr->zstream->next_in = png_ptr->zbuf;
         if (png_ptr->zbuf_size > png_ptr->idat_size)
            png_ptr->zstream->avail_in = (uInt)png_ptr->idat_size;
         png_crc_read(png_ptr, png_ptr->zbuf, png_ptr->zstream->avail_in);
         png_ptr->idat_size -= png_ptr->zstream->avail_in;
      }
      ret = inflate(png_ptr->zstream, Z_PARTIAL_FLUSH);
      if (ret == Z_STREAM_END)
      {
         if (png_ptr->zstream->avail_out || png_ptr->zstream->avail_in ||
            png_ptr->idat_size)
            png_error(png_ptr, "Extra compressed data");
         png_ptr->mode = PNG_AT_LAST_IDAT;
         break;
      }
      if (ret != Z_OK)
         png_error(png_ptr, "Compression Error");

   } while (png_ptr->zstream->avail_out);

   png_ptr->row_info.color_type = png_ptr->color_type;
   png_ptr->row_info.width = png_ptr->iwidth;
   png_ptr->row_info.channels = png_ptr->channels;
   png_ptr->row_info.bit_depth = png_ptr->bit_depth;
   png_ptr->row_info.pixel_depth = png_ptr->pixel_depth;
   png_ptr->row_info.rowbytes = ((png_ptr->row_info.width *
      (png_uint_32)png_ptr->row_info.pixel_depth + 7) >> 3);

   if (png_ptr->row_buf[0])
      png_read_filter_row(&(png_ptr->row_info),
         png_ptr->row_buf + 1, png_ptr->prev_row + 1,
         (int)(png_ptr->row_buf[0]));

   memcpy(png_ptr->prev_row, png_ptr->row_buf, (png_size_t)png_ptr->rowbytes + 1);

   if (png_ptr->transformations)
      png_do_read_transformations(png_ptr);

   /* blow up interlaced rows to full size */
   if (png_ptr->interlaced &&
      (png_ptr->transformations & PNG_INTERLACE))
   {
      if (png_ptr->pass < 6)
         png_do_read_interlace(&(png_ptr->row_info),
            png_ptr->row_buf + 1, png_ptr->pass);

      if (dsp_row)
         png_combine_row(png_ptr, dsp_row, row,
            png_pass_dsp_mask[png_ptr->pass]);
      if (row)
         png_combine_row(png_ptr, row, row,
            png_pass_mask[png_ptr->pass]);
   }
   else
   {
      if (row)
         png_combine_row(png_ptr, row, row, 0xff);
      if (dsp_row)
         png_combine_row(png_ptr, dsp_row, dsp_row, 0xff);
   }
   png_read_finish_row(png_ptr);
}

/* read a one or more rows of image data.   If the image is interlaced,
   and png_set_interlace_handling() has been called, the rows need to
   to contain the contents of the rows from the previous pass.  If
   the image has alpha or transparency, and png_handle_alpha() has been
   called, the rows contents must be initialized to the contents of the
   screen.  row holds the actual image, and pixels are placed in it
   as they arrive.  If the image is displayed after each pass, it will
   appear to "sparkle" in.  display_row can be used to display a
   "chunky" progressive image, with finer detail added as it becomes
   available.  If you do not want this "chunky" display, you may pass
   NULL for display_rows.  If you do not want the sparkle display, and
   you have not called png_handle_alpha(), you may pass NULL for rows.
   If you have called png_handle_alpha(), and the image has either an
   alpha channel or a transparency chunk, you must provide a buffer for
   rows.  In this case, you do not have to provide a display_rows buffer
   also, but you may.  If the image is not interlaced, or if you have
   not called png_set_interlace_handling(), the display_row buffer will
   be ignored, so pass NULL to it. */
void
png_read_rows(png_struct *png_ptr, png_byte **row,
   png_byte **display_row, png_uint_32 num_rows)
{
   png_uint_32 i;
   png_byte **rp;
   png_byte **dp;

   rp = row;
   dp = display_row;
   for (i = 0; i < num_rows; i++)
   {
      png_read_row(png_ptr, *rp, *dp);
      if (row)
         rp++;
      if (display_row)
         dp++;
   }
}

/* read the image.  If the image has an alpha channel or a transparency
   chunk, and you have called png_handle_alpha(), you will need to
   initialize the image to the current image that png will be overlaying.
   Note that png_set_interlace_handling() has no effect on this call.
   You only need to call this function once.  If you desire to have
   an image for each pass of a interlaced image, use png_read_rows() */
void
png_read_image(png_struct *png_ptr, png_byte **image)
{
   png_uint_32 i;
   int pass, j;
   png_byte **rp;

   pass = png_set_interlace_handling(png_ptr);
   for (j = 0; j < pass; j++)
   {
      rp = image;
      for (i = 0; i < png_ptr->height; i++)
      {
         png_read_row(png_ptr, *rp, NULL);
         rp++;
      }
   }
}

/* read the end of the png file.  Will not read past the end of the
   file, will verify the end is accurate, and will read any comments
   or time information at the end of the file, if info is not NULL. */
void
png_read_end(png_struct *png_ptr, png_info *info)
{
   png_byte chunk_start[8];
   png_uint_32 length;
   png_uint_32 crc;

   png_read_data(png_ptr, chunk_start, 4);
   crc = png_get_uint_32(chunk_start);
   if (((crc ^ 0xffffffffL) & 0xffffffffL) !=
      (png_ptr->crc & 0xffffffffL))
      png_error(png_ptr, "Bad CRC value");

   do
   {
      png_read_data(png_ptr, chunk_start, 8);
      length = png_get_uint_32(chunk_start);
      png_reset_crc(png_ptr);
      png_calculate_crc(png_ptr, chunk_start + 4, 4);

      if (!memcmp(chunk_start + 4, png_IHDR, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_PLTE, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_gAMA, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_sBIT, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_cHRM, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_tRNS, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_bKGD, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_hIST, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_IDAT, 4))
      {
         if (length > 0 || png_ptr->mode != PNG_AT_LAST_IDAT)
            png_error(png_ptr, "too many IDAT's found");
      }
      else if (!memcmp(chunk_start + 4, png_pHYs, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_oFFs, 4))
      {
         png_error(png_ptr, "invalid chunk after IDAT");
      }
      else if (!memcmp(chunk_start + 4, png_tIME, 4))
      {
         if (png_ptr->mode == PNG_BEFORE_IHDR ||
            png_ptr->mode == PNG_AFTER_IEND)
            png_error(png_ptr, "Out of Place tIME");

         if (info)
            png_handle_tIME(png_ptr, info, length);
         else
            png_crc_skip(png_ptr, length);
      }
      else if (!memcmp(chunk_start + 4, png_tEXt, 4))
      {
         if (png_ptr->mode == PNG_BEFORE_IHDR ||
            png_ptr->mode == PNG_AFTER_IEND)
            png_error(png_ptr, "Out of Place tEXt");

         if (info)
            png_handle_tEXt(png_ptr, info, length);
         else
            png_crc_skip(png_ptr, length);
      }
      else if (!memcmp(chunk_start + 4, png_zTXt, 4))
      {
         if (png_ptr->mode == PNG_BEFORE_IHDR ||
            png_ptr->mode == PNG_AFTER_IEND)
            png_error(png_ptr, "Out of Place zTXt");

         if (info)
            png_handle_zTXt(png_ptr, info, length);
         else
            png_crc_skip(png_ptr, length);
      }
      else if (!memcmp(chunk_start + 4, png_IEND, 4))
      {
         png_ptr->mode = PNG_AFTER_IEND;
      }
      else
      {
         if (isupper(chunk_start[4]))
            png_error(png_ptr, "Unknown Critical Chunk");

         png_crc_skip(png_ptr, length);
      }
      png_read_data(png_ptr, chunk_start, 4);
      crc = png_get_uint_32(chunk_start);
      if (((crc ^ 0xffffffffL) & 0xffffffffL) !=
         (png_ptr->crc & 0xffffffffL))
         png_error(png_ptr, "Bad CRC value");
      if (png_ptr->mode == PNG_AT_LAST_IDAT)
         png_ptr->mode = PNG_AFTER_IDAT;
   } while (png_ptr->mode != PNG_AFTER_IEND);
}

/* free all memory used by the read */
void
png_read_destroy(png_struct *png_ptr, png_info *info)
{
   int i;
   jmp_buf tmp_jmp;

   if (info)
   {
      if (info->palette != png_ptr->palette)
         png_free(png_ptr, info->palette);
      if (info->trans != png_ptr->trans)
         png_free(png_ptr, info->trans);
      if (info->hist != png_ptr->hist)
         png_free(png_ptr, info->hist);
      for (i = 0; i < info->num_text; i++)
      {
         png_large_free(png_ptr, info->text[i].key);
      }

      /* free stuff for transformations when I get them written */

      png_free(png_ptr, info->text);
      memset(info, 0, sizeof(png_info));
   }

   png_free(png_ptr, png_ptr->zbuf);
   png_free(png_ptr, png_ptr->row_buf);
   png_free(png_ptr, png_ptr->prev_row);
   png_free(png_ptr, png_ptr->palette_lookup);
   png_free(png_ptr, png_ptr->gamma_table);
   if (png_ptr->gamma_16_table)
   {
      for (i = 0; i < (1 << (8 - png_ptr->gamma_shift)); i++)
      {
         png_free(png_ptr, png_ptr->gamma_16_table[i]);
      }
   }
   png_free(png_ptr, png_ptr->gamma_16_table);
   png_free(png_ptr, png_ptr->trans);
   png_free(png_ptr, png_ptr->hist);

   inflateEnd(png_ptr->zstream);

   memcpy(tmp_jmp, png_ptr->jmpbuf, sizeof (jmp_buf));
   memset(png_ptr, 0, sizeof (png_struct));
   memcpy(png_ptr->jmpbuf, tmp_jmp, sizeof (jmp_buf));
}


