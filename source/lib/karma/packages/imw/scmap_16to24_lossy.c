/*LINTLIBRARY*/
/*  scmap_16to24_o.c

    This code provides routines to compute a raw image from a data structure.

    Copyright (C) 1996  Richard Gooch

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


    Written by      Richard Gooch   22-APR-1996

    Last updated by Richard Gooch   22-APR-1996


*/

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <karma.h>
#include <karma_imw.h>
#include <os.h>


/*  Private routines  */


/*  Private data  */


/*  Public routines follow  */

/*EXPERIMENTAL_FUNCTION*/
flag imw_scmap_16to24_lossy (unsigned char *out_red_image,
			     unsigned char *out_green_image,
			     unsigned char *out_blue_image, flag preserve_pad,
			     iaddr out_hstride, iaddr out_vstride,
			     int out_width, int out_height,
			     CONST unsigned short *inp_image,
			     int inp_width, int inp_height,
			     CONST iaddr *inp_hoffsets,
			     CONST iaddr *inp_voffsets,
			     CONST unsigned char *cmap_red,
			     CONST unsigned char *cmap_green,
			     CONST unsigned char *cmap_blue, iaddr cmap_stride)
/*  [SUMMARY] Write 16bit image to a 24bit image using a software colourmap.
    [PURPOSE] This routine will write a 16bit PseudoColour image into a 24bit
    TrueColour image using a software colourmap to convert 16bit values to
    24bit RGB values. The input and output sizes may be different. The output
    image is flipped vertically relative to the input image.
    <out_red_image> The output red image components will be written here.
    <out_green_image> The output green image components will be written here.
    <out_blue_image> The output blue image components will be written here.
    <preserve_pad> If TRUE, padding bytes between output pixels will be
    preserved, else they may be overwritten for efficiency.
    <out_hstride> The horizontal stride in bytes between output pixels.
    <out_vstride> The vertical stride in bytes between output lines.
    <out_width> The width of the output image.
    <out_height> The height of the output image.
    <inp_image> The input 16bit PseudoColour image.
    <inp_hoffsets> The array of horizontal input byte offsets.
    <inp_voffsets> The array of vertical input byte offsets.
    <inp_width> The width of the input image.
    <inp_height> The height of the input image.
    <cmap_red> The red component virtual colourmap.
    <cmap_green> The green component virtual colourmap.
    <cmap_blue> The blue component virtual colourmap.
    <cmap_stride> The stride in bytes between colourmap entries. The number of
    entries must be 65536 (16bit addressing).
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    iaddr line_offset;
    int out_x, out_y, inp_x, inp_y;
    int pixel16;
    float h_factor, v_factor;
    float tiny_offset = 1e-6;
    CONST char *inp_line;
    unsigned char *out_red_ptr, *out_green_ptr, *out_blue_ptr;

    h_factor = (float) inp_width / (float) out_width;
    v_factor = (float) inp_height / (float) out_height;
    /*  Generate output lines  */
    for (out_y = 0; out_y < out_height; ++out_y)
    {
	line_offset = (out_height - out_y - 1) * out_vstride;
	out_red_ptr = out_red_image + line_offset;
	out_green_ptr = out_green_image + line_offset;
	out_blue_ptr = out_blue_image + line_offset;
	inp_y = (int) (v_factor * (float) out_y + tiny_offset);
        inp_line = (CONST char *) inp_image + inp_voffsets[inp_y];
	/*  Generate output pixels  */
	for (out_x = 0; out_x < out_width;
	     ++out_x, out_red_ptr += out_hstride, out_green_ptr += out_hstride,
		 out_blue_ptr += out_hstride)
	{
	    inp_x = (int) (h_factor * (float) out_x + tiny_offset);
	    pixel16 = *(unsigned short *) (inp_line + inp_hoffsets[inp_x]);
	    pixel16 *= cmap_stride;
	    *out_red_ptr = cmap_red[pixel16];
	    *out_green_ptr = cmap_green[pixel16];
	    *out_blue_ptr = cmap_blue[pixel16];
	}
    }
    return (TRUE);
}   /*  End Function imw_scmap_16to24_lossy  */
