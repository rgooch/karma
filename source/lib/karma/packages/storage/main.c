/*LINTLIBRARY*/
/*PREFIX:"storage_"*/
/*  storage.c

    This code implements a Generic Storage Manager.

    Copyright (C) 1993,1994,1995  Richard Gooch

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

/*  This file contains all routines needed for the transfer of Tables to and
  from disc files. This implements a Generic Storage Manager.


    Written by      Richard Gooch   8-APR-1993

    Updated by      Richard Gooch   6-JAN-1995

    Last updated by Richard Gooch   7-MAY-1995: Placate gcc -Wall


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <karma.h>
#include <karma_dsxfr.h>
#include <karma_dsrw.h>
#include <karma_st.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_m.h>
#include <karma_a.h>

typedef struct
{
    unsigned int magic_number;
    multi_array *multi_desc;
    array_desc *arr_desc;
    unsigned int array_num;
    Channel channel;
    flag vm;
    flag new;
} *DataStore;

typedef struct
{
    unsigned int magic_number;
    DataStore datastore;
    /*  vector location  */
} *DataVectorInternal;

typedef struct
{
    unsigned int magic_number;
    DataStore datastore;
    /*  section location  */
    char *single_array;            /*  array start + element offset       */
    /*  Information about keys that must be processed in a special order  */
    unsigned int num_o_keys;       /*  Number of ordered keys             */
    unsigned long *o_key_lengths;  /*  Lengths of co-ordinate arrays      */
    unsigned int *o_key_indices;   /*  Dimension index of keys            */
    unsigned long **o_key_coords;  /*  Co-ordinate arrays for each key    */
    unsigned long *o_location;     /*  Count up to key length             */
    /*  Information about keys that may be processed in any order         */
    unsigned int num_u_keys;       /*  Number of unordered keys           */
    unsigned int *u_key_indices;   /*  Dimension index of keys            */
    unsigned long *u_location;     /*  Count up to dimension length       */
} *DataSectionInternal;

#define DATASTORE_DEFINED
#include <karma_storage.h>

#define DATASTORE_MAGIC_NUMBER (unsigned int) 543988743
#define DATASECTIONINTERNAL_MAGIC_NUMBER (unsigned int) 750457498

/*  Definitions for file types  */
#define FILE_TYPE_UNKNOWN (unsigned int) 0
#define FILE_TYPE_KARMA (unsigned int) 1
#define FILE_TYPE_FITS (unsigned int) 2
#define FILE_TYPE_REMOTE (unsigned int) 3
/*#define FILE_TYPE_MEMORY (unsigned int) 4*/

#define VERIFY_DATASTORE(datastore) if (datastore == NULL) \
{(void) fprintf (stderr, "NULL DataStore passed\n"); \
 a_prog_bug (function_name); } \
