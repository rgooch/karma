/*LINTLIBRARY*/
/*  misc.c

    This code provides routines to read ASCII data structures from Channels.

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

    This file contains the various utility routines for reading the general
    data structure supported in Karma from files in an ASCII format.


    Written by      Richard Gooch   24-MAR-1993: Copied from  kgen.c  in
  source/modules/convert

    Updated by      Richard Gooch   24-MAR-1993: Changed to use of channels.

    Updated by      Richard Gooch   25-MAR-1993: Took account of changes to
  list_header  structure.

    Updated by      Richard Gooch   28-MAR-1993: Added support for tiling
  specification with  K_ARRAY  .

    Updated by      Richard Gooch   17-MAY-1993: Added support for string data
  types.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/dsra/misc.c

    Updated by      Richard Gooch   19-APR-1995: Cleaned some code.

    Updated by      Richard Gooch   10-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   3-JUN-1996: Took account of new fields in
  dimension descriptor for first and last co-ordinate.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_dsra.h>
#include <karma_chs.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>
#include <os.h>


#define ARRAYP        6    /*  Backwards compatibility  */


/*PUBLIC_FUNCTION*/
multi_array *dsra_multi_desc (Channel channel)
/*  [SUMMARY] Read ASCII representation of a multi_array data structure.
    [PURPOSE] This routine will read the ASCII representation of a multi_array
    data structure descriptor from a channel object.
    [NOTE] The routine will NOT allocate space for data, nor will it read any
    data.
    <channel> The channel object to read from.
    [RETURNS] A pointer to the multi array header on success, else NULL.
*/
{
    unsigned int num_arrays;
    unsigned int array_count;
    char temp_line[STRING_LENGTH];
    multi_array *multi_desc;
    packet_desc *pack_desc;
    static char function_name[] = "dsra_multi_desc";

    /*  Read multi array descriptor */
    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	(void) fprintf (stderr, "\"MULTI_ARRAY\" not found\n");
        return (NULL);
    }
    if (st_icmp (temp_line, "MULTI_ARRAY") != 0)
    {
	(void) fprintf (stderr, "\"MULTI_ARRAY\" not found\n");
        return (NULL);
    }
    if (dsra_uint (channel, &num_arrays) == FALSE)
    {
	(void) fprintf (stderr, "Number of arrays not found\n");
        return (NULL);
    }
    if ( ( multi_desc = ds_alloc_multi (num_arrays) ) == NULL )
    {
	(void) fprintf (stderr,
			"Error allocating memory for multi array header\n");
        return (NULL);
    }
    for (array_count = 0; (array_count < num_arrays) && (num_arrays > 1);
	 ++array_count)
    {
	/*  Load the names of the arrays  */
        if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
        {
	    (void) fprintf (stderr, "Array name not found\n");
            ds_dealloc_multi (multi_desc);
            return (NULL);
        }
        if ( ( multi_desc->array_names[array_count] =
	      m_alloc ( (int) strlen (temp_line) + 1 ) ) == NULL )
        {
	    m_error_notify (function_name, "array name");
            ds_dealloc_multi (multi_desc);
            return (NULL);
        }
        (void) strcpy (multi_desc->array_names[array_count], temp_line );
    }
    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	(void) fprintf (stderr, "\"END\" not found\n");
        ds_dealloc_multi (multi_desc);
        return (NULL);
    }
    if (st_icmp (temp_line, "END") != 0)
    {
	(void) fprintf (stderr, "\"END\" not found\n");
        ds_dealloc_multi (multi_desc);
        return (NULL);
    }
    /*  Load the top level packet descriptors  */
    array_count = 0;
    while (array_count < num_arrays)
    {
	/*  Load the descriptor for one array  */
        if ( ( pack_desc = dsra_packet_desc (channel) ) == NULL )
        {
	    ds_dealloc_multi (multi_desc);
            return (NULL);
        }
        multi_desc->headers[array_count] = pack_desc;
        ++array_count;
    }
    return (multi_desc);
}   /*  End Function dsra_multi_desc */

