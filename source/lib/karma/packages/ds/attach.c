/*LINTLIBRARY*/
/*  attach.c

    This code provides data structure attachment routines.

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

    This file contains the various utility routines for attaching
  miscellaneous data to Karma general data structures.


    Written by      Richard Gooch   7-OCT-1992

    Updated by      Richard Gooch   26-NOV-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   2-JUL-1993: Improved error trapping.

    Updated by      Richard Gooch   31-AUG-1993: Added
  ds_put_unique_named_string  .

    Updated by      Richard Gooch   4-SEP-1993: Added
  ds_get_unique_named_string  .

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   4-NOV-1994: Improved some diagnostics.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ds/attach.c

    Updated by      Richard Gooch   19-APR-1995: Removed warning message in
  <ds_get_unique_named_value> and <ds_get_unique_named_string> when element not
  found.

    Updated by      Richard Gooch   15-JUN-1995: Made use of IS_ALIGNED macro.

    Updated by      Richard Gooch   22-AUG-1995: Inserted code to handle
  unaligned string types in <ds_put_unique_named_string> and
  <ds_get_unique_named_string>.

    Updated by      Richard Gooch   9-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   8-AUG-1996: Created
  <ds_copy_unique_named_element> routine.

    Updated by      Richard Gooch   9-AUG-1996: Added <fail_if_not_found>
  parameter to <ds_copy_unique_named_element> routine.

    Updated by      Richard Gooch   29-AUG-1996: Made
  <ds_get_unique_named_value> tolerant when element is a string type.

    Last updated by Richard Gooch   29-SEP-1996: Created <ds_get_fits_axis>.


*/
#include <stdio.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>
#include <os.h>


/*PUBLIC_FUNCTION*/
flag ds_put_unique_named_value (packet_desc *pack_desc, char **packet,
				CONST char *name, unsigned int type,
				double value[2], flag update)
