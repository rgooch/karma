/*LINTLIBRARY*/
/*  cmap.c

    This code provides colourmap manipulation for Karma data structures.

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

    This file contains various Karma utility routines which create and find
  colourmaps in the Karma general data structure.


    Written by      Richard Gooch   20-SEP-1992

    Updated by      Richard Gooch   9-DEC-1992

    Updated by      Richard Gooch   3-JAN-1993: Took acccount of change to
  ds_alloc_data

    Updated by      Richard Gooch   2-MAR-1993: Fixed bug in
  ds_cmap_alloc_colourmap  .Bottom tile lengths not set.

    Updated by      Richard Gooch   13-APR-1993: Moved to  ds_cmap_

    Updated by      Richard Gooch   21-NOV-1993: Fixed bug in ordering of
  colours (was red, blue, green): incompatible with  kcmap_  package.

    Updated by      Richard Gooch   30-NOV-1993: Fixed bug in reordering of
  colours: could end up in infinite loop.

    Updated by      Richard Gooch   27-JAN-1994: Fixed error in documentation
  regarding ordering of colours in  ds_cmap_find_colourmap  .

    Updated by      Richard Gooch   20-JUL-1994: Added some CONST declarations.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ds/cmap.c

    Last updated by Richard Gooch   19-APR-1995: Cleaned some code.


*/
#include <stdio.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>


#define NUM_ELEMENTS (unsigned int) 3

static char *element_names[NUM_ELEMENTS] = 
{
    "Red Intensity",
    "Green Intensity",
    "Blue Intensity"
};