/*PUBLIC_FUNCTION*/
packet_desc *dsra_packet_desc (Channel channel)
/*  [SUMMARY] Read ASCII representation of a packet descriptor.
    [PURPOSE] This routine will read the ASCII representation of a packet
    descriptor from a channel object. The routine will recursively read in
    array and linked list descriptors if required.
    <channel> The channel object to read from.
    [RETURNS] A pointer to the packet descriptor on success, else NULL.
*/
{
    unsigned int num_elements = 0;
    unsigned int element_count = 0;
    unsigned int elem_type;
    char elem_name[STRING_LENGTH];
    char temp_line[STRING_LENGTH];
    packet_desc *pack_desc;
    array_desc *arr_desc;
    packet_desc *list_desc;
    static char function_name[] = "dsra_packet_desc";

    /*  Get "PACKET"  */
    if ( (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE) ||
	(st_icmp (temp_line, "PACKET") != 0) )
    {
	(void) fprintf (stderr, "\"PACKET\" not found\n");
        return (NULL);
    }
    /*  Get number of elements  */
    if (dsra_uint (channel, &num_elements) == FALSE)
    {
	(void) fprintf (stderr, "Error reading number of elements\n");
        return (NULL);
    }
    if (num_elements < 1)
    {
	(void) fprintf (stderr, "Bad number of elements: %u\n", num_elements);
        return (NULL);
    }
    /*  Get "END"  */
    if ( (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE) ||
	(st_icmp (temp_line, "END") != 0) )
    {
	(void) fprintf (stderr, "\"END\" not found\n");
        return (NULL);
    }
    /*  Allocate packet descriptor  */
    if ( ( pack_desc = ds_alloc_packet_desc (num_elements) ) == NULL )
    {
	(void) fprintf (stderr,
			"Error allocating memory for packet descriptor\n");
        return (NULL);
    }
    for (element_count = 0; element_count < num_elements; ++element_count)
    {
	if (dsra_element_desc (channel, &elem_type, elem_name) == FALSE)
        {
	    ds_dealloc_packet (pack_desc, NULL);
            return (NULL);
        }
        pack_desc->element_types[element_count] = elem_type;
	if ( ds_element_is_named (elem_type) )
	{
	    /*  Named data type  */
	    if ( ( pack_desc->element_desc[element_count] =
		  st_dup (elem_name) ) == NULL )
	    {
		(void) fprintf (stderr,
				"Error allocating memory for element name: %s\n",
				elem_name);
		ds_dealloc_packet (pack_desc, NULL);
		return (NULL);
	    }
	}
	else
	{
	    switch (elem_type)
	    {
	      case ARRAYP:
		pack_desc->element_types[element_count] = K_ARRAY;
	      case K_ARRAY:
		if ( ( arr_desc = dsra_array_desc (channel, elem_type) )
		    == NULL )
		{
		    ds_dealloc_packet (pack_desc, NULL);
		    return (NULL);
		}
		pack_desc->element_desc[element_count] = (char *) arr_desc;
		break;
	      case LISTP:
		if ( ( list_desc = dsra_packet_desc (channel) ) == NULL )
		{
		    ds_dealloc_packet (pack_desc, NULL);
		    return (NULL);
		}
		pack_desc->element_desc[element_count] = (char *) list_desc;
		break;
	      default:
		/*  This should NEVER happen!  */
		(void) fprintf (stderr,
				"Illegal element type: %u returned from function: dsra_element_desc\n",
				elem_type);
		a_prog_bug (function_name);
		break;
	    }
	}
    }
    return (pack_desc);
}   /*  End Function dsra_packet_desc  */

/*PUBLIC_FUNCTION*/
flag dsra_element_desc (Channel channel, unsigned int *type, char name[])
/*  [SUMMARY] Read ASCII representation of an element descriptor.
    <channel> The channel object to read from.
    <type> The type of the element is written here.
    <name> The name of the element is written here. The length of the buffer
    must be <<STRING_LENGTH>> bytes.
    [RETURNS] TRUE if a valid element descriptor was read, else FALSE.
*/
{
    char temp_line[STRING_LENGTH];

    /*  Get "ELEMENT"  */
    if ( (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE) ||
	(st_icmp (temp_line, "ELEMENT") != 0) )
    {
	(void) fprintf (stderr, "\"ELEMENT\" not found\n");
        return (FALSE);
    }
    /*  Get element type  */
    if (dsra_type (channel, type) == FALSE)
    {
	(void) fprintf (stderr, "Element type not found\n");
        return (FALSE);
    }
    switch (*type)
    {
      case ARRAYP:
      case K_ARRAY:
      case LISTP:
        break;
      case MULTI_ARRAY:
      case NONE:
	/*  Not permitted  */
	(void) fprintf (stderr,
			"\"NONE\" is not a permitted element type\n");
	return (FALSE);
/*
	break;
*/
      default:
	/*  Atomic data type  */
	/*  Get element name  */
	if (chs_get_line (channel, name, STRING_LENGTH) == FALSE)
	{
	    (void) fprintf (stderr, "Element name not found\n");
	    return (FALSE);
	}
	break;
    }
    /*  Get "END"  */
    if ( (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE) ||
        (st_icmp (temp_line, "END") != 0) )
    {
	(void) fprintf (stderr, "\"END\" not found\n");
        return (FALSE);
    }
    /*  Everything OK  */
    return (TRUE);
}   /*  End Function dsra_element_desc  */

