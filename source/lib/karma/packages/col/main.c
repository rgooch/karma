/*LINTLIBRARY*/
/*  main.c

    This code provides basic colour conversion support.

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

    This file provides basic colour conversion support.


    Written by      Richard Gooch   29-DEC-1995

    Updated by      Richard Gooch   6-APR-1996: Changed to new documentation
  style.

    Updated by      Richard Gooch   1-MAY-1996: Threaded code.

    Last updated by Richard Gooch   3-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <karma.h>
#include <karma_col.h>
#include <karma_mt.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_c.h>


/*  Structures  */
typedef struct
{
    unsigned long *rgb_array;
    float start_hue;
    float hue_scale;
    float saturation;
    float min_brightness;
    float brightness_scale;
    int red_shift;
    int green_shift;
    int blue_shift;
} common_info;


/*  Private routines  */
STATIC_FUNCTION (void job_func,
		 (void *pool_info,
		  void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4,
		  void *thread_info) );


/*  Private data  */


/*  Public routines follow  */

/*PUBLIC_FUNCTION*/
void col_hsb_slice_to_rgb_array (unsigned long rgb_array[65536],
				 unsigned long red_mask,
				 unsigned long green_mask,
				 unsigned long blue_mask,
				 float min_brightness, float max_brightness,
				 float start_hue, float stop_hue,
				 float saturation)
/*  [SUMMARY] Generate a table of RGB values from a slice of HSB space.
    [PURPOSE] This routine will convert a slice in HSB (Hue, Saturation and
    Brightness) colour space to a 2-dimensional array of RGB (Red, Green and
    Blue) values. The HSB slice covers a range of hue and brightness at a fixed
    satutation.
    <rgb_array> The 256*256 array of RGB values will be written here. The upper
    dimension will map to brightness and the lower dimension will map to hue.
    <red_mask> The bitmask used when writing the red colour component.
    <green_mask> The bitmask used when writing the green colour component.
    <blue_mask> The bitmask used when writing the blue colour component.
    <min_brightness> The minimum brightness. The valid range is 0.0 to 1.0
    <max_brightness> The maximum brightness. The valid range is 0.0 to 1.0
    <start_hue> The start hue. The valid range is 0.0 to just under 6.0
    <stop_hue> The end hue. The valid range is 0.0 to just under 6.0
    <saturation> The saturation value. The valid range is 0.0 to 1.0
    [MT-LEVEL] Safe.
    [RETURNS] Nothing.
*/
{
    KThreadPool pool;
    common_info info;
    iaddr count, row_step, num_rows;
    static char function_name[] = "col_hsb_slice_to_rgb_array";

    pool = mt_get_shared_pool ();
    if (start_hue >= 6.0)
    {
	(void) fprintf (stderr, "start_hue: %e must be less than 6.0\n",
			start_hue);
	a_prog_bug (function_name);
    }
    if (stop_hue >= 6.0)
    {
	(void) fprintf (stderr, "stop_hue: %e must be less than 6.0\n",
			stop_hue);
	a_prog_bug (function_name);
    }
/*
    (void) fprintf (stderr, "%s: minb: %e  maxb: %e\nstarth: %e  stoph: %e\n",
		    function_name,
		    min_brightness, max_brightness, start_hue, stop_hue);
    (void) fprintf (stderr, "sat: %e\n", saturation);
*/
    /*  Fill in common structure  */
    info.rgb_array = rgb_array;
    info.start_hue = start_hue;
    info.saturation = saturation;
    /*  Compute the (left) bit shifts for each mask  */
    for (info.red_shift = 0; !(red_mask & 1);
	 red_mask = red_mask >> 1, ++info.red_shift);
    if (red_mask != 0xff)
    {
	(void) fprintf (stderr, "Shifted red_mask: %lx is not 0xff\n",
			red_mask);
	a_prog_bug (function_name);
    }
    for (info.green_shift = 0; !(green_mask & 1);
	 green_mask = green_mask >> 1, ++info.green_shift);
    if (green_mask != 0xff)
    {
	(void) fprintf (stderr, "Shifted green_mask: %lx is not 0xff\n",
			green_mask);
	a_prog_bug (function_name);
    }
    for (info.blue_shift = 0; !(blue_mask & 1);
	 blue_mask = blue_mask >> 1, ++info.blue_shift);
    if (blue_mask != 0xff)
    {
	(void) fprintf (stderr, "Shifted blue_mask: %lx is not 0xff\n",
			blue_mask);
	a_prog_bug (function_name);
    }
    /*  Loop through the brightness range  */
    /*  <pixel> will go: 0, 1, 2, 3, ..., 255, 256, 257, ...  */
    info.brightness_scale = max_brightness - min_brightness;
    info.min_brightness = min_brightness * 255.0;
    info.hue_scale = (stop_hue - start_hue) / 255.0;
    /*  Divide job equally between threads  */
    row_step = 256 / mt_num_threads (pool);
    count = 0;
    while (count < 256)
    {
	num_rows = (count + row_step <= 256) ? row_step : 256 - count;
	mt_launch_job (pool, job_func, (void *) count, (void *) num_rows,
		       &info, NULL);
	count += row_step;
    }
    mt_wait_for_all_jobs (pool);
}  /*  End Function col_hsb_slice_to_rgb_array  */


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
    int index1, end_row, index2;
    int pixel;
    int colour;
    int i_hue, i_brightness;
    int red, green, blue;
    int p, q, t;
    float hue;
    float brightness;
    float f;
    float one = 1.0;
    common_info *info = (common_info *) call_info3;
    /*static char function_name[] = "col_hsb_slice_to_rgb_array";*/

    /*  Loop through the brightness range  */
    /*  <pixel> will go: 0, 1, 2, 3, ..., 255, 256, 257, ...  */
    index1 = (iaddr) call_info1;
    end_row = index1 + (iaddr) call_info2;
    pixel = index1 * 256;
    for (; index1 < end_row; ++index1)
    {
	/*  Compute brightness from co-ordinate  */
	brightness = info->min_brightness +
	    (float) index1 * info->brightness_scale;
	i_brightness = brightness;  /*  Truncate to integer  */
	p = ( brightness * (one - info->saturation) );
	/*  Loop through the hue range  */
	for (index2 = 0; index2 < 256; ++index2, ++pixel)
	{
	    /*  Compute hue from co-ordinate  */
	    hue = info->start_hue + info->hue_scale * (float) index2;
	    i_hue = hue;  /*  Truncate to integer  */
	    /*  Compute RGB value from HSB value  */
	    f = hue - (float) i_hue;  /*  The fractional hue component  */
	    q = ( brightness * ( one - (info->saturation * f) ) );
	    t = ( brightness * ( one - ( info->saturation * (one - f) ) ) );
	    switch (i_hue)
	    {
	      case 0:
		red = i_brightness;
		green = t;
		blue = p;
		break;
	      case 1:
		red = q;
		green = i_brightness;
		blue = p;
		break;
	      case 2:
		red = p;
		green = i_brightness;
		blue = t;
		break;
	      case 3:
		red = p;
		green = q;
		blue = i_brightness;
		break;
	      case 4:
		red = t;
		green = p;
		blue = i_brightness;
		break;
	      case 5:
	      default:
		red = i_brightness;
		green = p;
		blue = q;
		break;
	    }
	    colour = (red << info->red_shift) |
		(green << info->green_shift) | (blue << info->blue_shift);
	    /*  Write the computed RGB value into the arrays  */
	    info->rgb_array[pixel] = colour;
	}
    }
}  /*  End Function job_func  */
