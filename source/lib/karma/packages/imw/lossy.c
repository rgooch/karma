/*LINTLIBRARY*/
/*  lossy.c

    This code provides routines to compute a raw image from a data structure.

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

    This file contains various utility routines for drawing into a raw
    image.


    Written by      Richard Gooch   12-JUL-1995

    Updated by      Richard Gooch   12-JUL-1995

    Updated by      Richard Gooch   26-JUL-1995: Fixed bug which appeared now
  that routine is finally being used.

    Updated by      Richard Gooch   10-DEC-1995: Fixed bug in computation of
  horizontal and vertical scale factors.

    Last updated by Richard Gooch   12-APR-1996: Changed to new documentation
  format.


*/

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <karma.h>
#include <karma_imw.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>


/*  Private routines  */
STATIC_FUNCTION (double *alloc_inp_values_buffer, (unsigned int num_values) );
STATIC_FUNCTION (unsigned char *alloc_out_pixels_buffer,
		 (unsigned int num_pixels) );


/*  Public routines follow  */

/*PUBLIC_FUNCTION*/
flag imw_to8_lossy (unsigned char *out_image,
		    iaddr out_hstride, iaddr out_vstride,
		    int out_width, int out_height, CONST char *inp_image,
		    CONST iaddr *inp_hoffsets, CONST iaddr *inp_voffsets,
		    int inp_width, int inp_height,
		    unsigned int inp_type, unsigned int conv_type,
		    unsigned int num_pixels, CONST unsigned char *pixel_values,
		    unsigned char blank_pixel, unsigned char min_sat_pixel,
		    unsigned char max_sat_pixel,
		    double i_min, double i_max,
		    flag (*iscale_func) (), void *iscale_info)
