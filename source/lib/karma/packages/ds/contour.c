/*LINTLIBRARY*/
/*  contour.c

    This code provides contour extraction routines.

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

    This file contains the various utility routines for extracting contours
    from images in the general data structure supported in Karma.


    Written by      Richard Gooch   20-JUL-1996

    Last updated by Richard Gooch   20-JUL-1996


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>


/*  Private functions  */
STATIC_FUNCTION (void reallocate_coords,
		 (uaddr buf_size, uaddr num_to_copy,
		  double **x0, double **y0, double **x1, double **y1) );


/*  Public functions follow  */

/*EXPERIMENTAL_FUNCTION*/
unsigned int ds_contour (CONST char *image, unsigned int elem_type,
			 CONST dim_desc *hdim, CONST uaddr *hoffsets,
			 CONST dim_desc *vdim, CONST uaddr *voffsets,
			 unsigned int num_contours,
			 CONST double *contour_levels,
			 uaddr *buf_size,
			 double **x0_arr, double **y0_arr,
			 double **x1_arr, double **y1_arr)
/*  [SUMMARY] Extract contours from a 2-dimensional array.
    [PURPOSE] This routine will extract contours from a 2-dimensional
    Intelligent Array, producing a list of line segments that approximate the
    countours. The co-ordinates of the line segments are in linear world
    co-ordinates.
    <image> The start of the image slice data.
    <elem_type> The type of the data.
    <hdim> The horizontal dimension descriptor.
    <hoffsets> The address offsets for data along the horizontal dimension.
    <vdim> The vertical dimension descriptor.
    <voffsets> The address offsets for data along the vertical dimension.
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
    int icase;
    unsigned int xlen, ylen, x, y, contour_count;
    unsigned int seg_count = 0;
    uaddr buf_inc;
    double xpos, ypos, first_x, xdelt, ydelt;
    double val_00, val_10, val_01, val_11;
    double cval, x0, y0, x1, y1;
    CONST char *line1_ptr, *line2_ptr;
    static char function_name[] = "ds_contour";

    if ( (image == NULL) || (hoffsets == NULL) || (voffsets == NULL) )
    {
	fputs ("NULL pointer(s) passed\n", stderr);
	a_prog_bug (function_name);
    }
    if (elem_type != K_FLOAT)
    {
	(void) fprintf (stderr, "Array is not floating point\n");
	a_prog_bug (function_name);
    }
    xlen = hdim->length;
    ylen = vdim->length;
    buf_inc = xlen * ylen / 10;
    if ( (hdim->coordinates != NULL) || (vdim->coordinates != NULL) )
    {
	(void) fprintf (stderr, "Co-ordinate array not supported\n");
	a_prog_bug (function_name);
    }
    if (*buf_size < buf_inc)
    {
	reallocate_coords (buf_inc, 0, x0_arr, y0_arr, x1_arr, y1_arr);
	*buf_size = buf_inc;
    }
    /*  Process image pixel-by-pixel  */
    xdelt = (hdim->last_coord - hdim->first_coord) /
	(double) (hdim->length - 1);
    ydelt = (vdim->last_coord - vdim->first_coord) /
	(double) (vdim->length - 1);
    first_x = hdim->first_coord;  /*Use a variable to reduce memory accesses */
    for (y = 0; y < ylen - 1; ++y)
    {
	ypos = ds_get_coordinate (vdim, y);
	line1_ptr = image + voffsets[y];
	line2_ptr = image + voffsets[y + 1];
	for (x = 0; x < xlen - 1; ++x)
	{
	    xpos = (double) x * xdelt + first_x;
	    val_00 = *(float *) (line1_ptr + hoffsets[x]);
	    val_10 = *(float *) (line1_ptr + hoffsets[x + 1]);
	    val_01 = *(float *) (line2_ptr + hoffsets[x]);
	    val_11 = *(float *) (line2_ptr + hoffsets[x + 1]);
	    /*  Search contour levels for an intersection or two  */
	    for (contour_count = 0; contour_count < num_contours;
		 ++contour_count)
	    {
		cval = contour_levels[contour_count];
		icase = 1;
		if (cval > val_00) icase = icase + 1;
		if (cval > val_10) icase = icase + 2;
		if (cval > val_01) icase = icase + 4;
		if (cval > val_11) icase = 9 - icase;
		if (icase < 2) continue;
		switch (icase)
		{
		  case 2:
		    x0 = xpos +  xdelt * (cval - val_00) / (val_10 - val_00);
		    y0 = ypos;
		    x1 = xpos;
		    y1 = ypos + ydelt * (cval - val_00) / (val_01 - val_00);
		    break;
		  case 3:
		    x0 = xpos + xdelt * (cval - val_00) / (val_10 - val_00);
		    y0 = ypos;
		    x1 = xpos + xdelt;
		    y1 = ypos + ydelt * (cval - val_10) / (val_11 - val_10);
		    break;
		  case 4:
		    x0 = xpos;
		    y0 = ypos + ydelt * (cval - val_00) / (val_01 - val_00);
		    x1 = xpos + xdelt;
		    y1 = ypos + ydelt * (cval - val_10) / (val_11 - val_10);
		    break;
		  case 5:
		    x0 = xpos;
		    y0 = ypos + ydelt * (cval - val_00) / (val_01 - val_00);
		    x1 = xpos + xdelt * (cval - val_01) / (val_11 - val_01);
		    y1 = ypos + ydelt;
		    break;
		  case 6:
		    x0 = xpos + xdelt * (cval - val_00) / (val_10 - val_00);
		    y0 = ypos;
		    x1 = xpos + xdelt * (cval - val_01) / (val_11 - val_01);
		    y1 = ypos + ydelt;
		    break;
		  case 7:
		    x0 = xpos + xdelt * (cval - val_00) / (val_10 - val_00);
		    y0 = ypos;
		    x1 = xpos;
		    y1 = ypos + ydelt * (cval - val_00) / (val_01 - val_00);
		    if (seg_count >= *buf_size)
		    {
			reallocate_coords (*buf_size + buf_inc, *buf_size,
					   x0_arr, y0_arr, x1_arr, y1_arr);
			*buf_size += buf_inc;
		    }
		    (*x0_arr)[seg_count] = x0;
		    (*y0_arr)[seg_count] = y0;
		    (*x1_arr)[seg_count] = x1;
		    (*y1_arr)[seg_count] = y1;
		    ++seg_count;
		    x0 = xpos + xdelt * (cval - val_01) / (val_11 - val_01);
		    y0 = ypos + ydelt;
		    x1 = xpos + xdelt;
		    y1 = ypos + ydelt * (cval - val_10) / (val_11 - val_10);
		    break;
		  case 8:
		    x0 = xpos + xdelt * (cval - val_01) / (val_11 - val_01);
		    y0 = ypos + ydelt;
		    x1 = xpos + xdelt;
		    y1 = ypos + ydelt * (cval - val_10) / (val_11 - val_10);
		    break;
		  default:
		    continue;
		}
		if (seg_count >= *buf_size)
		{
		    reallocate_coords (*buf_size + buf_inc, *buf_size,
				       x0_arr, y0_arr, x1_arr, y1_arr);
		    *buf_size += buf_inc;
		}
		(*x0_arr)[seg_count] = x0;
		(*y0_arr)[seg_count] = y0;
		(*x1_arr)[seg_count] = x1;
		(*y1_arr)[seg_count] = y1;
		++seg_count;
	    }
	}
    }
    return (seg_count);
}   /*  End Function ds_contour  */


