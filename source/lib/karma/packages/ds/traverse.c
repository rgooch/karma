/*LINTLIBRARY*/
/*  traverse.c

    This code provides routines to traverse complex data structures.

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

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/ds/traverse.c

    Last updated by Richard Gooch   9-APR-1996: Changed to new documentation
  format.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>

/*PUBLIC_FUNCTION*/
flag ds_reorder_array (array_desc *arr_desc, unsigned int order_list[],
		       char *array, flag mod_desc)
/*  [SUMMARY] Re-order a multi-dimensional array.
    <arr_desc> The array descriptor.
    <order_list> This specifies the ordering of the dimensions. The first entry
    in the order list contains the number of the dimension, in the old order,
    which is to become the most significant dimension in the new ordering.
    <array> The array. If this is NULL, only the array descriptor can be
    re-ordered (no data can be re-ordered).
    <mod_desc> If TRUE the array descriptor will have its dimension descriptors
    re-ordered, else they will not be. This is useful to traverse a data
    structure, re-ordering the data, and then finally re-ordering the array
    descriptor to match.
    [RETURNS] TRUE on success, else FALSE.
*/
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
    if (arr_desc->packet == NULL)
    {
	(void) fprintf (stderr, "Array packet descriptor missing\n");
	a_prog_bug (function_name);
    }
    num_dim = arr_desc->num_dimensions;
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
	packet_size = ds_get_packet_size (arr_desc->packet);
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
	    block_size *= arr_desc->dimensions[dim_count]->length;
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
	    while ( !next && (dim_count > 0) )
	    {
		if (++coordinates[ order_list[dim_count - 1] ] >=
		    arr_desc->dimensions[order_list[dim_count-1]]->length)
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
	    arr_desc->dimensions[ order_list[dim_count] ];
	}
	/*  Copy back to array descriptor  */
	for (dim_count = 0; dim_count < num_dim; ++dim_count)
	{
	    arr_desc->dimensions[dim_count] = new_dim_list[dim_count];
	}
	m_free ( (char *) new_dim_list );
    }
    return (TRUE);
}  /*  End Function ds_reorder_array  */

/*PUBLIC_FUNCTION*/
flag ds_foreach_occurrence ( packet_desc *pack_desc, char *packet,
			     CONST char *item, flag as_whole,
			     flag (*func) () )
/*  [SUMMARY] Recursively traverse a data structure, searching for an item.
    <pack_desc> The packet descriptor of the structure.
    <packet> The packet for the structure.
    <item> The name of the item to search for.
    <as_whole> If FALSE:
        If the item pointed to is the name of a dimension, then <<function>>
	will be called for each occurrence of that dimension (ie. the other
	dimensions will be iterated through).
	If the item pointed to is the name of an element, then <<function>>
	will be called for each occurrence of the element in the array or
	linked list which it is in.
    If TRUE:
        If the item pointed to is the name of a dimension or an element within
	a packet within an array, <<function>> is called once for each
	occurrence of the entire array.
	If the item pointed to is the name of an element in a linked list, then
	<<function>> will be called once for each occurrence of the linked list
    <func> The function to call for each occurrence. The prototype function is
    [<DS_PROTO_foreach_func>]. If this returns FALSE, iterations are stopped.
    [RETURNS] TRUE if all iterations completed successfully, else FALSE.
*/
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
    if (func == NULL)
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
	pack_desc->num_elements )
    {
	/*  Item is an atomic element is in this packet  */
	return ( (*func) ( (char *) pack_desc, NONE, packet,elem_num ) );
    }
    /*  Item is not in this packet: it must lie further down  */
    data = packet;
    while (elem_count < elem_num)
    {
	switch (pack_desc->element_types[elem_count])
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
					pack_desc->element_desc[elem_count],
					item, (char **) NULL,
					(unsigned int *) NULL ) )
	    {
	      case IDENT_NOT_FOUND:
		break;
	      case IDENT_DIMENSION:
	      case IDENT_ELEMENT:
		/*  Item lies somewhere down from this point  */
		return ( ds_foreach_in_array ( (array_desc *)
					      pack_desc->element_desc[elem_count],
					      *(char **) data, item, as_whole,
					      func ) );
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
					 pack_desc->element_desc[elem_count],
					 item, (char **) NULL,
					 (unsigned int *) NULL ) )
	    {
	      case IDENT_NOT_FOUND:
		break;
	      case IDENT_DIMENSION:
	      case IDENT_ELEMENT:
		/*  Item lies somewhere down from this point  */
		return ( ds_foreach_in_list ( (packet_desc *)
					     pack_desc->element_desc[elem_count],
					     *(list_header **) data, item,
					     as_whole, func ) );
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
			    pack_desc->element_types[elem_count]);
	    a_prog_bug (function_name);
	}
	data +=host_type_sizes[pack_desc->element_types[elem_count] ];
	++elem_count;
    }
    return (FALSE);
}  /*  End Function ds_foreach_occurrence  */

