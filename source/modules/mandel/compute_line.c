/*  compute_line.c

    Compute source file for  mandel  (Mandelbrot generator module).

    Copyright (C) 1993,1994  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This Karma module will compute the Mandelbrot set.


    Written by      Richard Gooch   9-OCT-1993

    Updated by      Richard Gooch   16-OCT-1993

    Updated by      Richard Gooch   29-NOV-1993: Removed
  #include <c_varieties.h>

    Updated by      Richard Gooch   6-AUG-1994: Moved to full ASCII function
  declaration syntax. Had to do this because of the confused Dec Alpha compiler

    Last updated by Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>


EXTERN_FUNCTION (int compute_x,
		 (unsigned char *data, int num_iterations, float cy,
		  float x_min, int x_pixels, float x_scale, int stride,
		  int num_colours) );


#ifndef OS_VXMVX
int compute_x (unsigned char *data, int num_iterations, float cy,
	       float x_min, int x_pixels, float x_scale, int stride,
	       int num_colours)
/*  [PURPOSE] This routine will compute a line in the complex plane of the
    Mandelbrot set.
    <data> The line data.
    <num_iterations> The number of iterations to perform.
    <cy> The y (imaginary) co-ordinate of the line.
    <x_pixels> The length of the line in pixels.
    <cx> An array of x (real) co-ordinates.
    <stride> The stride (in bytes) between pixels.
    <num_colours> The number of colours (not including black).
    [RETURNS] The number of floating point operations performed.
*/
{
    int x_index;
    register int iter_count;
    register int flop_count = 0;
    register float cx;
    register float r, i;
    register float xx, yy, xy2;
    static float four = 4.0;

    for (x_index = 0; x_index < x_pixels; ++x_index, data += stride)
    {
	cx = x_min + x_scale * (float) x_index;
	/*  Compute one pixel  */
	r = cx;
	i = cy;
	flop_count += 2;
	for (iter_count = 0; iter_count < num_iterations; ++iter_count)
	{
	    xx = r * r;
	    yy = i * i;
	    xy2 = r * i;
	    xy2 += xy2;
	    flop_count += 6;
	    if (xx + yy >= four) break;
	    r = xx - yy + cx;
	    i = xy2 + cy;
	    flop_count += 3;
	}
	*data = (iter_count < num_iterations) ? (iter_count % num_colours) : num_colours;
    }
    return (flop_count);
}   /*  End Function compute_x  */
#endif
