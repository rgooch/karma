/*LINTLIBRARY*/
/*  alloc.c

    This code provides data structure allocation routines.

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

    This file contains the various utility routines for allocating data
  structures for the general data structure supported in Karma.


    Written by      Richard Gooch   13-SEP-1992

    Updated by      Richard Gooch   3-DEC-1992

    Updated by      Richard Gooch   22-DEC-1992: Fixed memory leak in functions
  ds_easy_alloc_array  ,  ds_dealloc_array_desc  and  ds_dealloc_packet  .

    Updated by      Richard Gooch   1-JAN-1993: Added support for memory mapped
  arrays.

    Updated by      Richard Gooch   3-JAN-1993: Added support for  K_ARRAY
  (padded array) and moved deallocation routines into  ds_dealloc.c

    Updated by      Richard Gooch   4-JAN-1993: Added preliminary support for
  tiled arrays.

    Updated by      Richard Gooch   5-JAN-1993: Fixed bug in  ds_alloc_array  .

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   30-JAN-1993: Added  ds_alloc_tiling_info  .

    Updated by      Richard Gooch   1-FEB-1993: Added setting of
  (*arr_desc).lengths[*]  in  ds_easy_alloc_array  .

    Updated by      Richard Gooch   9-FEB-1993: Improved diagnostics in
  ds_alloc_array  .

    Updated by      Richard Gooch   24-MAR-1993: Fixed bug in
  ds_alloc_contiguous_list  with stepping through list entries to allocate.

    Updated by      Richard Gooch   25-MAR-1993: Took account of changes to
  list_header  structure and added  ds_alloc_packet_subdata  .

    Updated by      Richard Gooch   17-MAY-1993: Added support for string data
  types.

    Updated by      Richard Gooch   10-AUG-1993: Changed initial attachment
  count for  multi_array  descriptor from one to zero to reflect recent changes
  to attachment implementation.

    Updated by      Richard Gooch   12-AUG-1993: Added
  ds_easy_alloc_n_element_array  .

    Updated by      Richard Gooch   25-AUG-1993: Improved documentation for
  ds_alloc_list_head  .

    Updated by      Richard Gooch   20-MAY-1994: Added  CONST  declarations
  where appropriate.

    Updated by      Richard Gooch   14-JUN-1994: Fixed syntax error in
  ds_alloc_packet  which caused problems on some platforms.

    Updated by      Richard Gooch   31-JUN-1994: Wrote in magic number in
  ds_alloc_multi  .

    Updated by      Richard Gooch   6-AUG-1994: Fixed bug in  ds_alloc_array
  which did not zero atomic array for Dec Alpha.

    Updated by      Richard Gooch   8-AUG-1994: Changed to  uaddr  type.

    Updated by      Richard Gooch   12-OCT-1994: Created
  ds_wrap_preallocated_n_element_array  routine.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ds/alloc.c

    Updated by      Richard Gooch   8-DEC-1994: Created
  ds_easy_alloc_array_desc  routine.

    Updated by      Richard Gooch   17-JAN-1995: Changed to  destroy_callbacks
  field for  multi_array  structure.

    Updated by      Richard Gooch   23-FEB-1995: Made use of CAN_ZERO_CLEAR.

    Updated by      Richard Gooch   19-APR-1995: Cleaned some code.

    Last updated by Richard Gooch   4-AUG-1995: Added more tests for misaligned
  data.


*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <os.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_c.h>


/*PUBLIC_FUNCTION*/
multi_array *ds_alloc_multi (num_arrays)
/*  This routine will allocate a header for  num_arrays  independent
    multi-dimensional arrays.
    The memory for the array of pointers to the array names, headers and
    data arrays is also allocated.
    The routine returns a pointer to the header. If memory could not be
    allocated, NULL is returned.
*/
unsigned int num_arrays;
{
    unsigned int count = 0;
    multi_array *return_value;
    static char function_name[] = "ds_alloc_multi";

    if ( ( return_value = (multi_array *)
	  m_alloc ( (unsigned int) sizeof *return_value ) ) == NULL )
    {
	m_error_notify (function_name, "multi_desc header");
        return (NULL);
    }
    return_value->num_arrays = num_arrays;
    return_value->array_names = NULL;
    return_value->first_hist = NULL;
    return_value->last_hist = NULL;
    return_value->destroy_callbacks = NULL;
    return_value->attachments = 0;
    if (num_arrays < 1)
    {
	return_value->headers = NULL;
        return_value->data = NULL;
        return (return_value);
    }
    if ( ( return_value->headers = (packet_desc **)
	  m_alloc (sizeof (packet_desc *) * num_arrays) ) == NULL )
    {
	m_free ( (char *) return_value->array_names );
        m_free ( (char *) return_value);
        m_error_notify (function_name, "multi_desc packet descriptors");
        return (NULL);
    }
    if ( ( return_value->data =
	  (char **) m_alloc (sizeof (char *) * num_arrays) ) == NULL )
    {
	m_free ( (char *) return_value->array_names );
        m_free ( (char *) return_value->headers );
        m_free ( (char *) return_value);
        m_error_notify (function_name, "array of data pointers");
        return (NULL);
    }
    if (num_arrays > 1)
    {
	/*  Many arrays: allocate memory for names  */
        if ( ( return_value->array_names =
	      (char **) m_alloc (sizeof (char *) * num_arrays) ) == NULL )
        {
	    m_free ( (char *) return_value);
            m_error_notify (function_name,
			    "array of pointers for array names");
            return (NULL);
        }
    }
    while (count < num_arrays)
    {
	if (num_arrays > 1)
        {
	    return_value->array_names[count] = NULL;
        }
        return_value->headers[count] = NULL;
        return_value->data[count] = NULL;
        ++count;
    }
    return_value->magic_number = MAGIC_MULTI_ARRAY;
    return (return_value);
}   /*  End Function ds_alloc_multi  */

