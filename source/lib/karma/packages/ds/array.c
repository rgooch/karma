/*LINTLIBRARY*/
/*  array.c

    This code provides array manipulation routines.

    Copyright (C) 1992-1996  Richard Gooch

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

    Updated by      Richard Gooch   23-OCT-1994: Created
  ds_find_single_histogram  routine.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ds/array.c

    Updated by      Richard Gooch   28-JAN-1995: Declared routines MT-Unsafe.

    Updated by      Richard Gooch   7-APR-1995: Changed to CONV_CtoR_*.

    Updated by      Richard Gooch   9-APR-1995: Created <ds_complex_to_real_1D>

    Updated by      Richard Gooch   19-APR-1995: Cleaned some code.

    Last updated by Richard Gooch   24-JAN-1996: Created <ds_find_1D_extremes>
  and <ds_find_2D_extremes>.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>

#define BLOCK_SIZE 1024


/*PUBLIC_FUNCTION*/
flag ds_find_1D_extremes (CONST char *data,
			  unsigned int num_values, uaddr *offsets,
			  unsigned int elem_type, unsigned int conv_type,
			  double *min, double *max)
/*  [PURPOSE] This routine will find the extremes (minimum and maximum) of a
    single trace (element versus a dimension).
    <data> A pointer to the data.
    <num_values> The number of values to process.
    <offsets> The address offsets for data along the dimension.
    <elem_type> The type of the element.
    <conv_type> The type of conversion to use for complex numbers.
    <min> The minimum value will be written here.
    <max> The maximum value will be written here.
    [NOTE] The minimum and maximum value must be initialised to a very large
    positive number and a very large negative number, respectively, outside of
    the routine. In other words, the routine does not initialise these values
    prior to testing for the minimum and maximum.
    [MT-LEVEL] Safe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int block_size;
    unsigned int value_count;
    double value = 0.0;  /*  Initialised to keep compiler happy  */
    double min_val, max_val;
    flag complex = FALSE;
    double *val;
    double values[2 * BLOCK_SIZE];
    static char function_name[] = "ds_find_single_extremes";

    if ( (data == NULL) || (min == NULL) || (max == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    min_val = *min;
    max_val = *max;
    /*  Loop over blocks  */
    while (num_values > 0)
    {
	block_size = (num_values > BLOCK_SIZE) ? BLOCK_SIZE : num_values;
	/*  Convert data  */
	if ( !ds_get_scattered_elements (data, elem_type, offsets, values,
					 &complex,block_size) ) return (FALSE);
	for (value_count = 0, val = values; value_count < block_size;
	     ++value_count, val += 2)
	{
	    if (complex)
	    {
		/*  Complex conversion  */
		switch (conv_type)
		{
		  case CONV_CtoR_REAL:
		    value = val[0];
		    break;
		  case CONV_CtoR_IMAG:
		    value = val[1];
		    break;
		  case CONV_CtoR_ABS:
		  case CONV_CtoR_ENVELOPE:
		    value = sqrt (val[0] * val[0] + val[1] * val[1]);
		    break;
		  case CONV_CtoR_SQUARE_ABS:
		    value = val[0] * val[0] + val[1] * val[1];
		    break;
		  case CONV_CtoR_PHASE:
		    value = atan2 (val[0], val[1]) / PION180;
		    break;
		  case CONV_CtoR_CONT_PHASE:
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
	    if (value < min_val) min_val = value;
	    if (value > max_val) max_val = value;
	    if ( complex && (conv_type == CONV_CtoR_ENVELOPE) )
	    {
		/*  Do again for negative part  */
		if (-value < min_val) min_val = -value;
		if (-value > max_val) max_val = -value;
	    }
	}
	num_values -= block_size;
	offsets += block_size;
    }
    *min = min_val;
    *max = max_val;
    return (TRUE);
}   /*  End Function ds_find_1D_extremes  */

/*PUBLIC_FUNCTION*/
flag ds_find_2D_extremes (CONST char *data,
			  unsigned int length1, uaddr *offsets1,
			  unsigned int length2, uaddr *offsets2,
			  unsigned int elem_type, unsigned int conv_type,
			  double *min, double *max)
/*  [PURPOSE] This routine will find the extremes (minimum and maximum) of a
    single plane (element versus two dimensions).
    <data> A pointer to the data.
    <length1> The number of values to process along one of the dimensions. For
    efficiency this should be the more significant dimension.
    <offsets1> The address offsets for data along the corresponding dimension.
    <length2> The number of values to process along the other of the dimensions
    <offsets2> The address offsets for data along the corresponding dimension.
    <elem_type> The type of the element.
    <conv_type> The type of conversion to use for complex numbers.
    <min> The minimum value will be written here.
    <max> The maximum value will be written here.
    [NOTE] The minimum and maximum value must be initialised to a very large
    positive number and a very large negative number, respectively, outside of
    the routine. In other words, the routine does not initialise these values
    prior to testing for the minimum and maximum.
    [MT-LEVEL] Safe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int count;

    for (count = 0; count < length1; ++count)
    {
	if ( !ds_find_1D_extremes (data + offsets1[count], length2, offsets2,
				   elem_type, conv_type,
				   min, max) ) return (FALSE);
    }
    return (TRUE);
}   /*  End Function ds_find_2D_extremes  */

/*OBSOLETE_FUNCTION*/
flag ds_find_single_extremes (char *data, unsigned int elem_type,
			      unsigned int conv_type, dim_desc *dimension,
			      unsigned int stride, double scan_start,
			      double scan_end, double *min, double *max)
/*  [PURPOSE] This routine will find the extremes (minimum and maximum) of a
    single trace (element versus a dimension).
    <data> A pointer to the data.
    <elem_type> The type of the element.
    <conv_type> The type of conversion to use for complex numbers.
    <dimension> The dimension descriptor.
    <stride> The stride of the elements in memory.
    <scan_start> The starting co-ordinate to scan from.
    <scan_end> The ending co-ordinate to scan to.
    <min> The minimum value will be written here.
    <max> The maximum value will be written here.
    [NOTE] The minimum and maximum value must be initialised to a very large
    positive number and a very large negative number, respectively, outside of
    the routine. In other words, the routine does not initialise these values
    prior to testing for the minimum and maximum.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int start_coord;
    unsigned int end_coord;
    unsigned int num_values;
    unsigned int value_count;
    double value = 0.0;  /*  Initialised to keep compiler happy  */
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
    (void) fprintf (stderr,
		    "Function: <%s> will be removed in Karma version 2.0\n",
		    function_name);
    (void) fprintf (stderr, "Use: <ds_find_1D_extremes> instead.\n");
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
    if ( !ds_get_elements (data, elem_type, stride, values, &complex,
			   num_values) ) return (FALSE);
    for (value_count = 0, val = values; value_count < num_values;
	 ++value_count, val += 2)
    {
	if (complex)
	{
	    /*  Complex conversion  */
	    switch (conv_type)
	    {
	      case CONV_CtoR_REAL:
		value = val[0];
		break;
	      case CONV_CtoR_IMAG:
		value = val[1];
		break;
	      case CONV_CtoR_ABS:
	      case CONV_CtoR_ENVELOPE:
		value = sqrt (val[0] * val[0] + val[1] * val[1]);
		break;
	      case CONV_CtoR_SQUARE_ABS:
		value = val[0] * val[0] + val[1] * val[1];
		break;
	      case CONV_CtoR_PHASE:
		value = atan2 (val[0], val[1]) / PION180;
		break;
	      case CONV_CtoR_CONT_PHASE:
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
	if ( complex && (conv_type == CONV_CtoR_ENVELOPE) )
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

/*OBSOLETE_FUNCTION*/
flag ds_find_plane_extremes (char *data, unsigned int elem_type,
			     unsigned int conv_type, dim_desc *abs_dim_desc,
			     unsigned int abs_dim_stride,
			     dim_desc *ord_dim_desc,
			     unsigned int ord_dim_stride,
			     double abs_scan_start, double abs_scan_end,
			     double ord_scan_start, double ord_scan_end,
			     double *min, double *max)
/*  [PURPOSE] This routine will find the extremes (minimum and maximum) of a
    single plane (element versus two dimensions).
    <data> A pointer to the data.
    <elem_type> The type of the element.
    <conv_type> The type of conversion to use for complex numbers.
    <abs_dim_desc> The abscissa dimension descriptor.
    <abs_dim_stride> The abscissa dimension stride of the elements in memory.
    <ord_dim_desc> The ordinate dimension descriptor.
    <ord_dim_stride> The ordinate dimension stride of the elements in memory.
    <abs_scan_start> The starting abscissa co-ordinate to scan from.
    <abs_scan_end> The ending abscissa co-ordinate to scan to.
    <ord_scan_start> The starting ordinate co-ordinate to scan from.
    <ord_scan_end> The ending ordinate co-ordinate to scan to.
    <min> The minimum value will be written here.
    <max> The maximum value will be written here.
    [NOTE] The minimum and maximum value must be initialised to a very large
    positive number and a very large negative number, respectively, outside of
    the routine. In other words, the routine does not initialise these values
    prior to testing for the minimum and maximum.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
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
    (void) fprintf (stderr,
		    "Function: <%s> will be removed in Karma version 2.0\n",
		    function_name);
    (void) fprintf (stderr, "Use: <ds_find_2D_extremes> instead.\n");
    ord_start_coord = ds_get_coord_num (ord_dim_desc, ord_scan_start,
					SEARCH_BIAS_UPPER);
    ord_end_coord = ds_get_coord_num (ord_dim_desc, ord_scan_end,
				      SEARCH_BIAS_LOWER);
    data += ord_start_coord * ord_dim_stride;
    for (ord_coord_num = ord_start_coord; ord_coord_num <= ord_end_coord;
	 ++ord_coord_num, data += ord_dim_stride)
    {
	/*  Get extremes along abscissa dimension  */
	if ( !ds_find_single_extremes (data, elem_type, conv_type,
				       abs_dim_desc, abs_dim_stride,
				       abs_scan_start, abs_scan_end,
				       min, max) ) return (FALSE);
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
/*  [PURPOSE] This routine will find the histogram of a single trace (element
    versus a dimension). This routine may be called repeatedly with multiple
    traces in order to build an aggregate histogram of all traces.
    <data> A pointer to the data. Misaligned data will cause bus errors on some
    platforms.
    <elem_type> The type of the element.
    <conv_type> The type of conversion to use for complex numbers.
    Legal value for this include:
        CONV_CtoR_REAL        CONV_CtoR_IMAG
        CONV_CtoR_ABS         CONV_CtoR_SQUARE_ABS
	CONV_CtoR_PHASE       CONV_CtoR_CONT_PHASE
    <num_values> The length of the dimension.
    <offsets> The address offsets for data along the dimension.
    <stride> If the value of  offsets  is NULL, this gives the stride (in
    bytes) between consecutive values along the dimension.
    <min> Data values below this will be ignored.
    <max> Data values above this will be ignored.
    <num_bins> The number of histogram bins.
    <histogram_array> The histogram array. The values in this array are updated
    and hence must be initialised externally.
    <histogram_peak> The peak of the histogram is written here. This value is
    updated, and hence must be externally initialised to 0.
    <histogram_mode> The mode of the histogram (index value of the peak) will
    be written here. This value is updated, and hence must be externally
    initialised to 0.
    [MT-LEVEL] Safe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int value_count, block_size;
    unsigned long index, hval, hpeak, hmode;
    double value, bin_factor;
    double toobig = TOOBIG;
    flag complex = FALSE;
    double *val;
    double values[2 * BLOCK_SIZE];
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
    /*  Loop over blocks  */
    while (num_values > 0)
    {
	block_size = (num_values > BLOCK_SIZE) ? BLOCK_SIZE : num_values;
	/*  Convert data  */
	if (offsets == NULL)
	{
	    if ( !ds_get_elements (data, elem_type, stride, values, &complex,
				   block_size) ) return (FALSE);
	    data += stride * block_size;
	}
	else
	{
	    if ( !ds_get_scattered_elements (data, elem_type, offsets, values,
					     &complex, block_size) )
	    {
		return (FALSE);
	    }
	    offsets += block_size;
	}
	if (complex) ds_complex_to_real_1D (values, 2, values, block_size,
					    conv_type);
	bin_factor = (double) (num_bins - 1) / (max - min);
	for (value_count = 0, val = values; value_count < block_size;
	     ++value_count, val += 2)
	{
	    value = val[0];
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
	num_values -= block_size;
    }
    *histogram_peak = hpeak;
    *histogram_mode = hmode;
    return (TRUE);
}   /*  End Function ds_find_single_histogram  */

/*PUBLIC_FUNCTION*/
void ds_complex_to_real_1D (double *out, unsigned int out_stride,
			    double *inp, unsigned int num_values,
			    unsigned int conv_type)
/*  [PURPOSE] This routine will convert a 1 dimensional array of complex values
    to real values.
    <out> A pointer to the output array.
    <out_stride> The stride (in doubles) of the output array.
    <inp> The array of input complex values.
    <num_values> The number of values to convert.
    <conv_type> The conversion type to apply.
    [MT-LEVEL] Safe.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double real, imag;
    double toobig = TOOBIG;
    static char function_name[] = "ds_complex_to_real_1D";

    for (count = 0; count < num_values; ++count, out += out_stride, inp += 2)
    {
	if ( (real = inp[0]) >= toobig )
	{
	    *out = toobig;
	    continue;
	}
	if ( (imag = inp[1]) >= toobig )
	{
	    *out = toobig;
	    continue;
	}
	switch (conv_type)
	{
	  case CONV_CtoR_REAL:
	    *out = real;
	    break;
	  case CONV_CtoR_IMAG:
	    *out = imag;
	    break;
	  case CONV_CtoR_ABS:
	    *out = sqrt (real * real + imag * imag);
	    break;
	  case CONV_CtoR_SQUARE_ABS:
	    *out = real * real + imag * imag;
	    break;
	  case CONV_CtoR_PHASE:
	    if ( (real == 0.0) && (imag == 0.0) )
	    {
		*out = 0.0;
	    }
	    else
	    {
		*out = atan2 (imag, real);
	    }
	    break;
	  case CONV_CtoR_CONT_PHASE:
	    (void) fprintf (stderr,
			    "Not finished continuous phase\n");
	    return;
/*
	    break;
*/
	  default:
	    (void) fprintf (stderr,
			    "Illegal value of conversion: %d\n",
			    conv_type);
	    a_prog_bug (function_name);
	    break;
	}
    }	
}   /*  End Function ds_complex_to_real_1D  */
