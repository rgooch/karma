/*LINTLIBRARY*/
/*  wrappers.c

    This code provides the Intelligent Array wrapper interface to Karma data
    structures.

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

/*  This file contains all routines needed for the simple processing of
  n-dimensional data structures.


    Written by      Richard Gooch   5-SEP-1993

    Updated by      Richard Gooch   5-SEP-1993: Extracted wrapper functions
  from  iarray.c

    Updated by      Richard Gooch   6-AUG-1994: Moved to full ASCII function
  declaration syntax. Had to do this because of the confused Dec Alpha compiler

    Last updated by Richard Gooch   26-NOV-1994: Moved to
  packages/iarray/wrappers.c


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>


#define VERIFY_IARRAY(array) if (array == NULL) \
{(void) fprintf (stderr, "NULL iarray passed\n"); \
 a_prog_bug (function_name); }
#ifdef dummy
if (array->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid iarray\n"); \
 a_prog_bug (function_name); }
#endif


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
iarray iarray_create_1D (unsigned long xlen, unsigned int type)
/*  [PURPOSE] This routine will create a 1-dimensional "Intelligent Array",
    using the Karma general data structure format as the underlying data format
    <xlen> The length of the dimension. The name of the dimension will be the
    default: "Axis 0".
    <type> The type of the data. Legal values for this are:
        K_FLOAT, K_DOUBLE, K_BYTE, K_INT, K_SHORT, K_COMPLEX, K_DCOMPLEX,
	K_BCOMPLEX, K_ICOMPLEX,
        K_SCOMPLEX, K_LONG, K_LCOMPLEX, K_UBYTE, K_UINT, K_USHORT, K_ULONG,
	K_UBCOMPLEX,
        K_UICOMPLEX, K_USCOMPLEX, K_ULCOMPLEX.
    The name of the element will be the default name: "Intensity".
    [RETURNS] A dynamically allocated intelligent array on success, else NULL.
*/
{
    return ( iarray_create (type, 1, (char **) NULL, &xlen, (char *) NULL,
			    (iarray) NULL) );
}   /*  End Function iarray_create_1D  */

/*PUBLIC_FUNCTION*/
iarray iarray_create_2D (unsigned long ylen, unsigned long xlen,
			 unsigned int type)
/*  [PURPOSE] This routine will create a 2-dimensional "Intelligent Array",
    using the Karma general data structure format as the underlying data format
    <ylen> The length of the most significant dimension (ie. the dimension with
    the greatest stride in memory.
    <xlen> The length of the least significant dimension.
    The names of the dimensions will be the defaults: "Axis 0" and "Axis 1",
    respectively.
    <type> The type of the data. Legal values for this are:
        K_FLOAT, K_DOUBLE, K_BYTE, K_INT, K_SHORT, K_COMPLEX, K_DCOMPLEX,
	K_BCOMPLEX, K_ICOMPLEX,
        K_SCOMPLEX, K_LONG, K_LCOMPLEX, K_UBYTE, K_UINT, K_USHORT, K_ULONG,
	K_UBCOMPLEX,
        K_UICOMPLEX, K_USCOMPLEX, K_ULCOMPLEX.
    The name of the element will be the default name: "Intensity".
    [RETURNS] A dynamically allocated intelligent array on success, else NULL.
*/
{
    unsigned long dim_lengths[2];

    dim_lengths[0] = ylen;
    dim_lengths[1] = xlen;
    return ( iarray_create (type, 2, (char **) NULL, dim_lengths,
			    (char *) NULL, (iarray) NULL) );
}   /*  End Function iarray_create_2D  */

/*PUBLIC_FUNCTION*/
iarray iarray_create_3D (unsigned long zlen, unsigned long ylen,
			 unsigned long xlen, unsigned int type)
/*  [PURPOSE] This routine will create a 3-dimensional "Intelligent Array",
    using the Karma general data structure format as the underlying data format
    <zlen> The length of the most significant dimension (ie. the dimension with
    the greatest stride in memory).
    <ylen> The length of the middle dimension.
    <xlen> The length of the least significant dimension.
    The names of the dimensions will be the defaults: "Axis 0", "Axis 1" and
    "Axis 2",
    respectively.
    <type> The type of the data. Legal values for this are:
        K_FLOAT, K_DOUBLE, K_BYTE, K_INT, K_SHORT, K_COMPLEX, K_DCOMPLEX,
	K_BCOMPLEX, K_ICOMPLEX,
        K_SCOMPLEX, K_LONG, K_LCOMPLEX, K_UBYTE, K_UINT, K_USHORT, K_ULONG,
	K_UBCOMPLEX,
        K_UICOMPLEX, K_USCOMPLEX, K_ULCOMPLEX.
    The name of the element will be the default name: "Intensity".
    [RETURNS] A dynamically allocated intelligent array on success, else NULL.
*/
{
    unsigned long dim_lengths[3];

    dim_lengths[0] = zlen;
    dim_lengths[1] = ylen;
    dim_lengths[2] = xlen;
    return ( iarray_create (type, 3, (char **) NULL, dim_lengths,
			    (char *) NULL, (iarray) NULL) );
}   /*  End Function iarray_create_3D  */

