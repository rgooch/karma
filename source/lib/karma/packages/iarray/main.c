/*LINTLIBRARY*/
/*  main.c

    This code provides the Intelligent Array core interface to Karma data
    structures.

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

    Updated by      Richard Gooch   10-OCT-1993: Fixed bug in
  iarray_get_from_multi_array  which printed NULL string on error.

    Updated by      Richard Gooch   4-DEC-1993: Added support for slice
  aliasing and fixed bugs in  iarray_add_and_scale  and  iarray_sub_and_scale

    Updated by      Richard Gooch   5-DEC-1993: Added  iarray_get_restrictions

    Updated by      Richard Gooch   29-MAR-1994: Added  iarray_get_dim_desc  .

    Updated by      Richard Gooch   10-MAY-1994: Improved documentation for
  iarray_scale_and_offset  .

    Updated by      Richard Gooch   20-JUL-1994: Added some CONST casts.

    Updated by      Richard Gooch   6-AUG-1994: Moved to full ASCII function
  declaration syntax. Had to do this because of the confused Dec Alpha compiler

    Updated by      Richard Gooch   8-AUG-1994: Changed to  uaddr  type.

    Updated by      Richard Gooch   14-AUG-1994: Changed return type of
  iarray_dim_length  from  unsigned int  to  unsigned long  .

    Updated by      Richard Gooch   11-OCT-1994: Fixed bug in
  iarray_get_next_element  and other routines which used number of dimensions
  in the underlying array rather than number of Intelligent Array dimensions.

    Updated by      Richard Gooch   14-OCT-1994: Fixed bug in
  iarray_get_2D_slice_from_3D  which didn't copy element index.

    Uupdated by     Richard Gooch   23-OCT-1994: Created
  iarray_compute_histogram  routine.

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/iarray/main.c

    Updated by      Richard Gooch   18-JAN-1995: Added  destroy_callbacks  and
  magic_number  fields.

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   7-MAY-1995: Placate gcc -Wall

    Updated by      Richard Gooch   24-JUL-1995: Created
  <iarray_get_element_4D>.

    Updated by      Richard Gooch   13-DEC-1995: Unpublished
  <iarray_get_element_*D> routines.

    Updated by      Richard Gooch   25-JAN-1996: Threaded <iarray_min_max> and
  <iarray_compute_histogram> routines.

    Updated by      Richard Gooch   1-APR-1996: Moved remaing functions to new
  documentation style.

    Updated by      Richard Gooch   6-APR-1996: Referenced <karma_data_types>
  table.

    Updated by      Richard Gooch   29-APR-1996: Added tiny offset to
  <iarray_scale_and_offset> when converting to integer arrays.

    Updated by      Richard Gooch   16-MAY-1996: Created
  <iarray_clip_scale_and_offset>.

    Updated by      Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   3-JUN-1996: Took account of new fields in
  dimension descriptor for first and last co-ordinate.

    Last updated by Richard Gooch   14-JUN-1996: Changed more pointers to
  CONST.


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_mt.h>
#include <karma_m.h>
#include <karma_r.h>
#include <karma_a.h>
#include <karma_c.h>


#define MAGIC_NUMBER 939032982

#define VERIFY_IARRAY(array) if (array == NULL) \
{(void) fprintf (stderr, "NULL iarray passed\n"); \
 a_prog_bug (function_name); } \
if (array->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid iarray\n"); \
 a_prog_bug (function_name); }


/*  Private structures  */
typedef struct
{
    unsigned int conv_type;
    double min;
    double max;
} min_max_thread_info;

typedef struct
{
    unsigned int conv_type;
    double min;
    double max;
    unsigned int num_bins;
} histogram_finfo;


/*  Private functions  */
STATIC_FUNCTION (iarray get_array_from_array,
		 (multi_array *multi_desc, unsigned int array_num,
		  array_desc *arr_desc, char *array,unsigned int elem_index) );
STATIC_FUNCTION (flag iarray_allocate_records, (iarray array, flag offsets) );
STATIC_FUNCTION (char *iarray_get_next_element,
		 (iarray array, unsigned long *coordinates,
		  unsigned int increment) );
STATIC_FUNCTION (unsigned int iarray_get_max_contiguous, (iarray array) );
STATIC_FUNCTION (flag iarray_is_full_array, (iarray array) );
STATIC_FUNCTION (flag mem_debug_required, () );
STATIC_FUNCTION (void initialise_thread_pool, () );
/*  Threaded functions requiring address offsets  */
STATIC_FUNCTION (flag scatter_process,
		 (iarray array,
		  flag (*func) (KThreadPool pool, iarray array, char *data,
				uaddr *lengths, uaddr **offsets,
				unsigned int num_dim, void *f_info,
				void *thread_info),
		  unsigned int max_dim, void *f_info) );
STATIC_FUNCTION (void scatter_job_func,
		 (void *pool_info, void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4, void *thread_info) );
STATIC_FUNCTION (flag min_max_scatter_job_func,
		 (KThreadPool pool, iarray array, char *data,
		  uaddr *lengths, uaddr **offsets,
		  unsigned int num_dim, void *f_info, void *thread_info) );
STATIC_FUNCTION (flag histogram_scatter_job_func,
		 (KThreadPool pool, iarray array, char *data,
		  uaddr *lengths, uaddr **offsets,
		  unsigned int num_dim, void *f_info, void *thread_info) );
/*  Threaded functions requiring contiguous data  */
STATIC_FUNCTION (flag contiguous_process,
		 (iarray array,
		  flag (*func) (KThreadPool pool, iarray array,
				char *data, uaddr stride, unsigned int values,
				void *f_info, void *thread_info),
		  void *f_info) );
STATIC_FUNCTION (void contiguous_job_func,
		 (void *pool_info, void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4, void *thread_info) );
STATIC_FUNCTION (flag min_max_contiguous_job_func,
		 (KThreadPool pool, iarray array, char *data, uaddr stride,
		  unsigned int num_values, void *f_info, void *thread_info) );
STATIC_FUNCTION (flag histogram_contiguous_job_func,
		 (KThreadPool pool, iarray array, char *data, uaddr stride,
		  unsigned int num_values, void *f_info, void *thread_info) );
/*  Other functions  */
STATIC_FUNCTION (flag ds_find_2D_histogram,
		 (CONST char *data, unsigned int elem_type,
		  unsigned int conv_type,
		  unsigned int length1, CONST uaddr *offsets1,
		  unsigned int length2, CONST uaddr *offsets2,
		  double min, double max, unsigned long num_bins,
		  unsigned long *histogram_array,
		  unsigned long *histogram_peak,
		  unsigned long *histogram_mode) );


/*  Private data  */
KThreadPool pool = NULL;


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
iarray iarray_read_nD (CONST char *object, flag cache, CONST char *arrayname,
		       unsigned int num_dim, CONST char **dim_names,
		       CONST char *elem_name,
		       unsigned int mmap_option)
