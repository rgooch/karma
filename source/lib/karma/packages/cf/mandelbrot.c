/*LINTLIBRARY*/
/*  mandelbrot.c

    This code provides simple colourmap generation routines.

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

    This file contains the various utility routines for writing simple colour
    values.


    Written by      Richard Gooch   31-JAN-1995

    Updated by      Richard Gooch   31-JAN-1995

    Last updated by Richard Gooch   20-APR-1995: Cosmetic changes.


*/

#include <stdio.h>
#include <math.h>
#include <karma_cf.h>
#include <karma_a.h>
#include <karma_n.h>
#include <karma.h>

#define MAX_INTENSITY 65535

/*PUBLIC_FUNCTION*/
void cf_mandelbrot (unsigned int num_cells, unsigned short *reds,
		    unsigned short *greens, unsigned short *blues,
		    unsigned int stride, double x, double y,
		    void *var_param)
/*  [PURPOSE] This routine will compute a mandelbrot colourmap.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values.
    <greens> The green intensity values.
    <blues> The blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values.
    <y> A parameter used to compute the colour values.
    <var_param> A parameter used to compute the colour values.
    [RETURNS] Nothing.
*/
{
    unsigned int pixel_count, num_cells1;
    int block_size, index;
    float iscale;
    static char function_name[] = "cf_mandelbrot";

    block_size = num_cells / (6 * 4);
    num_cells1 = block_size * (6 * 4);
    iscale = (float) MAX_INTENSITY / (float) block_size;
    /*  Now compute the colours  */
    for (pixel_count = 0; pixel_count < num_cells1; ++pixel_count)
    {
	index = pixel_count % block_size;
	switch (pixel_count / block_size)
	{
	  case 0:
	  case 6:
	  case 12:
	  case 18:
	    reds[pixel_count * stride] = MAX_INTENSITY;
	    greens[pixel_count * stride] = (iscale * (float) index);
	    blues[pixel_count * stride] = 0;
	    break;
	  case 1:
	  case 7:
	  case 13:
	  case 19:
	    reds[pixel_count * stride] = MAX_INTENSITY - (iscale * (float) index);
	    greens[pixel_count * stride] = MAX_INTENSITY;
	    blues[pixel_count * stride] = 0;
	    break;
	  case 2:
	  case 8:
	  case 14:
	  case 20:
	    reds[pixel_count * stride] = 0;
	    greens[pixel_count * stride] = MAX_INTENSITY;
	    blues[pixel_count * stride] = (iscale * (float) index);
	    break;
	  case 3:
	  case 9:
	  case 15:
	  case 21:
	    reds[pixel_count * stride] = 0;
	    greens[pixel_count * stride] = MAX_INTENSITY - (iscale * (float) index);
	    blues[pixel_count * stride] = MAX_INTENSITY;
	    break;
	  case 4:
	  case 10:
	  case 16:
	  case 22:
	    reds[pixel_count * stride] = (iscale * (float) index);
	    greens[pixel_count * stride] = 0;
	    blues[pixel_count * stride] = MAX_INTENSITY;
	    break;
	  case 5:
	  case 11:
	  case 17:
	  case 23:
	    reds[pixel_count * stride] = MAX_INTENSITY;
	    greens[pixel_count * stride] = 0;
	    blues[pixel_count * stride] = MAX_INTENSITY - (iscale * (float) index);
	    break;
	  default:
	    (void) fprintf (stderr, "Bad  %d\n", pixel_count % block_size);
	    a_prog_bug (function_name);
	    break;
	}
    }
    /*  Put in the black pixels  */
    for (; pixel_count < num_cells; ++pixel_count)
    {
	reds[pixel_count * stride] = 0;
	greens[pixel_count * stride] = 0;
	blues[pixel_count * stride] = 0;
    }
}   /*  End Function cf_mandelbrot  */
