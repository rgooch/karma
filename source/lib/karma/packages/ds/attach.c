/*LINTLIBRARY*/
/*  attach.c

    This code provides data structure attachment routines.

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

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/ds/attach.c


*/
#include <stdio.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>


/*PUBLIC_FUNCTION*/
flag ds_put_unique_named_value (pack_desc, packet, name, type, value, update)
/*  This routine will add a unique named value to a Karma general data
    structure.
    The packet descriptor to add the name to must be pointed to by  pack_desc
    This descriptor will be modified.
    The pointer to the unique packet which is described by  pack_desc  must be
    pointed to by  packet  .Note that the existing packet data is copied to a
    new packet, and a pointer to this packet is written back to the storage
    pointed to by  packet  .
    The name of the element must be pointed to by  name  .
    The type of the data which is to be written must be given by  type  .
    The value of the data must be pointed to by  value  .
    If the value of  update  is TRUE, then the routine will allow an existing
    named value to be updated, otherwise the routine will fail if an update
    (rather than a create) is attempted. The  type  parameters is ignored for
    updates (ie. you can't change the type).
    The routine returns TRUE on success, else it returns FALSE.
*/
packet_desc *pack_desc;
char **packet;
CONST char *name;
unsigned int type;
double *value;
flag update;
{
    unsigned int old_packet_size;
    char *new_packet;
    char **new_element_desc;
    unsigned int *new_element_types;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_put_unique_named_value";

    FLAG_VERIFY (update);
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
    /*  Test to see if element is legal  */
    if (ds_element_is_atomic (type) != TRUE)
    {
	(void) fprintf (stderr, "Element type: %u is not atomic\n", type);
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
	(void) fprintf (stderr, "Element: \"%s\" already exists\n", name);
	return (FALSE);
/*
	break;
*/
      case IDENT_DIMENSION:
	(void) fprintf (stderr,
			"Item: \"%s\" already used for a dimension name\n",
			name);
	return (FALSE);
/*
	break;
*/
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Item: \"%s\" has multiple occurrences\n", name);
	return (FALSE);
/*
	break;
*/
      default:
	(void) fprintf (stderr,
			"Illegal return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    /*  Allocate new element type and descriptor arrays  */
    if ( ( new_element_types = (unsigned int *)
	  m_alloc ( sizeof *new_element_types *
		   (1 + (*pack_desc).num_elements) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of new element types");
	return (FALSE);
    }
    if ( ( new_element_desc = (char **)
	  m_alloc ( sizeof *new_element_desc *
		   (1 + (*pack_desc).num_elements) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of new element descriptors");
	m_free ( (char *) new_element_types );
	return (FALSE);
    }
    /*  Allocate space for new element name  */
    if ( ( new_element_desc[(*pack_desc).num_elements] =
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
				 host_type_sizes[type]) )
	== NULL )
    {
	m_error_notify (function_name, "new packet");
	m_free ( (char *) new_element_types );
	m_free (new_element_desc[(*pack_desc).num_elements]);
	m_free ( (char *) new_element_desc );
	return (FALSE);
    }
    /*  Copy over old types, descriptor pointers and packet data  */
    m_copy ( (char *) new_element_types,
	    (char *) (*pack_desc).element_types,
	    sizeof *(*pack_desc).element_types *
	    (*pack_desc).num_elements );
    m_copy ( (char *) new_element_desc,
	    (char *) (*pack_desc).element_desc,
	    sizeof *(*pack_desc).element_desc *
	    (*pack_desc).num_elements );
    m_copy (new_packet, *packet, old_packet_size);
    /*  Write in new element descriptor information  */
    new_element_types[(*pack_desc).num_elements] = type;
    (void) strcpy (new_element_desc[(*pack_desc).num_elements], name);
    /*  Write in new data  */
    (void) ds_put_element (new_packet + old_packet_size, type, value);
    /*  Deallocate old types, descriptor pointers and packet data  */
    m_free ( (char *) (*pack_desc).element_types );
    m_free ( (char *) (*pack_desc).element_desc );
    m_free (*packet);
    /*  Link in new types, descriptor pointers and packet data  */
    (*pack_desc).element_types = new_element_types;
    (*pack_desc).element_desc = new_element_desc;
    *packet = new_packet;
    /*  Increment number of elements  */
    ++(*pack_desc).num_elements;
    return (TRUE);
}   /*  End Function ds_put_unique_named_value  */

/*PUBLIC_FUNCTION*/
flag ds_put_unique_named_string (pack_desc, packet, name, string, update)
/*  This routine will add a unique named string to a Karma general data
    structure.
    The packet descriptor to add the name to must be pointed to by  pack_desc
    This descriptor will be modified.
    The pointer to the unique packet which is described by  pack_desc  must be
    pointed to by  packet  .Note that the existing packet data is copied to a
    new packet, and a pointer to this packet is written back to the storage
    pointed to by  packet  .
    The name of the element must be pointed to by  name  .
    The string must be pointed to by  string  .
    If the value of  update  is TRUE, then the routine will allow an existing
    named string to be updated, otherwise the routine will fail if an update
    (rather than a create) is attempted.
    The routine returns TRUE on success, else it returns FALSE.
*/
packet_desc *pack_desc;
char **packet;
CONST char *name;
CONST char *string;
flag update;
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
	(void) fprintf (stderr, "NULL pointer passed\n");
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
	if ( (*pack_desc).element_types[elem_index] != K_VSTRING )
	{
	    (void) fprintf (stderr,
			    "Element: \"%s\" must be of type K_VSTRING\n",
			    name);
	    return (FALSE);
	}
	if (!update)
	{
	    (void) fprintf (stderr, "Element: \"%s\" already exists\n", name);
	    return (FALSE);
	}
	if ( ( copy = m_alloc (strlen (string) + 1) ) == NULL )
	{
	    m_error_notify (function_name, "copy of string");
	    return (FALSE);
	}
	(void) strcpy (copy, string);
	/*  Deallocate old string  */
	element = *packet + ds_get_element_offset (pack_desc, elem_index);
	if (*(char **) element != NULL) m_free (*(char **) element);
	*(char **) element = copy;
	return (TRUE);
/*
	break;
*/
      case IDENT_DIMENSION:
	(void) fprintf (stderr,
			"Item: \"%s\" already used for a dimension name\n",
			name);
	return (FALSE);
/*
	break;
*/
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Item: \"%s\" has multiple occurrences\n", name);
	return (FALSE);
/*
	break;
*/
      default:
	(void) fprintf (stderr,
			"Illegal return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    /*  Allocate new element type and descriptor arrays  */
    if ( ( new_element_types = (unsigned int *)
	  m_alloc ( sizeof *new_element_types *
		   (1 + (*pack_desc).num_elements) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of new element types");
	return (FALSE);
    }
    if ( ( new_element_desc = (char **)
	  m_alloc ( sizeof *new_element_desc *
		   (1 + (*pack_desc).num_elements) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of new element descriptors");
	m_free ( (char *) new_element_types );
	return (FALSE);
    }
    /*  Allocate space for new element name  */
    if ( ( new_element_desc[(*pack_desc).num_elements] =
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
	m_free (new_element_desc[(*pack_desc).num_elements]);
	m_free ( (char *) new_element_desc );
	return (FALSE);
    }
    /*  Copy over old types, descriptor pointers and packet data  */
    m_copy ( (char *) new_element_types,
	    (char *) (*pack_desc).element_types,
	    sizeof *(*pack_desc).element_types *
	    (*pack_desc).num_elements );
    m_copy ( (char *) new_element_desc,
	    (char *) (*pack_desc).element_desc,
	    sizeof *(*pack_desc).element_desc *
	    (*pack_desc).num_elements );
    m_copy (new_packet, *packet, old_packet_size);
    /*  Write in new element descriptor information  */
    new_element_types[(*pack_desc).num_elements] = K_VSTRING;
    (void) strcpy (new_element_desc[(*pack_desc).num_elements], name);
    /*  Write in new data  */
    if ( ( copy = m_alloc (strlen (string) + 1) ) == NULL )
    {
	m_error_notify (function_name, "copy of string");
	m_error_notify (function_name, "new packet");
	m_free ( (char *) new_element_types );
	m_free (new_element_desc[(*pack_desc).num_elements]);
	m_free ( (char *) new_element_desc );
	return (FALSE);
    }
    (void) strcpy (copy, string);
    element = new_packet + ds_get_element_offset (pack_desc,
						  (*pack_desc).num_elements);
    *(char **) element = copy;
    /*  Deallocate old types, descriptor pointers and packet data  */
    m_free ( (char *) (*pack_desc).element_types );
    m_free ( (char *) (*pack_desc).element_desc );
    m_free (*packet);
    /*  Link in new types, descriptor pointers and packet data  */
    (*pack_desc).element_types = new_element_types;
    (*pack_desc).element_desc = new_element_desc;
    *packet = new_packet;
    /*  Increment number of elements  */
    ++(*pack_desc).num_elements;
    return (TRUE);
}   /*  End Function ds_put_unique_named_string  */

/*PUBLIC_FUNCTION*/
flag ds_get_unique_named_value (pack_desc, packet, name, type, value)
/*  This routine will get a unique named value from a Karma general data
    structure.
    The packet descriptor to get the name from must be pointed to by  pack_desc
    The unique packet which is described by  pack_desc  must be pointed to by
    packet  .
    The name of the element must be pointed to by  name  .
    The type of the data found will be written to the storage pointed to by
    type  .If this is NULL, nothing is written here.
    The value of the data will be written to the storage pointed to by  value
    The routine returns TRUE on success, else it returns FALSE.
*/
packet_desc *pack_desc;
char *packet;
CONST char *name;
unsigned int *type;
double *value;
{
    unsigned int elem_num;
    static char function_name[] = "ds_get_unique_named_value";

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
    /*  Try to find element  */
    if ( ( elem_num = ds_f_elem_in_packet (pack_desc, name) ) >=
	(*pack_desc).num_elements )
    {
	(void) fprintf (stderr, "%s: Could not find element: \"%s\"\n",
			function_name, name);
	return (FALSE);
    }
    if (type != NULL)
    {
	*type = (*pack_desc).element_types[elem_num];
    }
    if (ds_get_element (packet + ds_get_element_offset (pack_desc, elem_num),
			(*pack_desc).element_types[elem_num], value,
			(flag *) NULL)
	!= TRUE)
    {
	(void) fprintf (stderr, "Error getting data for element: \"%s\"\n",
			name);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function ds_get_unique_named_value  */

/*PUBLIC_FUNCTION*/
char *ds_get_unique_named_string (pack_desc, packet, name)
/*  This routine will get a unique named string from a Karma general data
    structure.
    The packet descriptor to get the name from must be pointed to by  pack_desc
    The unique packet which is described by  pack_desc  must be pointed to by
    packet  .
    The name of the element must be pointed to by  name  .
    The routine returns a pointer to a dynamically allocated copy of the string
    on success, else it returns NULL.
*/
packet_desc *pack_desc;
char *packet;
CONST char *name;
{
    unsigned int elem_num;
    char *element;
    char *orig;
    char *copy;
    static char function_name[] = "ds_get_unique_named_string";

    /*  Try to find element  */
    if ( ( elem_num = ds_f_elem_in_packet (pack_desc, name) ) >=
	(*pack_desc).num_elements )
    {
	(void) fprintf (stderr, "%s: Could not find element: \"%s\"\n",
			function_name, name);
	return (NULL);
    }
    element = packet + ds_get_element_offset (pack_desc, elem_num);
    switch ( (*pack_desc).element_types[elem_num] )
    {
      case K_VSTRING:
	orig = *(char **) element;
	break;
      case K_FSTRING:
	orig = (* (FString *) element ).string;
	break;
      default:
	(void) fprintf (stderr, "Element is not a named string\n");
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
    (void) strcpy (copy, orig);
    return (copy);
}   /*  End Function ds_get_unique_named_string  */