/*PUBLIC_FUNCTION*/
array_desc *dsra_array_desc (Channel channel, unsigned int type)
/*  [SUMMARY] Read ASCII representation of an array descriptor.
    <channel> The channel object to read from.
    <type> The type of the array. Legal values for this are:
        ARRAYP    K_ARRAY
    [RETURNS] A pointer to the array descriptor on success, else NULL.
*/
{
    unsigned int num_levels = 0;
    unsigned int num_dim = 0;
    unsigned int level_count;
    unsigned int product;
    unsigned int dim_count;
    unsigned int data;
    char temp_line[STRING_LENGTH];
    array_desc *arr_desc;
    dim_desc *dimension;
    packet_desc *pack_desc;
    static char function_name[] = "dsra_array_desc";

    if ( (type != ARRAYP) && (type != K_ARRAY) )
    {
	(void) fprintf (stderr, "Illegal type: %u\n", type);
	a_prog_bug (function_name);
    }
    /*  Get "ARRAY" */
    if ( (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE) ||
        (st_icmp (temp_line, "ARRAY") != 0) )
    {
	(void) fprintf (stderr, "\"ARRAY\" not found\n");
        return (NULL);
    }
    /*  Get number of dimensions  */
    if (dsra_uint (channel, &num_dim) == FALSE)
    {
	(void) fprintf (stderr, "Error reading number of dimensions\n");
        return (NULL);
    }
    if (num_dim < 1)
    {
	(void) fprintf (stderr, "Bad number of dimensions: %u\n", num_dim);
        return (NULL);
    }
    if (type == K_ARRAY)
    {
	/*  Get number of tiling levels  */
	if (dsra_uint (channel, &num_levels) != TRUE)
	{
	    (void) fprintf (stderr, "Error reading number of tiling levels\n");
	    return (NULL);
	}
	if (num_levels > 0)
	{
	    (void) fprintf (stderr, "Reading tiled array\n");
	}
    }
    else
    {
	num_levels = 0;
    }
    if ( ( arr_desc = ds_alloc_array_desc (num_dim, num_levels) ) == NULL )
    {
	m_error_notify (function_name, "array descriptor");
        return (NULL);
    }
    /*  Get any tiling information needed  */
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	/*  Get tile lengths if appropriate  */
	for (level_count = 0; level_count < num_levels; ++level_count)
	{
	    if (dsra_uint (channel, &data) != TRUE)
	    {
		(void) fprintf (stderr, "Error reading tile length[%u][%u]\n",
				dim_count, level_count);
		ds_dealloc_array_desc (arr_desc);
		return (NULL);
	    }
	    arr_desc->tile_lengths[dim_count][level_count] = data;
	}
    }
    /*  Get "END"  */
    if ( (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE) ||
        (st_icmp (temp_line, "END") != 0) )
    {
	(void) fprintf (stderr, "\"END\" not found\n");
        ds_dealloc_array_desc (arr_desc);
        return (NULL);
    }
    if (type == K_ARRAY)
    {
	arr_desc->padded = TRUE;
    }
    else
    {
	arr_desc->padded = FALSE;
    }
    /*  Get dimension descriptors  */
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if ( ( dimension = dsra_dim_desc (channel) ) == NULL )
        {
	    ds_dealloc_array_desc (arr_desc);
            return (NULL);
        }
        arr_desc->dimensions[dim_count] = dimension;
	/*  Compute tile product  */
	for (level_count = 0, product = 1; level_count < num_levels;
	     ++level_count)
	{
	    product *= arr_desc->tile_lengths[dim_count][level_count];
	}
	/*  Check if tile lengths appropriate  */
	if (dimension->length % product != 0 )
	{
	    (void) fprintf (stderr,
			    "Tile product: %u not a factor of length: %lu\n",
			    product, dimension->length);
	    a_func_abort (function_name, "bad data");
	    ds_dealloc_array_desc (arr_desc);
	    return (NULL);
	}
	arr_desc->lengths[dim_count] = dimension->length / product;
    }
    /*  Get packet descriptor for array */
    if ( ( pack_desc = dsra_packet_desc (channel) ) == NULL )
    {
	ds_dealloc_array_desc (arr_desc);
        return (NULL);
    }
    arr_desc->packet = pack_desc;
    return (arr_desc);
}   /*  End Function dsra_array_desc */