/*PUBLIC_FUNCTION*/
flag ds_foreach_in_array ( array_desc *arr_desc, char *array, CONST char *item,
			   flag as_whole, flag (*func) () )
/*  [SUMMARY] Recursively traverse an array, searching for an item.
    <arr_desc> The array descriptor.
    <array> The array data.
    <item> The name of the item to search for.
    <as_whole> If FALSE:
        If the item pointed to is the name of a dimension, then <<func>>
	will be called for each occurrence of that dimension (ie. the other
	dimensions will be iterated through).
	If the item pointed to is the name of an element, then <<func>>
	will be called for each occurrence of the element in the array or
	linked list which it is in.
    If TRUE:
        If the item pointed to is the name of a dimension or an element within
	a packet within an array, <<func>> is called once for each
	occurrence of the entire array.
	If the item pointed to is the name of an element in a linked list, then
	<<func>> will be called once for each occurrence of the linked list
    <func> The function to call for each occurrence. The prototype function is
    [<DS_PROTO_foreach_func>]. If this returns FALSE, iterations are stopped.
    [RETURNS] TRUE if all iterations completed successfully, else FALSE.
*/
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
    if (func == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer\n");
	a_prog_bug (function_name);
    }
    if ( (pack_desc = arr_desc->packet) == NULL )
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
	arr_desc->num_dimensions )
    {
	/*  Item is a dimension in this array  */
	if (as_whole == TRUE)
	{
	    /*  Process the array as a whole  */
	    return ( (*func) ( (char *) arr_desc, K_ARRAY, array,
				  dim_num ) );
	}
	/*  Process each occurrence of the dimension  */
	dim = arr_desc->dimensions[dim_num];
	pack_size = ds_get_packet_size (pack_desc);
	/*  Determine number of iterations for higher order dimensions  */
	high_iter_num = 1;
	for (dim_count = 0; dim_count < dim_num; ++dim_count)
	{
	    high_iter_num *= arr_desc->dimensions[dim_count]->length;
	}
	/*  Determine number of iterations for lower order dimensions  */
	low_iter_num = 1;
	for (dim_count = dim_num + 1; dim_count < arr_desc->num_dimensions;
	     ++dim_count)
	{
	    low_iter_num *= arr_desc->dimensions[dim_count]->length;
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
		if ( !(*func) ( (char *) dim, IDENT_DIMENSION, data,
				    step_size ) )
		{
		    return (FALSE);
		}
		/*  Increment  data  pointer  */
		data += pack_size;
	    }
	    /*  Increment  array  pointer  */
	    array += step_size * dim->length;
	}
	/*  Done with processing the required part of array  */
	return (TRUE);
    }

    /*  Item is either in the array packet descriptor or below it  */
    if ( ( elem_num = ds_f_elem_in_packet (pack_desc, item) ) <
	pack_desc->num_elements )
    {
	/*  Item is an atomic element in the array packet descriptor  */
	if (as_whole == TRUE)
	{
	    /*  Process the array as a whole  */
	    return ( (*func) ( (char *) arr_desc, K_ARRAY, array,
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
	    if ( !(*func) ( (char *) pack_desc, NONE, array, elem_num ) )
	    {
		return (FALSE);
	    }
	}
	else
	{
	    /*  Item is below the array packet descriptor  */
	    if ( !ds_foreach_occurrence (pack_desc, array, item, as_whole,
					 func) )
	    {
		return (FALSE);
	    }
	}
	array += step_size;
    }
    return (TRUE);
}   /*  End Function ds_foreach_in_array  */

/*PUBLIC_FUNCTION*/
flag ds_foreach_in_list ( packet_desc *list_desc, list_header *list_head,
			  CONST char *item, flag as_whole, flag (*func) () )
/*  [SUMMARY] Recursively traverse a linked list, searching for an item.
    <list_desc> The packet descriptor for the linked list.
    <list_head> The linked list header.
    <item> The name of the item to search for.
    <as_whole> If FALSE:
        If the item pointed to is the name of a dimension, then <<func>>
	will be called for each occurrence of that dimension (ie. the other
	dimensions will be iterated through).
	If the item pointed to is the name of an element, then <<func>>
	will be called for each occurrence of the element in the array or
	linked list which it is in.
    If TRUE:
        If the item pointed to is the name of a dimension or an element within
	a packet within an array, <<func>> is called once for each
	occurrence of the entire array.
	If the item pointed to is the name of an element in a linked list, then
	<<func>> will be called once for each occurrence of the linked list
    <func> The function to call for each occurrence. The prototype function is
    [<DS_PROTO_foreach_func>]. If this returns FALSE, iterations are stopped.
    [RETURNS] TRUE if all iterations completed successfully, else FALSE.
*/
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
    if (func == NULL)
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
    if (list_head->length == 0)
    {
	/*  The linked list is empty: this is not a problem: just skip it  */
	return (TRUE);
    }
    pack_size = ds_get_packet_size (list_desc);
    if ( ( elem_num  = ds_f_elem_in_packet (list_desc, item) ) <
	list_desc->num_elements )
    {
	/*  Item is an atomic element in this linked list  */
	if (as_whole == TRUE)
	{
	    /*  Process the linked list as a whole  */
	    return ( (*func) ( (char *) list_desc, LISTP,
				  (char *) list_head, elem_num ) );
	}
	/*  The list must be stepped through for each entry  */
	/*  Step through contiguous entries  */
	for (count = 0, data = list_head->contiguous_data;
	     count < list_head->contiguous_length;
	     ++count, data += pack_size)
	{
	    if ( !(*func) ( (char *) list_desc, NONE, data, elem_num ) )
	    {
		return (FALSE);
	    }
	}
	/*  Step through the fragmented entries  */
	for (curr_entry = list_head->first_frag_entry; curr_entry != NULL;
	     curr_entry = curr_entry->next)
	{
	    if ( !(*func) ( (char *) list_desc, NONE, curr_entry->data,
				elem_num ) )
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
    for (count = 0, data = list_head->contiguous_data;
	 count < list_head->contiguous_length;
	 ++count, data += pack_size)
    {
	if ( !ds_foreach_occurrence (list_desc, data, item, as_whole,
				     func) )
	{
	    return (FALSE);
	}
    }
    for (curr_entry = list_head->first_frag_entry; curr_entry != NULL;
	 curr_entry = curr_entry->next)
    {
	if ( !ds_foreach_occurrence (list_desc, curr_entry->data, item,
				     as_whole, func) )
	{
	    return (FALSE);
	}
    }
    /*  Finished with linked list  */
    return (TRUE);
}   /*  End Function ds_foreach_in_list  */

/*PUBLIC_FUNCTION*/
flag ds_traverse_and_process ( packet_desc *desc1, char *data1,
			       packet_desc *desc2, char *data2,
			       flag as_whole, flag (*func) () )
/*  [SUMMARY] Recursively traverse a pair of data structures.
    [PURPOSE] This routine will traverse a pair of general data structures and
    will process a sub structure for every occurence wherever there is a
    difference in the two data structures' descriptors.
    <desc1> One of the structures packet descriptor.
    <data1> One of the structures data.
    <desc2> The other structures packet descriptor.
    <data2> The other structures data.
    <as_whole> If FALSE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	<<func>>.
	If any aspect of the two array's dimension descriptors are different,
	then the arrays are deemed divergent and their descriptors are passed
	to <<func>>.
    If TRUE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	<<func>>.
	If any aspect of the two array's dimension descriptors are different,
	or their packet descriptors are divergent, then the arrays are deemed
	divergent and the array descriptors are passed to <<func>>.
	If the packet descriptors for linked lists are divergent, then the two
	lists are deemed divergent and their descriptors and headers are passed
	to <<func>>.
    <func> The function to call for each divergence. The prototype function is
    [<DS_PROTO_traverse_func>]. If this returns FALSE, iterations are stopped.
    [RETURNS] TRUE if all iterations completed successfully, else FALSE.
*/
{
    unsigned int elem_count = 0;
    unsigned int type1;
    unsigned int type2;
    char *sub1_desc;
    char *sub2_desc;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_traverse_and_process";

    if ( (desc1 == NULL) || (data1 == NULL) ||
        (desc2 == NULL) || (data2 == NULL) )
    {
	a_func_abort (function_name, "NULL pointer(s) passed");
        return (FALSE);
    }
    if (func == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( !ds_compare_packet_desc (desc1, desc2, FALSE) )
    {
	/*  Packets differ: pass to  function  and finish  */
	return ( (*func) ( (char *) desc1, NONE, data1,
			      (char *) desc2, NONE, data2 ) );
    }
    while (elem_count < desc1->num_elements)
    {
	type1 = desc1->element_types[elem_count];
        type2 = desc2->element_types[elem_count];
        sub1_desc = desc1->element_desc[elem_count];
        sub2_desc = desc2->element_desc[elem_count];
        if (type1 != type2)
        {
	    /*  Difference in element types: should not have happened */
	    (void) fprintf (stderr,
			    "Element types: %u and %u are now different!\n",
			    type1, type2);
	    a_prog_bug (function_name);
	}
	if (type1 == K_ARRAY)
	{
	    if ( !ds_traverse_array ( (array_desc *) sub1_desc,
				      *(char **) data1,
				      (array_desc *) sub2_desc,
				      *(char **) data2,
				      as_whole, func) )
	    {
		return (FALSE);
	    }
	}
	if (type1 == LISTP)
	{
	    if ( !ds_traverse_list ( (packet_desc *) sub1_desc,
				     *(list_header **) data1,
				     (packet_desc *) sub2_desc,
				     *(list_header **) data2,
				     as_whole, func) )
	    {
		return (FALSE);
	    }
        }
        data1 += host_type_sizes[type1];
        data2 += host_type_sizes[type2];
        ++elem_count;
    }
    return (TRUE);
}   /*  End Function ds_traverse_and_process   */

/*PUBLIC_FUNCTION*/
flag ds_traverse_array ( array_desc *desc1, char *data1,
			 array_desc *desc2, char *data2, flag as_whole,
			 flag (*func) () )
/*  [SUMMARY] Recursively traverse a pair of arrays.
    [PURPOSE] This routine will traverse a pair of multi-dimensional arrays
    will process a sub structure for every occurence wherever there is a
    difference in the two data structures' descriptors.
    <desc1> One of the array descriptors.
    <data1> One of the arrays.
    <desc2> The other array descriptor.
    <data2> The other array.
    <as_whole> If FALSE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	<<func>>.
	If any aspect of the two array's dimension descriptors are different,
	then the arrays are deemed divergent and their descriptors are passed
	to <<func>>.
    If TRUE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	<<func>>.
	If any aspect of the two array's dimension descriptors are different,
	or their packet descriptors are divergent, then the arrays are deemed
	divergent and the array descriptors are passed to <<func>>.
	If the packet descriptors for linked lists are divergent, then the two
	lists are deemed divergent and their descriptors and headers are passed
	to <<func>>.
    <func> The function to call for each divergence. The prototype function is
    [<DS_PROTO_traverse_func>]. If this returns FALSE, iterations are stopped.
    [RETURNS] TRUE if all iterations completed successfully, else FALSE.
*/
{
    unsigned int size1;
    unsigned int size2;
    unsigned int array_length;
    unsigned int count;
    packet_desc *pack1_desc;
    packet_desc *pack2_desc;
    static char function_name[] = "ds_traverse_array";

    if ( (desc1 == NULL) || (data1 == NULL) ||
        (desc2 == NULL) || (data2 == NULL) )
    {
	a_func_abort (function_name, "NULL pointer(s) passed");
        return (FALSE);
    }
    if (func == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( !ds_compare_array_desc (desc1, desc2, FALSE) )
    {
	/*  Array descriptors are different: must pass to  func  */
	return ( (*func) ( (char *) desc1, K_ARRAY, data1,
			      (char *) desc2, K_ARRAY, data2 ) );
    }
    pack1_desc = desc1->packet;
    pack2_desc = desc2->packet;
    if ( as_whole && !ds_compare_packet_desc (pack1_desc, pack2_desc,
					      FALSE) )
    {
	/*  The packet descriptors are different and  as_whole  is TRUE  */
	return ( (*func) ( (char *) desc1, K_ARRAY, data1,
			      (char *) desc2, K_ARRAY, data2 ) );
    }
    size1 = ds_get_packet_size (pack1_desc);
    size2 = ds_get_packet_size (pack2_desc);
    array_length = ds_get_array_size (desc1);
    /*  The array descriptors are the same. The packet descriptors are the same
	or the  as_whole  flag is FALSE  */
    /*  Iterate through the arrays  */
    for (count = 0; count < array_length; ++count)
    {
	if ( !ds_traverse_and_process (pack1_desc, data1, pack2_desc,
				       data2, as_whole, func) )
	{
	    return (FALSE);
	}
	data1 += size1;
	data2 += size2;
    }
    return (TRUE);
}   /*  End Function ds_traverse_array  */

/*PUBLIC_FUNCTION*/
flag ds_traverse_list ( packet_desc *desc1, list_header *head1,
			packet_desc *desc2, list_header *head2,
			flag as_whole, flag (*func) () )
/*  [SUMMARY] Recursively traverse a pair of linked lists.
    [PURPOSE] This routine will traverse a pair of linked lists and will
    process a sub structure for every occurence wherever there is a difference
    in the two data structures' descriptors.
    <desc1> One of lists descriptor.
    <data1> One of the lists header.
    <desc2> The other lists descriptor.
    <data2> The other lists header.
    <as_whole> If FALSE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	<<func>>.
	If any aspect of the two array's dimension descriptors are different,
	then the arrays are deemed divergent and their descriptors are passed
	to <<func>>.
    If TRUE:
        If the type or name of any element in the two packet descriptors are
	different, the packets are deemed divergent and they are passed to
	<<func>>.
	If any aspect of the two array's dimension descriptors are different,
	or their packet descriptors are divergent, then the arrays are deemed
	divergent and the array descriptors are passed to <<func>>.
	If the packet descriptors for linked lists are divergent, then the two
	lists are deemed divergent and their descriptors and headers are passed
	to <<func>>.
    <func> The function to call for each divergence. The prototype function is
    [<DS_PROTO_traverse_func>]. If this returns FALSE, iterations are stopped.
    [RETURNS] TRUE if all iterations completed successfully, else FALSE.
*/
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

    if ( (desc1 == NULL) || (head1 == NULL) ||
        (desc2 == NULL) || (head2 == NULL) )
    {
	a_func_abort (function_name, "NULL pointer(s) passed");
        return (FALSE);
    }
    if (func == NULL)
    {
	(void) fprintf (stderr, "NULL function pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( !ds_compare_packet_desc (desc1, desc2, FALSE) )
    {
	/*  Packet descriptors are different  */
	if (as_whole == TRUE)
	{
	    /*  The packet descriptors are different and  as_whole  is TRUE  */
	    /*  Must process linked lists as a whole  */
	    return ( (*func) ( (char *) desc1, LISTP, (char *) head1,
				  (char *) desc2, LISTP,
				  (char *) head2 ) );
	}
#ifdef dummy
	/*  Must process each packet individually: do it later  */
	if (head2->length < 1)
	{
	    /*  Second list is empty: allocate for it  */
	    if ( !ds_alloc_contiguous_list (desc2, head2,
					    head1->length, TRUE, TRUE) )
	    {
		m_error_notify (function_name, "output list entries");
		return (FALSE);
	    }
	    head2->sort_type = SORT_RANDOM;
	}
#endif
    }
    if (head1->length != head2->length)
    {
	/*  Since the lists are of different lengths, MUST process as whole  */
	return ( (*func) ( (char *) desc1, LISTP, (char *) head1,
			      (char *) desc2, LISTP, (char *) head2 ) );
    }
    /*  The packet descriptors are the same or the  as_whole  flag is FALSE  */
    /*  Iterate through the lists  */
    /*  Process the contiguous entries of the first list  */
    data1 = head1->contiguous_data;
    data2 = head2->contiguous_data;
    pack_size1 = ds_get_packet_size (desc1);
    pack_size2 = ds_get_packet_size (desc2);
    entry2 = head2->first_frag_entry;
    for (count1 = 0, count2 = 0; count1 < head1->contiguous_length;
	 ++count1, data1 += pack_size1)
    {
	if (count2 < head2->contiguous_length)
	{
	    /*  Still in second list's contiguous section  */
	    if ( !ds_traverse_and_process (desc1, data1,
					   desc2, data2,
					   as_whole, func) )
	    {
		return (FALSE);
	    }
	    ++count2;
	    data2 += pack_size2;
	}
	else
	{
	    /*  In second list's fragmented section  */
	    if ( !ds_traverse_and_process (desc1, data1,
					   desc2, entry2->data,
					   as_whole, func) )
	    {
		return (FALSE);
	    }
	    entry2 = entry2->next;
	}
    }
    /*  Process the fragmented entries of the first list  */
    for (entry1 = head1->first_frag_entry; entry1 != NULL;
	 entry1 = entry1->next)
    {
	if (count2 < head2->contiguous_length)
	{
	    /*  Still in second list's contiguous section  */
	    if ( !ds_traverse_and_process (desc1, entry1->data,
					   desc2, data2,
					   as_whole, func) )
	    {
		return (FALSE);
	    }
	    ++count2;
	    data2 += pack_size2;
	}
	else
	{
	    /*  In second list's fragmented section  */
	    if ( !ds_traverse_and_process (desc1, entry1->data,
					   desc2, entry2->data,
					   as_whole, func) )
	    {
		return (FALSE);
	    }
	    entry2 = entry2->next;
	}
    }
    return (TRUE);
}   /*  End Function ds_traverse_list  */
