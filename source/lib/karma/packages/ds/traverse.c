/*LINTLIBRARY*/
/*  traverse.c

    This code provides routines to traverse complex data structures.

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

    This file contains the various utility routines for traversing data
    structures in the general data structure supported in Karma.


    Written by      Richard Gooch   12-OCT-1992

    Updated by      Richard Gooch   12-OCT-1992

    Updated by      Richard Gooch   2-JAN-1993: Removed call to
  ds_alloc_contiguous_list  in  ds_traverse_list  .

    Updated by      Richard Gooch   3-JAN-1993: Switched to  K_ARRAY  .

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   25-MAR-1993: Took account of changes to
  list_header  structure.

    Last updated by Richard Gooch   26-NOV-1994: Moved to
  packages/ds/traverse.c


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>

/*PUBLIC_FUNCTION*/
flag ds_reorder_array (arr_desc, order_list, array, mod_desc)
/*  This routine will re-order an array with descriptor pointed to by
    arr_desc.
    The ordering of the dimensions is specified by the array of dimension
    numbers pointed to by  order_list  .
    The first entry in the order list contains the number of the dimension, in
    the old order, which is to become the most significant dimension in the new
    ordering.
    The array must be pointed to by  array  .If this is NULL, only the array
    descriptor can be re-ordered (no data can be re-ordered).
    If the value of  mod_desc  is TRUE the array descriptor will have it's
    dimension descriptors re-ordered, else they will not be. This is useful to
    traverse a data structure, re-ordering the data, and then finally
    re-ordering the array descriptor to match.
    The routine returns TRUE on success, else it returns FALSE.
*/
array_desc *arr_desc;
unsigned int order_list[];
char *array;
flag mod_desc;
{
    flag next = TRUE;
    unsigned int num_dim;
    unsigned int dim_count = 0;
    unsigned int start_dim;
    unsigned int end_dim;
    unsigned int block_size;
    unsigned int packet_size;
    unsigned int array_size;
    char *new_array;
    char *out_array;
    unsigned long *coordinates;
    dim_desc **new_dim_list;
    static char function_name[] = "ds_reorder_array";

    if ( (arr_desc == NULL) || (order_list == NULL) )
    {
	a_func_abort (function_name, "NULL pointer(s) passed");
	return (FALSE);
    }
    if ( (*arr_desc).packet == NULL )
    {
	(void) fprintf (stderr, "Array packet descriptor missing\n");
	a_prog_bug (function_name);
    }
    num_dim = (*arr_desc).num_dimensions;
    /*  Find first dimension which needs to be re-ordered  */
    start_dim = num_dim;
    dim_count = 0;
    while ( (dim_count < num_dim) && (start_dim == num_dim) )
    {
	if (order_list[dim_count] >= num_dim)
	{
	    (void) fprintf (stderr, "order_list[%u]: %u  >=  num_dim: %u\n",
			    dim_count, order_list[dim_count], num_dim);
	    a_prog_bug (function_name);
	}
	if (order_list[dim_count] != dim_count)
	{
	    start_dim = dim_count;
	}
	++dim_count;
    }
    if (start_dim >= num_dim)
    {
	return (TRUE);
    }
    /*  Find last dimension plus 1 which needs to be re-ordered  */
    end_dim = 0;
    dim_count = num_dim;
    while ( (dim_count > 0) && (end_dim == 0) )
    {
	if (order_list[dim_count - 1] != dim_count - 1)
	{
	    end_dim = dim_count;
	}
	--dim_count;
    }
    if (array != NULL)
    {
	/*  Array data needs to be re-ordered  */
	array_size = ds_get_array_size (arr_desc);
	packet_size = ds_get_packet_size ( (*arr_desc).packet );
	if ( ( new_array = m_alloc (packet_size * array_size) ) == NULL )
	{
	    m_error_notify (function_name, "temporary array");
	    return (FALSE);
	}
	if ( ( coordinates = (unsigned long *) m_alloc (sizeof *coordinates *
							num_dim) ) == NULL )
	{
	    m_error_notify (function_name, "co-ordinate counter array");
	    m_free (new_array);
	    return (FALSE);
	}
	for (dim_count = 0; dim_count < num_dim; ++dim_count)
	{
	    coordinates[dim_count] = 0;
	}
	/*  Determine size of block we can copy
	    (bottom dimensions with same order)  */
	block_size = packet_size;
	for (dim_count = end_dim; dim_count < num_dim; ++dim_count)
	{
	    block_size *= (* (*arr_desc).dimensions[dim_count] ).length;
	}
	out_array = new_array;
	while (next == TRUE)
	{
	    /*  Process this co-ordinate  */
	    m_copy (out_array,
		    array + ds_get_array_offset (arr_desc,
						 coordinates) * packet_size,
		    block_size);
	    out_array += block_size;
	    /*  Increment the dimension co-ordinate(s)  */
	    next = FALSE;
	    dim_count = end_dim;
	    while ( (next == FALSE) && (dim_count > 0) )
	    {
		if (++coordinates[ order_list[dim_count - 1] ] >=
		    (*(*arr_desc).dimensions[order_list[dim_count-1]]).length)
		{
		    coordinates[ order_list[dim_count - 1] ] = 0;
		    --dim_count;
		}
		else
		{
		    next = TRUE;
		}
	    }
	}
	m_free ( (char *) coordinates );
	/*  Copy re-ordered array back to old array and free temporary array */
	m_copy (array, new_array, array_size * packet_size);
	m_free (new_array);
    }
    if (mod_desc == TRUE)
    {
	/*  Need to re-order array descriptor  */
	/*  Create new dimension descriptor pointer array  */
	if ( ( new_dim_list = (dim_desc **)
	      m_alloc (sizeof *new_dim_list * num_dim) ) == NULL )
	{
	    m_error_notify (function_name, "temporary dimension list");
	    return (FALSE);
	}
	for (dim_count = 0; dim_count < num_dim; ++dim_count)
	{
	    new_dim_list[dim_count] =
	    (*arr_desc).dimensions[ order_list[dim_count] ];
	}
	/*  Copy back to array descriptor  */
	for (dim_count = 0; dim_count < num_dim; ++dim_count)
	{
	    (*arr_desc).dimensions[dim_count] = new_dim_list[dim_count];
	}
	m_free ( (char *) new_dim_list );
    }
    return (TRUE);
}  /*  End Function ds_reorder_array  */