/*PUBLIC_FUNCTION*/
dim_desc *dsra_dim_desc (Channel channel)
/*  [SUMMARY] Read ASCII representation of a dimension descriptor.
    <channel> The channel object to read from.
    [RETURNS] A pointer to the dimension descriptor on success, else NULL.
*/
{
    unsigned int dim_length = 0;
    unsigned int coord_count = 0;
    double first_coord, last_coord;
    double coordinate;
    char dim_name[STRING_LENGTH];
    char temp_line[STRING_LENGTH];
    dim_desc *dimension;
    static char function_name[] = "dsra_dim_desc";

    /*  Get "DIMENSION" */
    if ( (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE) ||
        (st_icmp (temp_line, "DIMENSION") != 0) )
    {
	(void) fprintf (stderr, "\"DIMENSION\" not found\n");
        return (NULL);
    }
    /*  Get name  */
    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	(void) fprintf (stderr, "Dimension name not found\n");
        return (NULL);
    }
    (void) strcpy (dim_name, temp_line);
    /*  Get length  */
    if (dsra_uint (channel, &dim_length) == FALSE)
    {
	(void) fprintf (stderr, "Error reading dimension length\n");
        return (NULL);
    }
    if (dim_length < 1)
    {
	(void) fprintf (stderr, "Bad dimension length: %s\n", temp_line);
        return (NULL);
    }
    /*  Get "REGULAR" or "RANDOM"  */
    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	(void) fprintf (stderr, "\"REGULAR\" or \"RANDOM\" not found\n");
        return (NULL);
    }
    if (st_icmp (temp_line, "REGULAR") == 0)
    {
	/*  Dimension co-ordinates are regularly spaced */
        /*  Get first co-ordinate */
        if (dsra_double (channel, &first_coord) == FALSE)
        {
	    (void) fprintf (stderr, "Error reading first co-ordinate\n");
            return (NULL);
        }
        /*  Get last co-ordinate  */
        if (dsra_double (channel, &last_coord) == FALSE)
        {
	    (void) fprintf (stderr, "Error reading last co-ordinate\n");
            return (NULL);
        }
        /*  Get "END"  */
        if ( (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE) ||
            (st_icmp (temp_line, "END") != 0) )
        {
	    (void) fprintf (stderr, "\"END\" not found\n");
            return (NULL);
        }
        if ( ( dimension = ds_alloc_dim_desc (dim_name, dim_length,
					      first_coord, last_coord,
					      TRUE) ) == NULL )
        {
	    m_error_notify (function_name, "dimension descriptor");
            return (NULL);
        }
        return (dimension);
    }
    if (st_icmp (temp_line, "RANDOM") == 0)
    {
	/*  Dimension co-ordinates are to be supplied  */
        if ( ( dimension = ds_alloc_dim_desc (dim_name, dim_length,
					      0.0, 1.0, FALSE) ) == NULL )
        {
	    m_error_notify (function_name, "dimension descriptor");
            return (NULL);
        }
        /*  Get co-ordinates  */
        while (coord_count < dim_length)
        {
	    if ( !dsra_double (channel, &coordinate) )
            {
		(void) fprintf (stderr,
				"Error reading Co-ordinate number: %u\n",
				coord_count);
                m_free (dimension->name );
                m_free ( (char *) dimension->coordinates );
                m_free ( (char *) dimension );
                return (NULL);
            }
            dimension->coordinates[coord_count] = coordinate;
            ++coord_count;
        }
	dimension->first_coord = first_coord;
	dimension->last_coord = last_coord;
	if (first_coord < last_coord)
	{
	    dimension->minimum = first_coord;
	    dimension->maximum = last_coord;
	}
	else
	{
	    dimension->minimum = last_coord;
	    dimension->maximum = first_coord;
	}
        if ( (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE) ||
            (st_icmp (temp_line, "END") != 0) )
        {
	    (void) fprintf (stderr, "\"END\" not found\n");
            m_free (dimension->name );
            m_free ( (char *) dimension->coordinates );
            m_free ( (char *) dimension );
            return (NULL);
        }
        return (dimension);
    }
    (void) fprintf (stderr, "\"REGULAR\" or \"RANDOM\" not found\n");
    return (NULL);
}   /*  End Function dsra_dim_desc  */

