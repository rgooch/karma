/*LINTLIBRARY*/
/*MISALIGNED*/
/*  m_ds_get.c

    This code provides miscellaneous Karma data structure manipulation routines

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

    This file contains the various utility routines for getting info on the
    general data structure supported in Karma. This code is only compiled on
    platforms where misaligned data accesses require a special compile-time
    switch.


    Written by      Richard Gooch   21-MAR-1993: Copied from  ds_get.c

    Updated by      Richard Gooch   21-MAR-1993

    Last updated by Richard Gooch   7-AUG-1994: Added some CONST declatations.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <os.h>
#include <karma_ds.h>
#include <karma_a.h>

#ifdef NEEDS_MISALIGN_COMPILE

double misalign__ds_convert_atomic (datum, datum_type, real_out, imag_out)
/*  This routine will convert an atomic datum to a double precision value.
    The datum to be converted must be pointed to by  datum  and the data type
    value must be in  datum_type  .
    Complex data types are converted to their absolute value.
    The following data types are not convertible, and the routine will return
    the value TOOBIG:
        NONE, K_ARRAY, LISTP, MULTI_ARRAY.
    The routine will write the real and imaginary components into  real_out
    and  imag_out  ,respectively, if these pointers are not NULL. For a real
    value, the imaginary value is 0.0 .
    The routine returns the converted value on success, else it returns TOOBIG.
*/
CONST char *datum;
unsigned int datum_type;
double *real_out;
double *imag_out;
{
    double real;
    double imag;
    static char function_name[] = "misalign__ds_convert_atomic";

    if (datum == NULL)
    {
	/*  No data supplied    */
	if (real_out != NULL)
	{
	    *real_out = TOOBIG;
	}
	if (imag_out != NULL)
	{
	    *imag_out = TOOBIG;
	}
        return (TOOBIG);
    }
    imag = 0.0;
    switch (datum_type)
    {
      case NONE:
      case K_ARRAY:
      case LISTP:
      case MULTI_ARRAY:
	/*  Bad data type   */
	if (real_out != NULL)
	{
	    *real_out = TOOBIG;
	}
	if (imag_out != NULL)
	{
	    *imag_out = TOOBIG;
	}
	return (TOOBIG);
/*
	break;
*/
      case K_FLOAT:
        real = *(float *) datum;
	break;
      case K_DOUBLE:
	real = *(double *) datum;
	break;
      case K_BYTE:
	real = *datum;
	break;
      case K_INT:
	real = *(int *) datum;
	break;
      case K_SHORT:
        real = *(short *) datum;
	break;
      case K_COMPLEX:
	real = *(float *) datum;
	imag = *( (float *) datum + 1 );
	break;
      case K_DCOMPLEX:
	real = *(double *) datum;
	imag = *( (double *) datum + 1 );
	break;
      case K_BCOMPLEX:
	real = *datum;
	imag = *(datum + 1);
	break;
      case K_ICOMPLEX:
	real = *(int *) datum;
	imag = *( (int *) datum + 1 );
	break;
      case K_SCOMPLEX:
	real = *(short *) datum;
	imag = *( (short *) datum + 1 );
	break;
      case K_LONG:
        real = *(long *) datum;
	break;
      case K_LCOMPLEX:
	real = *(long *) datum;
	imag = *( (long *) datum + 1 );
	break;
      case K_UBYTE:
	real = *(unsigned char *) datum;
	break;
      case K_UINT:
	real = *(unsigned int *) datum;
	break;
      case K_USHORT:
	real = *(unsigned short *) datum;
	break;
      case K_ULONG:
	real = *(unsigned long *) datum;
	break;
      case K_UBCOMPLEX:
	real = *(unsigned char *) datum;
	imag = *( (unsigned char *) datum + 1 );
	break;
      case K_UICOMPLEX:
	real = *(unsigned int *) datum;
	imag = *( (unsigned int *) datum + 1 );
	break;
      case K_USCOMPLEX:
	real = *(unsigned short *) datum;
	imag = *( (unsigned short *) datum + 1 );
	break;
      case K_ULCOMPLEX:
	real = *(unsigned long *) datum;
	imag = *( (unsigned long *) datum + 1 );
	break;
      default:
	/*  Unknown data type   */
	(void) fprintf (stderr, "Illegal data type: %u\n", datum_type);
	a_prog_bug (function_name);
	break;
    }
    if (real_out != NULL)
    {
	*real_out = real;
    }
    if (imag_out != NULL)
    {
	*imag_out = imag;
    }
    /*  Complex value   */
    return ( sqrt (real * real + imag * imag) );
}   /*  End Function misalign__ds_convert_atomic  */

