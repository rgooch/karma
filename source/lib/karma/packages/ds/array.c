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

    Updated by      Richard Gooch   24-JAN-1996: Created <ds_find_1D_extremes>
  and <ds_find_2D_extremes>.

    Updated by      Richard Gooch   7-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   30-JUN-1996: Created
  <ds_find_contiguous_extremes>.

    Updated by      Richard Gooch   27-JUL-1996: Created <ds_find_1D_sum>.

    Updated by      Richard Gooch   22-OCT-1996: Accepted and fixed code for
  <ds_find_?D_stats> from Vincent McIntyre.

    Last updated by Richard Gooch   3-NOV-1996: Returned total square from
  <ds_find_2D_stats>.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>


#define BLOCK_SIZE 1024


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag ds_find_1D_extremes (CONST char *data,
			  unsigned int num_values, uaddr *offsets,
			  unsigned int elem_type, unsigned int conv_type,
			  double *min, double *max)
/*  [SUMMARY] Find the minimum and maximum of a 1D array.
    [PURPOSE] This routine will find the extremes (minimum and maximum) of a
    single trace (element versus a dimension).
    <data> A pointer to the data. Misaligned data will cause bus errors on some
    platforms.
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
    static char function_name[] = "ds_find_1D_extremes";

    if ( (data == NULL) || (min == NULL) || (max == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
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
		    fprintf (stderr,
				    "Continuous phase not implemented yet\n");
		    return (FALSE);
/*
		    break;
*/
		  default:
		    fprintf (stderr,
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
/*  [SUMMARY] Find the minimum and maximum of a 2D array.
    [PURPOSE] This routine will find the extremes (minimum and maximum) of a
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

/*PUBLIC_FUNCTION*/
flag ds_find_contiguous_extremes (CONST char *data, unsigned int num_values,
				  uaddr stride, unsigned int elem_type,
				  unsigned int conv_type,
				  double *min, double *max)
/*  [SUMMARY] Find the minimum and maximum of a contiguous array.
    [PURPOSE] This routine will find the extremes (minimum and maximum) of a
    single trace (element versus a dimension).
    <data> A pointer to the data. Misaligned data will cause bus errors on some
    platforms.
    <num_values> The number of values to process.
    <stride> The stride (in bytes) between consecutive data values.
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
    flag slow = FALSE;
    int i_min, i_max, i_val, i_toobig;
    unsigned int block_size;
    unsigned int value_count;
    float f_min, f_max, f_val, f_toobig = TOOBIG;
    double value = 0.0;  /*  Initialised to keep compiler happy  */
    double min_val, max_val;
    flag complex = FALSE;
    double *val;
    double values[2 * BLOCK_SIZE];
#ifdef NEED_ALIGNED_DATA
    extern char host_type_sizes[NUMTYPES];
#endif
    static char function_name[] = "ds_find_contiguous_extremes";

    if ( (data == NULL) || (min == NULL) || (max == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    min_val = *min;
    max_val = *max;
    /*  Determine if this can be optimised  */
    if ( ds_element_is_complex (elem_type) ) slow = TRUE;
#ifdef NEED_ALIGNED_DATA
    if ( !IS_ALIGNED ( data, host_type_sizes[elem_type] ) ) slow = TRUE;
    if (stride % host_type_sizes[elem_type] != 0) slow = TRUE;
#endif
    switch (elem_type)
    {
      case K_BYTE:
      case K_UBYTE:
      case K_SHORT:
      case K_USHORT:
      case K_INT:
      case K_FLOAT:
      case K_DOUBLE:
	break;
      default:
	slow = TRUE;
	break;
    }
    if (slow)
    {
	/*  Loop over blocks  */
	for (; num_values > 0;
	     num_values -= block_size, data += block_size * stride)
	{
	    block_size = (num_values > BLOCK_SIZE) ? BLOCK_SIZE : num_values;
	    /*  Convert data  */
	    if ( !ds_get_elements (data, elem_type, stride, values,
				   &complex, block_size) ) return (FALSE);
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
			fprintf (stderr,
				 "Continuous phase not implemented yet\n");
		      return (FALSE);
/*
		      break;
*/
		      default:
			fprintf (stderr,
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
	}
	*min = min_val;
	*max = max_val;
	return (TRUE);
    }
    /*  Can do it quickly  */
    f_min = min_val;
    f_max = max_val;
    switch (elem_type)
    {
      case K_BYTE:
	i_min = 127;
	i_max = -127;
	for (value_count = 0; value_count < num_values;
	     ++value_count, data += stride)
	{
	    if ( (i_val = *(signed char *) data) == -128 ) continue;
	    if (i_val < i_min) i_min = i_val;
	    if (i_val > i_max) i_max = i_val;
	}
	if ( (double) i_min < min_val ) min_val = i_min;
	if ( (double) i_max > max_val ) max_val = i_max;
	break;
      case K_UBYTE:
	i_min = 255;
	i_max = 0;
	for (value_count = 0; value_count < num_values;
	     ++value_count, data += stride)
	{
	    i_val = *(unsigned char *) data;
	    if (i_val < i_min) i_min = i_val;
	    if (i_val > i_max) i_max = i_val;
	}
	if ( (double) i_min < min_val ) min_val = i_min;
	if ( (double) i_max > max_val ) max_val = i_max;
	break;
      case K_SHORT:
	i_min = 32767;
	i_max = -32767;
	for (value_count = 0; value_count < num_values;
	     ++value_count, data += stride)
	{
	    if ( (i_val = *(signed short *) data) == -32768 ) continue;
	    if (i_val < i_min) i_min = i_val;
	    if (i_val > i_max) i_max = i_val;
	}
	if ( (double) i_min < min_val ) min_val = i_min;
	if ( (double) i_max > max_val ) max_val = i_max;
	break;
      case K_USHORT:
	i_min = 65535;
	i_max = 0;
	for (value_count = 0; value_count < num_values;
	     ++value_count, data += stride)
	{
	    i_val = *(unsigned short *) data;
	    if (i_val < i_min) i_min = i_val;
	    if (i_val > i_max) i_max = i_val;
	}
	if ( (double) i_min < min_val ) min_val = i_min;
	if ( (double) i_max > max_val ) max_val = i_max;
	break;
      case K_INT:
	i_min = 2147483647;
	i_max = -2147483647;
	i_toobig = -2147483647;
	--i_toobig;
	for (value_count = 0; value_count < num_values;
	     ++value_count, data += stride)
	{
	    if ( (i_val = *(signed int *) data) == i_toobig ) continue;
	    if (i_val < i_min) i_min = i_val;
	    if (i_val > i_max) i_max = i_val;
	}
	if ( (double) i_min < min_val ) min_val = i_min;
	if ( (double) i_max > max_val ) max_val = i_max;
	break;
      case K_FLOAT:
	for (value_count = 0; value_count < num_values;
	     ++value_count, data += stride)
	{
	    if ( (f_val = *(float *) data) >= f_toobig ) continue;
	    if (f_val < f_min) f_min = f_val;
	    if (f_val > f_max) f_max = f_val;
	}
	min_val = f_min;
	max_val = f_max;
	break;
      case K_DOUBLE:
	for (value_count = 0; value_count < num_values;
	     ++value_count, data += stride)
	{
	    if ( (value = *(double *) data) >= TOOBIG ) continue;
	    if (value < min_val) min_val = value;
	    if (value > max_val) max_val = value;
	}
	break;
    }
    *min = min_val;
    *max = max_val;
    return (TRUE);
}   /*  End Function ds_find_contiguous_extremes  */

/*PUBLIC_FUNCTION*/
flag ds_find_single_histogram (CONST char *data, unsigned int elem_type,
			       unsigned int conv_type, unsigned int num_values,
			       CONST uaddr *offsets, unsigned int stride,
			       double min, double max, unsigned long num_bins,
			       unsigned long *histogram_array,
			       unsigned long *histogram_peak,
			       unsigned long *histogram_mode)
/*  [SUMMARY] Find the histogram of a 1D array.
    [PURPOSE] This routine will find the histogram of a single trace (element
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
    flag fast = TRUE;
    flag complex = FALSE;
    unsigned int value_count, block_size;
    unsigned long index, hval, hpeak, hmode;
    float f_val, f_bin_factor, f_toobig = TOOBIG, f_min, f_max;
    double value, bin_factor;
    double toobig = TOOBIG;
    double *val;
    double values[2 * BLOCK_SIZE];
    static char function_name[] = "ds_find_single_histogram";

    if ( (data == NULL) || (histogram_array == NULL) ||
	(histogram_peak == NULL) || (histogram_mode == NULL ) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    hpeak = *histogram_peak;
    hmode = *histogram_mode;
    if (min >= max)
    {
	fprintf (stderr, "min: %e is not less than max: %e\n",
			min, max);
	a_prog_bug (function_name);
    }
    if ( ds_element_is_complex (elem_type) ) fast = FALSE;
#ifdef NEED_ALIGNED_DATA
    if ( !IS_ALIGNED ( data, host_type_sizes[elem_type] ) ) fast = FALSE;
    if (stride % host_type_sizes[elem_type] != 0) fast = FALSE;
#endif
    switch (elem_type)
    {
      case K_BYTE:
      case K_UBYTE:
      case K_FLOAT:
      case K_DOUBLE:
	break;
      default:
	fast = FALSE;
	break;
    }
    if (offsets != NULL) fast = FALSE;
    if ( fast && (elem_type == K_FLOAT) )
    {
	f_min = min;
	f_max = max;
	f_bin_factor = (float) (num_bins - 1) / (f_max - f_min);
	for (; num_values > 0; --num_values, data += stride)
	{
	    if ( (f_val = *(float *) data) >= f_toobig ) continue;
	    if (f_val < f_min) continue;
	    if (f_val > f_max) continue;
	    /*  Convert value to bin number  */
	    index = (unsigned int) ( (f_val - f_min) * f_bin_factor );
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
    }
    bin_factor = (double) (num_bins - 1) / (max - min);
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
/*  [SUMMARY] Convert a 1 dimensional array of complex values to real values.
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
	    fprintf (stderr,
			    "Not finished continuous phase\n");
	    return;
/*
	    break;
*/
	  default:
	    fprintf (stderr,
			    "Illegal value of conversion: %d\n",
			    conv_type);
	    a_prog_bug (function_name);
	    break;
	}
    }	
}   /*  End Function ds_complex_to_real_1D  */