/*PUBLIC_FUNCTION*/
flag dsra_multi_data (Channel channel, multi_array *multi_desc)
/*  [SUMMARY] READ ASCII representation of data in a multi_array descriptor.
    <channel> The channel object to read from.
    <multi_desc> The multi_array descriptor to store the data in. This is
    modified.
    [NOTE] The routine will only allocate space for linked list as it reads
    them.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int array_count;

    /*  Load the data  */
    for (array_count = 0; array_count < multi_desc->num_arrays;
	 ++array_count)
    {
	/*  Load the data for one array */
	if (dsra_packet (channel, multi_desc->headers[array_count],
			 multi_desc->data[array_count]) == FALSE)
	{
	    (void) fprintf (stderr, "Error reading array number %u\n",
			    array_count);
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function dsra_multi_data  */

/*PUBLIC_FUNCTION*/
flag dsra_packet (Channel channel, packet_desc *descriptor, char *packet)
/*  [SUMMARY] Read ASCII representation of a data packet.
    [PURPOSE] This routine will read the ASCII representation of a data packet.
    The routine will recursively read in sub arrays and linked lists.
    <channel> The channel object to read from.
    <descriptor> The packet descriptor.
    <packet> The packet data will be written here. The packet storage must
    already have been allocated.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int elem_count = 0;
    unsigned int type;
    extern char host_type_sizes[NUMTYPES];

    if (descriptor == NULL)
    {
	(void) fprintf (stderr,
			"No packet descriptor for packet to be read\n");
        return (FALSE);
    }
    if (packet == NULL)
    {
	(void) fprintf (stderr, "No packet to write to\n");
        return (FALSE);
    }
    while (elem_count < descriptor->num_elements)
    {
	type = descriptor->element_types[elem_count];
        if (dsra_element (channel, type,
			  (char *)descriptor->element_desc[elem_count],
			  packet)
            == FALSE)
        {
	    (void) fprintf (stderr,
			    "Error reading packet: element number %u\n",
			    elem_count);
            return (FALSE);
        }
        packet += host_type_sizes[type];
        ++elem_count;
    }
    return (TRUE);
}   /*  End Function dsra_packet */

/*PUBLIC_FUNCTION*/
flag dsra_element (Channel channel, unsigned int type, char *desc,
		   char *element)
/*  [SUMMARY] Read ASCII representation of an element.
    <channel> The channel object to read from.
    <type> The type of the element.
    <desc> The descriptor for the element.
    <element> The element data will be written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    FString *fstring;
    int b_value_r;
    int b_value_i;
    unsigned int ub_value_r;
    unsigned int ub_value_i;
    unsigned int fstring_len;
    char temp_line[STRING_LENGTH];
    static char function_name[] = "dsra_element";

    if (element == NULL)
    {
	(void) fprintf (stderr, "No element to write to\n");
        return (FALSE);
    }
    if ( (type != K_ARRAY) && (type != LISTP) )
    {
	if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
        {
	    (void) fprintf (stderr, "Error reading atomic element\n");
            return (FALSE);
        }
    }
    switch (type)
    {
      case K_FLOAT:
	if (sscanf (temp_line, "%e", (float *) element) != 1)
	{
	    (void) fprintf (stderr, "Error converting float value\n");
	    return (FALSE);
	}
	break;
      case K_DOUBLE:
	if (sscanf (temp_line, "%le", (double *) element) != 1)
	{
	    (void) fprintf (stderr,"Error converting double value\n");
	    return (FALSE);
	}
	break;
      case K_BYTE:
	if (sscanf (temp_line, "%d", &b_value_r) != 1)
	{
	    (void) fprintf (stderr, "Error converting byte value\n");
	    return (FALSE);
	}
	*element = b_value_r;
	break;
      case K_INT:
	if (sscanf (temp_line, "%d", (int *) element) != 1)
	{
	    (void) fprintf(stderr,"Error converting integer value\n");
	    return (FALSE);
	}
	break;
      case K_SHORT:
	if (sscanf (temp_line, "%hd", (short *) element) != 1)
	{
	    (void) fprintf (stderr, "Error converting short value\n");
	    return (FALSE);
	}
	break;
      case K_COMPLEX:
	if (sscanf (temp_line, "%e %e",
		    (float *) element, (float *) element + 1) != 2)
	{
	    (void) fprintf (stderr, "Error converting complex value\n");
	    return (FALSE);
	}
	break;
      case K_DCOMPLEX:
	if (sscanf (temp_line, "%le %le",
		    (double *) element, (double *) element + 1)
	    != 2)
	{
	    (void) fprintf (stderr, "Error converting dcomplex value\n");
	    return (FALSE);
	}
	break;
      case K_BCOMPLEX:
	if (sscanf (temp_line, "%d %d",
		    &b_value_r, &b_value_i) != 2)
	{
	    (void) fprintf (stderr, "Error converting bcomplex value\n");
	    return (FALSE);
	}
	*element++ = b_value_r;
	*element = b_value_i;
	break;
      case K_ICOMPLEX:
	if (sscanf (temp_line, "%d %d",
		    (int *) element, (int *) element + 1) != 2)
	{
	    (void) fprintf (stderr, "Error converting icomplex value\n");
	    return (FALSE);
	}
	break;
      case K_SCOMPLEX:
	if (sscanf (temp_line, "%hd %hd",
		    (short *) element, (short *) element + 1) != 2)
	{
	    (void) fprintf (stderr, "Error converting scomplex value\n");
	    return (FALSE);
	}
	break;
      case K_LONG:
	if (sscanf (temp_line, "%ld", (long *) element) != 1)
	{
	    (void) fprintf (stderr, "Error converting long value\n");
	    return (FALSE);
	}
	break;
      case K_LCOMPLEX:
	if (sscanf (temp_line, "%ld %ld",
		    (long *) element, (long *) element + 1) != 2)
	{
	    (void) fprintf (stderr, "Error converting lcomplex value\n");
	    return (FALSE);
	}
	break;
      case K_UBYTE:
	if (sscanf (temp_line, "%u", &ub_value_r) != 1)
	{
	    (void) fprintf (stderr, "Error converting ubyte value\n");
	    return (FALSE);
	}
	*element = ub_value_r;
	break;
      case K_UINT:
	if (sscanf (temp_line, "%u", (unsigned int *) element) != 1)
	{
	    (void) fprintf (stderr, "Error converting uint value\n");
	    return (FALSE);
	}
	break;
      case K_USHORT:
	if (sscanf (temp_line, "%hu", (unsigned short *) element) != 1)
	{
	    (void) fprintf (stderr, "Error converting ushort value\n");
	    return (FALSE);
	}
	break;
      case K_ULONG:
	if (sscanf (temp_line, "%lu", (unsigned long *) element) != 1)
	{
	    (void) fprintf (stderr, "Error converting ulong value\n");
	    return (FALSE);
	}
	break;
      case K_UBCOMPLEX:
	if (sscanf (temp_line, "%u %u", &ub_value_r, &ub_value_i) != 2)
	{
	    (void) fprintf (stderr, "Error converting ubcomplex value\n");
	    return (FALSE);
	}
	*element++ = ub_value_r;
	*element = ub_value_i;
	break;
      case K_UICOMPLEX:
	if (sscanf (temp_line, "%u %u",
		    (unsigned int *) element, (unsigned int *) element + 1)
	    != 2)
	{
	    (void) fprintf (stderr, "Error converting uicomplex value\n");
	    return (FALSE);
	}
	break;
      case K_USCOMPLEX:
	if (sscanf (temp_line, "%hu %hu",
		    (unsigned short *) element, (unsigned short *) element + 1)
	    != 2)
	{
	    (void) fprintf (stderr, "Error converting uscomplex value\n");
	    return (FALSE);
	}
	break;
      case K_ULCOMPLEX:
	if (sscanf (temp_line, "%lu %lu",
		    (unsigned long *) element, (unsigned long *) element + 1)
	    != 2)
	{
	    (void) fprintf (stderr, "Error converting ulcomplex value\n");
	    return (FALSE);
	}
	break;
      case K_VSTRING:
	if (*(char **) element != NULL)
	{
	    (void) fprintf (stderr, "Variable string already allocated\n");
	    a_prog_bug (function_name);
	}
	if ( ( *(char **) element = st_dup (temp_line) ) == NULL )
	{
	    m_error_notify (function_name, "variable string");
	    return (FALSE);
	}
	break;
      case K_FSTRING:
	fstring = (FString *) element;
	if ( (fstring->max_len > 0 ) || (fstring->string != NULL ) )
	{
	    (void) fprintf (stderr, "Fixed string already allocated\n");
	    a_prog_bug (function_name);
	}
	if (sscanf (temp_line, "%u", &fstring_len) != 1)
	{
	    (void) fprintf (stderr,
			    "Error converting string allocation value\n");
	    return (FALSE);
	}
	if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
        {
	    (void) fprintf (stderr, "Error reading fixed string\n");
            return (FALSE);
        }
	if ( ( fstring->string = m_alloc (fstring_len) ) == NULL )
	{
	    m_error_notify (function_name, "fixed string");
	    return (FALSE);
	}
	(void) strcpy (fstring->string, temp_line );
	fstring->max_len = fstring_len;
	break;
      case K_ARRAY:
	if (dsra_array ( channel, (array_desc *) desc,
			*( (char **) element ) ) == FALSE)
	{
	    return (FALSE);
	}
	break;
      case LISTP:
	if (dsra_list ( channel, (packet_desc *) desc,
		       *( (list_header **) element ) ) ==
	    FALSE)
	{
	    return (FALSE);
	}
	break;
      default:
	(void) fprintf (stderr, "Bad data type: %u\n", type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function dsra_element  */

/*PUBLIC_FUNCTION*/
flag dsra_array (Channel channel, array_desc *descriptor, char *array)
/*  [SUMMARY] Read ASCII representation of an array of data packets.
    <channel> The channel object to read from.
    <descriptor> The array descriptor.
    <array> The array data will be written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int array_count = 0;
    unsigned int array_size;
    unsigned int packet_size;

    if (descriptor == NULL)
    {
	(void) fprintf (stderr, "No array descriptor for array to be read\n");
        return (FALSE);
    }
    if ( descriptor->packet == NULL )
    {
	(void) fprintf (stderr, "No packet descriptor for array to be read\n");
        return (FALSE);
    }
    if (array == NULL)
    {
	(void) fprintf (stderr, "No array to write to\n");
        return (FALSE);
    }
    array_size = ds_get_array_size (descriptor);
    packet_size = ds_get_packet_size ( descriptor->packet );
    while (array_count++ < array_size)
    {
	if (dsra_packet (channel, descriptor->packet, array) == FALSE)
        {
	    (void) fprintf (stderr, "Error reading array: packet number %u\n",
			    array_count - 1);
            return (FALSE);
        }
        array += packet_size;
    }
    return (TRUE);
}   /*  End Function dsra_array  */

/*PUBLIC_FUNCTION*/
flag dsra_list (Channel channel, packet_desc *descriptor, list_header *header)
/*  [SUMMARY] Read ASCII represention of a linked list.
    [PURPOSE] This routine will read in the ASCII representation of a linked
    list of data from a channel object. The routine will recursively read in
    sub arrays and linked lists.
    <channel> The channel object to read from.
    <descriptor> The descriptor for the list packets.
    <header> The linked list header. This is modified. The linked list entries
    will be contiguous in memory.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int length;
    unsigned int pack_size;
    unsigned int count;
    char *data;
    char temp_line[STRING_LENGTH];
    static char function_name[] = "dsra_list";

    if (descriptor == NULL)
    {
	(void) fprintf (stderr, "No descriptor for list to be read\n");
        return (FALSE);
    }
    if (header == NULL)
    {
	(void) fprintf (stderr, "No header for list to be read\n");
    }
    if ( header->magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    /*  Get linked list length  */
    if (dsra_uint (channel, &length) == FALSE)
    {
	(void) fprintf (stderr, "Error reading linked list length\n");
        return (FALSE);
    }
    if (ds_alloc_contiguous_list (descriptor, header, length, TRUE, TRUE)
	== FALSE)
    {
	m_error_notify (function_name, "linked list");
        return (FALSE);
    }
    /*  Get list sort type  */
    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	(void) fprintf (stderr, "Error reading list sort type\n");
        ds_dealloc_list_entries (descriptor, header);
        return (FALSE);
    }
    if (st_icmp (temp_line, "INCREASING") == 0)
    {
	header->sort_type = SORT_INCREASING;
    }
    else if (st_icmp (temp_line, "DECREASING") == 0)
    {
	header->sort_type = SORT_DECREASING;
    }
    else if (st_icmp (temp_line, "RANDOM") == 0)
    {
	header->sort_type = SORT_RANDOM;
    }
    else
    {
	(void) fprintf (stderr, "Bad sort type: %s\n", temp_line);
        ds_dealloc_list_entries (descriptor, header);
        return (FALSE);
    }
    /*  Get list sort element number  */
    if (dsra_uint (channel, &header->sort_elem_num) == FALSE)
    {
	(void) fprintf (stderr, "Error reading sort element number\n");
        ds_dealloc_list_entries (descriptor, header);
        return (FALSE);
    }
    if ( header->sort_elem_num >= header->length )
    {
	(void) fprintf (stderr,
			"List sort element number: %u is not less than length: %lu\n",
			header->sort_elem_num, header->length);
        ds_dealloc_list_entries (descriptor, header);
        return (FALSE);
    }
    /*  Read data one packet at a time  */
    pack_size = ds_get_packet_size (descriptor);
    for (count = 0, data = header->contiguous_data;
	 count < header->contiguous_length; ++count, data += pack_size)
    {
	if (dsra_packet (channel, descriptor, data) != TRUE)
        {
	    (void) fprintf (stderr, "Error reading linked list\n");
            ds_dealloc_list_entries (descriptor, header);
            return (FALSE);
        }
    }
    return (TRUE);
}   /*  End Function dsra_list  */