/*PUBLIC_FUNCTION*/
iarray iarray_create_4D (unsigned long zlen, unsigned long ylen,
			 unsigned long xlen, unsigned int wlen,
			 unsigned int type)
/*  [PURPOSE] This routine will create a 4-dimensional "Intelligent Array",
    using the Karma general data structure format as the underlying data format
    <zlen> The length of the most significant dimension (ie. the dimension with
    the greatest stride in memory).
    <ylen> The length of the second most significant dimension.
    <xlen> The length of the second least significant dimension.
    <wlen> The length of the least significant dimension.
    The names of the dimensions will be the defaults: "Axis 0", "Axis 1",
    "Axis 2" and "Axis 3", respectively.
    <type> The type of the data. Legal values for this are:
        K_FLOAT, K_DOUBLE, K_BYTE, K_INT, K_SHORT, K_COMPLEX, K_DCOMPLEX,
	K_BCOMPLEX, K_ICOMPLEX,
        K_SCOMPLEX, K_LONG, K_LCOMPLEX, K_UBYTE, K_UINT, K_USHORT, K_ULONG,
	K_UBCOMPLEX,
        K_UICOMPLEX, K_USCOMPLEX, K_ULCOMPLEX.
    The name of the element will be the default name: "Intensity".
    [RETURNS] A dynamically allocated intelligent array on success, else NULL.
*/
{
    unsigned long dim_lengths[4];

    dim_lengths[0] = zlen;
    dim_lengths[1] = ylen;
    dim_lengths[2] = xlen;
    dim_lengths[3] = wlen;
    return ( iarray_create (type, 4, (char **) NULL, dim_lengths,
			    (char *) NULL, (iarray) NULL) );
}   /*  End Function iarray_create_4D  */

/*PUBLIC_FUNCTION*/
flag iarray_put_float (iarray array, char *name, float value)
/*  This routine will put a unique named float value into the underlying Karma
    general data structure of an "Intelligent Array".
    The array must be given by  array  .
    The name of the element must be pointed to by  name  .
    The value of the data must be given by  value  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    double val[2];
    static char function_name[] = "iarray_put_float";

    VERIFY_IARRAY (array);
    val[0] = value;
    val[1] = 0.0;
    return ( iarray_put_named_value (array, name, K_FLOAT, val) );
}   /*  End Function iarray_put_float  */

/*PUBLIC_FUNCTION*/
flag iarray_put_int (iarray array, char *name, int value)
/*  This routine will put a unique named integer value into the underlying
    Karma general data structure of an "Intelligent Array".
    The array must be given by  array  .
    The name of the element must be pointed to by  name  .
    The value of the data must be given by  value  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    double val[2];
    static char function_name[] = "iarray_put_int";

    VERIFY_IARRAY (array);
    val[0] = value;
    val[1] = 0.0;
    return ( iarray_put_named_value (array, name, K_INT, val) );
}   /*  End Function iarray_put_int  */

/*PUBLIC_FUNCTION*/
float iarray_get_float (iarray array, char *name)
/*  This routine will get a unique named float value from the underlying Karma
    general data structure of an "Intelligent Array".
    The array must be given by  array  .
    The name of the element must be pointed to by  name  .
    The routine returns the value on success, else it aborts the process on
    error.
*/
{
    double val[2];
    static char function_name[] = "iarray_get_float";

    VERIFY_IARRAY (array);
    if (iarray_get_named_value (array, name, (unsigned int *) NULL, val)
	== TRUE)
    {
	return (val[0]);
    }
    (void) fprintf (stderr, "Could not find unique element: \"%s\"\n", name);
    a_print_abort ();
    return (TOOBIG);
}   /*  End Function iarray_get_float  */

/*PUBLIC_FUNCTION*/
int iarray_get_int (iarray array, char *name)
/*  This routine will get a unique named integer value into the underlying
    Karma general data structure of an "Intelligent Array".
    The array must be given by  array  .
    The name of the element must be pointed to by  name  .
    The routine returns the value on success, else it aborts the process on
    error.
*/
{
    double val[2];
    static char function_name[] = "iarray_get_int";

    VERIFY_IARRAY (array);
    if (iarray_get_named_value (array, name, (unsigned int *) NULL, val)
	== TRUE)
    {
	return (val[0]);
    }
    (void) fprintf (stderr, "Could not find unique element: \"%s\"\n", name);
    a_print_abort ();
    return (0);
}   /*  End Function iarray_get_int  */

