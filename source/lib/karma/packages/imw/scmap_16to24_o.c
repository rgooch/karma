/*LINTLIBRARY*/
/*  scmap_16to24_o.c

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


    Written by      Richard Gooch   5-AUG-1995

    Updated by      Richard Gooch   5-SEP-1995: Bug fixes.

    Updated by      Richard Gooch   30-DEC-1995: Flipped vertically.

    Updated by      Richard Gooch   12-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   2-MAY-1996: Threaded code.


*/

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <karma.h>
#include <karma_imw.h>
#include <karma_mt.h>
#include <os.h>


/*  Structures  */
typedef struct
{
    int width;
    int height;
    /*  Stuff for the output image  */
    unsigned char *out_pixel_base;
    unsigned char *out_red_image;
    unsigned char *out_green_image;
    unsigned char *out_blue_image;
    iaddr out_hstride;
    iaddr out_vstride;
    /*  Stuff for the input image  */
    CONST iaddr *inp_hoffsets;
    CONST iaddr *inp_voffsets;
    CONST char *inp_image;
    /*  Stuff for the colourmap  */
    CONST unsigned int *cmap_base;
    CONST unsigned char *cmap_red;
    CONST unsigned char *cmap_green;
    CONST unsigned char *cmap_blue;
    iaddr cmap_stride;
    /*  Extra information  */
    flag h_contig;
    flag slow;
} common_info;


/*  Private routines  */
STATIC_FUNCTION (void job_func,
		 (void *pool_info,
		  void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4,
		  void *thread_info) );


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
/*  [SUMMARY] Write 16bit image to a 24bit image using a software colourmap.
    [PURPOSE] This routine will write a 16bit PseudoColour image into a 24bit
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
    [MT-LEVEL] Safe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    KThreadPool pool;
    iaddr inp_voffset;
    iaddr out_red_offset, out_green_offset, out_blue_offset;
    iaddr cmap_red_offset, cmap_green_offset, cmap_blue_offset;
    iaddr x, y, ystep, num_lines;
    common_info info;
    int uint_size = sizeof (unsigned int);
    unsigned char *out_pixel_base;
    CONST unsigned char *cmap_base;

    info.slow = FALSE;
    if (preserve_pad) info.slow = TRUE;
    if (uint_size != out_hstride) info.slow = TRUE;
    if (uint_size != cmap_stride) info.slow = TRUE;
    if (uint_size < 3) info.slow = TRUE;
    if (out_vstride % uint_size != 0) info.slow = TRUE;
#ifdef MACHINE_crayPVP
    info.slow = TRUE;
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
    if ( (out_green_offset < 0) ||
	 (out_green_offset > out_hstride) ) info.slow = TRUE;
    if ( (out_blue_offset < 0) ||
	 (out_blue_offset > out_hstride) ) info.slow = TRUE;
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
    if (cmap_red_offset != out_red_offset) info.slow = TRUE;
    if (cmap_green_offset != out_green_offset) info.slow = TRUE;
    if (cmap_blue_offset != out_blue_offset) info.slow = TRUE;
    /*  If using fast algorithm, check if the input image is contiguous along
	the horizontal dimension.  */
    if (!info.slow)
    {
	for (x = 0, info.h_contig = TRUE, inp_voffset = 0;
	     info.h_contig && (x < width);
	     ++x, inp_voffset += sizeof *inp_image)
	{
	    if (inp_voffset != inp_hoffsets[x]) info.h_contig = FALSE;
	}
    }
    /*  Fill in common structure  */
    info.width = width;
    info.height = height;
    info.out_pixel_base = out_pixel_base;
    info.out_red_image = out_red_image;
    info.out_green_image = out_green_image;
    info.out_blue_image = out_blue_image;
    info.out_hstride = out_hstride;
    info.out_vstride = out_vstride;
    info.inp_hoffsets = inp_hoffsets;
    info.inp_voffsets = inp_voffsets;
    info.inp_image = (CONST char *) inp_image;
    info.cmap_base = (CONST unsigned int *) cmap_base;
    info.cmap_red = cmap_red;
    info.cmap_green = cmap_green;
    info.cmap_blue = cmap_blue;
    info.cmap_stride = cmap_stride;

    /*  Divide job equally between threads  */
    pool = mt_get_shared_pool ();
    ystep = height / mt_num_threads (pool);
    y = 0;
    while (y < height)
    {
	num_lines = (y + ystep <= height) ? ystep : height - y;
	mt_launch_job (pool, job_func, (void *) y, (void *) num_lines,
		       &info, NULL);
	y += num_lines;
    }
    mt_wait_for_all_jobs (pool);
    return (TRUE);
}   /*  End Function imw_scmap_16to24_o  */