/*PUBLIC_FUNCTION*/
flag dsra_flag (Channel channel, flag *logical)
/*  [SUMMARY] Read ASCII representation of a boolean value.
    <channel> The channel object to read from.
    <logical> The boolean value will be written here.
    [RETURNS] TRUE if the flag was read in without error, else FALSE.
*/
{
    char temp_line[STRING_LENGTH];

    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	return (FALSE);
    }
    if (st_icmp (temp_line, "TRUE") == 0)
    {
	*logical = TRUE;
        return (TRUE);
    }
    if (st_icmp (temp_line, "FALSE") == 0)
    {
	*logical = FALSE;
        return (TRUE);
    }
    (void) fprintf (stderr, "Error converting string: \"%s\" to flag\n",
		    temp_line);
    return (FALSE);
}   /*  End Function dsra_flag  */

/*PUBLIC_FUNCTION*/
flag dsra_type (Channel channel, unsigned int *type)
/*  [SUMMARY] Read ASCII representation of a data type.
    <channel> The channel object to read from.
    <type> The type value will be written here.
    [RETURNS] TRUE if the type was read without error, else FALSE.
*/
{
    char temp_line[STRING_LENGTH];

    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	return (FALSE);
    }
    if (st_icmp (temp_line, "NONE") == 0)
    {
	*type = NONE;
        return (TRUE);
    }
    if (st_icmp (temp_line, "FLOAT") == 0)
    {
	*type = K_FLOAT;
        return (TRUE);
    }
    if (st_icmp (temp_line, "DOUBLE") == 0)
    {
	*type = K_DOUBLE;
        return (TRUE);
    }
    if (st_icmp (temp_line, "BYTE") == 0)
    {
	*type = K_BYTE;
        return (TRUE);
    }
    if (st_icmp (temp_line, "INT") == 0)
    {
	*type = K_INT;
        return (TRUE);
    }
    if (st_icmp (temp_line, "SHORT") == 0)
    {
	*type = K_SHORT;
        return (TRUE);
    }
    if (st_icmp (temp_line, "ARRAYP") == 0)
    {
	(void) fprintf (stderr, "Reading unpadded array\n");
	*type = ARRAYP;
        return (TRUE);
    }
    if (st_icmp (temp_line, "K_ARRAY") == 0)
    {
	(void) fprintf (stderr, "Reading padded array\n");
	*type = K_ARRAY;
        return (TRUE);
    }
    if (st_icmp (temp_line, "LISTP") == 0)
    {
	*type = LISTP;
        return (TRUE);
    }
    if (st_icmp (temp_line, "MULTI_ARRAY") == 0)
    {
	*type = MULTI_ARRAY;
        return (TRUE);
    }
    if (st_icmp (temp_line, "COMPLEX") == 0)
    {
	*type = K_COMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "DCOMPLEX") == 0)
    {
	*type = K_DCOMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "BCOMPLEX") == 0)
    {
	*type = K_BCOMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "ICOMPLEX") == 0)
    {
	*type = K_ICOMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "SCOMPLEX") == 0)
    {
	*type = K_SCOMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "LONG") == 0)
    {
	*type = K_LONG;
        return (TRUE);
    }
    if (st_icmp (temp_line, "LCOMPLEX") == 0)
    {
	*type = K_LCOMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "UBYTE") == 0)
    {
	*type = K_UBYTE;
        return (TRUE);
    }
    if (st_icmp (temp_line, "UINT") == 0)
    {
	*type = K_UINT;
        return (TRUE);
    }
    if (st_icmp (temp_line, "USHORT") == 0)
    {
	*type = K_USHORT;
        return (TRUE);
    }
    if (st_icmp (temp_line, "ULONG") == 0)
    {
	*type = K_ULONG;
        return (TRUE);
    }
    if (st_icmp (temp_line, "UBCOMPLEX") == 0)
    {
	*type = K_UBCOMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "UICOMPLEX") == 0)
    {
	*type = K_UICOMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "USCOMPLEX") == 0)
    {
	*type = K_USCOMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "ULCOMPLEX") == 0)
    {
	*type = K_ULCOMPLEX;
        return (TRUE);
    }
    if (st_icmp (temp_line, "VSTRING") == 0)
    {
	*type = K_VSTRING;
        return (TRUE);
    }
    if (st_icmp (temp_line, "FSTRING") == 0)
    {
	*type = K_FSTRING;
        return (TRUE);
    }
    (void) fprintf (stderr, "Error converting string: \"%s\" to type\n",
		    temp_line);
    return (FALSE);
}   /*  End Function dsra_type  */

