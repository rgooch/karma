/*LINTLIBRARY*/
/*MISALIGNED*/
/*  put.c

    This code provides routines to put data into Karma data structures.

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

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/ds/put.c


*/
#include <stdio.h>
#include <sys/types.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>


/*PUBLIC_FUNCTION*/
char *ds_put_element (output, type, input)
/*  This routine will write out an element of data to the storage pointed to by
    output  .
    The type of the element must be in  type  .
    The input data must be pointed to by  input  .
    The routine returns the address of the next element on success, else it
    returns NULL.
*/
char *output;
unsigned int type;
double *input;
{
    double toobig = TOOBIG;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_put_element";

    if ( (output == NULL) || (input == NULL) )
    {
	return (NULL);
    }
    /*  Convert value back into type of primary array  */
    switch (type)
    {
      case K_FLOAT:
	*(float *) output = input[0];
	break;
      case K_DOUBLE:
	*(double *) output = input[0];
	break;
      case K_BYTE:
	if (input[0] >= toobig)
	{
	    *output = -128;
	}
	else
	{
	    *output = input[0];
	}
	break;
      case K_INT:
	*(int *) output = input[0];
	break;
      case K_SHORT:
	*(short *) output = input[0];
	break;
      case K_COMPLEX:
	*(float *) output = input[0];
	*( (float *) output + 1 ) = input[1];
	break;
      case K_DCOMPLEX:
	*(double *) output = input[0];
	*( (double *) output + 1 ) = input[1];
	break;
      case K_BCOMPLEX:
	*output = input[0];
	*(output + 1) = input[1];
	break;
      case K_ICOMPLEX:
	*(int *) output = input[0];
	*( (int *) output + 1 ) = input[1];
	break;
      case K_SCOMPLEX:
	*(short *) output = input[0];
	*( (short *) output + 1 ) = input[1];
	break;
      case K_LONG:
	*(long *) output = input[0];
	break;
      case K_LCOMPLEX:
	*(long *) output = input[0];
	*( (long *) output + 1 ) = input[1];
	break;
      case K_UBYTE:
	*(unsigned char *) output = input[0];
	break;
      case K_UINT:
	*(unsigned int *) output = input[0];
	break;
      case K_USHORT:
	*(unsigned short *) output = input[0];
	break;
      case K_ULONG:
	*(unsigned long *) output = input[0];
	break;
      case K_UBCOMPLEX:
	*(unsigned char *) output = input[0];
	*( (unsigned char *) output + 1 ) = input[1];
	break;
      case K_UICOMPLEX:
	*(unsigned int *) output = input[0];
	*( (unsigned int *) output + 1 ) = input[1];
	break;
      case K_USCOMPLEX:
	*(unsigned short *) output = input[0];
	*( (unsigned short *) output + 1 ) = input[1];
	break;
      case K_ULCOMPLEX:
	*(unsigned long *) output = input[0];
	*( (unsigned long *) output + 1 ) = input[1];
	break;
      default:
	(void) fprintf (stderr, "Illegal data type: %u\n", type);
	a_prog_bug (function_name);
	break;
    }
    return (output + host_type_sizes[type]);
}   /*  End Function ds_put_element  */

