/*LINTLIBRARY*/
/*PREFIX:"iarray_"*/
/*  iarray.c

    This code provides the Intelligent Array core interface to Karma data
    structures.

    Copyright (C) 1992,1993  Richard Gooch

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

/*  This file contains all routines needed for the simple processing of
  n-dimensional data structures.


    Written by      Richard Gooch   16-NOV-1992

    Updated by      Richard Gooch   21-DEC-1992

    Updated by      Richard Gooch   3-JAN-1993: Took acccount of change to
  ds_alloc_data

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   1-FEB-1993: Used  ds_compute_array_offsets
  rather than computing within Iarray. Reading tiled arrays now transparent.

    Updated by      Richard Gooch   9-FEB-1993: Added 3-dimensional array
  support.

    Updated by      Richard Gooch   12-FEB-1993: Added toroidal mapping support

    Updated by      Richard Gooch   17-FEB-1993: Fixed bug in
  iarray_get_sub_array_2D

    Updated by      Richard Gooch   7-MAR-1993: Changed explanation of
  arrayfile  name conventions in  iarray_write  to a reference to
  dsxfr_put_multi  .

    Updated by      Richard Gooch   27-MAR-1993: Added explicit control for
  memory mapping to  iarray_read_nD  by adding parameter.

    Updated by      Richard Gooch   31-MAR-1993: Added  iarray_dim_name
  routine.

    Updated by      Richard Gooch   1-APR-1993: Fixed bug in
  iarray_get_element_3D  .

    Updated by      Richard Gooch   24-APR-1993: Removed  iarray_draw_ellipse
  and  iarray_draw_polygon  routines (superceeded by  viewimg_  package).

    Updated by      Richard Gooch   17-MAY-1993: Took account of change to
  dsxfr_get_multi  .

    Updated by      Richard Gooch   5-JUL-1993: Added more error testing.

    Updated by      Richard Gooch   8-AUG-1993: Made explicit reference to
  documentation on  dsxfr_get_multi  and documented changes to attachment
  counts.

    Updated by      Richard Gooch   16-AUG-1993: Fixed bug in
  iarray_get_from_multi_array  when specifying element name.

    Updated by      Richard Gooch   5-SEP-1993: Added  iarray_put_named_string
  and  iarray_get_named_string  routines and moved wrapper functions to
  iarray_wrappers.c
  Also added test for environment variable "IARRAY_ALLOC_DEBUG".

    Updated by      Richard Gooch   9-SEP-1993: Added  iarray_set_world_coords
  and  iarray_get_world_coords  routines.

    Updated by      Richard Gooch   19-SEP-1993: Fixed bug in  iarray_copy_data
  when copying between two arrays of the same type where one array has multiple
  element packets.

    Last updated by Richard Gooch   10-OCT-1993: Fixed bug in
  iarray_get_from_multi_array  which printed NULL string on error.


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_r.h>
#include <karma_a.h>


#define VERIFY_IARRAY(array) if (array == NULL) \
{(void) fprintf (stderr, "NULL iarray passed\n"); \
 a_prog_bug (function_name); }
#ifdef dummy
if ( (*array).magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid iarray\n"); \
 a_prog_bug (function_name); }
#endif


/*  Private functions  */
static iarray get_array_from_array ();
static flag iarray_allocate_records ();
static char *iarray_get_next_element ();
static unsigned int iarray_get_max_contiguous ();
static flag mem_debug_required ();


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
iarray iarray_read_nD (object, cache, arrayname, num_dim, dim_names, elem_name,
		       mmap_option)
/*  This routine will read in a Karma arrayfile and will extract information
    relevent to Patrick Jordan's "Intelligent Arrays".
    The name of the arrayfile to read must be pointed to by  object  .This
    parameter is passed directly to the  dsxfr_get_multi  routine. In order to
    understand the operation of the  iarray_read_nD  routine, the operation of
    the  dsxfr_get_multi  routine must be understood.
    The value of  cache  is passed directly to the  dsxfr_get_multi  routine.
    This controls whether disc arrayfiles are cached in memory for later use.
    The name of the general data structure in the arrayfile to search for must
    be pointed to by  arrayname  .If this is NULL, the routine searches for
    the default name "Intelligent Array". If the arrayfile has only one
    general data structure, then this parameter is ignored.
    The routine searches for an n-dimensional array with a single atomic
    element at each point in multi-dimensional space.
    If  num_dim  is greater than 0, the routine will only return an array with
    num_dim  dimensions. If  num_dim  is 0, then the routine will return an
    n-dimensional array.
    If  num_dim  is not 0, then if  dim_names  is NULL, the routine will search
    for and return an array with the default dimension names (see iarray_create
    for a list of these) if more than one n-dimensional, single element array
    exists in the general data structure, or the only n-dimensional array with
    the specified number of dimensions. If the routine can't find an adequate
    default, it will not return an array.
    If  num_dim  is not 0, and  dim_names  points to an array of strings, then
    the routine will only return an array which matches the specified dimension
    names. The first name in the array of strings must be the highest order
    dimension.
    If  elem_name  is NULL, the routine will ignore the element name of the
    array which is located, else it will insist on the array element name
    matching the name pointed to by  elem_name  .
    The  mmap_option  parameter is passed directly to the  dsxfr_get_multi
    routine. This parameter controls the memory mapping of disc arrayfiles.
    If the data structure is likely to be subsequently modified, the value of
    must be K_CH_MAP_NEVER, otherwise the data may be read-only memory mapped
    and writing to it will cause a segmentation fault.
    The routine returns a dynamically allocated intelligent array on success,
    else it prints an error message to the standard output and returns NULL.
*/
char *object;
flag cache;
char *arrayname;
unsigned int num_dim;
char **dim_names;
char *elem_name;
unsigned int mmap_option;
{
    iarray array;
    multi_array *multi_desc;
    static char function_name[] = "iarray_read_nD";

    /*  First read in arrayfile  */
    if ( ( multi_desc = dsxfr_get_multi (object, cache, mmap_option, FALSE) )
	== NULL )
    {
	(void) fprintf (stderr, "Error reading Intelligent Array\n");
	return (NULL);
    }
    array = iarray_get_from_multi_array (multi_desc, arrayname,
					 num_dim, dim_names, elem_name);
    /*  Decrement attachment count  */
    ds_dealloc_multi (multi_desc);
    return (array);
}   /*  End Function iarray_read_nD  */