flag misalign__ds_get_element (datum, datum_type, value, complex)
/*  This routine will convert an atomic datum to a double precision complex
    value.
    The datum to be converted must be pointed to by  datum  and the data type
    value must be in  datum_type  .
    The data value will be written to the storage pointed to by  value  .
    If the datum is a complex type, then the value of TRUE is written to the
    storage pointed to by  complex  ,else the value FALSE is written here. If
    this is NULL, nothing is written here.
    The routine returns TRUE if the data was successfully converted,
    else it returns FALSE.
*/
CONST char *datum;
unsigned int datum_type;
double *value;
flag *complex;
{
    flag tmp_complex = FALSE;
    static char function_name[] = "misalign__ds_get_element";

    if (datum == NULL)
    {
	(void) fprintf (stderr, "NULL datum pointer passed\n");
	a_prog_bug (function_name);
    }
    if (value == NULL)
    {
	(void) fprintf (stderr, " NULL value storage pointer passed\n");
	a_prog_bug (function_name);
    }
    value[1] = 0.0;
    switch (datum_type)
    {
      case NONE:
      case K_ARRAY:
      case LISTP:
      case MULTI_ARRAY:
	(void) fprintf (stderr, "Non-atomic data type: %u\n", datum_type);
	a_prog_bug (function_name);
	break;
      case K_FLOAT:
        value[0] = *(float *) datum;
	break;
      case K_DOUBLE:
	value[0] = *(double *) datum;
	break;
      case K_BYTE:
	if (*datum < -127)
	{
	    value[0] = TOOBIG;
	}
	else
	{
	    value[0] = *datum;
	}
	break;
      case K_INT:
	value[0] = *(int *) datum;
	break;
      case K_SHORT:
        value[0] = *(short *) datum;
	break;
      case K_COMPLEX:
	value[0] = *(float *) datum;
	value[1] = *( (float *) datum + 1 );
	tmp_complex = TRUE;
	break;
      case K_DCOMPLEX:
	value[0] = *(double *) datum;
	value[1] = *( (double *) datum + 1 );
	tmp_complex = TRUE;
	break;
      case K_BCOMPLEX:
	value[0] = *datum;
	value[1] = *(datum + 1);
	tmp_complex = TRUE;
	break;
      case K_ICOMPLEX:
	value[0] = *(int *) datum;
	value[1] = *( (int *) datum + 1 );
	tmp_complex = TRUE;
	break;
      case K_SCOMPLEX:
	value[0] = *(short *) datum;
	value[1] = *( (short *) datum + 1 );
	tmp_complex = TRUE;
	break;
      case K_LONG:
        value[0] = *(long *) datum;
	break;
      case K_LCOMPLEX:
	value[0] = *(long *) datum;
	value[1] = *( (long *) datum + 1 );
	tmp_complex = TRUE;
	break;
      case K_UBYTE:
	value[0] = *(unsigned char *) datum;
	break;
      case K_UINT:
	value[0] = *(unsigned int *) datum;
	break;
      case K_USHORT:
	value[0] = *(unsigned short *) datum;
	break;
      case K_ULONG:
	value[0] = *(unsigned long *) datum;
	break;
      case K_UBCOMPLEX:
	value[0] = *(unsigned char *) datum;
	value[1] = *( (unsigned char *) datum + 1 );
	tmp_complex = TRUE;
	break;
      case K_UICOMPLEX:
	value[0] = *(unsigned int *) datum;
	value[1] = *( (unsigned int *) datum + 1 );
	tmp_complex = TRUE;
	break;
      case K_USCOMPLEX:
	value[0] = *(unsigned short *) datum;
	value[1] = *( (unsigned short *) datum + 1 );
	tmp_complex = TRUE;
	break;
      case K_ULCOMPLEX:
	value[0] = *(unsigned long *) datum;
	value[1] = *( (unsigned long *) datum + 1 );
	tmp_complex = TRUE;
	break;
      default:
	/*  Unknown data type   */
	(void) fprintf (stderr, "Illegal data type: %u\n", datum_type);
	a_prog_bug (function_name);
	break;
    }
    if (complex != NULL)
    {
	*complex = tmp_complex;
    }
    return (TRUE);
}   /*  End Function misalign__ds_get_element  */