/*PUBLIC_FUNCTION*/
flag dsra_uint (Channel channel, unsigned int *value)
/*  [SUMMARY] Read the ASCII representation of an unsigned integer.
    <channel> The channel object to read from.
    <value> The result is written here.
    [RETURNS] TRUE if successful, else FALSE.
*/
{
    char temp_line[STRING_LENGTH];

    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	(void) fprintf (stderr, "Error reading unsigned integer value\n");
        return (FALSE);
    }
    if (sscanf (temp_line, "%u", value) != 1)
    {
	(void) fprintf (stderr,
			"Error converting string: \"%s\" to integer value\n",
			temp_line);
        return (FALSE);
    }
    return (TRUE);
}   /*  End Function dsra_uint  */

/*PUBLIC_FUNCTION*/
flag dsra_int (Channel channel, int *value)
/*  [SUMMARY] Read the ASCII representation of a signed integer.
    <channel> The channel object to read from.
    <value> The result is written here.
    [RETURNS] TRUE if successful, else FALSE.
*/
{
    char temp_line[STRING_LENGTH];

    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	(void) fprintf (stderr, "Error reading integer value\n");
        return (FALSE);
    }
    if (sscanf (temp_line, "%d", value) != 1)
    {
	(void) fprintf (stderr,
			"Error converting string: \"%s\" to integer value\n",
			temp_line);
        return (FALSE);
    }
    return (TRUE);
}   /*  End Function dsra_int  */