if (datastore->magic_number != DATASTORE_MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid DataStore object\n"); \
 a_prog_bug (function_name); }
#define VERIFY_DATASECTION(section) if (section == NULL) \
{(void) fprintf (stderr, "NULL DataSection passed\n"); \
 a_prog_bug (function_name); } \
if (section->priv == NULL) \
{(void) fprintf (stderr, "NULL priv pointer\n"); \
 a_prog_bug (function_name); } \
if (section->priv->magic_number != DATASECTIONINTERNAL_MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid private structure\n"); \
 a_prog_bug (function_name); }


/*  Private functions  */
STATIC_FUNCTION (DataStore alloc_datastore, () );
STATIC_FUNCTION (DataSection alloc_datasection, (DataStore datastore) );
STATIC_FUNCTION (void dealloc_datastore, (DataStore datastore) );
STATIC_FUNCTION (unsigned int determine_filetype,
		 (CONST char *name, char filename[SM_MAX_PATHNAME_LENGTH],
		  char structname[SM_MAX_PATHNAME_LENGTH],
		  char hostname[SM_MAX_PATHNAME_LENGTH]) );
STATIC_FUNCTION (flag test_for_list, (packet_desc *pack_desc) );
STATIC_FUNCTION (void compute_section_from_location, (DataSection section) );
STATIC_FUNCTION (flag find_next_section, (DataSection section) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
DataStore storage_open (CONST char *pathname, flag read_only, flag reshape)
/*  This routine will open a file and get the primary DataStore.
    The filename to open must be pointed to by  pathname  .The following
    filename extensions are recognised:
        .kf    Karma data structure (native AIPS++ format: default)
        .fits  Binary FITS
    If no extension is specifed, ".kf" is assumed.
    All data formats are machine independent.
    If the value of  read_only  is TRUE, then the DataStore as stored on disc
    cannot be modified, else changes to sections will be stored back in the
    file.
    If the DataStore will be reshaped (ie. data added or removed) then the
    value of  reshape  must be TRUE.
    The routine returns a DataStore on success,
    else it returns NULL (and displays a message on the standard error).
*/
{
    DataStore datastore;
    multi_array *multi_desc;
    char filename[SM_MAX_PATHNAME_LENGTH];
    char structname[SM_MAX_PATHNAME_LENGTH];
    char hostname[SM_MAX_PATHNAME_LENGTH];
    static char function_name[] = "storage_open";

    if (pathname == NULL)
    {
	(void) fprintf (stderr, "NULL pathname passed\n");
	a_prog_bug (function_name);
    }
    if (reshape)
    {
	(void) fprintf (stderr, "Reshaping not yet supported\n");
	return (NULL);
    }
    if ( ( datastore = alloc_datastore () ) == NULL )
    {
	m_error_notify (function_name, "DataStore");
	return (NULL);
    }
    switch ( determine_filetype (pathname, filename, structname, hostname) )
    {
      case FILE_TYPE_UNKNOWN:
	(void) fprintf (stderr, "File: \"%s\" is of unknown type\n",
			pathname);
	dealloc_datastore (datastore);
	return (NULL);
/*
	break;
*/
      case FILE_TYPE_KARMA:
	/*  Quick and dirty: read  */
	if (hostname[0] != '\0')
	{
	    (void) fprintf (stderr,
			    "Remote Storage Manager not yet implemented\n");
	    dealloc_datastore (datastore);
	    return (NULL);
	}
	if ( ( multi_desc = dsxfr_get_multi (pathname, FALSE, K_CH_MAP_NEVER,
					     TRUE) )
	    == NULL )
	{
	    dealloc_datastore (datastore);
	    return (NULL);
	}
	datastore->multi_desc = multi_desc;
	switch ( ds_f_array_name (multi_desc, structname, (char **) NULL,
				  &datastore->array_num) )
	{
	  case IDENT_GEN_STRUCT:
	    /*  Perfect  */
	    break;
	  case IDENT_NOT_FOUND:
	    (void) fprintf (stderr, "Structure name: \"%s\" not found\n",
			    structname);
	    dealloc_datastore (datastore);
	    return (NULL);
/*
	    break;
*/
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr,
			    "Multiple occurrences of structure name: \"%s\" found\n",
			    structname);
	    dealloc_datastore (datastore);
	    return (NULL);
/*
	    break;
*/
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value from  ds_f_array_name\n");
	    a_prog_bug (function_name);
	    break;
	}
	datastore->vm = TRUE;
	datastore->new = FALSE;
	break;
      default:
	(void) fprintf (stderr,
			"File type for: \"%s\" is not supported yet\n",
			pathname);
	dealloc_datastore (datastore);
	return (NULL);
/*
	break;
*/
    }
    return (datastore);
}   /*  End Function storage_open  */

/*PUBLIC_FUNCTION*/
DataStore storage_create (CONST char *pathname, packet_desc *top_pack_desc)
/*  This routine will create a DataStore. The DataStore may be associated with
    a disc file if permanent storage is required.
    The filename to create must be pointed to by  pathname  .If this is NULL,
    no file is created, and the DataStore will exist only in virtual memory.
    By specifying a particular filename extension, a different storage format
    will be used. If no extension is specifed, ".kf" is assumed. See the
    documentation on the  storage_open  routine for a list of supported formats
    The top level packet descriptor which describes the storage layout required
    must be pointed to by  top_pack_desc  .This may be subsequently deallocated
    by the caller.
    The routine returns a DataStore on success,
    else it returns NULL (and displays a message on the standard error).
*/
{
    DataStore datastore;
    flag rename_file;
    char *tilde_filename;
    multi_array *multi_desc;
    char filename[SM_MAX_PATHNAME_LENGTH];
    char structname[SM_MAX_PATHNAME_LENGTH];
    char hostname[SM_MAX_PATHNAME_LENGTH];
    extern char *sys_errlist[];
    static char function_name[] = "storage_create";

    if ( ( top_pack_desc = ds_copy_desc_until (top_pack_desc, NULL) ) == NULL )
    {
	m_error_notify (function_name, "copy of data structure description");
	return (NULL);
    }
    if ( ( datastore = alloc_datastore () ) == NULL )
    {
	m_error_notify (function_name, "DataStore");
	ds_dealloc_packet (top_pack_desc, NULL);
	return (NULL);
    }
    if (pathname == NULL)
    {
	/*  Create VM data structure  */
	if ( ( multi_desc = ds_alloc_multi (1) ) == NULL )
	{
	    dealloc_datastore (datastore);
	    ds_dealloc_packet (top_pack_desc, NULL);
	    return (NULL);
	}
	datastore->multi_desc = multi_desc;
	datastore->array_num = 0;
	multi_desc->headers[0] = top_pack_desc;
	if ( ( multi_desc->data[0] = ds_alloc_data (top_pack_desc, TRUE,TRUE) )
	    == NULL )
	{
	    dealloc_datastore (datastore);
	    return (NULL);
	}
	datastore->vm = TRUE;
	datastore->new = TRUE;
	return (datastore);
    }
    /*  A file must be associated with the DataStore  */
    switch ( determine_filetype (pathname, filename, structname, hostname) )
    {
      case FILE_TYPE_UNKNOWN:
	(void) fprintf (stderr, "File: \"%s\" is of unknown type\n",
			pathname);
	dealloc_datastore (datastore);
	ds_dealloc_packet (top_pack_desc, NULL);
	return (NULL);
/*
	break;
*/
      case FILE_TYPE_KARMA:
	/*  Quick and dirty: create file and VM data structure  */
	if (access (filename, F_OK) == 0)
	{
	    /*  Old file exists  */
	    rename_file = TRUE;
	}
	else
	{
	    /*  Error accessing old file  */
	    if (errno != ENOENT)
	    {
		/*  Error, not simply missing file  */
		(void) fprintf (stderr, "Error accessing file: \"%s\"\t%s\n",
				filename, sys_errlist[errno]);
		dealloc_datastore (datastore);
		ds_dealloc_packet (top_pack_desc, NULL);
		return (NULL);
	    }
	    /*  File does not exist  */
	    rename_file = FALSE;
	}
	if (rename_file)
	{
	    if ( ( tilde_filename = m_alloc (strlen (filename) + 2) ) == NULL )
	    {
		m_error_notify (function_name, "tilde filename");
		dealloc_datastore (datastore);
		ds_dealloc_packet (top_pack_desc, NULL);
		return (NULL);
	    }
	    (void) strcpy (tilde_filename, filename);
	    (void) strcat (tilde_filename, "~");
	    if (rename (filename, tilde_filename) != 0)
	    {
		(void) fprintf (stderr, "Error renaming file: \"%s\"\t%s\n",
				filename, sys_errlist[errno]);
		m_free (tilde_filename);
		dealloc_datastore (datastore);
		ds_dealloc_packet (top_pack_desc, NULL);
		return (NULL);
	    }
	    m_free (tilde_filename);
	}
	if ( ( datastore->channel = ch_open_file (filename, "w") ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Error opening file: \"%s\" for output\t%s\n",
			    filename, sys_errlist[errno]);
	    m_free (filename);
	    ds_dealloc_packet (top_pack_desc, NULL);
	    return (FALSE);
	}
	if ( ( multi_desc = ds_alloc_multi (1) ) == NULL )
	{
	    (void) ch_close (datastore->channel);
	    datastore->channel = NULL;
	    dealloc_datastore (datastore);
	    ds_dealloc_packet (top_pack_desc, NULL);
	    return (NULL);
	}
	datastore->multi_desc = multi_desc;
	datastore->array_num = 0;
	multi_desc->headers[0] = top_pack_desc;
	if ( ( multi_desc->data[0] = ds_alloc_data (top_pack_desc, TRUE,
						    TRUE) )
	    == NULL )
	{
	    (void) ch_close (datastore->channel);
	    datastore->channel = NULL;
	    dealloc_datastore (datastore);
	    return (NULL);
	}
	datastore->vm = TRUE;
	datastore->new = TRUE;
	break;
      default:
	(void) fprintf (stderr,
			"File type for: \"%s\" is not supported yet\n",
			pathname);
	dealloc_datastore (datastore);
	ds_dealloc_packet (top_pack_desc, NULL);
	return (NULL);
/*
	break;
*/
    }
    return (datastore);
}   /*  End Function storage_create  */

/*PUBLIC_FUNCTION*/
flag storage_close (DataStore datastore)
/*  This routine will close a DataStore, flushing any buffers which have not
    yet been saved and closing the associated file.
    The DataStore must be given by  datastore  .
    The routine returns TRUE on success,
    else it returns FALSE (and displays a message on the standard error).
*/
{
    static char function_name[] = "storage_close";

    VERIFY_DATASTORE (datastore);
    dealloc_datastore (datastore);
    return (TRUE);
}   /*  End Function storage_close  */

/*PUBLIC_FUNCTION*/
CONST char *storage_get_one_value (DataStore datastore, CONST char *value_name,
				   unsigned int *type, unsigned int num_keys,
				   CONST char **key_names, double *key_values,
				   unsigned int *errcode)
/*  This routine will get a single named data value in a DataStore.
    The DataStore must be given by  datastore  .
    The name of the value must be pointed to by  value_name  .
    The type of the value will be written to the storage pointed to by  type  .
    The number of keys used to uniquely identify the value must be given by
    num_keys  .
    The array of key names must be pointed to by  key_names  .
    The array of key values must be pointed to by  key_values  .
    On error, the routine writes the error code to the storage pointed to by
    errcode  .
    The routine returns a pointer to the value on success, else it returns NULL
*/
{
    unsigned int parent_type;
    unsigned int index;
    char *top_packet;
    char *parent_desc;
    char *parent;
    packet_desc *top_pack_desc;
    multi_array *multi_desc;
    static char function_name[] = "storage_get_one_value";

    VERIFY_DATASTORE (datastore);
    if (!datastore->vm)
    {
	(void) fprintf (stderr,
			"Operation on non-VM DataStore not implemented\n");
	a_prog_bug (function_name);
    }
    multi_desc = datastore->multi_desc;
    top_pack_desc = multi_desc->headers[datastore->array_num];
    top_packet = multi_desc->data[datastore->array_num];
    switch ( ds_get_handle_in_packet (top_pack_desc, top_packet, value_name,
				      key_names, key_values, num_keys,
				      &parent_desc, &parent, &parent_type,
				      &index) )
    {
      case IDENT_NOT_FOUND:
	*errcode = STORAGE_ERR_VALUE_NOT_FOUND;
	return (NULL);
/*
	break;
*/
      case IDENT_MULTIPLE:
	*errcode = STORAGE_ERR_DUPLICATE_NAME;
	return (NULL);
/*
	break;
*/
      case IDENT_DIMENSION:
	*errcode = STORAGE_ERR_DIMENSION_NOT_VALUE;
	return (NULL);
/*
	break;
*/
      case IDENT_ELEMENT:
	/*  Yowza!  */
	break;
      default:
	(void) fprintf (stderr,
			"Illegal return value from:ds_get_handle_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    if (parent_type == K_ARRAY)
    {
	(void) fprintf (stderr, "Element found but parent is an array!\n");
	a_prog_bug (function_name);
    }
    *type = (*(packet_desc *) parent_desc).element_types[index];
    return (ds_get_element_offset ( (packet_desc *) parent_desc, index ) +
	    parent);
}   /*  End Function storage_get_one_value  */

/*PUBLIC_FUNCTION*/
flag storage_put_one_value (DataStore datastore, CONST char *value_name,
			    unsigned int type, char *value,
			    unsigned int num_keys, CONST char **key_names,
			    double *key_values, unsigned int *errcode)
/*  This routine will put a single named data value in a DataStore.
    The DataStore must be given by  datastore  .
    The name of the value must be pointed to by  value_name  .
    The type of the value must be given by  type  .This must match the type of
    the named value in the DataStore.
    The value data must be pointed to by  value  .
    The number of keys used to uniquely identify the value must be given by
    num_keys  .
    The array of key names must be pointed to by  key_names  .
    The array of key values must be pointed to by  key_values  .
    On error, the routine writes the error code to the storage pointed to by
    errcode  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int parent_type;
    unsigned int index;
    char *top_packet;
    char *parent_desc;
    char *parent;
    packet_desc *top_pack_desc;
    multi_array *multi_desc;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "storage_put_one_value";

    VERIFY_DATASTORE (datastore);
    if (!datastore->vm)
    {
	(void) fprintf (stderr,
			"Operation on non-VM DataStore not implemented\n");
	a_prog_bug (function_name);
    }
    multi_desc = datastore->multi_desc;
    top_pack_desc = multi_desc->headers[datastore->array_num];
    top_packet = multi_desc->data[datastore->array_num];
    switch ( ds_get_handle_in_packet (top_pack_desc, top_packet, value_name,
				      key_names, key_values, num_keys,
				      &parent_desc, &parent, &parent_type,
				      &index) )
    {
      case IDENT_NOT_FOUND:
	*errcode = STORAGE_ERR_VALUE_NOT_FOUND;
	return (FALSE);
/*
	break;
*/
      case IDENT_MULTIPLE:
	*errcode = STORAGE_ERR_DUPLICATE_NAME;
	return (FALSE);
/*
	break;
*/
      case IDENT_DIMENSION:
	*errcode = STORAGE_ERR_DIMENSION_NOT_VALUE;
	return (FALSE);
/*
	break;
*/
      case IDENT_ELEMENT:
	/*  Yowza!  */
	break;
      default:
	(void) fprintf (stderr,
			"Illegal return value from:ds_get_handle_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    if (parent_type == K_ARRAY)
    {
	(void) fprintf (stderr, "Element found but parent is an array!\n");
	a_prog_bug (function_name);
    }
    if (type != (*(packet_desc *) parent_desc).element_types[index])
    {
	(void) fprintf (stderr,
			"Declared type: %u does not match predefined data type: %u\n",
			type,
			(*(packet_desc *) parent_desc).element_types[index]);
	a_prog_bug (function_name);
    }
    m_copy (ds_get_element_offset ( (packet_desc *) parent_desc, index ) +
	    parent, value, host_type_sizes[type]);
    return (TRUE);
}   /*  End Function storage_put_one_value  */

/*PUBLIC_FUNCTION*/
DataSection storage_get_one_section (DataStore datastore,
				     CONST char *value_name, flag auto_update,
				     unsigned int num_axis_keys,
				     CONST char **axis_key_names,
				     unsigned long *axis_key_lengths,
				     double **axis_key_coordinates,
				     unsigned int num_loc_keys,
				     CONST char **loc_key_names,
				     double *loc_key_values,
				     unsigned int *errcode)
/*  This routine will get a single section of data values in a DataStore. If
    the DataStore is opened read only, the section data should not be modified,
    else a segmentation fault could occur.
    The DataStore must be given by  datastore  .
    The name of the value must be pointed to by  value_name  .
    If the section data should be automatically written back to the DataStore
    then the value of  auto_update  must be TRUE. If this is FALSE, the
    storage_flush_section  routine must be used to update the section to the
    DataStore.
    The number of keys which will be assigned to section axes must be given by
    num_axis_keys  .
    The array of section axes key names must be pointed to by  axis_key_names
    The number of co-ordinates along each axis key must be pointed to by
    axis_key_lengths  .
    The arrays of co-ordinates for each axis key must be pointed to by
    axis_key_coordinates  .The co-ordinates for each axis must be increasing.
    The number of keys used to uniquely identify (locate) the section must be
    given by  num_loc_keys  .
    The array of location key names must be pointed to by  loc_key_names  .
    The array of location key values must be pointed to by  loc_key_values  .
    On error, the routine writes the error code to the storage pointed to by
    errcode  .
    The routine returns a DataSection on success, else it returns NULL.
*/
{
    DataSection section;
    char *top_packet;
    packet_desc *top_pack_desc;
    multi_array *multi_desc;
    static char function_name[] = "storage_get_one_section";

    VERIFY_DATASTORE (datastore);
    if (!datastore->vm)
    {
	(void) fprintf (stderr,
			"Operation on non-VM DataStore not implemented\n");
	a_prog_bug (function_name);
    }
    multi_desc = datastore->multi_desc;
    top_pack_desc = multi_desc->headers[datastore->array_num];
    top_packet = multi_desc->data[datastore->array_num];
    /*  Disable this for now. Maybe eventually I can remove the code. Some
	thought will be needed.  */
#ifdef dummy
    if (datastore->new)
    {
	(void) fprintf (stderr, "DataStore must not be a newly created one\n");
	a_prog_bug (function_name);
    }
#endif
    /*  This is an ugly hack which will have to be changed later  */
    datastore->new = TRUE;
    section = storage_create_section (datastore, value_name, num_axis_keys,
				      axis_key_names, axis_key_lengths,
				      axis_key_coordinates, num_loc_keys,
				      loc_key_names, loc_key_values, errcode);
    datastore->new = FALSE;
    return (section);
}   /*  End Function storage_get_one_section  */

/*PUBLIC_FUNCTION*/
DataSection storage_define_iterator (DataStore datastore,
				     CONST char *value_name, flag auto_update,
				     unsigned int num_axis_keys,
				     CONST char **axis_key_names,
				     unsigned long *axis_key_lengths,
				     double **axis_key_coordinates,
				     unsigned int num_ordered_keys,
				     CONST char **ordered_key_names,
				     unsigned long *ordered_key_lengths,
				     double **ordered_key_coordinates,
				     unsigned int *errcode)
/*  This routine will define a section of data values in a DataStore, which
    can be iterated through part of the DataStore.
    The section may be subsequently moved to another position in the DataStore
    by using the  storage_goto_next_section  routine.
    If the DataStore is opened read only, the section data should not be
    modified, else a segmentation fault could occur.
    The DataStore must be given by  datastore  .
    The name of the value must be pointed to by  value_name  .
    If the section data should be automatically written back to the DataStore
    then the value of  auto_update  must be TRUE. If this is FALSE, the
    storage_flush_section  routine must be used to update the section to the
    DataStore.
    The number of keys which will be assigned to section axes must be given by
    num_axis_keys  .
    The array of section axes key names must be pointed to by  axis_key_names
    The number of co-ordinates along each axis key must be pointed to by
    axis_key_lengths  .
    The arrays of co-ordinates for each axis key must be pointed to by
    axis_key_coordinates  .The co-ordinates for each axis must be increasing.
    If either  axis_key_lengths  or  axis_key_coordinates  are NULL, the
    routine will choose an optimal section size.
    The remaining keys in the DataStore will be accessed in some pattern. If
    some of these keys must be accessed in a particular pattern, this must be
    specified. Specifying an order may result in greatly reduced efficiency.
    The number of keys that must be accessed in a specified pattern must be
    given by  num_ordered_keys  .
    The array of ordered key names must be pointed to by  ordered_key_names
    The number of co-ordinates along each ordered key must be pointed to by
    ordered_key_lengths  .
    The arrays of co-ordinates for each ordered key must be pointed to by
    ordered_key_coordinates  .
    On error, the routine writes the error code to the storage pointed to by
    errcode  .
    The location of the section in the DataStore is indeterminate. Processing
    of all data in the DataStore in sections requires the use of the
    storage_get_next_section  routine.
    The routine returns a DataSection on success, else it returns NULL.
*/
{
    DataSection section;
    DataSectionInternal internal;
    iarray iarr;
    flag in_one_array = TRUE;
    flag found_key;
    unsigned int key_count, dim_count, u_key_count;
    unsigned int list_key_count;
    unsigned int index, value_index;
    unsigned int coord_count, length;
    unsigned int elem_count;
    unsigned int num_unordered_keys;
    uaddr off;
    double coord;
    uaddr **offsets;
    char *var_desc;
    char *single_array;
    char *top_packet;
    unsigned int *ordered_key_indices;
    packet_desc *top_pack_desc;
    packet_desc *value_pack_desc = NULL; /*Initialised to keep compiler happy*/
    multi_array *multi_desc;
    array_desc *prev_arr_desc;
    array_desc *arr_desc = NULL;  /*  Initialised to keep compiler happy  */
    array_desc *single_arr_desc = NULL;
    dim_desc *dim;
    static char function_name[] = "storage_define_iterator";

    VERIFY_DATASTORE (datastore);
    FLAG_VERIFY (auto_update);
    if ( (value_name == NULL) || (axis_key_names == NULL) ||
	(ordered_key_names == NULL) || (ordered_key_lengths == NULL) ||
	(ordered_key_coordinates == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    *errcode = STORAGE_ERR_UNDEFINED;
    if (!datastore->vm)
    {
	(void) fprintf (stderr,
			"Only virtual memory DataStore implemented yet\n");
	return (NULL);
    }
    multi_desc = datastore->multi_desc;
    top_pack_desc = multi_desc->headers[datastore->array_num];
    top_packet = multi_desc->data[datastore->array_num];
    /*  Get parent packet descriptor for data value  */
    switch ( ds_f_name_in_packet (top_pack_desc, value_name,
				  &var_desc, &value_index) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr, "Data Value name: \"%s\" not found\n",
			value_name);
	return (NULL);
/*
	break;
*/
      case IDENT_DIMENSION:
	(void) fprintf (stderr,
			"Data Value name: \"%s\" must not be a dimension\n",
			value_name);
	return (NULL);
/*
	break;
*/
      case IDENT_ELEMENT:
	/*  Fine  */
	value_pack_desc = (packet_desc *) var_desc;
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Data Value name: \"%s\" found multiple times\n",
			value_name);
	a_prog_bug (function_name);
	break;
      default:
	(void) fprintf (stderr,
			"Illegal return value from: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    /*  Make sure no more than one key is a list key  */
    list_key_count = 0;
    for (key_count = 0, prev_arr_desc = NULL; key_count < num_axis_keys;
	 ++key_count)
    {
	if (axis_key_names[key_count] == NULL)
	{
	    (void) fprintf (stderr, "No name for axis key: %u\n", key_count);
	    a_prog_bug (function_name);
	}
	switch ( ds_f_name_in_packet (top_pack_desc, axis_key_names[key_count],
				      &var_desc, &index) )
	{
	  case IDENT_NOT_FOUND:
	    (void) fprintf (stderr, "Key: \"%s\" not found\n",
			    axis_key_names[key_count]);
	    return (NULL);
/*
	    break;
*/
	  case IDENT_DIMENSION:
	    arr_desc = (array_desc *) var_desc;
	    /*  Test to see if the same array is always returned  */
	    if (single_arr_desc == NULL) single_arr_desc = arr_desc;
	    if (prev_arr_desc == NULL)
	    {
		prev_arr_desc = arr_desc;
		if (value_pack_desc != arr_desc->packet)
		{
		    /*  Data value is not in this array  */
		    in_one_array = FALSE;
		}
	    }
	    else
	    {
		if (prev_arr_desc != arr_desc)
		{
		    in_one_array = FALSE;
		}
	    }
	    dim = arr_desc->dimensions[index];
	    /*  Verify co-ordinates match  */
	    /*  Fine  */
	    break;
	  case IDENT_ELEMENT:
	    if (list_key_count++ > 0)
	    {
		(void) fprintf (stderr, "Only one list key allowed\n");
		return (NULL);
	    }
	    /*  Data is not all in one n-dimensional array  */
	    in_one_array = FALSE;
	    break;
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr, "Key: \"%s\" found multiple times\n",
			    axis_key_names[key_count]);
	    a_prog_bug (function_name);
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value from: ds_f_name_in_packet\n"
			    );
	    a_prog_bug (function_name);
	    break;
	}
    }
    if ( !in_one_array || (single_arr_desc == NULL) )
    {
	(void) fprintf (stderr, "Single array only supported at this time\n");
	return (NULL);
    }
    if ( !ds_compute_array_offsets (single_arr_desc) )
    {
	(void) fprintf (stderr, "Error computing array offsets\n");
	return (NULL);
    }
    offsets = single_arr_desc->offsets;
    /*  At this point the axis keys have been grokked. Time to meditate on the
	order keys.  */
    if ( ( ordered_key_indices = (unsigned int *)
	  m_alloc (num_ordered_keys * sizeof *ordered_key_indices) ) == NULL )
    {
	m_error_notify (function_name, "index array");
	return (NULL);
    }
    for (key_count = 0, prev_arr_desc = NULL;
	 key_count < num_ordered_keys; ++key_count)
    {
	if (ordered_key_names[key_count] == NULL)
	{
	    (void) fprintf (stderr, "No name for order key: %u\n",
			    key_count);
	    a_prog_bug (function_name);
	}
	switch ( ds_f_name_in_packet (top_pack_desc,
				      ordered_key_names[key_count],
				      &var_desc, &index) )
	{
	  case IDENT_NOT_FOUND:
	    (void) fprintf (stderr, "Key: \"%s\" not found\n",
			    ordered_key_names[key_count]);
	    m_free ( (char *) ordered_key_indices );
	    return (NULL);
/*
	    break;
*/
	  case IDENT_DIMENSION:
	    arr_desc = (array_desc *) var_desc;
	    /*  Test to see if the same array is always returned  */
	    if (prev_arr_desc == NULL)
	    {
		prev_arr_desc = arr_desc;
		if (value_pack_desc != arr_desc->packet)
		{
		    /*  Data value is not in this array  */
		    in_one_array = FALSE;
		}
	    }
	    else
	    {
		if (prev_arr_desc != arr_desc)
		{
		    in_one_array = FALSE;
		}
	    }
	    /*  Fine  */
	    ordered_key_indices[key_count] = index;
	    break;
	  case IDENT_ELEMENT:
	    if (list_key_count++ > 0)
	    {
		(void) fprintf (stderr, "Only one list key allowed\n");
		m_free ( (char *) ordered_key_indices );
		return (NULL);
	    }
	    /*  Data is not all in one n-dimensional array  */
	    in_one_array = FALSE;
	    break;
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr, "Key: \"%s\" found multiple times\n",
			    ordered_key_names[key_count]);
	    a_prog_bug (function_name);
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value from: ds_f_name_in_packet\n"
			    );
	    a_prog_bug (function_name);
	    break;
	}
    }
    if (!in_one_array)
    {
	(void) fprintf (stderr, "Single array only supported at this time\n");
	m_free ( (char *) ordered_key_indices );
	return (NULL);
    }
    if ( (prev_arr_desc != NULL) && (prev_arr_desc != single_arr_desc) )
    {
	(void) fprintf (stderr, "Single array only supported at this time\n");
	m_free ( (char *) ordered_key_indices );
	return (NULL);
    }
    /*  Simple, n-dimensional array: yay!  */
    if (num_axis_keys + num_ordered_keys > single_arr_desc->num_dimensions)
    {
	(void) fprintf (stderr, "Too many axis/order keys for array\n");
	m_free ( (char *) ordered_key_indices );
	return (NULL);
    }
    /*  Locate section  */
    /*  First find array in top packet  */
    for (elem_count = 0, single_array = NULL;
	 elem_count < top_pack_desc->num_elements;
	 ++elem_count)
    {
	if (top_pack_desc->element_types[elem_count] != K_ARRAY) continue;
	if (single_arr_desc ==
	    (array_desc *) top_pack_desc->element_desc[elem_count])
	{
	    single_array = top_packet + ds_get_element_offset (top_pack_desc,
							       elem_count);
	    single_array = *(char **) single_array;
	    elem_count = top_pack_desc->num_elements;
	}
    }
    if (single_array == NULL)
    {
	(void) fprintf (stderr, "Couldn't find right array!\n");
	m_free ( (char *) ordered_key_indices );
	return (NULL);
    }
    /*  Create a section and start filling it in  */
    if ( ( section = alloc_datasection (datastore) ) == NULL )
    {
	m_error_notify (function_name, "DataSection");
	m_free ( (char *) ordered_key_indices );
	return (NULL);
    }
    internal = section->priv;
    internal->num_o_keys = num_ordered_keys;
    internal->o_key_indices = ordered_key_indices;
    internal->single_array = single_array;
    internal->single_array += ds_get_element_offset (value_pack_desc,
						     value_index);
    internal->num_u_keys = single_arr_desc->num_dimensions - num_axis_keys;
    internal->num_u_keys -= internal->num_u_keys;
    if (internal->num_u_keys > 0)
    {
	if ( ( internal->u_key_indices = (unsigned int *)
	      m_alloc (sizeof *internal->u_key_indices *
		       internal->num_u_keys) ) == NULL )
	{
	    m_error_notify (function_name, "unordered key indices");
	    storage_free_section (section);
	    return (NULL);
	}
    }
    else
    {
	internal->u_key_indices = NULL;
    }
    if ( ( iarr = (iarray) m_alloc (sizeof *iarr) ) == NULL )
    {
	m_error_notify (function_name, "Intelligent Array");
	storage_free_section (section);
	return (NULL);
    }
    m_clear ( (char *) iarr, sizeof *iarr );
    section->iarray = iarr;
    iarr->multi_desc = datastore->multi_desc;
    ++iarr->multi_desc->attachments;
    if ( ( iarr->offsets = (uaddr **)
	  m_alloc (num_axis_keys * sizeof *iarr->offsets) ) == NULL )
    {
	m_error_notify (function_name, "array of offset array pointers");
	storage_free_section (section);
	return (NULL);
    }
    if ( ( iarr->lengths = (unsigned long *)
	  m_alloc (num_axis_keys * sizeof *iarr->lengths) ) == NULL )
    {
	m_error_notify (function_name, "array of lengths");
	storage_free_section (section);
	return (NULL);
    }
    if ( ( iarr->contiguous = (flag *)
	  m_alloc (num_axis_keys * sizeof *iarr->contiguous) ) == NULL )
    {
	m_error_notify (function_name, "array of lengths");
	storage_free_section (section);
	return (NULL);
    }
    if ( ( iarr->orig_dim_indices = (unsigned int *)
	  m_alloc (num_axis_keys * sizeof *iarr->orig_dim_indices) )
	== NULL )
    {
	m_error_notify (function_name, "array of dimension indices");
	storage_free_section (section);
	return (NULL);
    }
    /*  Loop through axis keys  */
    for (key_count = 0; key_count < num_axis_keys; ++key_count)
    {
	if ( ( index = ds_f_dim_in_array (single_arr_desc,
					  axis_key_names[key_count]) )
	    >= single_arr_desc->num_dimensions )
	{
	    (void) fprintf (stderr, "Axis: \"%s\" not found\n",
			    axis_key_names[key_count]);
	    a_prog_bug (function_name);
	}
	dim = arr_desc->dimensions[index];
	/*  For VM case, make undefined section size equal to whole section
	    size. This will break for non-VM code.  */
	length = (axis_key_lengths ==
		  NULL) ? dim->length : axis_key_lengths[key_count];
	if ( ( iarr->offsets[key_count] = (unsigned long *)
	      m_alloc (sizeof **iarr->offsets * length) )
	    == NULL )
	{
	    m_error_notify (function_name, "offset array");
	    storage_free_section (section);
	    return (NULL);
	}
	iarr->lengths[key_count] = length;
	for (coord_count = 0; coord_count < length; ++coord_count)
	{
	    if (axis_key_coordinates == NULL)
	    {
		off = coord_count;
	    }
	    else
	    {
		coord = axis_key_coordinates[key_count][coord_count];
		off = ds_get_coord_num (dim, coord, SEARCH_BIAS_CLOSEST);
	    }
	    off = offsets[index][off];
	    iarr->offsets[key_count][coord_count] = off;
	}
	iarr->contiguous[key_count] = FALSE;
	iarr->orig_dim_indices[key_count] = index;
    }
    iarr->data = NULL;
    iarr->top_pack_desc = top_pack_desc;
    iarr->top_packet = multi_desc->data + datastore->array_num;
    iarr->arr_desc = single_arr_desc;
    iarr->array_num = datastore->array_num;
    iarr->boundary_width = 0;
    iarr->elem_index = value_index;
    iarr->num_dim = num_axis_keys;
    iarr->restrictions = NULL;
    /*  Do more work  */
    if ( ( internal->o_key_lengths = (unsigned long *)
	  m_alloc (num_ordered_keys * sizeof *internal->o_key_lengths) )
	== NULL )
    {
	m_error_notify (function_name, "array of ordered key lengths");
	storage_free_section (section);
	return (NULL);
    }
    if ( ( internal->o_key_coords = (unsigned long **)
	  m_alloc (num_ordered_keys * sizeof *internal->o_key_coords) )
	== NULL )
    {
	m_error_notify (function_name, "array of co-ordinate array pointers");
	storage_free_section (section);
	return (NULL);
    }
    if ( ( internal->o_location = (unsigned long *)
	  m_alloc (sizeof *internal->o_location * num_ordered_keys) )
	== NULL )
    {
	m_error_notify (function_name, "array of ordered co-ordinates");
	storage_free_section (section);
	return (NULL);
    }
    /*  Initialise ordered location  */
    m_clear ( (char *) internal->o_location, sizeof *internal->o_location *
	     num_ordered_keys );
    num_unordered_keys = datastore->arr_desc->num_dimensions - num_axis_keys - num_ordered_keys;
    if ( ( internal->u_location = (unsigned long *)
	  m_alloc (sizeof *internal->u_location * num_unordered_keys) )
	== NULL )
    {
	m_error_notify (function_name, "array of ordered co-ordinates");
	storage_free_section (section);
	return (NULL);
    }
    /*  Initialise unordered location  */
    m_clear ( (char *) internal->u_location, sizeof *internal->u_location *
	     num_unordered_keys );
    /*  Fill in rest of the ordered key information  */
    for (key_count = 0; key_count < num_ordered_keys; ++key_count)
    {
	index = ordered_key_indices[key_count];
	internal->o_key_lengths[key_count] = ordered_key_lengths[key_count];
	dim = single_arr_desc->dimensions[index];
	for (coord_count = 0; coord_count < ordered_key_lengths[key_count];
	     ++coord_count)
	{
	    coord = ordered_key_coordinates[key_count][coord_count];
	    off = offsets[index][ds_get_coord_num (dim, coord,
						   SEARCH_BIAS_CLOSEST)];
	    internal->o_key_coords[key_count][coord_count] = off;
	}
    }
    /*  Fill in the unordered key information  */
    for (dim_count = 0, u_key_count = 0;
	 dim_count < single_arr_desc->num_dimensions;
	 ++dim_count)
    {
	for (key_count = 0, found_key = FALSE;
	     (key_count < num_axis_keys) && !found_key;
	     ++key_count)
	{
	    if (dim_count == iarr->orig_dim_indices[key_count])
	    {
		found_key = TRUE;
	    }
	}
	for (key_count = 0, found_key = FALSE;
	     (key_count < internal->num_u_keys) && !found_key;
	     ++key_count)
	{
	    if (dim_count == internal->o_key_indices[key_count])
	    {
		found_key = TRUE;
	    }
	}
	if (found_key) continue;
	if (u_key_count >= internal->num_u_keys)
	{
	    (void) fprintf (stderr, "Inconsistent number of unordered keys\n");
	    a_prog_bug (function_name);
	}
	internal->u_key_indices[u_key_count] = dim_count;
    }
    /*  YES! The structure is filled in. Now get the first section  */
    compute_section_from_location (section);
    return (section);
}   /*  End Function storage_define_iterator  */

/*PUBLIC_FUNCTION*/
flag storage_goto_next_section (DataSection section)
/*  This routine will move the window (section) that a DataSection has on the
    data in a DataStore to the next section. The location of the next section
    in the DataStore is indeterminate. Iterating through the sections in this
    fashion is guaranteed to be the most efficient way of processing the data.
    This routine only updates the section location information.
    The DataSection must be given by  section  .
    The routine returns TRUE if the DataSection was moved to the next section,
    else it returns FALSE indicting that there are no more sections to process.
*/
{
/*
    static char function_name[] = "storage_goto_next_section";
*/

    if ( !find_next_section (section) ) return (FALSE);
    compute_section_from_location (section);
    return (TRUE);
}   /*  End Function storage_goto_next_section  */

/*PUBLIC_FUNCTION*/
DataSection storage_create_section (DataStore datastore,
				    CONST char *value_name,
				    unsigned int num_axis_keys,
				    CONST char **axis_key_names,
				    unsigned long *axis_key_lengths,
				    double **axis_key_coordinates,
				    unsigned int num_loc_keys,
				    CONST char **loc_key_names,
				    double *loc_key_values,
				    unsigned int *errcode)
/*  This routine will create a single section of data values in a DataStore.
    The DataStore must be given by  datastore  .
    The name of the value must be pointed to by  value_name  .
    The number of keys which will be assigned to section axes must be given by
    num_axis_keys  .
    The array of section axes key names must be pointed to by  axis_key_names
    The number of co-ordinates along each axis key must be pointed to by
    axis_key_lengths  .
    The arrays of co-ordinates for each axis key must be pointed to by
    axis_key_coordinates  .The co-ordinates for each axis must be increasing.
    The number of keys used to uniquely identify (locate) the section must be
    given by  num_loc_keys  .
    The array of location key names must be pointed to by  loc_key_names  .
    The array of location key values must be pointed to by  loc_key_values  .
    On error, the routine writes the error code to the storage pointed to by
    errcode  .
    The routine returns a DataSection on success, else it returns NULL.
*/
{
    DataSection section;
    iarray iarr;
    flag in_one_array = TRUE;
    unsigned int key_count;
    unsigned int list_key_count;
    unsigned int index, value_index;
    unsigned int coord_count, off;
    unsigned int elem_count;
    uaddr section_offset;
    double coord;
    char *var_desc;
    char *single_array, *array_ptr;
    char *top_packet;
    uaddr **offsets;
    packet_desc *top_pack_desc;
    packet_desc *value_pack_desc = NULL; /*Initialised to keep compiler happy*/
    multi_array *multi_desc;
    array_desc *prev_arr_desc;
    array_desc *arr_desc = NULL;  /*  Initialised to keep compiler happy  */
    array_desc *single_arr_desc = NULL;
    dim_desc *dim;
    static char function_name[] = "storage_create_section";

    VERIFY_DATASTORE (datastore);
    if ( (value_name == NULL) || (axis_key_names == NULL) ||
	(axis_key_lengths == NULL) || (axis_key_coordinates == NULL) ||
	(loc_key_names == NULL) || (loc_key_values == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    *errcode = STORAGE_ERR_UNDEFINED;
    if (!datastore->vm)
    {
	(void) fprintf (stderr,
			"Only virtual memory DataStore implemented yet\n");
	return (NULL);
    }
    if (!datastore->new)
    {
	(void) fprintf (stderr, "DataStore must be a newly created one\n");
	return (NULL);
    }
    multi_desc = datastore->multi_desc;
    top_pack_desc = multi_desc->headers[datastore->array_num];
    top_packet = multi_desc->data[datastore->array_num];
    /*  Get parent packet descriptor for data value  */
    switch ( ds_f_name_in_packet (top_pack_desc, value_name,
				  &var_desc, &value_index) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr, "Data Value name: \"%s\" not found\n",
			value_name);
	return (NULL);
/*
	break;
*/
      case IDENT_DIMENSION:
	(void) fprintf (stderr,
			"Data Value name: \"%s\" must not be a dimension\n",
			value_name);
	return (NULL);
/*
	break;
*/
      case IDENT_ELEMENT:
	/*  Fine  */
	value_pack_desc = (packet_desc *) var_desc;
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Data Value name: \"%s\" found multiple times\n",
			value_name);
	a_prog_bug (function_name);
	break;
      default:
	(void) fprintf (stderr,
			"Illegal return value from: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    /*  Make sure no more than one key is a list key  */
    list_key_count = 0;
    for (key_count = 0, prev_arr_desc = NULL; key_count < num_axis_keys;
	 ++key_count)
    {
	if (axis_key_names[key_count] == NULL)
	{
	    (void) fprintf (stderr, "No name for axis key: %u\n", key_count);
	    a_prog_bug (function_name);
	}
	/*  Verify co-ordinates are increasing  */
	for (coord_count = 0, coord = -TOOBIG;
	     (coord_count < axis_key_lengths[key_count]) &&
	     (axis_key_coordinates[key_count] != NULL);
	     ++coord_count)
	{
	    if (axis_key_coordinates[key_count][coord_count] <= coord)
	    {
		(void) fprintf (stderr,
				"Co-ordinates for key: \"%s\" not increasing\n"
				,axis_key_names[key_count]);
		a_prog_bug (function_name);
	    }
	    coord = axis_key_coordinates[key_count][coord_count];
	}
	switch ( ds_f_name_in_packet (top_pack_desc, axis_key_names[key_count],
				      &var_desc, &index) )
	{
	  case IDENT_NOT_FOUND:
	    (void) fprintf (stderr, "Key: \"%s\" not found\n",
			    axis_key_names[key_count]);
	    return (NULL);
/*
	    break;
*/
	  case IDENT_DIMENSION:
	    arr_desc = (array_desc *) var_desc;
	    /*  Test to see if the same array is always returned  */
	    if (single_arr_desc == NULL) single_arr_desc = arr_desc;
	    if (prev_arr_desc == NULL)
	    {
		prev_arr_desc = arr_desc;
		if (value_pack_desc != arr_desc->packet)
		{
		    /*  Data value is not in this array  */
		    in_one_array = FALSE;
		}
	    }
	    else
	    {
		if (prev_arr_desc != arr_desc)
		{
		    in_one_array = FALSE;
		}
	    }
	    dim = arr_desc->dimensions[index];
	    /*  Verify co-ordinates match  */
	    /*  Fine  */
	    break;
	  case IDENT_ELEMENT:
	    if (list_key_count++ > 0)
	    {
		(void) fprintf (stderr, "Only one list key allowed\n");
		return (NULL);
	    }
	    /*  Data is not all in one n-dimensional array  */
	    in_one_array = FALSE;
	    break;
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr, "Key: \"%s\" found multiple times\n",
			    axis_key_names[key_count]);
	    a_prog_bug (function_name);
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value from: ds_f_name_in_packet\n"
			    );
	    a_prog_bug (function_name);
	    break;
	}
    }
    if ( !in_one_array || (single_arr_desc == NULL) )
    {
	(void) fprintf (stderr, "Single array only supported at this time\n");
	return (NULL);
    }
    if ( !ds_compute_array_offsets (single_arr_desc) )
    {
	(void) fprintf (stderr, "Error computing array offsets\n");
	return (NULL);
    }
    offsets = single_arr_desc->offsets;
    /*  At this point the axis keys have been grokked. Time to meditate on the
	location keys.  */
    for (key_count = 0, prev_arr_desc = NULL, section_offset = 0;
	 key_count < num_loc_keys; ++key_count)
    {
	if (loc_key_names[key_count] == NULL)
	{
	    (void) fprintf (stderr, "No name for location key: %u\n",
			    key_count);
	    a_prog_bug (function_name);
	}
	switch ( ds_f_name_in_packet (top_pack_desc, loc_key_names[key_count],
				      &var_desc, &index) )
	{
	  case IDENT_NOT_FOUND:
	    (void) fprintf (stderr, "Key: \"%s\" not found\n",
			    loc_key_names[key_count]);
	    return (NULL);
/*
	    break;
*/
	  case IDENT_DIMENSION:
	    arr_desc = (array_desc *) var_desc;
	    /*  Test to see if the same array is always returned  */
	    if (prev_arr_desc == NULL)
	    {
		prev_arr_desc = arr_desc;
		if (value_pack_desc != arr_desc->packet)
		{
		    /*  Data value is not in this array  */
		    in_one_array = FALSE;
		}
	    }
	    else
	    {
		if (prev_arr_desc != arr_desc)
		{
		    in_one_array = FALSE;
		}
	    }
	    /*  Fine  */
	    dim = arr_desc->dimensions[index];
	    off = offsets[index][ds_get_coord_num (dim,
						   loc_key_values[key_count],
						   SEARCH_BIAS_CLOSEST)];
	    section_offset += off;
	    break;
	  case IDENT_ELEMENT:
	    if (list_key_count++ > 0)
	    {
		(void) fprintf (stderr, "Only one list key allowed\n");
		return (NULL);
	    }
	    /*  Data is not all in one n-dimensional array  */
	    in_one_array = FALSE;
	    break;
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr, "Key: \"%s\" found multiple times\n",
			    loc_key_names[key_count]);
	    a_prog_bug (function_name);
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value from: ds_f_name_in_packet\n"
			    );
	    a_prog_bug (function_name);
	    break;
	}
    }
    if (!in_one_array)
    {
	(void) fprintf (stderr, "Single array only supported at this time\n");
	return (NULL);
    }
    if ( (prev_arr_desc != NULL) && (prev_arr_desc != single_arr_desc) )
    {
	(void) fprintf (stderr, "Single array only supported at this time\n");
	return (NULL);
    }
    /*  Simple, n-dimensional array: yay!  */
    if (num_axis_keys + num_loc_keys > single_arr_desc->num_dimensions)
    {
	(void) fprintf (stderr, "Too many axis/location keys for array\n");
	return (NULL);
    }
    if (num_axis_keys + num_loc_keys < single_arr_desc->num_dimensions)
    {
	(void) fprintf (stderr, "Insufficient axis/location keys for array\n");
	return (NULL);
    }
    /*  Locate section  */
    /*  First find array in top packet  */
    for (elem_count = 0, single_array = NULL;
	 elem_count < top_pack_desc->num_elements;
	 ++elem_count)
    {
	if (top_pack_desc->element_types[elem_count] != K_ARRAY) continue;
	if (single_arr_desc ==
	    (array_desc *) top_pack_desc->element_desc[elem_count])
	{
	    single_array = top_packet + ds_get_element_offset (top_pack_desc,
							       elem_count);
	    single_array = *(char **) single_array;
	    elem_count = top_pack_desc->num_elements;
	}
    }
    if (single_array == NULL)
    {
	(void) fprintf (stderr, "Couldn't find right array!\n");
	return (NULL);
    }
    array_ptr = single_array + section_offset;
    /*  Create a section and start filling it in  */
    if ( ( section = alloc_datasection (datastore) ) == NULL )
    {
	m_error_notify (function_name, "DataSection");
	return (NULL);
    }
    if ( ( iarr = (iarray) m_alloc (sizeof *iarr) ) == NULL )
    {
	m_error_notify (function_name, "Intelligent Array");
	storage_free_section (section);
	return (NULL);
    }
    m_clear ( (char *) iarr, sizeof *iarr );
    section->iarray = iarr;
    iarr->multi_desc = datastore->multi_desc;
    ++iarr->multi_desc->attachments;
    if ( ( iarr->offsets = (uaddr **)
	  m_alloc (num_axis_keys * sizeof *iarr->offsets) ) == NULL )
    {
	m_error_notify (function_name, "array of offset array pointers");
	storage_free_section (section);
	return (NULL);
    }
    if ( ( iarr->lengths = (unsigned long *)
	  m_alloc (num_axis_keys * sizeof *iarr->lengths) ) == NULL )
    {
	m_error_notify (function_name, "array of lengths");
	storage_free_section (section);
	return (NULL);
    }
    if ( ( iarr->contiguous = (flag *)
	  m_alloc (num_axis_keys * sizeof *iarr->contiguous) ) == NULL )
    {
	m_error_notify (function_name, "array of lengths");
	storage_free_section (section);
	return (NULL);
    }
    if ( ( iarr->orig_dim_indices = (unsigned int *)
	  m_alloc (num_axis_keys * sizeof *iarr->orig_dim_indices) )
	== NULL )
    {
	m_error_notify (function_name, "array of dimension indices");
	storage_free_section (section);
	return (NULL);
    }
    /*  Loop through axis keys  */
    for (key_count = 0; key_count < num_axis_keys; ++key_count)
    {
	if ( ( index = ds_f_dim_in_array (single_arr_desc,
					  axis_key_names[key_count]) )
	    >= single_arr_desc->num_dimensions )
	{
	    (void) fprintf (stderr, "Axis: \"%s\" not found\n",
			    axis_key_names[key_count]);
	    a_prog_bug (function_name);
	}
	dim = arr_desc->dimensions[index];
	if ( ( iarr->offsets[key_count] = (uaddr *)
	      m_alloc (sizeof **iarr->offsets *axis_key_lengths[key_count]) )
	    == NULL )
	{
	    m_error_notify (function_name, "offset array");
	    storage_free_section (section);
	    return (NULL);
	}
	iarr->lengths[key_count] = axis_key_lengths[key_count];
	for (coord_count = 0; coord_count < axis_key_lengths[key_count];
	     ++coord_count)
	{
	    coord = axis_key_coordinates[key_count][coord_count];
	    off = offsets[index][ds_get_coord_num (dim, coord,
						   SEARCH_BIAS_CLOSEST)];
	    iarr->offsets[key_count][coord_count] = off;
	}
	iarr->contiguous[key_count] = FALSE;
	iarr->orig_dim_indices[key_count] = index;
    }
    iarr->data = ds_get_element_offset (value_pack_desc,
					  value_index) + array_ptr;
    iarr->top_pack_desc = top_pack_desc;
    iarr->top_packet = multi_desc->data + datastore->array_num;
    iarr->arr_desc = single_arr_desc;
    iarr->array_num = datastore->array_num;
    iarr->boundary_width = 0;
    iarr->elem_index = value_index;
    iarr->num_dim = num_axis_keys;
    iarr->restrictions = NULL;
    /*  Do more work  */
    return (section);
}   /*  End Function storage_create_section  */