flag misalign__ds_get_elements (data, data_type, data_stride, values, complex,
				num_values)
/*  This routine will convert many atomic data to an array of double precision
    complex values.
    The data to be converted must be pointed to by  data  and the data type
    value must be in  data_type  .
    The stride of data elements in memory (in bytes) must be given by
    data_stride  .
    The data values will be written to the storage pointed to by  values  .
    If the data are a complex type, then the value of TRUE is written to the
    storage pointed to by  complex  ,else the value FALSE is written here. If
    this is NULL, nothing is written here.
    The number of data values to convert must be pointed to by  num_values  .
    The routine returns TRUE if the data was successfully converted,
    else it returns FALSE.
*/
CONST char *data;
unsigned int data_type;
unsigned int data_stride;
double *values;
flag *complex;
unsigned int num_values;
{
    flag tmp_complex = FALSE;
    unsigned int data_count;
    static char function_name[] = "misalign__ds_get_elements";

    if (data == NULL)
    {
	(void) fprintf (stderr, "NULL data pointer passed\n");
	a_prog_bug (function_name);
    }
    if (values == NULL)
    {
	(void) fprintf (stderr, "NULL values storage pointer passed\n");
	a_prog_bug (function_name);
    }
    switch (data_type)
    {
      case NONE:
      case K_ARRAY:
      case LISTP:
      case MULTI_ARRAY:
	(void) fprintf (stderr, "Non-atomic data type: %u\n", data_type);
	a_prog_bug (function_name);
	break;
      case K_FLOAT:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(float *) data;
	    *values++ = 0.0;
	}
	break;
      case K_DOUBLE:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(double *) data;
	    *values++ = 0.0;
	}
	break;
      case K_BYTE:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    if (*(char *) data < -127)
	    {
		*values++ = TOOBIG;
	    }
	    else
	    {
		*values++ = *(char *) data;
	    }
	    *values++ = 0.0;
	}
	break;
      case K_INT:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(int *) data;
	    *values++ = 0.0;
	}
	break;
      case K_SHORT:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(short *) data;
	    *values++ = 0.0;
	}
	break;
      case K_COMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(float *) data;
	    *values++ = *( (float *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_DCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(double *) data;
	    *values++ = *( (double *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_BCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(char *) data;
	    *values++ = *( (char *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_ICOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(int *) data;
	    *values++ = *( (int *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_SCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(short *) data;
	    *values++ = *( (short *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_LONG:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(long *) data;
	    *values++ = 0.0;
	}
	break;
      case K_LCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(long *) data;
	    *values++ = *( (long *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_UBYTE:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(unsigned char *) data;
	    *values++ = 0.0;
	}
	break;
      case K_UINT:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(unsigned int *) data;
	    *values++ = 0.0;
	}
	break;
      case K_USHORT:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(unsigned short *) data;
	    *values++ = 0.0;
	}
	break;
      case K_ULONG:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(unsigned long *) data;
	    *values++ = 0.0;
	}
	break;
      case K_UBCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(unsigned char *) data;
	    *values++ = *( (unsigned char *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_UICOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(unsigned int *) data;
	    *values++ = *( (unsigned int *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_USCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(unsigned short *) data;
	    *values++ = *( (unsigned short *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_ULCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *values++ = *(unsigned long *) data;
	    *values++ = *( (unsigned long *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      default:
	/*  Unknown data type   */
	(void) fprintf (stderr, "%s: Illegal data type: %u\n",
			function_name, data_type);
	a_prog_bug (function_name);
	break;
    }
    if (complex != NULL)
    {
	*complex = tmp_complex;
    }
    return (TRUE);
}   /*  End Function misalign__ds_get_elements  */

#endif  /*  NEEDS_MISALIGN_COMPILE  */