/*PUBLIC_FUNCTION*/
packet_desc *ds_alloc_packet_desc (num_elem)
/*  This routine will allocate a packet descriptor  .The number of data
    elements in the packet must be in  num_elem  .
    The memory for the array of element types and descriptors is also
    allocated.
    The routine returns a pointer to the descriptor. If memory could not be
    allocated, the routine returns NULL.
*/
unsigned int num_elem;
{
    unsigned int element_count = 0;
    packet_desc *return_value;
    static char function_name[] = "ds_alloc_packet_desc";

    if ( ( return_value = (packet_desc *)
	  m_alloc ( sizeof (packet_desc) ) ) == NULL )
    {
	m_error_notify (function_name, "pack_descriptor");
        return (NULL);
    }
    if ( ( return_value->element_types = (unsigned int *)
	  m_alloc (sizeof (unsigned int) * num_elem) ) == NULL )
    {
	m_free ( (char *) return_value);
        m_error_notify (function_name, "array of element types");
        return (NULL);
    }
    if ( ( return_value->element_desc = (char **)
	  m_alloc (sizeof (char *) * num_elem) ) == NULL )
    {
	m_free ( (char *) return_value->element_types );
        m_free ( (char *) return_value);
        m_error_notify(function_name,"array of element descriptor pointers");
        return (NULL);
    }
    return_value->num_elements = num_elem;
    while (element_count < num_elem)
    {
	return_value->element_types[element_count] = NONE;
        return_value->element_desc[element_count++] = NULL;
    }
    return (return_value);
}   /*  End Function ds_alloc_packet_desc  */

/*PUBLIC_FUNCTION*/
char *ds_alloc_data (pack_desc, clear, array_alloc)
/*  This routine will allocate all memory required to store data in the packet
    with descriptor pointed to by  pack_desc  .
    The routine will recursively allocate space for packets, sub arrays of
    packets and linked list headers.
    The routine is quite robust, cleanly bypassing missing sections of the
    descriptor hierarchy, and deallocating any memory allocated after an
    error occurs.
    If the value of  clear  is TRUE, then the routine will initialise (set to
    zero) all the data.
    If the value of  array_alloc  is FALSE, and an array is an atomic array,
    then instances of that array will NOT be allocated, and a NULL pointer will
    be written into the parent element, else the array will be allocated.
    The routine returns a pointer to the data memory if all memory could be
    allocated, else it retuns NULL.
*/
packet_desc *pack_desc;
flag clear;
flag array_alloc;
{
    char *return_value;
    static char function_name[] = "ds_alloc_data";

    FLAG_VERIFY (clear);
    FLAG_VERIFY (array_alloc);
    if (pack_desc == NULL)
    {
	return (NULL);
    }
    if ( ( return_value = ds_alloc_packet (pack_desc) ) == NULL )
    {
	m_error_notify (function_name, "packet");
        return (NULL);
    }
    if ( !ds_alloc_packet_subdata (pack_desc, return_value, clear,
				   array_alloc) )
    {
	m_free (return_value);
	m_error_notify (function_name, "packet subdata");
	return (NULL);
    }
    return (return_value);
}   /*  End Function ds_alloc_data  */

/*PUBLIC_FUNCTION*/
flag ds_alloc_packet_subdata (pack_desc, packet, clear, array_alloc)
/*  This routine will recursively allocate space for sub arrays of packets and
    linked list headers for a packet. The data space for the packet is NOT
    allocated, it must be supplied.
    The packet descriptor must be pointed to by  pack_desc  .
    The packet data must be pointed to by  packet  .
    The routine is quite robust, cleanly bypassing missing sections of the
    descriptor hierarchy, and deallocating any memory allocated after an
    error occurs.
    If the value of  clear  is TRUE, then the routine will initialise (set to
    zero) all the sub-structure data.
    If the value of  array_alloc  is FALSE, and an array is an atomic array,
    then instances of that array will NOT be allocated, and a NULL pointer will
    be written into the parent element, else the array will be allocated.
    The routine returns TRUE if all memory could be allocated,
    else it retuns FALSE.
*/
packet_desc *pack_desc;
char *packet;
flag clear;
flag array_alloc;
{
    unsigned int elem_count;
    unsigned int type;
    char *element;
    list_header *head;
    static char function_name[] = "ds_alloc_packet_subdata";

    FLAG_VERIFY (clear);
    FLAG_VERIFY (array_alloc);
    if (pack_desc == NULL)
    {
	return (FALSE);
    }
    if (packet == NULL)
    {
	return (FALSE);
    }
    for (elem_count = 0; elem_count < pack_desc->num_elements; ++elem_count)
    {
	type = pack_desc->element_types[elem_count];
	if ( !ds_element_is_named (type) )
	{
	    /*  Recursive data type  */
	    element = packet + ds_get_element_offset (pack_desc, elem_count);
	    switch (type)
	    {
	      case K_ARRAY:
		/*  Array of packets to allocate memory for  */
		if ( !ds_alloc_array ( (array_desc *)
				      pack_desc->element_desc[elem_count],
				      element, clear, array_alloc ) )
		{
		    ds_dealloc_packet_subdata (pack_desc, packet);
		    m_error_notify (function_name, "array of packets");
		    return (FALSE);
		}
		break;
	      case LISTP:
		/*  Linked list: allocate header  */
		if ( ( head = ds_alloc_list_head () ) == NULL )
		{
		    ds_dealloc_packet_subdata (pack_desc, packet);
		    m_error_notify (function_name, "linked list header");
		    return (FALSE);
		}
#ifdef NEED_ALIGNED_DATA
		if ( IS_ALIGNED (element, sizeof head) )
		{
		    *(list_header **) element = head;
		}
		else m_copy (element, (char *) &head, sizeof head);
#else
		*(list_header **) element = head;
#endif
		break;
	      default:
		/*  Bad data type  */
		(void) fprintf (stderr, "Bad data type: %u\n",
				pack_desc->element_types[elem_count]);
		a_prog_bug (function_name);
		break;
	    }
	}
    }
    return (TRUE);
}   /*  End Function ds_alloc_packet_subdata  */

