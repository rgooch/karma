/*LINTLIBRARY*/
/*  scmap_16to24_o.c

    This code provides routines to compute a raw image from a data structure.

    Copyright (C) 1995  Richard Gooch

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


    Written by      Richard Gooch   5-AUG-1995

    Updated by      Richard Gooch   5-SEP-1995: Bug fixes.

    Last updated by Richard Gooch   30-DEC-1995: Flipped vertically.


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
flag imw_scmap_16to24_o (unsigned char *out_red_image,
			 unsigned char *out_green_image,
			 unsigned char *out_blue_image, flag preserve_pad,
			 iaddr out_hstride, iaddr out_vstride,
			 int width, int height,CONST unsigned short *inp_image,
			 CONST iaddr *inp_hoffsets, CONST iaddr *inp_voffsets,
			 CONST unsigned char *cmap_red,
			 CONST unsigned char *cmap_green,
			 CONST unsigned char *cmap_blue, iaddr cmap_stride)
/*  [PURPOSE] This routine will write a 16bit PseudoColour image into a 24bit
    TrueColour image using a software colourmap to convert 16bit values to
    24bit RGB values. The image size (in pixels) is preserved. The output
    image is flipped vertically relative to the input image.
    <out_red_image> The output red image components will be written here.
    <out_green_image> The output green image components will be written here.
    <out_blue_image> The output blue image components will be written here.
    <preserve_pad> If TRUE, padding bytes between output pixels will be
    preserved, else they may be overwritten for efficiency.
    <out_hstride> The horizontal stride in bytes between output pixels.
    <out_vstride> The vertical stride in bytes between output lines.
    <width> The width of the image.
    <height> The height of the image.
    <inp_image> The input 16bit PseudoColour image.
    <inp_hoffsets> The array of horizontal input byte offsets.
    <inp_voffsets> The array of vertical input byte offsets.
    <cmap_red> The red component virtual colourmap.
    <cmap_green> The green component virtual colourmap.
    <cmap_blue> The blue component virtual colourmap.
    <cmap_stride> The stride in bytes between colourmap entries. The number of
    entries must be 65536 (16bit addressing).
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag slow = FALSE;
    flag h_contig;
    iaddr inp_voffset;
    iaddr out_hoffset;
    iaddr out_red_offset, out_green_offset, out_blue_offset;
    iaddr cmap_red_offset, cmap_green_offset, cmap_blue_offset;
    int x, y;
    int uint_size = sizeof (unsigned int);
    unsigned short pixel;
    unsigned char *out_pixel_base;
    CONST unsigned char *cmap_base;
    CONST char *inp_ptr;
    CONST unsigned short *inp_us_ptr;
    unsigned int *out_uint_ptr, *cmap_ptr;

    if (preserve_pad) slow = TRUE;
    if (uint_size != out_hstride) slow = TRUE;
    if (uint_size != cmap_stride) slow = TRUE;
    if (uint_size < 3) slow = TRUE;
    if (out_vstride % uint_size != 0) slow = TRUE;
#ifdef MACHINE_crayPVP
    slow = TRUE;
#endif
    /*  Check if nice output  */
    out_pixel_base = out_red_image;
    out_red_offset = out_red_image - out_pixel_base;
    out_green_offset = out_green_image - out_pixel_base;
    out_blue_offset = out_blue_image - out_pixel_base;
    while ( !IS_ALIGNED (out_pixel_base, uint_size) )
    {
	--out_pixel_base;
	++out_red_offset;
	++out_green_offset;
	++out_blue_offset;
    }
    if ( (out_green_offset <0) || (out_green_offset >out_hstride) ) slow =TRUE;
    if ( (out_blue_offset < 0) || (out_blue_offset > out_hstride) ) slow =TRUE;
    /*  Check if nice cmap  */
    cmap_base = cmap_red;
    cmap_red_offset = cmap_red - cmap_base;
    cmap_green_offset = cmap_green - cmap_base;
    cmap_blue_offset = cmap_blue - cmap_base;
    while ( !IS_ALIGNED (cmap_base, uint_size) )
    {
	--cmap_base;
	++cmap_red_offset;
	++cmap_green_offset;
	++cmap_blue_offset;
    }
    if (cmap_red_offset != out_red_offset) slow = TRUE;
    if (cmap_green_offset != out_green_offset) slow = TRUE;
    if (cmap_blue_offset != out_blue_offset) slow = TRUE;
    inp_ptr = (CONST char *) inp_image;
    if (slow)
    {
	/*  Do this the simple (slow) way  */
	for (y = 0; y < height; ++y)
	{
	    inp_voffset = inp_voffsets[height - y - 1];
	    for (x = 0, out_hoffset = 0; x < width;
		 ++x, out_hoffset += out_hstride)
	    {
		pixel = *(unsigned short *) (inp_ptr + inp_hoffsets[x] +
					     inp_voffset);
		pixel *= cmap_stride;
		out_red_image[out_hoffset] = cmap_red[pixel];
		out_green_image[out_hoffset] = cmap_green[pixel];
		out_blue_image[out_hoffset] = cmap_blue[pixel];
	    }
	    out_red_image += out_vstride;
	    out_green_image += out_vstride;
	    out_blue_image += out_vstride;
	}
	return (TRUE);
    }
    /*  Hoon along  */
    for (x = 0, h_contig = TRUE, inp_voffset = 0; h_contig && (x < width);
	 ++x, inp_voffset += sizeof *inp_image)
    {
	if (inp_voffset != inp_hoffsets[x]) h_contig = FALSE;
    }
    cmap_ptr = (unsigned int *) cmap_base;
    for (y = 0; y < height; ++y, out_pixel_base += out_vstride)
    {
	inp_voffset = inp_voffsets[height - y - 1];
	out_uint_ptr = (unsigned int *) out_pixel_base;
	if (h_contig)
	{
	    inp_us_ptr = (unsigned short *) (inp_ptr + inp_voffset);
	    for (x = 0; x < width; ++x, ++out_uint_ptr)
	    {
		pixel = inp_us_ptr[x];
		*out_uint_ptr = cmap_ptr[pixel];
	    }
	}
	else for (x = 0; x < width; ++x, ++out_uint_ptr)
	{
	    pixel = *(unsigned short *) (inp_ptr +inp_hoffsets[x]+inp_voffset);
	    *out_uint_ptr = cmap_ptr[pixel];
	}
    }
    return (TRUE);
}   /*  End Function imw_scmap_16to24_o  */
