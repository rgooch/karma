/*LINTLIBRARY*/
/*  main.c

    This code provides basic colour conversion support.

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

    This file provides basic colour conversion support.


    Written by      Richard Gooch   29-DEC-1995

    Last updated by Richard Gooch   29-DEC-1995


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
#include <karma_a.h>
#include <karma_m.h>
#include <karma_c.h>

/*PUBLIC_FUNCTION*/
void col_hsb_slice_to_rgb_array (unsigned long rgb_array[65536],
				 unsigned long red_mask,
				 unsigned long green_mask,
				 unsigned long blue_mask,
				 float min_brightness, float max_brightness,
				 float start_hue, float stop_hue,
				 float saturation)
/*  [PURPOSE] This routine will convert a slice in HSB (Hue, Saturation and
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
    [RETURNS] Nothing.
*/
{
    int index1, index2;
    int pixel;
    int colour;
    int i_hue, i_brightness;
    int red, green, blue;
    int p, q, t;
    int red_shift, green_shift, blue_shift;
    float hue;
    float brightness;
    float brightness_scale, hue_scale;
    float f;
    float one = 1.0;
    static char function_name[] = "col_hsb_slice_to_rgb_array";

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
    (void) fprintf (stderr, "%s: minb: %e  maxb: %e\nstarth: %e  stoph: %e\n",
		    function_name,
		    min_brightness, max_brightness, start_hue, stop_hue);
    (void) fprintf (stderr, "sat: %e\n", saturation);
    /*  Compute the (left) bit shifts for each mask  */
    for (red_shift = 0; !(red_mask & 1); red_mask = red_mask >> 1,++red_shift);
    if (red_mask != 0xff)
    {
	(void) fprintf (stderr, "Shifted red_mask: %lx is not 0xff\n",
			red_mask);
	a_prog_bug (function_name);
    }
    for (green_shift = 0; !(green_mask & 1);
	 green_mask = green_mask >> 1, ++green_shift);
    if (green_mask != 0xff)
    {
	(void) fprintf (stderr, "Shifted green_mask: %lx is not 0xff\n",
			green_mask);
	a_prog_bug (function_name);
    }
    for (blue_shift = 0; !(blue_mask & 1);
	 blue_mask = blue_mask >> 1, ++blue_shift);
    if (blue_mask != 0xff)
    {
	(void) fprintf (stderr, "Shifted blue_mask: %lx is not 0xff\n",
			blue_mask);
	a_prog_bug (function_name);
    }
    (void) fprintf (stderr, "red_s: %d  green_s: %d  blue_s: %d\n",
		    red_shift, green_shift, blue_shift);
    /*  Loop through the brightness range  */
    /*  <pixel> will go: 0, 1, 2, 3, ..., 255, 256, 257, ...  */
    brightness_scale = max_brightness - min_brightness;
    min_brightness *= 255.0;
    hue_scale = (stop_hue - start_hue) / 255.0;
    for (index1 = 0, pixel = 0; index1 < 256; ++index1)
    {
	/*  Compute brightness from co-ordinate  */
	brightness = min_brightness + (float) index1 * brightness_scale;
	i_brightness = brightness;  /*  Truncate to integer  */
	p = ( brightness * (one - saturation) );
	/*  Loop through the hue range  */
	for (index2 = 0; index2 < 256; ++index2, ++pixel)
	{
	    /*  Compute hue from co-ordinate  */
	    hue = start_hue + hue_scale * (float) index2;
	    i_hue = hue;  /*  Truncate to integer  */
	    /*  Compute RGB value from HSB value  */
	    f = hue - (float) i_hue;  /*  The fractional hue component  */
	    q = ( brightness * ( one - (saturation * f) ) );
	    t = ( brightness * ( one - ( saturation * (one - f) ) ) );
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
		red = i_brightness;
		green = p;
		blue = q;
		break;
	    }
	    colour = (red << red_shift) | (green <<
					   green_shift) | (blue << blue_shift);
	    /*  Write the computed RGB value into the arrays  */
	    rgb_array[pixel] = colour;
	}
    }
}  /*  End Function col_hsb_slice_to_rgb_array  */