/*PUBLIC_FUNCTION*/
char *ds_alloc_packet (pack_desc)
/*  This routine will allocate memory for a packet which has a descriptor
    pointed to by  pack_desc  .
    The elements of the packet will be set to zero (for all types).
    This routine is NOT recursive (ie. sub arrays and linked lists are not
    allocated).
    The routine returns a pointer to the packet. If memory could not be
    allocated, NULL is returned.
*/
packet_desc *pack_desc;
{
    unsigned int packet_size;
#ifndef CAN_ZERO_CLEAR
    FString *fstring;
    unsigned int element_count;
    char *element;
    extern char host_type_sizes[NUMTYPES];
#endif
    char *packet;
    static char function_name[] = "ds_alloc_packet";

    packet_size = ds_get_packet_size (pack_desc);
    if (packet_size < 1)
    {
	/*  Zero sized packet: don't bother  */
        return (NULL);
    }
    if ( ( packet = m_alloc (packet_size) ) == NULL )
    {
	/*  Packet could not be allocated  */
        m_error_notify (function_name, "packet");
        return (NULL);
    }
#ifdef CAN_ZERO_CLEAR
    m_clear (packet, packet_size);
#else
    element = packet;
    /*  Zero packet  */
    for (element_count = 0; element_count < pack_desc->num_elements;
	 ++element_count)
    {
	/*  Zero element  */
        switch (pack_desc->element_types[element_count])
        {
	  case K_FLOAT:
	    *( (float *) element ) = 0.0;
	    break;
	  case K_DOUBLE:
	    *( (double *) element ) = 0.0;
	    break;
	  case K_BYTE:
	    *element = NULL;
	    break;
	  case K_INT:
	    *( (int *) element ) = NULL;
	    break;
	  case K_SHORT:
	    *( (short *) element ) = NULL;
	    break;
	  case K_ARRAY:
	    *( (unsigned int *) ( element + sizeof (char *) ) ) = K_ARRAY_UNALLOCATED;
	    /*  Fall through to define pointer  */
	  case LISTP:
	    *( (char **) element ) = NULL;
	    break;
	  case K_COMPLEX:
	    *( (float *) element ) = 0.0;
	    *( (float *) element + 1 ) = 0.0;
	    break;
	  case K_DCOMPLEX:
	    *( (double *) element ) = 0.0;
	    *( (double *) element + 1 ) = 0.0;
	    break;
	  case K_BCOMPLEX:
	    *element = NULL;
	    *(element + 1) = NULL;
	    break;
	  case K_ICOMPLEX:
	    *( (int *) element ) = NULL;
	    *( (int *) element + 1 ) = NULL;
	    break;
	  case K_SCOMPLEX:
	    *( (short *) element ) = NULL;
	    *( (short *) element + 1 ) = NULL;
	    break;
	  case K_LONG:
	    *( (long *) element ) = NULL;
	    break;
	  case K_LCOMPLEX:
	    *( (long *) element ) = NULL;
	    *( (long *) element + 1 ) = NULL;
	    break;
	  case K_UBYTE:
	    *( (unsigned char *) element ) = NULL;
	    break;
	  case K_UINT:
	    *( (unsigned int *) element ) = NULL;
	    break;
	  case K_USHORT:
	    *( (unsigned short *) element ) = NULL;
	    break;
	  case K_ULONG:
	    *( (unsigned long *) element ) = NULL;
	    break;
	  case K_UBCOMPLEX:
	    *( (unsigned char *) element ) = NULL;
	    *( (unsigned char *) element + 1 ) = NULL;
	    break;
	  case K_UICOMPLEX:
	    *( (unsigned int *) element ) = NULL;
	    *( (unsigned int *) element + 1 ) = NULL;
	    break;
	  case K_USCOMPLEX:
	    *( (unsigned short *) element ) = NULL;
	    *( (unsigned short *) element + 1 ) = NULL;
	    break;
	  case K_ULCOMPLEX:
	    *( (unsigned long *) element ) = NULL;
	    *( (unsigned long *) element + 1 ) = NULL;
	    break;
	  case K_VSTRING:
	    *(char **) element = NULL;
	    break;
	  case K_FSTRING:
	    fstring = (FString *) element;
	    fstring->max_len = 0;
	    fstring->string = NULL;
	    break;
	  default:
	    *element = NULL;
        }
        element += host_type_sizes[pack_desc->element_types[element_count] ];
    }
#endif
    return (packet);
}   /*  End Function ds_alloc_packet  */

/*PUBLIC_FUNCTION*/
array_desc *ds_alloc_array_desc (num_dimensions, num_levels)
/*  This function will allocate a header for a multi-dimensional tiled array of
    data packets. The number of dimensions to allocate their respective
    headers must be in  num_dimensions  .
    The number of levels of tiling must be given by  num_levels  .If this is
    0, the array is not tiled.
    Note that the dimension descriptors are not allocated, however, the array
    of pointers for them is allocated.
    The routine returns a pointer to the header. If memory could not be
    allocated, NULL is returned.
*/
unsigned int num_dimensions;
unsigned int num_levels;
{
    unsigned int dim_count;
    array_desc *return_value;
    static char function_name[] = "ds_alloc_array_desc";

    if (num_dimensions < 1)
    {
	(void) fprintf (stderr, "Cannot allocate zero-dimensioned array\n");
	a_prog_bug (function_name);
    }
    if (num_levels > 0)
    {
	(void) fprintf (stderr, "WARNING: tiling being used\n");
    }
    /*  Allocate descriptor  */
    if ( ( return_value =
	  (array_desc *) m_alloc (sizeof *return_value) ) == NULL )
    {
	m_error_notify (function_name, "array descriptor");
        return (NULL);
    }
    return_value->num_dimensions = num_dimensions;
    return_value->num_levels = 0;
    /*  Alocate array of dimension descriptor pointers  */
    if ( ( return_value->dimensions = (dim_desc **)
	  m_alloc (sizeof (dim_desc *) * num_dimensions) ) == NULL )
    {
	m_free ( (char *) return_value);
        m_error_notify (function_name,
			"array of dimension descriptor pointers");
        return (NULL);
    }
    return_value->tile_lengths = NULL;
    /*  Allocate array of lengths  */
    if ( ( return_value->lengths = (uaddr *)
	  m_alloc (sizeof *return_value->lengths * num_dimensions) )
	== NULL )
    {
	m_error_notify (function_name, "array of lengths");
	m_free ( (char *) return_value->dimensions );
	m_free ( (char *) return_value);
	return (NULL);
    }
    for (dim_count = 0; dim_count < num_dimensions; ++dim_count)
    {
	return_value->dimensions[dim_count] = NULL;
	return_value->lengths[dim_count] = 0;
    }
    if (ds_alloc_tiling_info (return_value, num_levels) != TRUE)
    {
	m_error_notify (function_name, "tiling information");
	m_free ( (char *) return_value->dimensions );
	m_free ( (char *) return_value);
	return (NULL);
    }
    return_value->packet = NULL;
    return_value->offsets = NULL;
    return_value->padded = 2 * abs (TRUE) + 2 * abs (FALSE);
    return (return_value);
}   /*  End Function ds_alloc_array_desc  */