/*  Private routines follow  */

static void job_func (void *pool_info,
		      void *call_info1, void *call_info2,
		      void *call_info3, void *call_info4,
		      void *thread_info)
/*  [SUMMARY] Perform a job.
    <pool_info> The arbitrary pool information pointer.
    <call_info1> The first arbitrary call information pointer.
    <call_info2> The second arbitrary call information pointer.
    <call_info3> The third arbitrary call information pointer.
    <call_info4> The fourth arbitrary call information pointer.
    <thread_info> A pointer to arbitrary, per thread, information. This
    information is private to the thread.
    [RETURNS] Nothing.
*/
{
    int x, width, y, endy;
    iaddr inp_voffset;
    iaddr out_hoffset, out_hstride;
    iaddr cmap_stride;
    unsigned short pixel;
    unsigned char *out_red_ptr;
    unsigned char *out_green_ptr;
    unsigned char *out_blue_ptr;
    CONST iaddr *inp_hoffsets;
    common_info *info = (common_info *) call_info3;
    unsigned int *out_uint_ptr;
    CONST unsigned int *cmap_ptr;
    CONST char *inp_image;
    CONST unsigned char *cmap_red;
    CONST unsigned char *cmap_green;
    CONST unsigned char *cmap_blue;
    CONST unsigned short *inp_us_ptr;

    y = (iaddr) call_info1;
    endy = y + (iaddr) call_info2;
    out_red_ptr = info->out_red_image + y * info->out_vstride;
    out_green_ptr = info->out_green_image + y * info->out_vstride;
    out_blue_ptr = info->out_blue_image + y * info->out_vstride;
    width = info->width;
    out_hstride = info->out_hstride;
    inp_image = (CONST char *) info->inp_image;
    inp_hoffsets = info->inp_hoffsets;
    cmap_stride = info->cmap_stride;
    cmap_red = info->cmap_red;
    cmap_green = info->cmap_green;
    cmap_blue = info->cmap_blue;
    cmap_ptr = info->cmap_base;
    if (info->slow)
    {
	/*  Do this the simple (slow) way  */
	for (; y < endy; ++y)
	{
	    inp_voffset = info->inp_voffsets[info->height - y - 1];
	    for (x = 0, out_hoffset = 0; x < width;
		 ++x, out_hoffset += out_hstride)
	    {
		pixel = *(unsigned short *) (inp_image + inp_hoffsets[x] +
					     inp_voffset);
		pixel *= cmap_stride;
		out_red_ptr[out_hoffset] = cmap_red[pixel];
		out_green_ptr[out_hoffset] = cmap_green[pixel];
		out_blue_ptr[out_hoffset] = cmap_blue[pixel];
	    }
	    out_red_ptr += info->out_vstride;
	    out_green_ptr += info->out_vstride;
	    out_blue_ptr += info->out_vstride;
	}
	return;
    }
    /*  Hoon along  */
    for (; y < endy; ++y)
    {
	inp_voffset = info->inp_voffsets[info->height - y - 1];
	out_uint_ptr = (unsigned int *) (info->out_pixel_base +
					 y * info->out_vstride);
	if (info->h_contig)
	{
	    inp_us_ptr = (unsigned short *) (inp_image + inp_voffset);
	    for (x = 0; x < width; ++x, ++out_uint_ptr)
	    {
		pixel = inp_us_ptr[x];
		*out_uint_ptr = cmap_ptr[pixel];
	    }
	}
	else for (x = 0; x < width; ++x, ++out_uint_ptr)
	{
	    pixel = *(unsigned short *) (inp_image +inp_hoffsets[x]+inp_voffset);
	    *out_uint_ptr = cmap_ptr[pixel];
	}
    }
}  /*  End Function job_func  */