/*PUBLIC_FUNCTION*/
flag ds_put_elements (data,  data_type, data_stride, values, num_values)
/*  This routine will convert an array of double precision complex values to
    an array of atomic data.
    The array of output data must be pointed to by  data  and the data type
    value must be in  data_type  .
    The stride of data elements in memory (in bytes) must be given by
    data_stride  .
    The data values will be read from the storage pointed to by  values  .
    The number of data values to convert must be pointed to by  num_values  .
    The routine returns TRUE if the data was successfully converted,
    else it returns FALSE.
*/
char *data;
unsigned int data_type;
unsigned int data_stride;
double *values;
unsigned int num_values;
{
    unsigned int data_count;
    double toobig = TOOBIG;
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
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    *(float *) data = *values;
	}
	break;
      case K_DOUBLE:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    *(double *) data = *values;
	}
	break;
      case K_BYTE:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    if (*values >= toobig)
	    {
		*(char *) data = -128;
	    }
	    else
	    {
		*(char *) data = *values;
	    }
	}
	break;
      case K_INT:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    *(int *) data = *values;
	}
	break;
      case K_SHORT:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    *(short *) data = *values;
	}
	break;
      case K_COMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *(float *) data = *values++;
	    *( (float *) data + 1 ) = *values++;
	}
	break;
      case K_DCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *(double *) data = *values++;
	    *( (double *) data + 1 ) = *values++;
	}
	break;
      case K_BCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *(char *) data = *values++;
	    *( (char *) data + 1 ) = *values++;
	}
	break;
      case K_ICOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *(int *) data = *values++;
	    *( (int *) data + 1 ) = *values++;
	}
	break;
      case K_SCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *(short *) data = *values++;
	    *( (short *) data + 1 ) = *values++;
	}
	break;
      case K_LONG:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    *(long *) data = *values;
	}
	break;
      case K_LCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *(long *) data = *values++;
	    *( (long *) data + 1 ) = *values++;
	}
	break;
      case K_UBYTE:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    *(unsigned char *) data = *values;
	}
	break;
      case K_UINT:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    *(unsigned int *) data = *values;
	}
	break;
      case K_USHORT:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    *(unsigned short *) data = *values;
	}
	break;
      case K_ULONG:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride, values += 2)
	{
	    *(unsigned long *) data = *values;
	}
	break;
      case K_UBCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *(unsigned char *) data = *values++;
	    *( (unsigned char *) data + 1 ) = *values++;
	}
	break;
      case K_UICOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *(unsigned int *) data = *values++;
	    *( (unsigned int *) data + 1 ) = *values++;
	}
	break;
      case K_USCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
	{
	    *(unsigned short *) data = *values++;
	    *( (unsigned short *) data + 1 ) = *values++;
	}
	break;
      case K_ULCOMPLEX:
	for (data_count = 0; data_count < num_values;
	     ++data_count, data += data_stride)
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
flag ds_put_element_many_times (data,  data_type, data_stride, value, num_elem)
/*  This routine will convert and write a double precision complex value to
    an array of atomic data elements.
    The array of output data must be pointed to by  data  and the data type
    value must be in  data_type  .
    The stride of data elements in memory (in bytes) must be given by
    data_stride  .
    The data value will be read from the storage pointed to by  value  .
    The number of data elements to write to must be pointed to by  num_elem  .
    The routine returns TRUE if the data was successfully converted,
    else it returns FALSE.
*/
char *data;
unsigned int data_type;
unsigned int data_stride;
double *value;
unsigned int num_elem;
{
    int sir, sii;
    long slr, sli;
    unsigned int uir, uii;
    unsigned long ulr, uli;
    unsigned int data_count;
    float fr, fi;
    double dr, di;
    double toobig = TOOBIG;
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
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(float *) data = fr;
	}
	break;
      case K_DOUBLE:
	dr = *value;
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(double *) data = dr;
	}
	break;
      case K_BYTE:
	if (*value >= toobig)
	{
	    sir = -128;
	}
	else
	{
	    sir = *value;
	}
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(char *) data = sir;
	}
	break;
      case K_INT:
	sir = *value;
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(int *) data = sir;
	}
	break;
      case K_SHORT:
	sir = *value;
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(short *) data = sir;
	}
	break;
      case K_COMPLEX:
	fr = value[0];
	fi = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(float *) data = fr;
	    *( (float *) data + 1 ) = fi;
	}
	break;
      case K_DCOMPLEX:
	dr = value[0];
	di = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(double *) data = dr;
	    *( (double *) data + 1 ) = di;
	}
	break;
      case K_BCOMPLEX:
	sir = value[0];
	sii = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(char *) data = sir;
	    *( (char *) data + 1 ) = sii;
	}
	break;
      case K_ICOMPLEX:
	sir = value[0];
	sii = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(int *) data = sir;
	    *( (int *) data + 1 ) = sii;
	}
	break;
      case K_SCOMPLEX:
	sir = value[0];
	sii = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(short *) data = sir;
	    *( (short *) data + 1 ) = sii;
	}
	break;
      case K_LONG:
	slr = value[0];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(long *) data = slr;
	}
	break;
      case K_LCOMPLEX:
	slr = value[0];
	sli = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(long *) data = slr;
	    *( (long *) data + 1 ) = sli;
	}
	break;
      case K_UBYTE:
	uir = *value;
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(unsigned char *) data = uir;
	}
	break;
      case K_UINT:
	uir = *value;
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(unsigned int *) data = uir;
	}
	break;
      case K_USHORT:
	uir = *value;
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(unsigned short *) data = uir;
	}
	break;
      case K_ULONG:
	ulr = *value;
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(unsigned long *) data = ulr;
	}
	break;
      case K_UBCOMPLEX:
	uir = value[0];
	uii = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(unsigned char *) data = uir;
	    *( (unsigned char *) data + 1 ) = uii;
	}
	break;
      case K_UICOMPLEX:
	uir = value[0];
	uii = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(unsigned int *) data = uir;
	    *( (unsigned int *) data + 1 ) = uii;
	}
	break;
      case K_USCOMPLEX:
	uir = value[0];
	uii = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
	{
	    *(unsigned short *) data = uir;
	    *( (unsigned short *) data + 1 ) = uii;
	}
	break;
      case K_ULCOMPLEX:
	ulr = value[0];
	uli = value[1];
	for (data_count = 0; data_count < num_elem;
	     ++data_count, data += data_stride)
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
flag ds_put_named_element (pack_desc, packet, name, value)
/*  This routine will write a named element into a specified packet.
    The packet descriptor must be pointed to by  pack_desc  .
    The packet which is described by  pack_desc  must be pointed to by
    packet  .
    The name of the element must be pointed to by  name  .
    The value of the data must be pointed to by  value  .
    The routine returns TRUE on success, else it returns FALSE.
*/
packet_desc *pack_desc;
char *packet;
CONST char *name;
double *value;
{
    unsigned int elem_index;
    static char function_name[] = "ds_put_named_element";

    /*  Test to see if named item exists  */
    if ( ( elem_index = ds_f_elem_in_packet (pack_desc, name) )
	>= (*pack_desc).num_elements )
    {
	(void) fprintf (stderr, "Element: \"%s\" not found\n", name);
	return (FALSE);
    }
    /*  Write in new data  */
    (void) ds_put_element (packet + ds_get_element_offset (pack_desc,
							   elem_index),
			   (*pack_desc).element_types[elem_index], value);
    return (TRUE);
}   /*  End Function ds_put_named_element  */
