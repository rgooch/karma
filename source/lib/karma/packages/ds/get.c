/*LINTLIBRARY*/
/*  get.c

    This code provides miscellaneous Karma data structure manipulation routines

    Copyright (C) 1992,1993,1994,1995  Richard Gooch

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

    Last updated by Richard Gooch   2-JAN-1995: Changed some comments.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <os.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>


/*  External functions  */
#ifdef NEEDS_MISALIGN_COMPILE
EXTERN_FUNCTION (double misalign__ds_convert_atomic, (CONST char *datum,
						      unsigned int datum_type,
						      double *real_out,
						      double *imag_out) );
EXTERN_FUNCTION (flag misalign__ds_get_element, (CONST char *datum,
						 unsigned int datum_type,
						 double *value,
						 flag *complex) );
EXTERN_FUNCTION (flag misalign__ds_get_elements, (CONST char *data,
						  unsigned int data_type,
						  unsigned int data_stride,
						  double *values,
						  flag *complex,
						  unsigned int num_values) );
#endif  /*  NEEDS_MISALIGN_COMPILE  */

/*PUBLIC_FUNCTION*/
double ds_convert_atomic (CONST char *datum, unsigned int datum_type,
			  double *real_out, double *imag_out)
/*  This routine will convert an atomic datum to a double precision value.
    The datum to be converted must be pointed to by  datum  and the data type
    value must be in  datum_type  .
    Complex data types are converted to their absolute value.
    The following data types are not convertible, and the routine will return
    the value TOOBIG:
        NONE, K_ARRAY, LISTP, MULTI_ARRAY.
    The routine will write the real and imaginary components into the storage
    pointed to by  real_out  and  imag_out  ,respectively, if these pointers
    are not NULL. For a real value, the imaginary value is 0.0 .
    These storages MUST lie on a  double  boundary.
    The routine returns the converted value on success, else it returns TOOBIG.
*/
{
    double real;
    double imag;
    static char function_name[] = "ds_convert_atomic";

#ifdef NEEDS_MISALIGN_COMPILE
    extern char host_type_sizes[NUMTYPES];

    if ( (int) datum % host_type_sizes[datum_type] != 0 )
    {
	/*  Data is not aligned  */
	return ( misalign__ds_convert_atomic (datum, datum_type,
					      real_out, imag_out) );
    }
#endif  /*  NEEDS_MISALIGN_COMPILE  */

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
	(void) fprintf (stderr, "Bad data type\n");
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
	/*  PATCH  */
	/*  The following  (float)  cast is required for the Sun bundled C
	    compiler. Otherwise, a segmentation fault can happen. Arrgghhh!  */
	real = (float) *(unsigned char *) datum;
	break;
      case K_UINT:
	/*  PATCH  */
	/*  The following  (float)  cast is required for the Sun bundled C
	    compiler. Otherwise, a segmentation fault can happen. Arrgghhh!  */
	real = (float) *(unsigned int *) datum;
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
}   /*  End Function ds_convert_atomic  */

/*PUBLIC_FUNCTION*/
double ds_get_coordinate (dim_desc *dimension, unsigned long coord_num)
/*  This routine will calculate or extract from a list the co-ordinate number
    coord_num  in the dimension descriptor pointed to by  dimension  .
    The co-ordinate is returned. If the co-ordinate is not obtainable, for
    any reason, the value TOOBIG is returned.
*/
{
    static char function_name[] = "ds_get_coordinate";

    if (dimension == NULL)
    {
	/*  No dimension supplied   */
        return (TOOBIG);
    }
    if (coord_num >= (*dimension).length)
    {
	/*  Invalid co-ordinate number  */
        return (TOOBIG);
    }
    if (coord_num == 0)
    {
	return ( (*dimension).minimum );
    }
    if ( (*dimension).coordinates == NULL )
    {
	/*  Co-ordinate list not present: calculate co-ordinate */
        return ( (*dimension).minimum + (double) coord_num *
                ( (*dimension).maximum - (*dimension).minimum ) /
                (double) ( (*dimension).length - 1 ) );
    }
    else
    {
	return ( (*dimension).coordinates[coord_num] );
    }
}   /*  End Function ds_get_coordinate  */

