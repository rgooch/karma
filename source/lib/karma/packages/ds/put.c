/*LINTLIBRARY*/
/*  put.c

    This code provides routines to put data into Karma data structures.

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

    This file contains the various utility routines for writing items into the
    general data structure supported in Karma.


    Written by      Richard Gooch   7-OCT-1992

    Updated by      Richard Gooch   14-NOV-1992

    Updated by      Richard Gooch   3-JAN-1993: Added support for K_ARRAY

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   3-MAY-1993: Improved
  ds_put_element_many_times  .

    Updated by      Richard Gooch   25-OCT-1993: Changed to ANSI function
  definitions, then changed it back again because this code is still compiled
  using the K&R compiler under SunOS (cc -misalign)

    Updated by      Richard Gooch   4-NOV-1993: Used automatic variable for
  TOOBIG value to improve speed.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ds/put.c

    Updated by      Richard Gooch   19-APR-1995: Cleaned some code.

    Updated by      Richard Gooch   5-JUN-1995: Added code to cope with
  misaligned data accesses on sensitive platforms.

    Updated by      Richard Gooch   9-JUN-1995: Explicitly declared integer
  data types to be signed: crayPVP does not have compile switch to treat all
  "char" types as signed.

    Updated by      Richard Gooch   9-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   6-JUN-1996: Write 0 to K_UBYTE for blanks.


*/
#include <stdio.h>
#include <sys/types.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>
#include <os.h>


/*PUBLIC_FUNCTION*/
char *ds_put_element (char *output, unsigned int type, double input[2])
/*  [SUMMARY] Write out an element of data.
    <output> A pointer to the output storage.
    <type> The type of the element to be written.
    <input> The input data.
    [RETURNS] The address of the next element on success, else NULL.
*/
{
    int datum_size;
    double toobig = TOOBIG;
    char *out_ptr;
    extern char host_type_sizes[NUMTYPES];
#ifdef NEED_ALIGNED_DATA
    int count;
    double buf[2];
#endif
    static char function_name[] = "ds_put_element";

    if ( (output == NULL) || (input == NULL) )
    {
	return (NULL);
    }
    datum_size = host_type_sizes[type];
#ifdef NEED_ALIGNED_DATA
    if ( (iaddr) output % datum_size != 0 )
    {
	/*  Data is not aligned  */
	out_ptr = (char *) buf;
    }
    else
    {
	out_ptr = output;
    }
#else
    out_ptr = output;
#endif
    /*  Convert value back into specific type  */
    switch (type)
    {
      case K_FLOAT:
	*(float *) out_ptr = input[0];
	break;
      case K_DOUBLE:
	*(double *) out_ptr = input[0];
	break;
      case K_BYTE:
	if (input[0] >= toobig)
	{
	    *(signed char *) out_ptr = -128;
	}
	else
	{
	    *(signed char *) out_ptr = input[0];
	}
	break;
      case K_INT:
	*(signed int *) out_ptr = input[0];
	break;
      case K_SHORT:
	*(signed short *) out_ptr = input[0];
	break;
      case K_COMPLEX:
	*(float *) out_ptr = input[0];
	*( (float *) out_ptr + 1 ) = input[1];
	break;
      case K_DCOMPLEX:
	*(double *) out_ptr = input[0];
	*( (double *) out_ptr + 1 ) = input[1];
	break;
      case K_BCOMPLEX:
	*(signed char *) out_ptr = input[0];
	*( (signed char *) out_ptr + 1 ) = input[1];
	break;
      case K_ICOMPLEX:
	*(signed int *) out_ptr = input[0];
	*( (signed int *) out_ptr + 1 ) = input[1];
	break;
      case K_SCOMPLEX:
	*(signed short *) out_ptr = input[0];
	*( (signed short *) out_ptr + 1 ) = input[1];
	break;
      case K_LONG:
	*(signed long *) out_ptr = input[0];
	break;
      case K_LCOMPLEX:
	*(signed long *) out_ptr = input[0];
	*( (signed long *) out_ptr + 1 ) = input[1];
	break;
      case K_UBYTE:
	if (input[0] >= toobig) *(unsigned char *) out_ptr = 0;
	else *(unsigned char *) out_ptr = input[0];
	break;
      case K_UINT:
	*(unsigned int *) out_ptr = input[0];
	break;
      case K_USHORT:
	*(unsigned short *) out_ptr = input[0];
	break;
      case K_ULONG:
	*(unsigned long *) out_ptr = input[0];
	break;
      case K_UBCOMPLEX:
	*(unsigned char *) out_ptr = input[0];
	*( (unsigned char *) out_ptr + 1 ) = input[1];
	break;
      case K_UICOMPLEX:
	*(unsigned int *) out_ptr = input[0];
	*( (unsigned int *) out_ptr + 1 ) = input[1];
	break;
      case K_USCOMPLEX:
	*(unsigned short *) out_ptr = input[0];
	*( (unsigned short *) out_ptr + 1 ) = input[1];
	break;
      case K_ULCOMPLEX:
	*(unsigned long *) out_ptr = input[0];
	*( (unsigned long *) out_ptr + 1 ) = input[1];
	break;
      default:
	(void) fprintf (stderr, "Illegal data type: %u\n", type);
	a_prog_bug (function_name);
	break;
    }
#ifdef NEED_ALIGNED_DATA
    if (out_ptr != output)
    {
	for (count = 0; count < datum_size; ++count)
	output[count] = out_ptr[count];
    }
#endif
    return (output + datum_size);
}   /*  End Function ds_put_element  */