/*  [SUMMARY] Read in a Karma arrayfile and yield an "Intelligent Array".
    <object> The name of the arrayfile to read. This parameter is passed
    directly to the [<dsxfr_get_multi>] routine. In order to understand the
    operation of this routine, the operation of the [<dsxfr_get_multi>] routine
    must be understood.
    <cache> This is passed directly to the [<dsxfr_get_multi>] routine.
    This controls whether disc arrayfiles are cached in memory for later use.
    <arrayname> The name of the general data structure in the arrayfile to
    search for. If this is NULL, the routine searches for the default name
    "Intelligent Array". If the arrayfile has only one general data structure,
    then this parameter is ignored.
    <num_dim> The routine searches for an n-dimensional array with a single
    atomic element at each point in multi-dimensional space. If this parameter
    is greater than 0, the routine will only return an array with the specified
    number of dimensions. If the value is 0, then the routine will return an
    n-dimensional array.
    <dim_names> If <<num_dim>> is not 0, then if this parameter is NULL, the
    routine will search for and return an array with the default dimension
    names (see [<iarray_create>] for a list of these) if more than one
    n-dimensional, single element array exists in the general data structure,
    or the only n-dimensional array with the specified number of dimensions.
    If the routine can't find an adequate default, it will not return an array.
    If <<num_dim>> is not 0, and this parameter points to an array of strings,
    then the routine will only return an array which matches the specified
    dimension names. The first name in the array of strings must be the highest
    order dimension.
    <elem_name> If this is NULL, the routine will ignore the element name of
    the array which is located, else it will insist on the array element name
    matching the specified name.
    <mmap_option> This is passed directly to the [<dsxfr_get_multi>] routine.
    This parameter controls the memory mapping of disc arrayfiles.
    If the data structure is likely to be subsequently modified, the value must
    be K_CH_MAP_NEVER, otherwise the data may be read-only memory mapped
    and writing to it will cause a segmentation fault.
    [RETURNS] A dynamically allocated intelligent array on success, else an
    error message is printed to the standard output and NULL is returned.
*/
{
    iarray array;
    multi_array *multi_desc;
/*
    static char function_name[] = "iarray_read_nD";
*/

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
flag iarray_write (iarray array, CONST char *arrayfile)
/*  [SUMMARY] Write an "Intelligent Array" in the Karma data format.
    <array> The "Intelligent Array".
    <arrayfile> The name of the arrayfile to write. See [<dsxfr_put_multi>] for
    details on the interpretation of this.
    [RETURNS] TRUE on success, else an error message is printed to the standard
    output and FALSE is returned.
*/
{
    static char function_name[] = "iarray_write";

    VERIFY_IARRAY (array);
    if (array->multi_desc == NULL)
    {
	(void) fprintf (stderr,
			"Intelligent array is not an original array\n");
	a_prog_bug (function_name);
    }
    if ( !dsxfr_put_multi (arrayfile, array->multi_desc) )
    {
	(void) fprintf (stderr, "Error writing Intelligent Array\n");
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function iarray_write  */

/*PUBLIC_FUNCTION*/
iarray iarray_create (unsigned int type, unsigned int num_dim,
		      CONST char **dim_names, CONST unsigned long *dim_lengths,
		      CONST char *elem_name, iarray old_array)
/*  [SUMMARY] Create an Intelligent Array.
    [PURPOSE] This routine will create an "Intelligent Array", using the Karma
    general data structure format as the underlying data format.
    If the environment variable "IARRAY_ALLOC_DEBUG" is set to "TRUE" then the
    routine will print allocation debugging information.
    <type> The desired type of the data elements. See [<DS_KARMA_DATA_TYPES>]
    for a list of defined data types.
    <num_dim> The number of dimensions the array must have.
    <dim_names> The names of the dimensions. If this is NULL, the default names
    "Axis 0", "Axis 1", etc. are used.
    <dim_lengths> The lengths of the dimensions. The first entry in this array
    and the <<dim_lengths>> array refers to the most significant dimension
    (i.e. the dimension with the greatest stride in memory).
    <elem_name> The name of the element. If this is NULL, the default name
    "Data Value" is choosen.
    <old_array> Any auxilary information not representable with "Intelligent
    Arrays" which is to be included in the Karma data format is copied from
    here. If this is NULL, no auxilary information is copied.
    [RETURNS] A dynamically allocated intelligent array on success, else an
    error message is printed to the standard output and NULL is returned.
*/
{
    unsigned int array_count;
    unsigned int elem_num;
    unsigned int dim_count;
    char *array, *arrayname;
    multi_array *out_multi_desc;
    multi_array *inp_multi_desc;
    iarray new;
    packet_desc *out_pack_desc;
    array_desc *out_arr_desc;
    dim_desc *dim;
    extern char *data_type_names[NUMTYPES];
    static char *def_elem_name = "Data Value";
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
	out_arr_desc = ( (array_desc *)
			out_multi_desc->headers[0]->element_desc[0] );
	if ( ( new = get_array_from_array (out_multi_desc,
					   (unsigned int) 0,
					   out_arr_desc, array, 0) )
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
		(void) fprintf (stderr, "%lu * ", dim_lengths[dim_count]);
	    }
	    (void) fprintf (stderr, "%lu  type: %s\n",
			    dim_lengths[dim_count], data_type_names[type]);
	}
	return (new);
    }
    /*  Auxilary data is to be copied  */
    if (elem_name == NULL) elem_name = def_elem_name;
    inp_multi_desc = old_array->multi_desc;
    if ( ( out_multi_desc = ds_alloc_multi (inp_multi_desc->num_arrays) )
	== NULL )
    {
	return (NULL);
    }
    /*  Copy auxilary descriptors and data  */
    for (array_count = 0; array_count < inp_multi_desc->num_arrays;
	 ++array_count)
    {
	if (array_count == old_array->array_num)
	{
	    /*  Skip this one  */
	    continue;
	}
	/*  Copy name  */
	if ( ( out_multi_desc->array_names[array_count] =
	      st_dup (inp_multi_desc->array_names[array_count]) )
	    == NULL )
	{
	    m_error_notify (function_name, "general data structure name");
	    ds_dealloc_multi (out_multi_desc);
	    return (NULL);
	}
	/*  Copy descriptor  */
	if ( ( out_multi_desc->headers[array_count] =
	      ds_copy_desc_until (inp_multi_desc->headers[array_count],
				  (char *) NULL) )
	    == NULL )
	{
	    m_error_notify (function_name,
			    "general data structure descriptor");
	    ds_dealloc_multi (out_multi_desc);
	    return (NULL);
	}
	/*  Allocate data  */
	if ( ( out_multi_desc->data[array_count] =
	      ds_alloc_data (inp_multi_desc->headers[array_count], TRUE,
			     TRUE) )
	    == NULL )
	{
	    m_error_notify (function_name,
			    "general data structure data");
	    ds_dealloc_multi (out_multi_desc);
	    return (NULL);
	}
	/*  Copy data  */
	if ( !ds_copy_data (inp_multi_desc->headers[array_count],
			    inp_multi_desc->data[array_count],
			    out_multi_desc->headers[array_count],
			    out_multi_desc->data[array_count]) )
	{
	    (void) fprintf (stderr, "\nError copying auxilary data");
	    ds_dealloc_multi (out_multi_desc);
	    return (NULL);
	}
    }
    /*  Auxilary data in other general data structures all copied  */
    /*  Copy most of input general data structure  */
    dim = iarray_get_dim_desc (old_array, 0);
    if ( ( out_multi_desc->headers[old_array->array_num] =
	  ds_copy_desc_until (inp_multi_desc->headers[old_array->array_num],
			      dim->name) )
	== NULL )
    {
	m_error_notify (function_name, "general data structure descriptor");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    switch ( ds_find_hole (out_multi_desc->headers[old_array->array_num],
			   &out_pack_desc, &elem_num) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr,
			"Old array does not have Intelligent Array\n");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
/*
	break;
*/
      case IDENT_MULTIPLE:
	(void) fprintf (stderr, "Old array has multiple holes\n");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
/*
	break;
*/
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
    if ( ( out_arr_desc =
	  ds_easy_alloc_array_desc (num_dim, dim_lengths,
				    (CONST double *) NULL,
				    (CONST double *) NULL,
				    (CONST double **) NULL, dim_names,
				    1, &type, &elem_name) )
	== NULL )
    {
	return (NULL);
    }
    /*  Link into data structure  */
    out_pack_desc->element_types[elem_num] = K_ARRAY;
    out_pack_desc->element_desc[elem_num] = (char *) out_arr_desc;
    /*  Entire data structure now described: allocate data  */
    if ( ( out_multi_desc->data[old_array->array_num] =
	  ds_alloc_data (out_multi_desc->headers[old_array->array_num],
			 TRUE, TRUE) )
	== NULL )
    {
	m_error_notify (function_name, "data space");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    /*  Copy over auxilary data  */
    if ( !ds_copy_data (inp_multi_desc->headers[old_array->array_num],
			inp_multi_desc->data[old_array->array_num],
			out_multi_desc->headers[old_array->array_num],
			out_multi_desc->data[old_array->array_num]) )
    {
	(void) fprintf (stderr, "Error copying auxilary data\n");
	ds_dealloc_multi (out_multi_desc);
	return (NULL);
    }
    /*  Return "Intelligent Array" data structure  */
    if (out_multi_desc->num_arrays == 1)
    {
	arrayname = NULL;
    }
    else
    {
	arrayname = out_multi_desc->array_names[old_array->array_num];
    }
    if ( ( new =
	  iarray_get_from_multi_array (out_multi_desc, arrayname, num_dim,
				       (CONST char **) NULL, elem_name) )
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
	    (void) fprintf (stderr, "%lu * ", dim_lengths[dim_count]);
	}
	(void) fprintf (stderr, "%lu  type: %s\n",
			dim_lengths[dim_count], data_type_names[type]);
    }
    return (new);
}   /*  End Function iarray_create  */

/*PUBLIC_FUNCTION*/
iarray iarray_get_from_multi_array (multi_array *multi_desc,
				    CONST char *arrayname,
				    unsigned int num_dim,
				    CONST char **dim_names,
				    CONST char *elem_name)
/*  [SUMMARY] Create an Intelligent Array from a Karma data structure.
    [PURPOSE] This routine will yield an "Intelligent Array" from a multi array
    Karma general data structure. The routine searches for a n-dimensional
    array with a single atomic element at each point in multi-dimensional space
    <multi_desc> The multi array header pointer. The attachment count is
    incremented on successful completion of this routine.
    <arrayname> The name of the general data structure in the arrayfile to
    search for. If this is NULL, the routine searches for the default name
    "Intelligent Array". If the arrayfile has only one general data structure,
    then this parameter is ignored.
    <num_dim> If greater than 0, the routine will only return an array with
    this many dimensions. If 0, then the routine will return an n-dimensional
    array.
    <dim_names> If <<num_dim>> is not 0, then if this NULL, the routine will
    search for and return an array with the default dimension names (see
    <<iarray_create>> for a list of these) if more than one n-dimensional,
    single element array exists in the general data structure, or the only
    n-dimensional array with the specified number of dimensions. If the routine
    can't find an adequate default, it will not return an array. If <<num_dim>>
    is not 0, and this points to an array of strings, then the routine will
    only return an array which matches the specified dimension names. The first
    name in the array of strings must be the highest order dimension.
    <elem_name> If NULL, the routine will ignore the element name of the array
    which is located, else it will insist on the array element name matching
    this name.
    [RETURNS] A dynamically allocated intelligent array on success, else an
    error message is printed to the standard output and NULL is returned.
*/
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
    if (multi_desc->num_arrays == 1)
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
    top_pack_desc = multi_desc->headers[array_num];
    top_packet = multi_desc->data[array_num];
    if (num_dim == 0)
    {
	/*  Get array of any dimensionality  */
	for (elem_count = 0, match_index = top_pack_desc->num_elements;
	     elem_count < top_pack_desc->num_elements;
	     ++elem_count)
	{
	    if (top_pack_desc->element_types[elem_count] != K_ARRAY)
	    {
		continue;
	    }
	    arr_desc =(array_desc *) top_pack_desc->element_desc[elem_count];
	    if (elem_name == NULL)
	    {
		/*  Case where no element name is specified: must have
		    single element array  */
		if (arr_desc->packet->num_elements == 1)
		{
		    /*  Yup: fine.  */
		    /*  Match to any array  */
		    if (match_index < top_pack_desc->num_elements)
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
	    if ( ( elem_index = ds_f_elem_in_packet (arr_desc->packet,
						     elem_name) )
		< arr_desc->packet->num_elements )
	    /*  Found the requested one  */
	    return ( get_array_from_array
		    (multi_desc, array_num, arr_desc, *(char **)
		     (ds_get_element_offset (top_pack_desc,
					     elem_count) + top_packet),
		     elem_index) );
	}
	if (match_index >= top_pack_desc->num_elements)
	{
	    (void) fprintf (stderr, "No candidate arrays found\n");
	    return (NULL);
	}
	/*  Got the one and only decent match  */
	arr_desc = (array_desc *) top_pack_desc->element_desc[match_index];
	return ( get_array_from_array
		(multi_desc, array_num, arr_desc, *(char **)
		 (ds_get_element_offset (top_pack_desc, match_index) +
		  top_packet), 0) );
    }
    /*  Get array of specified dimensionality  */
    if (dim_names == NULL)
    {
	/*  Get any array of specified dimensionality  */
	for (elem_count = 0, match_index = top_pack_desc->num_elements;
	     elem_count < top_pack_desc->num_elements;
	     ++elem_count)
	{
	    if (top_pack_desc->element_types[elem_count] != K_ARRAY)
	    {
		continue;
	    }
	    arr_desc =(array_desc *) top_pack_desc->element_desc[elem_count];
	    if (arr_desc->num_dimensions != num_dim) continue;
	    if (elem_name == NULL)
	    {
		/*  Case where no element name is specified: must have
		    single element array  */
		if (arr_desc->packet->num_elements == 1)
		{
		    /*  Yup: fine.  */
		    /*  Match to any array  */
		    if (match_index < top_pack_desc->num_elements)
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
	    if ( ( elem_index = ds_f_elem_in_packet (arr_desc->packet,
						     elem_name) )
		< arr_desc->packet->num_elements )
	    {
		/*  Found the requested one  */
		return ( get_array_from_array
			(multi_desc, array_num, arr_desc, *(char **)
			 (ds_get_element_offset (top_pack_desc, elem_count) +
			  top_packet), elem_index) );
	    }
	}
	if (match_index >= top_pack_desc->num_elements)
	{
	    (void) fprintf (stderr, "No candidate arrays found\n");
	    return (NULL);
	}
	/*  Got the one and only decent match  */
	arr_desc = (array_desc *) top_pack_desc->element_desc[match_index];
	return ( get_array_from_array
		(multi_desc, array_num, arr_desc, *(char **)
		 (ds_get_element_offset (top_pack_desc, match_index) +
		  top_packet), 0) );
    }
    /*  Find unique occurrence of array  */
    switch ( ds_get_handle_in_packet (top_pack_desc, top_packet, dim_names[0],
				      (CONST char **) NULL, (double *) NULL, 0,
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
    if (num_dim != arr_desc->num_dimensions)
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
    for (dim_count = 0; dim_count < arr_desc->num_dimensions; ++dim_count)
    {
	if ( ( dim_index = ds_f_dim_in_array (arr_desc, dim_names[dim_count]) )
	    >= arr_desc->num_dimensions)
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
    if (arr_desc->packet->num_elements != 1)
    {
	(void) fprintf (stderr,
			"Intelligent Array must have only one element\n");
	m_free ( (char *) reorder_indices );
	return (NULL);
    }
    if (elem_name != NULL)
    {
	if (ds_f_elem_in_packet (arr_desc->packet, elem_name) >=
	    arr_desc->packet->num_elements)
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
	if ( !ds_reorder_array (arr_desc, reorder_indices, parent, TRUE) )
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
void iarray_dealloc (iarray array)
/*  [SUMMARY] Deallocate an Intelligent Array.
    [PURPOSE] This routine will deallocate an "Intelligent Array". If the
    environment variable "IARRAY_ALLOC_DEBUG" is set to "TRUE" then the routine
    will print deallocation debugging information.
    <array> The Intelligent Array.
    [RETURNS] Nothing.
*/
{
    unsigned int dim_count;
    multi_array *multi_desc;
    extern char *data_type_names[NUMTYPES];
    static char function_name[] = "iarray_dealloc";

    VERIFY_IARRAY (array);
    if (array->destroy_callbacks != NULL)
    {
	c_call_callbacks (array->destroy_callbacks, array);
	c_destroy_list (array->destroy_callbacks);
    }
    if (array->offsets != array->arr_desc->offsets)
    {
	/*  Specially allocated offsets  */
	for (dim_count = 0; dim_count < iarray_num_dim (array); ++dim_count)
	{
	    m_free ( (char *) (array->offsets[dim_count] -
			       array->boundary_width) );
	}
	m_free ( (char *) array->offsets );
    }
    multi_desc = array->multi_desc;
    if ( (multi_desc->attachments == 0) && mem_debug_required () )
    {
	(void) fprintf (stderr, "iarray_dealloc: ");
	for (dim_count = 0; dim_count < iarray_num_dim (array) -1; ++dim_count)
	{
	    (void) fprintf (stderr, "%lu * ", array->lengths[dim_count]);
	}
	(void) fprintf (stderr, "%lu  type: %s\n",
			array->lengths[dim_count],
			data_type_names[iarray_type (array)]);
    }
    ds_dealloc_multi (multi_desc);
    m_free ( (char *) array->lengths );
    m_free ( (char *) array->contiguous );
    m_free ( (char *) array->orig_dim_indices );
    m_free ( (char *) array );
}   /*  End Function iarray_dealloc  */

/*PUBLIC_FUNCTION*/
flag iarray_put_named_value (iarray array, CONST char *name, unsigned int type,
			     double value[2])
/*  [SUMMARY] Attach a data value to an Intelligent Array.
    [PURPOSE] This routine will add a unique named value to the underlying
    Karma general data structure of an "Intelligent Array".
    <array> The Intelligent Array.
    <name> The name of the element to add.
    <type> The type of the data which is to be written.
    <value> The value of the data.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "iarray_put_named_value";

    VERIFY_IARRAY (array);
    return ( ds_put_unique_named_value
	    (array->top_pack_desc, array->top_packet,
	     name, type, value, TRUE) );
}   /*  End Function iarray_put_named_value  */

/*PUBLIC_FUNCTION*/
flag iarray_put_named_string (iarray array, CONST char *name, char *string)
/*  [SUMMARY] Attach a string to an Intelligent Array.
    [PURPOSE] This routine will add a unique named string to the underlying
    Karma general data structure of an "Intelligent Array".
    <array> The Intelligent Array.
    <name> The name of the element to add.
    <string> The string data.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "iarray_put_named_string";

    VERIFY_IARRAY (array);
    return ( ds_put_unique_named_string
	    (array->top_pack_desc, array->top_packet,
	     name, string, TRUE) );
}   /*  End Function iarray_put_named_string  */

/*PUBLIC_FUNCTION*/
flag iarray_get_named_value (iarray array, CONST char *name,
			     unsigned int *type, double value[2])
/*  [SUMMARY] Get attached data from an Intelligent Array.
    [PURPOSE] This routine will get a unique named value from the underlying
    Karma general data structure of an "Intelligent Array".
    <array> The Intelligent Array.
    <name> The name of the element.
    <type> The type of the input data found will be written here. If this is
    NULL, nothing is written here.
    <value> The value of the converted data will be written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "iarray_get_named_value";

    VERIFY_IARRAY (array);
    return ( ds_get_unique_named_value
	    (array->top_pack_desc, *array->top_packet,
	     name, type, value) );
}   /*  End Function iarray_get_named_value  */

/*PUBLIC_FUNCTION*/
char *iarray_get_named_string (iarray array, CONST char *name)
/*  [SUMMARY] Get attached string from an Intelligent Array.
    [PURPOSE] This routine will get a unique named string from the underlying
    Karma general data structure of an "Intelligent Array".
    <array> The Intelligent Array.
    <name> The name of the element.
    [RETURNS] A pointer to a dynamically allocated copy of the string on
    success, else NULL.
*/
{
    static char function_name[] = "iarray_get_named_string";

    VERIFY_IARRAY (array);
    return ( ds_get_unique_named_string
	    (array->top_pack_desc, *array->top_packet, name) );
}   /*  End Function iarray_get_named_string  */

/*PUBLIC_FUNCTION*/
flag iarray_copy_data (iarray output, iarray input, flag magnitude)
/*  [SUMMARY] Copy data between Intelligent Arrays.
    [PURPOSE] This routine will copy data from one "Intelligent Array" to
    another. The sizes of the two arrays must be identical.
    The routine can deal with the types of the two arrays being different
    <output> The output Intelligent Array.
    <input> The input Intelligent Array.
    <magnitude> If TRUE then when converting from a complex array to a real
    array, the magnitude of the complex data is taken, else the real component
    is copied.
    [NOTE] When converting from a real array to a complex data array, the
    imaginary components are set to zero.
    [RETURNS] TRUE on success, else FALSE.
*/
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
    unsigned long *coordinates;
    double *buffer = NULL;
    double *ptr;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "iarray_copy_data";

    VERIFY_IARRAY (output);
    VERIFY_IARRAY (input);
    /*  Test array sizes  */
    if ( ( num_dim = iarray_num_dim (input) ) != iarray_num_dim (output) )
    {
	(void) fprintf ( stderr,
			"Input array has: %u dimensions whilst output array has: %u\n",
			iarray_num_dim (input), iarray_num_dim (output) );
	return (FALSE);
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if (input->lengths[dim_count] != output->lengths[dim_count])
	{
	    (void) fprintf (stderr, "Input dimension: %u has length: %lu\n",
			    dim_count, input->lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %lu\n",
			    dim_count, output->lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
    }
    inp_stride = input->offsets[num_dim - 1][1];
    inp_stride -= input->offsets[num_dim - 1][0];
    out_stride = output->offsets[num_dim - 1][1];
    out_stride -= output->offsets[num_dim - 1][0];
    inp_elem_size = host_type_sizes[iarray_type (input)];
    /*  Check if lower dimensions are contiguous  */
    if (!input->contiguous[num_dim - 1])
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if (!output->contiguous[num_dim - 1])
    {
	/*  Output array not contiguous  */
	contiguous = FALSE;
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned long *) m_alloc (sizeof *coordinates *
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
		  m_alloc (sizeof *buffer * 2 * input->lengths[num_dim -1]) )
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
			       input->lengths[num_dim - 1] -
			       coordinates[num_dim - 1]);
	    }
	    else
	    {
		/*  Have to convert data  */
		if ( !ds_get_elements (inp_data, iarray_type (input),
				       inp_stride, buffer, &inp_complex,
				       input->lengths[num_dim - 1]) )
		{
		    m_free ( (char *) coordinates );
		    m_free ( (char *) buffer );
		    return (FALSE);
		}
		if (!ds_element_is_complex ( iarray_type (output) )
		    && inp_complex && magnitude)
		{
		    /*  Complex to real conversion  */
		    for (elem_count = 0, ptr = buffer;
			 elem_count < input->lengths[num_dim - 1];
			 ++elem_count, ptr += 2)
		    {
			*ptr = sqrt (ptr[0] * ptr[0] + ptr[1] * ptr[1]);
		    }
		}
		if ( !ds_put_elements (out_data, iarray_type (output),
				       out_stride, buffer,
				       input->lengths[num_dim - 1]) )
		{
		    m_free ( (char *) coordinates );
		    m_free ( (char *) buffer );
		    return (FALSE);
		}
	    }
	    inp_data = iarray_get_next_element (input, coordinates,
						input->lengths[num_dim - 1]);
	    out_data = iarray_get_next_element (output, coordinates, 0);
	}
	else
	{
	    /*  Data is not contiguous: copy one element at a time  */
	    if ( !ds_get_element (inp_data, iarray_type (input), data,
				  &inp_complex) )
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    if (!ds_element_is_complex ( iarray_type (output) )
		&& inp_complex && magnitude)
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

/*UNPUBLISHED_FUNCTION*/
char *iarray_get_element_1D (iarray array, unsigned int type, int x)
/*  [SUMMARY] Get element from 1-dimensional Intelligent Array.
    [PURPOSE] This routine will get an element from a simple 1 dimensional
    array. This routine is NOT meant to be called by applications.
    <array> The array.
    <type> The type of the element in the array (this is used to enforce type
    checking).
    <x> The array index.
    [RETURNS] A pointer to the element.
*/
{
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
    if (x < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d is less than -boundary_width: %d\n",
			x, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (x >= array->lengths[0] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d exceeds dimension end: %ld\n",
			x, array->lengths[0] - array->boundary_width);
	a_prog_bug (function_name);
    }
    return (array->data + array->offsets[0][x]);
}   /*  End Function iarray_get_element_1D  */

/*UNPUBLISHED_FUNCTION*/
char *iarray_get_element_2D (iarray array, unsigned int type, int y, int x)
/*  [SUMMARY] Get element from 2-dimensional Intelligent Array.
    [PURPOSE] This routine will get an element from a simple 2 dimensional
    array. This routine is NOT meant to be called by applications.
    <array> The array.
    <type> The type of the element in the array (this is used to enforce type
    checking).
    <y> The upper array index.
    <x> The lower array index.
    [RETURNS] A pointer to the element.
*/
{
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
    if (x < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d is less than -boundary_width: %d\n",
			x, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (x >= array->lengths[1] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d exceeds dimension end: %ld\n",
			x, array->lengths[1] - array->boundary_width);
	a_prog_bug (function_name);
    }
    if (y < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d is less than -boundary_width: %d\n",
			y, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (y >= array->lengths[0] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d exceeds dimension end: %ld\n",
			y, array->lengths[0] - array->boundary_width);
	a_prog_bug (function_name);
    }
    return (array->data + array->offsets[0][y] + array->offsets[1][x]);
}   /*  End Function iarray_get_element_2D  */

/*UNPUBLISHED_FUNCTION*/
char *iarray_get_element_3D (iarray array, unsigned int type, int z, int y,
			     int x)
/*  [SUMMARY] Get element from 3-dimensional Intelligent Array.
    [PURPOSE] This routine will get an element from a simple 3 dimensional
    array. This routine is NOT meant to be called by applications.
    <array> The array.
    <type> The type of the element in the array (this is used to enforce type
    checking).
    <z> The upper array index.
    <y> The middle array index.
    <x> The lower array index.
    [RETURNS] A pointer to the element.
*/
{
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
    if (x < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d is less than -boundary_width: %d\n",
			x, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (x >= array->lengths[2] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d exceeds dimension end: %ld\n",
			x, array->lengths[2] - array->boundary_width);
	a_prog_bug (function_name);
    }
    if (y < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d is less than -boundary_width: %d\n",
			y, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (y >= array->lengths[1] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d exceeds dimension end: %ld\n",
			y, array->lengths[1] - array->boundary_width);
	a_prog_bug (function_name);
    }
    if (z < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"z coordinate: %d is less than -boundary_width: %d\n",
			z, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (z >= array->lengths[0] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"z coordinate: %d exceeds dimension end: %ld\n",
			z, array->lengths[0] - array->boundary_width);
	a_prog_bug (function_name);
    }
    return (array->data + array->offsets[0][z] + array->offsets[1][y] +
	    array->offsets[2][x]);
}   /*  End Function iarray_get_element_3D  */

/*UNPUBLISHED_FUNCTION*/
char *iarray_get_element_4D (iarray array, unsigned int type, int z, int y,
			     int x, int w)
/*  [SUMMARY] Get element from 4-dimensional Intelligent Array.
    [PURPOSE] This routine will get an element from a simple 4 dimensional
    array. This routine is NOT meant to be called by applications.
    <array> The array.
    <type> The type of the element in the array (this is used to enforce type
    checking).
    <z> The upper array index.
    <y> The second upper array index.
    <x> The second lower array index.
    <w> The lower array index.
    [RETURNS] A pointer to the element.
*/
{
    static char function_name[] = "iarray_get_element_4D";

    VERIFY_IARRAY (array);
    if (iarray_num_dim (array) != 4)
    {
	(void) fprintf ( stderr,
			"Array has: %u dimensions: must have only 4\n",
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
    if (w < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"w coordinate: %d is less than -boundary_width: %d\n",
			w, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (w >= array->lengths[3] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"w coordinate: %d exceeds dimension end: %ld\n",
			w, array->lengths[3] - array->boundary_width);
	a_prog_bug (function_name);
    }
    if (x < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d is less than -boundary_width: %d\n",
			x, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (x >= array->lengths[2] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"x coordinate: %d exceeds dimension end: %ld\n",
			x, array->lengths[2] - array->boundary_width);
	a_prog_bug (function_name);
    }
    if (y < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d is less than -boundary_width: %d\n",
			y, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (y >= array->lengths[1] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"y coordinate: %d exceeds dimension end: %ld\n",
			y, array->lengths[1] - array->boundary_width);
	a_prog_bug (function_name);
    }
    if (z < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"z coordinate: %d is less than -boundary_width: %d\n",
			z, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (z >= array->lengths[0] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"z coordinate: %d exceeds dimension end: %ld\n",
			z, array->lengths[0] - array->boundary_width);
	a_prog_bug (function_name);
    }
    return (array->data + array->offsets[0][z] + array->offsets[1][y] +
	    array->offsets[2][x] + array->offsets[3][w]);
}   /*  End Function iarray_get_element_4D  */

/*PUBLIC_FUNCTION*/
iarray iarray_get_sub_array_2D (iarray array, int starty, int startx,
				unsigned int ylen, unsigned int xlen)
/*  [SUMMARY] Create a 2-dimensional alias Intelligent Array.
    [PURPOSE] This routine will create an "Intelligent Array" which is an alias
    or a sub-array of another "Intelligent Array". Subsequent modification of
    the alias array will modify the data of the original array. Sub-arrays may
    be created from other sub-arrays. The attachment count of the underlying
    <<multi_array>> data structure is incremented on successful completion.
    <array> The original array.
    <starty> The starting y (row) index in the original array corresponding to
    the first row of the alias array.
    <startx> The starting x (column) index in the original array corresponding
    to the first column of the alias array.
    <ylen> The number of y co-ordinates (rows) in the alias array.
    <xlen> The number of x co-ordinates (columns) in the alias array.
    [RETURNS] A dynamically allocated intelligent array on success, else NULL.
    [NOTE] Sub-arrays cannot be saved to disc.
*/
{
    int coord_count;
    unsigned int num_restr;
    iarray sub;
    static char function_name[] = "iarray_get_sub_array_2D";

    VERIFY_IARRAY (array);
    /*  Sanity checks  */
    if (iarray_num_dim (array) != 2)
    {
	(void) fprintf ( stderr,
			"Input array has: %u dimensions, must have only 2\n",
			iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    if (starty < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"starty: %d is less than -boundary_width: %d\n",
			starty, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (starty >= array->lengths[0] - array->boundary_width)
    {
	(void) fprintf (stderr, "starty: %d exceeds dimension end: %ld\n",
			starty, array->lengths[0] - array->boundary_width);
	a_prog_bug (function_name);
    }
    if (starty + ylen > array->lengths[0] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"starty + ylen: %d exceeds dimension end: %ld\n",
			starty + ylen,
			array->lengths[0] - array->boundary_width);
	a_prog_bug (function_name);
    }
    if (startx < -array->boundary_width)
    {
	(void) fprintf (stderr,
			"startx: %d is less than -boundary_width: %d\n",
			startx, -array->boundary_width);
	a_prog_bug (function_name);
    }
    if (startx >= array->lengths[1] - array->boundary_width)
    {
	(void) fprintf (stderr, "startx: %d exceeds dimension end: %ld\n",
			startx, array->lengths[1] - array->boundary_width);
	a_prog_bug (function_name);
    }
    if (startx + xlen > array->lengths[1] - array->boundary_width)
    {
	(void) fprintf (stderr,
			"startx + xlen: %d exceeds dimension end: %ld\n",
			startx + xlen,
			array->lengths[1] - array->boundary_width);
	a_prog_bug (function_name);
    }
    /*  Checks OK: create the alias  */
    if ( ( sub = (iarray) m_alloc (sizeof *sub) ) == NULL )
    {
	m_error_notify (function_name, "iarray");
    }
    if ( ( sub->lengths = (unsigned long *)
	  m_alloc (sizeof *sub->lengths * 2) ) == NULL )
    {
	m_error_notify (function_name, "iarray");
	m_free ( (char *) sub );
    }
    sub->lengths[0] = ylen;
    sub->lengths[1] = xlen;
    sub->data = array->data;
    sub->array_num = array->array_num;
    sub->multi_desc = array->multi_desc;
    sub->top_pack_desc = array->top_pack_desc;
    sub->top_packet = array->top_packet;
    sub->arr_desc = array->arr_desc;
    sub->num_dim = iarray_num_dim (array);
    sub->orig_dim_indices = NULL;
    sub->restrictions = NULL;
    if ( !iarray_allocate_records (sub, TRUE) )
    {
	m_free ( (char *) sub->lengths );
	m_free ( (char *) sub );
	return (NULL);
    }
    /*  Copy over offset information  */
    for (coord_count = 0; coord_count < ylen; ++coord_count)
    {
	sub->offsets[0][coord_count] = array->offsets[0][coord_count +
							     starty];
    }
    for (coord_count = 0; coord_count < xlen; ++coord_count)
    {
	sub->offsets[1][coord_count] = array->offsets[1][coord_count +
							     startx];
    }
    /*  Copy contiguous flags  */
    m_copy ( (char *) sub->contiguous, (char *) array->contiguous,
	    sizeof sub->contiguous * iarray_num_dim (array) );
    /*  Copy over original dimension indices  */
    m_copy ( (char *) sub->orig_dim_indices,
	    (char *) array->orig_dim_indices,
	    sizeof *sub->orig_dim_indices *
	    array->arr_desc->num_dimensions );
    /*  Copy over restrictions  */
    num_restr = array->arr_desc->num_dimensions - iarray_num_dim (array);
    if (num_restr > 0)
    {
	m_copy ( (char *) sub->restrictions, (char *) array->restrictions,
		sizeof *sub->restrictions * num_restr );
    }
    /*  Increment attachment count  */
    ++array->multi_desc->attachments;
    return (sub);
}   /*  End Function iarray_get_sub_array_2D  */

/*PUBLIC_FUNCTION*/
iarray iarray_get_2D_slice_from_3D (iarray cube, unsigned int ydim,
				    unsigned int xdim, unsigned int slice_pos)
/*  [SUMMARY] Create a 2-dimensional Intelligent Array alias from a slice.
    [PURPOSE] This routine will create a 2-D "Intelligent Array" which is an
    alias of an arbitrary slice of a 3-D array.
    <cube> The input 3-D array.
    <ydim> The dimension in the 3-D array which will become the y dimension
    (most significant) of the output array.
    <xdim> The dimension in the 3-D array which will become the x dimension
    (least significant) of the output array.
    <slice_pos> The position of the slice along the unspecified (remaining)
    dimension in the 3-D array.
    [RETURNS] A dynamically allocated intelligent array on success, else NULL.
    [NOTE] Alias arrays cannot be saved to disc.
*/
{
    unsigned int dim_count;
    unsigned int num_restr;
    unsigned int rdim;
    unsigned int inp_num_dim;
    unsigned int out_num_dim;
    unsigned int ylen, xlen;
    unsigned int tmp;
    iarray slice;
    static char function_name[] = "iarray_get_2D_slice_from_3D";

    VERIFY_IARRAY (cube);
    /*  Sanity checks  */
    if ( ( inp_num_dim = iarray_num_dim (cube) ) != 3 )
    {
	(void) fprintf (stderr,
			"Input array has: %u dimensions, must have only 3\n",
			inp_num_dim);
	a_prog_bug (function_name);
    }
    if (ydim == xdim)
    {
	(void) fprintf (stderr, "ydim and xdim must have different values\n");
	(void) fprintf (stderr, "Common value: %u\n", ydim);
	a_prog_bug (function_name);
    }
    if (ydim >= inp_num_dim)
    {
	(void) fprintf (stderr, "ydim: %u must be less than: %u\n",
			ydim, inp_num_dim);
	a_prog_bug (function_name);
    }
    if (xdim >= inp_num_dim)
    {
	(void) fprintf (stderr, "xdim: %u must be less than: %u\n",
			xdim, inp_num_dim);
	a_prog_bug (function_name);
    }
    /*  Find remaining dimension  */
    for (dim_count = 0, rdim = inp_num_dim;
	 (dim_count < inp_num_dim) && (rdim >= inp_num_dim);
	 ++dim_count)
    {
	if ( (dim_count != ydim) && (dim_count != xdim) ) rdim = dim_count;
    }
    if (slice_pos >= cube->lengths[rdim])
    {
	(void) fprintf (stderr,
			"slice_pos: %u must be less than dim. length: %lu\n",
			slice_pos, cube->lengths[rdim]);
	a_prog_bug (function_name);
    }
    /*  Checks OK: create the alias  */
    out_num_dim = inp_num_dim - 1;
    ylen = cube->lengths[ydim];
    xlen = cube->lengths[xdim];
    if ( ( slice = (iarray) m_alloc (sizeof *slice) ) == NULL )
    {
	m_error_notify (function_name, "iarray");
    }
    if ( ( slice->lengths = (unsigned long *)
	  m_alloc (sizeof *slice->lengths * out_num_dim) ) == NULL )
    {
	m_error_notify (function_name, "iarray");
	m_free ( (char *) slice );
    }
    slice->lengths[0] = ylen;
    slice->lengths[1] = xlen;
    slice->data = cube->data + cube->offsets[rdim][slice_pos];
    slice->array_num = cube->array_num;
    slice->elem_index = cube->elem_index;
    slice->multi_desc = cube->multi_desc;
    slice->top_pack_desc = cube->top_pack_desc;
    slice->top_packet = cube->top_packet;
    slice->arr_desc = cube->arr_desc;
    slice->num_dim = out_num_dim;
    slice->orig_dim_indices = NULL;
    slice->restrictions = NULL;
    if ( !iarray_allocate_records (slice, TRUE) )
    {
	m_free ( (char *) slice->lengths );
	m_free ( (char *) slice );
	return (NULL);
    }
    /*  Copy over offset information  */
    m_copy ( (char *) slice->offsets[0], (char *) cube->offsets[ydim],
	    sizeof *slice->offsets[0] * ylen );
    m_copy ( (char *) slice->offsets[1], (char *) cube->offsets[xdim],
	    sizeof *slice->offsets[1] * xlen );
    /*  Copy contiguous flags  */
    slice->contiguous[0] = cube->contiguous[ydim];
    slice->contiguous[1] = cube->contiguous[xdim];
    /*  Copy over original dimension indices  */
    slice->orig_dim_indices[0] = cube->orig_dim_indices[ydim];
    slice->orig_dim_indices[1] = cube->orig_dim_indices[xdim];
    slice->orig_dim_indices[2] = cube->orig_dim_indices[rdim];
    tmp = cube->arr_desc->num_dimensions - inp_num_dim;
    m_copy ( (char *) (inp_num_dim + slice->orig_dim_indices),
	    (char *) (inp_num_dim + cube->orig_dim_indices),
	    sizeof *slice->orig_dim_indices * tmp );
    /*  Insert a new restriction  */
    slice->restrictions[0] = slice_pos;
    /*  Copy over old restrictions  */
    num_restr = cube->arr_desc->num_dimensions - iarray_num_dim (cube);
    if (num_restr > 0)
    {
	m_copy ( (char *) (1 + slice->restrictions),
		(char *) cube->restrictions,
		sizeof *slice->restrictions * num_restr );
    }
    /*  Increment attachment count  */
    ++slice->multi_desc->attachments;
    return (slice);
}   /*  End Function iarray_get_2D_slice_from_3D  */

/*PUBLIC_FUNCTION*/
unsigned long iarray_dim_length (iarray array, unsigned int index)
/*  [SUMMARY] Get length of a dimension in an Intelligent Array.
    <array> The Intelligent Array.
    <index> The index of the dimension.
    [RETURNS] The length of the specified dimension.
*/
{
    static char function_name[] = "iarray_dim_length";

    VERIFY_IARRAY (array);
    if ( index >= iarray_num_dim (array) )
    {
	(void) fprintf ( stderr,
			"Dimension index: %u is not less than number of dimensions: %u\n",
			index, iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    return (array->lengths[index]);
}   /*  End Function iarray_dim_length  */

/*PUBLIC_FUNCTION*/
unsigned int iarray_get_restrictions (iarray array, char ***restr_names,
				      double **restr_values)
/*  [SUMMARY] Get restriction information.
    [PURPOSE] This routine will get any associated restrictions for an
    Intelligent Array. The routine will dynamically allocate space for the
    restriction data, which must be externally freed.
    <array> The Intelligent Array.
    <restr_names> The array of pointers to restrictions names will be written
    here.
    <restr_values> The array of restriction values will be written here.
    [RETURNS] The number of restrictions. This may be 0.
*/
{
    unsigned int count;
    unsigned int num_restr;
    char **names;
    double *values;
    dim_desc *dim;
    dim_desc **dimensions;
    static char function_name[] = "iarray_get_restrictions";

    *restr_names = NULL;
    *restr_values = NULL;
    num_restr = array->arr_desc->num_dimensions - array->num_dim;
    if (num_restr < 1) return (0);
    /*  Add restrictions  */
    if ( ( names = (char **) m_alloc (sizeof *names * num_restr) ) == NULL )
    {
	m_abort (function_name, "array of restriction name pointers");
    }
    if ( ( values = (double *) m_alloc (sizeof *values * num_restr) ) == NULL )
    {
	m_abort (function_name, "array of restriction values");
    }
    dimensions = array->arr_desc->dimensions;
    for (count = 0; count < num_restr; ++count)
    {
	dim = dimensions[ array->orig_dim_indices[count] ];
	if ( ( names[count] = st_dup (dim->name) ) == NULL )
	{
	    m_abort (function_name, "restriction name");
	}
	values[count] = ds_get_coordinate (dim, array->restrictions[count]);
    }
    *restr_names = names;
    *restr_values = values;
    return (num_restr);
}   /*  End Function iarray_get_restrictions  */

/*PUBLIC_FUNCTION*/
flag iarray_fill (iarray array, double value[2])
/*  [SUMMARY] Fill an Intelligent Array with a single value.
    <array> The Intelligent Array.
    <value> The fill value.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int elem_size;
    unsigned int stride;
    unsigned int num_dim;
    unsigned int increment;
    char *data;
    char *val;
    unsigned long *coordinates;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "iarray_fill";

    VERIFY_IARRAY (array);
    num_dim = iarray_num_dim (array);
    stride = array->offsets[num_dim -1][1] - array->offsets[num_dim -1][0];
    elem_size = host_type_sizes[iarray_type (array)];
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
    if ( ( coordinates = (unsigned long *) m_alloc (sizeof *coordinates *
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
	m_fill (data, stride, val, elem_size, increment);
	data = iarray_get_next_element (array, coordinates, increment);
    }
    m_free ( (char *) coordinates );
    m_free (val);
    return (TRUE);
}   /*  End Function iarray_fill  */

/*PUBLIC_FUNCTION*/
flag iarray_min_max (iarray array, unsigned int conv_type, double *min,
		     double *max)
/*  [SUMMARY] Determine the minimum and maximum value of an Intelligent Array.
    <array> The Intelligent Array.
    <conv_type> The conversion type to use for complex numbers. This is ignored
    if the array is not complex. See [<DS_COMPLEX_CONVERSIONS>] for legal
    values.
    <min> The routine will write the minimum value here.
    <max> The routine will write the maximum value here.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag full_array;
    unsigned int num_dim;
    unsigned int num_threads, thread_count;
    array_desc *arr_desc;
    min_max_thread_info *info;
    extern KThreadPool pool;
    static char function_name[] = "iarray_min_max";

    VERIFY_IARRAY (array);
    if ( (min == NULL) || (max == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    *min = TOOBIG;
    *max = -TOOBIG;
    num_dim = iarray_num_dim (array);
    full_array = iarray_is_full_array (array);
    initialise_thread_pool ();
    num_threads = mt_num_threads (pool);
    if ( full_array && (num_threads < 2) )
    {
	/*  Can do this in one contiguous block  */
	arr_desc = array->arr_desc;
	return ( ds_find_contiguous_extremes
		 (array->data, ds_get_array_size (arr_desc),
		  ds_get_packet_size (arr_desc->packet), iarray_type (array),
		  conv_type, min, max) );
    }
    if ( !full_array && (num_dim == 1) )
    {
	/*  Simple 1-dimensional process  */
	return ( ds_find_1D_extremes (array->data,
				      array->lengths[0], array->offsets[0],
				      iarray_type (array), conv_type,
				      min, max) );
    }
    if ( !full_array && (num_dim == 2) && (num_threads < 2) )
    {
	/*  Simple unthreaded 2-dimensional process  */
	return ( ds_find_2D_extremes (array->data,
				      array->lengths[0], array->offsets[0],
				      array->lengths[1], array->offsets[1],
				      iarray_type (array), conv_type,
				      min, max) );
    }
    /*  Initialise thread info  */
    mt_new_thread_info (pool, NULL, sizeof *info);
    info = mt_get_thread_info (pool);
    for (thread_count = 0; thread_count < num_threads; ++thread_count)
    {
	info[thread_count].conv_type = conv_type;
	info[thread_count].min = TOOBIG;
	info[thread_count].max = -TOOBIG;
    }
    if (full_array)
    {
	if ( !contiguous_process (array, min_max_contiguous_job_func, NULL) )
	    return (FALSE);
    }
    else
    {
	if ( !scatter_process (array, min_max_scatter_job_func, 2, NULL) )
	    return (FALSE);
    }
    /*  Collect data from threads' private data  */
    for (thread_count = 0; thread_count < num_threads; ++thread_count)
    {
	if (info[thread_count].min < *min) *min = info[thread_count].min;
	if (info[thread_count].max > *max) *max = info[thread_count].max;
    }
    return (TRUE);
}   /*  End Function iarray_min_max  */

/*PUBLIC_FUNCTION*/
flag iarray_scale_and_offset (iarray out, iarray inp, double scale[2],
			      double offset[2], flag magnitude)
/*  [SUMMARY] Scale and offset an Intelligent Array.
    [PURPOSE] This routine will perform a scale and offset on every element in
    an "Intelligent Array" (output = input * scale + offset).
    <out> The output Intelligent Array.
    <inp> The input Intelligent Array.
    [NOTE] The input and output arrays MUST be the same size (though not
    necessarily the same type).
    <scale> The complex scale value.
    <offset> The complex offset value.
    <magnitude> If TRUE and converting from a complex to a real array, the
    magnitude of the complex data (after scale and offset have been applied) is
    used, else the real component of the complex scaled data is used.
    [NOTE] When converting from a real to a complex array, the imaginary
    component of the output array is set to 0.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag contiguous = TRUE;
    flag inp_complex;
    unsigned int elem_count;
    unsigned int inp_stride;
    unsigned int out_stride;
    unsigned int dim_count;
    unsigned int num_dim;
    double data[2];
    double tmp[2];
    char *inp_data;
    char *out_data;
    unsigned long *coordinates;
    double *buffer = NULL;
    double *ptr;
    static char function_name[] = "iarray_scale_and_offset";

    VERIFY_IARRAY (inp);
    VERIFY_IARRAY (out);
    /*  Test array sizes  */
    if ( ( num_dim = iarray_num_dim (inp) ) != iarray_num_dim (out) )
    {
	(void) fprintf ( stderr,
			"Input array has: %u dimensions whilst output array has: %u\n",
			iarray_num_dim (inp), iarray_num_dim (out) );
	return (FALSE);
    }
    /*  Add a tiny offset if output type is integer, to get around rounding
	errors which can cause unsedired truncation to the next lower integer.
	*/
    switch ( iarray_type (out) )
    {
      case K_BYTE:
      case K_INT:
      case K_SHORT:
      case K_LONG:
      case K_UBYTE:
      case K_UINT:
      case K_USHORT:
      case K_ULONG:
      case K_BCOMPLEX:
      case K_ICOMPLEX:
      case K_SCOMPLEX:
      case K_LCOMPLEX:
      case K_UBCOMPLEX:
      case K_UICOMPLEX:
      case K_USCOMPLEX:
      case K_ULCOMPLEX:
	offset[0] += 1e-6;
	offset[1] += 1e-6;
	break;
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if (inp->lengths[dim_count] != out->lengths[dim_count])
	{
	    (void) fprintf (stderr, "Input dimension: %u has length: %lu\n",
			    dim_count, inp->lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %lu\n",
			    dim_count, out->lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
    }
    inp_stride = inp->offsets[num_dim -1][1] - inp->offsets[num_dim -1][0];
    out_stride = out->offsets[num_dim -1][1] - out->offsets[num_dim -1][0];
    /*  Check if lower dimensions are contiguous  */
    if (!inp->contiguous[num_dim - 1])
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if (!out->contiguous[num_dim - 1])
    {
	/*  Output array not contiguous  */
	contiguous = FALSE;
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned long *) m_alloc (sizeof *coordinates *
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
	      m_alloc (sizeof *buffer * 2 * inp->lengths[num_dim - 1]) )
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
	    if ( !ds_get_elements (inp_data, iarray_type (inp),
				   inp_stride, buffer, &inp_complex,
				   inp->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer );
		return (FALSE);
	    }
	    if (inp_complex)
	    {
		/*  Input array is complex  */
		for (elem_count = 0, ptr = buffer;
		     elem_count < inp->lengths[num_dim - 1];
		     ++elem_count, ptr += 2)
		{
		    tmp[0] = ptr[0] * scale[0] - ptr[1] * scale[1];
		    tmp[1] = ptr[0] * scale[1] + ptr[1] * scale[0];
		    ptr[0] = tmp[0] + offset[0];
		    ptr[1] = tmp[1] + offset[1];
		}
		if (!ds_element_is_complex ( iarray_type (out) ) && magnitude)
		{
		    /*  Complex to real conversion  */
		    for (elem_count = 0, ptr = buffer;
			 elem_count < inp->lengths[num_dim - 1];
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
		     elem_count < inp->lengths[num_dim - 1];
		     ++elem_count, ptr += 2)
		{
		    ptr[0] = ptr[0] * scale[0] + offset[0];
		}
	    }
	    if ( !ds_put_elements (out_data, iarray_type (out),
				   out_stride, buffer,
				   inp->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer );
		return (FALSE);
	    }
	    inp_data = iarray_get_next_element (inp, coordinates,
						inp->lengths[num_dim - 1]);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
	else
	{
	    /*  Data is not contiguous: copy one element at a time  */
	    if ( !ds_get_element (inp_data, iarray_type (inp), data,
				  &inp_complex) )
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
		if (!ds_element_is_complex ( iarray_type (out) )
		    && magnitude)
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

/*EXPERIMENTAL_FUNCTION*/
flag iarray_clip_scale_and_offset (iarray out, iarray inp, double scale,
				   double offset,
				   double lower_clip, double upper_clip,
				   flag blank)
/*  [SUMMARY] Clip, scale and offset an Intelligent Array.
    [PURPOSE] This routine will perform a clipping operation followed by a
    scale and offset on every element in  an "Intelligent Array". The operation
    following the clipping is: (output = input * scale + offset).
    <out> The output Intelligent Array.
    <inp> The input Intelligent Array.
    [NOTE] The input and output arrays MUST be the same size (though not
    necessarily the same type). Both arrays must be real.
    <scale> The scale value.
    <offset> The offset value.
    <lower_clip> The input data is clipped so that no value is below this.
    <upper_clip> The input data is clipped so that no value is above this.
    <blank> If TRUE, clipped values are blanked.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag contiguous = TRUE;
    unsigned int elem_count;
    unsigned int inp_stride;
    unsigned int out_stride;
    unsigned int dim_count;
    unsigned int num_dim;
    double toobig = TOOBIG;
    double value;
    double data[2];
    char *inp_data;
    char *out_data;
    unsigned long *coordinates;
    double *buffer = NULL;
    double *ptr;
    static char function_name[] = "iarray_clip_scale_and_offset";

    VERIFY_IARRAY (inp);
    VERIFY_IARRAY (out);
    /*  Test array sizes  */
    if ( ( num_dim = iarray_num_dim (inp) ) != iarray_num_dim (out) )
    {
	(void) fprintf ( stderr,
			"Input array has: %u dimensions whilst output array has: %u\n",
			iarray_num_dim (inp), iarray_num_dim (out) );
	return (FALSE);
    }
    /*  Add a tiny offset if output type is integer, to get around rounding
	errors which can cause unsedired truncation to the next lower integer.
	*/
    if ( ds_element_is_complex ( iarray_type (inp) ) )
    {
	(void) fprintf (stderr, "Input array is complex\n");
	return (FALSE);
    }
    if ( ds_element_is_complex ( iarray_type (out) ) )
    {
	(void) fprintf (stderr, "Input array is complex\n");
	return (FALSE);
    }
    switch ( iarray_type (out) )
    {
      case K_BYTE:
      case K_INT:
      case K_SHORT:
      case K_LONG:
      case K_UBYTE:
      case K_UINT:
      case K_USHORT:
      case K_ULONG:
	offset += 1e-6;
	break;
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if (inp->lengths[dim_count] != out->lengths[dim_count])
	{
	    (void) fprintf (stderr, "Input dimension: %u has length: %lu\n",
			    dim_count, inp->lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %lu\n",
			    dim_count, out->lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
    }
    inp_stride = inp->offsets[num_dim -1][1] - inp->offsets[num_dim -1][0];
    out_stride = out->offsets[num_dim -1][1] - out->offsets[num_dim -1][0];
    /*  Check if lower dimensions are contiguous  */
    if (!inp->contiguous[num_dim - 1])
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if (!out->contiguous[num_dim - 1])
    {
	/*  Output array not contiguous  */
	contiguous = FALSE;
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned long *) m_alloc (sizeof *coordinates *
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
	      m_alloc (sizeof *buffer * 2 * inp->lengths[num_dim - 1]) )
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
	    if ( !ds_get_elements (inp_data, iarray_type (inp),
				   inp_stride, buffer, NULL,
				   inp->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer );
		return (FALSE);
	    }
	    for (elem_count = 0, ptr = buffer;
		 elem_count < inp->lengths[num_dim - 1];
		 ++elem_count, ptr += 2)
	    {
		if ( (value = ptr[0]) >= toobig ) continue;
		if (value < lower_clip)
		{
		    if (blank)
		    {
			ptr[0] = toobig;
			continue;
		    }
		    else value = lower_clip;
		}
		else if (value > upper_clip)
		{
		    if (blank)
		    {
			ptr[0] = toobig;
			continue;
		    }
		    else value = upper_clip;
		}
		ptr[0] = value * scale + offset;
	    }
	    if ( !ds_put_elements (out_data, iarray_type (out),
				   out_stride, buffer,
				   inp->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer );
		return (FALSE);
	    }
	    inp_data = iarray_get_next_element (inp, coordinates,
						inp->lengths[num_dim - 1]);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
	else
	{
	    /*  Data is not contiguous: copy one element at a time  */
	    if ( !ds_get_element (inp_data, iarray_type (inp), data, NULL) )
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    if ( (value = data[0]) < toobig )
	    {
		if (value < lower_clip)
		{
		    if (blank) value = toobig;
		    else value = lower_clip;
		}
		else if (value > upper_clip)
		{
		    if (blank) value = toobig;
		    else value = upper_clip;
		}
		if (value >= toobig) data[0] = toobig;
		else data[0] = value * scale + offset;
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
}   /*  End Function iarray_clip_scale_and_offset  */

/*PUBLIC_FUNCTION*/
flag iarray_add_and_scale (iarray out, iarray inp1, iarray inp2,
			   double scale[2], flag magnitude)
/*  [SUMMARY] Add Intelligent Arrays and scale.
    [PURPOSE] This routine will add two "Intelligent Array" to each other and
    scales the result. The sizes of the two input arrays and the output must be
    identical.
    The routine performs the following computation:
        OUT = INP1 + INP2 * scale
    The routine will automatically perform type conversions if necessary.
    <out> The output Intelligent Array.
    <inp1> The first input Intelligent Array.
    <inp2> The second input Intelligent Array.
    <scale> The complex scale value.
    <magnitude> If TRUE then when converting from a complex to a real data
    type, the magnitude is taken, else the real component is copied.
    [NOTE] When converting from a real to a complex data type, the imaginary
    component is set to zero.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag contiguous = TRUE;
    flag inp1_complex;
    flag inp2_complex;
    unsigned int elem_count;
    unsigned int inp1_stride;
    unsigned int inp2_stride;
    unsigned int out_stride;
    unsigned int dim_count;
    unsigned int num_dim;
    double data1[2];
    double data2[2];
    char *inp1_data;
    char *inp2_data;
    char *out_data;
    unsigned long *coordinates;
    double *buffer1 = NULL;
    double *buffer2 = NULL;
    double *ptr;
    static char function_name[] = "iarray_add_and_scale";

    VERIFY_IARRAY (out);
    VERIFY_IARRAY (inp1);
    VERIFY_IARRAY (inp2);
    /*  Test array sizes  */
    if ( ( num_dim = iarray_num_dim (inp1) ) != iarray_num_dim (out) )
    {
	(void) fprintf ( stderr,
			"Input array1 has: %u dimensions whilst output array has: %u\n",
			iarray_num_dim (inp1), iarray_num_dim (out) );
	return (FALSE);
    }
    if ( iarray_num_dim (inp2) != iarray_num_dim (out) )
    {
	(void) fprintf ( stderr,
			"Input array2 has: %u dimensions whilst output array has: %u\n",
			iarray_num_dim (inp2), iarray_num_dim (out) );
	return (FALSE);
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if (inp1->lengths[dim_count] != out->lengths[dim_count])
	{
	    (void) fprintf (stderr, "Input1 dimension: %u has length: %lu\n",
			    dim_count, inp1->lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %lu\n",
			    dim_count, out->lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
	if (inp2->lengths[dim_count] != out->lengths[dim_count])
	{
	    (void) fprintf (stderr, "Input2 dimension: %u has length: %lu\n",
			    dim_count, inp2->lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %lu\n",
			    dim_count, out->lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
    }
    inp1_stride = inp1->offsets[num_dim-1][1] -inp1->offsets[num_dim-1][0];
    inp2_stride = inp2->offsets[num_dim-1][1] -inp2->offsets[num_dim-1][0];
    out_stride = out->offsets[num_dim -1][1] - out->offsets[num_dim -1][0];
    /*  Check if lower dimensions are contiguous  */
    if (!inp1->contiguous[num_dim - 1])
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if (!inp2->contiguous[num_dim - 1])
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if (!out->contiguous[num_dim - 1])
    {
	/*  Output array not contiguous  */
	contiguous = FALSE;
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned long *) m_alloc (sizeof *coordinates *
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
	      m_alloc (sizeof *buffer1 * 2 * inp1->lengths[num_dim -1]) )
	    == NULL )
	{
	    m_error_notify (function_name, "conversion copy buffer1");
	    m_free ( (char *) coordinates );
	    return (FALSE);
	}
	if ( ( buffer2 = (double *)
	      m_alloc (sizeof *buffer2 * 2 * inp2->lengths[num_dim -1]) )
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
	    if ( !ds_get_elements (inp1_data, iarray_type (inp1),
				   inp1_stride, buffer1, &inp1_complex,
				   inp1->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    if ( !ds_get_elements (inp2_data, iarray_type (inp2),
				   inp2_stride, buffer2, &inp2_complex,
				   inp2->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    for (elem_count = 0; elem_count < inp1->lengths[num_dim - 1];
		 ++elem_count)
	    {
		buffer1[elem_count * 2] += buffer2[elem_count * 2];
		buffer1[elem_count * 2 + 1] += buffer2[elem_count * 2 + 1];
	    }
	    if (!ds_element_is_complex ( iarray_type (out) )
		&& inp1_complex && inp2_complex && magnitude)
	    {
		/*  Complex to real conversion  */
		for (elem_count = 0, ptr = buffer1;
		     elem_count < inp1->lengths[num_dim - 1];
		     ++elem_count, ptr += 2)
		{
		    *ptr = sqrt (ptr[0] * ptr[0] + ptr[1] * ptr[1]);
		}
	    }
	    if ( !ds_put_elements (out_data, iarray_type (out),
				   out_stride, buffer1,
				   out->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    inp1_data = iarray_get_next_element (inp1, coordinates,
						 inp1->lengths[num_dim - 1]);
	    inp2_data = iarray_get_next_element (inp2, coordinates, 0);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
	else
	{
	    /*  Data is not contiguous: copy one element at a time  */
	    if ( !ds_get_element (inp1_data, iarray_type (inp1), data1,
				  &inp1_complex) )
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    if ( !ds_get_element (inp2_data, iarray_type (inp2), data2,
				  &inp1_complex) )
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    data1[0] += data2[0];
	    data1[1] += data2[1];
	    if (!ds_element_is_complex ( iarray_type (out) )
		&& inp1_complex && inp2_complex && magnitude)
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
flag iarray_sub_and_scale (iarray out, iarray inp1, iarray inp2,
			   double scale[2], flag magnitude)
/*  [SUMMARY] Subtract Intelligent Arrays and scale.
    [PURPOSE] This routine will subtract two "Intelligent Array" from each
    other and scales the result. The sizes of the two input arrays and the
    output must be identical.
    The routine performs the following computation:
        OUT = INP1 - INP2 * scale
    The routine will automatically perform type conversions if necessary.
    <out> The output Intelligent Array.
    <inp1> The first input Intelligent Array.
    <inp2> The second input Intelligent Array.
    <scale> The complex scale value.
    <magnitude> If TRUE then when converting from a complex to a real data
    type, the magnitude is taken, else the real component is copied.
    [NOTE] When converting from a real to a complex data type, the imaginary
    component is set to zero.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag contiguous = TRUE;
    flag inp1_complex;
    flag inp2_complex;
    unsigned int elem_count;
    unsigned int inp1_stride;
    unsigned int inp2_stride;
    unsigned int out_stride;
    unsigned int dim_count;
    unsigned int num_dim;
    double data1[2];
    double data2[2];
    char *inp1_data;
    char *inp2_data;
    char *out_data;
    unsigned long *coordinates;
    double *buffer1 = NULL;
    double *buffer2 = NULL;
    double *ptr;
    static char function_name[] = "iarray_sub_and_scale";

    VERIFY_IARRAY (out);
    VERIFY_IARRAY (inp1);
    VERIFY_IARRAY (inp2);
    /*  Test array sizes  */
    if ( ( num_dim = iarray_num_dim (inp1) ) != iarray_num_dim (out) )
    {
	(void) fprintf (stderr,
			"Input array1 has: %u dimensions whilst output array has: %u\n",
			iarray_num_dim (inp1), iarray_num_dim (out) );
	return (FALSE);
    }
    if ( iarray_num_dim (inp2) != iarray_num_dim (out) )
    {
	(void) fprintf (stderr,
			"Input array2 has: %u dimensions whilst output array has: %u\n",
			iarray_num_dim (inp2), iarray_num_dim (out) );
	return (FALSE);
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if (inp1->lengths[dim_count] != out->lengths[dim_count])
	{
	    (void) fprintf (stderr, "Input1 dimension: %u has length: %lu\n",
			    dim_count, inp1->lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %lu\n",
			    dim_count, out->lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
	if (inp2->lengths[dim_count] != out->lengths[dim_count])
	{
	    (void) fprintf (stderr, "Input2 dimension: %u has length: %lu\n",
			    dim_count, inp2->lengths[dim_count]);
	    (void) fprintf (stderr, "Output dimension: %u has length: %lu\n",
			    dim_count, out->lengths[dim_count]);
	    (void) fprintf (stderr, "Must be the same\n");
	    return (FALSE);
	}
    }
    inp1_stride = inp1->offsets[num_dim-1][1] -inp1->offsets[num_dim-1][0];
    inp2_stride = inp2->offsets[num_dim-1][1] -inp2->offsets[num_dim-1][0];
    out_stride = out->offsets[num_dim -1][1] - out->offsets[num_dim -1][0];
    /*  Check if lower dimensions are contiguous  */
    if (!inp1->contiguous[num_dim - 1])
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if (!inp2->contiguous[num_dim - 1])
    {
	/*  Input array not contiguous  */
	contiguous = FALSE;
    }
    if (!out->contiguous[num_dim - 1])
    {
	/*  Output array not contiguous  */
	contiguous = FALSE;
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned long *) m_alloc (sizeof *coordinates *
						    num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of co-ordinate counters");
	return (FALSE);
    }
    m_clear ( (char *) coordinates, sizeof *coordinates * num_dim );
    if (contiguous)
    {
	/*  Lower dimensions are contiguous  */
	/*  Allocate conversion copy buffer  */
	if ( ( buffer1 = (double *)
	      m_alloc (sizeof *buffer1 * 2 * inp1->lengths[num_dim -1]) )
	    == NULL )
	{
	    m_error_notify (function_name, "conversion copy buffer1");
	    m_free ( (char *) coordinates );
	    return (FALSE);
	}
	if ( ( buffer2 = (double *)
	      m_alloc (sizeof *buffer2 * 2 * inp2->lengths[num_dim -1]) )
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
	    if ( !ds_get_elements (inp1_data, iarray_type (inp1),
				   inp1_stride, buffer1, &inp1_complex,
				   inp1->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    if ( !ds_get_elements (inp2_data, iarray_type (inp2),
				   inp2_stride, buffer2, &inp2_complex,
				   inp2->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    for (elem_count = 0; elem_count < inp1->lengths[num_dim - 1];
		 ++elem_count)
	    {
		buffer1[elem_count * 2] -= buffer2[elem_count * 2];
		buffer1[elem_count * 2 + 1] -= buffer2[elem_count * 2 + 1];
	    }
	    if (!ds_element_is_complex ( iarray_type (out) )
		&& inp1_complex && inp2_complex && magnitude)
	    {
		/*  Complex to real conversion  */
		for (elem_count = 0, ptr = buffer1;
		     elem_count < inp1->lengths[num_dim - 1];
		     ++elem_count, ptr += 2)
		{
		    *ptr = sqrt (ptr[0] * ptr[0] + ptr[1] * ptr[1]);
		}
	    }
	    if ( !ds_put_elements (out_data, iarray_type (out),
				   out_stride, buffer1,
				   out->lengths[num_dim - 1]) )
	    {
		m_free ( (char *) coordinates );
		m_free ( (char *) buffer1 );
		m_free ( (char *) buffer2 );
		return (FALSE);
	    }
	    inp1_data = iarray_get_next_element (inp1, coordinates,
						 inp1->lengths[num_dim - 1]);
	    inp2_data = iarray_get_next_element (inp2, coordinates, 0);
	    out_data = iarray_get_next_element (out, coordinates, 0);
	}
	else
	{
	    /*  Data is not contiguous: copy one element at a time  */
	    if ( !ds_get_element (inp1_data, iarray_type (inp1), data1,
				  &inp1_complex) )
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    if ( !ds_get_element (inp2_data, iarray_type (inp2), data2,
				  &inp1_complex) )
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    data1[0] -= data2[0];
	    data1[1] -= data2[1];
	    if (!ds_element_is_complex ( iarray_type (out) )
		&& inp1_complex && inp2_complex && magnitude)
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
CONST char *iarray_dim_name (iarray array, unsigned int index)
/*  [SUMMARY] Get dimension name in an Intelligent Array.
    [PURPOSE] This routine will get the name of a specified dimension in a
    simple, n-dimensional array.
    <array> The Intelligent Array.
    <index> The index of the dimension.
    [RETURNS] A pointer to the name of the specified dimension.
*/
{
    array_desc *arr_desc;
    static char function_name[] = "iarray_dim_name";

    VERIFY_IARRAY (array);
    arr_desc = array->arr_desc;
    if ( index >= iarray_num_dim (array) )
    {
	(void) fprintf ( stderr,
			"Dimension index: %u is not less than number of dimensions: %u\n",
			index, iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    return (arr_desc->dimensions[array->orig_dim_indices[index]]->name);
}   /*  End Function iarray_dim_name  */

/*PUBLIC_FUNCTION*/
void iarray_remap_torus (iarray array, unsigned int boundary_width)
/*  [SUMMARY] Remap Intelligent Array into a torus.
    [PURPOSE] This routine will remap an N-dimensional "Intelligent Array" to a
    pseudo-toroidal array.
    <array> The Intelligent Array.
    <boundary_width> The width of the array boundary within which the array
    appears to be toroidal.
    [RETURNS] Nothing.
*/
{
    unsigned int dim_count;
    unsigned long *offsets;
    uaddr **off_ptr;
    static char function_name[] = "iarray_remap_torus";

    VERIFY_IARRAY (array);
    if (array->offsets == array->arr_desc->offsets)
    {
	/*  Need a new array of pointers  */
	if ( ( off_ptr = (uaddr **) m_alloc ( sizeof *off_ptr *
					     iarray_num_dim (array) ) )
	    == NULL )
	{
	    m_abort (function_name, "array of address offset array pointers");
	}
	m_copy ( (char *) off_ptr, (char *) array->offsets,
		sizeof *off_ptr * iarray_num_dim (array) );
	array->offsets = off_ptr;
    }
    for (dim_count = 0; dim_count < iarray_num_dim (array); ++dim_count)
    {
	if (array->offsets[dim_count] == NULL)
	{
	    (void) fprintf (stderr, "No address offsets for dimension: %u\n",
			    dim_count);
	    a_prog_bug (function_name);
	}
	if ( ( offsets = (unsigned long *)
	      m_alloc ( sizeof *offsets *
		       (array->lengths[dim_count] + 2 * boundary_width) ) )
	    == NULL )
	{
	    m_abort (function_name, "address offset array");
	}
	/*  Copy old offsets  */
	m_copy ( (char *) (offsets + boundary_width),
		(char *) array->offsets[dim_count],
		sizeof *offsets * array->lengths[dim_count] );
	/*  Copy old end to new beginning  */
	m_copy ( (char *) offsets,
		(char *) (array->offsets[dim_count] +
			  array->lengths[dim_count] - boundary_width),
		sizeof *offsets * boundary_width );
	/*  Copy old beginning to new end  */
	m_copy ( (char *) (offsets + array->lengths[dim_count] +
			   boundary_width),
		(char *) array->offsets[dim_count],
		sizeof *offsets * boundary_width );
	m_free ( (char *) (array->offsets[dim_count] -
			   array->boundary_width) );
	array->offsets[dim_count] = offsets + boundary_width;
	array->contiguous[dim_count] = FALSE;
    }
    array->boundary_width = boundary_width;
}   /*  End Function iarray_remap_torus  */

/*PUBLIC_FUNCTION*/
void iarray_set_world_coords (iarray array, unsigned int index, double first,
			      double last)
/*  [SUMMARY] Set the world co-ordinates of an Intelligent Array dimension.
    <array> The Intelligent Array.
    <index> The index of the dimension.
    <first> The first real world co-ordinate.
    <last> The last real world co-ordinate.
    [RETURNS] Nothing.
*/
{
    array_desc *arr_desc;
    static char function_name[] = "iarray_set_world_coords";

    VERIFY_IARRAY (array);
    arr_desc = array->arr_desc;
    if ( index >= iarray_num_dim (array) )
    {
	(void) fprintf ( stderr,
			"Dimension index: %u is not less than number of dimensions: %u\n",
			index, iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    index = array->orig_dim_indices[index];
    arr_desc->dimensions[index]->first_coord = first;
    arr_desc->dimensions[index]->last_coord = last;
    if (first < last)
    {
	arr_desc->dimensions[index]->minimum = first;
	arr_desc->dimensions[index]->maximum = last;
    }
    else
    {
	arr_desc->dimensions[index]->minimum = last;
	arr_desc->dimensions[index]->maximum = first;
    }
}   /*  End Function iarray_set_world_coords  */

/*PUBLIC_FUNCTION*/
void iarray_get_world_coords (iarray array, unsigned int index,
			      double *first, double *last)
/*  [SUMMARY] Get the world co-ordinates of an Intelligent Array dimension.
    <array> The Intelligent Array.
    <index> The index of the dimension.
    <first> The first real world co-ordinate is written here.
    <last> The last real world co-ordinate is written here.
    [RETURNS] Nothing.
*/
{
    array_desc *arr_desc;
    static char function_name[] = "iarray_get_world_coords";

    VERIFY_IARRAY (array);
    arr_desc = array->arr_desc;
    if ( index >= iarray_num_dim (array) )
    {
	(void) fprintf ( stderr,
			"Dimension index: %u is not less than number of dimensions: %u\n",
			index, iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    if ( (first == NULL) || (last == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    index = array->orig_dim_indices[index];
    *first = arr_desc->dimensions[index]->first_coord;
    *last = arr_desc->dimensions[index]->last_coord;
}   /*  End Function iarray_get_world_coords  */

/*PUBLIC_FUNCTION*/
dim_desc *iarray_get_dim_desc (iarray array, unsigned int index)
/*  [SUMMARY] Get a dimension descriptor from an Intelligent Array.
    <array> The Intelligent Array.
    <index> The index of the dimension.
    [RETURNS] A pointer to the dimension descriptor.
*/
{
    array_desc *arr_desc;
    static char function_name[] = "iarray_get_dim_desc";

    VERIFY_IARRAY (array);
    arr_desc = array->arr_desc;
    if ( index >= iarray_num_dim (array) )
    {
	(void) fprintf ( stderr,
			"Dimension index: %u is not less than number of dimensions: %u\n",
			index, iarray_num_dim (array) );
	a_prog_bug (function_name);
    }
    index = array->orig_dim_indices[index];
    return (arr_desc->dimensions[index]);
}   /*  End Function iarray_get_dim_desc  */

/*PUBLIC_FUNCTION*/
flag iarray_compute_histogram (iarray array, unsigned int conv_type,
			       double min, double max, unsigned long num_bins,
			       unsigned long *histogram_array,
			       unsigned long *histogram_peak,
			       unsigned long *histogram_mode)
/*  [SUMMARY] Compute a histogram of an "Intelligent Array".
    <array> The array.
    <conv_type> The conversion type to use for complex numbers. See
    [<DS_COMPLEX_CONVERSIONS>] for legal values. CONV_CtoR_ENVELOPE is not
    legal.
    <min> Data values below this will be ignored.
    <max> Data values above this will be ignored.
    <num_bins> The number of histogram bins.
    <histogram_array> A pointer to the histogram array. The values in this
    array are updated, and hence must be initialised externally.
    <histogram_peak> The peak of the histogram is written here. This value is
    updated, and hence must be externally initialised to 0.
    <histogram_mode> The mode of the histogram (index value of the peak) will
    be written here. This value is updated, and hence must be externally
    initialised to 0.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag full_array;
    histogram_finfo finfo;
    uaddr info_size;
    unsigned int num_dim;
    unsigned int bin_count;
    unsigned int num_threads, thread_count;
    unsigned long hval, hpeak, hmode;
    char *info;
    array_desc *arr_desc;
    unsigned long *harray_ptr;
    extern KThreadPool pool;
    static char function_name[] = "iarray_compute_histogram";

    VERIFY_IARRAY (array);
    if ( (histogram_array == NULL) || (histogram_peak == NULL) ||
	(histogram_mode == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    num_dim = iarray_num_dim (array);
    full_array = iarray_is_full_array (array);
    initialise_thread_pool ();
    num_threads = mt_num_threads (pool);
    if ( full_array && (num_threads < 2) )
    {
	/*  Can do this in one contiguous block  */
	arr_desc = array->arr_desc;
	return ( ds_find_single_histogram
		 (array->data, iarray_type (array), conv_type,
		  ds_get_array_size (arr_desc), NULL,
		  ds_get_packet_size (arr_desc->packet), 
		  min, max, num_bins,
		  histogram_array, histogram_peak, histogram_mode) );
    }
    if ( !full_array && (num_dim == 1) )
    {
	/*  Simple 1-dimensional process  */
	return ( ds_find_single_histogram (array->data, iarray_type (array),
					   conv_type, array->lengths[0],
					   array->offsets[0], 0,
					   min, max, num_bins,
					   histogram_array, histogram_peak,
					   histogram_mode) );
    }
    if ( !full_array && (num_dim == 2) && (num_threads < 2) )
    {
	/*  Simple unthreaded 2-dimensional process  */
	return ( ds_find_2D_histogram (array->data, iarray_type (array),
				       conv_type,
				       array->lengths[0], array->offsets[0],
				       array->lengths[1], array->offsets[1],
				       min, max, num_bins,
				       histogram_array, histogram_peak,
				       histogram_mode) );
    }
    /*  Initialise thread info  */
    info_size = sizeof *histogram_array * num_bins;
    mt_new_thread_info (pool, NULL, info_size);
    info = mt_get_thread_info (pool);
    m_clear (info, info_size * num_threads);
    finfo.conv_type = conv_type;
    finfo.min = min;
    finfo.max = max;
    finfo.num_bins = num_bins;
    if (full_array)
    {
	if ( !contiguous_process (array, histogram_contiguous_job_func,
				  &finfo) ) return (FALSE);
    }
    else
    {
	if ( !scatter_process (array, histogram_scatter_job_func, 2,
			       &finfo) ) return (FALSE);
    }
    hpeak = *histogram_peak;
    hmode = *histogram_mode;
    /*  Collect data from threads' private data  */
    for (bin_count = 0; bin_count < num_bins; ++bin_count)
    {
	hval = 0;  /*  Initialised to keep compiler happy  */
	for (thread_count = 0; thread_count < num_threads; ++thread_count)
	{
	    harray_ptr = (unsigned long *) (info + thread_count * info_size);
	    hval = histogram_array[bin_count] + harray_ptr[bin_count];
	    histogram_array[bin_count] = hval;
	}
	/*  hval contains the running total across threads for this bin  */
	if (hval > hpeak)
	{
	    hpeak = hval;
	    hmode = bin_count;
	}
    }
    *histogram_peak = hpeak;
    *histogram_mode = hmode;
    return (TRUE);
}   /*  End Function iarray_compute_histogram  */


/*  Private functions follow  */

static iarray get_array_from_array (multi_array *multi_desc,
				    unsigned int array_num,
				    array_desc *arr_desc, char *array,
				    unsigned int elem_index)
/*  This routine will create an "Intelligent Array" structure from a Karma,
    n-dimensional array.
    The multi array header must be pointed to by  multi_desc  .The attachment
    count is incremented on successful completion of this routine.
    The index of the general data structure which contains the
    "Intelligent Array" must be given by  array_num  .
    The array descriptor must be pointed to by  arr_desc  .
    The array data must be pointed to by  array  .
    The index of the element in the array packet descriptor must be given by
    elem_index  .
    The routine returns a dynamically allocated intelligent array on success,
    else it prints an error message to the standard output and returns NULL.
*/
{
    unsigned int dim_count;
    iarray new_array;
    dim_desc *dim;
    static char function_name[] = "get_array_from_array";

    if (arr_desc->offsets == NULL)
    {
	/*  No offsets yet: compute  */
	if ( !ds_compute_array_offsets (arr_desc) )
	{
	    m_error_notify (function_name, "offset arrays");
	    return (NULL);
	}
    }
    if (elem_index >= arr_desc->packet->num_elements)
    {
	(void) fprintf (stderr,
			"elem_index: %u  is not less than num elements: %u\n",
			elem_index, arr_desc->packet->num_elements);
	a_prog_bug (function_name);
    }
    if ( ( new_array = (iarray) m_alloc (sizeof *new_array) )
	== NULL )
    {
	m_error_notify (function_name, "Intelligent Array structure");
	return (NULL);
    }
    if ( ( new_array->lengths = (unsigned long *)
	  m_alloc (sizeof *new_array->lengths * arr_desc->num_dimensions) )
	== NULL )
    {
	m_error_notify (function_name, "array of dimension lengths");
	m_free ( (char *) new_array );
	return (NULL);
    }
    for (dim_count = 0; dim_count < arr_desc->num_dimensions; ++dim_count)
    {
	dim = arr_desc->dimensions[dim_count];
	new_array->lengths[dim_count] = dim->length;
    }
    /*  Write the other information  */
    new_array->data = array + ds_get_element_offset (arr_desc->packet,
						     elem_index);
    new_array->array_num = array_num;
    new_array->multi_desc = multi_desc;
    new_array->top_pack_desc = multi_desc->headers[array_num];
    new_array->top_packet = &multi_desc->data[array_num];
    new_array->arr_desc = arr_desc;
    new_array->elem_index = elem_index;
    new_array->num_dim = arr_desc->num_dimensions;
    new_array->orig_dim_indices = NULL;
    new_array->restrictions = NULL;
    if ( !iarray_allocate_records (new_array, FALSE) )
    {
	m_free ( (char *) new_array->lengths );
	m_free ( (char *) new_array );
	return (NULL);
    }
    new_array->offsets = arr_desc->offsets;
    for (dim_count = 0; dim_count < arr_desc->num_dimensions; ++dim_count)
    {
	if (arr_desc->num_levels > 0)
	{
	    /*  Tiled  */
	    new_array->contiguous[dim_count] = FALSE;
	}
	else
	{
	    /*  Not tiled  */
	    new_array->contiguous[dim_count] = TRUE;
	}
	new_array->orig_dim_indices[dim_count] = dim_count;
    }
    ++multi_desc->attachments;
    return (new_array);
}   /*  End Function get_array_from_array  */

static flag iarray_allocate_records (iarray array, flag offsets)
/*  This routine will allocate the array offsets and origins. The dimension
    lengths must be contained within the array.
    The array must be given by  array  .
    If the value of  offsets  is TRUE, then the routine will allocate space for
    the address offset arrays.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int dim_count;
    unsigned int num_dim;
    unsigned int num_restr;
    unsigned long *lengths;
    array_desc *arr_desc;
    static char function_name[] = "iarray_allocate_records";

    array->magic_number = MAGIC_NUMBER;
    array->destroy_callbacks = NULL;
    array->boundary_width = 0;
    arr_desc = array->arr_desc;
    if ( (num_dim = array->num_dim) > arr_desc->num_dimensions )
    {
	(void) fprintf (stderr,
			"iarray num_dim: %u greater than base num_dim: %u\n",
			num_dim, arr_desc->num_dimensions);
	a_prog_bug (function_name);
    }
    num_restr = arr_desc->num_dimensions - num_dim;
    lengths = array->lengths;
    if (offsets)
    {
	if ( ( array->offsets = (uaddr **)
	      m_alloc (sizeof *array->offsets * num_dim) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of offset pointers");
	    return (FALSE);
	}
    }
    else
    {
	array->offsets = NULL;
    }
    if ( ( array->contiguous = (flag *)
	  m_alloc (sizeof *array->contiguous * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "array of offset pointers");
	return (FALSE);
    }
    if ( ( array->orig_dim_indices = (unsigned int *)
	  m_alloc (sizeof *array->orig_dim_indices *
		   arr_desc->num_dimensions) ) == NULL )
    {
	m_error_notify (function_name, "array of original dimension indices");
	return (FALSE);
    }
    if (num_restr > 0)
    {
	if ( ( array->restrictions = (unsigned int *)
	      m_alloc (sizeof *array->restrictions * num_restr) ) == NULL )
	{
	    m_error_notify (function_name, "array of restrictions");
	    return (FALSE);
	}
    }
    else
    {
	array->restrictions = NULL;
    }
    if (!offsets)
    {
	/*  No offsets need be allocated: finished now  */
	return (TRUE);
    }
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	if ( ( array->offsets[dim_count] = (uaddr *)
	      m_alloc (sizeof **array->offsets * lengths[dim_count]) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of offsets");
	    while (dim_count-- > 0)
	    {
		m_free ( (char *) array->offsets[dim_count] );
	    }
	    m_free ( (char *) array->offsets );
	    m_free ( (char *) array->contiguous );
	    m_free ( (char *) array );
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function iarray_allocate_records  */

static char *iarray_get_next_element (iarray array, unsigned long *coordinates,
				      unsigned int increment)
/*  This routine will increment a co-ordinate counter for an array.
    The array must be given by  array  .
    The co-ordinate counters must be pointed to by  coordinates  .
    The increment (of the lowest dimension) must be given by  increment  .
    The routine returns a pointer to the element on success, else it returns
    NULL indicating that the end of the array has been reached.
*/
{
    unsigned int dim_count;
    char *data;
    static char function_name[] = "iarray_get_next_element";

    VERIFY_IARRAY (array);
    dim_count = array->num_dim - 1;
    coordinates[dim_count] += increment;
    if (coordinates[dim_count] < array->lengths[dim_count])
    {
	/*  Have valid co-ordinate  */
	data = array->data;
	for (dim_count = 0; dim_count < iarray_num_dim (array); ++dim_count)
	{
	    data += array->offsets[dim_count][ coordinates[dim_count] ];
	}
	return (data);
    }
    /*  Have exceeded lowest dimension bounds  */
    increment = coordinates[dim_count] / array->lengths[dim_count];
    coordinates[dim_count] %= array->lengths[dim_count];
    while (dim_count-- > 0)
    {
	if ( (coordinates[dim_count] += increment) <
	    array->lengths[dim_count] )
	{
	    /*  Have valid co-ordinate  */
	    data = array->data;
	    for (dim_count = 0; dim_count < iarray_num_dim (array);++dim_count)
	    {
		data += array->offsets[dim_count][ coordinates[dim_count] ];
	    }
	    return (data);
	}
	increment = coordinates[dim_count] / array->lengths[dim_count];
	coordinates[dim_count] %= array->lengths[dim_count];
    }
    return (NULL);
}   /*  End Function iarray_get_next_element  */

static unsigned int iarray_get_max_contiguous (iarray array)
/*  This routine will get the maximum number of contiguous elements in an
    "Intelligent Array".
    The array must be given by  array  .
    The routine returns the number of contiguous elements.
*/
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
	dim = array->arr_desc->dimensions[dim_count];
	/*  Check if dimension is contiguous  */
	if (array->contiguous[dim_count])
	{
	    /*  Elements along dimension are contiguous  */
	    max_contig *= array->lengths[dim_count];
	    if (array->lengths[dim_count] == dim->length)
	    {
		/*  Dimension is full length  */
		continue;
	    }
	}
	return (max_contig);
    }
    return (max_contig);
}   /*  End Function iarray_get_max_contiguous  */

static flag iarray_is_full_array (iarray array)
/*  [SUMMARY] Test if iarray is the same size as the unlerlying array.
    [PURPOSE] This routine will test if an Intelligent Array is the same size
    and dimensionality as the underlying array descriptor.
    <array> The Intelligent Array.
    [RETURNS] TRUE if the Intelligent Array is the same size, else FALSE.
*/
{
    unsigned int count;
    static char function_name[] = "iarray_is_full_array";

    VERIFY_IARRAY (array);
    if (iarray_num_dim (array) != array->arr_desc->num_dimensions)
	return (FALSE);
    for (count = 0; count < iarray_num_dim (array); ++count)
    {
	if (array->lengths[count] !=
	    array->arr_desc->dimensions[count]->length) return (FALSE);
    }
    return (TRUE);
}   /*  End Function iarray_is_full_array  */

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

static void initialise_thread_pool ()
/*  [PURPOSE] This routine will initialise the thread pool.
    [RETURNS] Nothing.
*/
{
    extern KThreadPool pool;
    static char function_name[] = "__iarray_initialise_thread_pool";

    if (pool != NULL) return;
    if ( ( pool = mt_create_pool (NULL) ) == NULL )
    {
	m_abort (function_name, "thread pool");
    }
}   /*  End Function initialise_thread_pool  */


/*  Threaded support using offset arrays  */

typedef struct
{
    KThreadPool pool;
    iarray array;
    uaddr *lengths;
    uaddr **offsets;
    unsigned int num_dim;
    void *f_info;
    flag (*func) (KThreadPool pool, iarray array, char *data,
		  uaddr *lengths, uaddr **offsets,
		  unsigned int num_dim, void *f_info, void *thread_info);
    flag failed;
} scatter_process_info_type;

static flag scatter_process (iarray array,
			     flag (*func) (KThreadPool pool, iarray array,
					   char *data,
					   uaddr *lengths, uaddr **offsets,
					   unsigned int num_dim,
					   void *f_info, void *thread_info),
			     unsigned int max_dim, void *f_info)
/*  [PURPOSE] This routine will perform a generic processing task on an
    Intelligent Array. The task is threaded to make maximum use of multiple
    CPUs. This routine can only be used if the processing tasks are seperable.
    <array> The array.
    <func> The routine to call when some work needs to be done. The interface
    to this routine is as follows:
    [<pre>]
    flag func (KThreadPool pool, iarray array, char *data, uaddr *lengths,
               uaddr **offsets, unsigned int num_dim, void *f_info,
	       void *thread_info)
    *   [PURPOSE] This routine performs a processing task.
        <pool> The thread pool the job is running in. If NULL the job is
	running single-threaded.
        <array> The array.
        <data> A pointer to the section of data to process.
	<lengths> An array of dimension lengths.
	<offsets> An array of address offset array pointers.
	<num_dim> The number of dimensions to process.
	<f_info> The arbitrary function information pointer.
	<thread_info> The arbitrary thread information pointer.
	[RETURNS] TRUE on success, else FALSE.
    *
    [</pre>]
    <max_dim> The maximum number of dimensions the function can deal with.
    <f_info> The arbitrary function information pointer.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    scatter_process_info_type info_struct;
    unsigned int dim_count, num_dim;
    uaddr increment;
    uaddr *lengths, **offsets;
    char *data;
    void *thread_info;
    unsigned long *coordinates;
    extern KThreadPool pool;
    static char function_name[] = "__iarray_scatter_process";

    if (pool == NULL)
    {
	(void) fprintf (stderr, "Thread pool not yet initialised\n");
	a_prog_bug (function_name);
    }
    if (max_dim < 1)
    {
	(void) fprintf (stderr, "max_dim: %u is not greater than zero\n",
			max_dim);
	a_prog_bug (function_name);
    }
    num_dim = iarray_num_dim (array);
    thread_info = mt_get_thread_info (pool);
    if (num_dim == 1)
    {
	/*  Single dimension: don't bother threading  */
	return ( (*func) (NULL, array, array->data,
			  array->lengths, array->offsets, 1,
			  f_info, thread_info) );
    }
    /*  Limit allowable dimensions to one less than array dimensions  */
    if (num_dim <= max_dim) max_dim = num_dim - 1;
    /*  Determine the maximum allowed increment  */
    increment = 1;
    for (dim_count = num_dim - max_dim; dim_count < num_dim; ++dim_count)
    {
	increment *= array->lengths[dim_count];
    }
    /*  Set up co-ordinate counters  */
    if ( ( coordinates = (unsigned long *)
	  m_alloc (sizeof *coordinates * num_dim) ) == NULL )
    {
	m_error_notify (function_name, "array of co-ordinate counters");
	return (FALSE);
    }
    m_clear ( (char *) coordinates, sizeof *coordinates * num_dim);
    data = iarray_get_next_element (array, coordinates, 0);
    lengths = array->lengths + num_dim - max_dim;
    offsets = array->offsets + num_dim - max_dim;
    if (mt_num_threads (pool) < 2)
    {
	/*  No threads: do this the simple way  */
	while (data != NULL)
	{
	    if ( !(*func) (NULL, array, data, lengths, offsets, max_dim,
			   f_info, thread_info) )
	    {
		m_free ( (char *) coordinates );
		return (FALSE);
	    }
	    data = iarray_get_next_element (array, coordinates, increment);
	}
	m_free ( (char *) coordinates );
	return (TRUE);
    }
    /*  Process blocks  */
    info_struct.pool = pool;
    info_struct.array = array;
    info_struct.lengths = lengths;
    info_struct.offsets = offsets;
    info_struct.num_dim = max_dim;
    info_struct.f_info = f_info;
    info_struct.func = func;
    info_struct.failed = FALSE;
    while (data != NULL)
    {
	mt_launch_job (pool, scatter_job_func, &info_struct, data, NULL, NULL);
	data = iarray_get_next_element (array, coordinates, increment);
	if (info_struct.failed)
	{
	    m_free ( (char *) coordinates );
	    mt_wait_for_all_jobs (pool);
	    return (FALSE);
	}
    }
    m_free ( (char *) coordinates );
    mt_wait_for_all_jobs (pool);
    return (info_struct.failed ? FALSE : TRUE);
}   /*  End Function scatter_process  */

static void scatter_job_func (void *pool_info,
			      void *call_info1, void *call_info2,
			      void *call_info3, void *call_info4,
			      void *thread_info)
/*  [PURPOSE] This routine performs a job within a thread of execution.
    <pool_info> The pool information pointer.
    [RETURNS] Nothing.
*/
{
    scatter_process_info_type *info = (scatter_process_info_type *) call_info1;

    if ( ! (*info->func) (info->pool, info->array, (char *) call_info2,
			  info->lengths, info->offsets, info->num_dim,
			  info->f_info, thread_info) )
    {
	info->failed = TRUE;
    }
}   /*  End Function scatter_job_func  */


/*  Private functions to perform some job in parallel using offset arrays  */

static flag min_max_scatter_job_func (KThreadPool pool, iarray array,
				      char *data, uaddr *lengths,
				      uaddr **offsets, unsigned int num_dim,
				      void *f_info, void *thread_info)
/*  [PURPOSE] This routine will do some work to compute the minimum and maximum
    of an array.
    <pool> The thread pool the job is running in. If NULL the job is running
    single-threaded.
    <array> The array.
    <data> The section of data to process.
    <lengths> An array of dimension lengths.
    <offsets> An array of address offset array pointers.
    <num_dim> The number of dimensions to process.
    <f_info> The arbitrary function information pointer.
    <thread_info> The arbitrary thread information pointer.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    min_max_thread_info *info = (min_max_thread_info *) thread_info;
    static char function_name[] = "__iarray_min_max_scatter_job_func";

    if (num_dim == 1)
    {
	return ( ds_find_1D_extremes (data, lengths[0], offsets[0],
				      iarray_type (array), info->conv_type,
				      &info->min, &info->max) );
    }
    if (num_dim == 2)
    {
	return ( ds_find_2D_extremes (data, lengths[0], offsets[0],
				      lengths[1], offsets[1],
				      iarray_type (array), info->conv_type,
				      &info->min, &info->max) );
    }
    (void) fprintf (stderr, "num_dim: %u illegal\n", num_dim);
    a_prog_bug (function_name);
    return (FALSE);
}   /*  End Function min_max_scatter_job_func  */

static flag histogram_scatter_job_func (KThreadPool pool, iarray array,
					char *data, uaddr *lengths,
					uaddr **offsets, unsigned int num_dim,
					void *f_info, void *thread_info)
/*  [PURPOSE] This routine will do some work to compute the histogram of an
    array.
    <pool> The thread pool the job is running in. If NULL the job is running
    single-threaded.
    <array> The array.
    <data> The section of data to process.
    <lengths> An array of dimension lengths.
    <offsets> An array of address offset array pointers.
    <num_dim> The number of dimensions to process.
    <f_info> The arbitrary function information pointer.
    <thread_info> The arbitrary thread information pointer.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned long hpeak = 0;
    unsigned long hmode = 0;
    histogram_finfo *info = (histogram_finfo *) f_info;
    unsigned long *histogram_array = (unsigned long *) thread_info;
    static char function_name[] = "__iarray_histogram_scatter_job_func";

    if (num_dim == 1)
    {
	return ( ds_find_single_histogram (data, iarray_type (array),
					   info->conv_type,
					   lengths[0], offsets[0], 0,
					   info->min, info->max,info->num_bins,
					   histogram_array, &hpeak, &hmode) );
    }
    if (num_dim == 2)
    {
	return ( ds_find_2D_histogram (data, iarray_type (array),
				       info->conv_type,
				       lengths[0], offsets[0],
				       lengths[1], offsets[1],
				       info->min, info->max,info->num_bins,
				       histogram_array, &hpeak, &hmode) );
    }
    (void) fprintf (stderr, "num_dim: %u illegal\n", num_dim);
    a_prog_bug (function_name);
    return (FALSE);
}   /*  End Function histogram_scatter_job_func  */


/*  Threaded support for contiguous data  */

typedef struct
{
    KThreadPool pool;
    iarray array;
    void *f_info;
    uaddr stride;
    flag (*func) (KThreadPool pool, iarray array, char *data, uaddr stride,
		  unsigned int num_values, void *f_info, void *thread_info);
    flag failed;
} contiguous_process_info_type;

static flag contiguous_process (iarray array,
				flag (*func) (KThreadPool pool, iarray array,
					      char *data, uaddr stride,
					      unsigned int num_values,
					      void *f_info, void *thread_info),
				void *f_info)
/*  [PURPOSE] This routine will perform a generic processing task on an
    Intelligent Array. The task is threaded to make maximum use of multiple
    CPUs. This routine can only be used if the processing tasks are seperable.
    <array> The array.
    <func> The routine to call when some work needs to be done. The interface
    to this routine is as follows:
    [<pre>]
    flag func (KThreadPool pool, iarray array, char *data, uaddr stride,
               unsigned int num_values, void *f_info, void *thread_info)
    *   [PURPOSE] This routine performs a processing task.
        <pool> The thread pool the job is running in. If NULL the job is
	running single-threaded.
        <array> The array.
        <data> A pointer to the section of data to process.
	<stride> The stride in bytes between consecutive data values.
	<num_values> The number of values to process.
	<f_info> The arbitrary function information pointer.
	<thread_info> The arbitrary thread information pointer.
	[RETURNS] TRUE on success, else FALSE.
    *
    [</pre>]
    <f_info> The arbitrary function information pointer.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    contiguous_process_info_type info_struct;
    uaddr num_values, block_size;
    uaddr stride;
    char *data;
    void *thread_info;
    extern KThreadPool pool;
    static char function_name[] = "__iarray_contiguous_process";

    if (pool == NULL)
    {
	(void) fprintf (stderr, "Thread pool not yet initialised\n");
	a_prog_bug (function_name);
    }
    thread_info = mt_get_thread_info (pool);
    data = array->data;
    stride = ds_get_packet_size (array->arr_desc->packet);
    num_values = ds_get_array_size (array->arr_desc);
    if (mt_num_threads (pool) < 2)
    {
	/*  No threads: do this the simple way  */
	return ( (*func) (NULL, array, data, stride, num_values,
			  f_info, thread_info) );
    }
    /*  Process blocks  */
    info_struct.pool = pool;
    info_struct.array = array;
    info_struct.stride = stride;
    info_struct.f_info = f_info;
    info_struct.func = func;
    info_struct.failed = FALSE;
    block_size = num_values / mt_num_threads (pool);
    for (; num_values > 0;
	 num_values -= block_size, data += stride * block_size)
    {
	if (block_size > num_values) block_size = num_values;
	mt_launch_job (pool, contiguous_job_func, &info_struct, data,
		       (void *) block_size, NULL);
	if (info_struct.failed)
	{
	    mt_wait_for_all_jobs (pool);
	    return (FALSE);
	}
    }
    mt_wait_for_all_jobs (pool);
    return (info_struct.failed ? FALSE : TRUE);
}   /*  End Function contiguous_process  */

static void contiguous_job_func (void *pool_info,
				 void *call_info1, void *call_info2,
				 void *call_info3, void *call_info4,
				 void *thread_info)
/*  [PURPOSE] This routine performs a job within a thread of execution.
    <pool_info> The pool information pointer.
    [RETURNS] Nothing.
*/
{
    contiguous_process_info_type *info = (contiguous_process_info_type *) call_info1;

    if ( ! (*info->func) (info->pool, info->array, (char *) call_info2,
			  info->stride, (uaddr) call_info3,
			  info->f_info, thread_info) )
    {
	info->failed = TRUE;
    }
}   /*  End Function contiguous_job_func  */

/*  Private functions to perform some job in parallel with contiguous data  */


static flag min_max_contiguous_job_func (KThreadPool pool, iarray array,
					 char *data, uaddr stride,
					 unsigned int num_values,
					 void *f_info, void *thread_info)
/*  [PURPOSE] This routine performs a processing task.
    <pool> The thread pool the job is running in. If NULL the job is
    running single-threaded.
    <array> The array.
    <data> A pointer to the section of data to process.
    <stride> The stride in bytes between consecutive data values.
    <num_values> The number of values to process.
    <f_info> The arbitrary function information pointer.
    <thread_info> The arbitrary thread information pointer.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    min_max_thread_info *info = (min_max_thread_info *) thread_info;
    array_desc *arr_desc;

    arr_desc = array->arr_desc;
    return ( ds_find_contiguous_extremes
	     (data, num_values, stride, iarray_type (array),
	      info->conv_type, &info->min, &info->max) );
}   /*  End Function min_max_contiguous_job_func  */

static flag histogram_contiguous_job_func (KThreadPool pool, iarray array,
					   char *data, uaddr stride,
					   unsigned int num_values,
					   void *f_info, void *thread_info)
/*  [PURPOSE] This routine performs a processing task.
    <pool> The thread pool the job is running in. If NULL the job is
    running single-threaded.
    <array> The array.
    <data> A pointer to the section of data to process.
    <stride> The stride in bytes between consecutive data values.
    <num_values> The number of values to process.
    <f_info> The arbitrary function information pointer.
    <thread_info> The arbitrary thread information pointer.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned long hpeak = 0;
    unsigned long hmode = 0;
    histogram_finfo *info = (histogram_finfo *) f_info;
    unsigned long *histogram_array = (unsigned long *) thread_info;
    array_desc *arr_desc;

    arr_desc = array->arr_desc;
    return ( ds_find_single_histogram (data, iarray_type (array),
				       info->conv_type, num_values,
				       NULL, stride,
				       info->min, info->max, info->num_bins,
				       histogram_array, &hpeak, &hmode) );
}   /*  End Function histogram_contiguous_job_func  */


/*  Miscellaneuous support routines  */

static flag ds_find_2D_histogram (CONST char *data, unsigned int elem_type,
				  unsigned int conv_type,
				  unsigned int length1, CONST uaddr *offsets1,
				  unsigned int length2, CONST uaddr *offsets2,
				  double min, double max,
				  unsigned long num_bins,
				  unsigned long *histogram_array,
				  unsigned long *histogram_peak,
				  unsigned long *histogram_mode)
/*  [PURPOSE] This routine will find the histogram of a single plane (element
    versus two dimensions). This routine may be called repeatedly with multiple
    planes in order to build an aggregate histogram of all planes.
    <data> A pointer to the data. Misaligned data will cause bus errors on some
    platforms.
    <elem_type> The type of the element.
    <conv_type> The type of conversion to use for complex numbers.
    Legal value for this include:
        CONV_CtoR_REAL        CONV_CtoR_IMAG
        CONV_CtoR_ABS         CONV_CtoR_SQUARE_ABS
	CONV_CtoR_PHASE       CONV_CtoR_CONT_PHASE
    <length1> The length of one of the dimensions.
    <offsets1> The address offsets for data along the dimension.
    <length2> The length of the other the dimension.
    <offsets2> The address offsets for data along the dimension.
    <min> Data values below this will be ignored.
    <max> Data values above this will be ignored.
    <num_bins> The number of histogram bins.
    <histogram_array> The histogram array. The values in this array are updated
    and hence must be initialised externally.
    <histogram_peak> The peak of the histogram is written here. This value is
    updated, and hence must be externally initialised to 0.
    <histogram_mode> The mode of the histogram (index value of the peak) will
    be written here. This value is updated, and hence must be externally
    initialised to 0.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int count;

    for (count = 0; count < length1; ++count)
    {
	if ( !ds_find_single_histogram (data + offsets1[count],
					elem_type, conv_type,
					length2, offsets2, 0,
					min, max, num_bins,
					histogram_array, histogram_peak,
					histogram_mode) ) return (FALSE);
    }
    return (TRUE);
}   /*  End Function ds_find_2D_histogram  */