/*PUBLIC_FUNCTION*/
flag ds_foreach_occurrence (pack_desc, packet, item, as_whole, function)
/*  This routine will traverse the general data structure with descriptor
    pointed to by  pack_desc  with data pointed to by  data  for occurrences
    of an item with name pointed to by  item  .
    The routine will process each occurrence using the function pointed to by
    function  .
    The following rules apply if  as_whole  is FALSE:
        If the item pointed to is the name of a dimension, then  function
	will be called for each occurrence of that dimension (ie. the other
	dimensions will be iterated through).
	If the item pointed to is the name of an element, then  function  will
	be called for each occurrence of the element in the array or linked
	list which it is in.
    The following rules apply if  as_whole  is TRUE:
        If the item pointed to is the name of a dimension or an element within
	a packet within an array,  function  is called once for each occurrence
	of the entire array.
	If the item pointed to is the name of an element in a linked list, then
	function  will be called once for each occurrence of the linked list.
    The interface to function is as follows:

    flag function (encls_desc, type, data, index)
        This routine will process an item with enclosing descriptor pointed to
	by  encls_desc  .
	The type of the descriptor must be in  type  .This may be one of the
	following:
	    NONE (for a packet descriptor),
	    IDENT_DIMENSION (for a dimension descriptor), 
	    K_ARRAY (for an array descriptor) or
	    LISTP (for a linked list descriptor).
	The pointer to the data must be in  data  .This is:
	    A pointer to the first element (for a packet descriptor)
	    A pointer to a portion of the array (for an array or dimension
	    descriptor)
	    A pointer to the linked list header (for a linked list descriptor)
	The index number of the item in the enclosing descriptor must be in
	index  ,for the cases where the enclosing descriptor is a packet, array
	or linked list descriptor. For the case where the enclosing descriptor
	is a dimension descriptor,  index  carries the stride (in bytes)
	between consecutive co-ordinates in the dimension.
	The routine returns TRUE on success, else it returns FALSE.
    char *encls_desc;
    unsigned int type;
    char *data;
    unsigned int index;
    {}

    If  function  returns FALSE, the routine will cease iterating and return
    FALSE, else it returns TRUE.
*/
packet_desc *pack_desc;
char *packet;
CONST char *item;
flag as_whole;
flag (*function) ();
{
    unsigned int elem_count = 0;
    unsigned int elem_num;
    char *data;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_foreach_occurrence";

    if ( (pack_desc == NULL) || (packet == NULL) || (item == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s)\n");
	a_prog_bug (function_name);
    }
    if (function == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer\n");
	a_prog_bug (function_name);
    }
    switch ( ds_f_name_in_packet (pack_desc, item, (char **) NULL,
				  (unsigned int *) NULL) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr, "Function: %s\titem: \"%s\" not found\n",
			function_name, item);
	return (FALSE);
/*
	break;
*/
      case IDENT_DIMENSION:
      case IDENT_ELEMENT:
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Function: %s\tmultiple occurences of item: \"%s\"\n",
			function_name, item);