/*PUBLIC_FUNCTION*/
flag iarray_write (array, arrayfile)
/*  This routine will write an "Intelligent Array" in the Karma data format.
    The "Intelligent Array" must be given by  array  .
    The name of the arrayfile to write must be pointed to by  arrayfile  .
    See  dsxfr_put_multi  for details on the interpretation of  arrayfile  .
    The routine returns TRUE on success, else it prints an error message to the
    standard output and returns FALSE.
*/
iarray array;
char *arrayfile;
{
    static char function_name[] = "iarray_write";

    VERIFY_IARRAY (array);
    if ( (*array).multi_desc == NULL )
    {
	(void) fprintf (stderr,
			"Intelligent array is not an original array\n");
	a_prog_bug (function_name);
    }
    if (dsxfr_put_multi (arrayfile, (*array).multi_desc) != TRUE)
    {
	(void) fprintf (stderr, "Error writing Intelligent Array\n");
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function iarray_write  */

/*PUBLIC_FUNCTION*/
iarray iarray_create (type, num_dim, dim_names, dim_lengths, elem_name,
		      old_array)
/*  This routine will create an "Intelligent Array", using the Karma general
    data structure format as the underlying data format.
    If the environment variable "IARRAY_ALLOC_DEBUG" is set to "TRUE" then the
    routine will print allocation debugging information.
    The type of the data must be given by  type  .Legal values for this are:
        K_FLOAT, K_DOUBLE, K_BYTE, K_INT, K_SHORT, K_COMPLEX, K_DCOMPLEX,
	K_BCOMPLEX, K_ICOMPLEX,
        K_SCOMPLEX, K_LONG, K_LCOMPLEX, K_UBYTE, K_UINT, K_USHORT, K_ULONG,
	K_UBCOMPLEX,
        K_UICOMPLEX, K_USCOMPLEX, K_ULCOMPLEX.
    The number of dimensions the array must have must be given by  num_dim  .
    The names of the dimensions must be pointed to by  dim_names  .If this is
    NULL, the default names: "Axis 0", "Axis 1", etc. are used.
    The lengths of the dimensions must be pointed to by  dim_lengths  .
    The first entry in both  dim_names  and  dim_lengths  refers to the most
    significant dimension (ie. the dimension with the greatest stride in
    memory).
    The name of the element must be given by  elem_name  .If this is NULL, the
    default name: "Data Value" is choosen.
    Any auxilary information not representable with "Intelligent Arrays" which
    is to be included in the Karma data format is copied from the array pointed
    to by  old_array  .If this is NULL, no auxilary information is copied.
    The routine returns a dynamically allocated intelligent array on success,
    else it prints an error message to the standard output and returns NULL.
*/
unsigned int type;
unsigned int num_dim;
char **dim_names;
unsigned int *dim_lengths;
char *elem_name;
iarray old_array;
{
    unsigned int array_count;
    unsigned int elem_num;
    unsigned int dim_count;
    char *array;
    multi_array *out_multi_desc;
    multi_array *inp_multi_desc;
    iarray return_value;
    packet_desc *out_pack_desc;
    array_desc *out_array_desc;
    extern char *data_type_names[NUMTYPES];
    static char function_name[] = "iarray_create";

    if (old_array == NULL)
    {
	/*  Do not copy any old information  */
	if ( ( array = ds_easy_alloc_array (&out_multi_desc,
					    num_dim, dim_lengths,
					    (double *) NULL, (double *) NULL,
					    dim_names,
					    type, elem_name) )
	    == NULL )
	{
	    return (NULL);
	}
	out_array_desc = ( (array_desc *)
			  (* (*out_multi_desc).headers[0] ).element_desc[0] );
	if ( ( return_value = get_array_from_array (out_multi_desc,
						    (unsigned int) 0,
						    out_array_desc, array, 0) )
	    == NULL )
	{
	    ds_dealloc_multi (out_multi_desc);
	    return (NULL);
	}
	/*  Decrement attachment count  */
	ds_dealloc_multi (out_multi_desc);
	if ( mem_debug_required () )
	{
	    (void) fprintf (stderr, "iarray_create: ");
	    for (dim_count = 0; dim_count < num_dim - 1; ++dim_count)
	    {
		(void) fprintf (stderr, "%u * ", dim_lengths[dim_count]);
	    }
	    (void) fprintf (stderr, "%u  type: %s\n",
			    dim_lengths[dim_count], data_type_names[type]);
	}
	return (return_value);
    }
#ifdef dummy
    /*  Auxilary data is to be copied  */
    inp_multi_desc = (*old_array).multi_desc;
    if ( ( out_multi_desc = ds_alloc_multi ( (*inp_multi_desc).num_arrays ) )
	== NULL )
    {
	return (NULL);
    }
    /*  Copy auxilary descriptors and data  */
    for (array_count = 0; array_count < (*inp_multi_desc).num_arrays;
	 ++array_count)
    {
	if (array_count == (*old_array).array_num)
	{
	    /*  Skip this one  */
	    continue;
	}
	/*  Copy name  */
	if ( ( (*out_multi_desc).array_names[array_count] =
	      st_dup ( (*inp_multi_desc).array_names[array_count] ) )
	    == NULL )
	{
	    m_error_notify (function_name, "general data structure name");
	    ds_dealloc_multi (out_multi_desc);
	    return (NULL);
	}
	/*  Copy descriptor  */
	if ( ( (*out_multi_desc).headers[array_count] =
	      ds_copy_desc_until ( (*inp_multi_desc).headers[array_count],
				  (char *) NULL ) )
	    == NULL )
	{
	    m_error_notify (function_name,
			    "general data structure descriptor");
	    ds_dealloc_multi (out_multi_desc);
	    return (NULL);
	}
	/*  Allocate data  */
	if ( ( (*out_multi_desc).data[array_count] =
	      ds_alloc_data ( (*inp_multi_desc).headers[array_count], TRUE,
			     TRUE ) )
	    == NULL )
	{
	    m_error_notify (function_name,
			    "general data structure data");
	    ds_dealloc_multi (out_multi_desc);
	    return (NULL);
	}
	/*  Copy data  */
	if (ds_copy_data ( (*inp_multi_desc).headers[array_count],
			  (*inp_multi_desc).data[array_count],
			  (*out_multi_desc).headers[array_count],
			  (*out_multi_desc).data[array_count] ) != TRUE)
	{
	    (void) fprintf (stderr, "\nError copying auxilary data");
	    ds_dealloc_multi (out_multi_desc);
	    return (NULL);
	}
    }
    /*  Auxilary data in other general data structures all copied  */
    /*  Copy most of input general data structure  */
    if ( ( (*out_multi_desc).headers[(*old_array).array_num] =
	  ds_copy_desc_until ( (*inp_multi_desc).headers[(*old_array).array_num],
			   "x" ) )
	== NULL )
    {
	m_error_notify (function_name, "general data structure descriptor");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    switch ( ds_find_hole ( (*out_multi_desc).headers[(*old_array).array_num],
			   &out_pack_desc, &elem_num ) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr,
			"Old array does not have Intelligent Array\n");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr, "Old array has multiple holes\n");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
	break;
      case IDENT_ELEMENT:
	/*  Got what we wanted  */
	break;
      default:
	(void) fprintf (stderr,
			"Illegal return value from function: ds_find_hole\n");
	a_prog_bug (function_name);
	break;
    }
    /*  Allocate array descriptor  */
    if ( ( out_array_desc = ds_alloc_array_desc (2) ) == NULL )
    {
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    (*out_pack_desc).element_types[elem_num] = K_ARRAY;
    (*out_pack_desc).element_desc[elem_num] = (char *) out_array_desc;
    /*  Allocate dimension descriptors  */
    if ( ( (*out_array_desc).dimensions[0] =
	  ds_alloc_dim_desc (dim_names[0], num_rows, 0.0,
			     (double) (num_rows - 1), TRUE) )
	== NULL )
    {
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    if ( ( (*out_array_desc).dimensions[1] =
	  ds_alloc_dim_desc (dim_names[1], num_cols, 0.0,
			     (double) (num_cols - 1), TRUE) )
	== NULL )
    {
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    /*  Allocate array packet descriptor  */
    if ( ( (*out_array_desc).packet = ds_alloc_packet_desc (1) ) == NULL )
    {
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    /*  Allocate element name  */
    if ( ( (* (*out_array_desc).packet ).element_desc[0] =
	  st_dup (elem_name) ) == NULL )
    {
	m_error_notify (function_name, "element name");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    (* (*out_array_desc).packet ).element_types[0] = type;
    /*  Entire data structure now described: allocate data  */
    if ( ( (*out_multi_desc).data[(*old_array).array_num] =
	  ds_alloc_data ( (*out_multi_desc).headers[(*old_array).array_num],
			 TRUE, TRUE ) )
	== NULL )
    {
	m_error_notify (function_name, "data space");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    /*  Copy over auxilary data  */
    if (ds_copy_data ( (*inp_multi_desc).headers[(*old_array).array_num],
		      (*inp_multi_desc).data[(*old_array).array_num],
		      (*out_multi_desc).headers[(*old_array).array_num],
		      (*out_multi_desc).data[(*old_array).array_num] )
	!= TRUE)
    {
	(void) fprintf (stderr, "Error copying auxilary data\n");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    /*  Return "Intelligent Array" data structure  */
    if ( ( return_value = get_array_from_array (out_multi_desc,
						(*old_array).array_num,
						out_array_desc, array, 0) )
	== NULL )
    {
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    /*  Decrement attachment count  */
    ds_dealloc_multi (out_multi_desc);
    return (return_value);
#else
    (void) fprintf (stderr, "Auxilary data copy not finished\n");
    return (NULL);
#endif
}   /*  End Function iarray_create  */

/*PUBLIC_FUNCTION*/
iarray iarray_get_from_multi_array (multi_desc, arrayname, num_dim, dim_names,
				    elem_name)
/*  This routine will extract information relevent to Patrick Jordan's
    "Intelligent Arrays" from a multi array Karma general data structure.
    The multi array header must be pointed to by  multi_desc  .The attachment
    count is incremented on successful completion of this routine.
    The name of the general data structure in the arrayfile to search for must
    be pointed to by  arrayname  .If this is NULL, the routine searches for
    the default name "Intelligent Array". If the arrayfile has only one
    general data structure, then this parameter is ignored.
    The routine searches for an n-dimensional array with a single atomic
    element at each point in multi-dimensional space.
    If  num_dim  is greater than 0, the routine will only return an array with
    num_dim  dimensions. If  num_dim  is 0, then the routine will return an
    n-dimensional array.
    If  num_dim  is not 0, then if  dim_names  is NULL, the routine will search
    for and return an array with the default dimension names (see iarray_create
    for a list of these) if more than one n-dimensional, single element array
    exists in the general data structure, or the only n-dimensional array with
    the specified number of dimensions. If the routine can't find an adequate
    default, it will not return an array.
    If  num_dim  is not 0, and  dim_names  points to an array of strings, then
    the routine will only return an array which matches the specified dimension
    names. The first name in the array of strings must be the highest order
    dimension.
    If  elem_name  is NULL, the routine will ignore the element name of the
    array which is located, else it will insist on the array element name
    matching the name pointed to by  elem_name  .
    The routine returns a dynamically allocated intelligent array on success,
    else it prints an error message to the standard output and returns NULL.
*/
multi_array *multi_desc;
char *arrayname;
unsigned int num_dim;
char **dim_names;
char *elem_name;
{
    flag reorder_needed;
    unsigned int parent_type;
    unsigned int array_num;
    unsigned int elem_count;
    unsigned int match_index;
    unsigned int dim_count;
    unsigned int dim_index;
    unsigned int elem_index;
    unsigned int *reorder_indices;
    char *top_packet;
    char *parent_desc;
    char *parent;
    packet_desc *top_pack_desc;
    array_desc *arr_desc;
    static char *def_arrayname = "Intelligent Array";
    static char function_name[] = "iarray_get_from_multi_array";

    if (arrayname == NULL)
    {
	arrayname = def_arrayname;
    }
    /*  Determine which general data structure to use  */
    if ( (*multi_desc).num_arrays == 1 )
    {
	array_num = 0;
    }
    else
    {
	switch ( ds_f_array_name (multi_desc, arrayname,
				  (char **) NULL, &array_num) )
	{
	  case IDENT_NOT_FOUND:
	    (void) fprintf (stderr,
			    "Could not find general data structure: \"%s\"\n",
			    arrayname);
	    return (NULL);
/*
	    break;
*/
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr,
			    "Multiple general data structure name: \"%s\"\n",
			    arrayname);
	    return (NULL);
/*
	    break;
*/
	  case IDENT_GEN_STRUCT:
	    /*  Got what we wanted  */
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value from function: f_array_name\n");
	    a_prog_bug (function_name);
	    break;
	}
    }
    /*  Have number of array we are interested in  */
    top_pack_desc = (*multi_desc).headers[array_num];
    top_packet = (*multi_desc).data[array_num];
    if (num_dim == 0)
    {
	/*  Get array of any dimensionality  */
	for (elem_count = 0, match_index = (*top_pack_desc).num_elements;
	     elem_count < (*top_pack_desc).num_elements;
	     ++elem_count)
	{
	    if ( (*top_pack_desc).element_types[elem_count] != K_ARRAY )
	    {
		continue;
	    }
	    arr_desc =(array_desc *) (*top_pack_desc).element_desc[elem_count];
	    if (elem_name == NULL)
	    {
		/*  Case where no element name is specified: must have
		    single element array  */
		if ( (* (*arr_desc).packet ).num_elements == 1 )
		{
		    /*  Yup: fine.  */
		    /*  Match to any array  */
		    if (match_index < (*top_pack_desc).num_elements)
		    {
			/*  Match already found  */
			(void) fprintf (stderr,
					"Too many candidate arrays\n");
			return (NULL);
		    }
		    match_index = elem_count;
		}
		continue;
	    }
	    /*  Case where element name is specified: can get array
		provided element is found  */
	    if ( ( elem_index = ds_f_elem_in_packet ( (*arr_desc).packet,
						     elem_name ) )
		< (* (*arr_desc).packet ).num_elements )
	    /*  Found the requested one  */
	    return ( get_array_from_array
		    (multi_desc, array_num, arr_desc, *(char **)
		     (ds_get_element_offset (top_pack_desc,
					     elem_count) + top_packet),
		     elem_index) );
	}
	if (match_index >= (*top_pack_desc).num_elements)
	{
	    (void) fprintf (stderr, "No candidate arrays found\n");
	    return (NULL);
	}
	/*  Got the one and only decent match  */
	arr_desc = (array_desc *) (*top_pack_desc).element_desc[match_index];
	return ( get_array_from_array
		(multi_desc, array_num, arr_desc, *(char **)
		 (ds_get_element_offset (top_pack_desc, match_index) +
		  top_packet), 0) );
    }
    /*  Get array of specified dimensionality  */
    if (dim_names == NULL)
    {
	/*  Get any array of specified dimensionality  */
	for (elem_count = 0, match_index = (*top_pack_desc).num_elements;
	     elem_count < (*top_pack_desc).num_elements;
	     ++elem_count)
	{
	    if ( (*top_pack_desc).element_types[elem_count] != K_ARRAY )
	    {
		continue;
	    }
	    arr_desc =(array_desc *) (*top_pack_desc).element_desc[elem_count];
	    if ( (*arr_desc).num_dimensions != num_dim ) continue;
	    if (elem_name == NULL)
	    {
		/*  Case where no element name is specified: must have
		    single element array  */
		if ( (* (*arr_desc).packet ).num_elements == 1 )
		{
		    /*  Yup: fine.  */
		    /*  Match to any array  */
		    if (match_index < (*top_pack_desc).num_elements)
		    {
			/*  Match already found  */
			(void) fprintf (stderr,
					"Too many candidate arrays\n");
			return (NULL);
		    }
		    match_index = elem_count;
		}
		continue;
	    }
	    /*  Case where element name is specified: can get array
		provided element is found  */
	    if ( ( elem_index = ds_f_elem_in_packet ( (*arr_desc).packet,
						     elem_name ) )
		< (* (*arr_desc).packet ).num_elements )
	    {
		/*  Found the requested one  */
		return ( get_array_from_array
			(multi_desc, array_num, arr_desc, *(char **)
			 (ds_get_element_offset (top_pack_desc, elem_count) +
			  top_packet), elem_index) );
	    }
	}
	if (match_index >= (*top_pack_desc).num_elements)
	{
	    (void) fprintf (stderr, "No candidate arrays found\n");
	    return (NULL);
	}
	/*  Got the one and only decent match  */
	arr_desc = (array_desc *) (*top_pack_desc).element_desc[match_index];
	return ( get_array_from_array
		(multi_desc, array_num, arr_desc, *(char **)
		 (ds_get_element_offset (top_pack_desc, match_index) +
		  top_packet), 0) );
    }
    /*  Find unique occurrence of array  */
    switch ( ds_get_handle_in_packet (top_pack_desc, top_packet, dim_names[0],
				      (char **) NULL, (double *) NULL, 0,
				      &parent_desc, &parent, &parent_type,
				      &dim_index) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr,
			"Could not find dimension: \"%s\"\n", dim_names[0]);
	return (NULL);
/*
	break;
*/
      case IDENT_MULTIPLE:
	(void) fprintf (stderr, "Multiple instances of Intelligent Array\n");
	return (NULL);
/*
        break;
*/
      case IDENT_DIMENSION:
	/*  This is what we want  */
	break;
      case IDENT_ELEMENT:
	(void) fprintf (stderr,
			"Item: \"%s\" is an atomic element and not a dimension\n",
			dim_names[0]);
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
    arr_desc = (array_desc *) parent_desc;
    /*  Check number of dimensions  */
    if (num_dim != (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"Array with dimension: \"%s\" must have: %u dimensions\n",
			dim_names[0], num_dim);
	return (NULL);
    }
    /*  Allocate re-order list  */
    if ( ( reorder_indices = (unsigned int *)
	  m_alloc (sizeof *reorder_indices * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of re-ordering indices");
	return (NULL);
    }
    reorder_needed = FALSE;
    /*  Check for all dimension names  */
    for (dim_count = 0; dim_count < (*arr_desc).num_dimensions; ++dim_count)
    {
	if ( ( dim_index = ds_f_dim_in_array (arr_desc, dim_names[dim_count]) )
	    >= (*arr_desc).num_dimensions)
	{
	    (void) fprintf (stderr,
			    "Could not find dimension: \"%s\" with dimension: \"%s\"\n",
			    dim_names[dim_count], dim_names[0]);
	    m_free ( (char *) reorder_indices );
	    return (NULL);
	}
	if (dim_index != dim_count)
	{
	    reorder_needed = TRUE;
	}
	reorder_indices[dim_count] = dim_index;
    }
    /*  Check array packet descriptor  */
    if ( (* (*arr_desc).packet ).num_elements != 1 )
    {
	(void) fprintf (stderr,
			"Intelligent Array must have only one element\n");
	m_free ( (char *) reorder_indices );
	return (NULL);
    }
    if (elem_name != NULL)
    {
	if (ds_f_elem_in_packet ( (*arr_desc).packet, elem_name ) >=
	    (* (*arr_desc).packet ).num_elements)
	{
	    (void) fprintf (stderr,
			    "Intelligent array must have \"%s\" element\n",
			    elem_name);
	    m_free ( (char *) reorder_indices );
	    return (NULL);
	}
    }
    /*  Now that array checks out, transpose dimensions if necessary  */
    if (reorder_needed)
    {
	(void) fprintf (stderr, "Re-ordering array\n");
	if (ds_reorder_array (arr_desc, reorder_indices, parent, TRUE) != TRUE)
	{
	    (void) fprintf (stderr, "Error re-ordering Intelligent Array\n");
	    m_free ( (char *) reorder_indices );
	    return (NULL);
	}
    }
    m_free ( (char *) reorder_indices );
    /*  Everything is fine: get an "Intelligent Array" structure  */
    return ( get_array_from_array (multi_desc, array_num, arr_desc,parent,0) );
}   /*  End Function iarray_get_from_multi_array  */

/*PUBLIC_FUNCTION*/
void iarray_dealloc (array)
/*  This routine will deallocate an "Intelligent Array".
    If the environment variable "IARRAY_ALLOC_DEBUG" is set to "TRUE" then the
    routine will print deallocation debugging information.
    The array  must be given by  array  .
    The routine returns nothing.
*/
iarray array;
{
    unsigned int dim_count;
    multi_array *multi_desc;
    extern char *data_type_names[NUMTYPES];
    static char function_name[] = "iarray_dealloc";

    VERIFY_IARRAY (array);
    if ( (*array).offsets != (* (*array).arr_desc ).offsets )
    {
	/*  Specially allocated offsets  */
	for (dim_count = 0; dim_count < iarray_num_dim (array); ++dim_count)
	{
	    m_free ( (char *) ( (*array).offsets[dim_count] -
			       (*array).boundary_width ) );
	}
	m_free ( (char *) (*array).offsets );
    }
    multi_desc = (*array).multi_desc;
    if ( ( (*multi_desc).attachments == 0 ) && mem_debug_required () )
    {
	(void) fprintf (stderr, "iarray_dealloc: ");
	for (dim_count = 0; dim_count < iarray_num_dim (array) -1; ++dim_count)
	{
	    (void) fprintf (stderr, "%u * ", (*array).lengths[dim_count]);
	}
	(void) fprintf (stderr, "%u  type: %s\n",
			(*array).lengths[dim_count],
			data_type_names[iarray_type (array)]);
    }
    ds_dealloc_multi (multi_desc);
    m_free ( (char *) (*array).lengths );
    m_free ( (char *) (*array).contiguous );
    m_free ( (char *) array );
}   /*  End Function iarray_dealloc  */

/*PUBLIC_FUNCTION*/
flag iarray_put_named_value (array, name, type, value)
/*  This routine will add a unique named value to the underlying Karma general
    data structure of an "Intelligent Array".
    The array must be given by  array  .
    The name of the element must be pointed to by  name  .
    The type of the data which is to be written must be given by  type  .
    The value of the data must be pointed to by  value  .
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray array;
char *name;
unsigned int type;
double *value;
{
    static char function_name[] = "iarray_put_named_value";

    VERIFY_IARRAY (array);
    return ( ds_put_unique_named_value
	    ( (*array).top_pack_desc, (*array).top_packet,
	     name, type, value, TRUE ) );
}   /*  End Function iarray_put_named_value  */

/*PUBLIC_FUNCTION*/
flag iarray_put_named_string (array, name, string)
/*  This routine will add a unique named string to the underlying Karma general
    data structure of an "Intelligent Array".
    The array must be given by  array  .
    The name of the element must be pointed to by  name  .
    The string must be pointed to by  string  .
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray array;
char *name;
char *string;
{
    static char function_name[] = "iarray_put_named_string";

    VERIFY_IARRAY (array);
    return ( ds_put_unique_named_string
	    ( (*array).top_pack_desc, (*array).top_packet,
	     name, string, TRUE ) );
}   /*  End Function iarray_put_named_string  */

/*PUBLIC_FUNCTION*/
flag iarray_get_named_value (array, name, type, value)
/*  This routine will get a unique named value from the underlying Karma
    general data structure of an "Intelligent Array".
    The array must be given by  array  .
    The name of the element must be pointed to by  name  .
    The type of the data found will be written to the storage pointed to by
    type  .If this is NULL, nothing is written here.
    The value of the data will be written to the storage pointed to by  value
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray array;
char *name;
unsigned int *type;
double *value;
{
    static char function_name[] = "iarray_get_named_value";

    VERIFY_IARRAY (array);
    return ( ds_get_unique_named_value
	    ( (*array).top_pack_desc, *(*array).top_packet,
	     name, type, value ) );
}   /*  End Function iarray_get_named_value  */

/*PUBLIC_FUNCTION*/
char *iarray_get_named_string (array, name)
/*  This routine will get a unique named string from the underlying Karma
    general data structure of an "Intelligent Array".
    The array must be given by  array  .
    The name of the element must be pointed to by  name  .
    The routine returns a pointer to a dynamically allocated copy of the string
    on success, else it returns NULL.
*/
iarray array;
char *name;
{
    static char function_name[] = "iarray_get_named_string";

    VERIFY_IARRAY (array);
    return ( ds_get_unique_named_string
	    ( (*array).top_pack_desc, *(*array).top_packet, name ) );
}   /*  End Function iarray_get_named_string  */

/*PUBLIC_FUNCTION*/
flag iarray_copy_data (output, input, magnitude)
/*  This routine will copy data from one "Intelligent Array" to another. The
    sizes of the two arrays must be identical.
    The output array must be given by  output  .
    The input array must be given by  input  .
    The routine will automatically perform type conversions if necessary.
    If the value of  magnitude  is  TRUE  then when converting from a complex
    to a real data type, the magnitude is taken, else the real component is
    copied.
    Note that when converting from a real to a complex data type, the imaginary
    component is set to zero.
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray output;
iarray input;
flag magnitude;
{
    flag contiguous = TRUE;
    flag inp_complex;
    unsigned int elem_count;
    unsigned int inp_elem_size;
    unsigned int inp_stride;
    unsigned int out_stride;
    unsigned int dim_count;
    unsigned int num_dim;
    double data[2];
    char *inp_data;
    char *out_data;
    unsigned int *coordinates;
    double *buffer = NULL;
    double *ptr;
    array_desc *inp_arr;
    array_desc *out_arr;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "iarray_copy_data";

    VERIFY_IARRAY (output);
    VERIFY_IARRAY (input);
    inp_arr = (*input).arr_desc;
    out_arr = (*output).arr_desc;
    /*  Test array sizes  */
    if ( (num_dim = (*inp_arr).num_dimensions) != (*out_arr).num_dimensions )
    {
	(void) fprintf (stderr,
			"Input array has: %u dimensions whilst output array has: %u\n",
			(*inp_arr).num_dimensions,(*out_arr).num_dimensions);
	return (FALSE);
    }
    for (dim_count = 0; dim_count < (*inp_arr).num_dimensions; ++dim_count)
    {
	if ( (*input).lengths[dim_count] != (*output).lengths[dim_count] )
	{
	    (void) fprintf (stderr, "Input dimension: %u has length: %u\n",
			    dim_count, (*input).lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %u\n",
			    dim_count, (*output).lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
    }
    inp_stride = ds_get_packet_size ( (*inp_arr).packet );
    out_stride = ds_get_packet_size ( (*out_arr).packet );
    inp_elem_size =
    host_type_sizes[(*(*inp_arr).packet).element_types[(*input).elem_index]];
    /*  Check if lower dimensions are contiguous  */
    if ( (*input).contiguous[num_dim - 1] != TRUE )
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if ( (*output).contiguous[num_dim - 1] != TRUE )
    {
	/*  Output array not contiguous  */
	contiguous = FALSE;
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned int *) m_alloc (sizeof *coordinates *
						   num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of co-ordinate counters");
	return (FALSE);
    }
    m_clear ( (char *) coordinates, sizeof *coordinates * num_dim);
    if (contiguous)
    {
	/*  Lower dimensions are contiguous  */
	/*  Test to see if arrays have the same type  */
	if ( iarray_type (input) != iarray_type (output) )
	{
	    /*  Allocate conversion copy buffer  */
	    if ( ( buffer = (double *)
		  m_alloc (sizeof *buffer * 2 * (*input).lengths[num_dim -1]) )
		== NULL )
	    {
		m_error_notify (function_name, "conversion copy buffer");
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	}
    }
    /*  Iterate through the arrays  */
    inp_data = iarray_get_next_element (input, coordinates, 0);
    out_data = iarray_get_next_element (output, coordinates, 0);
    while (inp_data != NULL)
    {
	/*  More data to process  */
	if (contiguous)
	{
	    /*  Lower dimensions are contiguous  */
	    /*  Test to see if arrays have the same type  */
	    if ( iarray_type (input) == iarray_type (output) )
	    {
		/*  Do a fast copy  */
		m_copy_blocks (out_data, inp_data,
			       out_stride, inp_stride,
			       inp_elem_size,
			       (*input).lengths[num_dim - 1] -
			       coordinates[num_dim - 1]);
	    }
	    else
	    {
		/*  Have to convert data  */
		if (ds_get_elements (inp_data, iarray_type (input),
				     inp_stride, buffer, &inp_complex,
				     (*input).lengths[num_dim - 1])
		    != TRUE)
		{
		    m_free ( (char *) coordinates );
		    m_free ( (char *) buffer );
		    return (FALSE);
		}
		if ( (ds_element_is_complex ( iarray_type (output) ) != TRUE)
		    && inp_complex && magnitude )
		{
		    /*  Complex to real conversion  */
		    for (elem_count = 0, ptr = buffer;
			 elem_count < (*input).lengths[num_dim - 1];
			 ++elem_count, ptr += 2)
		    {
			*ptr = sqrt (ptr[0] * ptr[0] + ptr[1] * ptr[1]);
		    }
		}
		if (ds_put_elements (out_data, iarray_type (output),
				     out_stride, buffer,
				     (*input).lengths[num_dim - 1])
		    != TRUE)
		{
		    m_free ( (char *) coordinates );
		    m_free ( (char *) buffer );
		    return (FALSE);
		}
	    }
	    inp_data = iarray_get_next_element (input, coordinates,
						(*input).lengths[num_dim -
								 1]);
	    out_data = iarray_get_next_element (output, coordinates, 0);
	}
	else
	{
	    /*  Data is not contiguous: copy one element at a time  */
	    if (ds_get_element (inp_data, iarray_type (input), data,
				&inp_complex) != TRUE)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    if ( (ds_element_is_complex ( iarray_type (output) ) != TRUE)
		&& inp_complex && magnitude )
	    {
		/*  Complex to real conversion  */
		data[0] = sqrt (data[0] * data[0] + data[1] * data[1]);
	    }
	    if (ds_put_element (out_data, iarray_type (output), data) == NULL)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    inp_data = iarray_get_next_element (input, coordinates, 1);
	    out_data = iarray_get_next_element (output, coordinates, 0);
	}
    }
    m_free ( (char *) coordinates );
    if (buffer != NULL)
    {
	m_free ( (char *) buffer );
    }
    return (TRUE);
}   /*  End Function iarray_copy_data  */

/*PUBLIC_FUNCTION*/
char *iarray_get_element_1D (array, type, x)
/*  This routine will get an element from a simple 1 dimensional array.
    The array must be pointed to by  array  .
    The type of the element in the array must be given by  type  (this is used
    to enforce type checking).
    The lower array index must be given by  x  .
    The routine returns a pointer to the element.
*/
iarray array;
unsigned int type;
int x;
{
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "iarray_get_element_1D";

    VERIFY_IARRAY (array);
    if (iarray_num_dim (array) != 1)
    {
	(void) fprintf ( stderr,
			"Array has: %u dimensions: must have only 1\n",
			iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    if (iarray_type (array) != type)
    {
	(void) fprintf ( stderr,
			"Type requested: %u is not equal to type of array: %u\n",
			type, iarray_type (array) );
	a_prog_bug (function_name);
    }
    if (x < -(*array).boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d is less than -boundary_width: %d\n",
			x, -(*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (x >= (*array).lengths[0] - (*array).boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d exceeds dimension end: %d\n",
			x, (*array).lengths[0] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    return ( (*array).data + (*array).offsets[0][x] );
}   /*  End Function iarray_get_element_1D  */

/*PUBLIC_FUNCTION*/
char *iarray_get_element_2D (array, type, y, x)
/*  This routine will get an element from a simple 2 dimensional array.
    The array must be pointed to by  array  .
    The type of the element in the array must be given by  type  (this is used
    to enforce type checking).
    The upper array index must be given by  y  .
    The lower array index must be given by  x  .
    The routine returns a pointer to the element.
*/
iarray array;
unsigned int type;
int y;
int x;
{
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "iarray_get_element_2D";

    VERIFY_IARRAY (array);
    if (iarray_num_dim (array) != 2)
    {
	(void) fprintf ( stderr,
			"Array has: %u dimensions: must have only 2\n",
			iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    if (iarray_type (array) != type)
    {
	(void) fprintf ( stderr,
			"Type requested: %u is not equal to type of array: %u\n",
			type, iarray_type (array) );
	a_prog_bug (function_name);
    }
    if (x < -(*array).boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d is less than -boundary_width: %d\n",
			x, -(*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (x >= (*array).lengths[1] - (*array).boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d exceeds dimension end: %d\n",
			x, (*array).lengths[1] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (y < -(*array).boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d is less than -boundary_width: %d\n",
			y, -(*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (y >= (*array).lengths[0] - (*array).boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d exceeds dimension end: %d\n",
			y, (*array).lengths[0] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    return ( (*array).data + (*array).offsets[0][y] + (*array).offsets[1][x] );
}   /*  End Function iarray_get_element_2D  */

/*PUBLIC_FUNCTION*/
char *iarray_get_element_3D (array, type, z, y, x)
/*  This routine will get an element from a simple 3 dimensional array.
    The array must be pointed to by  array  .
    The type of the element in the array must be given by  type  (this is used
    to enforce type checking).
    The upper array index must be given by  z  .
    The middle array index must be given by  y  .
    The lower array index must be given by  x  .
    The routine returns a pointer to the element.
*/
iarray array;
unsigned int type;
int z;
int y;
int x;
{
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "iarray_get_element_3D";

    VERIFY_IARRAY (array);
    if (iarray_num_dim (array) != 3)
    {
	(void) fprintf ( stderr,
			"Array has: %u dimensions: must have only 3\n",
			iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    if (iarray_type (array) != type)
    {
	(void) fprintf ( stderr,
			"Type requested: %u is not equal to type of array: %u\n",
			type, iarray_type (array) );
	a_prog_bug (function_name);
    }
    if (x < -(*array).boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d is less than -boundary_width: %d\n",
			x, -(*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (x >= (*array).lengths[2] - (*array).boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d exceeds dimension end: %d\n",
			x, (*array).lengths[2] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (y < -(*array).boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d is less than -boundary_width: %d\n",
			y, -(*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (y >= (*array).lengths[1] - (*array).boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d exceeds dimension end: %d\n",
			y, (*array).lengths[1] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (z < -(*array).boundary_width)
    {
	(void) fprintf (stderr,
			"z coordinate: %d is less than -boundary_width: %d\n",
			z, -(*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (z >= (*array).lengths[0] - (*array).boundary_width)
    {
	(void) fprintf (stderr,
			"z coordinate: %d exceeds dimension end: %d\n",
			z, (*array).lengths[0] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    return ( (*array).data + (*array).offsets[0][z] + (*array).offsets[1][y] +
	    (*array).offsets[2][x] );
}   /*  End Function iarray_get_element_3D  */

/*PUBLIC_FUNCTION*/
iarray iarray_get_sub_array_2D (array, starty, startx, ylen, xlen)
/*  This function will create an "Intelligent Array" which is an alias or a
    sub-array of another "Intelligent Array". Subsequent modification of the
    sub-array will modify the data of the original array. Sub-arrays may be
    created from other sub-arrays. The attachment count of the underlying
    multi_array  data structure is incremented on successful completion.
    The original array must be given by  array  .
    The starting y (row) and x (column) indices which will indicate the origin
    of the new array must be given by  starty  and  startx  ,respectively.
    The size of the aliased array must be given by  ylen  and  xlen  .
    The routine returns a dynamically allocated intelligent array on success,
    else it returns NULL.
    NOTE: sub-arrays cannot be saved to disc.
*/
iarray array;
int starty;
int startx;
unsigned int ylen;
unsigned int xlen;
{
    int coord_count;
    iarray sub;
    static char function_name[] = "iarray_get_sub_array_2D";

    VERIFY_IARRAY (array);
    if (iarray_num_dim (array) != 2)
    {
	(void) fprintf ( stderr,
			"Input array has: %u dimensions, must have only 2\n",
			iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    if (starty < -(*array).boundary_width)
    {
	(void) fprintf (stderr,
			"starty: %d is less than -boundary_width: %d\n",
			starty, -(*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (starty >= (*array).lengths[0] - (*array).boundary_width)
    {
	(void) fprintf (stderr, "starty: %d exceeds dimension end: %d\n",
			starty, (*array).lengths[0] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (starty + ylen > (*array).lengths[0] - (*array).boundary_width)
    {
	(void) fprintf (stderr,
			"starty + ylen: %d exceeds dimension end: %d\n",
			starty + ylen,
			(*array).lengths[0] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (startx < -(*array).boundary_width)
    {
	(void) fprintf (stderr,
			"startx: %d is less than -boundary_width: %d\n",
			startx, -(*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (startx >= (*array).lengths[1] - (*array).boundary_width)
    {
	(void) fprintf (stderr, "startx: %d exceeds dimension end: %d\n",
			startx, (*array).lengths[1] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    if (startx + xlen > (*array).lengths[1] - (*array).boundary_width)
    {
	(void) fprintf (stderr,
			"startx + xlen: %d exceeds dimension end: %d\n",
			startx + xlen,
			(*array).lengths[1] - (*array).boundary_width);
	a_prog_bug (function_name);
    }
    if ( ( sub = (iarray) m_alloc (sizeof *sub) ) == NULL )
    {
	m_error_notify (function_name, "iarray");
    }
    if ( ( (*sub).lengths = (unsigned int *) m_alloc (sizeof *(*sub).lengths
						      * 2) )
	== NULL )
    {
	m_error_notify (function_name, "iarray");
	m_free ( (char *) sub );
    }
    (*sub).lengths[0] = ylen;
    (*sub).lengths[1] = xlen;
    (*sub).data = (*array).data;
    (*sub).array_num = (*array).array_num;
    (*sub).multi_desc = (*array).multi_desc;
    (*sub).top_pack_desc = (*array).top_pack_desc;
    (*sub).top_packet = (*array).top_packet;
    (*sub).arr_desc = (*array).arr_desc;
    if (iarray_allocate_records (sub, TRUE) != TRUE)
    {
	m_free ( (char *) (*sub).lengths );
	m_free ( (char *) sub );
	return (NULL);
    }
    /*  Copy over offset information  */
    for (coord_count = 0; coord_count < ylen; ++coord_count)
    {
	(*sub).offsets[0][coord_count] = (*array).offsets[0][coord_count +
							     starty];
    }
    for (coord_count = 0; coord_count < xlen; ++coord_count)
    {
	(*sub).offsets[1][coord_count] = (*array).offsets[1][coord_count +
							     startx];
    }
    /*  Copy contiguous flags  */
    m_copy ( (char *) (*sub).contiguous, (char *) (*array).contiguous,
	    sizeof (*sub).contiguous * 2 );
    /*  Increment attachment count  */
    ++(* (*array).multi_desc ).attachments;
    return (sub);
}   /*  End Function iarray_get_sub_array_2D  */

/*PUBLIC_FUNCTION*/
unsigned int iarray_dim_length (array, index)
/*  This routine will get the length of a specified dimension in a simple,
    n-dimensional array.
    The array must be given by  array  .
    The index of the dimension must be given by  index  .
    The routine returns the length of the specified dimension.
*/
iarray array;
unsigned int index;
{
    array_desc *arr_desc;
    static char function_name[] = "iarray_dim_length";

    VERIFY_IARRAY (array);
    arr_desc = (*array).arr_desc;
    if (index >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"Dimension index: %u is not less than number of dimensions: %u\n",
			index, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    return ( (*array).lengths[index] );
}   /*  End Function iarray_dim_length  */

/*PUBLIC_FUNCTION*/
flag iarray_fill (array, value)
/*  This routine will fill an "Intelligent Array" with a single value.
    The array must be given by  array  .
    The fill value must be pointed to by  value  .
    If filling a complex array, only the real component of the fill value will
    be used.
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray array;
double *value;
{
    unsigned int elem_size;
    unsigned int num_dim;
    unsigned int coord_count;
    unsigned int increment;
    char *data;
    char *val;
    unsigned int *coordinates;
    array_desc *arr_desc;
    static char function_name[] = "iarray_fill";

    VERIFY_IARRAY (array);
    arr_desc = (*array).arr_desc;
    num_dim = (*arr_desc).num_dimensions;
    elem_size = ds_get_packet_size ( (*arr_desc).packet );
    /*  Allocate value buffer  */
    if ( ( val = m_alloc (elem_size) ) == NULL )
    {
	m_error_notify (function_name, "value buffer");
	return (FALSE);
    }
    if (ds_put_element (val, iarray_type (array), value) == NULL)
    {
	m_free (val);
	return (FALSE);
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned int *) m_alloc (sizeof *coordinates *
						   num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of co-ordinate counters");
	m_free (val);
	return (FALSE);
    }
    m_clear ( (char *) coordinates, sizeof *coordinates * num_dim);
    increment = iarray_get_max_contiguous (array);
    /*  Iterate through the array  */
    data = iarray_get_next_element (array, coordinates, 0);
    while (data != NULL)
    {
	/*  More data to process  */
	m_fill (data, elem_size, val, elem_size, increment);
	data = iarray_get_next_element (array, coordinates, increment);
    }
    m_free ( (char *) coordinates );
    m_free (val);
    return (TRUE);
}   /*  End Function iarray_fill  */

/*PUBLIC_FUNCTION*/
flag iarray_min_max (array, conv_type, min, max)
/*  This routine will determine the minimum and maximum value of an
    "Intelligent Array".
    The array must be given by  array  .
    The conversion type to use for complex numbers must be given by  conv_type
    Legal value for this include:
        CONV1_REAL        CONV1_IMAG        CONV1_ABS        CONV1_SQUARE_ABS
	CONV1_PHASE       CONV1_CONT_PHASE  CONV1_ENVELOPE
    The routine will write the minimum value to the storage pointed to by  min
    The routine will write the maximum value to the storage pointed to by  max
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray array;
unsigned int conv_type;
double *min;
double *max;
{
    unsigned int elem_size;
    unsigned int num_dim;
    char *data;
    unsigned int *coordinates;
    array_desc *arr_desc;
    dim_desc *dim;
    static char function_name[] = "iarray_min_max";

    VERIFY_IARRAY (array);
    if ( (min == NULL) || (max == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    arr_desc = (*array).arr_desc;
    num_dim = (*arr_desc).num_dimensions;
    dim = (*arr_desc).dimensions[num_dim - 1];
    elem_size = ds_get_packet_size ( (*arr_desc).packet );
    *min = TOOBIG;
    *max = -TOOBIG;
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned int *) m_alloc (sizeof *coordinates *
						   num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of co-ordinate counters");
	return (FALSE);
    }
    m_clear ( (char *) coordinates, sizeof *coordinates * num_dim);
    /*  Iterate through the array  */
    data = iarray_get_next_element (array, coordinates, 0);
    while (data != NULL)
    {
	/*  More data to process  */
	if ( (*array).contiguous[num_dim - 1] )
	{
	    /*  Lowest dimension is contiguous  */
	    if (ds_find_single_extremes
		(data, iarray_type (array), conv_type, dim, elem_size,
		 (*dim).minimum,
		 ds_get_coordinate (dim, (*array).lengths[num_dim - 1] - 1),
		 min, max) != TRUE)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    data = iarray_get_next_element (array, coordinates,
					    (*array).lengths[num_dim - 1]);
	}
	else
	{
	    /*  Do it the slow way  */
	    if (ds_find_single_extremes (data, iarray_type (array), conv_type,
					 dim, elem_size,
					 (*dim).minimum, (*dim).minimum,
					 min, max) != TRUE)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    data = iarray_get_next_element (array, coordinates, 1);
	}
    }
    m_free ( (char *) coordinates );
    return (TRUE);
}   /*  End Function iarray_min_max  */

/*PUBLIC_FUNCTION*/
flag iarray_scale_and_offset (out, inp, scale, offset, magnitude)
/*  This routine will perform a scale and offset on every element in an
    "Intelligent Array".
    The output array must be given by  out  .
    The input array must be given by  inp  .
    NOTE: the input and output arrays MUST be the same size (though not
    necessarily the same type).
    The complex scale value must be pointed to by  scale  .
    The complex offset value must be pointed to by  offset  .
    When converting from a complex to a real array, the magnitude of the
    complex data (after scale and offset have been applied) is used if
    magnitude  is TRUE, else the real component of the complex scaled data is
    used.
    When converting from a real to a complex array, the imaginary component of
    the output array is unaffected (NOTE: it is NOT set to 0).
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray out;
iarray inp;
double *scale;
double *offset;
flag magnitude;
{
    flag contiguous = TRUE;
    flag inp_complex;
    unsigned int elem_count;
    unsigned int inp_elem_size;
    unsigned int out_elem_size;
    unsigned int dim_count;
    unsigned int num_dim;
    double data[2];
    double tmp[2];
    char *inp_data;
    char *out_data;
    unsigned int *coordinates;
    double *buffer = NULL;
    double *ptr;
    array_desc *inp_arr;
    array_desc *out_arr;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "iarray_scale_and_offset";

    VERIFY_IARRAY (inp);
    VERIFY_IARRAY (out);
    inp_arr = (*inp).arr_desc;
    out_arr = (*out).arr_desc;
    /*  Test array sizes  */
    if ( (num_dim = (*inp_arr).num_dimensions) != (*out_arr).num_dimensions )
    {
	(void) fprintf (stderr,
			"Input array has: %u dimensions whilst output array has: %u\n",
			(*inp_arr).num_dimensions,(*out_arr).num_dimensions);
	return (FALSE);
    }
    for (dim_count = 0; dim_count < (*inp_arr).num_dimensions; ++dim_count)
    {
	if ( (*inp).lengths[dim_count] != (*out).lengths[dim_count] )
	{
	    (void) fprintf (stderr, "Input dimension: %u has length: %u\n",
			    dim_count, (*inp).lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %u\n",
			    dim_count, (*out).lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
    }
    inp_elem_size = ds_get_packet_size ( (*inp_arr).packet );
    out_elem_size = ds_get_packet_size ( (*out_arr).packet );
    /*  Check if lower dimensions are contiguous  */
    if ( (*inp).contiguous[num_dim - 1] != TRUE )
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if ( (*out).contiguous[num_dim - 1] != TRUE )
    {
	/*  Output array not contiguous  */
	contiguous = FALSE;
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned int *) m_alloc (sizeof *coordinates *
						   num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of co-ordinate counters");
	return (FALSE);
    }
    m_clear ( (char *) coordinates, sizeof *coordinates * num_dim);
    if (contiguous)
    {
	/*  Lower dimensions are contiguous  */
	/*  Allocate conversion copy buffer  */
	if ( ( buffer = (double *)
	      m_alloc (sizeof *buffer * 2 * (*inp).lengths[num_dim - 1]) )
	    == NULL )
	{
	    m_error_notify (function_name, "conversion copy buffer");
	    m_free ( (char *) coordinates );
	    return (FALSE);
	}
    }
    /*  Iterate through the arrays  */
    inp_data = iarray_get_next_element (inp, coordinates, 0);
    out_data = iarray_get_next_element (out, coordinates, 0);
    while (inp_data != NULL)
    {
	/*  More data to process  */
	if (contiguous)
	{
	    /*  Lower dimensions are contiguous  */
	    if (ds_get_elements (inp_data, iarray_type (inp),
				 inp_elem_size, buffer, &inp_complex,
				 (*inp).lengths[num_dim - 1])
		!= TRUE)
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer );
		return (FALSE);
	    }
	    if (inp_complex)
	    {
		/*  Input array is complex  */
		for (elem_count = 0, ptr = buffer;
		     elem_count < (*inp).lengths[num_dim - 1];
		     ++elem_count, ptr += 2)
		{
		    tmp[0] = ptr[0] * scale[0] - ptr[1] * scale[1];
		    tmp[1] = ptr[0] * scale[1] + ptr[1] * scale[0];
		    ptr[0] = tmp[0] + offset[0];
		    ptr[1] = tmp[1] + offset[1];
		}
		if ( (ds_element_is_complex ( iarray_type (out) ) != TRUE)
		    && magnitude )
		{
		    /*  Complex to real conversion  */
		    for (elem_count = 0, ptr = buffer;
			 elem_count < (*inp).lengths[num_dim - 1];
			 ++elem_count, ptr += 2)
		    {
			*ptr = sqrt (ptr[0] * ptr[0] + ptr[1] * ptr[1]);
		    }
		}
	    }
	    else
	    {
		/*  Input array is real  */
		for (elem_count = 0, ptr = buffer;
		     elem_count < (*inp).lengths[num_dim - 1];
		     ++elem_count, ptr += 2)
		{
		    ptr[0] = ptr[0] * scale[0] + offset[0];
		}
	    }
	    if (ds_put_elements (out_data, iarray_type (out),
				 out_elem_size, buffer,
				 (*inp).lengths[num_dim - 1])
		!= TRUE)
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer );
		return (FALSE);
	    }
	    inp_data = iarray_get_next_element (inp, coordinates,
						(*inp).lengths[num_dim - 1]);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
	else
	{
	    /*  Data is not contiguous: copy one element at a time  */
	    if (ds_get_element (inp_data, iarray_type (inp), data,
				&inp_complex) != TRUE)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    if (inp_complex)
	    {
		/*  Input is complex  */
		tmp[0] = data[0] * scale[0] - data[1] * scale[1];
		tmp[1] = data[0] * scale[1] + data[1] * scale[0];
		data[0] = tmp[0] + offset[0];
		data[1] = tmp[1] + offset[1];
		if ( (ds_element_is_complex ( iarray_type (out) ) != TRUE)
		    && magnitude )
		{
		    /*  Complex to real conversion  */
		    data[0] = sqrt (data[0] * data[0] + data[1] * data[1]);
		}
	    }
	    else
	    {
		/*  Input is real  */
		data[0] = data[0] * scale[0] + offset[0];
	    }
	    if (ds_put_element (out_data, iarray_type (out), data) == NULL)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    inp_data = iarray_get_next_element (inp, coordinates, 1);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
    }
    m_free ( (char *) coordinates );
    if (buffer != NULL)
    {
	m_free ( (char *) buffer );
    }
    return (TRUE);
}   /*  End Function iarray_scale_and_offset  */

/*PUBLIC_FUNCTION*/
flag iarray_add_and_scale (out, inp1, inp2, scale, magnitude)
/*  This routine will add two "Intelligent Array" to each other and scales the
    result. The sizes of the two input arrays and the output must be identical.
    The output array must be given by  out  .
    The first input array must be given by  inp1  .
    The second input array must be given by  inp2  .
    The complex scale value must be pointed to by  scale  .
    The routine performs the following computation:
        OUT = INP1 + INP2 * scale
    The routine will automatically perform type conversions if necessary.
    If the value of  magnitude  is  TRUE  then when converting from a complex
    to a real data type, the magnitude is taken, else the real component is
    copied.
    Note that when converting from a real to a complex data type, the imaginary
    component is set to zero.
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray out;
iarray inp1;
iarray inp2;
double *scale;
flag magnitude;
{
    flag contiguous = TRUE;
    flag inp1_complex;
    flag inp2_complex;
    unsigned int elem_count;
    unsigned int inp1_elem_size;
    unsigned int inp2_elem_size;
    unsigned int out_elem_size;
    unsigned int dim_count;
    unsigned int num_dim;
    double data1[2];
    double data2[2];
    char *inp1_data;
    char *inp2_data;
    char *out_data;
    unsigned int *coordinates;
    double *buffer1 = NULL;
    double *buffer2 = NULL;
    double *ptr;
    array_desc *inp1_arr;
    array_desc *inp2_arr;
    array_desc *out_arr;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "iarray_add_and_scale";

    VERIFY_IARRAY (out);
    VERIFY_IARRAY (inp1);
    VERIFY_IARRAY (inp2);
    inp1_arr = (*inp1).arr_desc;
    inp2_arr = (*inp2).arr_desc;
    out_arr = (*out).arr_desc;
    /*  Test array sizes  */
    if ( (num_dim = (*inp1_arr).num_dimensions) != (*out_arr).num_dimensions )
    {
	(void) fprintf (stderr,
			"Input array1 has: %u dimensions whilst output array has: %u\n",
			(*inp1_arr).num_dimensions,(*out_arr).num_dimensions);
	return (FALSE);
    }
    if ( (*inp2_arr).num_dimensions != (*out_arr).num_dimensions )
    {
	(void) fprintf (stderr,
			"Input array2 has: %u dimensions whilst output array has: %u\n",
			(*inp2_arr).num_dimensions,(*out_arr).num_dimensions);
	return (FALSE);
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if ( (*inp1).lengths[dim_count] != (*out).lengths[dim_count] )
	{
	    (void) fprintf (stderr, "Input1 dimension: %u has length: %u\n",
			    dim_count, (*inp1).lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %u\n",
			    dim_count, (*out).lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
	if ( (*inp2).lengths[dim_count] != (*out).lengths[dim_count] )
	{
	    (void) fprintf (stderr, "Input2 dimension: %u has length: %u\n",
			    dim_count, (*inp2).lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %u\n",
			    dim_count, (*out).lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
    }
    inp1_elem_size = ds_get_packet_size ( (*inp1_arr).packet );
    inp2_elem_size = ds_get_packet_size ( (*inp2_arr).packet );
    out_elem_size = ds_get_packet_size ( (*out_arr).packet );
    /*  Check if lower dimensions are contiguous  */
    if ( (*inp1).contiguous[num_dim - 1] != TRUE )
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if ( (*inp2).contiguous[num_dim - 1] != TRUE )
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if ( (*out).contiguous[num_dim - 1] != TRUE )
    {
	/*  Output array not contiguous  */
	contiguous = FALSE;
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned int *) m_alloc (sizeof *coordinates *
						   num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of co-ordinate counters");
	return (FALSE);
    }
    m_clear ( (char *) coordinates, sizeof *coordinates * num_dim);
    if (contiguous)
    {
	/*  Lower dimensions are contiguous  */
	/*  Allocate conversion copy buffer  */
	if ( ( buffer1 = (double *)
	      m_alloc (sizeof *buffer1 * 2 * (*inp1).lengths[num_dim -1]) )
	    == NULL )
	{
	    m_error_notify (function_name, "conversion copy buffer1");
	    m_free ( (char *) coordinates );
	    return (FALSE);
	}
	if ( ( buffer2 = (double *)
	      m_alloc (sizeof *buffer2 * 2 * (*inp2).lengths[num_dim -1]) )
	    == NULL )
	{
	    m_error_notify (function_name, "conversion copy buffer2");
	    m_free ( (char *) coordinates );
	    return (FALSE);
	}
    }
    /*  Iterate through the arrays  */
    inp1_data = iarray_get_next_element (inp1, coordinates, 0);
    inp2_data = iarray_get_next_element (inp2, coordinates, 0);
    out_data = iarray_get_next_element (out, coordinates, 0);
    while (inp1_data != NULL)
    {
	/*  More data to process  */
	if (contiguous)
	{
	    /*  Lower dimensions are contiguous  */
	    if (ds_get_elements (inp1_data, iarray_type (inp1),
				 inp1_elem_size, buffer1, &inp1_complex,
				 (*inp1).lengths[num_dim - 1])
		!= TRUE)
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    if (ds_get_elements (inp2_data, iarray_type (inp2),
				 inp2_elem_size, buffer2, &inp2_complex,
				 (*inp2).lengths[num_dim - 1])
		!= TRUE)
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    for (elem_count = 0; elem_count < (*inp1).lengths[num_dim - 1];
		 ++elem_count)
	    {
		buffer1[elem_count * 2] += buffer2[elem_count * 2];
		buffer1[elem_count * 2 + 1] += buffer2[elem_count * 2 + 1];
	    }
	    if ( (ds_element_is_complex ( iarray_type (out) ) != TRUE)
		&& inp1_complex && inp2_complex && magnitude )
	    {
		/*  Complex to real conversion  */
		for (elem_count = 0, ptr = buffer1;
		     elem_count < (*inp1).lengths[num_dim - 1];
		     ++elem_count, ptr += 2)
		{
		    *ptr = sqrt (ptr[0] * ptr[0] + ptr[1] * ptr[1]);
		}
	    }
	    if (ds_put_elements (out_data, iarray_type (out),
				 out_elem_size, buffer1,
				 (*out).lengths[num_dim - 1])
		!= TRUE)
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    inp1_data = iarray_get_next_element (inp1, coordinates,
						 (*inp1).lengths[num_dim -
								 1]);
	    inp2_data = iarray_get_next_element (inp2, coordinates,
						 (*inp2).lengths[num_dim -
								 1]);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
	else
	{
	    /*  Data is not contiguous: copy one element at a time  */
	    if (ds_get_element (inp1_data, iarray_type (inp1), data1,
				&inp1_complex) != TRUE)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    if (ds_get_element (inp2_data, iarray_type (inp2), data2,
				&inp1_complex) != TRUE)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    data1[0] += data2[0];
	    data1[1] += data2[1];
	    if ( (ds_element_is_complex ( iarray_type (out) ) != TRUE)
		&& inp1_complex && inp2_complex && magnitude )
	    {
		/*  Complex to real conversion  */
		data1[0] = sqrt (data1[0] * data1[0] + data1[1] * data1[1]);
	    }
	    if (ds_put_element (out_data, iarray_type (out), data1) == NULL)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    inp1_data = iarray_get_next_element (inp1, coordinates, 1);
	    inp2_data = iarray_get_next_element (inp2, coordinates, 0);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
    }
    m_free ( (char *) coordinates );
    if (buffer1 != NULL)
    {
	m_free ( (char *) buffer1 );
    }
    if (buffer2 != NULL)
    {
	m_free ( (char *) buffer2 );
    }
    return (TRUE);
}   /*  End Function iarray_add_and_scale  */

/*PUBLIC_FUNCTION*/
flag iarray_sub_and_scale (out, inp1, inp2, scale, magnitude)
/*  This routine will subtract two "Intelligent Array" from each other and
    scales the result. The sizes of the two input arrays and the output must be
    identical.
    The output array must be given by  out  .
    The first input array must be given by  inp1  .
    The second input array must be given by  inp2  .
    The complex scale value must be pointed to by  scale  .
    The routine performs the following computation:
        OUT = INP1 - INP2 * scale
    The routine will automatically perform type conversions if necessary.
    If the value of  magnitude  is  TRUE  then when converting from a complex
    to a real data type, the magnitude is taken, else the real component is
    copied.
    Note that when converting from a real to a complex data type, the imaginary
    component is set to zero.
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray out;
iarray inp1;
iarray inp2;
double *scale;
flag magnitude;
{
    flag contiguous = TRUE;
    flag inp1_complex;
    flag inp2_complex;
    unsigned int elem_count;
    unsigned int inp1_elem_size;
    unsigned int inp2_elem_size;
    unsigned int out_elem_size;
    unsigned int dim_count;
    unsigned int num_dim;
    double data1[2];
    double data2[2];
    char *inp1_data;
    char *inp2_data;
    char *out_data;
    unsigned int *coordinates;
    double *buffer1 = NULL;
    double *buffer2 = NULL;
    double *ptr;
    array_desc *inp1_arr;
    array_desc *inp2_arr;
    array_desc *out_arr;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "iarray_sub_and_scale";

    VERIFY_IARRAY (out);
    VERIFY_IARRAY (inp1);
    VERIFY_IARRAY (inp2);
    inp1_arr = (*inp1).arr_desc;
    inp2_arr = (*inp2).arr_desc;
    out_arr = (*out).arr_desc;
    /*  Test array sizes  */
    if ( (num_dim = (*inp1_arr).num_dimensions) != (*out_arr).num_dimensions )
    {
	(void) fprintf (stderr,
			"Input array1 has: %u dimensions whilst output array has: %u\n",
			(*inp1_arr).num_dimensions,(*out_arr).num_dimensions);
	return (FALSE);
    }
    if ( (*inp2_arr).num_dimensions != (*out_arr).num_dimensions )
    {
	(void) fprintf (stderr,
			"Input array2 has: %u dimensions whilst output array has: %u\n",
			(*inp2_arr).num_dimensions,(*out_arr).num_dimensions);
	return (FALSE);
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if ( (*inp1).lengths[dim_count] != (*out).lengths[dim_count] )
	{
	    (void) fprintf (stderr, "Input1 dimension: %u has length: %u\n",
			    dim_count, (*inp1).lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %u\n",
			    dim_count, (*out).lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
	if ( (*inp2).lengths[dim_count] != (*out).lengths[dim_count] )
	{
	    (void) fprintf (stderr, "Input2 dimension: %u has length: %u\n",
			    dim_count, (*inp2).lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %u\n",
			    dim_count, (*out).lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
    }
    inp1_elem_size = ds_get_packet_size ( (*inp1_arr).packet );
    inp2_elem_size = ds_get_packet_size ( (*inp2_arr).packet );
    out_elem_size = ds_get_packet_size ( (*out_arr).packet );
    /*  Check if lower dimensions are contiguous  */
    if ( (*inp1).contiguous[num_dim - 1] != TRUE )
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if ( (*inp2).contiguous[num_dim - 1] != TRUE )
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if ( (*out).contiguous[num_dim - 1] != TRUE )
    {
	/*  Output array not contiguous  */
	contiguous = FALSE;
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned int *) m_alloc (sizeof *coordinates *
						   num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of co-ordinate counters");
	return (FALSE);
    }
    m_clear ( (char *) coordinates, sizeof *coordinates * num_dim);
    if (contiguous)
    {
	/*  Lower dimensions are contiguous  */
	/*  Allocate conversion copy buffer  */
	if ( ( buffer1 = (double *)
	      m_alloc (sizeof *buffer1 * 2 * (*inp1).lengths[num_dim -1]) )
	    == NULL )
	{
	    m_error_notify (function_name, "conversion copy buffer1");
	    m_free ( (char *) coordinates );
	    return (FALSE);
	}
	if ( ( buffer2 = (double *)
	      m_alloc (sizeof *buffer2 * 2 * (*inp2).lengths[num_dim -1]) )
	    == NULL )
	{
	    m_error_notify (function_name, "conversion copy buffer2");
	    m_free ( (char *) coordinates );
	    return (FALSE);
	}
    }
    /*  Iterate through the arrays  */
    inp1_data = iarray_get_next_element (inp1, coordinates, 0);
    inp2_data = iarray_get_next_element (inp2, coordinates, 0);
    out_data = iarray_get_next_element (out, coordinates, 0);
    while (inp1_data != NULL)
    {
	/*  More data to process  */
	if (contiguous)
	{
	    /*  Lower dimensions are contiguous  */
	    if (ds_get_elements (inp1_data, iarray_type (inp1),
				 inp1_elem_size, buffer1, &inp1_complex,
				 (*inp1).lengths[num_dim - 1])
		!= TRUE)
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    if (ds_get_elements (inp2_data, iarray_type (inp2),
				 inp2_elem_size, buffer2, &inp2_complex,
				 (*inp2).lengths[num_dim - 1])
		!= TRUE)
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    for (elem_count = 0; elem_count < (*inp1).lengths[num_dim - 1];
		 ++elem_count)
	    {
		buffer1[elem_count * 2] -= buffer2[elem_count * 2];
		buffer1[elem_count * 2 + 1] -= buffer2[elem_count * 2 + 1];
	    }
	    if ( (ds_element_is_complex ( iarray_type (out) ) != TRUE)
		&& inp1_complex && inp2_complex && magnitude )
	    {
		/*  Complex to real conversion  */
		for (elem_count = 0, ptr = buffer1;
		     elem_count < (*inp1).lengths[num_dim - 1];
		     ++elem_count, ptr += 2)
		{
		    *ptr = sqrt (ptr[0] * ptr[0] + ptr[1] * ptr[1]);
		}
	    }
	    if (ds_put_elements (out_data, iarray_type (out),
				 out_elem_size, buffer1,
				 (*out).lengths[num_dim - 1])
		!= TRUE)
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    inp1_data = iarray_get_next_element (inp1, coordinates,
						 (*inp1).lengths[num_dim -
								 1]);
	    inp2_data = iarray_get_next_element (inp2, coordinates,
						 (*inp2).lengths[num_dim -
								 1]);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
	else
	{
	    /*  Data is not contiguous: copy one element at a time  */
	    if (ds_get_element (inp1_data, iarray_type (inp1), data1,
				&inp1_complex) != TRUE)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    if (ds_get_element (inp2_data, iarray_type (inp2), data2,
				&inp1_complex) != TRUE)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    data1[0] -= data2[0];
	    data1[1] -= data2[1];
	    if ( (ds_element_is_complex ( iarray_type (out) ) != TRUE)
		&& inp1_complex && inp2_complex && magnitude )
	    {
		/*  Complex to real conversion  */
		data1[0] = sqrt (data1[0] * data1[0] + data1[1] * data1[1]);
	    }
	    if (ds_put_element (out_data, iarray_type (out), data1) == NULL)
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    inp1_data = iarray_get_next_element (inp1, coordinates, 1);
	    inp2_data = iarray_get_next_element (inp2, coordinates, 0);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
    }
    m_free ( (char *) coordinates );
    if (buffer1 != NULL)
    {
	m_free ( (char *) buffer1 );
    }
    if (buffer2 != NULL)
    {
	m_free ( (char *) buffer2 );
    }
    return (TRUE);
}   /*  End Function iarray_sub_and_scale  */

/*PUBLIC_FUNCTION*/
char *iarray_dim_name (array, index)
/*  This routine will get the name of a specified dimension in a simple,
    n-dimensional array.
    The array must be given by  array  .
    The index of the dimension must be given by  index  .
    The routine returns a pointer to the name of the specified dimension.
*/
iarray array;
unsigned int index;
{
    array_desc *arr_desc;
    static char function_name[] = "iarray_dim_name";

    VERIFY_IARRAY (array);
    arr_desc = (*array).arr_desc;
    if (index >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"Dimension index: %u is not less than number of dimensions: %u\n",
			index, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    return ( (* (* (*array).arr_desc ).dimensions[index] ).name );
}   /*  End Function iarray_dim_name  */

/*PUBLIC_FUNCTION*/
void iarray_remap_torus (array, boundary_width)
/*  This routine will remap an N-dimensional "Intelligent Array" to a pseudo
    toroidal array.
    The array must be given by  array  .
    The width of the array boundary within which the array appears to be
    toroidal must be given by  boundary_width  .
    The routine returns nothing.
*/
iarray array;
unsigned int boundary_width;
{
    unsigned int dim_count;
    unsigned int *offsets;
    unsigned int **off_ptr;
    static char function_name[] = "iarray_remap_torus";

    VERIFY_IARRAY (array);
    if ( (*array).offsets == (* (*array).arr_desc ).offsets )
    {
	/*  Need a new array of pointers  */
	if ( ( off_ptr = (unsigned int **) m_alloc ( sizeof *off_ptr *
						    iarray_num_dim (array) ) )
	    == NULL )
	{
	    m_abort (function_name, "array of address offset array pointers");
	}
	m_copy ( (char *) off_ptr, (char *) (*array).offsets,
		sizeof *off_ptr * iarray_num_dim (array) );
	(*array).offsets = off_ptr;
    }
    for (dim_count = 0; dim_count < iarray_num_dim (array); ++dim_count)
    {
	if ( (*array).offsets[dim_count] == NULL )
	{
	    (void) fprintf (stderr, "No address offsets for dimension: %u\n",
			    dim_count);
	    a_prog_bug (function_name);
	}
	if ( ( offsets = (unsigned int *)
	      m_alloc ( sizeof *offsets *
		       ( (*array).lengths[dim_count] + 2 * boundary_width ) ) )
	    == NULL )
	{
	    m_abort (function_name, "address offset array");
	}
	/*  Copy old offsets  */
	m_copy ( (char *) (offsets + boundary_width),
		(char *) (*array).offsets[dim_count],
		sizeof *offsets * (*array).lengths[dim_count] );
	/*  Copy old end to new beginning  */
	m_copy ( (char *) offsets,
		(char *) ( (*array).offsets[dim_count] +
			  (*array).lengths[dim_count] - boundary_width ),
		sizeof *offsets * boundary_width );
	/*  Copy old beginning to new end  */
	m_copy ( (char *) (offsets + (*array).lengths[dim_count] +
			   boundary_width),
		(char *) (*array).offsets[dim_count],
		sizeof *offsets * boundary_width );
	m_free ( (char *) ( (*array).offsets[dim_count] -
			   (*array).boundary_width ) );
	(*array).offsets[dim_count] = offsets + boundary_width;
	(*array).contiguous[dim_count] = FALSE;
    }
    (*array).boundary_width = boundary_width;
}   /*  End Function iarray_remap_torus  */

/*PUBLIC_FUNCTION*/
void iarray_set_world_coords (array, index, minimum, maximum)
/*  This routine will set the world co-ordinates of a specified dimension in a
    simple, n-dimensional array.
    The array must be given by  array  .
    The index of the dimension must be given by  index  .
    The minimum real world co-ordinate must be given by  minimum  .
    The maximum real world co-ordinate must be given by  maximum  .
    The routine returns nothing.
*/
iarray array;
unsigned int index;
double minimum;
double maximum;
{
    array_desc *arr_desc;
    static char function_name[] = "iarray_set_world_coords";

    VERIFY_IARRAY (array);
    arr_desc = (*array).arr_desc;
    if (index >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"Dimension index: %u is not less than number of dimensions: %u\n",
			index, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if (minimum >= maximum)
    {
	(void) fprintf (stderr, "Minimum: %e  is not less than maximum: %e\n",
			minimum, maximum);
	a_prog_bug (function_name);
    }
    (* (*arr_desc).dimensions[index] ).minimum = minimum;
    (* (*arr_desc).dimensions[index] ).maximum = maximum;
}   /*  End Function iarray_set_world_coords  */

/*PUBLIC_FUNCTION*/
void iarray_get_world_coords (array, index, minimum, maximum)
/*  This routine will get the world co-ordinates of a specified dimension in a
    simple, n-dimensional array.
    The array must be given by  array  .
    The index of the dimension must be given by  index  .
    The minimum real world co-ordinate will be written to the storage pointed
    to by  minimum  .
    The maximum real world co-ordinate will be written to the storage pointed
    to by  maximum  .
    The routine returns nothing.
*/
iarray array;
unsigned int index;
double *minimum;
double *maximum;
{
    array_desc *arr_desc;
    static char function_name[] = "iarray_get_world_coords";

    VERIFY_IARRAY (array);
    arr_desc = (*array).arr_desc;
    if (index >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"Dimension index: %u is not less than number of dimensions: %u\n",
			index, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if ( (minimum == NULL) || (maximum == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    *minimum = (* (*arr_desc).dimensions[index] ).minimum;
    *maximum = (* (*arr_desc).dimensions[index] ).maximum;
}   /*  End Function iarray_get_world_coords  */


/*  Private functions follow  */

static iarray get_array_from_array (multi_desc, array_num, arr_desc, array,
				    elem_index)
/*  This routine will create an "Intelligent Array" structure from a Karma,
    n-dimensional array.
    The multi array header must be pointed to by  multi_desc  .The attachment
    count is incremented on successful completion of this routine.
    The index of the general data structure which contains the
    "Intelligent Array" must be given by  array_num  .
    The array descriptor must be pointed to by  arr_desc  .
    The array data must be pointed to by  array  .
    The index of the element in the array packet descriptor must be goven by
    elem_index  .
    The routine returns a dynamically allocated intelligent array on success,
    else it prints an error message to the standard output and returns NULL.
*/
multi_array *multi_desc;
unsigned int array_num;
array_desc *arr_desc;
char *array;
unsigned int elem_index;
{
    unsigned int dim_count;
    unsigned int coord_count;
    unsigned int stride;
    iarray new_array;
    dim_desc *dim;
    static char function_name[] = "get_array_from_array";

    if ( (*arr_desc).offsets == NULL )
    {
	/*  No offsets yet: compute  */
	if (ds_compute_array_offsets (arr_desc) != TRUE)
	{
	    m_error_notify (function_name, "offset arrays");
	    return (NULL);
	}
    }
    if (elem_index >= (* (*arr_desc).packet ).num_elements)
    {
	(void) fprintf (stderr,
			"elem_index: %u  is not less than num elements: %u\n",
			elem_index, (* (*arr_desc).packet ).num_elements);
	a_prog_bug (function_name);
    }
    if ( ( new_array = (iarray) m_alloc (sizeof *new_array) )
	== NULL )
    {
	m_error_notify (function_name, "Intelligent Array structure");
	return (NULL);
    }
    if ( ( (*new_array).lengths = (unsigned int *)
	  m_alloc (sizeof *(*new_array).lengths * (*arr_desc).num_dimensions) )
	== NULL )
    {
	m_error_notify (function_name, "array of dimension lengths");
	m_free ( (char *) new_array );
	return (NULL);
    }
    for (dim_count = 0; dim_count < (*arr_desc).num_dimensions; ++dim_count)
    {
	dim = (*arr_desc).dimensions[dim_count];
	(*new_array).lengths[dim_count] = (*dim).length;
    }
    /*  Write the other information  */
    (*new_array).data = array + ds_get_element_offset ( (*arr_desc).packet,
						       elem_index );
    (*new_array).array_num = array_num;
    (*new_array).multi_desc = multi_desc;
    (*new_array).top_pack_desc = (*multi_desc).headers[array_num];
    (*new_array).top_packet = &(*multi_desc).data[array_num];
    (*new_array).arr_desc = arr_desc;
    (*new_array).elem_index = elem_index;
    if (iarray_allocate_records (new_array, FALSE) != TRUE)
    {
	m_free ( (char *) (*new_array).lengths );
	m_free ( (char *) new_array );
	return (NULL);
    }
    (*new_array).offsets = (*arr_desc).offsets;
    for (dim_count = 0; dim_count < (*arr_desc).num_dimensions; ++dim_count)
    {
	if ( (*arr_desc).num_levels > 0 )
	{
	    /*  Tiled  */
	    (*new_array).contiguous[dim_count] = FALSE;
	}
	else
	{
	    /*  Not tiled  */
	    (*new_array).contiguous[dim_count] = TRUE;
	}
    }
    ++(*multi_desc).attachments;
    return (new_array);
}   /*  End Function get_array_from_array  */

static flag iarray_allocate_records (array, offsets)
/*  This routine will allocate the array offsets and origins. The dimension
    lengths must be contained within the array.
    The array must be given by  array  .
    If the value of  offsets  is TRUE, then the routine will allocate space for
    the address offset arrays.
    The routine returns TRUE on success, else it returns FALSE.
*/
iarray array;
flag offsets;
{
    unsigned int dim_count;
    unsigned int num_dim;
    unsigned int *lengths;
    array_desc *arr_desc;
    static char function_name[] = "iarray_allocate_records";

    VERIFY_IARRAY (array);
    (*array).boundary_width = 0;
    arr_desc = (*array).arr_desc;
    num_dim = (*arr_desc).num_dimensions;
    lengths = (*array).lengths;
    if (offsets)
    {
	if ( ( (*array).offsets = (unsigned int **)
	      m_alloc (sizeof *(*array).offsets * num_dim) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of offset pointers");
	    return (FALSE);
	}
    }
    else
    {
	(*array).offsets = NULL;
    }
    if ( ( (*array).contiguous = (flag *)
	  m_alloc (sizeof *(*array).contiguous * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of offset pointers");
	return (FALSE);
    }
    if (!offsets)
    {
	/*  No offsets need be allocated: finished now  */
	return (TRUE);
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if ( ( (*array).offsets[dim_count] = (unsigned int *)
	      m_alloc (sizeof **(*array).offsets * lengths[dim_count]) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of offsets");
	    while (dim_count-- > 0)
	    {
		m_free ( (char *) (*array).offsets[dim_count] );
	    }
	    m_free ( (char *) (*array).offsets );
	    m_free ( (char *) (*array).contiguous );
	    m_free ( (char *) array );
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function iarray_allocate_records  */

static char *iarray_get_next_element (array, coordinates, increment)
/*  This routine will increment a co-ordinate counter for an array.
    The array must be given by  array  .
    The co-ordinate counters must be pointed to by  coordinates  .
    The increment (of the lowest dimension) must be given by  increment  .
    The routine returns a pointer to the element on success, else it returns
    NULL indicating that the end of the array has been reached.
*/
iarray array;
unsigned int *coordinates;
unsigned int increment;
{
    unsigned int dim_count;
    char *data;
    static char function_name[] = "iarray_get_next_element";

    VERIFY_IARRAY (array);
    dim_count = (* (*array).arr_desc ).num_dimensions - 1;
    coordinates[dim_count] += increment;
    if (coordinates[dim_count] < (*array).lengths[dim_count])
    {
	/*  Have valid co-ordinate  */
	data = (*array).data;
	for (dim_count = 0; dim_count < (* (*array).arr_desc ).num_dimensions;
	     ++dim_count)
	{
	    data += (*array).offsets[dim_count][ coordinates[dim_count] ];
	}
	return (data);
    }
    /*  Have exceeded lowest dimension bounds  */
    increment = coordinates[dim_count] / (*array).lengths[dim_count];
    coordinates[dim_count] %= (*array).lengths[dim_count];
    while (dim_count-- > 0)
    {
	if ( (coordinates[dim_count] += increment) <
	    (*array).lengths[dim_count] )
	{
	    /*  Have valid co-ordinate  */
	    data = (*array).data;
	    for (dim_count = 0;
		 dim_count < (* (*array).arr_desc ).num_dimensions;
		 ++dim_count)
	    {
		data += (*array).offsets[dim_count][ coordinates[dim_count] ];
	    }
	    return (data);
	}
	increment = coordinates[dim_count] / (*array).lengths[dim_count];
	coordinates[dim_count] %= (*array).lengths[dim_count];
    }
    return (NULL);
}   /*  End Function iarray_get_next_element  */

static unsigned int iarray_get_max_contiguous (array)
/*  This routine will get the maximum number of contiguous elements in an
    "Intelligent Array".
    The array must be given by  array  .
    The routine returns the number of contiguous elements.
*/
iarray array;
{
    unsigned int max_contig;
    int dim_count;
    dim_desc *dim;
    static char function_name[] = "iarray_get_max_contiguous";

    VERIFY_IARRAY (array);
    max_contig = 1;
    dim_count = iarray_num_dim (array);
    while (--dim_count >= 0)
    {
	dim = (* (*array).arr_desc ).dimensions[dim_count];
	/*  Check if dimension is contiguous  */
	if ( (*array).contiguous[dim_count] )
	{
	    /*  Elements along dimension are contiguous  */
	    max_contig *= (*array).lengths[dim_count];
	    if ( (*array).lengths[dim_count] == (*dim).length )
	    {
		/*  Dimension is full length  */
		continue;
	    }
	}
	return (max_contig);
    }
    return (max_contig);
}   /*  End Function iarray_get_max_contiguous  */

static flag mem_debug_required ()
/*  This routine will determine if debugging information is required.
    The routine returns TRUE if debugging information is required,
    else it returns FALSE.
*/
{
    char *env;
    static flag checked = FALSE;
    static flag debug = FALSE;

    if (checked) return (debug);
    /*  Determine if debug needed  */
    checked = TRUE;
    if ( ( ( env = r_getenv ("IARRAY_ALLOC_DEBUG") ) != NULL ) &&
	(st_icmp (env, "TRUE") == 0) )
    {
	debug = TRUE;
	(void) fprintf (stderr,
			"Running iarray_create and iarray_dealloc with debugging\n");
    }
    return (debug);
}   /*  End Function mem_debug_required  */
