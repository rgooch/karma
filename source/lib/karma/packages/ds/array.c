/*LINTLIBRARY*/
/*  array.c

    This code provides array manipulation routines.

    Copyright (C) 1992,1993,1994  Richard Gooch

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

    This file contains the various utility routines for manipulating arrays
    in the general data structure supported in Karma.


    Written by      Richard Gooch   20-SEP-1992

    Updated by      Richard Gooch   3-DEC-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   23-OCT-1994: Create
  ds_find_single_histogram  routine.

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/ds/array.c


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>


/*PUBLIC_FUNCTION*/
flag ds_find_single_extremes (char *data, unsigned int elem_type,
			      unsigned int conv_type, dim_desc *dimension,
			      unsigned int stride, double scan_start,
			      double scan_end, double *min, double *max)
/*  This routine will find the extremes (minimum and maximum) of a single trace
    (element versus a dimension).
    The data must be pointed to by  data  .
    The type of the element must be in  elem_type  .
    The type of conversion to use for complex numbers must be in  conv_type  .
    The dimension descriptor must be pointed to by  dimension  .
    The stride of the elements in memory must be in  stride  .
    The starting and ending co-ordinate values to scan between must be in
    scan_start  and  scan_end  ,respectively.
    The minimum value will be written to the storage pointed to by  min  .
    The maximum value will be written to the storage pointed to by  max  .
    The minimum and maximum value must be initialised to a very large positive
    number and a very large negative number, respectively, outside of the
    routine. In other words, the routine does not initialise these values prior
    to testing for the minimum and maximum.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int start_coord;
    unsigned int end_coord;
    unsigned int num_values;
    unsigned int value_count;
    double value;
    flag complex = FALSE;
    double *val;
    static unsigned int value_buf_len = 0;
    static double *values = NULL;
    static char function_name[] = "ds_find_single_extremes";

    if ( (data == NULL) || (dimension == NULL) ||
	(min == NULL) || (max == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (scan_start < (*dimension).minimum)
    {
	scan_start = (*dimension).minimum;
    }
    if (scan_end > (*dimension).maximum)
    {
	scan_end = (*dimension).maximum;
    }
    start_coord = ds_get_coord_num (dimension, scan_start, SEARCH_BIAS_UPPER);
    end_coord = ds_get_coord_num (dimension, scan_end, SEARCH_BIAS_LOWER);
    num_values = end_coord - start_coord + 1;
    if (num_values > value_buf_len)
    {
	/*  Need to allocate temporary values array  */
	if (values != NULL)
	{
	    m_free ( (char *) values );
	}
	if ( ( values = (double *) m_alloc (sizeof *values * 2 * num_values) )
	    == NULL )
	{
	    value_buf_len = 0;
	    m_error_notify (function_name, "temporary array");
	    return (FALSE);
	}
	value_buf_len = num_values;
    }
    data += start_coord * stride;
    /*  Convert data  */
    if (ds_get_elements (data, elem_type, stride, values, &complex, num_values)
	!= TRUE)
    {
	return (FALSE);
    }
    for (value_count = 0, val = values; value_count < num_values;
	 ++value_count, val += 2)
    {
	if (complex == TRUE)
	{
	    /*  Complex conversion  */
	    switch (conv_type)
	    {
	      case CONV1_REAL:
		value = val[0];
		break;
	      case CONV1_IMAG:
		value = val[1];
		break;
	      case CONV1_ABS:
	      case CONV1_ENVELOPE:
		value = sqrt (val[0] * val[0] + val[1] * val[1]);
		break;
	      case CONV1_SQUARE_ABS:
		value = val[0] * val[0] + val[1] * val[1];
		break;
	      case CONV1_PHASE:
		value = atan2 (val[0], val[1]) / PION180;
		break;
	      case CONV1_CONT_PHASE:
		(void) fprintf (stderr,
				"Continuous phase not implemented yet\n");
		return (FALSE);
/*
		break;
*/
	      default:
		(void) fprintf (stderr,
				"Bad value of conversion type: %u\n",
				conv_type);
		a_prog_bug (function_name);
		break;
	    }
	}
	else
	{
	    value = val[0];
	}
	if (value >= TOOBIG)
	{
	    /*  Hole in data: skip  */
	    continue;
	}
	if (value < *min)
	{
	    *min = value;
	}
	if (value > *max)
	{
	    *max = value;
	}
	if ( (complex == TRUE) && (conv_type == CONV1_ENVELOPE) )
	{
	    /*  Do again for negative part  */
	    if (-value < *min)
	    {
		*min = -value;
	    }
	    if (-value > *max)
	    {
		*max = -value;
	    }
	}
    }
    return (TRUE);
}   /*  End Function ds_find_single_extremes  */