/*  [SUMMARY] Add a unique named value to a Karma general data structure.
    <pack_desc> The packet descriptor to add the name to. This descriptor will
    be modified.
    <packet> The pointer to the unique packet. Note that the existing packet
    data is copied to a new packet, and a pointer to this packet is written
    back here.
    <name> The name of the element.
    <type> The type of the data which is to be written.
    <value> The value of the data.
    <update> If TRUE, then the routine will allow an existing named value to be
    updated, otherwise the routine will fail if an update (rather than a
    create) is attempted. The <<type>> parameter is ignored for  updates (i.e.
    you can't change the type).
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int old_packet_size;
    char *new_packet;
    char **new_element_desc;
    unsigned int *new_element_types;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_put_unique_named_value";

    FLAG_VERIFY (update);
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
    /*  Test to see if element is legal  */
    if ( !ds_element_is_atomic (type) )
    {
	fprintf (stderr, "Element type: %u is not atomic\n", type);
	return (FALSE);
    }
    /*  Test to see if named item already exists  */
    switch ( ds_f_name_in_packet (pack_desc, name, (char **) NULL,
				  (unsigned int *) NULL) )
    {
      case IDENT_NOT_FOUND:
	/*  This is what we want  */
	break;
      case IDENT_ELEMENT:
	if (update)
	{
	    return ( ds_put_named_element (pack_desc, *packet, name, value) );
	}
	fprintf (stderr, "Element: \"%s\" already exists\n", name);
	return (FALSE);
/*
	break;
*/
      case IDENT_DIMENSION:
	fprintf (stderr,
			"Item: \"%s\" already used for a dimension name\n",
			name);
	return (FALSE);
/*
	break;
*/
      case IDENT_MULTIPLE:
	fprintf (stderr,
			"Item: \"%s\" has multiple occurrences\n", name);
	return (FALSE);
/*
	break;
*/
      default:
	fprintf (stderr,
			"Illegal return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    /*  Allocate new element type and descriptor arrays  */
    if ( ( new_element_types = (unsigned int *)
	  m_alloc ( sizeof *new_element_types *
		   (1 + pack_desc->num_elements) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of new element types");
	return (FALSE);
    }
    if ( ( new_element_desc = (char **)
	  m_alloc ( sizeof *new_element_desc *
		   (1 + pack_desc->num_elements) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of new element descriptors");
	m_free ( (char *) new_element_types );
	return (FALSE);
    }
    /*  Allocate space for new element name  */
    if ( ( new_element_desc[pack_desc->num_elements] =
	  m_alloc (strlen (name) + 1) )
	== NULL )
    {
	m_error_notify (function_name, "new element name");
	m_error_notify (function_name, "new packet");
	m_free ( (char *) new_element_types );
	m_free ( (char *) new_element_desc );
	return (FALSE);
    }
    old_packet_size = ds_get_packet_size (pack_desc);
    /*  Allocate new packet  */
    if ( ( new_packet = m_alloc (old_packet_size + host_type_sizes[type]) )
	== NULL )
    {
	m_error_notify (function_name, "new packet");
	m_free ( (char *) new_element_types );
	m_free (new_element_desc[pack_desc->num_elements]);
	m_free ( (char *) new_element_desc );
	return (FALSE);
    }
    /*  Copy over old types, descriptor pointers and packet data  */
    m_copy ( (char *) new_element_types,
	    (char *) pack_desc->element_types,
	    sizeof *pack_desc->element_types *
	    pack_desc->num_elements );
    m_copy ( (char *) new_element_desc,
	    (char *) pack_desc->element_desc,
	    sizeof *pack_desc->element_desc *
	    pack_desc->num_elements );
    m_copy (new_packet, *packet, old_packet_size);
    /*  Write in new element descriptor information  */
    new_element_types[pack_desc->num_elements] = type;
    strcpy (new_element_desc[pack_desc->num_elements], name);
    /*  Write in new data  */
    ds_put_element (new_packet + old_packet_size, type, value);
    /*  Deallocate old types, descriptor pointers and packet data  */
    m_free ( (char *) pack_desc->element_types );
    m_free ( (char *) pack_desc->element_desc );
    m_free (*packet);
    /*  Link in new types, descriptor pointers and packet data  */
    pack_desc->element_types = new_element_types;
    pack_desc->element_desc = new_element_desc;
    *packet = new_packet;
    /*  Increment number of elements  */
    ++pack_desc->num_elements;
    return (TRUE);
}   /*  End Function ds_put_unique_named_value  */

/*PUBLIC_FUNCTION*/
flag ds_put_unique_named_string (packet_desc *pack_desc, char **packet,
				 CONST char *name, CONST char *string,
				 flag update)
/*  [SUMMARY] Add a unique named string to a Karma general data structure.
    <pack_desc> The packet descriptor to add the name to. This descriptor will
    be modified.
    <packet> The pointer to the unique packet. Note that the existing packet
    data is copied to a new packet, and a pointer to this packet is written
    back here.
    <name> The name of the element.
    <string> The string value.
    <update> If TRUE, then the routine will allow an existing named string to
    be updated, otherwise the routine will fail if an update (rather than a
    create) is attempted.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int old_packet_size;
    unsigned int elem_index;
    char *copy;
    char *element;
    char *new_packet;
    char **new_element_desc;
    unsigned int *new_element_types;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_put_unique_named_string";

    FLAG_VERIFY (update);
    if (string == NULL)
    {
	fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Test to see if named item already exists  */
    switch ( ds_f_name_in_packet (pack_desc, name, (char **) NULL,
				  &elem_index) )
    {
      case IDENT_NOT_FOUND:
	/*  This is what we want  */
	break;
      case IDENT_ELEMENT:
	if (pack_desc->element_types[elem_index] != K_VSTRING)
	{
	    fprintf (stderr,
			    "Element: \"%s\" must be of type K_VSTRING\n",
			    name);
	    return (FALSE);
	}
	if (!update)
	{
	    fprintf (stderr, "Element: \"%s\" already exists\n", name);
	    return (FALSE);
	}
	if ( ( copy = m_alloc (strlen (string) + 1) ) == NULL )
	{
	    m_error_notify (function_name, "copy of string");
	    return (FALSE);
	}
	strcpy (copy, string);
	/*  Deallocate old string  */
	element = *packet + ds_get_element_offset (pack_desc, elem_index);
	if (*(char **) element != NULL) m_free (*(char **) element);
	*(char **) element = copy;
	return (TRUE);
/*
	break;
*/
      case IDENT_DIMENSION:
	fprintf (stderr,
			"Item: \"%s\" already used for a dimension name\n",
			name);
	return (FALSE);
/*
	break;
*/
      case IDENT_MULTIPLE:
	fprintf (stderr,
			"Item: \"%s\" has multiple occurrences\n", name);
	return (FALSE);
/*
	break;
*/
      default:
	fprintf (stderr,
			"Illegal return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    /*  Allocate new element type and descriptor arrays  */
    if ( ( new_element_types = (unsigned int *)
	  m_alloc ( sizeof *new_element_types *
		   (1 + pack_desc->num_elements) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of new element types");
	return (FALSE);
    }
    if ( ( new_element_desc = (char **)
	  m_alloc ( sizeof *new_element_desc *
		   (1 + pack_desc->num_elements) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of new element descriptors");
	m_free ( (char *) new_element_types );
	return (FALSE);
    }
    /*  Allocate space for new element name  */
    if ( ( new_element_desc[pack_desc->num_elements] =
	  m_alloc (strlen (name) + 1) )
	== NULL )
    {
	m_error_notify (function_name, "new element name");
	m_error_notify (function_name, "new packet");
	m_free ( (char *) new_element_types );
	m_free ( (char *) new_element_desc );
	return (FALSE);
    }
    old_packet_size = ds_get_packet_size (pack_desc);
    /*  Allocate new packet  */
    if ( ( new_packet = m_alloc (old_packet_size +
				 host_type_sizes[K_VSTRING]) )
	== NULL )
    {
	m_error_notify (function_name, "new packet");
	m_free ( (char *) new_element_types );
	m_free (new_element_desc[pack_desc->num_elements]);
	m_free ( (char *) new_element_desc );
	return (FALSE);
    }
    /*  Copy over old types, descriptor pointers and packet data  */
    m_copy ( (char *) new_element_types,
	    (char *) pack_desc->element_types,
	    sizeof *pack_desc->element_types *
	    pack_desc->num_elements );
    m_copy ( (char *) new_element_desc,
	    (char *) pack_desc->element_desc,
	    sizeof *pack_desc->element_desc *
	    pack_desc->num_elements );
    m_copy (new_packet, *packet, old_packet_size);
    /*  Write in new element descriptor information  */
    new_element_types[pack_desc->num_elements] = K_VSTRING;
    strcpy (new_element_desc[pack_desc->num_elements], name);
    /*  Write in new data  */
    if ( ( copy = m_alloc (strlen (string) + 1) ) == NULL )
    {
	m_error_notify (function_name, "copy of string");
	m_error_notify (function_name, "new packet");
	m_free ( (char *) new_element_types );
	m_free (new_element_desc[pack_desc->num_elements]);
	m_free ( (char *) new_element_desc );
	return (FALSE);
    }
    strcpy (copy, string);
    element = new_packet + ds_get_element_offset (pack_desc,
						  pack_desc->num_elements);
#ifdef NEED_ALIGNED_DATA
    m_copy (element, (char *) &copy, sizeof copy);
#else
    *(char **) element = copy;
#endif
    /*  Deallocate old types, descriptor pointers and packet data  */
    m_free ( (char *) pack_desc->element_types );
    m_free ( (char *) pack_desc->element_desc );
    m_free (*packet);
    /*  Link in new types, descriptor pointers and packet data  */
    pack_desc->element_types = new_element_types;
    pack_desc->element_desc = new_element_desc;
    *packet = new_packet;
    /*  Increment number of elements  */
    ++pack_desc->num_elements;
    return (TRUE);
}   /*  End Function ds_put_unique_named_string  */

/*PUBLIC_FUNCTION*/
flag ds_get_unique_named_value (CONST packet_desc *pack_desc,
				CONST char *packet,
				CONST char *name, unsigned int *type,
				double value[2])
/*  [SUMMARY] Get a unique named value from a Karma general data structure.
    <pack_desc> A pointer to the packet descriptor.
    <packet> A pointer to the packet containing the named value.
    <name> The name of the value.
    <type> The type of the element found will be written here. If this is NULL,
    nothing is written here. On error, the type of the element is written here.
    The value NONE is written here if the element does not exist.
    <value> The value of the element data will be written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int elem_num, elem_type;
    static char function_name[] = "ds_get_unique_named_value";

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
    /*  Try to find element  */
    if ( ( elem_num = ds_f_elem_in_packet (pack_desc, name) ) >=
	 pack_desc->num_elements )
    {
	if (type != NULL) *type = NONE;
	return (FALSE);
    }
    elem_type = pack_desc->element_types[elem_num];
    if (type != NULL) *type = elem_type;
    if ( !ds_element_is_atomic (elem_type) )
    {
	fprintf (stderr, "Element: \"%s\" is not atomic\n", name);
	return (FALSE);
    }
    if ( !ds_get_element (packet + ds_get_element_offset (pack_desc, elem_num),
			  pack_desc->element_types[elem_num], value,
			  (flag *) NULL) )
    {
	fprintf (stderr, "Error getting data for element: \"%s\"\n", name);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function ds_get_unique_named_value  */

/*PUBLIC_FUNCTION*/
char *ds_get_unique_named_string (CONST packet_desc *pack_desc,
				  CONST char *packet, CONST char *name)
/*  [SUMMARY] Get a unique named string from a Karma general data structure.
    <pack_desc> The packet descriptor.
    <packet> The packet containing the named string.
    <name> The element name.
    [RETURNS] A pointer to a dynamically allocated copy of the string on
    success, else NULL.
*/
{
    FString tmp;  /*  Assume this is always big enough  */
    unsigned int type, size, elem_num;
    CONST char *element;
    char *orig;
    char *copy;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_get_unique_named_string";

    /*  Try to find element  */
    if ( ( elem_num = ds_f_elem_in_packet (pack_desc, name) ) >=
	pack_desc->num_elements )
    {
	return (NULL);
    }
    element = packet + ds_get_element_offset (pack_desc, elem_num);
    type = pack_desc->element_types[elem_num];
    size = host_type_sizes[type];
    m_copy ( (char *) &tmp, element, size );
    element = (char *) &tmp;
    switch (type)
    {
      case K_VSTRING:
	orig = *(char **) element;
	break;
      case K_FSTRING:
	orig = (* (FString *) element ).string;
	break;
      default:
	fprintf (stderr, "Element is not a named string\n");
	return (NULL);
/*
	break;
*/
    }
    if ( ( copy = m_alloc (strlen (orig) + 1) ) == NULL )
    {
	m_error_notify (function_name, "string copy");
	return (NULL);
    }
    strcpy (copy, orig);
    return (copy);
}   /*  End Function ds_get_unique_named_string  */

/*PUBLIC_FUNCTION*/
flag ds_copy_unique_named_element (packet_desc *out_desc, char **out_packet,
				   CONST packet_desc *in_desc,
				   CONST char *in_packet, CONST char *name,
				   flag fail_if_not_found,
				   flag fail_on_duplicate, flag replace)
/*  [SUMMARY] Copy a unique named element from one packet to another.
    <out_desc> The packet descriptor to add the name to. This descriptor will
    be modified.
    <out_packet> The pointer to the unique packet. Note that the existing
    packet data is copied to a new packet, and a pointer to this packet is
    written back here.
    <in_desc> The input packet descriptor.
    <in_packet> The input packet.
    <name> The name of the element to copy.
    <fail_if_not_found> If TRUE, the routine will fail if the element does not
    exist in the input packet.
    <fail_on_duplicate> If TRUE, the routine will fail if the element already
    exists in the output packet.
    <replace> If TRUE and the element already exists in the output packet, it
    is replaced.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag ok;
    unsigned int elem_num, elem_type;
    char *new_string;
    double value[2];

    if (ds_f_elem_in_packet (out_desc, name) < out_desc->num_elements)
    {
	/*  Element already exists  */
	if (fail_on_duplicate) return (FALSE);
	if (!replace) return (TRUE);
    }
    /*  Try to find element  */
    if ( ( elem_num = ds_f_elem_in_packet (in_desc, name) ) >=
	 in_desc->num_elements )
    {
	return (fail_if_not_found ? FALSE : TRUE);
    }
    elem_type = in_desc->element_types[elem_num];
    if ( ds_element_is_atomic (elem_type) )
    {
	if ( !ds_get_element (in_packet + ds_get_element_offset (in_desc,
								 elem_num),
			      in_desc->element_types[elem_num], value,
			      (flag *) NULL) )
	{
	    fprintf (stderr, "Error getting data for element: \"%s\"\n",
			    name);
	    return (FALSE);
	}
	return ( ds_put_unique_named_value (out_desc, out_packet,
					    name, elem_type, value, FALSE) );
    }
    if (elem_type != K_VSTRING)
    {
	fprintf (stderr, "Not VSTRING type\n");
	return (FALSE);
    }
    /*  VSTRING type  */
    if ( ( new_string = ds_get_unique_named_string (in_desc, in_packet, name) )
	 == NULL ) return (FALSE);
    ok = ds_put_unique_named_string (out_desc, out_packet, name, new_string,
				     TRUE);
    m_free (new_string);
    return (ok);
}   /*  End Function ds_copy_unique_named_element  */

/*PUBLIC_FUNCTION*/
unsigned int ds_get_fits_axis (CONST packet_desc *top_pack_desc,
			       CONST char *top_packet, CONST char *dim_name)
/*  [SUMMARY] Get the FITS axis number of a dimension.
    <top_pack_desc> The top-level packet descriptor.
    <top_packet> The top-level packet.
    <dim_name> The name of the dimension.
    [RETURNS] The FITS axis number on success, else 0.
*/
{
    unsigned int count;
    char *ptr;
    char ctype[STRING_LENGTH], dname[STRING_LENGTH];

    /*  Theoretically, one should use the "NAXIS" keyword to determine how many
	axes there are to check, but since perhaps not everyone writes this
	keyword, I just keep searching until there are no more "CTYPEn"
	keywords. If "CTYPEn" does not exist, assume "CTYPEm" (where m > n)
	does not exist.
	Note that this whole scheme depends on the Karma dimension names being
	the same as those in the FITS header
	*/
    for (count = 1; TRUE; ++count)
    {
	sprintf (ctype, "CTYPE%u", count);
	if ( ( ptr = ds_get_unique_named_string (top_pack_desc, top_packet,
						 ctype) ) == NULL )
	{
	    return (0);
	}
	strcpy (dname, ptr);
	m_free (ptr);
	if (strcmp (dim_name, dname) != 0) continue;
	/*  Found it!  */
	return (count);
    }
}   /*  End Function ds_get_fits_axis  */
