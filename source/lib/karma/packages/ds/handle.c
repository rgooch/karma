/*LINTLIBRARY*/
/*  handle.c

    This code provides routines for getting handles to parts of data structures

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

    This file contains the various utility routines for obtaining handles to
    instances of a sub-structure (object) in a general data structure.


    Written by      Richard Gooch   21-SEP-1992

    Updated by      Richard Gooch   18-NOV-1992

    Updated by      Richard Gooch   3-JAN-1993: Added support for K_ARRAY

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   17-MAY-1993: Changed from supporting atomic
  data types to supporting named data types in  ds_get_handle_in_packet  .

    Updated by      Richard Gooch   20-JUL-1994: Added some CONST declarations.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ds/handle.c

    Updated by      Richard Gooch   19-APR-1995: Cleaned some code.

    Last updated by Richard Gooch   5-MAY-1995: Placate SGI compiler.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <string.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>

/*PUBLIC_FUNCTION*/
unsigned int ds_get_handle_in_packet (pack_desc, packet, item_name,
				      restr_names, restr_values, num_restr,
				      parent_desc, parent, parent_type, index)
/*  This routine will find a unique occurrence of an object (sub-structure)
    within a specified general data structure.
    The general data structure packet descriptor must be pointed to by
    pack_desc  and the packet must be pointed to by  packet  .
    The name of the object that the handle is desired for must be pointed to by
    item_name  .
    The matched list of restrictions must be pointed to by  restr_names  and
    restr_values  .The length of these lists must be in  num_restr  .
    The routine writes a pointer to the item's parent descriptor on success to
    the storage pointed to by  parent_desc  .
    The routine writes a pointer to the item's parent on success to the
    storage pointed to by  parent  .The following rules apply to items:
        If the item is an atomic element, the parent is a packet.
	If the item is a dimension, the parent is an array.
	If the item is an atomic element in a linked list, the parent is a
	list header.
    The routine writes a value indicating the type of the parent to
    parent_type  . This may be one of the following values:
	NONE          if the item's parent is a packet.
	K_ARRAY       if the item's parent is an array.
	LISTP         if the item's parent is a linked list header.
    The routine writes the index number of the item in its parent descriptor to
    the storage pointed to by  index  .
    The routine returns one of the following values:
        IDENT_NOT_FOUND    if the item was not found.
	IDENT_MULTIPLE     if the restrictions were insufficient
	IDENT_DIMENSION    on success and the item was a dimension
	IDENT_ELEMENT      on success and the item was an atomic element
*/
packet_desc *pack_desc;
char *packet;
CONST char *item_name;
CONST char *restr_names[];
double *restr_values;
unsigned int num_restr;
char **parent_desc;
char **parent;
unsigned int *parent_type;
unsigned int *index;
{
    unsigned int elem_count;
    unsigned int elem_type;
    char *elem_desc;
    extern char host_type_sizes[];
    static char function_name[] = "ds_get_handle_in_packet";

    if ( (pack_desc == NULL) || (packet == NULL) || (item_name == NULL)
	|| (parent_desc == NULL) || (parent == NULL) || (parent_type == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( (num_restr > 0) &&
	( (restr_names == NULL) || (restr_values == NULL) ) )
    {
	(void) fprintf (stderr,
			"NULL restriction pointer(s) passed for: %d restrictions\n",
			num_restr);
	a_prog_bug (function_name);
    }
    for (elem_count = 0; elem_count < (*pack_desc).num_elements; ++elem_count)
    {
	elem_type = (*pack_desc).element_types[elem_count];
	elem_desc = (*pack_desc).element_desc[elem_count];
	if (ds_element_is_named (elem_type) == TRUE)
	{
	    /*  Atomic element: see if item is it  */
	    if (strcmp (item_name, elem_desc) == 0)
	    {
		/*  Found it  */
		*parent_desc = (char *) pack_desc;
		*parent = packet;
		*parent_type = NONE;
		*index = elem_count;
		return (IDENT_ELEMENT);
	    }
	}
	switch (elem_type)
	{
	  case K_ARRAY:
	    /*  See if element is down from here  */
	    switch ( ds_f_name_in_array ( (array_desc *) elem_desc,
					 item_name, (char **) NULL,
					 (unsigned int *) NULL ) )
	    {
	      case IDENT_NOT_FOUND:
		break;
	      case IDENT_DIMENSION:
	      case IDENT_ELEMENT:
		/*  It's down from here: go to it  */
		return ( ds_get_handle_in_array ( (array_desc *)
						 elem_desc,
						 *(char **) packet, item_name,
						 restr_names, restr_values,
						 num_restr, parent_desc,
						 parent, parent_type,
						 index ) );
/*
		break;
*/
	      case IDENT_MULTIPLE:
		(void) fprintf (stderr,
				"Multiple occurrences of: \"%s\" found\n",
				item_name);
		a_prog_bug (function_name);
		break;
	      default:
		(void) fprintf (stderr,
				"Bad return value from function: f_name_in_array\n");
		a_prog_bug (function_name);
		break;
	    }
	    break;
	  case LISTP:
	    /*  See if element is down from here  */
	    switch ( ds_f_name_in_packet ( (packet_desc *) elem_desc,
					  item_name, (char **) NULL,
					  (unsigned int *) NULL ) )
	    {
	      case IDENT_NOT_FOUND:
		break;
	      case IDENT_DIMENSION:
	      case IDENT_ELEMENT:
		/*  It's down from here: go to it  */
		return ( ds_get_handle_in_list ( (packet_desc *)
						elem_desc,
						*(list_header **) packet,
						item_name,
						restr_names, restr_values,
						num_restr, parent_desc, parent,
						parent_type, index ) );
/*
		break;
*/
	      case IDENT_MULTIPLE:
		(void) fprintf (stderr,
				"Multiple occurrences of: \"%s\" found\n",
				item_name);
		a_prog_bug (function_name);
		break;
	      default:
		(void) fprintf (stderr,
				"Bad return value from function: f_name_in_array\n");
		a_prog_bug (function_name);
		break;
	    }
	    break;
	}
	packet += host_type_sizes[elem_type];
    }
    return (IDENT_NOT_FOUND);
}   /*  End Function ds_get_handle_in_packet  */

/*PUBLIC_FUNCTION*/
unsigned int ds_get_handle_in_array (arr_desc, array, item_name,
				     restr_names, restr_values, num_restr,
				     parent_desc, parent, parent_type, index)
/*  This routine will find a unique occurrence of an object (sub-structure)
    within a multi-dimensional array.
    The array descriptor must be pointed to by  arr_desc  and the array must
    be pointed to by  array  .
    The name of the object that the handle is desired for must be pointed to by
    item_name  .
    The matched list of restrictions must be pointed to by  restr_names  and
    restr_values  .The length of these lists must be in  num_restr  .
    The routine writes a pointer to the item's parent descriptor on success to
    the storage pointed to by  parent_desc  .
    The routine writes a pointer to the item's parent on success to the
    storage pointed to by  parent  .The following rules apply to items:
        If the item is an atomic element, the parent is a packet.
	If the item is a dimension, the parent is an array.
	If the item is an atomic element in a linked list, the parent is a
	list header.
    The routine writes a value indicating the type of the parent to
    parent_type  . This may be one of the following values:
	NONE          if the item's parent is a packet.
	K_ARRAY       if the item's parent is an array.
	LISTP         if the item's parent is a linked list header.
    The routine writes the index number of the item in it parent descriptor to
    the storage pointed to by  index  .
    The routine returns one of the following values:
        IDENT_NOT_FOUND    if the item was not found.
	IDENT_MULTIPLE     if the restrictions were insufficient
	IDENT_DIMENSION    on success and the item was a dimension
	IDENT_ELEMENT      on success and the item was an atomic element
*/
array_desc *arr_desc;
char *array;
CONST char *item_name;
CONST char *restr_names[];
double *restr_values;
unsigned int num_restr;
char **parent_desc;
char **parent;
unsigned int *parent_type;
unsigned int *index;
{
    unsigned int dim_count;
    unsigned int restr_count;
    unsigned int packet_size;
    unsigned long *coordinates;
    dim_desc *dim;
    static char function_name[] = "ds_get_handle_in_array";

    if ( (arr_desc == NULL) || (array == NULL) || (item_name == NULL)
	|| (parent_desc == NULL) || (parent == NULL) || (parent_type == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( (num_restr > 0) &&
	( (restr_names == NULL) || (restr_values == NULL) ) )
    {
	(void) fprintf (stderr,
			"NULL restriction pointer(s) passed for: %d restrictions\n",
			num_restr);
	a_prog_bug (function_name);
    }
    /*  Search through dimension names for item name  */
    for (dim_count = 0; dim_count < (*arr_desc).num_dimensions; ++dim_count)
    {
	dim = (*arr_desc).dimensions[dim_count];
	if (strcmp (item_name, (*dim).name) == 0)
	{
	    /*  Found it  */
	    *parent_desc = (char *) arr_desc;
	    *parent = array;
	    *parent_type = K_ARRAY;
	    *index = dim_count;
	    return (IDENT_DIMENSION);
	}
    }
    /*  Item may be down from array packet descriptor: need to check
	restrictions to pinpoint packet in multi-dimensional space  */
    packet_size = ds_get_packet_size ( (*arr_desc).packet );
    /*  Allocate co-ordinate list  */
    if ( ( coordinates = (unsigned long *)
	  m_alloc ( (*arr_desc).num_dimensions * sizeof (*coordinates) ) )
	== NULL )
    {
	m_abort (function_name, "co-ordinate array");
    }
    /*  Search through dimension names for restrictions  */
    for (dim_count = 0; dim_count < (*arr_desc).num_dimensions; ++dim_count)
    {
	dim = (*arr_desc).dimensions[dim_count];
	coordinates[dim_count] = (*dim).length;
	for (restr_count = 0; restr_count < num_restr; ++restr_count)
	{
	    if (strcmp (restr_names[restr_count], (*dim).name) != 0)
	    {
		/*  Not a restriction for this dimension  */
		continue;
	    }
	    /*  Found a restriction for this dimension  */
	    if (coordinates[dim_count] < (*dim).length)
	    {
		/*  Already have a restriction:
		    print warning but keep going  */
		(void) fprintf (stderr,
				"Multiple restrictions for dimension: \"%s\"  using last value: %g\n",
				(*dim).name, restr_values[restr_count]);
	    }
	    coordinates[dim_count] =ds_get_coord_num(dim,
						     restr_values[restr_count],
						     SEARCH_BIAS_CLOSEST);
	    
	    if (ds_get_coordinate (dim, coordinates[dim_count]) !=
		restr_values[restr_count])
	    {
		/*  Not exactly the same value: print a warning  */
		(void) fprintf (stderr,
				"Nearest co-ordinate: %g for dimension: \"%s\"\n",
				ds_get_coordinate (dim,
						   coordinates[dim_count]),
				(*dim).name);
	    }
	}
	if (coordinates[dim_count] >= (*dim).length)
	{
	    /*  No restriction found for this dimension  */
	    /*  Deallocate co-ordinate list  */
	    m_free ( (char *) coordinates );
	    (void) fprintf (stderr,
			    "No restrictions found for dimension: \"%s\"\n",
			    (*dim).name);
	    return (IDENT_MULTIPLE);
	}
    }
    /*  Index into multi-dimensional space  */
    array += packet_size * ds_get_array_offset (arr_desc, coordinates);
    /*  Deallocate co-ordinate list  */
    m_free ( (char *) coordinates );
    /*  Delve into packet  */
    return ( ds_get_handle_in_packet ( (*arr_desc).packet, array, item_name,
				      restr_names, restr_values, num_restr,
				      parent_desc, parent, parent_type,
				      index ) );
}   /*  End Function ds_get_handle_in_array  */

/*PUBLIC_FUNCTION*/
unsigned int ds_get_handle_in_list (list_desc, list_head, item_name,
				    restr_names, restr_values, num_restr,
				    parent_desc, parent, parent_type, index)
/*  This routine will find a unique occurrence of an object (sub-structure)
    within a linked list.
    The list descriptor must be pointed to by  list_desc  and the list header
    must be pointed to by  list_head  .
    The name of the object that the handle is desired for must be pointed to by
    item_name  .
    The matched list of restrictions must be pointed to by  restr_names  and
    restr_values  .The length of these lists must be in  num_restr  .
    The routine writes a pointer to the item's parent descriptor on success to
    the storage pointed to by  parent_desc  .
    The routine writes a pointer to the item's parent on success to the
    storage pointed to by  parent  .The following rules apply to items:
        If the item is an atomic element, the parent is a packet.
	If the item is a dimension, the parent is an array.
	If the item is an atomic element in a linked list, the parent is a
	list header.
    The routine writes a value indicating the type of the parent to
    parent_type  . This may be one of the following values:
	NONE          if the item's parent is a packet.
	K_ARRAY       if the item's parent is an array.
	LISTP         if the item's parent is a linked list header.
    The routine writes the index number of the item in it parent descriptor to
    the storage pointed to by  index  .
    The routine returns one of the following values:
        IDENT_NOT_FOUND    if the item was not found.
	IDENT_MULTIPLE     if the restrictions were insufficient
	IDENT_DIMENSION    on success and the item was a dimension
	IDENT_ELEMENT      on success and the item was an atomic element
*/
packet_desc *list_desc;
list_header *list_head;
CONST char *item_name;
CONST char *restr_names[];
double *restr_values;
unsigned int num_restr;
char **parent_desc;
char **parent;
unsigned int *parent_type;
unsigned int *index;
{
    unsigned int elem_count;
#ifdef dummy
    unsigned int elem_type;
#endif
    char *elem_desc;
    static char function_name[] = "ds_get_handle_in_list";

    if ( (list_desc == NULL) || (list_head == NULL) || (item_name == NULL)
	|| (parent_desc == NULL) || (parent == NULL) || (parent_type == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( (num_restr > 0) &&
	( (restr_names == NULL) || (restr_values == NULL) ) )
    {
	(void) fprintf (stderr,
			"NULL restriction pointer(s) passed for: %d restrictions\n",
			num_restr);
	a_prog_bug (function_name);
    }
    /*  Search through element names for item name  */
    for (elem_count = 0; elem_count < (*list_desc).num_elements; ++elem_count)
    {
	elem_desc = (*list_desc).element_desc[elem_count];
#ifdef dummy
	elem_type = (*list_desc).element_types[elem_count];
#endif
	if (strcmp (item_name, elem_desc) == 0)
	{
	    /*  Found it  */
	    *parent_desc = (char *) list_desc;
	    *parent = (char *) list_head;
	    *parent_type = LISTP;
	    *index = elem_count;
	    return (IDENT_ELEMENT);
	}
    }
#ifdef dummy
    /*  Item may be down from list descriptor: need to check
	restrictions to pinpoint packet in linked list  */
    /*  Search through element names for restrictions  */
    for (elem_count = 0; elem_count < (*list_desc).num_elements; ++elem_count)
    {
	elem_desc = (*list_desc).element_desc[elem_count];
	elem_type = (*list_desc).element_type[elem_count];
	best_match = TOOBIG;
	for (restr_count = 0; restr_count < num_restr; ++restr_count)
	{
	    if (strcmp (restr_names[restr_count], elem_desc) != 0)
	    {
		/*  Not a restriction for this element  */
		continue;
	    }
	    /*  Found a restriction for this element  */
	    if (best_match < TOOBIG)
	    {
		/*  Already have a restriction:
		    print warning but keep going  */
		(void) fprintf (stderr,
				"Multiple restrictions for element: \"%s\"  using last value: %g\n",
				elem_desc, restr_values[restr_count]);
	    }
	    /*  Search though list to find best match  */
	    if (1 == 1)
	    {
		/*  Not exactly the same value: print a warning  */
		(void) fprintf (stderr,
				"Nearest co-ordinate: %g for dimension: \"%s\"\n",
				ds_get_coordinate (dim,
						   coordinates[dim_count]),
				(*dim).name);
	    }
	}
	if (coordinates[dim_count] >= (*dim).length)
	{
	    /*  No restriction found for this dimension  */
	    (void) fprintf (stderr,
			    "No restrictions found for dimension: \"%s\"\n",
			    (*dim).name);
	    return (IDENT_MULTIPLE);
	}
    }
    /*  Index into multi-dimensional space  */
    array += packet_size * ds_get_array_offset (arr_desc, coordinates);
    /*  Delve into packet  */
    return ( ds_get_handle_in_packet ( (*arr_desc).packet, array, item_name,
				      restr_names, restr_values, num_restr,
				      parent_desc, parent, parent_type,
				      index ) );
#endif
    (void) fprintf (stderr, "%s not finished yet\n", function_name);
    return (IDENT_MULTIPLE);
}   /*  End Function ds_get_handle_in_list  */