/*PUBLIC_FUNCTION*/
unsigned short *ds_cmap_alloc_colourmap (size, multi_desc, pack_desc, packet)
/*  This routine will allocate a colourmap which comforms to the Karma general
    data structure format. Note that this does NOT allocate a colourmap on an
    X Windows server. This routine is intended to enable an application to
    write colourmap data to a file, maintaining a flexible format.
    The size (number of colours) of the colourmap to allocate must be given by
    size  .
    The pointer to the multi array header which is allocated will be written
    to the storage pointed to by  multi_desc  .If this is NULL, then no multi
    array header is allocated and nothing is written here.
    The pointer to the top level packet descriptor of the general data
    structure which contains the colourmap will be written to the storage
    pointed to by  pack_desc  .If this is NULL, nothing is written here. Note,
    however, that the top level packet descriptor is nevertheless allocated.
    The pointer to the top level packet of the general data structure which
    contains the colourmap will be written to the storage pointed to by  
    packet  .If this is NULL, nothing is written here. Note, however, that the
    top level packet is nevertheless allocated.
    The general data structure that is created will contain a 1 dimensional
    array with dimension name "Colour Number" and length equal to  size  .
    The array will contain packets with 3 elements, each of type  K_USHORT  and
    with the names "Red Intensity", "Green Intensity" and "Blue Intensity", in
    that order. For compatibility with the X Window system, it is recommended
    that these values lie in the range 0 to 65535.
    The routine returns a pointer to the allocated colourmap on success,
    else it returns NULL.
*/
unsigned int size;
multi_array **multi_desc;
packet_desc **pack_desc;
char **packet;
{
    unsigned int element_count;
    char *top_packet;
    packet_desc *top_pack_desc;
    array_desc *arr_desc;
    extern char *element_names[NUM_ELEMENTS];
    static char function_name[] = "ds_cmap_alloc_colourmap";

    if (size < 1)
    {
	return (NULL);
    }
    if ( ( top_pack_desc = ds_alloc_packet_desc (1) ) == NULL )
    {
	m_error_notify (function_name, "top level packet descriptor");
	return (NULL);
    }
    if ( ( arr_desc = ds_alloc_array_desc (1, 0) ) == NULL )
    {
	ds_dealloc_packet (top_pack_desc, (char *) NULL);
	m_error_notify (function_name, "array descriptor");
	return (NULL);
    }
    (*top_pack_desc).element_desc[0] = (char *) arr_desc;
    (*top_pack_desc).element_types[0] = K_ARRAY;
    (*arr_desc).lengths[0] = size;
    if ( ( (*arr_desc).dimensions[0] =
	  ds_alloc_dim_desc ("Colour Number", size, 0.0, (double) (size - 1),
			     TRUE) ) == NULL )
    {
	ds_dealloc_packet (top_pack_desc, (char *) NULL);
	m_error_notify (function_name, "dimension descriptor");
	return (NULL);
    }
    if ( ( (*arr_desc).packet = ds_alloc_packet_desc (3) ) == NULL )
    {
	ds_dealloc_packet (top_pack_desc, (char *) NULL);
	m_error_notify (function_name, "array packet descriptor");
	return (NULL);
    }
    /*  Fill in element names  */
    for (element_count = 0; element_count < NUM_ELEMENTS; ++element_count)
    {
	if ( ( (* (*arr_desc).packet ).element_desc[element_count] =
	      m_alloc (strlen (element_names[element_count]) + 1) ) == NULL )
	{
	    ds_dealloc_packet (top_pack_desc, (char *) NULL);
	    m_error_notify (function_name, "element name");
	    return (NULL);
	}
	(void) strcpy ( (* (*arr_desc).packet ).element_desc[element_count],
		       element_names[element_count] );
	(* (*arr_desc).packet ).element_types[element_count] = K_USHORT;
    }
    /*  Descriptors allocated: allocate data  */
    if ( ( top_packet = ds_alloc_data (top_pack_desc, TRUE, TRUE) ) == NULL )
    {
	ds_dealloc_packet (top_pack_desc, (char *) NULL);
	m_error_notify (function_name, "data");
	return (NULL);
    }
    if (multi_desc != NULL)
    {
	/*  Allocate multi array structure  */
	if ( ( *multi_desc = ds_alloc_multi (1) ) == NULL )
	{
	    ds_dealloc_packet (top_pack_desc, top_packet);
	    m_error_notify (function_name, "multi_desc structure");
	    return (NULL);
	}
	(**multi_desc).headers[0] = top_pack_desc;
	(**multi_desc).data[0] = top_packet;
    }
    if (pack_desc != NULL)
    {
	*pack_desc = top_pack_desc;
    }
    if (packet != NULL)
    {
	*packet = top_packet;
    }
    return (*(unsigned short **) top_packet);
}   /*  End Function ds_cmap_alloc_colourmap  */

/*PUBLIC_FUNCTION*/
unsigned short *ds_cmap_find_colourmap (top_pack_desc, top_packet, size,
					reordering_done,
					restr_names, restr_values, num_restr)