/*  Private functions follow  */

static void reallocate_coords (uaddr buf_size, uaddr num_to_copy,
			       double **x0, double **y0,
			       double **x1, double **y1)
/*  [SUMMARY] Reallocate co-ordinate buffers.
    <buf_size> The new desired buffer size.
    <num_to_copy> The number of values to copy from the old buffers.
    <x0> A pointer to a co-ordinate array pointer. The co-ordinate array may be
    internally reallocated, hence the array pointer may be modified.
    <y0> A pointer to a co-ordinate array pointer. The co-ordinate array may be
    internally reallocated, hence the array pointer may be modified.
    <x1> A pointer to a co-ordinate array pointer. The co-ordinate array may be
    internally reallocated, hence the array pointer may be modified.
    <y1> A pointer to a co-ordinate array pointer. The co-ordinate array may be
    internally reallocated, hence the array pointer may be modified.
    [RETURNS] Nothing. On failure the process aborts.
*/
{
    double *ptr;
    static char function_name[] = "iaray_contour__reallocate_coords";

    if (num_to_copy < 1)
    {
	if (*x0 != NULL) m_free ( (char *) *x0 );
	if (*x0 != NULL) m_free ( (char *) *y0 );
	if (*x0 != NULL) m_free ( (char *) *x1 );
	if (*x0 != NULL) m_free ( (char *) *y1 );
	if ( ( *x0 = (double *) m_alloc (sizeof **x0 * buf_size) ) == NULL )
	{
	    m_abort (function_name, "x0 array");
	}
	if ( ( *y0 = (double *) m_alloc (sizeof **y0 * buf_size) ) == NULL )
	{
	    m_abort (function_name, "y0 array");
	}
	if ( ( *x1 = (double *) m_alloc (sizeof **x1 * buf_size) ) == NULL )
	{
	    m_abort (function_name, "x1 array");
	}
	if ( ( *y1 = (double *) m_alloc (sizeof **y1 * buf_size) ) == NULL )
	{
	    m_abort (function_name, "y1 array");
	}
	return;
    }
    /*  Pity we have to do this  */
    /*  Copy x0  */
    if ( ( ptr = (double *) m_alloc (sizeof *ptr * buf_size) ) == NULL )
    {
	m_abort (function_name, "x0 array");
    }
    m_copy ( (char *) ptr, (char *) *x0, sizeof *ptr * num_to_copy );
    m_free ( (char *) *x0 );
    *x0 = ptr;
    /*  Copy y0  */
    if ( ( ptr = (double *) m_alloc (sizeof *ptr * buf_size) ) == NULL )
    {
	m_abort (function_name, "y0 array");
    }
    m_copy ( (char *) ptr, (char *) *y0, sizeof *ptr * num_to_copy );
    m_free ( (char *) *y0 );
    *y0 = ptr;
    /*  Copy x1  */
    if ( ( ptr = (double *) m_alloc (sizeof *ptr * buf_size) ) == NULL )
    {
	m_abort (function_name, "x1 array");
    }
    m_copy ( (char *) ptr, (char *) *x1, sizeof *ptr * num_to_copy );
    m_free ( (char *) *x1 );
    *x1 = ptr;
    /*  Copy y1  */
    if ( ( ptr = (double *) m_alloc (sizeof *ptr * buf_size) ) == NULL )
    {
	m_abort (function_name, "y1 array");
    }
    m_copy ( (char *) ptr, (char *) *y1, sizeof *ptr * num_to_copy );
    m_free ( (char *) *y1 );
    *y1 = ptr;
}   /*  End Function reallocate_coords  */