/*PUBLIC_FUNCTION*/
flag ds_find_plane_extremes (char *data, unsigned int elem_type,
			     unsigned int conv_type, dim_desc *abs_dim_desc,
			     unsigned int abs_dim_stride,
			     dim_desc *ord_dim_desc,
			     unsigned int ord_dim_stride,
			     double abs_scan_start, double abs_scan_end,
			     double ord_scan_start, double ord_scan_end,
			     double *min, double *max)
/*  This routine will find the extremes (minimum and maximum) of a single plane
    (element versus two dimensions).
    The data must be pointed to by  data  .
    The type of the element must be in  elem_type  .
    The type of conversion to use for complex numbers must be in  conv_type  .
    The abscissa dimension descriptor must be pointed to by  abs_dim_desc  .
    The abscissa dimension stride of the elements in memory must be in
    abs_dim_stride  .
    The ordinate dimension descriptor must be pointed to by  ord_dim_desc  .
    The ordinate dimension stride of the elements in memory must be in
    ord_dim_stride  .
    The starting and ending abscissa co-ordinate values to scan between must be
    in  abs_scan_start  and  abs_scan_end  ,respectively.
    The starting and ending ordinate co-ordinate values to scan between must be
    in  ord_scan_start  and  ord_scan_end  ,respectively.
    The minimum value will be written to the storage pointed to by  min  .
    The maximum value will be written to the storage pointed to by  max  .
    The minimum and maximum value must be initialised to a very large positive
    number and a very large negative number, respectively, outside of the
    routine. In other words, the routine does not initialise these values prior
    to testing for the minimum and maximum.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int ord_start_coord;
    unsigned int ord_end_coord;
    unsigned int ord_coord_num;
    static char function_name[] = "ds_find_plane_extremes";

    if ( (data == NULL) || (abs_dim_desc == NULL) || (ord_dim_desc == NULL) ||
	(min == NULL) || (max == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    ord_start_coord = ds_get_coord_num (ord_dim_desc, ord_scan_start,
					SEARCH_BIAS_UPPER);
    ord_end_coord = ds_get_coord_num (ord_dim_desc, ord_scan_end,
				      SEARCH_BIAS_LOWER);
    data += ord_start_coord * ord_dim_stride;
    for (ord_coord_num = ord_start_coord; ord_coord_num <= ord_end_coord;
	 ++ord_coord_num, data += ord_dim_stride)
    {
	/*  Get extremes along abscissa dimension  */
	if (ds_find_single_extremes (data, elem_type, conv_type,
				     abs_dim_desc, abs_dim_stride,
				     abs_scan_start, abs_scan_end, min, max)
	    != TRUE)
	{
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function ds_find_plane_extremes  */

/*PUBLIC_FUNCTION*/
flag ds_find_single_histogram (char *data, unsigned int elem_type,
			       unsigned int conv_type, unsigned int num_values,
			       uaddr *offsets, unsigned int stride,
			       double min, double max, unsigned long num_bins,
			       unsigned long *histogram_array,
			       unsigned long *histogram_peak,
			       unsigned long *histogram_mode)
/*  This routine will find the histogram of a single trace (element versus a
    dimension). This routine may be called repeatedly with multiple traces in
    order to build an aggregate histogram of all traces.
    The data must be pointed to by  data  .Misaligned data will cause bus
    errors on some platforms.
    The type of the element must be in  elem_type  .
    The type of conversion to use for complex numbers must be in  conv_type  .
    Legal value for this include:
        KIMAGE_COMPLEX_CONV_REAL        KIMAGE_COMPLEX_CONV_IMAG
        KIMAGE_COMPLEX_CONV_ABS         KIMAGE_COMPLEX_CONV_SQUARE_ABS
	KIMAGE_COMPLEX_CONV_PHASE       KIMAGE_COMPLEX_CONV_CONT_PHASE
    The length of the dimension must be given by  num_values  .
    The address offsets for data along the dimension must be pointed to by
    offsets  .
    If the value of  offsets  is NULL, the stride (in bytes) between
    consecutive values along the dimension must be given by  stride  .
    Data values below the value  min  will be ignored.
    Data values above the value  max  will be ignored.
    The number of histogram bins must be given by  num_bins  .
    The histogram array must be pointed to by  historgram_array  .The values in
    this array are updated, and hence must be initialised externally.
    The peak of the histogram is written to the storage pointed to by
    histogram_peak  .This value is updated, and hence must be externally
    initialised to 0.
    The mode of the histogram (index value of the peak) will be written to the
    storage pointed to by  histogram_mode  .This value is updated, and hence
    must be externally initialised to 0.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int value_count;
    unsigned long index, hval, hpeak, hmode;
    double value, bin_factor;
    double toobig = TOOBIG;
    flag complex = FALSE;
    double *val;
    static unsigned int value_buf_len = 0;
    static double *values = NULL;
    static char function_name[] = "ds_find_single_histogram";

    if ( (data == NULL) || (histogram_array == NULL) ||
	(histogram_peak == NULL) || (histogram_mode == NULL ) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    hpeak = *histogram_peak;
    hmode = *histogram_mode;
    if (min >= max)
    {
	(void) fprintf (stderr, "min: %e is not less than max: %e\n",
			min, max);
	a_prog_bug (function_name);
    }
    if (num_values > value_buf_len)
    {
	/*  Need to allocate temporary values array  */
	if (values != NULL)
	{
	    m_free ( (char *) values );
	}
	if ( ( values = (double *) m_alloc (sizeof *values * 2 * num_values) )
	    == NULL )
	{
	    value_buf_len = 0;
	    m_error_notify (function_name, "temporary array");
	    return (FALSE);
	}
	value_buf_len = num_values;
    }
    /*  Convert data  */
    if (offsets == NULL)
    {
	if ( !ds_get_elements (data, elem_type, stride, values, &complex,
			       num_values) )
	{
	    return (FALSE);
	}
    }
    else
    {
	if ( !ds_get_scattered_elements (data, elem_type, offsets, values,
					 &complex, num_values) )
	{
	    return (FALSE);
	}
    }
    bin_factor = (double) (num_bins - 1) / (max - min);
    for (value_count = 0, val = values; value_count < num_values;
	 ++value_count, val += 2)
    {
	if (complex)
	{
	    /*  Complex conversion  */
	    switch (conv_type)
	    {
	      case KIMAGE_COMPLEX_CONV_REAL:
		value = val[0];
		break;
	      case KIMAGE_COMPLEX_CONV_IMAG:
		value = val[1];
		break;
	      case KIMAGE_COMPLEX_CONV_ABS:
		value = sqrt (val[0] * val[0] + val[1] * val[1]);
		break;
	      case KIMAGE_COMPLEX_CONV_SQUARE_ABS:
		value = val[0] * val[0] + val[1] * val[1];
		break;
	      case KIMAGE_COMPLEX_CONV_PHASE:
		value = atan2 (val[0], val[1]) / PION180;
		break;
	      case KIMAGE_COMPLEX_CONV_CONT_PHASE:
		(void) fprintf (stderr,
				"Continuous phase not implemented yet\n");
		return (FALSE);
/*
		break;
*/
	      default:
		(void) fprintf (stderr,
				"Bad value of conversion type: %u\n",
				conv_type);
		a_prog_bug (function_name);
		break;
	    }
	}
	else
	{
	    value = val[0];
	}
	if (value >= toobig) continue;
	if (value < min) continue;
	if (value > max) continue;
	/*  Convert value to bin number  */
	index = (unsigned int) ( (value - min) * bin_factor );
	hval = ++histogram_array[index];
	if (hval > hpeak)
	{
	    hpeak = hval;
	    hmode = index;
	}
    }
    *histogram_peak = hpeak;
    *histogram_mode = hmode;
    return (TRUE);
}   /*  End Function ds_find_single_histogram  */
