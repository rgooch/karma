/*LINTLIBRARY*/
/*  get.c

    This code provides miscellaneous Karma data structure manipulation routines

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

    This file contains the various utility routines for getting info on the
    general data structure supported in Karma.


    Written by      Richard Gooch   17-SEP-1992

    Updated by      Richard Gooch   26-NOV-1992

    Updated by      Richard Gooch   3-JAN-1993: Added support for K_ARRAY

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   30-JAN-1993: Improved error diagnostic in
  ds_f_name_in_packet  .

    Updated by      Richard Gooch   21-MAR-1993: Added explicit support for
  misaligned data by copying specific functions.

    Updated by      Richard Gooch   17-MAY-1993: Added support for string data
  types and created  ds_element_is_named  .

    Updated by      Richard Gooch   29-JUN-1993: Added more error trapping to
  ds_get_element  and  ds_get_elements  .

    Updated by      Richard Gooch   16-NOV-1993: Added support for string data
  types in  ds_f_name_in_packet  by switching to use of  ds_element_is_named  .

    Updated by      Richard Gooch   7-AUG-1994: Added some CONST declarations.

    Updated by      Richard Gooch   23-OCT-1994: Created
  ds_get_scattered_elements  routine.

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ds/get.c

    Updated by      Richard Gooch   2-JAN-1995: Changed some comments.

    Updated by      Richard Gooch   23-FEB-1995: Created
  <ds_can_transfer_packet_as_block> routine.

    Updated by      Richard Gooch   19-APR-1995: Cleaned some code.

    Updated by      Richard Gooch   24-APR-1995: Added support for integer
  toobig values for K_BYTE, K_INT, K_SHORT and complex versions thereof.

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   2-JUN-1995: Cope with unaligned data in
  <ds_get_element>.

    Updated by      Richard Gooch   3-JUN-1995: Made <ds_convert_atomic> call
  <ds_get_element> instead. Made <ds_get_elements> call <ds_get_element> for
  misaligned data on sensitive machines.

    Updated by      Richard Gooch   9-JUN-1995: Explicitly declared integer
  data types to be signed: crayPVP does not have compile switch to treat all
  "char" types as signed.

    Updated by      Richard Gooch   15-JUN-1995: Made use of IS_ALIGNED macro.

    Updated by      Richard Gooch   11-JAN-1995: Minor documentation
  improvement for <ds_get_element>.

    Updated by      Richard Gooch   21-JAN-1995: Documented that
  <ds_get_scattered_elements> is MT-Safe.

    Updated by      Richard Gooch   9-APR-1996: Changed to new documentation
  format.

    Uupdated by     Richard Gooch   30-MAY-1996: Moved a few routines to CONST
  packet descriptor pointers.

    Updated by      Richard Gooch   3-JUN-1996: Took account of new fields in
  dimension descriptor for first and last co-ordinate.

    Updated by      Richard Gooch   25-JUN-1996: Created
  <ds_can_swaptransfer_element>.

    Updated by      Richard Gooch   28-JUN-1996: Changed more pointers to
  CONST.

    Updated by      Richard Gooch   29-JUN-1996: Fixed bug in
  <ds_can_swaptransfer_element> where some swappable types were not correctly
  reported as being such.

    Updated by      Richard Gooch   20-JUL-1996: Changed more pointers to
  CONST.

    Updated by      Richard Gooch   2-AUG-1996: Changed type of co-ordinate
  index parameter for <ds_get_coordinate> to double.

    Updated by      Richard Gooch   1-NOV-1996: Moved some code into find.c and
  compare.c

    Last updated by Richard Gooch   3-NOV-1996: Improved blank trapping.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>
#define OS_H_VARIABLES
#include <os.h>


/*PUBLIC_FUNCTION*/
double ds_convert_atomic (CONST char *datum, unsigned int datum_type,
			  double *real_out, double *imag_out)
/*  [SUMMARY] Convert an atomic datum to a double precision value.
    <datum> The datum to be converted.
    <datum_type> The type of the datum. See [<DS_KARMA_DATA_TYPES>] for a list
    of legal values. The following data types are not convertible, and the
    routine will return the value TOOBIG:  NONE, K_ARRAY, LISTP, MULTI_ARRAY.
    <real_out> The real component of the data is written here. If this is NULL,
    nothing is written here. This must lie on a <<double>> boundary.
    <imag_out> The imaginary component of the data is written here. If this is
    NULL, nothing is written here. This must lie on a <<double>> boundary. For
    a real value, 0.0 is written.
    [RETURNS] The absolute magnitude of the converted value on success,
    else TOOBIG.
*/
{
    flag complex;
    double zero = 0.0;
    double value[2];
    /*static char function_name[] = "ds_convert_atomic";*/

    if ( !ds_get_element (datum, datum_type, value, &complex) ) return TOOBIG;
    if (real_out != NULL) *real_out = value[0];
    if (complex)
    {
	/*  Complex value   */
	if (imag_out != NULL) *imag_out = value[1];
	return ( sqrt (value[0] * value[0] + value[1] * value[1]) );
    }
    /*  Real value  */
    if (imag_out != NULL) *imag_out = zero;
    if (value[0] < zero) return (-value[0]);
    return (value[0]);
}   /*  End Function ds_convert_atomic  */