/*PUBLIC_FUNCTION*/
flag ds_alloc_tiling_info (arr_desc, num_levels)
/*  This routine will allocate tiling information for an array descriptor which
    does not have any existing tiling information.
    The array descriptor must be pointed to by  arr_desc  .
    The number of levels of tiling must be given by  num_levels  .If this is
    0, the array is not tiled.
    The routine returns TRUE on success, else it returns FALSE.
*/
array_desc *arr_desc;
unsigned int num_levels;
{
    unsigned int dim_count;
    static char function_name[] = "ds_alloc_tiling_info";

    if (arr_desc->num_levels > 0)
    {
	(void) fprintf (stderr, "Existing tiling information\n");
	a_prog_bug (function_name);
    }
    
    if (num_levels < 1)
    {
	return (TRUE);
    }
    /*  Allocate array of tile length pointers  */
    if ( ( arr_desc->tile_lengths = (unsigned int **)
	  m_alloc (sizeof *arr_desc->tile_lengths *
		   arr_desc->num_dimensions) )
	== NULL )
    {
	m_error_notify (function_name, "array of tile length pointers");
	return (FALSE);
    }
    for (dim_count = 0; dim_count < arr_desc->num_dimensions; ++dim_count)
    {
	/*  Allocate array of tile lengths for this dimension  */
	if ( ( arr_desc->tile_lengths[dim_count] = (unsigned int *)
	      m_alloc (sizeof **arr_desc->tile_lengths * num_levels) )
	    == NULL )
	{
	    /*  Abort here because I can't be bothered cleaning up  */
	    m_abort (function_name, "array of tile lengths");
	}
	m_clear ( (char *) arr_desc->tile_lengths[dim_count],
		 sizeof **arr_desc->tile_lengths *num_levels );
    }
    arr_desc->num_levels = num_levels;
    return (TRUE);
}   /*  End Function ds_alloc_tiling_info  */

/*PUBLIC_FUNCTION*/
dim_desc *ds_alloc_dim_desc (CONST char *dim_name, uaddr length,
			     double min, double max, flag regular)
