/*LINTLIBRARY*/
/*  sunras_write.c

    This code provides a Sun rasterfile write facility.

    Copyright (C) 1995-1996  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*

    This file contains the various utility routines for writing images in 
  Sun rasterfile format.


    Written by      Richard Gooch   22-MAY-1995

    Updated by      Richard Gooch   22-MAY-1995

    Updated by      Richard Gooch   24-AUG-1995: Added code to pad image lines
  to 16 bits.

    Updated by      Richard Gooch   6-SEP-1995: Added _NO_IMAGE attribute.

    Updated by      Richard Gooch   12-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   4-JUN-1996: Added intensity mapping for
  <foreign_sunras_write_rgb>.


*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_iarray.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>


#define RAS_MAGIC 0x59a66a95

#define RT_OLD          0
#define RT_STANDARD     1
#define RT_BYTE_ENCODED 2
#define RT_FORMAT_RGB   3
#define RT_FORMAT_TIFF  4
#define RT_FORMAT_IFF   5
#define RT_EXPERIMENTAL 0xffff

#define RMT_NONE        0
#define RMT_EQUAL_RGB   1
#define RMT_RAW         2

/*  Rasterfile structure  */
struct rasterfile
{
    int ras_magic;
    int ras_width;
    int ras_height;
    int ras_depth;
    int ras_length;
    int ras_type;
    int ras_maptype;
    int ras_maplength;
};


/*  Private functions  */
STATIC_FUNCTION (double *alloc_values_buffer, (unsigned int num_values) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag foreign_sunras_write (Channel channel, multi_array *multi_desc, ...)
/*  [SUMMARY] Write a colour image to a channel in Sun rasterfile format
    <channel> The channel to write to. The channel is not flushed.
    <multi_desc> The multi_array descriptor pointer. The routine will find a
    TrueColour image or a PseudoColour image within the data structure.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_SUNRAS_WRITE_END. See [<FOREIGN_ATT_SUNRAS_WRITE>] for a list of defined
    attributes.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    iarray image_pseudo;
    iarray image_red, image_green, image_blue;
    flag ok;
    unsigned int cmap_index;
    unsigned int att_key;
    int width, height;
    va_list argp;
    flag *no_image = NULL;
    static char function_name[] = "foreign_sunras_write";

    va_start (argp, multi_desc);
    if ( ( channel == NULL) || (multi_desc == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_SUNRAS_WRITE_END )
    {
	switch (att_key)
	{
	  case FA_SUNRAS_WRITE_NO_IMAGE:
	    no_image = va_arg (argp, flag *);
	    break;
	  default:
	    (void) fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    if (no_image != NULL) *no_image = FALSE;
    if ( !iarray_get_image_from_multi (multi_desc, &image_pseudo,
				       &image_red, &image_green, &image_blue,
				       &cmap_index) )
    {
	if (no_image != NULL) *no_image = TRUE;
	return (FALSE);
    }
    /*  Sanity checks  */
    if (image_red != NULL)
    {
	/*  Have RGB image  */
	if ( (iarray_type (image_red) != K_UBYTE) ||
	     (iarray_type (image_green) != K_UBYTE) ||
	     (iarray_type (image_blue) != K_UBYTE) )
	{
	    (void) fprintf (stderr,
			    "TrueColour image data must be of type K_UBYTE\n");
	    iarray_dealloc (image_red);
	    iarray_dealloc (image_green);
	    iarray_dealloc (image_blue);
	    return (FALSE);
	}
	width = iarray_dim_length (image_red, 1);
	height = iarray_dim_length (image_red, 0);
	ok = foreign_sunras_write_rgb(channel,
				      (CONST unsigned char *) image_red->data,
				      (CONST unsigned char *)image_green->data,
				      (CONST unsigned char *) image_blue->data,
				      image_red->offsets[1],
				      image_red->offsets[0],
				      width, height, NULL, NULL, NULL, 0);
	iarray_dealloc (image_red);
	iarray_dealloc (image_green);
	iarray_dealloc (image_blue);
	return (ok);
    }
    a_func_abort (function_name,  "PseudoColour images not supported yet");
    iarray_dealloc (image_pseudo);
    return (FALSE);
}   /*  End Function foreign_sunras_write  */

/*EXPERIMENTAL_FUNCTION*/
flag foreign_sunras_write_pseudo (Channel channel,
				  CONST char *image, unsigned int type,
				  uaddr *hoffsets, uaddr *voffsets,
				  unsigned int width, unsigned int height,
				  CONST unsigned short *cmap_reds,
				  CONST unsigned short *cmap_greens,
				  CONST unsigned short *cmap_blues,
				  unsigned int cmap_size,
				  unsigned int cmap_stride,
				  double i_min, double i_max)