/*PUBLIC_FUNCTION*/
flag ds_find_1D_sum (CONST char *data, unsigned int elem_type,
		     unsigned int num_values, CONST uaddr *offsets,
		     unsigned int stride, double sum[2])
/*  [SUMMARY] Find the sum of a 1D array.
    [PURPOSE] This routine will find the sum of a single trace (element
    versus a dimension). This routine may be called repeatedly with multiple
    traces in order to build an aggregate sum of all traces.
    <data> A pointer to the data. Misaligned data will cause bus errors on some
    platforms.
    <elem_type> The type of the element.
    <num_values> The length of the dimension.
    <offsets> The address offsets for data along the dimension.
    <stride> If the value of  offsets  is NULL, this gives the stride (in
    bytes) between consecutive values along the dimension.
    <sum> The sum is written here.
    [MT-LEVEL] Safe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag fast = TRUE;
    flag complex = FALSE;
    unsigned int value_count, block_size, count;
    float f_val, f_toobig = TOOBIG;
    float f_sum_r = 0.0;
    double value_r, value_i;
    double d_sum_r = 0.0;
    double d_sum_i = 0.0;
    double toobig = TOOBIG;
    float *f_ptr;
    double *val;
    double values[2 * BLOCK_SIZE];
    static char function_name[] = "ds_find_1D_sum";

    if ( (data == NULL) || (sum == NULL ) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( ds_element_is_complex (elem_type) ) fast = FALSE;
#ifdef NEED_ALIGNED_DATA
    if ( !IS_ALIGNED ( data, host_type_sizes[elem_type] ) ) fast = FALSE;
    if (stride % host_type_sizes[elem_type] != 0) fast = FALSE;
#endif
    switch (elem_type)
    {
      case K_BYTE:
      case K_UBYTE:
      case K_FLOAT:
      case K_DOUBLE:
	break;
      default:
	fast = FALSE;
	break;
    }
    if (offsets != NULL) fast = FALSE;
    if ( fast && (elem_type == K_FLOAT) )
    {
	stride /= sizeof *f_ptr;
	f_ptr = (float *) data;
	for (count = 0; count < num_values; ++count, f_ptr += stride)
	{
	    if ( (f_val = *f_ptr) >= f_toobig ) continue;
	    f_sum_r += f_val;
	}
	sum[0] = f_sum_r;
	sum[1] = 0.0;
	return (TRUE);
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
	if (complex)
	{
	    for (value_count = 0, val = values; value_count < block_size;
		 ++value_count, val += 2)
	    {
		value_r = val[0];
		value_i = val[1];
		if (value_r >= toobig) continue;
		if (value_i >= toobig) continue;
		d_sum_r += value_r;
		d_sum_i += value_i;
	    }
	    num_values -= block_size;
	}
	else
	{
	    for (value_count = 0, val = values; value_count < block_size;
		 ++value_count, val += 2)
	    {
		value_r = val[0];
		if (value_r >= toobig) continue;
		d_sum_r += value_r;
	    }
	    num_values -= block_size;
	}
    }
    sum[0] = d_sum_r;
    sum[1] = d_sum_i;
    return (TRUE);
}   /*  End Function ds_find_1D_sum  */