/*PUBLIC_FUNCTION*/
flag iarray_fill_float (iarray array, float value)
/*  This routine will fill an "Intelligent Array" with a single float value.
    The array must be given by  array  .
    The fill value must be given by  value  .
    If filling a complex array, both the real and imaginary components are
    filled with the fill value.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    double val[2];
    static char function_name[] = "iarray_fill_float";

    VERIFY_IARRAY (array);
    val[0] = value;
    val[1] = value;
    return ( iarray_fill (array, val) );
}   /*  End Function iarray_fill_float  */

/*PUBLIC_FUNCTION*/
flag iarray_fill_int (iarray array, int value)
/*  This routine will fill an "Intelligent Array" with a single integer value.
    The array must be given by  array  .
    The fill value must be given by  array  .
    If filling a complex array, both the real and imaginary components are
    filled with the fill value.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    double val[2];
    static char function_name[] = "iarray_fill_int";

    VERIFY_IARRAY (array);
    val[0] = value;
    val[1] = value;
    return ( iarray_fill (array, val) );
}   /*  End Function iarray_fill_int  */

/*PUBLIC_FUNCTION*/
flag iarray_min_max_float (iarray array, float *min, float *max)
/*  This routine will determine the minimum and maximum value of an
    "Intelligent Array".
    The array must be given by  array  .
    If the array is a complex array, then the routine computes the minimum and
    maximum magnitudes.
    The routine will write the minimum value to the storage pointed to by  min
    The routine will write the maximum value to the storage pointed to by  max
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    flag return_value;
    double min_val;
    double max_val;
    static char function_name[] = "iarray_min_max_float";

    VERIFY_IARRAY (array);
    return_value = iarray_min_max (array, CONV1_ABS, &min_val, &max_val);
    *min = min_val;
    *max = max_val;
    return (return_value);
}   /*  End Function iarray_min_max_float  */

/*PUBLIC_FUNCTION*/
flag iarray_min_max_int (iarray array, int *min, int *max)
/*  This routine will determine the minimum and maximum value of an
    "Intelligent Array".
    The array must be given by  array  .
    If the array is a complex array, then the routine computes the minimum and
    maximum magnitudes.
    The routine will write the minimum value to the storage pointed to by  min
    The routine will write the maximum value to the storage pointed to by  max
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    flag return_value;
    double min_val;
    double max_val;
    static char function_name[] = "iarray_min_max_int";

    VERIFY_IARRAY (array);
    return_value = iarray_min_max (array, CONV1_ABS, &min_val, &max_val);
    *min = min_val;
    *max = max_val;
    return (return_value);
}   /*  End Function iarray_min_max_int  */

/*PUBLIC_FUNCTION*/
flag iarray_scale_and_offset_float (iarray out, iarray inp, float scale, 
				    float offset)
/*  This routine will perform a scale and offset on every element in an
    "Intelligent Array".
    The output array must be given by  out  .
    The input array must be given by  inp  .
    NOTE: the input and output arrays MUST be the same size (though not
    necessarily the same type).
    The scale value must be pointed to by  scale  .
    The offset value must be pointed to by  offset  .
    When converting from a complex to a real array, the magnitude is scaled
    and offset.
    When converting from a real to a complex array, the imaginary component of
    the output array is unaffected (NOTE: it is NOT set to 0).
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    double sc[2];
    double off[2];
    static char function_name[] = "iarray_scale_and_offset_float";

    VERIFY_IARRAY (out);
    VERIFY_IARRAY (inp);
    sc[0] = scale;
    sc[1] = 0.0;
    off[0] = offset;
    off[1] = 0.0;
    return ( iarray_scale_and_offset (out, inp, sc, off, TRUE) );
}   /*  End Function iarray_scale_and_offset_float  */

/*PUBLIC_FUNCTION*/
flag iarray_scale_and_offset_int (iarray out, iarray inp, int scale,int offset)
/*  This routine will perform a scale and offset on every element in an
    "Intelligent Array".
    The output array must be given by  out  .
    The input array must be given by  inp  .
    NOTE: the input and output arrays MUST be the same size (though not
    necessarily the same type).
    The scale value must be pointed to by  scale  .
    The offset value must be pointed to by  offset  .
    When converting from a complex to a real array, the magnitude is scaled
    and offset.
    When converting from a real to a complex array, the imaginary component of
    the output array is unaffected (NOTE: it is NOT set to 0).
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    double sc[2];
    double off[2];
    static char function_name[] = "iarray_scale_and_offset_int";

    VERIFY_IARRAY (out);
    VERIFY_IARRAY (inp);
    sc[0] = scale;
    sc[1] = 0.0;
    off[0] = offset;
    off[1] = 0.0;
    return ( iarray_scale_and_offset (out, inp, sc, off, TRUE) );
}   /*  End Function iarray_scale_and_offset_int  */