/*
	return (FALSE);
*/
	break;
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"Bad return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
    }
    if ( ( elem_num = ds_f_elem_in_packet (pack_desc, item) ) <
	(*pack_desc).num_elements )
    {
	/*  Item is an atomic element is in this packet  */
	return ( (*function) ( (char *) pack_desc, NONE, packet,elem_num ) );
    }
    /*  Item is not in this packet: it must lie further down  */
    data = packet;
    while (elem_count < elem_num)
    {
	switch ( (*pack_desc).element_types[elem_count] )
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
	    break;
	  case K_ARRAY:
	    switch (ds_f_name_in_array ( (array_desc *)
					(*pack_desc).element_desc[elem_count],
					item, (char **) NULL,
					(unsigned int *) NULL ) )
	    {
	      case IDENT_NOT_FOUND:
		break;
	      case IDENT_DIMENSION:
	      case IDENT_ELEMENT:
		/*  Item lies somewhere down from this point  */
		return ( ds_foreach_in_array ( (array_desc *)
					      (*pack_desc).element_desc[elem_count],
					      *(char **) data, item, as_whole,
					      function ) );
/*
		break;
*/
	      case IDENT_MULTIPLE:
		(void) fprintf (stderr,
				"Function: %s\tmultiple occurences of item: \"%s\"\n",
				function_name, item);
/*
		return (FALSE);
*/
		break;
	      case IDENT_GEN_STRUCT:
	      default:
		(void) fprintf (stderr,
				"Bad return value from function: ds_f_name_in_packet\n");
		a_prog_bug (function_name);
		
		break;
	    }
	    break;
	  case LISTP:
	    switch (ds_f_name_in_packet ( (packet_desc *)
					 (*pack_desc).element_desc[elem_count],
					 item, (char **) NULL,
					 (unsigned int *) NULL ) )
	    {
	      case IDENT_NOT_FOUND:
		break;
	      case IDENT_DIMENSION:
	      case IDENT_ELEMENT:
		/*  Item lies somewhere down from this point  */
		return ( ds_foreach_in_list ( (packet_desc *)
					     (*pack_desc).element_desc[elem_count],
					     *(list_header **) data, item,
					     as_whole, function ) );
/*
		break;
*/
	      case IDENT_MULTIPLE:
		(void) fprintf (stderr,
				"Function: %s\tmultiple occurences of item: \"%s\"\n",
				function_name, item);
/*
		return (FALSE);
*/
		break;
	      case IDENT_GEN_STRUCT:
	      default:
		(void) fprintf (stderr,
				"Bad return value from function: ds_f_name_in_packet\n");
		a_prog_bug (function_name);
		
		break;
	    }
	    break;
	  default:
	    (void) fprintf (stderr, "Bad data type: %u\n",
			    (*pack_desc).element_types[elem_count]);
	    a_prog_bug (function_name);
	}
	data +=host_type_sizes[(*pack_desc).element_types[elem_count] ];
	++elem_count;
    }
    return (FALSE);
}  /*  End Function ds_foreach_occurrence  */

/*PUBLIC_FUNCTION*/
flag ds_foreach_in_array (arr_desc, array, item, as_whole, function)
/*  This routine will traverse the array with descriptor pointed to by
    arr_desc  with data pointed to by  array  for occurrences of an item
    with name pointed to by  item  .
    The routine will recursively process the array packets.
    The routine will process each occurrence using the function pointed to by
    function  .
    The following rules apply if  as_whole  is FALSE:
        If the item pointed to is the name of a dimension, then  function
	will be called for each occurrence of that dimension (ie. the other
	dimensions will be iterated through).
	If the item pointed to is the name of an element, then  function  will
	be called for each occurrence of the element in the array or linked
	list which it is in.
    The following rules apply if  as_whole  is TRUE:
        If the item pointed to is the name of a dimension or an element within
	a packet within an array,  function  is called once for each occurrence
	of the entire array.
	If the item pointed to is the name of an element in a linked list, then
	function  will be called once for each occurrence of the linked list.
    The interface to function is as follows:

    flag function (encls_desc, type, data, index)
        This routine will process an item with enclosing descriptor pointed to
	by  encls_desc  .
	The type of the descriptor must be in  type  .This may be one of the
	following:
	    NONE (for a packet descriptor),
	    IDENT_DIMENSION (for a dimension descriptor), 
	    K_ARRAY (for an array descriptor) or
	    LISTP (for a linked list descriptor).
	The pointer to the data must be in  data  .This is:
	    A pointer to the first element (for a packet descriptor)
	    A pointer to a portion of the array (for an array or dimension
	    descriptor)
	    A pointer to the linked list header (for a linked list descriptor)
	The index number of the item in the enclosing descriptor must be in
	index  ,for the cases where the enclosing descriptor is a packet, array
	or linked list descriptor. For the case where the enclosing descriptor
	is a dimension descriptor,  index  carries the stride (in bytes)
	between consecutive co-ordinates in the dimension.
	The routine returns TRUE on success, else it returns FALSE.
    char *encls_desc;
    unsigned int type;
    char *data;
    unsigned int index;
    {}

    If  function  returns FALSE, the routine will cease iterating and return
    FALSE, else it returns TRUE.
*/
array_desc *arr_desc;
char *array;
CONST char *item;
flag as_whole;
flag (*function) ();
{
    flag item_in_packet = FALSE;
    unsigned int dim_num;
    unsigned int elem_num;
    unsigned int high_iter_num;
    unsigned int low_iter_num;
    unsigned int dim_count;
    unsigned int high_iter_count;
    unsigned int low_iter_count;
    unsigned int pack_size;
    unsigned int step_size;
    char *data;
    packet_desc *pack_desc;
    dim_desc *dim;
    static char function_name[] = "ds_foreach_in_array";

    if ( (arr_desc == NULL) || (array == NULL) || (item == NULL) )
    {
	(void) fprintf (stderr, "NULL pointers(s)\n");
	a_prog_bug (function_name);
    }
    if (function == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer\n");
	a_prog_bug (function_name);
    }
    if ( (pack_desc = (*arr_desc).packet) == NULL )
    {
	(void) fprintf (stderr, "Array descriptor has no packet descriptor\n");
	a_prog_bug (function_name);
    }
    switch ( ds_f_name_in_array (arr_desc, item, (char **) NULL,
			      (unsigned int *) NULL) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr, "Function: %s\titem: \"%s\" not found\n",
			function_name, item);
	return (FALSE);
/*
	break;
*/
      case IDENT_DIMENSION:
      case IDENT_ELEMENT:
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Function: %s\tmultiple occurences of item: \"%s\"\n",
			function_name, item);