/*PUBLIC_FUNCTION*/
void storage_free_section (DataSection section)
/*  This routine will free the storage associated with a section. The routine
    implicitly calls  storage_flush_section  first.
    The routine returns nothing.
*/
{
    iarray iarr;
/*
    unsigned int count;
    static char function_name[] = "storage_free_section";
*/

    storage_flush_section (section);
    iarr = section->iarray;
    /*  Deallocate section information  */
#ifdef not_implemented
    if (section->coordinates != NULL)
    {
	for (count = 0; count < iarray_num_dim (iarr); ++count)
	{
	    if (section->coordinates[count] != NULL)
	    {
		m_free ( (char *) section->coordinates[count] );
	    }
	}
	m_free ( (char *) section->coordinates );
    }
#endif
    if (section->priv != NULL) m_free ( (char *) section->priv );
    iarray_dealloc (iarr);
    m_free ( (char *) section );
}   /*  End Function storage_free_section  */

/*PUBLIC_FUNCTION*/
void storage_flush_section (DataSection section)
/*  This routine will flush the storage associated with a section to the
    DataStore memory. This does not imply any file IO operations.
    The routine returns nothing.
*/
{
/*
    static char function_name[] = "storage_flush_section";
*/

}   /*  End Function storage_flush_section  */

/*PUBLIC_FUNCTION*/
unsigned int storage_get_keywords (DataStore datastore, char ***keyword_names,
				   unsigned int **keyword_types)