/*PUBLIC_FUNCTION*/
flag ds_put_elements (char *data, unsigned int data_type,
		      unsigned int data_stride, double *values,
		      unsigned int num_values)
/*  [SUMMARY] Convert array of double precision complex data to atomic data.
    [PURPOSE] This routine will convert an array of double precision complex
    values to an array of atomic data.
    <data> The array of output data.
    <data_type> The type of the data.
    <data_stride> The stride of data elements in memory (in bytes).
    <values> The data values will be read from here.
    <num_values> The number of data values to convert.
    [RETURNS] TRUE if the data was successfully converted, else FALSE.
*/
{
    unsigned int count;
    double toobig = TOOBIG;
#ifdef NEED_ALIGNED_DATA
    extern char host_type_sizes[NUMTYPES];
#endif
    static char function_name[] = "ds_put_elements";

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
#ifdef NEED_ALIGNED_DATA
    if ( ( (iaddr) data % host_type_sizes[data_type] != 0 ) ||
	( (iaddr) data_stride % host_type_sizes[data_type] != 0 ) )
    {
	/*  Not all data is aligned  */
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    if (ds_put_element (data, data_type, values) == NULL )return FALSE;
	}
	return (TRUE);
    }
#endif
    switch (data_type)
    {
      case NONE:
      case K_ARRAY:
      case LISTP:
      case MULTI_ARRAY:
	(void) fprintf (stderr, "%s: Non-atomic data type: %u\n",
			function_name, data_type);
	return (FALSE);
/*
	break;
*/
      case K_FLOAT:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    *(float *) data = *values;
	}
	break;
      case K_DOUBLE:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    *(double *) data = *values;
	}
	break;
      case K_BYTE:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    if (*values >= toobig) *(signed char *) data = -128;
	    else *(signed char *) data = *values;
	}
	break;
      case K_INT:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    *(signed int *) data = *values;
	}
	break;
      case K_SHORT:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    *(signed short *) data = *values;
	}
	break;
      case K_COMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(float *) data = *values++;
	    *( (float *) data + 1 ) = *values++;
	}
	break;
      case K_DCOMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(double *) data = *values++;
	    *( (double *) data + 1 ) = *values++;
	}
	break;
      case K_BCOMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(signed char *) data = *values++;
	    *( (signed char *) data + 1 ) = *values++;
	}
	break;
      case K_ICOMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(signed int *) data = *values++;
	    *( (signed int *) data + 1 ) = *values++;
	}
	break;
      case K_SCOMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(signed short *) data = *values++;
	    *( (signed short *) data + 1 ) = *values++;
	}
	break;
      case K_LONG:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    *(signed long *) data = *values;
	}
	break;
      case K_LCOMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(signed long *) data = *values++;
	    *( (signed long *) data + 1 ) = *values++;
	}
	break;
      case K_UBYTE:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    if (*values >= toobig) *(unsigned char *) data = 0;
	    else *(unsigned char *) data = *values;
	}
	break;
      case K_UINT:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    *(unsigned int *) data = *values;
	}
	break;
      case K_USHORT:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    *(unsigned short *) data = *values;
	}
	break;
      case K_ULONG:
	for (count = 0; count < num_values;
	     ++count, data += data_stride, values += 2)
	{
	    *(unsigned long *) data = *values;
	}
	break;
      case K_UBCOMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(unsigned char *) data = *values++;
	    *( (unsigned char *) data + 1 ) = *values++;
	}
	break;
      case K_UICOMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(unsigned int *) data = *values++;
	    *( (unsigned int *) data + 1 ) = *values++;
	}
	break;
      case K_USCOMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(unsigned short *) data = *values++;
	    *( (unsigned short *) data + 1 ) = *values++;
	}
	break;
      case K_ULCOMPLEX:
	for (count = 0; count < num_values;
	     ++count, data += data_stride)
	{
	    *(unsigned long *) data = *values++;
	    *( (unsigned long *) data + 1 ) = *values++;
	}
	break;
      default:
	/*  Unknown data type   */
	(void) fprintf (stderr, "Illegal data type: %u\n", data_type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ds_put_elements  */

/*PUBLIC_FUNCTION*/
flag ds_put_element_many_times (char *data, unsigned int data_type,
				unsigned int data_stride, double value[2],
				unsigned int num_elem)
/*  [SUMMARY] Write a double precision complex value to atomic data many times.
    [PURPOSE] This routine will convert and write a double precision complex
    value to an array of atomic data elements.
    <data> The array of output data.
    <data_type> The type of the data.
    <data_stride> The stride of data elements in memory (in bytes).
    <value> The data value will be read from here.
    <num_elem> The number of data elements to write.
    [RETURNS] TRUE if the data was successfully converted, else FALSE.
*/
{
    signed int sir, sii;
    signed long slr, sli;
    unsigned int uir, uii;
    unsigned long ulr, uli;
    unsigned int count;
    float fr, fi;
    double dr, di;
    double toobig = TOOBIG;
#ifdef NEED_ALIGNED_DATA
    extern char host_type_sizes[NUMTYPES];
#endif
    static char function_name[] = "ds_put_element_many_times";

    if (data == NULL)
    {
	(void) fprintf (stderr, "NULL data pointer passed\n");
	a_prog_bug (function_name);
	return (FALSE);
    }
    if (value == NULL)
    {
	(void) fprintf (stderr, "NULL value storage pointer passed\n");
	a_prog_bug (function_name);
	return (FALSE);
    }
#ifdef NEED_ALIGNED_DATA
    if ( ( (iaddr) data % host_type_sizes[data_type] != 0 ) ||
	( (iaddr) data_stride % host_type_sizes[data_type] != 0 ) )
    {
	/*  Not all data is aligned  */
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    if (ds_put_element (data, data_type, value) == NULL ) return FALSE;
	}
	return (TRUE);
    }
#endif
    switch (data_type)
    {
      case NONE:
      case K_ARRAY:
      case LISTP:
      case MULTI_ARRAY:
	(void) fprintf (stderr, "Non-atomic data type: %u\n",
			data_type);
	return (FALSE);
/*
	break;
*/
      case K_FLOAT:
	fr = *value;
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(float *) data = fr;
	}
	break;
      case K_DOUBLE:
	dr = *value;
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(double *) data = dr;
	}
	break;
      case K_BYTE:
	if (*value >= toobig) sir = -128;
	else sir = *value;
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(signed char *) data = sir;
	}
	break;
      case K_INT:
	sir = *value;
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(signed int *) data = sir;
	}
	break;
      case K_SHORT:
	sir = *value;
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(signed short *) data = sir;
	}
	break;
      case K_COMPLEX:
	fr = value[0];
	fi = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(float *) data = fr;
	    *( (float *) data + 1 ) = fi;
	}
	break;
      case K_DCOMPLEX:
	dr = value[0];
	di = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(double *) data = dr;
	    *( (double *) data + 1 ) = di;
	}
	break;
      case K_BCOMPLEX:
	sir = value[0];
	sii = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(signed char *) data = sir;
	    *( (signed char *) data + 1 ) = sii;
	}
	break;
      case K_ICOMPLEX:
	sir = value[0];
	sii = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(signed int *) data = sir;
	    *( (signed int *) data + 1 ) = sii;
	}
	break;
      case K_SCOMPLEX:
	sir = value[0];
	sii = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(signed short *) data = sir;
	    *( (signed short *) data + 1 ) = sii;
	}
	break;
      case K_LONG:
	slr = value[0];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(signed long *) data = slr;
	}
	break;
      case K_LCOMPLEX:
	slr = value[0];
	sli = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(signed long *) data = slr;
	    *( (signed long *) data + 1 ) = sli;
	}
	break;
      case K_UBYTE:
	if (*value >= toobig) uir = 0;
	else uir = *value;
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(unsigned char *) data = uir;
	}
	break;
      case K_UINT:
	uir = *value;
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(unsigned int *) data = uir;
	}
	break;
      case K_USHORT:
	uir = *value;
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(unsigned short *) data = uir;
	}
	break;
      case K_ULONG:
	ulr = *value;
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(unsigned long *) data = ulr;
	}
	break;
      case K_UBCOMPLEX:
	uir = value[0];
	uii = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(unsigned char *) data = uir;
	    *( (unsigned char *) data + 1 ) = uii;
	}
	break;
      case K_UICOMPLEX:
	uir = value[0];
	uii = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(unsigned int *) data = uir;
	    *( (unsigned int *) data + 1 ) = uii;
	}
	break;
      case K_USCOMPLEX:
	uir = value[0];
	uii = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(unsigned short *) data = uir;
	    *( (unsigned short *) data + 1 ) = uii;
	}
	break;
      case K_ULCOMPLEX:
	ulr = value[0];
	uli = value[1];
	for (count = 0; count < num_elem; ++count, data += data_stride)
	{
	    *(unsigned long *) data = ulr;
	    *( (unsigned long *) data + 1 ) = uli;
	}
	break;
      default:
	/*  Unknown data type   */
	(void) fprintf (stderr, "Illegal data type: %u\n", data_type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ds_put_element_many_times  */

/*PUBLIC_FUNCTION*/
flag ds_put_named_element (packet_desc *pack_desc, char *packet,
			   CONST char *name, double value[2])
/*  [SUMMARY] Update a named element in a specified packet.
    <pack_desc> The packet descriptor.
    <packet> The packet.
    <name> The name of the element to update.
    <value> The value of the data.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int elem_index;

    /*  Test to see if named item exists  */
    if ( ( elem_index = ds_f_elem_in_packet (pack_desc, name) )
	>= pack_desc->num_elements )
    {
	(void) fprintf (stderr, "Element: \"%s\" not found\n", name);
	return (FALSE);
    }
    /*  Write in new data  */
    if (ds_put_element (packet + ds_get_element_offset (pack_desc, elem_index),
			pack_desc->element_types[elem_index], value) == NULL)
    {
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function ds_put_named_element  */
