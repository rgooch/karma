/*LINTLIBRARY*/
/*  find.c

    This code provides routines to find things in a data structure.

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

    This file contains the various utility routines for finding things in the
    general data structure supported in Karma.


    Written by      Richard Gooch   1-NOV-1996: Extracted from get.c

    Last updated by Richard Gooch   1-NOV-1996


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>
#include <os.h>


/*PUBLIC_FUNCTION*/
unsigned int ds_identify_name (CONST multi_array *multi_desc, CONST char *name,
			       char **encls_desc, unsigned int *index)
/*  [SUMMARY] Search a data structure for a name.
    <multi_desc> The multi_array descriptor.
    <name> The name of the item to search for.
    <encls_desc> A pointer to the enclosing descriptor of the item is written
    here. If this is NULL, nothing is written here.
    <index> The index (general data structure number, dimension number or
    element number) of the item in the enclosing structure will be written
    here. If this is NULL, nothing is written here.
    [RETURNS] A code based on the type of the item with the same name. See
    [<DS_IDENT_TABLE>] for a list of possible values.
*/
{
    unsigned int array_count = 0;
    unsigned int temp_ident;
    unsigned int return_value = IDENT_NOT_FOUND;
    static char function_name[] = "ds_identify_name";

    if (multi_desc == NULL)
    {
	return (IDENT_NOT_FOUND);
    }
    if (multi_desc->headers == NULL)
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
    while (array_count < multi_desc->num_arrays)
    {
	/*  Search one array    */
        if ( ( temp_ident =
	      ds_f_name_in_packet (multi_desc->headers[array_count],
				   name, encls_desc, index) )
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
unsigned int ds_f_array_name (CONST multi_array *multi_desc, CONST char *name,
			      char **encls_desc, unsigned int *index)
/*  [SUMMARY] Search a the top level of a data structure for a name.
    [PURPOSE] This routine will search a multi array general data structure
    header for an occurrence of an array name.
    <multi_desc> The multi_array data structure.
    <name> The array name.
    <encls_desc> If the array name is found, the pointer to the multi array
    header will be written here. If this is NULL, nothing is written here.
    <index> The index number of the general data structure with will be written
    here. If this is NULL, nothing is written here.
    [NOTE] The routine will not search the packet descriptors for name matches.
    [RETURNS] A code based on the number of matches found. See
    [<DS_IDENT_TABLE>] for a list of possible values.
*/
{
    unsigned int array_count = 0;
    unsigned int return_value = IDENT_NOT_FOUND;
    static char function_name[] = "ds_f_array_name";

    if (multi_desc == NULL)
    {
	return (IDENT_NOT_FOUND);
    }
    if (multi_desc->headers == NULL)
    {
	(void) fprintf (stderr,
			"Multi array descriptor has no list of packet descriptors\n");
        a_prog_bug (function_name);
    }
    if (multi_desc->num_arrays == 1)
    {
	/*  Only one array  */
        if (multi_desc->array_names != NULL)
        {
	    /*  Should be a NULL pointer    */
            (void) fprintf (stderr,
			    "Multi array descriptor has one array with name: \"%s\"\n",
			    multi_desc->array_names[0]);
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
    if (multi_desc->array_names == NULL)
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
    while (array_count < multi_desc->num_arrays)
    {
	if (multi_desc->array_names[array_count] == NULL)
        {
	    (void) fprintf (stderr, "Multiple arrays without names given\n");
            a_prog_bug (function_name);
        }
        /*  Compare  name  with name of an array    */
        if (strcmp (name, multi_desc->array_names[array_count]) == 0)
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
unsigned int ds_f_name_in_packet (CONST packet_desc *pack_desc,
				  CONST char *name,
				  char **encls_desc, unsigned int *index)
/*  [SUMMARY] Recursively search for named item under a packet.
    <pack_desc> The packet descriptor.
    <name> The name of the item to search form. If NULL, then the routine will
    not find anything.
    <encls_desc> The pointer to the enclosing structure of the named item will
    be written here. If this is NULL, nothing is written here.
    <index> The index (dimension number or element number) of the item will be
    written here. If this is NULL, nothing is written here.
    [RETURNS] A code based on the type of the item with the same name. See
    [<DS_IDENT_TABLE>] for a list of possible values.
*/
{
    unsigned int elem_count;
    unsigned int temp_ident;
    unsigned int return_value = IDENT_NOT_FOUND;
    static char function_name[] = "ds_f_name_in_packet";

    if ( (pack_desc == NULL) || (pack_desc->num_elements < 1)
	|| (name == NULL) )
    {
	return (IDENT_NOT_FOUND);
    }
    for (elem_count = 0; elem_count < pack_desc->num_elements; ++elem_count)
    {
	if ( ds_element_is_named (pack_desc->element_types[elem_count]) )
        {
	    /*  Atomic data type    */
	    if (strcmp (name, pack_desc->element_desc[elem_count])
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
	switch (pack_desc->element_types[elem_count])
	{
	  case K_ARRAY:
	    if ( ( temp_ident =
		  ds_f_name_in_array ( (array_desc *)
				      pack_desc->element_desc[elem_count],
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
				       pack_desc->element_desc[elem_count],
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
			    pack_desc->element_types[elem_count]);
	    a_prog_bug (function_name);
	    break;
	}
    }
    return (return_value);
}   /*  End Function ds_f_name_in_packet  */

/*PUBLIC_FUNCTION*/
unsigned int ds_f_name_in_array (CONST array_desc *arr_desc, CONST char *name,
				 char **encls_desc, unsigned int *index)
/*  [SUMMARY] Recursively search for named item under an array.
    [PURPOSE] This routine will search an array descriptor for occurrences of a
    named item. The routine searches both the dimension names and the packet
    associated with the array. The routine recursively searches the array
    packet descriptor.
    <arr_desc> The array descriptor.
    <name> The name of the item to search form. If NULL, then the routine will
    not find anything.
    <encls_desc> The pointer to the enclosing structure of the named item will
    be written here. If this is NULL, nothing is written here.
    <index> The index (dimension number or element number) of the item will be
    written here. If this is NULL, nothing is written here.
    [RETURNS] A code based on the type of the item with the same name. See
    [<DS_IDENT_TABLE>] for a list of possible values.
*/
{
    unsigned int dim_count = 0;
    unsigned int temp_ident;
    unsigned int return_value = IDENT_NOT_FOUND;

    if ( (arr_desc == NULL) || (name == NULL) )
    {
	return (IDENT_NOT_FOUND);
    }
    while (dim_count < arr_desc->num_dimensions)
    {
	if (strcmp (name, arr_desc->dimensions[dim_count]->name) == 0)
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
    if (arr_desc->packet == NULL)
    {
	return (return_value);
    }
    if ( ( temp_ident = ds_f_name_in_packet (arr_desc->packet, name,
					  encls_desc, index) )
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
unsigned int ds_f_elem_in_packet (CONST packet_desc *pack_desc,
				  CONST char *name)
/*  [SUMMARY] Search for a named element in a packet, without recursion.
    <pack_desc> The packet descriptor.
    <name> The element name to search for. If this is NULL, then the routine
    will not find anything.
    [NOTE] If the specified name occurs twice, the program aborts.
    [RETURNS] The number of the element in the packet if it was found, else the
    number of elements in the packet.
*/
{
    unsigned int elem_count = 0;
    unsigned int return_value;
    static char function_name[] = "ds_f_elem_in_packet";

    if (pack_desc == NULL)
    {
	(void) fprintf (stderr, "NULL  pack_desc  pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( ( pack_desc->num_elements < 1) || (name == NULL) )
    {
	return (pack_desc->num_elements);
    }
    return_value = pack_desc->num_elements;
    while (elem_count < pack_desc->num_elements)
    {
	if ( ds_element_is_named (pack_desc->element_types[elem_count]) )
        {
	    /*  Atomic data type    */
            if (strcmp (name, pack_desc->element_desc[elem_count]) == 0)
            {
		/*  Found atomic element name   */
                if (return_value < pack_desc->num_elements)
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
unsigned int ds_find_hole (CONST packet_desc *inp_desc, packet_desc **out_desc,
			   unsigned int *elem_num)
/*  [SUMMARY] Recursively search packet for a hole.
    [PURPOSE] This routine will recursively search a packet descriptor for a
    hole (element type NONE or element descriptor pointer NULL).
    <inp_desc> The packet descriptor to search.
    <out_desc> A pointer to the packet descriptor which contains the hole is
    written here. If this is NULL, nothing is written here.
    <elem_num> The element number in that packet which corresponds to the hole
    is written here. If this is NULL, nothing is written here.
    [RETURNS] A code indicating the status of the search. See
    [<DS_IDENT_TABLE>] for a list of possible values.
*/
{
    unsigned int elem_count = 0;
    unsigned int temp_ident;
    unsigned int return_value = IDENT_NOT_FOUND;
    packet_desc *elem_desc;
    array_desc *arr_desc;

    if (inp_desc == NULL)
    {
	return (IDENT_NOT_FOUND);
    }
    if (inp_desc->num_elements < 1)
    {
	return (IDENT_NOT_FOUND);
    }
    /*  Search for occurence of hole in this packet descriptor  */
    while (elem_count < inp_desc->num_elements)
    {
	elem_desc =(packet_desc *) inp_desc->element_desc[elem_count];
        if ( (inp_desc->element_types[elem_count] == NONE) ||
	    (elem_desc == NULL) )
        {
	    if (return_value != IDENT_NOT_FOUND)
            {
		return (IDENT_MULTIPLE);
            }
            return_value = IDENT_ELEMENT;
            *out_desc = (packet_desc *) inp_desc;
            *elem_num = elem_count;
        }
        if (inp_desc->element_types[elem_count] == K_ARRAY)
        {
	    arr_desc = (array_desc *) elem_desc;
            if ( ( temp_ident = ds_find_hole (arr_desc->packet, out_desc,
					      elem_num) ) != IDENT_NOT_FOUND )
            {
		if (return_value != IDENT_NOT_FOUND)
                {
		    return (IDENT_MULTIPLE);
                }
                return_value = temp_ident;
            }
        }
        if (inp_desc->element_types[elem_count] == LISTP)
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
unsigned int ds_f_dim_in_array (CONST array_desc *arr_desc, CONST char *name)
/*  [SUMMARY] Find dimension in array.
    <arr_desc> The array descriptor.
    <name> The name of the dimension to find. If this is NULL, then the routine
    will not find anything.
    [NOTE] If the specified name occurs twice, the program aborts.
    [RETURNS] The number of the dimension in the array if it was found, else
    the number of dimensions in the array.
*/
{
    unsigned int dim_count;
    unsigned int return_value;
    static char function_name[] = "ds_f_dim_in_array";

    if (arr_desc == NULL)
    {
	(void) fprintf (stderr, "NULL  arr_desc  pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (arr_desc->num_dimensions < 1) || (name == NULL) )
    {
	return (arr_desc->num_dimensions);
    }
    return_value = arr_desc->num_dimensions;
    for (dim_count = 0; dim_count < arr_desc->num_dimensions; ++dim_count)
    {
	if (strcmp (name, arr_desc->dimensions[dim_count]->name) == 0)
	{
	    /*  Found dimension name   */
	    if (return_value < arr_desc->num_dimensions)
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