/*  This routine will get all the (unique) keywords in a DataStore.
    The DataStore must be given by  datastore  .
    The pointer to the array of keyword name pointers will be written to the
    storage pointed to by  keyword_name  .Both the array of pointers and the
    string pointers should be freed by  m_free  .
    The pointer to the array of keyword types will be written to the storage
    pointed to by  keyword_types  .This should be freed by  m_free  .
    The routine returns the number of keywords.
*/
{
    unsigned int count;
    unsigned int num_keys;
    char **names;
    char *top_packet;
    unsigned int *types;
    packet_desc *top_pack_desc;
    multi_array *multi_desc;
    static char function_name[] = "storage_get_keywords";

    multi_desc = datastore->multi_desc;
    top_pack_desc = multi_desc->headers[datastore->array_num];
    top_packet = multi_desc->data[datastore->array_num];
    for (count = 0, num_keys = 0; count < top_pack_desc->num_elements;
	 ++count)
    {
	if ( ds_element_is_named (top_pack_desc->element_types[count]) )
	{
	    ++num_keys;
	}
    }
    if ( ( names = (char **) m_alloc (num_keys * sizeof *names) ) == NULL )
    {
	m_abort (function_name, "array of keyword name pointers");
    }
    if ( ( types = (unsigned int *)
	  m_alloc (num_keys * sizeof *keyword_types) ) == NULL )
    {
	m_abort (function_name, "array of keyword types");
    }
    for (count = 0, num_keys = 0; count < top_pack_desc->num_elements;
	 ++count)
    {
	if ( ds_element_is_named (top_pack_desc->element_types[count]) )
	{
	    if ( ( names[num_keys] =
		  st_dup (top_pack_desc->element_desc[count]) ) == NULL )
	    {
		m_abort (function_name, "keyword name");
	    }
	}
	types[num_keys] = top_pack_desc->element_types[count];
    }
    *keyword_names = names;
    *keyword_types = types;
    return (num_keys);
}   /*  End Function storage_get_keywords  */