/*PUBLIC_FUNCTION*/
double ds_get_coordinate (CONST dim_desc *dimension, double coord_num)
/*  [SUMMARY] Get a co-ordinate along a dimension.
    <dimension> The dimension descriptor.
    <coord_num> The co-ordinate index.
    [RETURNS] The co-ordinate on success, else TOOBIG.
*/
{
    if (dimension == NULL)
    {
	/*  No dimension supplied   */
        return (TOOBIG);
    }
    if (coord_num >= dimension->length)
    {
	/*  Invalid co-ordinate number  */
        return (TOOBIG);
    }
    if (coord_num <= 0.0)
    {
	return (dimension->first_coord);
    }
    if (dimension->coordinates == NULL)
    {
	/*  Co-ordinate list not present: calculate co-ordinate */
        return ( dimension->first_coord + coord_num *
		 (dimension->last_coord - dimension->first_coord) /
		 (double) (dimension->length - 1) );
    }
    else
    {
	return (dimension->coordinates[(uaddr)coord_num]);
    }
}   /*  End Function ds_get_coordinate  */

/*PUBLIC_FUNCTION*/
unsigned int ds_get_element_offset (CONST packet_desc *pack_desc,
				    unsigned int elem_num)
/*  [SUMMARY] Calculate the offset of the start of a data element in a packet.
    <pack_desc> The descriptor for the data packet.
    <elem_num> The number of the element.
    [RETURNS] The byte offset of the element in the packet on success, else the
    length of the packet is returned.
*/
{
    unsigned int elem_count = 0;
    unsigned int byte_offset = 0;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_get_element_offset";

    if (pack_desc == NULL)
    {
	fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (elem_num >= pack_desc->num_elements)
    {
	elem_num = pack_desc->num_elements;
    }
    while (elem_count < elem_num)
    {
	byte_offset +=
	host_type_sizes[ pack_desc->element_types[elem_count++] ];
    }
    return (byte_offset);
}   /*  End Function ds_get_element_offset  */

/*PUBLIC_FUNCTION*/
unsigned int ds_get_packet_size (CONST packet_desc *pack_desc)
/*  [SUMMARY] Calculate size in bytes of a packet.
    <pack_desc> The packet descriptor.
    [RETURNS] The size in bytes.
*/
{
    static char function_name[] = "ds_get_packet_size";

    if (pack_desc == NULL)
    {
	fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    return ( ds_get_element_offset (pack_desc,
				    pack_desc->num_elements) );
}   /*  End Function ds_get_packet_size  */

/*PUBLIC_FUNCTION*/
unsigned long ds_get_array_size (CONST array_desc *arr_desc)
/*  [SUMMARY] Calculate the number of co-ordinate points in an array.
    <arr_desc> The array descriptor.
    [RETURNS] The size of the array (in co-ordinate points).
*/
{
    unsigned long array_points = 1;
    unsigned int dim_count = 0;
    static char function_name[] = "ds_get_array_size";

    if (arr_desc == NULL)
    {
	fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while (dim_count < arr_desc->num_dimensions)
    {
	array_points *= arr_desc->dimensions[dim_count++]->length;
    }
    return (array_points);
}   /*  End Function ds_get_array_size  */

/*PUBLIC_FUNCTION*/
flag ds_packet_all_data (CONST packet_desc *pack_desc)
/*  [SUMMARY] Test if packet contains only atomic data elements.
    [PURPOSE] This routine will determine if all the elements in a packet are
    atomic (i.e. no sub-arrays, linked lists or strings). All element types in
    the packet descriptor must be legal, else the routine will print an error
    message and abort processing.
    <pack_desc> The packet descriptor.
    [RETURNS] TRUE if the data elements are all atomic, else FALSE.
*/
{
    unsigned int elem_count = 0;
    static char function_name[] = "ds_packet_all_data";

    if (pack_desc == NULL)
    {
	fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while (elem_count < pack_desc->num_elements)
    {
	if ( !ds_element_is_atomic (pack_desc->element_types[elem_count]) )
        {
	    return (FALSE);
        }
        ++elem_count;
    }
    return (TRUE);
}   /*  End Function ds_packet_all_data  */

/*PUBLIC_FUNCTION*/
flag ds_element_is_atomic (unsigned int element_type)
/*  [SUMMARY] Test if an element is atomic.
    [PURPOSE] This routine will determine if an element is atomic (i.e. not a
    sub-array, linked list or string). The element type must be legal, else the
    routine will print an error message and abort processing.
    <element_type> The type of the element.
    [RETURNS] TRUE if the element is atomic, else FALSE.
*/
{
    static char function_name[] = "ds_element_is_atomic";

    switch (element_type)
    {
      case K_FLOAT:
      case K_DOUBLE:
      case K_BYTE:
      case K_INT:
      case K_SHORT:
      case K_COMPLEX:
      case K_DCOMPLEX:
      case K_BCOMPLEX:
      case K_ICOMPLEX:
      case K_SCOMPLEX:
      case K_LONG:
      case K_LCOMPLEX:
      case K_UBYTE:
      case K_UINT:
      case K_USHORT:
      case K_ULONG:
      case K_UBCOMPLEX:
      case K_UICOMPLEX:
      case K_USCOMPLEX:
      case K_ULCOMPLEX:
	/*  Atomic data type  */
	break;
      case K_VSTRING:
      case K_FSTRING:
      case K_ARRAY:
      case LISTP:
	/*  Not atomic data type  */
	return (FALSE);
	/*break;*/
      default:
	/*  Illegal data type  */
	fprintf (stderr, "Illegal data type: %u\n", element_type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ds_element_is_atomic  */

/*PUBLIC_FUNCTION*/
flag ds_element_is_named (unsigned int element_type)
/*  [SUMMARY] Test if an element is a named data type.
    [PURPOSE] This routine will determine if an element is a named data type
    (i.e. not a sub-array or linked list). The element type must be legal, else
    the routine will print an error message and abort processing.
    <element_type> The type of the element.
    [RETURNS] TRUE if the element is a named type, else FALSE.
*/
{
    static char function_name[] = "ds_element_is_named";

    switch (element_type)
    {
      case K_FLOAT:
      case K_DOUBLE:
      case K_BYTE:
      case K_INT:
      case K_SHORT:
      case K_COMPLEX:
      case K_DCOMPLEX:
      case K_BCOMPLEX:
      case K_ICOMPLEX:
      case K_SCOMPLEX:
      case K_LONG:
      case K_LCOMPLEX:
      case K_UBYTE:
      case K_UINT:
      case K_USHORT:
      case K_ULONG:
      case K_UBCOMPLEX:
      case K_UICOMPLEX:
      case K_USCOMPLEX:
      case K_ULCOMPLEX:
      case K_VSTRING:
      case K_FSTRING:
	/*  Atomic data type  */
	break;
      case K_ARRAY:
      case LISTP:
	/*  Not atomic data type  */
	return (FALSE);
	/*break;*/
      default:
	/*  Illegal data type  */
	fprintf (stderr, "Illegal data type: %u\n", element_type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ds_element_is_named  */

/*PUBLIC_FUNCTION*/
flag ds_element_is_legal (unsigned int element_type)
/*  [SUMMARY] Test if an element is legal.
    <element_type> The type of the element.
    [RETURNS] TRUE if the element type is legal, else FALSE.
*/
{
    switch (element_type)
    {
      case K_FLOAT:
      case K_DOUBLE:
      case K_BYTE:
      case K_INT:
      case K_SHORT:
      case K_COMPLEX:
      case K_DCOMPLEX:
      case K_BCOMPLEX:
      case K_ICOMPLEX:
      case K_SCOMPLEX:
      case K_LONG:
      case K_LCOMPLEX:
      case K_UBYTE:
      case K_UINT:
      case K_USHORT:
      case K_ULONG:
      case K_UBCOMPLEX:
      case K_UICOMPLEX:
      case K_USCOMPLEX:
      case K_ULCOMPLEX:
      case K_VSTRING:
      case K_FSTRING:
      case K_ARRAY:
      case LISTP:
	/*  Legal data type */
	break;
      default:
	/*  Illegal data type   */
	return (FALSE);
	/*break;*/
    }
    return (TRUE);
}   /*  End Function ds_element_is_legal  */

/*PUBLIC_FUNCTION*/
unsigned long ds_get_array_offset (CONST array_desc *arr_desc,
				   CONST unsigned long *coordinates)
/*  [SUMMARY] Compute offset of a co-ordinate in an array.
    <arr_desc> The array descriptor.
    <coordinates> The array of dimension co-ordinates which specifies the
    co-ordinate.
    [RETURNS] The offset of the co-ordinate in packets.
*/
{
    unsigned int dim_count;
    unsigned long offset = 0;
    unsigned long current_length;
    unsigned long dim_scale = 1;
    static char function_name[] = "ds_get_array_offset";

    if ( (arr_desc == NULL) || (coordinates == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    for (dim_count = arr_desc->num_dimensions; dim_count > 0; --dim_count)
    {
	current_length = arr_desc->dimensions[dim_count - 1]->length;
	if (coordinates[dim_count - 1] >= current_length)
	{
	    fprintf (stderr,
			    "Coordinate[%u]: %lu >= dimension length: %lu\n",
			    dim_count - 1, coordinates[dim_count - 1],
			    current_length);
	    a_prog_bug (function_name);
	}
	offset += coordinates[dim_count - 1] * dim_scale;
	dim_scale *= current_length;
    }
    return (offset);
}   /*  End Function ds_get_array_offset  */

/*PUBLIC_FUNCTION*/
unsigned long ds_get_coord_num (CONST dim_desc *dimension, double coordinate,
				unsigned int bias)
/*  [SUMMARY] Get index of a co-ordinate along a dimension.
    <dimension> The dimension descriptor.
    <coordinate> The co-ordinate to find.
    <bias> This specifies which co-ordinate index to pick when the co-ordinate
    lies between two dimension co-ordinates. See [<DS_SEARCH_BIASES>] for legal
    values.
    [RETURNS] The index number of the co-ordinate found.
*/
{
    unsigned long coord_num;
    double tmp_found_coord;
    static char function_name[] = "ds_get_coord_num";

    if (dimension == NULL)
    {
	fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if co-ordinate specified is within range
	of dimension co-ordinates  */
    if ( (dimension->first_coord < dimension->last_coord) &&
	 (coordinate <= dimension->first_coord) ) return (0);
    if ( (dimension->first_coord > dimension->last_coord) &&
	 (coordinate >= dimension->first_coord) ) return (0);
    if ( (dimension->first_coord < dimension->last_coord) &&
	 (coordinate >= dimension->last_coord) ) return (dimension->length -1);
    if ( (dimension->first_coord > dimension->last_coord) &&
	 (coordinate <= dimension->last_coord) ) return (dimension->length -1);
    if (dimension->coordinates == NULL)
    {
	/*  Dimension co-ordinates are regularly spaced  */
	coord_num = ( (coordinate - dimension->first_coord) /
		     (dimension->last_coord - dimension->first_coord) *
		     (double) (dimension->length - 1) );
    }
    else
    {
	/*  Search through co-ordinate list  */
	if (dimension->first_coord < dimension->last_coord)
	{
	    for (coord_num = 0;
		 dimension->coordinates[coord_num + 1] < coordinate;
		 ++coord_num);
	}
	else
	{
	    for (coord_num = 0;
		 dimension->coordinates[coord_num + 1] > coordinate;
		 ++coord_num);
	}
    }
    tmp_found_coord = ds_get_coordinate (dimension, coord_num);
    if (coordinate == tmp_found_coord)
    {
	/*  Exact match: return it  */
	return (coord_num);
    }
    switch (bias)
    {
      case SEARCH_BIAS_LOWER:
	/*  Co-ordinate number is lower one due to integer truncation  */
	if (dimension->first_coord < dimension->last_coord) return (coord_num);
	else return (coord_num + 1);
	/*break;*/
      case SEARCH_BIAS_CLOSEST:
	if ( fabs (coordinate - tmp_found_coord) <
	     fabs (ds_get_coordinate (dimension, coord_num + 1) - coordinate) )
	{
	    /*  Closest co-ordinate is lower one  */
	    return (coord_num);
	}
	else
	{
	    /*  Closest co-ordinate is upper one  */
	    return (coord_num + 1);
	}
	/*break;*/
      case SEARCH_BIAS_UPPER:
	if (dimension->first_coord < dimension->last_coord)
	{
	    return (coord_num + 1);
	}
	else return (coord_num);
	/*break;*/
      default:
	fprintf (stderr, "Illegal value of  bias  passed: %u\n", bias);
	a_prog_bug (function_name);
	break;
    }
    /*  Here to keep lint happy  */
    return (coord_num);
}   /*  End Function ds_get_coord_num  */

/*PUBLIC_FUNCTION*/
flag ds_get_element (CONST char *datum, unsigned int datum_type,
		     double value[2], flag *complex)
/*  [SUMMARY] Convert an atomic datum to a double precision complex value.
    <datum> A pointer to the datum to be converted.
    <datum_type> The type of the datum.
    <value> The data value will be written here.
    <complex> If the datum is a complex type, then the value of TRUE is written
    here, else the value FALSE is written here. If this is NULL, nothing is
    written here.
    [RETURNS] TRUE if the data was successfully converted, else FALSE.
*/
{
    flag tmp_complex = FALSE;
#ifdef NEED_ALIGNED_DATA
    int datum_size, count;
    char *ptr;
    double buf[2];
    extern char host_type_sizes[NUMTYPES];
#endif
    static char function_name[] = "ds_get_element";

    if (datum == NULL)
    {
	fprintf (stderr, "NULL datum pointer passed\n");
	a_prog_bug (function_name);
    }
    if (value == NULL)
    {
	fprintf (stderr, "NULL value storage pointer passed\n");
	a_prog_bug (function_name);
    }
#ifdef NEED_ALIGNED_DATA
    datum_size = host_type_sizes[datum_type];
    if ( (iaddr) datum % datum_size != 0 )
    {
	/*  Data is not aligned  */
	ptr = (char *) buf;
	for (count = 0; count < datum_size; ++count) ptr[count] = datum[count];
	datum = (CONST char *) buf;
    }
#endif  /*  NEED_ALIGNED_DATA  */
#ifdef MACHINE_i386
    if ( !IS_ALIGNED ( value, sizeof (float) ) )
#else
    if ( !IS_ALIGNED ( value, sizeof (double) ) )
#endif
    {
	fprintf (stderr,
			"value  address: %p not on a double boundary\n",
			value);
	a_prog_bug (function_name);
    }
    value[1] = 0.0;
    switch (datum_type)
    {
      case NONE:
      case K_ARRAY:
      case LISTP:
      case MULTI_ARRAY:
	fprintf (stderr, "Non-atomic data type: %u\n", datum_type);
	a_prog_bug (function_name);
	break;
      case K_FLOAT:
        value[0] = *(float *) datum;
	break;
      case K_DOUBLE:
	value[0] = *(double *) datum;
	break;
      case K_BYTE:
	if (*(signed char *) datum < -127) value[0] = TOOBIG;
	else value[0] = *(signed char *) datum;
	break;
      case K_INT:
	if (*(int *) datum < -2147483647) value[0] = TOOBIG;
	else value[0] = *(int *) datum;
	break;
      case K_SHORT:
	if (*(short *) datum < -32767) value[0] = TOOBIG;
        else value[0] = *(short *) datum;
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
	value[0] = *(signed char *) datum;
	value[1] = *( (signed char *) datum + 1 );
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
	/*  PATCH  */
	/*  The following  (float)  cast is required for the Sun bundled C
	    compiler. Otherwise, a segmentation fault can happen. Arrgghhh!  */
	value[0] = *(unsigned char *) datum;
	break;
      case K_UINT:
	/*  PATCH  */
	/*  The following  (float)  cast is required for the Sun bundled C
	    compiler. Otherwise, a segmentation fault can happen. Arrgghhh!  */
	value[0] = (float) *(unsigned int *) datum;
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
	fprintf (stderr, "Illegal data type: %u\n", datum_type);
	a_prog_bug (function_name);
	break;
    }
    if (complex != NULL)
    {
	*complex = tmp_complex;
    }
    return (TRUE);
}   /*  End Function ds_get_element  */

/*PUBLIC_FUNCTION*/
flag ds_get_elements (CONST char *data, unsigned int data_type,
		      unsigned int data_stride, double *values, flag *complex,
		      unsigned int num_values)
/*  [SUMMARY] Convert atomic data values to double precision complex values.
    <data> The data to be converted.
    <data_type> The type of the data.
    <data_stride> The stride of data elements in memory (in bytes).
    <values> The data values will be written here. This MUST lie on a
    <<double>> boundary.
    <complex> TRUE is written here if the data are a complex type, else FALSE
    is written here. If this is NULL, nothing is written here.
    <num_values> The number of data values to convert.
    [RETURNS] TRUE if the data was successfully converted, else FALSE.
*/
{
    flag tmp_complex = FALSE;
    double toobig = TOOBIG;
    unsigned int count;
    signed char *b_ptr;
    signed short *s_ptr;
    signed int *i_ptr;
#ifdef NEED_ALIGNED_DATA
    extern char host_type_sizes[NUMTYPES];
#endif
    static char function_name[] = "ds_get_elements";

    if (data == NULL)
    {
	fprintf (stderr, "NULL data pointer passed\n");
	a_prog_bug (function_name);
    }
    if (values == NULL)
    {
	fprintf (stderr, "NULL values storage pointer passed\n");
	a_prog_bug (function_name);
    }
#ifdef MACHINE_i386
    if ( !IS_ALIGNED ( values, sizeof (float) ) )
#else
    if ( !IS_ALIGNED ( values, sizeof (double) ) )
#endif
    {
	fprintf (stderr,
			"values  address: %p not on a double boundary\n",
			values);
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
	    if ( !ds_get_element (data, data_type, values,
				  complex) ) return (FALSE);
	}
	return (TRUE);
    }
#endif  /*  NEED_ALIGNED_DATA  */
    switch (data_type)
    {
      case NONE:
      case K_ARRAY:
      case LISTP:
      case MULTI_ARRAY:
	fprintf (stderr, "Non-atomic data type: %u\n", data_type);
	a_prog_bug (function_name);
	break;
      case K_FLOAT:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(float *) data;
	    *values++ = 0.0;
	}
	break;
      case K_DOUBLE:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(double *) data;
	    *values++ = 0.0;
	}
	break;
      case K_BYTE:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    b_ptr = (signed char *) data;
	    if (b_ptr[0] < -127) *values++ = toobig;
	    else *values++ = b_ptr[0];
	    *values++ = 0.0;
	}
	break;
      case K_INT:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    i_ptr = (signed int *) data;
	    if (i_ptr[0] < -2147483647) *values++ = toobig;
	    else *values++ = i_ptr[0];
	    *values++ = 0.0;
	}
	break;
      case K_SHORT:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    s_ptr = (signed short *) data;
	    if (s_ptr[0] < -32767) *values++ = toobig;
	    else *values++ = s_ptr[0];
	    *values++ = 0.0;
	}
	break;
      case K_COMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(float *) data;
	    *values++ = *( (float *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_DCOMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(double *) data;
	    *values++ = *( (double *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_BCOMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    b_ptr = (signed char *) data;
	    if (b_ptr[0] < -127) *values++ = toobig;
	    else *values++ = b_ptr[0];
	    if (b_ptr[1] < -127) *values++ = toobig;
	    else *values++ = b_ptr[1];
	}
	tmp_complex = TRUE;
	break;
      case K_ICOMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    i_ptr = (signed int *) data;
	    if (i_ptr[0] < -2147483647) *values++ = toobig;
	    else *values++ = i_ptr[0];
	    if (i_ptr[1] < -2147483647) *values++ = toobig;
	    else *values++ = i_ptr[1];
	}
	tmp_complex = TRUE;
	break;
      case K_SCOMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    s_ptr = (signed short *) data;
	    if (s_ptr[0] < -32767) *values++ = toobig;
	    else *values++ = s_ptr[0];
	    if (s_ptr[1] < -32767) *values++ = toobig;
	    else *values++ = s_ptr[1];
	}
	tmp_complex = TRUE;
	break;
      case K_LONG:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(signed long *) data;
	    *values++ = 0.0;
	}
	break;
      case K_LCOMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(signed long *) data;
	    *values++ = *( (signed long *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_UBYTE:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(unsigned char *) data;
	    *values++ = 0.0;
	}
	break;
      case K_UINT:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(unsigned int *) data;
	    *values++ = 0.0;
	}
	break;
      case K_USHORT:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(unsigned short *) data;
	    *values++ = 0.0;
	}
	break;
      case K_ULONG:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(unsigned long *) data;
	    *values++ = 0.0;
	}
	break;
      case K_UBCOMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(unsigned char *) data;
	    *values++ = *( (unsigned char *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_UICOMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(unsigned int *) data;
	    *values++ = *( (unsigned int *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_USCOMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(unsigned short *) data;
	    *values++ = *( (unsigned short *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_ULCOMPLEX:
	for (count = 0; count < num_values; ++count, data += data_stride)
	{
	    *values++ = *(unsigned long *) data;
	    *values++ = *( (unsigned long *) data + 1 );
	}
	tmp_complex = TRUE;
	break;
      default:
	/*  Unknown data type   */
	fprintf (stderr, "%s: Illegal data type: %u\n",
			function_name, data_type);
	a_prog_bug (function_name);
	break;
    }
    if (complex != NULL)
    {
	*complex = tmp_complex;
    }
    return (TRUE);
}   /*  End Function ds_get_elements  */

/*PUBLIC_FUNCTION*/
double *ds_get_coordinate_array (CONST dim_desc *dimension)
/*  [SUMMARY] Get co-ordinate array for a dimension.
    [PURPOSE] This routine will get a co-ordinate array for a dimension. If the
    dimension is regularly spaced, then the co-ordinate array is computed, else
    if it is irregularly spaced, it is copied from the dimension descriptor.
    <dimension> The dimension descriptor.
    [RETURNS] A pointer to a co-ordinate array on success, else NULL.
*/
{
    unsigned int coord_count;
    double *return_value;
    static char function_name[] = "ds_get_coordinate_array";

    if (dimension == NULL)
    {
	fprintf (stderr, "NULL descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( ( return_value = (double *) m_alloc (sizeof *return_value *
					      dimension->length) )
	== NULL )
    {
	m_error_notify (function_name, "co-ordinate array");
	return (NULL);
    }
    if (dimension->coordinates == NULL)
    {
	/*  Regularly spaced: compute  */
	for (coord_count = 0; coord_count < dimension->length; ++coord_count)
	{
	    return_value[coord_count] = ds_get_coordinate (dimension,
							   coord_count);
	}
    }
    else
    {
	/*  Irregularly spaced: copy  */
	for (coord_count = 0; coord_count < dimension->length; ++coord_count)
	{
	    return_value[coord_count] = dimension->coordinates[coord_count];
	}
    }
    return (return_value);
}   /*  End Function ds_get_coordinate_array  */

/*PUBLIC_FUNCTION*/
flag ds_element_is_complex (unsigned int element_type)
/*  [SUMMARY] Test if the type of an element is complex or not.
    <element_type> The element type. If this is not atomic the routine will
    print an error message and abort processing.
    [RETURNS] TRUE if the element type is complex, else FALSE.
*/
{
    flag complex = FALSE;
    static char function_name[] = "ds_element_is_complex";

    switch (element_type)
    {
      case K_FLOAT:
      case K_DOUBLE:
      case K_BYTE:
      case K_INT:
      case K_SHORT:
      case K_LONG:
      case K_UBYTE:
      case K_UINT:
      case K_USHORT:
      case K_ULONG:
	/*  Element is real  */
	break;
      case K_COMPLEX:
      case K_DCOMPLEX:
      case K_BCOMPLEX:
      case K_ICOMPLEX:
      case K_SCOMPLEX:
      case K_LCOMPLEX:
      case K_UBCOMPLEX:
      case K_UICOMPLEX:
      case K_USCOMPLEX:
      case K_ULCOMPLEX:
	/*  Complex data type  */
	complex = TRUE;
	break;
      default:
	/*  Illegal data type  */
	fprintf (stderr, "Illegal data type: %u\n",
			element_type);
	a_prog_bug (function_name);
	break;
    }
    return (complex);
}   /*  End Function ds_element_is_complex  */

/*PUBLIC_FUNCTION*/
flag ds_get_scattered_elements (CONST char *data, unsigned int data_type,
				CONST uaddr *offsets, double *values,
				flag *complex, unsigned int num_values)
/*  [SUMMARY] Convert scattered atomic data to double precision complex values.
    [PURPOSE] This routine will convert many atomic data to an array of double
    precision complex values. The data values may be scattered randomly (an
    offset array is used to index to the actual data).
    <data> The data to be converted. Misaligned data will cause bus errors on
    some platforms.
    <data_type> The type of the data.
    <offsets> The offset array (in bytes).
    <values> The data values will be written here. Must be a  double  boundary.
    <complex> If the data are a complex type, then the value of TRUE is
    written here, else the value FALSE is written here. If this is NULL,
    nothing is written here.
    <num_values> The number of data values to convert.
    [MT-LEVEL] Safe.
    [RETURNS] TRUE if the data was successfully converted, else FALSE.
*/
{
    flag tmp_complex = FALSE;
    signed char *b_data;
    signed short *s_data;
    signed int *i_data;
    unsigned int count;
    double zero = 0.0;
    double toobig = TOOBIG;
    CONST char *ptr;
    static char function_name[] = "ds_get_scattered_elements";

    if (data == NULL)
    {
	fprintf (stderr, "NULL data pointer passed\n");
	a_prog_bug (function_name);
    }
    if (values == NULL)
    {
	fprintf (stderr, "NULL values storage pointer passed\n");
	a_prog_bug (function_name);
    }
#ifdef MACHINE_i386
    if ( !IS_ALIGNED ( values, sizeof (float) ) )
#else
    if ( !IS_ALIGNED ( values, sizeof (double) ) )
#endif
    {
	fprintf (stderr,
			"values  address: %p not on a double boundary\n",
			values);
	a_prog_bug (function_name);
    }
    if (offsets == NULL)
    {
	fprintf (stderr, "NULL offset array pointer passed\n");
	a_prog_bug (function_name);
    }
    switch (data_type)
    {
      case NONE:
      case K_ARRAY:
      case LISTP:
      case MULTI_ARRAY:
	fprintf (stderr, "Non-atomic data type: %u\n", data_type);
	a_prog_bug (function_name);
	break;
      case K_FLOAT:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(float *) ptr;
	    *values++ = zero;
	}
	break;
      case K_DOUBLE:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(double *) ptr;
	    *values++ = zero;
	}
	break;
      case K_BYTE:
	for (count = 0; count < num_values; ++count)
	{
	    b_data = (signed char *) (data + offsets[count]);
	    if (b_data[0] < -127) *values++ = toobig;
	    else *values++ = b_data[0];
	    *values++ = zero;
	}
	break;
      case K_INT:
	for (count = 0; count < num_values; ++count)
	{
	    i_data = (signed int *) (data + offsets[count]);
	    if (i_data[0] == 0x80000000) *values++ = toobig;
	    else *values++ = i_data[0];
	    *values++ = zero;
	}
	break;
      case K_SHORT:
	for (count = 0; count < num_values; ++count)
	{
	    s_data = (signed short *) (data + offsets[count]);
	    if (s_data[0] == -32768) *values++ = toobig;
	    else *values++ = s_data[0];
	    *values++ = zero;
	}
	break;
      case K_COMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(float *) ptr;
	    *values++ = *( (float *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_DCOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(double *) ptr;
	    *values++ = *( (double *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_BCOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    b_data = (signed char *) (data + offsets[count]);
	    if (b_data[0] < -127) *values++ = toobig;
	    else *values++ = b_data[0];
	    if (b_data[1] < -127) *values++ = toobig;
	    else *values++ = b_data[1];
	}
	tmp_complex = TRUE;
	break;
      case K_ICOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    i_data = (signed int *) (data + offsets[count]);
	    if (i_data[0] == 0x80000000) *values++ = toobig;
	    else *values++ = i_data[0];
	    if (i_data[1] == 0x80000000) *values++ = toobig;
	    else *values++ = i_data[1];
	}
	tmp_complex = TRUE;
	break;
      case K_SCOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    s_data = (signed short *) (data + offsets[count]);
	    if (s_data[0] == -32768) *values++ = toobig;
	    else *values++ = s_data[0];
	    if (s_data[1] == -32768) *values++ = toobig;
	    else *values++ = s_data[1];
	}
	tmp_complex = TRUE;
	break;
      case K_LONG:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(signed long *) ptr;
	    *values++ = zero;
	}
	break;
      case K_LCOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(signed long *) ptr;
	    *values++ = *( (signed long *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_UBYTE:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(unsigned char *) ptr;
	    *values++ = zero;
	}
	break;
      case K_UINT:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(unsigned int *) ptr;
	    *values++ = zero;
	}
	break;
      case K_USHORT:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(unsigned short *) ptr;
	    *values++ = zero;
	}
	break;
      case K_ULONG:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(unsigned long *) ptr;
	    *values++ = zero;
	}
	break;
      case K_UBCOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(unsigned char *) ptr;
	    *values++ = *( (unsigned char *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_UICOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(unsigned int *) ptr;
	    *values++ = *( (unsigned int *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_USCOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(unsigned short *) ptr;
	    *values++ = *( (unsigned short *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_ULCOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(unsigned long *) ptr;
	    *values++ = *( (unsigned long *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      default:
	/*  Unknown data type   */
	fprintf (stderr, "%s: Illegal data type: %u\n",
		 function_name, data_type);
	a_prog_bug (function_name);
	break;
    }
    if (complex != NULL) *complex = tmp_complex;
    return (TRUE);
}   /*  End Function ds_get_scattered_elements  */

/*PUBLIC_FUNCTION*/
flag ds_can_transfer_element_as_block (unsigned int type)
/*  [SUMMARY] Test if an element can be transferred in one block.
    [PURPOSE] This routine will determine if an element can be transferred as a
    single block of data (i.e. no conversion between host and network format is
    needed).
    <type> The type of the element.
    [RETURNS] TRUE if the element may be transferred as a single block, else
    FALSE.
*/
{
#ifdef MACHINE_BIG_ENDIAN
    extern char host_type_sizes[NUMTYPES];
    extern char network_type_bytes[NUMTYPES];
#endif
    static char function_name[] = "ds_can_transfer_element_as_block";

    switch (type)
    {
      case K_BYTE:
      case K_BCOMPLEX:
      case K_UBYTE:
      case K_UBCOMPLEX:
	/*  Byte types are always transferrable  */
	break;
      case K_FLOAT:
      case K_DOUBLE:
      case K_COMPLEX:
      case K_DCOMPLEX:
#ifndef HAS_IEEE
	return (FALSE);
#endif
      case K_INT:
      case K_SHORT:
      case K_ICOMPLEX:
      case K_SCOMPLEX:
      case K_LONG:
      case K_LCOMPLEX:
      case K_UINT:
      case K_USHORT:
      case K_ULONG:
      case K_UICOMPLEX:
      case K_USCOMPLEX:
      case K_ULCOMPLEX:
	/*  Non-byte atomic data types  */
#ifdef MACHINE_BIG_ENDIAN
	if (host_type_sizes[type] != network_type_bytes[type]) return (FALSE);
	break;
#else
	return (FALSE);
#endif
      case K_VSTRING:
      case K_FSTRING:
      case K_ARRAY:
      case LISTP:
	/*  Not atomic data type  */
	return (FALSE);
	/*break;*/
      default:
	/*  Illegal data type  */
	fprintf (stderr, "Illegal data type: %u\n", type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ds_can_transfer_element_as_block  */

/*PUBLIC_FUNCTION*/
flag ds_can_transfer_packet_as_block (CONST packet_desc *pack_desc)
/*  [SUMMARY] Test if a packet can be transferred in one block.
    [PURPOSE] This routine will determine if a packet can be transferred as a
    single block of data (i.e. no conversion between host and network format is
    needed).
    <pack_desc> A pointer to the packet descriptor.
    [RETURNS] TRUE if the packet may be transferred as a single block, else
    FALSE.
*/
{
    unsigned int elem_count;
    unsigned int type;
#ifdef MACHINE_BIG_ENDIAN
    extern char host_type_sizes[NUMTYPES];
    extern char network_type_bytes[NUMTYPES];
#endif
    static char function_name[] = "ds_can_transfer_packet_as_block";

    if (pack_desc == NULL)
    {
	fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    for (elem_count = 0; elem_count < pack_desc->num_elements; ++elem_count)
    {
	type = pack_desc->element_types[elem_count];
	switch (type)
	{
	  case K_BYTE:
	  case K_BCOMPLEX:
	  case K_UBYTE:
	  case K_UBCOMPLEX:
	    /*  Byte types are always transferrable  */
	    break;
	  case K_FLOAT:
	  case K_DOUBLE:
	  case K_COMPLEX:
	  case K_DCOMPLEX:
#ifndef HAS_IEEE
	    return (FALSE);
#endif
	  case K_INT:
	  case K_SHORT:
	  case K_ICOMPLEX:
	  case K_SCOMPLEX:
	  case K_LONG:
	  case K_LCOMPLEX:
	  case K_UINT:
	  case K_USHORT:
	  case K_ULONG:
	  case K_UICOMPLEX:
	  case K_USCOMPLEX:
	  case K_ULCOMPLEX:
	    /*  Non-byte atomic data types  */
#ifdef MACHINE_BIG_ENDIAN
	    if (host_type_sizes[type] !=network_type_bytes[type]) return FALSE;
	    break;
#else
	    return (FALSE);
#endif
	  case K_VSTRING:
	  case K_FSTRING:
	  case K_ARRAY:
	  case LISTP:
	    /*  Not atomic data type  */
	    return (FALSE);
	    /*break;*/
	  default:
	    /*  Illegal data type  */
	    fprintf (stderr, "Illegal data type: %u\n", type);
	    a_prog_bug (function_name);
	    break;
	}
    }
    return (TRUE);
}   /*  End Function ds_can_transfer_packet_as_block  */

/*PUBLIC_FUNCTION*/
flag ds_can_swaptransfer_element (unsigned int type)
/*  [SUMMARY] Test if an element can be swapped and transferred in one block.
    [PURPOSE] This routine will determine if an element can be transferred as a
    single block of data with swapping (i.e. no extra conversion other than
    byte-swapping between host and network format is needed).
    <type> The type of the element.
    [RETURNS] TRUE if the element may be byteswapped and transferred in a
    single block, else FALSE.
*/
{
#ifdef MACHINE_LITTLE_ENDIAN
    extern char host_type_sizes[NUMTYPES];
    extern char network_type_bytes[NUMTYPES];
#endif
    static char function_name[] = "ds_can_swaptransfer_element";

    switch (type)
    {
      case K_BYTE:
      case K_BCOMPLEX:
      case K_UBYTE:
      case K_UBCOMPLEX:
	/*  Byte types are always transferrable  */
	break;
      case K_FLOAT:
      case K_DOUBLE:
      case K_COMPLEX:
      case K_DCOMPLEX:
#ifndef HAS_IEEE
	return (FALSE);
#endif
      case K_INT:
      case K_SHORT:
      case K_ICOMPLEX:
      case K_SCOMPLEX:
      case K_LONG:
      case K_LCOMPLEX:
      case K_UINT:
      case K_USHORT:
      case K_ULONG:
      case K_UICOMPLEX:
      case K_USCOMPLEX:
      case K_ULCOMPLEX:
	/*  Non-byte atomic data types  */
#ifdef MACHINE_LITTLE_ENDIAN
	if (host_type_sizes[type] != network_type_bytes[type]) return (FALSE);
	break;
#else
	return (FALSE);
#endif
      case K_VSTRING:
      case K_FSTRING:
      case K_ARRAY:
      case LISTP:
	/*  Not atomic data type  */
	return (FALSE);
	/*break;*/
      default:
	/*  Illegal data type  */
	fprintf (stderr, "Illegal data type: %u\n", type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ds_can_swaptransfer_element  */