/*PUBLIC_FUNCTION*/
unsigned int ds_get_element_offset (pack_desc, elem_num)
/*  This routine will calculate the byte offset of the start of a data element
    in a data packet. The descriptor for the data packet must be pointed to
    by  pack_desc  and the number of the element must be in  elem_num  .
    If  elem_num  is greater or equal to the number of elements in the packet,
    then the length of the packet is returned.
    The routine returns the byte offset of the element in the packet.
*/
packet_desc *pack_desc;
unsigned int elem_num;
{
    unsigned int elem_count = 0;
    unsigned int byte_offset = 0;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_get_element_offset";

    if (pack_desc == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (elem_num >= (*pack_desc).num_elements)
    {
	elem_num = (*pack_desc).num_elements;
    }
    while (elem_count < elem_num)
    {
	byte_offset +=
	host_type_sizes[ (*pack_desc).element_types[elem_count++] ];
    }
    return (byte_offset);
}   /*  End Function ds_get_element_offset  */

/*PUBLIC_FUNCTION*/
unsigned int ds_get_packet_size (pack_desc)
/*  This routine will calculate the size (in bytes) of a data packet pointed
    to by  pack_desc  .
    The routine returns the size in bytes.
*/
packet_desc *pack_desc;
{
    static char function_name[] = "ds_get_packet_size";

    if (pack_desc == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    return ( ds_get_element_offset (pack_desc,
				    (*pack_desc).num_elements) );
}   /*  End Function ds_get_packet_size  */

/*PUBLIC_FUNCTION*/
unsigned long ds_get_array_size (array_desc *arr_desc)
/*  This routine will calculate the number of co-ordinate points in an array.
    The array descriptor must be pointed to by  arr_desc  .
    The routine returns the size of the array.
*/
{
    unsigned long array_points = 1;
    unsigned int dim_count = 0;
    static char function_name[] = "ds_get_array_size";

    if (arr_desc == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while (dim_count < (*arr_desc).num_dimensions)
    {
	array_points *= ( *( (*arr_desc).dimensions[dim_count++] ) ).length;
    }
    return (array_points);
}   /*  End Function ds_get_array_size  */

/*PUBLIC_FUNCTION*/
flag ds_packet_all_data (pack_desc)
/*  This routine will determine if the packet descriptor pointed to by
    pack_desc  is composed only of atomic data elements (ie. no sub arrays
    or linked lists or strings).
    All element types in the packet descriptor must be legal, else the routine
    will print an error message and abort processing.
    The routine will return TRUE if the data elements are all atomic, else
    it will return FALSE.
*/
packet_desc *pack_desc;
{
    unsigned int elem_count = 0;
    static char function_name[] = "ds_packet_all_data";

    if (pack_desc == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while (elem_count < (*pack_desc).num_elements)
    {
	if (ds_element_is_atomic ( (*pack_desc).element_types[elem_count] )
            != TRUE)
        {
	    return (FALSE);
        }
        ++elem_count;
    }
    return (TRUE);
}   /*  End Function ds_packet_all_data  */

/*PUBLIC_FUNCTION*/
flag ds_element_is_atomic (element_type)
/*  This routine will determine if the type of an element is atomic or not.
    The element type must be in  element_type  .
    The element type must be legal, else the routine will print an error
    message and abort processing.
    The routine returns TRUE if the element type is atomic, else
    it returns FALSE.
*/
unsigned int element_type;
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
/*
	break;
*/
      default:
	/*  Illegal data type  */
	(void) fprintf (stderr, "Illegal data type: %u\n", element_type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ds_element_is_atomic  */

/*PUBLIC_FUNCTION*/
flag ds_element_is_named (element_type)
/*  This routine will determine if an element type is named or not.
    The element type must be in  element_type  .
    The element type must be legal, else the routine will print an error
    message and abort processing.
    The routine returns TRUE if the element type is atomic, else
    it returns FALSE.
*/
unsigned int element_type;
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
/*
	break;
*/
      default:
	/*  Illegal data type  */
	(void) fprintf (stderr, "Illegal data type: %u\n", element_type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ds_element_is_named  */

/*PUBLIC_FUNCTION*/
flag ds_element_is_legal (element_type)
/*  This routine will determine if the element type in  element_type  is a
    legal value.
    The routine returns TRUE if the element type is legal, else it
    returns FALSE.
*/
unsigned int element_type;
{
    static char function_name[] = "ds_element_is_legal";

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
/*
	break;
*/
    }
    return (TRUE);
}   /*  End Function ds_element_is_legal  */

/*PUBLIC_FUNCTION*/
unsigned int ds_identify_name (multi_desc, name, encls_desc, index)
/*  This routine will search the multi array general data structure with
    descriptor pointed to by  multi_desc  for an occurrence of the name
    pointed to by  name  .
    The routine will write a pointer to the enclosing descriptor of the item
    to the storage pointed to by  encls_desc  .
    The index (general data structure number, dimension number or element
    number) of the item in the enclosing structure will be written to the
    storage pointed to by  index  .
    If these are NULL, nothing is written there.
    The routine will return a code based on the type of the item with the
    same name. These codes are defined in the file: general_struct.h
*/
multi_array *multi_desc;
CONST char *name;
char **encls_desc;
unsigned int *index;
{
    unsigned int array_count = 0;
    unsigned int temp_ident;
    unsigned int return_value = IDENT_NOT_FOUND;
    static char function_name[] = "ds_identify_name";

    if (multi_desc == NULL)
    {
	return (IDENT_NOT_FOUND);
    }
    if ( (*multi_desc).headers == NULL )
    {
	(void) fprintf (stderr,
			"Multi array descriptor has no list of packet descriptors\n");
        a_prog_bug (function_name);
    }
    if (name == NULL)
    {
	return (IDENT_NOT_FOUND);
    }
    return_value = ds_f_array_name (multi_desc, name, encls_desc, index);
    while (array_count < (*multi_desc).num_arrays)
    {
	/*  Search one array    */
        if ( ( temp_ident =
	      ds_f_name_in_packet ( (*multi_desc).headers[array_count],
				   name, encls_desc, index ) )
            != IDENT_NOT_FOUND )
        {
	    if (return_value != IDENT_NOT_FOUND)
            {
		return (IDENT_MULTIPLE);
            }
            return_value = temp_ident;
        }
        ++array_count;
    }
    return (return_value);
}   /*  End Function ds_identify_name  */

/*PUBLIC_FUNCTION*/
unsigned int ds_f_array_name (multi_desc, name, encls_desc, index)
/*  This routine will search the multi array general data structure header
    pointed to by  multi_desc  for an occurrence of the array name pointed
    to by  name  .
    If it is found, the pointer to the multi array header will be written to
    the storage pointed to by  encls_desc  .
    The index number of the general data structure with name pointed to by
    name  will be written to the storage pointed to by  index  .
    If these are NULL, nothing is written here.
    Note that the routine will not search the packet descriptors for name
    matches.
    The routine will return a code based on the number of matches found.
    These codes are defined in the file: general_struct.h
*/
multi_array *multi_desc;
CONST char *name;
char **encls_desc;
unsigned int *index;
{
    unsigned int array_count = 0;
    unsigned int return_value = IDENT_NOT_FOUND;
    static char function_name[] = "ds_f_array_name";

    if (multi_desc == NULL)
    {
	return (IDENT_NOT_FOUND);
    }
    if ( (*multi_desc).headers == NULL )
    {
	(void) fprintf (stderr,
			"Multi array descriptor has no list of packet descriptors\n");
        a_prog_bug (function_name);
    }
    if ( (*multi_desc).num_arrays == 1 )
    {
	/*  Only one array  */
        if ( (*multi_desc).array_names != NULL )
        {
	    /*  Should be a NULL pointer    */
            (void) fprintf (stderr,
			    "Multi array descriptor has one array with name: \"%s\"\n",
			    (*multi_desc).array_names[0]);
            a_prog_bug (function_name);
        }
        if ( (name == NULL) || (*name == '\0') )
        {
	    /*  Only one general data structure array   */
            if (encls_desc != NULL)
            {
		*encls_desc = (char *) multi_desc;
            }
	    if (index != NULL)
	    {
		*index = 0;
	    }
            return_value = IDENT_GEN_STRUCT;
        }
        return (return_value);
    }
    /*  Many arrays */
    if ( (*multi_desc).array_names == NULL )
    {
	(void) fprintf (stderr,
			"Multi array descriptor has many arrays and no list of array names\n");
        a_prog_bug (function_name);
    }
    if (name == NULL)
    {
	/*  Return here, else strcmp will bomb out  */
	return (IDENT_NOT_FOUND);
    }
    while (array_count < (*multi_desc).num_arrays)
    {
	if ( (*multi_desc).array_names[array_count] == NULL )
        {
	    (void) fprintf (stderr, "Multiple arrays without names given\n");
            a_prog_bug (function_name);
        }
        /*  Compare  name  with name of an array    */
        if (strcmp (name, (*multi_desc).array_names[array_count]) == 0)
        {
	    /*  Array name match    */
            if (return_value != IDENT_NOT_FOUND)
            {
		return (IDENT_MULTIPLE);
            }
            return_value = IDENT_GEN_STRUCT;
	    if (index != NULL)
	    {
		*index = array_count;
	    }
        }
        ++array_count;
    }
    if (encls_desc != NULL)
    {
	*encls_desc = (char *) multi_desc;
    }
    return (return_value);
}   /*  End Function ds_f_array_name  */

/*PUBLIC_FUNCTION*/
unsigned int ds_f_name_in_packet (pack_desc, name, encls_desc, index)
/*  This routine will search for occurrences of the name pointed to by  name
    in the packet descriptor pointed to by  pack_desc  .
    If  name  is NULL, then the routine will not find anything.
    The routine will recursively search for names in sub array and linked list
    descriptors.
    The pointer to the enclosing structure of the named item will be written
    to the storage pointed to by  encls_desc  .
    The index (dimension number or element number) of the item will be written
    to the storage pointed to by  index  .
    If these are NULL, nothing is written there.
    The routine will return a code based on the type of the item with the
    same name. These codes are defined in the file: general_struct.h
*/
packet_desc *pack_desc;
CONST char *name;
char **encls_desc;
unsigned int *index;
{
    unsigned int elem_count;
    unsigned int temp_ident;
    unsigned int return_value = IDENT_NOT_FOUND;
    static char function_name[] = "ds_f_name_in_packet";

    if ( (pack_desc == NULL) || ( (*pack_desc).num_elements < 1 )
	|| (name == NULL) )
    {
	return (IDENT_NOT_FOUND);
    }
    for (elem_count = 0; elem_count < (*pack_desc).num_elements; ++elem_count)
    {
	if (ds_element_is_named ( (*pack_desc).element_types[elem_count] )
            == TRUE)
        {
	    /*  Atomic data type    */
	    if (strcmp (name, (*pack_desc).element_desc[elem_count])
		== 0)
	    {
		if (return_value != IDENT_NOT_FOUND)
		{
		    return (IDENT_MULTIPLE);
		}
		return_value = IDENT_ELEMENT;
		if (encls_desc != NULL)
		{
		    *encls_desc = (char *) pack_desc;
		}
		if (index != NULL)
		{
		    *index = elem_count;
		}
	    }
	    continue;
	}
	/*  Not a named element  */
	switch ( (*pack_desc).element_types[elem_count] )
	{
	  case K_ARRAY:
	    if ( ( temp_ident =
		  ds_f_name_in_array ( (array_desc *)
				      (*pack_desc).element_desc[elem_count],
				      name, encls_desc, index) )
		!= IDENT_NOT_FOUND )
	    {
		if (return_value != IDENT_NOT_FOUND)
		{
		    return (IDENT_MULTIPLE);
		}
		return_value = temp_ident;
	    }
	    break;
	  case LISTP:
	    if ( ( temp_ident =
		  ds_f_name_in_packet ( (packet_desc *)
				       (*pack_desc).element_desc[elem_count],
				       name, encls_desc, index) )
		!= IDENT_NOT_FOUND )
	    {
		if (return_value != IDENT_NOT_FOUND)
		{
		    return (IDENT_MULTIPLE);
		}
		return_value = temp_ident;
	    }
	    break;
	  default:
	    /*  Bad data type   */
	    (void) fprintf (stderr, "Bad element type value: %u\n",
			    (*pack_desc).element_types[elem_count]);
	    a_prog_bug (function_name);
	    break;
	}
    }
    return (return_value);
}   /*  End Function ds_f_name_in_packet  */

/*PUBLIC_FUNCTION*/
unsigned int ds_f_name_in_array (arr_desc, name, encls_desc, index)
/*  This routine will search for occurrences of the name pointed by  name
    in the array descriptor pointed to by  arr_desc  .
    If  name  is NULL, then the routine will not find anything.
    The routine searches both the dimension names and the packet associated
    with the array.
    The routine recursively searches the array packet descriptor.
    The pointer to the descriptor of the enclosing structure of the named item
    is written to the storage pointed to by  encls_desc  .
    The index (dimension number or element number) of the item will be written
    to the storage pointed to by  index  .
    If these are NULL, nothing is written there.
    The routine will return a code based on the type of the item with the
    same name. These codes are defined in the file: general_struct.h
*/
array_desc *arr_desc;
CONST char *name;
char **encls_desc;
unsigned int *index;
{
    unsigned int dim_count = 0;
    unsigned int temp_ident;
    unsigned int return_value = IDENT_NOT_FOUND;
    static char function_name[] = "ds_f_name_in_array";

    if ( (arr_desc == NULL) || (name == NULL) )
    {
	return (IDENT_NOT_FOUND);
    }
    while (dim_count < (*arr_desc).num_dimensions)
    {
	if (strcmp (name, (*(*arr_desc).dimensions[dim_count]).name) == 0)
        {
	    if (return_value != IDENT_NOT_FOUND)
            {
		return (IDENT_MULTIPLE);
            }
            if (encls_desc != NULL)
            {
		*encls_desc = (char *) arr_desc;
            }
	    if (index != NULL)
	    {
		*index = dim_count;
	    }
            return_value = IDENT_DIMENSION;
        }
        ++dim_count;
    }
    if ( (*arr_desc).packet == NULL )
    {
	return (return_value);
    }
    if ( ( temp_ident = ds_f_name_in_packet ( (*arr_desc).packet, name,
					  encls_desc, index ) )
        != IDENT_NOT_FOUND )
    {
	if (return_value != IDENT_NOT_FOUND)
        {
	    return (IDENT_MULTIPLE);
        }
        return_value = temp_ident;
    }
    return (return_value);
}   /*  End Function ds_f_name_in_array  */

/*PUBLIC_FUNCTION*/
unsigned int ds_f_elem_in_packet (pack_desc, name)
/*  This routine will determine if a particular named element is in the
    packet descriptor pointed to by  pack_desc  .The name of the element
    to find must be pointed to by  name  .
    If  name  is NULL, then the routine will not find anything.
    The routine will only find named elements in this packet: it is not
    recursive.
    The routine returns the number of the element in the packet if it was found
    else it returns the number of elements in the packet.
    If the specified name occurs twice, the program aborts.
*/
packet_desc *pack_desc;
CONST char *name;
{
    unsigned int elem_count = 0;
    unsigned int return_value;
    static char function_name[] = "ds_f_elem_in_packet";

    if (pack_desc == NULL)
    {
	(void) fprintf (stderr, "NULL  pack_desc  pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( ( (*pack_desc).num_elements < 1 ) || (name == NULL) )
    {
	return ( (*pack_desc).num_elements );
    }
    return_value = (*pack_desc).num_elements;
    while (elem_count < (*pack_desc).num_elements)
    {
	if (ds_element_is_named ( (*pack_desc).element_types[elem_count] )
            == TRUE)
        {
	    /*  Atomic data type    */
            if (strcmp (name, (*pack_desc).element_desc[elem_count]) == 0)
            {
		/*  Found atomic element name   */
                if (return_value < (*pack_desc).num_elements)
                {
		    (void) fprintf (stderr,
				    "Multiple occurrences of: \"%s\"\n",
				    name);
		    a_prog_bug (function_name);
                }   
                return_value = elem_count;
            }
        }
        ++elem_count;
    }
    return (return_value);
}   /*  End Function ds_f_elem_in_packet  */

/*PUBLIC_FUNCTION*/
unsigned int ds_find_hole (inp_desc, out_desc, elem_num)
/*  This routine will recursively search the packet descriptor pointed to by
    inp_desc  for a hole (element type NONE or element descriptor pointer NULL)
    A pointer to the packet descriptor which contains the hole is written to
    the storage pointed to by  out_desc  and the element number in that packet
    which corresponds to the hole is written to the storage pointed to by
    elem_num  .If these are NULL, nothing is written to them.
    The routine returns a value indicating the status of the search. These
    codes are defined in the file: general_struct.h
*/
packet_desc *inp_desc;
packet_desc **out_desc;
unsigned int *elem_num;
{
    unsigned int elem_count = 0;
    unsigned int temp_ident;
    unsigned int return_value = IDENT_NOT_FOUND;
    packet_desc *elem_desc;
    array_desc *arr_desc;
    static char function_name[] = "ds_find_hole";

    if (inp_desc == NULL)
    {
	return (IDENT_NOT_FOUND);
    }
    if ( (*inp_desc).num_elements < 1 )
    {
	return (IDENT_NOT_FOUND);
    }
    /*  Search for occurence of hole in this packet descriptor  */
    while (elem_count < (*inp_desc).num_elements)
    {
	elem_desc =(packet_desc *) (*inp_desc).element_desc[elem_count];
        if ( ( (*inp_desc).element_types[elem_count] == NONE ) ||
	    (elem_desc == NULL) )
        {
	    if (return_value != IDENT_NOT_FOUND)
            {
		return (IDENT_MULTIPLE);
            }
            return_value = IDENT_ELEMENT;
            *out_desc = inp_desc;
            *elem_num = elem_count;
        }
        if ( (*inp_desc).element_types[elem_count] == K_ARRAY )
        {
	    arr_desc = (array_desc *) elem_desc;
            if ( ( temp_ident = ds_find_hole ( (*arr_desc).packet, out_desc,
					      elem_num ) ) != IDENT_NOT_FOUND )
            {
		if (return_value != IDENT_NOT_FOUND)
                {
		    return (IDENT_MULTIPLE);
                }
                return_value = temp_ident;
            }
        }
        if ( (*inp_desc).element_types[elem_count] == LISTP )
        {
	    if ( ( temp_ident = ds_find_hole (elem_desc, out_desc, elem_num) )
                != IDENT_NOT_FOUND )
            {
		if (return_value != IDENT_NOT_FOUND)
                {
		    return (IDENT_MULTIPLE);
                }
                return_value = temp_ident;
            }
        }
        ++elem_count;
    }
    return (return_value);
}   /*  End Function ds_find_hole  */

/*PUBLIC_FUNCTION*/
flag ds_compare_packet_desc (desc1, desc2, recursive)
/*  This routine will compare two packet descriptors, pointed to by  desc1  and
    desc2  .
    The routine will perform a recursive comparison of sub arrays and linked
    list descriptors if the value of  recursive  is TRUE.
    The routine returns TRUE if the two packet descriptors are equal,
    else it returns FALSE.
*/
packet_desc *desc1;
packet_desc *desc2;
flag recursive;
{
    unsigned int elem_count = 0;
    unsigned int elem_type1;
    unsigned int elem_type2;
    char *elem_name1;
    char *elem_name2;
    static char function_name[] = "ds_compare_packet_desc";

    if ( (desc1 == NULL) || (desc2 == NULL) )
    {
	a_func_abort (function_name, "NULL descriptor pointer(s)");
        return (FALSE);
    }
    if ( (*desc1).num_elements != (*desc2).num_elements )
    {
	return (FALSE);
    }
    while (elem_count < (*desc1).num_elements)
    {
	elem_type1 = (*desc1).element_types[elem_count];
        elem_type2 = (*desc2).element_types[elem_count];
        elem_name1 = (*desc1).element_desc[elem_count];
        elem_name2 = (*desc2).element_desc[elem_count];
        if (elem_type1 == elem_type2)
        {
	    /*  Element types are the same  */
            if (ds_element_is_named (elem_type1) == TRUE)
            {
		if (strcmp (elem_name1, elem_name2) != 0)
                    return (FALSE);
            }
            else
            {
		if ( (recursive == TRUE) && (elem_type1 == K_ARRAY) )
                {
		    if (ds_compare_array_desc ( (array_desc *)
					       elem_name1,
					       (array_desc *)
					       elem_name2,
					       recursive) != TRUE)
                        return (FALSE);
                }
                if ( (recursive == TRUE) &&(elem_type1 == LISTP) )
                {
		    if (ds_compare_packet_desc ( (packet_desc *)
						elem_name1,
						(packet_desc *)
						elem_name2,
						recursive) != TRUE)
                        return (FALSE);             
                }
            }
        }
        else
        {
	    /*  Element types are not the same  */
            /*  Trap for bad element types  */
            if (ds_element_is_legal (elem_type1) != TRUE)
            {
		(void) fprintf (stderr, "Element type: %u is not legal\n",
				elem_type1);
                a_prog_bug (function_name);
            }
            if (ds_element_is_legal (elem_type2) != TRUE)
            {
		(void) fprintf (stderr, "Element type: %u is not legal\n",
				elem_type2);
                a_prog_bug (function_name);
            }
	    return (FALSE);
        }
        ++elem_count;
    }
    return (TRUE);
}   /*  End Function ds_compare_packet_desc  */

/*PUBLIC_FUNCTION*/
flag ds_compare_array_desc (desc1, desc2, recursive)
/*  This routine will compare two array descriptors, pointed to by  desc1  and
    desc2  .
    The routine will perform a recursive comparison of the array packet
    descriptors if the value of  recursive  is TRUE.
    The routine returns TRUE if the two array descriptors are equal,
    else it returns FALSE.
*/
array_desc *desc1;
array_desc *desc2;
flag recursive;
{
    unsigned int dim_count = 0;
    static char function_name[] = "ds_compare_array_desc";

    if ( (desc1 == NULL) || (desc2 == NULL) )
    {
	a_func_abort (function_name, "NULL descriptor pointer(s)");
        return (FALSE);
    }
    if ( (*desc1).num_dimensions != (*desc2).num_dimensions )
    {
	return (FALSE);
    }
    while (dim_count < (*desc1).num_dimensions)
    {
	if (ds_compare_dim_desc ( (*desc1).dimensions[dim_count],
				 (*desc2).dimensions[dim_count] ) != TRUE)
	return (FALSE);
        ++dim_count;
    }
    if (recursive == TRUE)
    {
	if (ds_compare_packet_desc ( (*desc1).packet, (*desc2).packet,
				    recursive )
            != TRUE)
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function ds_compare_array_desc  */

/*PUBLIC_FUNCTION*/
flag ds_compare_dim_desc (desc1, desc2)
/*  This routine will compare two dimension descriptors pointed to by  desc1
    and  desc2  .
    The routine returns TRUE if the two descriptors are equal, else it
    returns FALSE.
*/
dim_desc *desc1;
dim_desc *desc2;
{
    unsigned int coord_count = 0;
    static char function_name[] = "ds_compare_dim_desc";

    if ( (desc1 == NULL) || (desc2 == NULL) )
    {
	a_func_abort (function_name, "NULL descriptor pointer(s)");
        return (FALSE);
    }
    if ( ( (*desc1).name == NULL ) || ( (*desc2).name == NULL ) )
    {
	(void) fprintf (stderr, "Dimension name is a NULL pointer\n");
        a_prog_bug (function_name);
    }
    if (strcmp ( (*desc1).name, (*desc2).name ) != 0)
    return (FALSE);
    if ( (*desc1).length != (*desc2).length )
    return (FALSE);
    if ( (*desc1).minimum != (*desc2).minimum )
    return (FALSE);
    if ( (*desc1).maximum != (*desc2).maximum )
    return (FALSE);
    if ( (*desc1).coordinates == NULL )
    {
	if ( (*desc2).coordinates != NULL )
	return (FALSE);
    }
    else
    {
	if ( (*desc2).coordinates == NULL )
	return (FALSE);
        while (coord_count < (*desc1).length)
        {
	    if ( (*desc1).coordinates[coord_count] !=
                (*desc2).coordinates[coord_count] )
	    return (FALSE);
            ++coord_count;
        }
    }
    return (TRUE);
}   /*  End Function ds_compare_dim_desc  */

/*PUBLIC_FUNCTION*/
unsigned int ds_f_dim_in_array (arr_desc, name)
/*  This routine will determine if a particular dimension is in the array
    descriptor pointed to by  arr_desc  .
    The name of the dimension to find must be pointed to by  name  .
    If  name  is NULL, then the routine will not find anything.
    The routine returns the number of the dimension in the array if it was
    found else it returns the number of dimensions in the array.
    If the specified name occurs twice, the program aborts.
*/
array_desc *arr_desc;
CONST char *name;
{
    unsigned int dim_count;
    unsigned int return_value;
    static char function_name[] = "ds_f_dim_in_array";

    if (arr_desc == NULL)
    {
	(void) fprintf (stderr, "NULL  arr_desc  pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( ( (*arr_desc).num_dimensions < 1 ) || (name == NULL) )
    {
	return ( (*arr_desc).num_dimensions );
    }
    return_value = (*arr_desc).num_dimensions;
    for (dim_count = 0; dim_count < (*arr_desc).num_dimensions; ++dim_count)
    {
	if (strcmp (name, (* (*arr_desc).dimensions[dim_count] ).name) == 0)
	{
	    /*  Found dimension name   */
	    if (return_value < (*arr_desc).num_dimensions)
	    {
		(void) fprintf (stderr, "Multiple occurrences of: \"%s\"\n",
				name);
		a_prog_bug (function_name);
	    }   
	    return_value = dim_count;
        }
    }
    return (return_value);
}   /*  End Function ds_f_dim_in_array  */

/*PUBLIC_FUNCTION*/
unsigned long ds_get_array_offset (array_desc *arr_desc,
				   unsigned long *coordinates)
/*  This routine will calculate the offset in array co-odinates of a packet.
    The array descriptor must be pointed to by  arr_desc  .
    The array of dimension co-ordinates which specifies which packet must be
    pointed to by  coordinates  .
    The routine returns the offset of the packet.
*/
{
    unsigned int dim_count;
    unsigned long offset = 0;
    unsigned long current_length;
    unsigned long dim_scale = 1;
    static char function_name[] = "ds_get_array_offset";

    if ( (arr_desc == NULL) || (coordinates == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    for (dim_count = (*arr_desc).num_dimensions; dim_count > 0; --dim_count)
    {
	current_length = (* (*arr_desc).dimensions[dim_count - 1] ).length;
	if (coordinates[dim_count - 1] >= current_length)
	{
	    (void) fprintf (stderr,
			    "Coordinate[%u]: %u >= dimension length: %u\n",
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
unsigned long ds_get_coord_num (dim_desc *dimension, double coordinate,
				unsigned int bias)
/*  This routine will determine the co-ordinate number of a co-ordinate.
    The dimension descriptor must be pointed to by  dimension  .
    The co-ordinate to find the number of must be in  coordinate  .
    If the specified co-ordinate lies between two dimension co-ordinates, then
    the routine will find the co-ordinate which is lesser, closer or higher,
    depending on the value of  bias  .Legal values for  bias  are:
        SEARCH_BIAS_LOWER, SEARCH_BIAS_CLOSEST, SEARCH_BIAS_UPPER.
    The routine returns the index number of the co-ordinate found.
*/
{
    unsigned long coord_num;
    double tmp_found_coord;
    static char function_name[] = "ds_get_coord_num";

    if (dimension == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if co-ordinate specified is within range
	of dimension co-ordinates  */
    if (coordinate <= (*dimension).minimum)
    {
	return (0);
    }
    if (coordinate >= (*dimension).maximum)
    {
	return ( (*dimension).length - 1 );
    }
    if ( (*dimension).coordinates == NULL )
    {
	/*  Dimension co-ordinates are regularly spaced  */
	coord_num = ( (coordinate - (*dimension).minimum) /
		     ( (*dimension).maximum - (*dimension).minimum ) *
		     (double) ( (*dimension).length - 1 ) );
    }
    else
    {
	/*  Search through co-ordinate list  */
	for (coord_num = 0;
	     (*dimension).coordinates[coord_num + 1] < coordinate;
	     ++coord_num);
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
	return (coord_num);
/*
	break;
*/
      case SEARCH_BIAS_CLOSEST:
	if (coordinate - tmp_found_coord <
	    ds_get_coordinate (dimension, coord_num + 1) - coordinate)
	{
	    /*  Closest co-ordinate is lower one  */
	    return (coord_num);
	}
	else
	{
	    /*  Closest co-ordinate is upper one  */
	    return (coord_num + 1);
	}
	break;
      case SEARCH_BIAS_UPPER:
	return (coord_num + 1);
/*
	break;
*/
      default:
	(void) fprintf (stderr, "Illegal value of  bias  passed: %u\n", bias);
	a_prog_bug (function_name);
	break;
    }
    /*  Here to keep lint happy  */
    return (coord_num);
}   /*  End Function ds_get_coord_num  */

/*PUBLIC_FUNCTION*/
flag ds_get_element (CONST char *datum, unsigned int datum_type,
		     double *value, flag *complex)
/*  This routine will convert an atomic datum to a double precision complex
    value.
    The datum to be converted must be pointed to by  datum  and the data type
    value must be in  datum_type  .
    The data value will be written to the storage pointed to by  value  .
    This storages MUST lie on a  double  boundary.
    If the datum is a complex type, then the value of TRUE is written to the
    storage pointed to by  complex  ,else the value FALSE is written here. If
    this is NULL, nothing is written here.
    The routine returns TRUE if the data was successfully converted,
    else it returns FALSE.
*/
{
    flag tmp_complex = FALSE;
    static char function_name[] = "ds_get_element";

#ifdef NEEDS_MISALIGN_COMPILE
    extern char host_type_sizes[NUMTYPES];

    if ( (int) datum % host_type_sizes[datum_type] != 0 )
    {
	/*  Data is not aligned  */
	return ( misalign__ds_get_element (datum, datum_type, value,
					   complex) );
    }
#endif  /*  NEEDS_MISALIGN_COMPILE  */

    if (datum == NULL)
    {
	(void) fprintf (stderr, "NULL datum pointer passed\n");
	a_prog_bug (function_name);
    }
    if (value == NULL)
    {
	(void) fprintf (stderr, "NULL value storage pointer passed\n");
	a_prog_bug (function_name);
    }
#ifdef MACHINE_i386
    if ( (int) value % 4 != 0 )
#else
    if ( (long) value % sizeof (double) != 0 )
#endif
    {
	(void) fprintf (stderr,
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
	(void) fprintf (stderr, "Illegal data type: %u\n", datum_type);
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
/*  This routine will convert many atomic data to an array of double precision
    complex values.
    The data to be converted must be pointed to by  data  and the data type
    value must be in  data_type  .
    The stride of data elements in memory (in bytes) must be given by
    data_stride  .
    The data values will be written to the storage pointed to by  values  .
    These storages MUST lie on a  double  boundary.
    If the data are a complex type, then the value of TRUE is written to the
    storage pointed to by  complex  ,else the value FALSE is written here. If
    this is NULL, nothing is written here.
    The number of data values to convert must be pointed to by  num_values  .
    The routine returns TRUE if the data was successfully converted,
    else it returns FALSE.
*/
{
    flag tmp_complex = FALSE;
    unsigned int data_count;
    static char function_name[] = "ds_get_elements";

#ifdef NEEDS_MISALIGN_COMPILE
    extern char host_type_sizes[NUMTYPES];

    if ( (int) data % host_type_sizes[data_type] != 0 )
    {
	/*  First datum is not aligned  */
	return ( misalign__ds_get_elements (data, data_type, data_stride,
					    values, complex,num_values) );
    }
    if ( (int) data_stride % host_type_sizes[data_type] != 0 )
    {
	/*  Subsequent data is not aligned  */
	return ( misalign__ds_get_elements (data, data_type, data_stride,
					    values, complex,num_values) );
    }
#endif  /*  NEEDS_MISALIGN_COMPILE  */

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
#ifdef MACHINE_i386
    if ( (int) values % 4 != 0 )
#else
    if ( (long) values % sizeof (double) != 0 )
#endif
    {
	(void) fprintf (stderr,
			"values  address: %p not on a double boundary\n",
			values);
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
}   /*  End Function ds_get_elements  */

/*PUBLIC_FUNCTION*/
double *ds_get_coordinate_array (dimension)
/*  This routine will get a co-ordinate array for a dimension. If the dimension
    is regularly spaced, then the co-ordinate array is computed, else if it is
    irregularly spaced, it is copied from the dimension descriptor.
    The routine returns a pointer to a co-ordinate array on success, else it
    returns NULL.
*/
dim_desc *dimension;
{
    unsigned int coord_count;
    double *return_value;
    static char function_name[] = "ds_get_coordinate_array";

    if (dimension == NULL)
    {
	(void) fprintf (stderr, "NULL descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( ( return_value = (double *) m_alloc (sizeof *return_value *
					      (*dimension).length) )
	== NULL )
    {
	m_error_notify (function_name, "co-ordinate array");
	return (NULL);
    }
    if ( (*dimension).coordinates == NULL )
    {
	/*  Regularly spaced: compute  */
	for (coord_count = 0; coord_count < (*dimension).length; ++coord_count)
	{
	    return_value[coord_count] = ds_get_coordinate (dimension,
							   coord_count);
	}
    }
    else
    {
	/*  Irregularly spaced: copy  */
	for (coord_count = 0; coord_count < (*dimension).length; ++coord_count)
	{
	    return_value[coord_count] = (*dimension).coordinates[coord_count];
	}
    }
    return (return_value);
}   /*  End Function ds_get_coordinate_array  */

/*PUBLIC_FUNCTION*/
flag ds_element_is_complex (element_type)
/*  This routine will determine if the type of an element is complex or not.
    The element type must be in  element_type  .
    The element type must be atomic, else the routine will print an error
    message and abort processing.
    The routine returns TRUE if the element type is complex, else
    it returns FALSE.
*/
unsigned int element_type;
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
	(void) fprintf (stderr, "Illegal data type: %u\n",
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
/*  [PURPOSE] This routine will convert many atomic data to an array of double
    precision complex values. The data values may be scattered randomly (an
    offset array is used to index to the actual data).
    <data> The data to be converted.
    <data_type> The type of the data.
    <offsets> The offset array (in bytes).
    <values> The data values will be written here. Must be a  double  boundary.
    <complex> If the data are a complex type, then the value of TRUE is
    written here, else the value FALSE is written here. If this is NULL,
    nothing is written here.
    <num_values> The number of data values to convert.
    [RETURNS] TRUE if the data was successfully converted, else FALSE.
*/
{
    flag tmp_complex = FALSE;
    unsigned int count;
    double zero = 0.0;
    CONST char *ptr;
    static char function_name[] = "ds_get_scattered_elements";

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
#ifdef MACHINE_i386
    if ( (int) values % 4 != 0 )
#else
    if ( (long) values % sizeof (double) != 0 )
#endif
    {
	(void) fprintf (stderr,
			"values  address: %p not on a double boundary\n",
			values);
	a_prog_bug (function_name);
    }
    if (offsets == NULL)
    {
	(void) fprintf (stderr, "NULL offset array pointer passed\n");
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
	    ptr = data + offsets[count];
	    if (*(char *) ptr < -127)
	    {
		*values++ = TOOBIG;
	    }
	    else
	    {
		*values++ = *(char *) ptr;
	    }
	    *values++ = zero;
	}
	break;
      case K_INT:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(int *) ptr;
	    *values++ = zero;
	}
	break;
      case K_SHORT:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(short *) ptr;
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
	    ptr = data + offsets[count];
	    *values++ = *(char *) ptr;
	    *values++ = *( (char *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_ICOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(int *) ptr;
	    *values++ = *( (int *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_SCOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(short *) ptr;
	    *values++ = *( (short *) ptr + 1 );
	}
	tmp_complex = TRUE;
	break;
      case K_LONG:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(long *) ptr;
	    *values++ = zero;
	}
	break;
      case K_LCOMPLEX:
	for (count = 0; count < num_values; ++count)
	{
	    ptr = data + offsets[count];
	    *values++ = *(long *) ptr;
	    *values++ = *( (long *) ptr + 1 );
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
}   /*  End Function ds_get_scattered_elements  */