/*PUBLIC_FUNCTION*/
CONST char *storage_get_keyword_value (DataStore datastore, CONST char *name,
				       unsigned int *keyword_type)
/*  This routine will get the value of a (unique) keyword in a DataStore.
    The DataStore must be given by  datastore  .
    The name of the keyword must be pointed to by  name  .
    The type of the keyword will be written to the storage pointed to by
    keyword_type  .
    The routine returns a pointer to the data value on success,
    else it returns NULL (indicating the keyword does not exist).
    The data value should be freed by  m_free  .
*/
{
    unsigned int index, type;
    FString *inp_fstring, *out_fstring;
    char *out_value, *inp_value, *str;
    char *top_packet;
    packet_desc *top_pack_desc;
    multi_array *multi_desc;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "storage_get_keyword_value";

    VERIFY_DATASTORE (datastore);
    if ( (name == NULL) || (keyword_type == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    multi_desc = datastore->multi_desc;
    top_pack_desc = multi_desc->headers[datastore->array_num];
    top_packet = multi_desc->data[datastore->array_num];
    if ( ( index = ds_f_elem_in_packet (top_pack_desc, name) ) >=
	top_pack_desc->num_elements )
    {
	return (NULL);
    }
    type = top_pack_desc->element_types[index];
    inp_value = top_packet + ds_get_element_offset (top_pack_desc, index);
    if ( ( out_value = m_alloc (host_type_sizes[type]) ) == NULL )
    {
	m_abort (function_name, "keyword value");
    }
    switch (type)
    {
      case K_VSTRING:
	if ( ( str = st_dup (*(char **) inp_value) ) == NULL )
	{
	    m_abort (function_name, "string");
	}
	*(char **) out_value = str;
	break;
      case K_FSTRING:
	inp_fstring = (FString *) inp_value;
	out_fstring = (FString *) out_value;
	if ( ( str = m_alloc (inp_fstring->max_len) ) == NULL )
	{
	    m_abort (function_name, "fixed string");
	}
	m_clear (str, inp_fstring->max_len);
	(void) strncpy (str, inp_fstring->string, inp_fstring->max_len);
	out_fstring->string = str;
	out_fstring->max_len = inp_fstring->max_len;
	break;
      default:
	m_copy (out_value, inp_value, host_type_sizes[type]);
	break;
    }
    *keyword_type = type;
    return (out_value);
}   /*  End Function storage_get_keyword_value  */

/*PUBLIC_FUNCTION*/
flag storage_change_keyword_value (DataStore datastore, CONST char *name,
				   char *value, unsigned int type)
/*  This routine will change the value of a (unique) keyword in a DataStore.
    The keyword must already exist.
    The DataStore must be given by  datastore  .
    The name of the keyword must be pointed to by  name  .
    The new data value for the keyword must be pointed to by  value  .
    The type of the data value must be given by  type  .This is used for type
    checking. The type cannot be changed.
    The routine returns TRUE on success,
    else it returns FALSE (indicating the keyword does not already exist).
*/
{
    unsigned int index;
    FString *inp_fstring, *out_fstring;
    char *str;
    char *top_packet, *new_value;
    packet_desc *top_pack_desc;
    multi_array *multi_desc;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "storage_change_keyword_value";

    VERIFY_DATASTORE (datastore);
    if ( ( name == NULL) || (value == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (!datastore->vm)
    {
	(void) fprintf (stderr,
			"Operation on non-VM DataStore not implemented\n");
	a_prog_bug (function_name);
    }
    multi_desc = datastore->multi_desc;
    top_pack_desc = multi_desc->headers[datastore->array_num];
    top_packet = multi_desc->data[datastore->array_num];
    if ( ( index = ds_f_elem_in_packet (top_pack_desc, name) ) >=
	top_pack_desc->num_elements )
    {
	return (FALSE);
    }
    if (type != top_pack_desc->element_types[index])
    {
	(void) fprintf (stderr,
			"Attempt to change keyword type from: %u to %u\n",
			top_pack_desc->element_types[index], type);
	a_prog_bug (function_name);
    }
    new_value = top_packet + ds_get_element_offset (top_pack_desc, index);
    if ( ds_element_is_atomic (type) )
    {
	/*  Just copy over the data  */
	m_copy (new_value, value, host_type_sizes[type]);
	return (TRUE);
    }
    /*  String type  */
    switch (type)
    {
      case K_VSTRING:
	if ( ( str = st_dup (*(char **) value) ) == NULL )
	{
	    m_abort (function_name, "string");
	}
	if (*(char **) new_value != NULL) m_free (*(char **) new_value);
	*(char **) new_value = str;
	break;
      case K_FSTRING:
	inp_fstring = (FString *) value;
	out_fstring = (FString *) new_value;
	if ( ( str = m_alloc (inp_fstring->max_len) ) == NULL )
	{
	    m_abort (function_name, "fixed string");
	}
	m_clear (str, inp_fstring->max_len);
	(void) strncpy (str, inp_fstring->string, inp_fstring->max_len);
	if (out_fstring->string != NULL) m_free (out_fstring->string);
	out_fstring->string = str;
	out_fstring->max_len = inp_fstring->max_len;
	break;
      default:
	break;
    }
    return (TRUE);
}   /*  End Function storage_change_keyword_value  */

#ifdef dummy
/*PUBLIC _ FUNCTION*/
DataVector storage_get_first_vector (DataStore datastore,
				     CONST char *value_name,
				     flag auto_update, unsigned int *errcode)
/*  This routine will get a vector (chunk) of values in a DataStore, starting
    at the first occurrence of the value.
    The DataStore must be given by  datastore  .
    The name of the value must be pointed to by  value_name  .
    If the section data should be automatically written back to the DataStore
    then the value of  auto_update  must be TRUE. If this is FALSE, the
    storage_flush_section  routine must be used to update the section to the
    DataStore.
    The length of the vector and the ordering of the values is choosen by the
    routine.
    On error, the routine writes the error code to the storage pointed to by
    errcode  .
    The routine returns a DataVector on success, else it returns NULL.
*/
{
    static char function_name[] = "storage_get_first_vector";

    (void) fprintf (stderr, "Vector iterations not implemented\n");
    a_prog_bug (function_name);
}   /*  End Function storage_get_first_vector  */

/*PUBLIC _ FUNCTION*/
flag storage_goto_next_vector (DataVector vector)
/*  This routine will move the vector (view) of values in a DataVector to the
    next chunk of data in a DataStore. The length of the vector and the
    ordering of the values is choosen by the routine.
    Iterating through the data in this fashion is guaranteed to be the most
    efficient way of processing the data.
    The DataVector must be given by  vector  .
    The routine returns TRUE if the DataVector was moved to next chunk,
    else it returns FALSE indicting that there are no more chunks to process.
*/
{
    static char function_name[] = "storage_goto_next_vector";

    (void) fprintf (stderr, "Vector iterations ot implemented\n");
    a_prog_bug (function_name);
}   /*  End Function storage_goto_next_vector  */
#endif

/*  Private functions follow  */

static DataStore alloc_datastore ()
/*  This routine will allocate a DataStore.
    The routine returns a DataStore on success, else it returns NULL.
*/
{
    DataStore datastore;
/*
    static char function_name[] = "alloc_datastore";
*/

    if ( ( datastore = (DataStore) m_alloc (sizeof *datastore) ) == NULL )
    {
	return (NULL);
    }
    datastore->magic_number = DATASTORE_MAGIC_NUMBER;
    datastore->multi_desc = NULL;
    datastore->arr_desc = NULL;
    datastore->channel = NULL;
    datastore->vm = FALSE;
    datastore->new = FALSE;
    return (datastore);
}   /*  End Function alloc_datastore  */

static DataSection alloc_datasection (DataStore datastore)
/*  This routine will allocate a DataSection for a DataStore.
    The DataStore must be given by  datastore  .
    The routine returns a DataSection on success, else it returns NULL.
*/
{
    DataSection datasection;
    DataSectionInternal internal;
    static char function_name[] = "alloc_datasection";

    if ( ( datasection = (DataSection) m_alloc (sizeof *datasection) )
	== NULL )
    {
	return (NULL);
    }
    m_clear ( (char *) datasection, sizeof *datasection );
    if ( ( internal = (DataSectionInternal)
	  m_alloc (sizeof *datasection->priv) ) == NULL )
    {
	m_free ( (char *) datasection );
	m_error_notify (function_name, "DataSectionInternal");
	return (NULL);
    }
    m_clear ( (char *) internal, sizeof *internal );
    datasection->priv = internal;
    internal->magic_number = DATASECTIONINTERNAL_MAGIC_NUMBER;
    internal->datastore = datastore;
    return (datasection);
}   /*  End Function alloc_datasection  */

static void dealloc_datastore (DataStore datastore)
/*  This routine will deallocate a DataStore.
    The DataStore must be given by  datastore  .
    The routine returns nothing.
*/
{
    static char function_name[] = "dealloc_datastore";

    VERIFY_DATASTORE (datastore);
    if (datastore->channel != NULL)
    {
	dsrw_write_multi (datastore->channel, datastore->multi_desc);
	(void) ch_close (datastore->channel);
    }
    ds_dealloc_multi (datastore->multi_desc);
    m_clear ( (char *) datastore, sizeof *datastore );
    m_free ( (char *) datastore );
}   /*  End Function dealloc_datastore  */

static unsigned int determine_filetype(CONST char *name,
				       char filename[SM_MAX_PATHNAME_LENGTH],
				       char structname[SM_MAX_PATHNAME_LENGTH],
				       char hostname[SM_MAX_PATHNAME_LENGTH])
/*  This routine will determine the filetype of a filename.
    The name must be pointed to by  name  .The length of this must be less than
    SM_MAX_PATHNAME_LENGTH.
    The filename will be written to the storage pointed to by  filename  .
    The general data structure name will be written to the storage pointed to
    by  structname  .This will be empty when the file is remote.
    The remote hostname will be written to the storage pointed to  hostname  .
    The routine returns the file type.
*/
{
    int string_length;
    CONST char *ptr, *end_type_ptr;
    static char function_name[] = "determine_filetype";

    if ( ( string_length = strlen (name) ) >= SM_MAX_PATHNAME_LENGTH )
    {
	(void) fprintf (stderr, "Name: \"%s\" too long\n", name);
	a_prog_bug (function_name);
    }
    /*  Check for remote file  */
    if ( ( ptr = strchr (name, '@') ) == NULL )
    {
	/*  Local  */
	hostname[0] = '\0';
    }
    else
    {
	(void) strncpy (filename, name, ptr - name);
	filename[ptr - name] = '\0';
	structname[0] = '\0';
	(void) strcpy (hostname, ptr + 1);
	return (FILE_TYPE_REMOTE);
    }
    /*  Strip named data structure  */
    if ( ( end_type_ptr = strchr (name, ':') ) == NULL )
    {
	end_type_ptr = name + string_length;
	structname[0] = '\0';
    }
    else
    {
	(void) strcpy (structname, end_type_ptr + 1);
    }
    /*  Search for '.'  */
    for (ptr = end_type_ptr - 1; (ptr > name) && (*ptr != '.'); --ptr);
    if (ptr <= name)
    {
	/*  Default filetype: Karma  */
	(void) strncpy (filename, name, end_type_ptr - name);
	filename[end_type_ptr - name] = '\0';
	(void) strcat (filename, ".kf");
	return (FILE_TYPE_KARMA);
    }
    if (strncmp (ptr, ".kf", end_type_ptr - ptr) == 0)
    {
	(void) strncpy (filename, name, end_type_ptr - name);
	filename[end_type_ptr - name] = '\0';
	return (FILE_TYPE_KARMA);
    }
    if (strncmp (ptr, ".fits", end_type_ptr - ptr) == 0)
    {
	(void) strncpy (filename, name, end_type_ptr - name);
	filename[end_type_ptr - name] = '\0';
	return (FILE_TYPE_FITS);
    }
    return (FILE_TYPE_UNKNOWN);
}   /*  End Function determine_filetype  */

static flag test_for_list (packet_desc *pack_desc)
/*  This routine will determine if a general data structure has one or more
    linked lists located in it's heirarchy.
    The packet descriptor must be given by  pack_desc  .
    The routine returns TRUE if the data structure has a linked list,
    else it returns FALSE.
*/
{
    unsigned int count;
    unsigned int type;
    array_desc *arr_desc;
    static char function_name[] = "test_for_list";

    if (pack_desc == NULL)
    {
	(void) fprintf (stderr, "NULL descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    for (count = 0; count < pack_desc->num_elements; ++count)
    {
	type = pack_desc->element_types[count];
	switch (type)
	{
	  case K_ARRAY:
	    arr_desc = (array_desc *) pack_desc->element_desc[count];
	    if ( test_for_list (arr_desc->packet) ) return (TRUE);
	    break;
	  case LISTP:
	    return (TRUE);
/*
	    break;
*/
	  default:
	    break;
	}
    }
    return (FALSE);
}   /*  End Function test_for_list  */

static void compute_section_from_location (DataSection section)
/*  This routine will compute the section start from the section location.
    The section must be given by  section  .
    The routine returns nothing.
*/
{
    DataSectionInternal internal;
    uaddr off = 0;
    unsigned int key_count, dim_index;
    unsigned long coord;
    array_desc *arr_desc;
    static char function_name[] = "compute_section_from_location";

    VERIFY_DATASECTION (section);
    internal = section->priv;
    if (internal->single_array == NULL)
    {
	(void) fprintf (stderr, "Illegal operation on non-iterator section\n");
	a_prog_bug (function_name);
    }
    arr_desc = section->iarray->arr_desc;
    /*  First run through the unordered keys  */
    for (key_count = 0; key_count < internal->num_u_keys; ++key_count)
    {
	dim_index = internal->u_key_indices[key_count];
	coord = internal->u_location[key_count];
	off += arr_desc->offsets[dim_index][coord];
    }
    /*  Now run through the ordered keys  */
    for (key_count = 0; key_count < internal->num_o_keys; ++key_count)
    {
	dim_index = internal->o_key_indices[key_count];
	coord = internal->o_location[key_count];
	coord = internal->o_key_coords[key_count][coord];
	off += arr_desc->offsets[dim_index][coord];
    }
    section->iarray->data = internal->single_array + off;
}   /*  End Function compute_section_from_location  */

static flag find_next_section (DataSection section)
/*  This routine will find the next section defined in the section iterator.
    The section must be given by  section  .
    The routine returns TRUE if the next section was found, else it returns
    FALSE indicating that there are no more sections left.
*/
{
    DataSectionInternal internal;
    int key_count;
    unsigned int dim_index;
    array_desc *arr_desc;
    dim_desc *dim;
    static char function_name[] = "find_next_section";

    VERIFY_DATASECTION (section);
    internal = section->priv;
    if (internal->single_array == NULL)
    {
	(void) fprintf (stderr, "Illegal operation on non-iterator section\n");
	a_prog_bug (function_name);
    }
    arr_desc = section->iarray->arr_desc;
    /*  First run through the unordered keys  */
    for (key_count = internal->num_u_keys - 1; key_count >= 0; --key_count)
    {
	dim_index = internal->u_key_indices[key_count];
	dim = arr_desc->dimensions[dim_index];
	if (++internal->u_location[key_count] < dim->length)
	{
	    /*  Haven't run past end of this dimension: return now  */
	    return (TRUE);
	}
	/*  Have run past end of this dimension: reset it and go up  */
	internal->u_location[key_count] = 0;
    }
    /*  Now run through the ordered keys  */
    for (key_count = internal->num_o_keys - 1; key_count >= 0; --key_count)
    {
	if (++internal->o_location[key_count] <
	    internal->o_key_lengths[key_count])
	{
	    /*  Haven't run past end of this dimension: return now  */
	    return (TRUE);
	}
	/*  Have run past end of this dimension: reset it and go up  */
	internal->o_location[key_count] = 0;
    }
    return (FALSE);
}   /*  End Function find_next_section  */
