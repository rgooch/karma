/*LINTLIBRARY*/
#define DEBUG
/*  contour.c

    This code provides contour extraction from Intelligent Arrays.

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

/*  This file contains all routines needed for the simple processing of
  n-dimensional data structures.


    Written by      Richard Gooch   18-JUL-1996

    Last updated by Richard Gooch   20-JUL-1996: Made use of <ds_contour>


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>


#define VERIFY_IARRAY(array) if (array == NULL) \
{(void) fprintf (stderr, "NULL iarray passed\n"); \
 a_prog_bug (function_name); }
#ifdef dummy
if (array->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid iarray\n"); \
 a_prog_bug (function_name); }
#endif

/*  Private functions  */


/*  Public functions follow  */

/*EXPERIMENTAL_FUNCTION*/
unsigned int iarray_contour (iarray array, unsigned int num_contours,
			     CONST double *contour_levels,
			     uaddr *buf_size,
			     double **x0_arr, double **y0_arr,
			     double **x1_arr, double **y1_arr)
/*  [SUMMARY] Extract contours from a 2-dimensional Intelligent Array.
    [PURPOSE] This routine will extract contours from a 2-dimensional
    Intelligent Array, producing a list of line segments that approximate the
    countours. The co-ordinates of the line segments are in linear world
    co-ordinates.
    <array> The array.
    <num_contours> The number of contour levels.
    <contour_levels> The array of contour levels.
    <buf_size> A pointer to the size of the co-ordinate arrays. This is
    modified.
    <x0_arr> A pointer to a co-ordinate array pointer. The co-ordinate array
    may be internally reallocated, hence the array pointer may be modified.
    <y0_arr> A pointer to a co-ordinate array pointer. The co-ordinate array
    may be internally reallocated, hence the array pointer may be modified.
    <x1_arr> A pointer to a co-ordinate array pointer. The co-ordinate array
    may be internally reallocated, hence the array pointer may be modified.
    <y1_arr> A pointer to a co-ordinate array pointer. The co-ordinate array
    may be internally reallocated, hence the array pointer may be modified.
    [RETURNS] The number of line segments extracted.
*/
{
    dim_desc *xdim, *ydim;
    static char function_name[] = "iarray_contour";

    VERIFY_IARRAY (array);
    if (iarray_type (array) != K_FLOAT)
    {
	(void) fprintf (stderr, "Intelligent array is not floating point\n");
	a_prog_bug (function_name);
    }
    xdim = iarray_get_dim_desc (array, 1);
    ydim = iarray_get_dim_desc (array, 0);
    return ( ds_contour (array->data, iarray_type (array),
			 xdim, array->offsets[1], ydim, array->offsets[0],
			 num_contours, contour_levels,
			 buf_size, x0_arr, y0_arr, x1_arr, y1_arr) );
}   /*  End Function iarray_contour  */