/*  [SUMMARY] Write a PseudoColor image to a channel in Sun rasterfile format
    <channel> The channel to write to. The channel is not flushed.
    <image> The image data.
    <type> The type of the image data.
    <hoffsets> The array of horizontal byte offsets.
    <voffsets> The array of vertical byte offsets.
    <width> The width of the image.
    <height> The height of the image.
    <cmap_reds> The red colourmap values.
    <cmap_greens> The green colourmap values.
    <cmap_blues> The blue colourmap values.
    <cmap_size> The number of colourmap entries.
    <cmap_stride> The stride (in unsigned shorts) between colourmap values.
    <i_min> The minimum image value. Image values below this will be clipped.
    <i_max> The maximum image value. Image values above this will be clipped.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    flag complex;
    char bval;
    int hcount, vcount;
    unsigned int count;
    long line_length;
    double d_toobig = TOOBIG;
    double d_mul;
    double d_data;
    CONST char *inp_line;
    double *values, *val_ptr;
    static char function_name[] = "foreign_sunras_write_pseudo";

    if ( (hoffsets == NULL) || (voffsets == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (cmap_size > 256)
    {
	(void) fprintf (stderr, "Too many colour values: %u\n", cmap_size);
	a_prog_bug (function_name);
    }
    if (i_min >= i_max)
    {
	(void) fprintf (stderr, "i_max: %e  is not greater than i_min: %e\n",
			i_max, i_min);
	a_prog_bug (function_name);
    }
    if ( !ds_element_is_atomic (type) )
    {
	(void) fprintf (stderr, "Input image must be atomic\n");
	a_prog_bug (function_name);
    }
    if ( ds_element_is_complex (type) )
    {
	(void) fprintf (stderr, "Input image must not be complex\n");
	a_prog_bug (function_name);
    }
    /*  Allocate values buffer  */
    if ( ( values = alloc_values_buffer (width) ) == NULL ) return (FALSE);
    /*  Compute line length padded to 16 bits  */
    line_length = width + ( (width & 1) ? 1 : 0 );
    if ( !pio_write32s (channel, RAS_MAGIC) ) return (FALSE);
    if ( !pio_write32s (channel, width) ) return (FALSE);
    if ( !pio_write32s (channel, height) ) return (FALSE);
    if ( !pio_write32s (channel, 8) ) return (FALSE);
    if ( !pio_write32s (channel, line_length * height) ) return (FALSE);
    if ( !pio_write32s (channel, RT_FORMAT_RGB) ) return (FALSE);
    if (cmap_size > 0)
    {
	if (cmap_size < 2)
	{
	    (void) fprintf (stderr, "Illegal colourmap size: %u\n", cmap_size);
	    a_prog_bug (function_name);
	}
	/*  Write colourmap  */
	if ( !pio_write32s (channel, RMT_EQUAL_RGB) ) return (FALSE);
	if ( !pio_write32s (channel, cmap_size * 3) ) return (FALSE);
	for (count = 0; count < cmap_size; ++count)
	{
	    bval = (cmap_reds[count * cmap_stride] >> 8) & 0xff;
	    if (ch_write (channel, &bval, 1) < 1) return FALSE;
	}
	for (count = 0; count < cmap_size; ++count)
	{
	    bval = (cmap_greens[count * cmap_stride] >> 8) & 0xff;
	    if (ch_write (channel, &bval, 1) < 1) return FALSE;
	}
	for (count = 0; count < cmap_size; ++count)
	{
	    bval = (cmap_blues[count * cmap_stride] >> 8) & 0xff;
	    if (ch_write (channel, &bval, 1) < 1) return FALSE;
	}
	d_mul = (cmap_size - 1) / (i_max - i_min);
    }
    else
    {
	/*  No colourmap  */
	if ( !pio_write32s (channel, RMT_NONE) ) return (FALSE);
	if ( !pio_write32s (channel, 0) ) return (FALSE);
	d_mul = 255.0 / (i_max - i_min);
    }
    /*  Loop through the image lines  */
    for (vcount = height - 1; vcount >= 0; --vcount)
    {
	inp_line = image + voffsets[vcount];
	/*  inp_line  may be modified now  */
	/*  Now have pointer to input image line  */
	/*  Convert data to generic data type  */
	if ( !ds_get_scattered_elements (inp_line, type, hoffsets,
					 values, &complex, width) )
	{
	    (void) fprintf (stderr, "Error converting data\n");
	    return (FALSE);
	}
	/*  Loop for each value  */
	for (hcount = 0, val_ptr = values; hcount < width;
	     ++hcount, val_ptr += 2)
	{
	    if ( (d_data = *val_ptr) < i_min ) bval = 0;
	    else if (d_data >= d_toobig) bval = cmap_size - 1;
	    else if (d_data > i_max) bval = cmap_size - 1;
	    else bval = (int) ( (d_data - i_min) * d_mul + 0.5 );
	    if (ch_write (channel, &bval, 1) < 1) return FALSE;
	}
	/*  Line written: maybe need to pad  */
	if (width & 1)
	{
	    bval = 0;
	    if (ch_write (channel, &bval, 1) < 1) return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function foreign_sunras_write_pseudo  */

/*EXPERIMENTAL_FUNCTION*/
flag foreign_sunras_write_rgb (Channel channel,
			       CONST unsigned char *image_red,
			       CONST unsigned char *image_green,
			       CONST unsigned char *image_blue,
			       uaddr *hoffsets, uaddr *voffsets,
			       unsigned int width, unsigned int height,
			       CONST unsigned short *cmap_red,
			       CONST unsigned short *cmap_green,
			       CONST unsigned short *cmap_blue,
			       unsigned int cmap_stride)
/*  [SUMMARY] Write a TrueColor image to a channel in Sun rasterfile format
    <channel> The channel to write to. The channel is not flushed.
    <red_image> The red image data.
    <green_image> The green image data.
    <blue_image> The blue image data.
    <hoffsets> The array of horizontal byte offsets.
    <voffsets> The array of vertical byte offsets.
    <width> The width of the image.
    <height> The height of the image.
    <cmap_red> The red component colourmap entries. 256 entries required. If
    this is NULL a linear mapping is assumed.
    <cmap_green> The green component colourmap entries. 256 entries required.
    If this is NULL a linear mapping is assumed.
    <cmap_blue> The blue component colourmap entries. 256 entries required. If
    this is NULL a linear mapping is assumed.
    <cmap_stride> The stride (in unsigned shorts) between colourmap values.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    int hcount, vcount;
    int red, green, blue;
    long line_length;
    uaddr voffset;
    unsigned char pixel[3];
    static char function_name[] = "foreign_sunras_write_rgb";

    if ( (hoffsets == NULL) || (voffsets == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Compute line length padded to 16 bits  */
    line_length = 3 * width + ( (width & 1) ? 1 : 0 );
    if ( !pio_write32s (channel, RAS_MAGIC) ) return (FALSE);
    if ( !pio_write32s (channel, width) ) return (FALSE);
    if ( !pio_write32s (channel, height) ) return (FALSE);
    if ( !pio_write32s (channel, 24) ) return (FALSE);
    if ( !pio_write32s (channel, line_length * height) ) return (FALSE);
    if ( !pio_write32s (channel, RT_FORMAT_RGB) ) return (FALSE);
    if ( !pio_write32s (channel, RMT_NONE) ) return (FALSE);
    if ( !pio_write32s (channel, 0) ) return (FALSE);
    /*  Loop through the image lines  */
    for (vcount = height - 1; vcount >= 0; --vcount)
    {
	voffset = voffsets[vcount];
	for (hcount = 0; hcount < width; ++hcount)
	{
	    red = image_red[hoffsets[hcount] + voffset];
	    if (cmap_red != NULL) red = (cmap_red[red * cmap_stride] >>
					 8) & 0xff;
	    pixel[0] = red;
	    green = image_green[hoffsets[hcount] + voffset];
	    if (cmap_green != NULL) green = (cmap_green[green * cmap_stride] >>
					     8) & 0xff;
	    pixel[1] = green;
	    blue = image_blue[hoffsets[hcount] + voffset];
	    if (cmap_blue != NULL) blue = (cmap_blue[blue * cmap_stride] >>
					   8) & 0xff;
	    pixel[2] = blue;
	    if (ch_write (channel, (char *) pixel, 3) < 3) return (FALSE);
	}
	/*  Line written: maybe need to pad  */
	if (width & 1)
	{
	    pixel[0] = 0;
	    if (ch_write (channel, (char *) pixel, 1) < 1) return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function foreign_sunras_write_rgb  */


/*  Private functions follow  */

static double *alloc_values_buffer (unsigned int num_values)
/*  This routine will allocate a buffer space for generic data values.
    The number of (DCOMPLEX) values to allocate must be given by  num_values  .
    The routine will return a pointer to the buffer space on success,
    else it returns NULL. The buffer is global and must NOT be deallocated.
*/
{
    static unsigned int value_buf_len = 0;
    static double *values = NULL;
    static char function_name[] = "alloc_values_buffer";

    /*  Make sure value buffer is big enough  */
    if (value_buf_len < num_values)
    {
	/*  Buffer too small  */
	if (values != NULL)
	{
	    m_free ( (char *) values );
	}
	value_buf_len = 0;
	if ( ( values = (double *) m_alloc (sizeof *values * 2 * num_values)
	      ) == NULL )
	{
	    m_error_notify (function_name, "values buffer");
	    return (NULL);
	}
	value_buf_len = num_values;
    }
    return (values);
}   /*  End Function alloc_values_buffer  */