/*
	return (FALSE);
*/
	break;
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"Bad return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
    }
    if ( ( dim_num = ds_f_dim_in_array (arr_desc, item) ) <
	(*arr_desc).num_dimensions )
    {
	/*  Item is a dimension in this array  */
	if (as_whole == TRUE)
	{
	    /*  Process the array as a whole  */
	    return ( (*function) ( (char *) arr_desc, K_ARRAY, array,
				  dim_num ) );
	}
	/*  Process each occurrence of the dimension  */
	dim = (*arr_desc).dimensions[dim_num];
	pack_size = ds_get_packet_size (pack_desc);
	/*  Determine number of iterations for higher order dimensions  */
	high_iter_num = 1;
	for (dim_count = 0; dim_count < dim_num; ++dim_count)
	{
	    high_iter_num *= (* (*arr_desc).dimensions[dim_count] ).length;
	}
	/*  Determine number of iterations for lower order dimensions  */
	low_iter_num = 1;
	for (dim_count = dim_num + 1; dim_count < (*arr_desc).num_dimensions;
	     ++dim_count)
	{
	    low_iter_num *= (* (*arr_desc).dimensions[dim_count] ).length;
	}
	step_size = pack_size * low_iter_num;
	/*  Process the higher order dimensions  */
	for (high_iter_count = 0; high_iter_count < high_iter_num;
	     ++high_iter_count)
	{
	    data = array;
	    /*  Process the lower order dimensions  */
	    for (low_iter_count = 0; low_iter_count < low_iter_num;
		 ++low_iter_count)
	    {
		/*  Process occurrence of dimension  */
		if ( (*function) ( (char *) dim, IDENT_DIMENSION, data,
				  step_size )
		    == FALSE )
		{
		    return (FALSE);
		}
		/*  Increment  data  pointer  */
		data += pack_size;
	    }
	    /*  Increment  array  pointer  */
	    array += step_size * (*dim).length;
	}
	/*  Done with processing the required part of array  */
	return (TRUE);
    }

    /*  Item is either in the array packet descriptor or below it  */
    if ( ( elem_num = ds_f_elem_in_packet (pack_desc, item) ) <
	(*pack_desc).num_elements )
    {
	/*  Item is an atomic element in the array packet descriptor  */
	if (as_whole == TRUE)
	{
	    /*  Process the array as a whole  */
	    return ( (*function) ( (char *) arr_desc, K_ARRAY, array,
				  elem_num ) );
	}
	/*  Process the packet for all co-ordinates in array  */
	item_in_packet = TRUE;
    }
    /*  Process the packet descriptor for all co-ordinates in array  */
    /*  Determine step size  */
    step_size = ds_get_packet_size (pack_desc);
    high_iter_num = ds_get_array_size (arr_desc);
    for (high_iter_count = 0; high_iter_count < high_iter_num;
	 ++high_iter_count)
    {
	if (item_in_packet == TRUE)
	{
	    /*  Item is within the array packet descriptor  */
	    if ( (*function) ( (char *) pack_desc, NONE, array, elem_num )
		== FALSE )
	    {
		return (FALSE);
	    }
	}
	else
	{
	    /*  Item is below the array packet descriptor  */
	    if (ds_foreach_occurrence (pack_desc, array, item, as_whole,
				    function) == FALSE)
	    {
		return (FALSE);
	    }
	}
	array += step_size;
    }
    return (TRUE);
}   /*  End Function ds_foreach_in_array  */