/*EXPERIMENTAL_FUNCTION*/
flag ds_find_1D_stats (CONST char *data,
		       unsigned int num_values, uaddr *offsets,
		       unsigned int elem_type, unsigned int conv_type,
		       double *min, double *max,
		       double *mean, double *stddev,
		       double *sum, double *sumsq, unsigned long *npoints)
/*  [SUMMARY] Compute simple statistics for a 1D array.
    [PURPOSE] This routine will find the minimum, maximum, mean, rms, sum and
    sum-of-squares of a single trace (element versus a dimension).
    Blanked values are excluded.
    <data> A pointer to the data. Misaligned data will cause bus errors on some
    platforms.
    <num_values> The number of values to process.
    <offsets> The address offsets for data along the dimension.
    <elem_type> The type of the element.
    <conv_type> The type of conversion to use for complex numbers.
    <min> The minimum value will be written here.
    <max> The maximum value will be written here.
    <mean> The mean value will be written here.
    <stddev> The standard deviation will be written here.
    <sum> The total of all values will be written here.
    <sumsq> The total of the squares of all values will be written here.
    <npoints> The number of values used to compute statistics will be written
    here. This may differ from num_values if there are blanked pixels.
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
    unsigned long npoints_val;
    double value = 0.0;  /*  Initialised to keep compiler happy  */
    double min_val, max_val, mean_val, total_val, totalsq_val;
    flag complex = FALSE;
    double *val;
    double values[2 * BLOCK_SIZE];
    static char function_name[] = "ds_find_1D_stats";

    if ( (data == NULL) || (min == NULL) || (max == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    min_val = *min;
    max_val = *max;
    mean_val = 0.0;
    total_val = 0.0;
    totalsq_val = 0.0;
    npoints_val = 0;
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
		    fprintf (stderr, "Continuous phase not implemented yet\n");
		    return (FALSE);
		    /*break;*/
		  default:
		    fprintf (stderr,
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
	    npoints_val++;
	    total_val += value;
	    totalsq_val += value * value;
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
    mean_val = total_val / (double) npoints_val;
    *mean    = mean_val;
    *stddev  = sqrt (totalsq_val / (double) npoints_val - mean_val * mean_val);
    *sum     = total_val;
    *sumsq   = totalsq_val;
    *npoints = npoints_val;
    return (TRUE);
}   /*  End Function ds_find_1D_stats  */

/*EXPERIMENTAL_FUNCTION*/
flag ds_find_2D_stats (CONST char *data,
		       unsigned int length1, uaddr *offsets1,
		       unsigned int length2, uaddr *offsets2,
		       unsigned int elem_type, unsigned int conv_type,
		       double *min, double *max, double *mean,
		       double *stddev, double *sum, double *sumsq,
		       unsigned long *npoints)
/*  [SUMMARY] Compute simple statistics for a 2D array.
    [PURPOSE] This routine will find the minimum, maximum, mean, rms and sum of
    a single plane (element versus two dimensions).
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
    <mean> The mean value will be written here.
    <stddev> The standard deviation will be written here.
    <sum> The total of all values will be written here.
    <sumsq> The total of the squares of all values will be written here.
    <npoints> The number of values used to compute statistics will be written
    here. This will differ from num_values if there are blanked pixels.
    [NOTE] The minimum and maximum value must be initialised to a very large
    positive number and a very large negative number, respectively, outside of
    the routine. In other words, the routine does not initialise these values
    prior to testing for the minimum and maximum.
    [MT-LEVEL] Safe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int count;
    unsigned long sub_npoints, points_val;
    double sub_total, sub_totalsq, sub_mean, sub_stddev;
    double total, totalsq;

    total = 0.0;
    totalsq = 0.0;
    points_val = 0;
    for (count = 0; count < length1; ++count)
    {
	if ( !ds_find_1D_stats (data + offsets1[count], length2, offsets2,
				elem_type, conv_type,
				min, max, &sub_mean, &sub_stddev,
				&sub_total, &sub_totalsq, &sub_npoints) )
	    return (FALSE);
	total      += sub_total;
	totalsq    += sub_totalsq;
	points_val += sub_npoints;
    }
    *sum     = total;
    *mean    = total / points_val;
    *stddev  = sqrt ( totalsq / (double) points_val -
		      (total / (double) points_val) *
		      (total / (double) points_val) );
    *sumsq = totalsq;
    *npoints = points_val;
    return (TRUE);
}   /*  End Function ds_find_2D_stats  */


/*  Obsolete functions follow  */

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
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    fprintf (stderr,
		    "Function: <%s> will be removed in Karma version 2.0\n",
		    function_name);
    fprintf (stderr, "Use: <ds_find_1D_extremes> instead.\n");
    if (scan_start < dimension->minimum)
    {
	scan_start = dimension->minimum;
    }
    if (scan_end > dimension->maximum)
    {
	scan_end = dimension->maximum;
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
		fprintf (stderr,
				"Continuous phase not implemented yet\n");
		return (FALSE);
/*
		break;
*/
	      default:
		fprintf (stderr,
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
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    fprintf (stderr,
		    "Function: <%s> will be removed in Karma version 2.0\n",
		    function_name);
    fprintf (stderr, "Use: <ds_find_2D_extremes> instead.\n");
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