/*  [SUMMARY] Convert generic image to 8 bit image, with resizing.
    [PURPOSE] This routine will convert an image from one format to an 8 bit
    image of pixels, permitting the input and output sizes to differ. If the
    input image is effectively shrunk, the input data is subsampled. The output
    image is flipped vertically relative to the input image.
    <out_image> The output image will be written here.
    <out_hstride> The stride between successive horizontal pixels (in bytes).
    <out_vstride> The stride between successive vertical pixels (in bytes).
    <out_width> The width of the output image.
    <out_height> The height of the output image.
    <inp_image> The input image data.
    <inp_hoffsets> The array of horizontal byte offsets.
    <inp_voffsets> The array of vertical byte offsets.
    <inp_width> The width of the input image.
    <inp_height> The height of the input image.
    <inp_type> The type of the input data.
    <conv_type> The input conversion type (when the input is complex).
    <num_pixels> The number of pixels in the pixel array.
    <pixel_values> The array of pixel values.
    <blank_pixel> The pixel value to be used when the intensity value is an
    undefined value.
    <min_sat_pixel> The pixel value to be used when the intensity value is
    below the minimum value.
    <max_sat_pixel> The pixel value to be used when the intensity value is
    above the maximum value.
    <i_min> The minimum intensity value.
    <i_max> The maximum intensity value.
    <iscale_func> The function to be called when non-linear intensity scaling
    is required. If NULL, linear intensity scaling is used. The prototype
    function is [<IMW_PROTO_iscale_func>].
    <iscale_info> A pointer to arbitrary information for <<iscale_func>>.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag complex;
    int out_x, out_y, inp_x, inp_y;
    float h_factor, v_factor;
    float offset = 0.5;
    float tiny_offset = 1e-6;
    double d_mul, d_data;
    double d_toobig = TOOBIG;
    CONST char *inp_line;
    CONST char *last_inp_line = NULL;
    unsigned char *out_line, *out_pixels;
    double *inp_values;
    static char function_name[] = "imw_to8_lossy";

    if ( (inp_hoffsets == NULL) || (inp_voffsets == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (i_min >= i_max)
    {
	(void) fprintf (stderr, "i_max: %e  is not greater than i_min: %e\n",
			i_max, i_min);
	a_prog_bug (function_name);
    }
    if ( !ds_element_is_atomic (inp_type) )
    {
	(void) fprintf (stderr, "Input image must be atomic\n");
	a_prog_bug (function_name);
    }
    if (getuid () == 465) (void) fprintf (stderr, "%s started...\n",
					  function_name);
    if (getuid () == 465)
    {
	(void) fprintf (stderr, "out_size: %d %d  inp_size: %d %d\n",
			out_width, out_height, inp_width, inp_height);
    }
    /*  Allocate value buffers  */
    if ( ( inp_values = alloc_inp_values_buffer ( (unsigned int) inp_width ) )
	== NULL )
    {
	return (FALSE);
    }
    if ( ( out_pixels = alloc_out_pixels_buffer ( (unsigned int) out_width ) )
	== NULL )
    {
	return (FALSE);
    }
    h_factor = (float) inp_width / (float) out_width;
    v_factor = (float) inp_height / (float) out_height;
    d_mul = (num_pixels - 1) / (i_max - i_min);
    for (out_y = 0; out_y < out_height; ++out_y)
    {
	out_line = out_image + (out_height - out_y - 1) * out_vstride;
	/*  out_line  may be modified now  */
	inp_y = (int) (v_factor * (float) out_y + tiny_offset);
	inp_line = inp_image + inp_voffsets[inp_y];
	/*  The following test will prevent the reading and conversion of
	    duplicated input image lines. A time saver  */
	if (inp_line != last_inp_line)
	{
	    /*  Convert line of input data to generic data type  */
	    /*  Note the cast from (iaddr *) to (uaddr *) for the offset
		arrays. This is dodgy, but it should work.  */
	    if ( !ds_get_scattered_elements (inp_line, inp_type,
					     (uaddr *) inp_hoffsets,
					     inp_values, &complex,
					     (unsigned int) inp_width) )
	    {
		(void) fprintf (stderr, "Error converting data\n");
		return (FALSE);
	    }
	    if (complex) ds_complex_to_real_1D (inp_values, 2, inp_values,
						(unsigned int) inp_width,
						conv_type);
	    if (iscale_func != NULL)
	    {
		if ( !(*iscale_func) (inp_values, 2, inp_values, 2,
				      (unsigned int) inp_width, i_min, i_max,
				      iscale_info) )
		{
		    (void) fprintf (stderr,
				    "Error applying intensity scale\n");
		    return (FALSE);
		}
	    }
	    /*  Now take converted input data and generate an output line. This
		algorithm should work for subsampled data (zoomed out) and
		pixel replicated data (zoomed in).  */
	    for (out_x = 0; out_x < out_width; ++out_x)
	    {
		inp_x = (int) (h_factor * (float) out_x + tiny_offset);
		if ( (d_data = inp_values[inp_x * 2]) < i_min )
		{
		    out_pixels[out_x] = min_sat_pixel;
		}
		else if (d_data >= d_toobig) out_pixels[out_x] = blank_pixel;
		else if (d_data > i_max) out_pixels[out_x] = max_sat_pixel;
		else out_pixels[out_x] = pixel_values[(int) ( (d_data - i_min)
							     *d_mul +offset )];
	    }
	}
	last_inp_line = inp_line;
	/*  Now ready to write the line  */
	/*  Loop for each pixel  */
	for (out_x = 0; out_x < out_width; ++out_x, out_line += out_hstride)
	{
	    *out_line = out_pixels[out_x];
	}
    }
    return (TRUE);
}   /*  End Function imw_to8_lossy  */

static double *alloc_inp_values_buffer (unsigned int num_values)
/*  This routine will allocate a buffer space for generic data values.
    The number of (DCOMPLEX) values to allocate must be given by  num_values  .
    The routine will return a pointer to the buffer space on success,
    else it returns NULL. The buffer is global and must NOT be deallocated.
*/
{
    static unsigned int value_buf_len = 0;
    static double *values = NULL;
    static char function_name[] = "alloc_inp_values_buffer";

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
}   /*  End Function alloc_inp_values_buffer  */

static unsigned char *alloc_out_pixels_buffer (unsigned int num_pixels)
/*  This routine will allocate a buffer space for output pixels.
    The number of pixels to allocate must be given by  num_pixels  .
    The routine will return a pointer to the buffer space on success,
    else it returns NULL. The buffer is global and must NOT be deallocated.
*/
{
    static unsigned int pixel_buf_len = 0;
    static unsigned char *pixels = NULL;
    static char function_name[] = "alloc_out_pixels_buffer";

    /*  Make sure pixel buffer is big enough  */
    if (pixel_buf_len < num_pixels)
    {
	/*  Buffer too small  */
	if (pixels != NULL)
	{
	    m_free ( (char *) pixels );
	}
	pixel_buf_len = 0;
	if ( ( pixels = (unsigned char *) m_alloc (sizeof *pixels * num_pixels)
	      ) == NULL )
	{
	    m_error_notify (function_name, "pixels buffer");
	    return (NULL);
	}
	pixel_buf_len = num_pixels;
    }
    return (pixels);
}   /*  End Function alloc_out_pixels_buffer  */