/*  This routine will allocate a header for a single dimension.
    The name of the dimension must be pointed to by  dim_name  . The name is
    copied, thus the input character string may be subsequently deallocated.
    The length (number of co-ordinates) of the dimension must be in  length  .
    The minimum and maximum values of the dimension co-ordinates must be in
    min  and  max  ,respectively.
    If the co-ordinates of the dimension are regularly spaced,  regular  must
    be TRUE, else it must be FALSE.
    If the co-ordinates are regular, then the  coordinates  field of the array
    descriptor will be set to NULL, else it will point to an array of doubles
    of length  length  .These co-ordinates will be set to 0.0 .
    The routine returns a pointer to the descriptor. If memory could not be
    allocated, NULL is returned.
*/
{
    unsigned int count = 0;
    dim_desc *return_value;
    static char function_name[] = "ds_alloc_dim_desc";

    if (dim_name == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (length < 1)
    {
	(void) fprintf (stderr, "Illegal dimension length: %lu passed\n",
			length);
	a_prog_bug (function_name);
    }
    if ( ( return_value =
	  (dim_desc *) m_alloc (sizeof *return_value) ) == NULL )
    {
	m_error_notify (function_name, "dimension_descriptor");
        return (NULL);
    }
    if ( ( return_value->name = m_dup (dim_name, strlen (dim_name) + 1) )
	== NULL )
    {
	m_free ( (char *) return_value);
        m_error_notify (function_name, "dimension name");
        return (NULL);
    }
    return_value->length = length;
    return_value->minimum = min;
    return_value->maximum = max;
    if ( regular || (length < 1) )
    {
	return_value->coordinates = NULL;
        return (return_value);
    }
    if ( ( return_value->coordinates =
	  (double *) m_alloc (sizeof (double) * length) ) == NULL )
    {
	m_free (return_value->name);
        m_free ( (char *) return_value);
        m_error_notify (function_name, "coordinate array");
        return (NULL);
    }
    while (count < length)
    {
	return_value->coordinates[count] = 0.0;
        ++count;
    }
    return (return_value);
}   /*  End Function ds_alloc_dim_desc */

/*PUBLIC_FUNCTION*/
list_header *ds_alloc_list_head ()
/*  This routine will allocate a linked list header.
    The length of the linked list specified in the header will be 0, and the
    contiguous_until  entry will be set to 0.
    The  sort_type  field in the header will be set to SORT_UNDEF. This MUST
    be set to some other value (ie. SORT_RANDOM) prior to use with other
    library routines.
    The  list_start  and  list_end  pointers will be set to NULL.
    The routine returns a pointer to the header. If memory could not be
    allocated, NULL is returned.
*/
{
    list_header *return_value;
    static char function_name[] = "ds_alloc_list_head";

    if ( ( return_value =
	  (list_header *) m_alloc (sizeof *return_value) ) == NULL )
    {
	m_error_notify (function_name, "linked list header");
        return (NULL);
    }
    return_value->magic = MAGIC_LIST_HEADER;
    return_value->length = 0;
    return_value->contiguous_length = 0;
    return_value->sort_type = SORT_UNDEF;
    return_value->sort_elem_num = 0;
    return_value->contiguous_data = NULL;
    return_value->first_frag_entry = NULL;
    return_value->last_frag_entry = NULL;
    return (return_value);
}   /*  End Function ds_alloc_list_head  */

/*PUBLIC_FUNCTION*/
list_entry *ds_alloc_list_entry (list_desc, array_alloc)
/*  This routine will allocate an entry in a linked list (it will NOT insert
    it in the list: see ds_list_insert).
    The descriptor for the linked list must be pointed to by  list_desc  .
    The list pointers will be set to NULL.
    The routine will initialise (set to zero) the data in the entry.
    The routine will recursively allocate memory for sub arrays and linked
    lists.
    If the value of  array_alloc  is FALSE, and an array is an atomic array,
    then instances of that array will NOT be allocated, and a NULL pointer will
    be written into the parent element, else the array will be allocated.
    The routine returns a pointer to the entry. If memory could not be
    allocated, NULL is returned.
*/
packet_desc *list_desc;
flag array_alloc;
{
    list_entry *return_value;
    static char function_name[] = "ds_alloc_list_entry";

    FLAG_VERIFY (array_alloc);
    if ( ( return_value =
	  (list_entry *) m_alloc (sizeof *return_value) ) == NULL )
    {
	m_error_notify (function_name, "linked list entry");
        return (NULL);
    }
    return_value->prev = NULL;
    return_value->next = NULL;
    if (ds_get_packet_size (list_desc) < 1)
    {
	return_value->data = NULL;
        return (return_value);
    }
    if ( ( return_value->data = ds_alloc_data (list_desc, TRUE,
					       array_alloc) ) == NULL )
    {
	m_free ( (char *) return_value);
	m_error_notify (function_name, "list data entry");
        return (NULL);
    }
    return (return_value);
}   /*  End Function ds_alloc_list_entry  */

/*PUBLIC_FUNCTION*/
flag ds_alloc_array (arr_desc, element, clear, array_alloc)
/*  This routine will allocate memory for an array which has a descriptor
    pointed to by  arr_desc  .
    The routine will recursively allocate sub arrays and linked lists.
    The element to write the array pointer to must be pointed to by  element  .
    If the value of  clear  is TRUE, then the routine will initialise (set to
    zero) all the data.
    If the value of  array_alloc  is FALSE, and an array is an atomic array,
    then instances of that array will NOT be allocated, and a NULL pointer will
    be written into the parent element, else the array will be allocated.
    Any memory which is allocated will be deallocated if an error occurs.
    The routine returns a pointer to the array. If memory could not be
    allocated, NULL is returned.
*/
array_desc *arr_desc;
char *element;
flag clear;
flag array_alloc;
{
    flag atomic;
    unsigned int data_bytes;
    unsigned int packet_size;
    unsigned int array_size, alloc_val;
    unsigned int packet_count = 0;
    char *data;
    char *array;
    static char function_name[] = "ds_alloc_array";

    FLAG_VERIFY (clear);
    FLAG_VERIFY (array_alloc);
    if (arr_desc == NULL)
    {
	a_func_abort (function_name, "NULL array descriptor");
	return (FALSE);
    }
    if (arr_desc->packet == NULL)
    {
	a_func_abort (function_name, "NULL packet descriptor for array");
	return (FALSE);
    }
    packet_size = ds_get_packet_size (arr_desc->packet);
    array_size = ds_get_array_size (arr_desc);
    data_bytes = packet_size * array_size;
    if (data_bytes < 1)
    {
	return (FALSE);
    }
    atomic = ds_packet_all_data (arr_desc->packet);
    if (atomic && !array_alloc)
    {
	/*  Array need not be allocated  */
	array = NULL;
	alloc_val = K_ARRAY_UNALLOCATED;
    }
    else
    {
	/*  Array must be allocated  */
	if ( ( array = m_alloc (data_bytes) ) == NULL )
	{
	    m_error_notify (function_name, "array");
	    return (FALSE);
	}
	alloc_val = K_ARRAY_M_ALLOC;
    }
#ifdef NEED_ALIGNED_DATA
    if ( IS_ALIGNED (element, sizeof array) ) *(char **) element = array;
    else m_copy (element, (char *) &array, sizeof array);
    element += sizeof array;
    if ( IS_ALIGNED (element, sizeof alloc_val) )
    {
	*(unsigned int *) element = alloc_val;
    }
    else m_copy (element, (char *) &alloc_val, sizeof alloc_val);
#else
    *(char **) element = array;
    element += sizeof array;
    *(unsigned int *) element = alloc_val;
#endif
    if (atomic && !array_alloc) return (TRUE);
    if (atomic)
    {
	/*  No sub-arrays or linked lists  */
	if (!clear)
	{
	    /*  Initialisation not required  */
	    return (TRUE);
	}
	/*  Do fast array initialize  */
	/*  NOTE: this will break on platforms which don't have 0 as the zero
	    value (for say floating point numbers).  */
	m_clear (array, data_bytes);
	return (TRUE);
    }
    /*  Initialize the array and allocate sub arrays and linked lists  */
    for (data = array; packet_count < array_size;
	 ++packet_count, data += packet_size)
    {
	/*  Allocate a packet  */
        if ( !ds_alloc_packet_subdata (arr_desc->packet, data, clear,
				       array_alloc) )
        {
	    m_error_notify (function_name, "packet subdata");
            return (FALSE);
        }
    }
    return (TRUE);
}   /*  End Function ds_alloc_array  */

/*PUBLIC_FUNCTION*/
char *ds_easy_alloc_array (multi_array **multi_desc, unsigned int num_dim,
			   uaddr *lengths, double *minima, double *maxima,
			   char **names, unsigned int data_type,
			   CONST char *data_name)
/*  This routine will allocate memory for a multi-dimensional, regular array,
    and the required headers and the multi-array header. The array is NOT tiled
    nor are any address offsets computed.
    Note that this routine does NOT create a Karma arrayfile. This must be done
    with a call to  dsxfr_put_multi  .
    The multi-array structure pointer to the created structure will be
    written to the storage pointed to by  multi_desc  .
    The number of dimensions of the array must be in  num_dim  ,the lengths
    of each dimension (axis) must be in the array pointed to by  lengths  .
    The minimum and maximum co-ordinate values of each dimension must be in the
    arrays pointed to by  minima  and maxima  ,respectively. If either of
    these pointers is NULL, the range of the co-ordinates will be the lengths
    of the dimensions minus 1. If one of the pointers is NULL, the other
    array is used to tie one end of the range for each dimension. If both
    pointers are NULL,the minima are 0.0 and the maxima are the lengths minus 1
    The names of each dimension must be pointed to by the list of character
    pointers  names  .If this is NULL, the names "Axis 0", "Axis 1", ...etc
    will be used. Note: the character arrays are copied, so the arrays of
    characters and the array of pointers may be subsequently deallocated.
    The data packet that may be stored in the array is a single, atomic datum,
    of type  data_type  .
    The following data types are not permitted:
        NONE, K_ARRAY, LISTP, MULTI_ARRAY.
    The name of the data type must be pointed to by  data_name  .If this is
    NULL, then the name "Data Value" will be used. The name string is copied,
    thus the memory used for the input string may be subsequently deallocated.
    The routine returns a pointer to the start of the array. If memory could
    not be allocated, or any error occurs, NULL is returned.
*/
{
    static char *def_data_name = "Data Value";
    static char function_name[] = "ds_easy_alloc_array";

    if ( (multi_desc == NULL) || (lengths == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (data_name == NULL)
    {
	data_name = def_data_name;
    }
    return ( ds_easy_alloc_n_element_array (multi_desc, num_dim, lengths,
					    minima, maxima, names,
					    1, &data_type,
					    (char **) &data_name) );
}   /*  End Function ds_easy_alloc_array  */

/*PUBLIC_FUNCTION*/
char *ds_easy_alloc_n_element_array (multi_array **multi_desc,
				     unsigned int num_dim, uaddr *lengths,
				     double *minima, double *maxima,
				     char **names, unsigned int num_elements,
				     unsigned int *data_types,
				     char **data_names)
/*  This routine will allocate memory for a multi-dimensional, regular array,
    and the required headers and the multi-array header. The array is NOT tiled
    nor are any address offsets computed.
    Note that this routine does NOT create a Karma arrayfile. This must be done
    with a call to  dsxfr_put_multi  .
    The multi-array structure pointer to the created structure will be
    written to the storage pointed to by  multi_desc  .
    The number of dimensions of the array must be in  num_dim  ,the lengths
    of each dimension (axis) must be in the array pointed to by  lengths  .
    The minimum and maximum co-ordinate values of each dimension must be in the
    arrays pointed to by  minima  and maxima  ,respectively. If either of
    these pointers is NULL, the range of the co-ordinates will be the lengths
    of the dimensions minus 1. If one of the pointers is NULL, the other
    array is used to tie one end of the range for each dimension. If both
    pointers are NULL,the minima are 0.0 and the maxima are the lengths minus 1
    The names of each dimension must be pointed to by the list of character
    pointers  names  .If this is NULL, the names "Axis 0", "Axis 1", ...etc
    will be used. Note: the character arrays are copied, so the arrays of
    characters and the array of pointers may be subsequently deallocated.
    The data packet that may be stored in the array may contain a number of
    atomic elements. The number of elements in this packet must be given by
    num_elements
    The data types of the elements must be pointed to by  data_types  .
    The following data types are not permitted:
        NONE, K_ARRAY, LISTP, MULTI_ARRAY.
    The names of the elements must be pointed to by  data_names  .The name
    strings are copied, thus the memory used for the input strings may be
    subsequently deallocated.
    The routine returns a pointer to the start of the array. If memory could
    not be allocated, or any error occurs, NULL is returned.
*/
{
    char *array;
    static char function_name[] = "ds_easy_alloc_n_element_array";

    if ( (multi_desc == NULL) || (lengths == NULL) || (data_types == NULL) ||
	(data_names == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( ( *multi_desc =
	  ds_wrap_preallocated_n_element_array (NULL, num_dim, lengths,
						minima, maxima, NULL,
						names, num_elements,
						data_types, data_names) )
	== NULL ) return (NULL);
    array = *( (char **) (**multi_desc).data[0] );
    return (array);
}   /*  End Function ds_easy_alloc_n_element_array  */

/*PUBLIC_FUNCTION*/
multi_array *ds_wrap_preallocated_n_element_array (char *array,
						   unsigned int num_dim,
						   uaddr *lengths,
						   double *minima,
						   double *maxima,
						   double **coordinates,
						   char **names,
						   unsigned int num_elements,
						   unsigned int *data_types,
						   char **data_names)
/*  This routine will "wrap" an externally allocated array by allocating memory
    for a multi-dimensional, regular array, and the required headers and the
    multi-array header. No address offsets are computed.
    Note that this routine does NOT create a Karma arrayfile. This must be done
    with a call to  dsxfr_put_multi  .
    The externally allocated array data must be pointed to by  array  .This
    data must be externally deallocated when no longer needed. If  array  is
    NULL, then the array data is internally allocated (and should not be
    externally deallocated).
    The number of dimensions of the array must be in  num_dim  ,the lengths
    of each dimension (axis) must be in the array pointed to by  lengths  .
    The minimum and maximum co-ordinate values of each dimension must be in the
    arrays pointed to by  minima  and maxima  ,respectively. If either of
    these pointers is NULL, the range of the co-ordinates will be the lengths
    of the dimensions minus 1. If one of the pointers is NULL, the other
    array is used to tie one end of the range for each dimension. If both
    pointers are NULL,the minima are 0.0 and the maxima are the lengths minus 1
    The array of co-ordinate array pointers must be pointed to by  coordinates
    If this is NULL, all dimensions are assumed to be regularly spaced. If any
    co-ordinate array pointer is NULL, the corresponding dimension is assumed
    to be regularly spaced.
    The names of each dimension must be pointed to by the list of character
    pointers  names  .If this is NULL, the names "Axis 0", "Axis 1", ...etc
    will be used. Note: the character arrays are copied, so the arrays of
    characters and the array of pointers may be subsequently deallocated.
    The data packet that may be stored in the array may contain a number of
    atomic elements. The number of elements in this packet must be given by
    num_elements
    The data types of the elements must be pointed to by  data_types  .
    The following data types are not permitted:
        NONE, K_ARRAY, LISTP, MULTI_ARRAY.
    The names of the elements must be pointed to by  data_names  .The name
    strings are copied, thus the memory used for the input strings may be
    subsequently deallocated.
    The routine returns a multi-array structure pointer on success,
    else NULL is returned.
*/
{
    multi_array *multi_desc;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    static char function_name[] = "ds_wrap_preallocated_n_element_array";

    if ( ( multi_desc = ds_alloc_multi ( (unsigned int) 1 ) ) == NULL )
    {
	m_error_notify (function_name, "multi_desc descriptor");
        return (NULL);
    }
    if ( ( pack_desc = ds_alloc_packet_desc ( (unsigned) 1 ) ) == NULL )
    {
	ds_dealloc_multi (multi_desc);
	m_error_notify (function_name, "packet descriptor");
        return (NULL);
    }
    multi_desc->headers[0] = pack_desc;
    if ( ( arr_desc = ds_easy_alloc_array_desc (num_dim, lengths,
						minima, maxima,
						coordinates, names,
						num_elements, data_types,
						data_names) ) == NULL )
    {
	ds_dealloc_multi (multi_desc);
	m_error_notify (function_name, "array descriptor");
        return (NULL);
    }
    pack_desc->element_desc[0] = (char *) arr_desc;
    pack_desc->element_types[0] = K_ARRAY;
    /*  Allocate the data space  */
    if ( ( multi_desc->data[0] =
	  ds_alloc_data (multi_desc->headers[0], TRUE,
			 (array == NULL) ? TRUE : FALSE) )
        == NULL )
    {
	ds_dealloc_multi (multi_desc);
	m_error_notify (function_name, "top level packet");
        return (NULL);
    }
    if (array != NULL) *( (char **) multi_desc->data[0] ) = array;
    return (multi_desc);
}   /*  End Function ds_wrap_preallocated_n_element_array  */

/*PUBLIC_FUNCTION*/
array_desc *ds_easy_alloc_array_desc (unsigned int num_dim, uaddr *lengths,
				      double *minima, double *maxima,
				      double **coordinates, char **names,
				      unsigned int num_elements,
				      unsigned int *data_types,
				      char **data_names)
/*  [PURPOSE] This routine will allocate an array descriptor and it's
    associated packet descriptor.
    <num_dim> The number of dimensions in the array.
    <lengths> An array of lengths for each dimension (axis).
    <minima> An array of minimum co-ordinate values for each dimension.
    <maxima> An array of maximum co-ordinate values for each dimension.
    If either of these pointers is NULL, the range of the co-ordinates will be
    the lengths of the dimensions minus 1. If one of the pointers is NULL, the
    other array is used to tie one end of the range for each dimension. If both
    pointers are NULL,the minima are 0.0 and the maxima are the lengths minus 1
    <coordinates> The array of co-ordinate array pointers. If this is NULL,
    all dimensions are assumed to be regularly spaced. If any co-ordinate array
    pointer is NULL, the corresponding dimension is assumed to be regularly
    spaced.
    <names> An array of string pointers to the names of each dimension. If this
    is NULL, the names "Axis 0", "Axis 1", ...etc will be used. Note: the
    character arrays are copied, so the arrays of characters and the array of
    pointers may be subsequently deallocated.
    <num_elements> The number of atomic elements in the array packet.
    <data_types> The array of data types. The following data types are not
    permitted:
        NONE, K_ARRAY, LISTP, MULTI_ARRAY.
    <data_names> The array of element names. The name strings are copied, thus
    the memory used for the input strings may be subsequently deallocated.
    [RETURNS] An array descriptor pointer on success, else NULL.
*/
{
    flag this_dim_has_coords;
    unsigned int dim_count;
    unsigned int elem_count;
    double minimum;
    double maximum;
    char *tmp_pointer;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    dim_desc *dimension;
    char tmp_name[129];
    static char function_name[] = "ds_easy_alloc_array_desc";

    if ( (lengths == NULL) || (data_types == NULL) || (data_names == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (names == NULL)
    {
	/*  No names supplied  */
        if ( (int) log10 ( (double) num_dim ) + 1 > 122 )
        {
	    /*  Too many dimensions for sprintf to print  */
	    a_func_abort (function_name, "Too many dimensions");
            return (NULL);
        }
    }
    if ( ( arr_desc = ds_alloc_array_desc (num_dim, 0) ) == NULL )
    {
	m_error_notify (function_name, "array descriptor");
        return (NULL);
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if ( (coordinates != NULL) && (coordinates[dim_count] != NULL) )
	{
	    this_dim_has_coords = TRUE;
	}
	else
	{
	    this_dim_has_coords = FALSE;
	}
	/*  Allocate and add adimension descriptor  */
        if (names == NULL)
        {
	    /*  Create temporary dimension name  */
#ifdef OS_VXMVX
            (void) sprintf (tmp_name, "Axis %d", (int) dim_count);
#else
            (void) sprintf (tmp_name, "Axis %u", dim_count);
#endif
            tmp_pointer = tmp_name;
        }
        else
        {
	    /*  Copy over character pointer  */
            tmp_pointer = names[dim_count];
        }
	if (this_dim_has_coords)
	{
	    minimum = coordinates[dim_count][0];
	    maximum = coordinates[dim_count][lengths[dim_count] - 1];
	}
	else
	{
	    if ( (minima == NULL) && (maxima == NULL) )
	    {
		/*  Range is from  0.0  to  lengths - 1  */
		minimum = 0.0;
		maximum = (lengths[dim_count] - 1);
	    }
	    else
	    {
		/*  Range has at least one boundary supplied  */
		if (minima == NULL)
		{
		    /*  Maximum only supplied  */
		    maximum = maxima[dim_count];
		    minimum = maximum - (lengths[dim_count] - 1);
		}
		else
		{
		    /*  Minimum supplied  */
		    minimum = minima[dim_count];
		    if (maxima == NULL)
		    {
			/*  Minimum only supplied  */
			maximum = minimum + (lengths[dim_count] - 1);
		    }
		    else
		    {
			/*  Maximum also supplied  */
			maximum = maxima[dim_count];
		    }
		}
	    }
	}
        if ( ( dimension =
	      ds_alloc_dim_desc (tmp_pointer, lengths[dim_count],
				 minimum, maximum,
				 this_dim_has_coords ? FALSE : TRUE) )
	    == NULL )
        {
	    ds_dealloc_array_desc (arr_desc);
	    m_error_notify (function_name, "dimension descriptor");
            return (NULL);
        }
        arr_desc->dimensions[dim_count] = dimension;
	arr_desc->lengths[dim_count] = dimension->length;
	if (this_dim_has_coords)
	{
	    m_copy ( (char *) dimension->coordinates,
		    (char *) coordinates[dim_count],
		    sizeof **coordinates * lengths[dim_count]);
	}
    }
    if ( ( pack_desc = ds_alloc_packet_desc (num_elements) ) == NULL )
    {
	ds_dealloc_array_desc (arr_desc);
	m_error_notify (function_name, "array packet descriptor");
        return (NULL);
    }
    arr_desc->packet = pack_desc;
    for (elem_count = 0; elem_count < num_elements; ++elem_count)
    {
	if (ds_element_is_atomic (data_types[elem_count]) != TRUE)
	{
	    /*  Bad data type  */
	    (void) fprintf (stderr, "Bad data type: %u\n",
			    data_types[elem_count]);
	    a_prog_bug (function_name);
	}
	pack_desc->element_types[elem_count] = data_types[elem_count];
	if (data_names[elem_count] == NULL)
	{
	    (void) fprintf (stderr, "NULL data name pointer in array\n");
	    a_prog_bug (function_name);
	}
	if ( ( pack_desc->element_desc[elem_count] = 
	      m_alloc ( (unsigned int) strlen (data_names[elem_count]) +
		       (unsigned int) 1 ) ) == NULL )
	{
	    ds_dealloc_array_desc (arr_desc);
	    m_error_notify (function_name, "element name");
	    return (NULL);
	}
	(void) strcpy (pack_desc->element_desc[elem_count],
		       data_names[elem_count]);
    }
    return (arr_desc);
}   /*  End Function ds_easy_alloc_array_desc  */

/*PUBLIC_FUNCTION*/
flag ds_alloc_contiguous_list (list_desc, list_head, length, clear,array_alloc)
/*  This routine will allocate a contiguous block of linked list entry
    data  packets. No  list_entry  structures are allocated, they are implied.
    The packet descriptor for the linked list must be pointed to by
    list_desc  and the list header must be pointed to by  list_header  .
    The number of list entries to allocate must be in  length  .
    The routine will recursively allocate memory for sub arrays and linked
    lists.
    The  contiguous_length  value in the list header will be set to the list
    length.
    If the value of  clear  is TRUE, then the routine will initialise (set to
    zero) all the data.
    If the value of  array_alloc  is FALSE, and an array is an atomic array,
    then instances of that array will NOT be allocated, and a NULL pointer will
    be written into the parent element, else the array will be allocated.
    The list must be empty.
    The routine returns TRUE on success, else it returns FALSE.
    On failure, the list header is deallocated.
*/
packet_desc *list_desc;
list_header *list_head;
unsigned int length;
flag clear;
flag array_alloc;
{
    unsigned int count;
    unsigned int pack_size;
    char *data;
    static char function_name[] = "ds_alloc_contiguous_list";

    FLAG_VERIFY (clear);
    FLAG_VERIFY (array_alloc);
    if ( (list_desc == NULL) || (list_head == NULL) )
    {
	return (FALSE);
    }
    if (list_head->magic != MAGIC_LIST_HEADER)
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    if (list_head->length > 0)
    {
	(void) fprintf (stderr, "List has: %lu entries: must be empty!\n",
			list_head->length);
	a_prog_bug (function_name);
    }
    if (length < 1) return (TRUE);
    pack_size = ds_get_packet_size (list_desc);
    /*  Allocate a contiguous block of list data packets  */
    if ( ( list_head->contiguous_data = m_alloc (pack_size * length) )
	== NULL )
    {
	m_error_notify (function_name, "linked list data");
        return (FALSE);
    }
    /*  Clear block of list data packets  */
    m_clear (list_head->contiguous_data, pack_size * length);
    /*  Write in data for length, etc. into header  */
    list_head->length = length;
    list_head->contiguous_length = length;
    if ( ds_packet_all_data (list_desc) )
    {
	/*  No sub-structure to allocate for: return OK here  */
	return (TRUE);
    }
    /*  Now go through each entry's data and allocate sub-structure  */
    for (count = 0, data = list_head->contiguous_data; count < length;
	 ++count, data += pack_size)
    {
	if ( !ds_alloc_packet_subdata (list_desc, data, clear, array_alloc) )
	{
	    ds_dealloc_list_entries (list_desc, list_head);
            return (FALSE);
        }
    }
    return (TRUE);
}   /*  End Function ds_alloc_contiguous_list  */