/*PUBLIC_FUNCTION*/
flag dsra_float (Channel channel, float *value)
/*  [SUMMARY] Read the ASCII representation of a floating point value.
    <channel> The channel object to read from.
    <value> The result is written here.
    [RETURNS] TRUE if successful, else FALSE.
*/
{
    char temp_line[STRING_LENGTH];

    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	(void) fprintf (stderr, "Error reading floating point value\n");
        return (FALSE);
    }
    if (sscanf (temp_line, "%e", value) != 1)
    {
	(void) fprintf (stderr,
			"Error converting string: \"%s\" to floating value\n",
			temp_line);
        return (FALSE);
    }
    return (TRUE);
}   /*  End Function dsra_float  */

/*PUBLIC_FUNCTION*/
flag dsra_double (Channel channel, double *value)
/*  [SUMMARY] Read ASCII representation of a double floating point value.
    <channel> The channel object to read from.
    <value> The result is written here.
    [RETURNS] TRUE if successful, else FALSE.
*/
{
    char temp_line[STRING_LENGTH];

    if (chs_get_line (channel, temp_line, STRING_LENGTH) == FALSE)
    {
	(void) fprintf (stderr, "Error reading floating point value\n");
        return (FALSE);
    }
    if (sscanf (temp_line, "%le", value) != 1)
    {
	(void) fprintf (stderr,
			"Error converting string: \"%s\" to double value\n",
			temp_line);
        return (FALSE);
    }
    return (TRUE);
}   /*  End Function dsra_double */