/*PUBLIC_FUNCTION*/
flag ds_foreach_in_list (list_desc, list_head, item, as_whole, function)
/*  This routine will traverse the liked list with descriptor pointed to by
    list_desc  with data pointed to by  list_head  for occurrences of an item
    with name pointed to by  item  .
    The routine will recursively process the linked list packets.
    The routine will process each occurrence using the function pointed to by
    function  .
    The following rules apply if  as_whole  is FALSE:
        If the item pointed to is the name of a dimension, then  function
	will be called for each occurrence of that dimension (ie. the other
	dimensions will be iterated through).
	If the item pointed to is the name of an element, then  function  will
	be called for each occurrence of the element in the array or linked
	list which it is in.
    The following rules apply if  as_whole  is TRUE:
        If the item pointed to is the name of a dimension or an element within
	a packet within an array,  function  is called once for each occurrence
	of the entire array.
	If the item pointed to is the name of an element in a linked list, then
	function  will be called once for each occurrence of the linked list.
    The interface to function is as follows:

    flag function (encls_desc, type, data, index)
        This routine will process an item with enclosing descriptor pointed to
	by  encls_desc  .
	The type of the descriptor must be in  type  .This may be one of the
	following:
	    NONE (for a packet descriptor),
	    IDENT_DIMENSION (for a dimension descriptor), 
	    K_ARRAY (for an array descriptor) or
	    LISTP (for a linked list descriptor).
	The pointer to the data must be in  data  .This is:
	    A pointer to the first element (for a packet descriptor)
	    A pointer to a portion of the array (for an array or dimension
	    descriptor)
	    A pointer to the linked list header (for a linked list descriptor)
	The index number of the item in the enclosing descriptor must be in
	index  ,for the cases where the enclosing descriptor is a packet, array
	or linked list descriptor. For the case where the enclosing descriptor
	is a dimension descriptor,  index  carries the stride (in bytes)
	between consecutive co-ordinates in the dimension.
	The routine returns TRUE on success, else it returns FALSE.
    char *encls_desc;
    unsigned int type;
    char *data;
    unsigned int index;
    {}

    If  function  returns FALSE, the routine will cease iterating and return
    FALSE, else it returns TRUE.
*/
packet_desc *list_desc;
list_header *list_head;
CONST char *item;
flag as_whole;
flag (*function) ();
{
    unsigned int elem_num;
    unsigned int count;
    unsigned int pack_size;
    list_entry *curr_entry;
    char *data;
    static char function_name[] = "ds_foreach_in_list";

    if ( (list_desc == NULL) || (list_head == NULL) || (item == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (function == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer\n");
	a_prog_bug (function_name);
    }
    switch ( ds_f_name_in_packet (list_desc, item, (char **) NULL,
				  (unsigned int *) NULL) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr, "Function: %s\titem: \"%s\" not found\n",
			function_name, item);
/*
	return (FALSE);
*/
	break;
      case IDENT_DIMENSION:
      case IDENT_ELEMENT:
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Function: %s\tmultiple occurences of item: \"%s\"\n",
			function_name, item);
/*
	return (FALSE);
*/
	break;
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"Bad return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).length == 0 )
    {
	/*  The linked list is empty: this is not a problem: just skip it  */
	return (TRUE);
    }
    pack_size = ds_get_packet_size (list_desc);
    if ( ( elem_num  = ds_f_elem_in_packet (list_desc, item) ) <
	(*list_desc).num_elements )
    {
	/*  Item is an atomic element in this linked list  */
	if (as_whole == TRUE)
	{
	    /*  Process the linked list as a whole  */
	    return ( (*function) ( (char *) list_desc, LISTP,
				  (char *) list_head, elem_num ) );
	}
	/*  The list must be stepped through for each entry  */
	/*  Step through contiguous entries  */
	for (count = 0, data = (*list_head).contiguous_data;
	     count < (*list_head).contiguous_length;
	     ++count, data += pack_size)
	{
	    if ( (*function) ( (char *) list_desc, NONE, data, elem_num )
		== FALSE )
	    {
		return (FALSE);
	    }
	}
	/*  Step through the fragmented entries  */
	for (curr_entry = (*list_head).first_frag_entry; curr_entry != NULL;
	     curr_entry = (*curr_entry).next)
	{
	    if ( (*function) ( (char *) list_desc, NONE, (*curr_entry).data,
			      elem_num ) == FALSE )
	    {
		return (FALSE);
	    }
	}
	/*  Finished with linked list  */
	return (TRUE);
    }
    /*  Item is below the list packet descriptor  */
    /*  The list must be stepped through for each entry  */
    /*  Step through contiguous entries  */
    for (count = 0, data = (*list_head).contiguous_data;
	 count < (*list_head).contiguous_length;
	 ++count, data += pack_size)
    {
	if (ds_foreach_occurrence (list_desc, data, item, as_whole, function)
	    == FALSE)
	{
	    return (FALSE);
	}
    }
    for (curr_entry = (*list_head).first_frag_entry; curr_entry != NULL;
	 curr_entry = (*curr_entry).next)
    {
	if (ds_foreach_occurrence (list_desc, (*curr_entry).data, item,
				   as_whole, function) == FALSE)
	{
	    return (FALSE);
	}
    }
    /*  Finished with linked list  */
    return (TRUE);
}   /*  End Function ds_foreach_in_list  */

/*PUBLIC_FUNCTION*/
flag ds_traverse_and_process (inp_desc, inp_data, out_desc, out_data, as_whole,
			      function)
/*  This routine will traverse a general data structure and will process
    a sub structure for every occurence in the above lying structure.
    The input data structure descriptor must be pointed to by  inp_desc  and
    the input data must be pointed to by  inp_data  .
    The output data structure descriptor must be pointed to by  out_desc  and
    the output data must be pointed to by  out_data  .
    Wherever there is a difference in the two data structures' descriptors,
    the pointers to the descriptors and the data where they diverge are
    passed to the function pointed to by  function  .
    The following rules apply if  as_whole  is FALSE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	function  .
	If any aspect of the two array's dimension descriptors are different,
	then the arrays are deemed divergent and their descriptors are passed
	to  function  .
    The following rules apply if  as_whole  is TRUE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	function  .
	If any aspect of the two array's dimension descriptors are different,
	or their packet descriptors are divergent, then the arrays are deemed
	divergent and the array descriptors are passed to  function  .
	If the packet descriptors for linked lists are divergent, then the two
	lists are deemed divergent and their descriptors and headers are passed
	to  function  .
    The interface to this function is as follows:

    flag function (inp_desc, inp_type, inp_data, out_desc, out_type, out_data)
        This routine will process an input general data structure with
        descriptor pointed to by  inp_desc  ,the descriptor type must be in
        inp_type  and  the input data pointed to by  inp_data  .
        The output data structure descriptor must be pointed to by
        out_desc  ,the type of descriptor must be in  out_type  and the
        output data must be pointed to by  out_data  .
	The  inp_type  and  out_type  may take the following values:
	    NONE (for a packet descriptor)
	    K_ARRAY (for an array descriptor)
	    LISTP (for a linked list descriptor)
        The routine returns TRUE on success, else it returns FALSE.
    char *inp_desc;
    unsigned int inp_type;
    char *inp_data;
    char *out_desc;
    unsigned int out_type;
    char *out_data;
    {}

    The routine returns TRUE on success, else it returns FALSE.
*/
packet_desc *inp_desc;
char *inp_data;
packet_desc *out_desc;
char *out_data;
flag as_whole;
flag (*function) ();
{
    unsigned int elem_count = 0;
    unsigned int inp_type;
    unsigned int out_type;
    char *sub1_desc;
    char *sub2_desc;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_traverse_and_process";

    if ( (inp_desc == NULL) || (inp_data == NULL) ||
        (out_desc == NULL) || (out_data == NULL) )
    {
	a_func_abort (function_name, "NULL pointer(s) passed");
        return (FALSE);
    }
    if (function == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer passed\n");
	a_prog_bug (function_name);
    }
    if (ds_compare_packet_desc (inp_desc, out_desc, FALSE) != TRUE)
    {
	/*  Packets differ: pass to  function  and finish  */
	return ( (*function) ( (char *) inp_desc, NONE, inp_data,
			      (char *) out_desc, NONE, out_data ) );
    }
    while (elem_count < (*inp_desc).num_elements)
    {
	inp_type = (*inp_desc).element_types[elem_count];
        out_type = (*out_desc).element_types[elem_count];
        sub1_desc = (*inp_desc).element_desc[elem_count];
        sub2_desc = (*out_desc).element_desc[elem_count];
        if (inp_type != out_type)
        {
	    /*  Difference in element types: should not have happened */
	    (void) fprintf (stderr,
			    "Element types: %u and %u are now different!\n",
			    inp_type, out_type);
	    a_prog_bug (function_name);
	}
	if (inp_type == K_ARRAY)
	{
	    if (ds_traverse_array ( (array_desc *) sub1_desc,
				   *(char **) inp_data,
				   (array_desc *) sub2_desc,
				   *(char **) out_data,
				   as_whole, function) == FALSE)
	    {
		return (FALSE);
	    }
	}
	if (inp_type == LISTP)
	{
	    if (ds_traverse_list ( (packet_desc *) sub1_desc,
				  *(list_header **) inp_data,
				  (packet_desc *) sub2_desc,
				  *(list_header **) out_data,
				  as_whole, function) == FALSE)
	    {
		return (FALSE);
	    }
        }
        inp_data += host_type_sizes[inp_type];
        out_data += host_type_sizes[out_type];
        ++elem_count;
    }
    return (TRUE);
}   /*  End Function ds_traverse_and_process   */

/*PUBLIC_FUNCTION*/
flag ds_traverse_array (inp_desc, inp_data, out_desc, out_data, as_whole,
			function)
/*  This routine will traverse a general data structure and will process
    a sub structure for every occurence in the above lying structure.
    The input data structure descriptor must be pointed to by  inp_desc  and
    the input data must be pointed to by  inp_data  .
    The output data structure descriptor must be pointed to by  out_desc  and
    the output data must be pointed to by  out_data  .
    Wherever there is a difference in the two data structures' descriptors,
    the pointers to the descriptors and the data where they diverge are
    passed to the function pointed to by  function  .
    The following rules apply if  as_whole  is FALSE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	function  .
	If any aspect of the two array's dimension descriptors are different,
	then the arrays are deemed divergent and their descriptors are passed
	to  function  .
    The following rules apply if  as_whole  is TRUE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	function  .
	If any aspect of the two array's dimension descriptors are different,
	or their packet descriptors are divergent, then the arrays are deemed
	divergent and the array descriptors are passed to  function  .
	If the packet descriptors for linked lists are divergent, then the two
	lists are deemed divergent and their descriptors and headers are passed
	to  function  .
    The interface to this function is as follows:

    flag function (inp_desc, inp_type, inp_data, out_desc, out_type, out_data)
        This routine will process an input general data structure with
        descriptor pointed to by  inp_desc  ,the descriptor type must be in
        inp_type  and  the input data pointed to by  inp_data  .
        The output data structure descriptor must be pointed to by
        out_desc  ,the type of descriptor must be in  out_type  and the
        output data must be pointed to by  out_data  .
	The  inp_type  and  out_type  may take the following values:
	    NONE (for a packet descriptor)
	    K_ARRAY (for an array descriptor)
	    LISTP (for a linked list descriptor)
        The routine returns TRUE on success, else it returns FALSE.
    char *inp_desc;
    unsigned int inp_type;
    char *inp_data;
    char *out_desc;
    unsigned int out_type;
    char *out_data;
    {}

    The routine returns TRUE on success, else it returns FALSE.
*/
array_desc *inp_desc;
char *inp_data;
array_desc *out_desc;
char *out_data;
flag as_whole;
flag (*function) ();
{
    unsigned int size1;
    unsigned int size2;
    unsigned int array_length;
    unsigned int count;
    packet_desc *pack1_desc;
    packet_desc *pack2_desc;
    static char function_name[] = "ds_traverse_array";

    if ( (inp_desc == NULL) || (inp_data == NULL) ||
        (out_desc == NULL) || (out_data == NULL) )
    {
	a_func_abort (function_name, "NULL pointer(s) passed");
        return (FALSE);
    }
    if (function == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer passed\n");
	a_prog_bug (function_name);
    }
    if (ds_compare_array_desc (inp_desc, out_desc, FALSE) != TRUE)
    {
	/*  Array descriptors are different: must pass to  function  */
	return ( (*function) ( (char *) inp_desc, K_ARRAY, inp_data,
			      (char *) out_desc, K_ARRAY, out_data ) );
    }
    pack1_desc = (*inp_desc).packet;
    pack2_desc = (*out_desc).packet;
    if ( (as_whole == TRUE) && (ds_compare_packet_desc (pack1_desc, pack2_desc,
							FALSE ) == FALSE) )
    {
	/*  The packet descriptors are different and  as_whole  is TRUE  */
	return ( (*function) ( (char *) inp_desc, K_ARRAY, inp_data,
			      (char *) out_desc, K_ARRAY, out_data ) );
    }
    size1 = ds_get_packet_size (pack1_desc);
    size2 = ds_get_packet_size (pack2_desc);
    array_length = ds_get_array_size (inp_desc);
    /*  The array descriptors are the same. The packet descriptors are the same
	or the  as_whole  flag is FALSE  */
    /*  Iterate through the arrays  */
    for (count = 0; count < array_length; ++count)
    {
	if (ds_traverse_and_process (pack1_desc, inp_data, pack2_desc,
				     out_data, as_whole, function) != TRUE)
	{
	    return (FALSE);
	}
	inp_data += size1;
	out_data += size2;
    }
    return (TRUE);
}   /*  End Function ds_traverse_array  */

/*PUBLIC_FUNCTION*/
flag ds_traverse_list (inp_desc, inp_head, out_desc, out_head, as_whole,
		       function)
/*  This routine will traverse a general data structure and will process
    a sub structure for every occurence in the above lying structure.
    The input data structure descriptor must be pointed to by  inp_desc  and
    the input list header must be pointed to by  inp_head  .
    The output data structure descriptor must be pointed to by  out_desc  and
    the output lsit header must be pointed to by  out_head  .
    Wherever there is a difference in the two data structures' descriptors,
    the pointers to the descriptors and the data where they diverge are
    passed to the function pointed to by  function  .
    The following rules apply if  as_whole  is FALSE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	function  .
	If any aspect of the two array's dimension descriptors are different,
	then the arrays are deemed divergent and their descriptors are passed
	to  function  .
    The following rules apply if  as_whole  is TRUE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	function  .
	If any aspect of the two array's dimension descriptors are different,
	or their packet descriptors are divergent, then the arrays are deemed
	divergent and the array descriptors are passed to  function  .
	If the packet descriptors for linked lists are divergent, then the two
	lists are deemed divergent and their descriptors and headers are passed
	to  function  .
    The interface to this function is as follows:

    flag function (inp_desc, inp_type, inp_data, out_desc, out_type, out_data)
        This routine will process an input general data structure with
        descriptor pointed to by  inp_desc  ,the descriptor type must be in
        inp_type  and  the input data pointed to by  inp_data  .
        The output data structure descriptor must be pointed to by
        out_desc  ,the type of descriptor must be in  out_type  and the
        output data must be pointed to by  out_data  .
	The  inp_type  and  out_type  may take the following values:
	    NONE (for a packet descriptor)
	    K_ARRAY (for an array descriptor)
	    LISTP (for a linked list descriptor)
        The routine returns TRUE on success, else it returns FALSE.
    char *inp_desc;
    unsigned int inp_type;
    char *inp_data;
    char *out_desc;
    unsigned int out_type;
    char *out_data;
    {}

    The routine returns TRUE on success, else it returns FALSE.
*/
packet_desc *inp_desc;
list_header *inp_head;
packet_desc *out_desc;
list_header *out_head;
flag as_whole;
flag (*function) ();
{
    unsigned int pack_size1;
    unsigned int pack_size2;
    unsigned int count1;
    unsigned int count2;
    list_entry *entry1;
    list_entry *entry2;
    char *data1;
    char *data2;
    static char function_name[] = "ds_traverse_list";

    if ( (inp_desc == NULL) || (inp_head == NULL) ||
        (out_desc == NULL) || (out_head == NULL) )
    {
	a_func_abort (function_name, "NULL pointer(s) passed");
        return (FALSE);
    }
    if (function == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer passed\n");
	a_prog_bug (function_name);
    }
    if (ds_compare_packet_desc (inp_desc, out_desc, FALSE) == FALSE)
    {
	/*  Packet descriptors are different  */
	if (as_whole == TRUE)
	{
	    /*  The packet descriptors are different and  as_whole  is TRUE  */
	    /*  Must process linked lists as a whole  */
	    return ( (*function) ( (char *) inp_desc, LISTP, (char *) inp_head,
				  (char *) out_desc, LISTP,
				  (char *) out_head ) );
	}
#ifdef dummy
	/*  Must process each packet individually: do it later  */
	if ( (*out_head).length < 1 )
	{
	    /*  Second list is empty: allocate for it  */
	    if (ds_alloc_contiguous_list (out_desc, out_head,
					  (*inp_head).length, TRUE, TRUE)
		== FALSE)
	    {
		m_error_notify (function_name, "output list entries");
		return (FALSE);
	    }
	    (*out_head).sort_type = SORT_RANDOM;
	}
#endif
    }
    if ( (*inp_head).length != (*out_head).length )
    {
	/*  Since the lists are of different lengths, MUST process as whole  */
	return ( (*function) ( (char *) inp_desc, LISTP, (char *) inp_head,
			      (char *) out_desc, LISTP, (char *) out_head ) );
    }
    /*  The packet descriptors are the same or the  as_whole  flag is FALSE  */
    /*  Iterate through the lists  */
    /*  Process the contiguous entries of the first list  */
    data1 = (*inp_head).contiguous_data;
    data2 = (*out_head).contiguous_data;
    pack_size1 = ds_get_packet_size (inp_desc);
    pack_size2 = ds_get_packet_size (out_desc);
    entry2 = (*out_head).first_frag_entry;
    for (count1 = 0, count2 = 0; count1 < (*inp_head).contiguous_length;
	 ++count1, data1 += pack_size1)
    {
	if (count2 < (*out_head).contiguous_length)
	{
	    /*  Still in second list's contiguous section  */
	    if (ds_traverse_and_process (inp_desc, data1,
					 out_desc, data2,
					 as_whole, function) != TRUE)
	    {
		return (FALSE);
	    }
	    ++count2;
	    data2 += pack_size2;
	}
	else
	{
	    /*  In second list's fragmented section  */
	    if (ds_traverse_and_process (inp_desc, data1,
					 out_desc, (*entry2).data,
					 as_whole, function) != TRUE)
	    {
		return (FALSE);
	    }
	    entry2 = (*entry2).next;
	}
    }
    /*  Process the fragmented entries of the first list  */
    for (entry1 = (*inp_head).first_frag_entry; entry1 != NULL;
	 entry1 = (*entry1).next)
    {
	if (count2 < (*out_head).contiguous_length)
	{
	    /*  Still in second list's contiguous section  */
	    if (ds_traverse_and_process (inp_desc, (*entry1).data,
					 out_desc, data2,
					 as_whole, function) != TRUE)
	    {
		return (FALSE);
	    }
	    ++count2;
	    data2 += pack_size2;
	}
	else
	{
	    /*  In second list's fragmented section  */
	    if (ds_traverse_and_process (inp_desc, (*entry1).data,
					 out_desc, (*entry2).data,
					 as_whole, function) != TRUE)
	    {
		return (FALSE);
	    }
	    entry2 = (*entry2).next;
	}
    }
    return (TRUE);
}   /*  End Function ds_traverse_list  */