/*  This routine will search a Karma general data structure for an instance of
    a colourmap.
    The top level packet descriptor must be pointed to by  top_pack_desc  .
    The top level packet data must be pointed to by  top_packet  .
    The size of the colourmap (number of colours) in the colourmap will be
    written to the storage pointed to by  size  .
    The routine is tolerant of incorrect ordering of the intensity elements.
    If they are ordered incorrectly, the data will be re-ordered, and the
    value TRUE will be written to the storage pointed to by  reordering_done  .
    If the data is not re-ordered, the value FALSE is written here. If this is
    NULL, then nothing is written here.
    The matched list of restrictions must be pointed to by  restr_names  and
    restr_values  .The length of these lists must be in  num_restr  .Note that
    NULL pointers may be passed for  restr_names  and restr_values  and
    num_restr  set to zero. In this case, the routine will only return a
    colourmap if there is only one instance of a colourmap in the entire data
    structure.
    The routine returns a pointer to a colourmap on success,
    else it returns NULL.
    Note that the returned colourmap will contain  *size  contiguous packets
    which each contain first a red intensity, then a green and then a blue.
*/
packet_desc *top_pack_desc;
char *top_packet;
unsigned int *size;
flag *reordering_done;
CONST char *restr_names[];
double *restr_values;
unsigned int num_restr;
{
    flag reorder_needed = FALSE;
    unsigned int parent_type;
    unsigned int index;
    unsigned int elem_count;
    unsigned int name_count;
    unsigned int packet_count;
    char *parent_desc;
    char *parent;
    char **names;
    unsigned short *packet;
    unsigned int name_indices[NUM_ELEMENTS];
    unsigned short tmp_packet[NUM_ELEMENTS];
    char *tmp_names[NUM_ELEMENTS];
    array_desc *arr_desc;
    packet_desc *pack_desc;
    extern char *element_names[NUM_ELEMENTS];
    static char function_name[] = "ds_cmap_find_colourmap";

    /*  Try to get handle to a unique instance of colourmap array  */
    switch ( ds_get_handle_in_packet (top_pack_desc, top_packet,
				      "Colour Number",
				      restr_names, restr_values, num_restr,
				      &parent_desc, &parent, &parent_type,
				      &index) )
    {
      case IDENT_NOT_FOUND:
	/*  Not found at all  */
	return (NULL);
/*
	break;
*/
      case IDENT_MULTIPLE:
	/*  Not enough restrictions  */
	return (NULL);
/*
	break;
*/
      case IDENT_DIMENSION:
	/*  Got it  */
	break;
      case IDENT_ELEMENT:
	(void) fprintf (stderr,
			"Item: \"Colour Number\" is an atomic element\n");
	return (NULL);
/*
	break;
*/
      default:
	(void) fprintf (stderr,
			"Illegal return value from function: ds_get_handle_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    if (parent_type != K_ARRAY)
    {
	(void) fprintf (stderr,
			"Found dimension but parent is not an array\n");
	a_prog_bug (function_name);
    }
    arr_desc = (array_desc *) parent_desc;
    /*  Check the array  */
    if ( (*arr_desc).num_dimensions != 1 )
    {
	(void) fprintf (stderr,
			"Array with dimension: \"Colour Number\" must have only one dimension\n");
	return (NULL);
    }
    pack_desc = (*arr_desc).packet;
    /*  Check the packet descriptor  */
    if ( (*pack_desc).num_elements != 3 )
    {
	(void) fprintf (stderr,
			"Colourmap array must have packets with only 3 elements\n");
	return (NULL);
    }
    if (sizeof (unsigned short) < 2)
    {
	(void) fprintf ( stderr,
			"Unsigned short size: %u is less than 2 bytes\n",
			sizeof (unsigned short) );
	a_prog_bug (function_name);
    }
    /*  Check each element in packet  */
    for (elem_count = 0; elem_count < NUM_ELEMENTS; ++elem_count)
    {
	if (K_USHORT != (*pack_desc).element_types[elem_count])
	{
	    (void) fprintf (stderr,
			    "Intensity elements must all be of type K_USHORT\n");
	    return (NULL);
	}
	/*  Search for name  */
	name_indices[elem_count] = NUM_ELEMENTS;
	for (name_count = 0; name_count < NUM_ELEMENTS; ++name_count)
	{
	    if (strcmp (element_names[name_count],
			(*pack_desc).element_desc[elem_count]) == 0)
	    {
		name_indices[elem_count] = name_count;
		if (name_count != elem_count)
		{
		    reorder_needed = TRUE;
		}
	    }
	}
	if (name_indices[elem_count] >= NUM_ELEMENTS)
	{
	    /*  No name found  */
	    (void) fprintf (stderr,
			    "Intensity element: \"%s\" does not have a recognised name\n",
			    (*pack_desc).element_desc[elem_count]);
	    return (NULL);
	}
	/*  Check to see if elements have the same name  */
	for (name_count = 0; name_count < elem_count; ++name_count)
	{
	    if (name_indices[name_count] == name_indices[elem_count])
	    {
		(void) fprintf (stderr,
				"Multiple occurrences of name: \"%s\"\n",
				(*pack_desc).element_desc[elem_count]);
		return (NULL);
	    }
	}
    }
    if (reordering_done != NULL)
    {
	*reordering_done = reorder_needed;
    }
    if (!reorder_needed)
    {
	*size = (* (*arr_desc).dimensions[0] ).length;
	return ( (unsigned short *) parent );
    }
    /*  Need to re-order the data  */
    for (packet_count = 0, packet = (unsigned short *) parent;
	 packet_count < (* (*arr_desc).dimensions[0] ).length;
	 ++packet_count, packet += NUM_ELEMENTS)
    {
	/*  Reorder one colour  */
	for (elem_count = 0; elem_count < NUM_ELEMENTS; ++elem_count)
	{
	    tmp_packet[ name_indices[elem_count] ] = packet[elem_count];
	}
	m_copy ( (char *) packet, (char *) tmp_packet,
		sizeof *packet * NUM_ELEMENTS );
    }
    /*  Reorder names  */
    names = (*pack_desc).element_desc;
    for (elem_count = 0; elem_count < NUM_ELEMENTS; ++elem_count)
    {
	tmp_names[ name_indices[elem_count] ] = names[elem_count];
    }
    m_copy ( (char *)names, (char *) tmp_names, sizeof *names * NUM_ELEMENTS );
    *size = (* (*arr_desc).dimensions[0] ).length;
    return ( (unsigned short *) parent );
}   /*  End Function ds_cmap_find_colourmap  */

/*PUBLIC_FUNCTION*/
unsigned int *ds_cmap_get_all_colourmaps (multi_desc, num_found,
					  reordering_done,
					  restr_names, restr_values, num_restr)
/*  This routine will search an array of Karma general data structures for
    instances of a colourmap.
    The multi_desc header must be pointed to by  multi_desc  .
    The number of general data structures which contain a colourmap will be
    written to the storage pointed to by  num_found  .
    The routine is tolerant of incorrect ordering of the intensity elements.
    If they are ordered incorrectly, the data will be re-ordered, and the
    value TRUE will be written to the storage pointed to by  reordering_done  ,
    else the value of FALSE will be written here.
    If this is NULL, nothing is written here.
    The matched list of restrictions must be pointed to by  restr_names  and
    restr_values  .The length of these lists must be in  num_restr  .Note that
    NULL pointers may be passed for  restr_names  and restr_values  and
    num_restr  set to zero. In this case, the routine will only return a
    colourmap if there is only one instance of a colourmap in the entire data
    structure.
    The routine returns a pointer to an array of general data structure
    indices on success, else it returns NULL. This array is dynamically
    allocated, and should be freed when no longer needed.
*/
multi_array *multi_desc;
unsigned int *num_found;
flag *reordering_done;
CONST char *restr_names[];
double *restr_values;
unsigned int num_restr;
{
    unsigned int array_count;
    unsigned int size;
    unsigned int *array_indices;
    static char function_name[] = "ds_cmap_get_all_colourmaps";

    if ( ( array_indices = (unsigned int *)
	  m_alloc (sizeof *array_indices * (*multi_desc).num_arrays) )
	== NULL )
    {
	m_error_notify (function_name, "array of indices");
	return (NULL);
    }
    if (reordering_done != NULL)
    {
	*reordering_done = FALSE;
    }
    *num_found = 0;
    for (array_count = 0; array_count < (*multi_desc).num_arrays;
	 ++array_count)
    {
	if (ds_cmap_find_colourmap ( (*multi_desc).headers[array_count],
				    (*multi_desc).data[array_count], &size,
				    reordering_done,
				    restr_names, restr_values, num_restr )
	    != NULL )
	{
	    /*  Found one  */
	    array_indices[*num_found] = array_count;
	    ++(*num_found);
	}
    }
    if (*num_found < 1)
    {
	m_free ( (char *) array_indices );
	array_indices = NULL;
    }
    return (array_indices);
}   /*  End Function ds_cmap_get_all_colourmaps  */
