/*LINTLIBRARY*/
/*PREFIX:"ds_"*/
/*  ds_mod_desc.c

    This code provides Karma data structure descriptor manipulation routines.

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

/*

    This file contains the various utility routines for modifying descriptors
  for the general data structure supported in Karma.


    Written by      Richard Gooch   16-OCT-1992

    Updated by      Richard Gooch   16-OCT-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   11-JAN-1993: Added support for tiling and
  address offset information in array descriptors.

    Updated by      Richard Gooch   29-JAN-1993: Added ds_compute_array_offsets

    Updated by      Richard Gooch   7-FEB-1993: Added ds_append_gen_struct

    Updated by      Richard Gooch   11-FEB-1993: Added ds_prepend_dim_desc

    Last updated by Richard Gooch   2-JUN-1993: Removed illegal use/ dependence
  on  st_dup  .


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <string.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>


/*  Private functions  */
static void compute_offsets (/* offsets, num, stride */);


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag ds_remove_dim_desc (arr_desc, dim_name)
/*  This routine will remove a dimension descriptor from an array descriptor.
    Tiling information is preserved, however, any address offset information is
    removed.
    With the exception of the dimension to be removed, the order of the
    dimensions is unaffected.
    The array descriptor must be pointed to by  arr_desc  .
    The name of the dimension to be removed must be pointed to by  dim_name  .
    The routine returns TRUE on success, else it returns FALSE.
*/
array_desc *arr_desc;
char *dim_name;
{
    unsigned int dim_count;
    unsigned int dim_num;
    unsigned int *new_lengths;
    unsigned int **new_tile_lengths;
    dim_desc *dimension;
    dim_desc **new_dimensions;
    static char function_name[] = "ds_remove_dim_desc";

    /*  Check for bad pointers  */
    if ( (arr_desc == NULL) || (dim_name == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Find dimension index  */
    if ( ( dim_num = ds_f_dim_in_array (arr_desc, dim_name) )
	>= (*arr_desc).num_dimensions )
    {
	(void) fprintf (stderr, "Dimension name: \"%s\" not found\n",
			dim_name);
	return (FALSE);
    }
    /*  Remove any address offset information if it exists  */
    if ( (*arr_desc).offsets != NULL )
    {
	for (dim_count = 0; dim_count < (*arr_desc).num_dimensions;
	     ++dim_count)
	{
	    m_free ( (char *) (*arr_desc).offsets[dim_count] );
	}
	m_free ( (char *) (*arr_desc).offsets );
	(*arr_desc).offsets = NULL;
    }
    /*  Allocate new dimension descriptor pointer array  */
    if ( ( new_dimensions = (dim_desc **) m_alloc
	  ( sizeof *new_dimensions * ( (*arr_desc).num_dimensions - 1 ) ) )
	== NULL )
    {
	m_error_notify (function_name,
			"array of dimension descriptor pointers");
	return (FALSE);
    }
    /*  Allocate new lengths pointer array  */
    if ( ( new_lengths = (unsigned int *) m_alloc
	  ( sizeof *new_lengths * ( (*arr_desc).num_dimensions - 1 ) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of lengths");
	m_free ( (char *) new_dimensions );
	return (FALSE);
    }
    if ( (*arr_desc).num_levels > 0 )
    {
	/*  Allocate new tile lengths pointer array  */
	if ( ( new_tile_lengths = (unsigned int **) m_alloc
	      ( sizeof *new_tile_lengths* ( (*arr_desc).num_dimensions -1 ) ) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of tile length pointers");
	    m_free ( (char *) new_dimensions );
	    m_free ( (char *) new_lengths );
	    return (FALSE);
	}
    }
    else
    {
	new_tile_lengths = NULL;
    }
    /*  Copy dimension descriptor pointers (except the named one)  */
    for (dim_count = 0; dim_count < dim_num; ++dim_count)
    {
	new_dimensions[dim_count] = (*arr_desc).dimensions[dim_count];
	new_lengths[dim_count] = (*arr_desc).lengths[dim_count];
	if ( (*arr_desc).num_levels > 0 )
	{
	    new_tile_lengths[dim_count] = (*arr_desc).tile_lengths[dim_count];
	}
    }
    for (dim_count = dim_num + 1; dim_count < (*arr_desc).num_dimensions;
	 ++dim_count)
    {
	new_dimensions[dim_count - 1] = (*arr_desc).dimensions[dim_count];
	new_lengths[dim_count - 1] = (*arr_desc).lengths[dim_count];
	if ( (*arr_desc).num_levels > 0 )
	{
	    new_tile_lengths[dim_count-1] =(*arr_desc).tile_lengths[dim_count];
	}
    }
    /*  Deallocate dimension descriptor  */
    dimension = (*arr_desc).dimensions[dim_num];
    /*  Deallocate dimension name   */
    if ( (*dimension).name != NULL )
    {
	m_free ( (*dimension).name );
    }
    /*  Deallocate dimension co-ordinates   */
    if ( (*dimension).coordinates != NULL )
    {
	m_free ( (char *) (*dimension).coordinates );
    }
    /*  Deallocate actual dimension descriptor */
    m_free ( (char *) dimension);
    /*  Deallocate tile length information for dimension  */
    if ( (*arr_desc).num_levels > 0 )
    {
	m_free ( (char *) (*arr_desc).tile_lengths[dim_num] );
    }
    /*  Deallocate old pointer arrays and link in new info  */
    m_free ( (char *) (*arr_desc).dimensions );
    m_free ( (char *) (*arr_desc).lengths );
    if ( (*arr_desc).num_levels > 0 )
    {
	m_free ( (char *) (*arr_desc).tile_lengths );
    }
    --(*arr_desc).num_dimensions;
    (*arr_desc).dimensions = new_dimensions;
    (*arr_desc).lengths = new_lengths;
    (*arr_desc).tile_lengths = new_tile_lengths;
    /*  Return OK  */
    return (TRUE);
}   /*  End Function ds_remove_dim_desc  */

/*PUBLIC_FUNCTION*/
flag ds_append_dim_desc (arr_desc, dimension)
/*  This routine will append a dimension descriptor to the list of dimensions
    attached to an array descriptor. The appended dimension will be the LEAST
    significant dimension (co-ordinates have lowest stride).
    Tiling information is preserved, however, any address offset information is
    removed. If the array is NOT tiled, the dimension length will be copied
    into the array of bottom tile lengths.
    The array descriptor must be pointed to by  arr_desc  .
    The dimension descriptor must be pointed to by  dimension  .
    The routine returns TRUE on success, else it returns FALSE.
*/
array_desc *arr_desc;
dim_desc *dimension;
{
    unsigned int dim_count;
    unsigned int *new_lengths;
    unsigned int **new_tile_lengths;
    dim_desc **new_dimensions;
    static char function_name[] = "ds_append_dim_desc";

    if ( (arr_desc == NULL) || (dimension == NULL) )
    {
	(void) fprintf (stderr, "NULL descriptor pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Check for repeated dimension name  */
    if (ds_f_dim_in_array (arr_desc, (*dimension).name)
	< (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr, "Another dimension with name: \"%s\" exists\n",
			(*dimension).name);
	a_prog_bug (function_name);
    }
    /*  Remove any address offset information if it exists  */
    if ( (*arr_desc).offsets != NULL )
    {
	for (dim_count = 0; dim_count < (*arr_desc).num_dimensions;
	     ++dim_count)
	{
	    m_free ( (char *) (*arr_desc).offsets[dim_count] );
	}
	m_free ( (char *) (*arr_desc).offsets );
	(*arr_desc).offsets = NULL;
    }
    /*  Allocate new dimension descriptor pointer array  */
    if ( ( new_dimensions = (dim_desc **) m_alloc
	  ( sizeof *new_dimensions * ( (*arr_desc).num_dimensions + 1 ) )
	  ) == NULL )
    {
	m_error_notify (function_name,
			  "array of dimension descriptor pointers");
	return (FALSE);
    }
    /*  Allocate new lengths pointer array  */
    if ( ( new_lengths = (unsigned int *) m_alloc
	  ( sizeof *new_lengths * ( (*arr_desc).num_dimensions + 1 ) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of lengths");
	m_free ( (char *) new_dimensions );
	return (FALSE);
    }
    if ( (*arr_desc).num_levels > 0 )
    {
	/*  Allocate new tile lengths pointer array  */
	if ( ( new_tile_lengths = (unsigned int **) m_alloc
	      ( sizeof *new_tile_lengths* ( (*arr_desc).num_dimensions +1 ) ) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of tile length pointers");
	    m_free ( (char *) new_dimensions );
	    m_free ( (char *) new_lengths );
	    return (FALSE);
	}
	if ( ( new_tile_lengths[(*arr_desc).num_dimensions] = (unsigned int *)
	      m_alloc (sizeof **new_tile_lengths * (*arr_desc).num_levels) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of tile lengths");
	    m_free ( (char *) new_dimensions );
	    m_free ( (char *) new_lengths );
	    m_free ( (char *) new_tile_lengths );
	    return (FALSE);
	}
    }
    else
    {
	new_tile_lengths = NULL;
    }
    /*  Copy dimension descriptor pointers  */
    m_copy ( (char *) new_dimensions,
	    (char *) (*arr_desc).dimensions,
	    sizeof *new_dimensions * (*arr_desc).num_dimensions );
    /*  Copy new dimension descriptor pointer  */
    new_dimensions[(*arr_desc).num_dimensions] = dimension;
    /*  Copy lengths  */
    m_copy ( (char *) new_lengths,
	    (char *) (*arr_desc).lengths,
	    sizeof *new_lengths * (*arr_desc).num_dimensions );
    new_lengths[(*arr_desc).num_dimensions] = 0;
    if ( (*arr_desc).num_levels > 0 )
    {
	/*  Copy tile length pointers  */
	m_copy ( (char *) new_tile_lengths,
		(char *) (*arr_desc).tile_lengths,
		sizeof *new_tile_lengths * (*arr_desc).num_dimensions );
    }
    m_free ( (char *) (*arr_desc).dimensions );
    m_free ( (char *) (*arr_desc).lengths );
    if ( (*arr_desc).num_levels > 0 )
    {
	/*  Tiled: remove old pointer array of tile lengths  */
	m_free ( (char *) (*arr_desc).tile_lengths );
    }
    else
    {
	/*  Not tiled: copy dimension length into length array  */
	new_lengths[(*arr_desc).num_dimensions] = (*dimension).length;
    }
    ++(*arr_desc).num_dimensions;
    (*arr_desc).dimensions = new_dimensions;
    (*arr_desc).lengths = new_lengths;
    (*arr_desc).tile_lengths = new_tile_lengths;
    return (TRUE);
}   /*  End Function ds_append_dim_desc  */

/*PUBLIC_FUNCTION*/
flag ds_prepend_dim_desc (arr_desc, dimension)
/*  This routine will prepend a dimension descriptor to the list of dimensions
    attached to an array descriptor. The prepended dimension will be the MOST
    significant dimension (co-ordinates have greatest stride).
    Tiling information is preserved, however, any address offset information is
    removed. If the array is NOT tiled, the dimension length will be copied
    into the array of bottom tile lengths.
    The array descriptor must be pointed to by  arr_desc  .
    The dimension descriptor must be pointed to by  dimension  .
    The routine returns TRUE on success, else it returns FALSE.
*/
array_desc *arr_desc;
dim_desc *dimension;
{
    unsigned int dim_count;
    unsigned int *new_lengths;
    unsigned int **new_tile_lengths;
    dim_desc **new_dimensions;
    static char function_name[] = "ds_prepend_dim_desc";

    if ( (arr_desc == NULL) || (dimension == NULL) )
    {
	(void) fprintf (stderr, "NULL descriptor pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Check for repeated dimension name  */
    if (ds_f_dim_in_array (arr_desc, (*dimension).name)
	< (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr, "Another dimension with name: \"%s\" exists\n",
			(*dimension).name);
	a_prog_bug (function_name);
    }
    /*  Remove any address offset information if it exists  */
    if ( (*arr_desc).offsets != NULL )
    {
	for (dim_count = 0; dim_count < (*arr_desc).num_dimensions;
	     ++dim_count)
	{
	    m_free ( (char *) (*arr_desc).offsets[dim_count] );
	}
	m_free ( (char *) (*arr_desc).offsets );
	(*arr_desc).offsets = NULL;
    }
    /*  Allocate new dimension descriptor pointer array  */
    if ( ( new_dimensions = (dim_desc **) m_alloc
	  ( sizeof *new_dimensions * ( (*arr_desc).num_dimensions + 1 ) )
	  ) == NULL )
    {
	m_error_notify (function_name,
			  "array of dimension descriptor pointers");
	return (FALSE);
    }
    /*  Allocate new lengths pointer array  */
    if ( ( new_lengths = (unsigned int *) m_alloc
	  ( sizeof *new_lengths * ( (*arr_desc).num_dimensions + 1 ) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of lengths");
	m_free ( (char *) new_dimensions );
	return (FALSE);
    }
    if ( (*arr_desc).num_levels > 0 )
    {
	/*  Allocate new tile lengths pointer array  */
	if ( ( new_tile_lengths = (unsigned int **) m_alloc
	      ( sizeof *new_tile_lengths* ( (*arr_desc).num_dimensions +1 ) ) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of tile length pointers");
	    m_free ( (char *) new_dimensions );
	    m_free ( (char *) new_lengths );
	    return (FALSE);
	}
	if ( ( new_tile_lengths[(*arr_desc).num_dimensions] = (unsigned int *)
	      m_alloc (sizeof **new_tile_lengths * (*arr_desc).num_levels) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of tile lengths");
	    m_free ( (char *) new_dimensions );
	    m_free ( (char *) new_lengths );
	    m_free ( (char *) new_tile_lengths );
	    return (FALSE);
	}
    }
    else
    {
	new_tile_lengths = NULL;
    }
    /*  Copy dimension descriptor pointers  */
    m_copy ( (char *) (new_dimensions + 1),
	    (char *) (*arr_desc).dimensions,
	    sizeof *new_dimensions * (*arr_desc).num_dimensions );
    /*  Copy new dimension descriptor pointer  */
    new_dimensions[0] = dimension;
    /*  Copy lengths  */
    m_copy ( (char *) (new_lengths + 1),
	    (char *) (*arr_desc).lengths,
	    sizeof *new_lengths * (*arr_desc).num_dimensions );
    new_lengths[0] = 0;
    if ( (*arr_desc).num_levels > 0 )
    {
	/*  Copy tile length pointers  */
	m_copy ( (char *) (new_tile_lengths + 1),
		(char *) (*arr_desc).tile_lengths,
		sizeof *new_tile_lengths * (*arr_desc).num_dimensions );
    }
    m_free ( (char *) (*arr_desc).dimensions );
    m_free ( (char *) (*arr_desc).lengths );
    if ( (*arr_desc).num_levels > 0 )
    {
	/*  Tiled: remove old pointer array of tile lengths  */
	m_free ( (char *) (*arr_desc).tile_lengths );
    }
    else
    {
	/*  Not tiled: copy dimension length into length array  */
	new_lengths[0] = (*dimension).length;
    }
    ++(*arr_desc).num_dimensions;
    (*arr_desc).dimensions = new_dimensions;
    (*arr_desc).lengths = new_lengths;
    (*arr_desc).tile_lengths = new_tile_lengths;
    return (TRUE);
}   /*  End Function ds_prepend_dim_desc  */

/*PUBLIC_FUNCTION*/
flag ds_compute_array_offsets (arr_desc)
/*  This routine will compute array address offsets for each dimension in the
    array.
    The array descriptor must be pointed to by  arr_desc  .The array
    descriptor is modified.
    The routine returns TRUE on success, else it returns FALSE.
*/
array_desc *arr_desc;
{
    flag more;
    int dim_count;
    int dim1_count;
    int level;
    unsigned int stride;
    unsigned int tile_size;
    unsigned int bot_tile_size;
    unsigned int coord_count;
    unsigned int offset;
    unsigned int block;
    unsigned int *coords;
    dim_desc *dim;
    static char function_name[] = "ds_compute_array_offsets";

    if (arr_desc == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*arr_desc).offsets == NULL )
    {
	if ( ( (*arr_desc).offsets = (unsigned int **)
	      m_alloc (sizeof *(*arr_desc).offsets *
		       (*arr_desc).num_dimensions) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of offset array pointers");
	    return (FALSE);
	}
	m_clear ( (char *) (*arr_desc).offsets, sizeof *(*arr_desc).offsets *
		 (*arr_desc).num_dimensions );
    }
    stride = ds_get_packet_size ( (*arr_desc).packet );
    if ( (*arr_desc).num_levels > 0 )
    {
	/*  Tiled: need co-ordinate array  */
	if ( ( coords = (unsigned int *)
	      m_alloc (sizeof *coords * (*arr_desc).num_levels) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of coordinate indices");
	    m_free ( (char *) (*arr_desc).offsets );
	    return (FALSE);
	}
	/*  Compute bottom tile size  */
	for (dim1_count = 0, bot_tile_size = stride;
	     dim1_count < (*arr_desc).num_dimensions;
	     ++dim1_count)
	{
	    bot_tile_size *= (*arr_desc).lengths[dim1_count];
	}
    }
    /*  Allocate and compute for each dimension  */
    for (dim_count = (*arr_desc).num_dimensions - 1; dim_count >= 0;
	 --dim_count)
    {
	dim = (*arr_desc).dimensions[dim_count];
	/*  Allocate  */
	if ( (*arr_desc).offsets[dim_count] == NULL )
	{
	    if ( ( (*arr_desc).offsets[dim_count] = (unsigned int *)
		  m_alloc (sizeof **(*arr_desc).offsets * (*dim).length) )
		== NULL )
	    {
		m_error_notify (function_name, "offset array");
		/*  Deallocate  */
		while (++dim_count < (*arr_desc).num_dimensions)
		{
		    m_free ( (char *) (*arr_desc).offsets[dim_count - 1] );
		}
		m_free ( (char *) (*arr_desc).offsets );
		(*arr_desc).offsets = NULL;
		m_free ( (char *) coords );
		return (FALSE);
	    }
	}
	/*  Compute  */
	if ( (*arr_desc).num_levels < 1 )
	{
	    /*  Not tiled  */
	    compute_offsets ( (*arr_desc).offsets[dim_count], (*dim).length,
			     0, stride );
	    stride *= (*dim).length;
	}
	else
	{
	    /*  Tiled  */
	    tile_size = bot_tile_size;
	    m_clear ( (char *) coords,
		     sizeof *coords * (*arr_desc).num_levels );
	    more = TRUE;
	    coord_count = 0;
	    while (more)
	    {
		/*  New tile: compute offset  */
		tile_size = bot_tile_size;
		offset = 0;
		for (level = (*arr_desc).num_levels - 1; level >= 0; --level)
		{
		    block = tile_size;
		    for (dim1_count = (*arr_desc).num_dimensions - 1;
			 dim1_count > dim_count; --dim1_count)
		    {
			block *= (*arr_desc).tile_lengths[dim1_count][level];
		    }
		    offset += block * coords[level];
		    for (dim1_count = 0;
			 dim1_count < (*arr_desc).num_dimensions;
			 ++dim1_count)
		    {
			tile_size*=(*arr_desc).tile_lengths[dim1_count][level];
		    }
		}
		compute_offsets ( (*arr_desc).offsets[dim_count] + coord_count,
				 (*arr_desc).lengths[dim_count],
				 offset, stride );
		coord_count += (*arr_desc).lengths[dim_count];
		/*  Increment in tile  */
		level = (*arr_desc).num_levels - 1;
		while (more)
		{
		    if (++coords[level] <
			(*arr_desc).tile_lengths[dim_count][level])
		    {
			/*  Still OK for this tile  */
			more = FALSE;
			continue;
		    }
		    /*  Step up to the next tile  */
		    coords[level] = 0;
		    if (--level < 0)
		    {
			more = FALSE;
			continue;
		    }
		}
		/*  Jumped to next tile (or done)  */
		if (level < 0)
		{
		    /*  Done: go to next dimension  */
		    continue;
		}
		more = TRUE;
	    }
	    (void) fprintf (stderr, "WARNING: tiling being used\n");
	    stride *= (*arr_desc).lengths[dim_count];
	}
    }
    return (TRUE);
}   /*  End Function ds_compute_array_offsets  */

/*PUBLIC_FUNCTION*/
void ds_remove_tiling_info (arr_desc)
/*  This routine will remove any tiling information from an array descriptor.
    The routine will NOT remove (or change) any offset information.
    The array descriptor must be pointed to by  arr_desc  .
    The routine returns nothing.
*/
array_desc *arr_desc;
{
    unsigned int dim_count;

    if ( (*arr_desc).num_levels > 0 )
    {
	for (dim_count = 0; dim_count < (*arr_desc).num_dimensions;
	     ++dim_count)
	{
	    m_free ( (char *) (*arr_desc).tile_lengths[dim_count] );
	}
	m_free ( (char *) (*arr_desc).tile_lengths );
	(*arr_desc).num_levels = 0;
	(*arr_desc).tile_lengths = NULL;
    }
}   /*  End Function ds_remove_tiling_info  */

/*PUBLIC_FUNCTION*/
flag ds_append_gen_struct (multi_desc, pack_desc, packet, existing_arrayname,
			   append_arrayname)
/*  This routine will append a general data structure to a multi-array general
    data structure.
    The multi-array general data structure must be pointed to by  multi_desc
    The top level packet descriptor of the general data structure to append
    must be poined to by  pack_desc  .
    The corresponding data for the general data structure must be pointed to
    by  packet  .
    If the multi-array data structure previously had only one general data
    structure, then the name pointed to by  existing_arrayname  will become
    the arrayname for that data structure.
    The name of the appended data structure must be pointed to by
    append_arrayname  .
    The routine returns TRUE on success, else it returns FALSE.
*/
multi_array *multi_desc;
packet_desc *pack_desc;
char *packet;
char *existing_arrayname;
char *append_arrayname;
{
    char **array_names;
    char **data;
    packet_desc **headers;
    static char function_name[] = "ds_append_gen_struct";

    if ( ( array_names = (char **)
	  m_alloc ( sizeof *array_names * ( (*multi_desc).num_arrays + 1 ) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of name pointers");
	return (FALSE);
    }
    if ( ( data = (char **)
	  m_alloc ( sizeof *data * ( (*multi_desc).num_arrays + 1 ) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of data pointers");
	m_free ( (char *) array_names );
	return (FALSE);
    }
    if ( ( headers = (packet_desc **)
	  m_alloc ( sizeof *headers * ( (*multi_desc).num_arrays + 1 ) ) )
	== NULL )
    {
	m_error_notify (function_name, "array of packet descriptor pointers");
	m_free ( (char *) array_names );
	m_free ( (char *) data );
	return (FALSE);
    }
    if ( (*multi_desc).num_arrays < 2 )
    {
	/*  Original had only one array  */
	if ( ( array_names[0] = m_alloc (strlen (existing_arrayname) + 1) )
	    == NULL )
	{
	    m_error_notify (function_name, "existing arrayname");
	    m_free ( (char *) array_names );
	    m_free ( (char *) data );
	    m_free ( (char *) headers );
	    return (FALSE);
	}
	(void) strcpy (array_names[0], existing_arrayname);
	if ( ( array_names[(*multi_desc).num_arrays] =
	      m_alloc (strlen (append_arrayname) + 1) ) == NULL )
	{
	    m_error_notify (function_name, "existing arrayname");
	    m_free (array_names[0]);
	    m_free ( (char *) array_names );
	    m_free ( (char *) data );
	    m_free ( (char *) headers );
	    return (FALSE);
	}
	(void) strcpy (array_names[(*multi_desc).num_arrays],append_arrayname);
    }
    else
    {
	/*   Original had more than one array  */
	m_copy ( (char *) array_names, (char *) (*multi_desc).array_names,
		sizeof *array_names * (*multi_desc).num_arrays );
	if ( ( array_names[(*multi_desc).num_arrays] =
	      m_alloc (strlen (append_arrayname) + 1) ) == NULL )
	{
	    m_error_notify (function_name, "existing arrayname");
	    m_free ( (char *) array_names );
	    m_free ( (char *) data );
	    m_free ( (char *) headers );
	    return (FALSE);
	}
	(void) strcpy (array_names[(*multi_desc).num_arrays],append_arrayname);
    }
    m_copy ( (char *) data, (char *) (*multi_desc).data,
	    sizeof *data * (*multi_desc).num_arrays );
    m_copy ( (char *) headers, (char *) (*multi_desc).headers,
	    sizeof *headers * (*multi_desc).num_arrays );
    data[(*multi_desc).num_arrays] = packet;
    headers[(*multi_desc).num_arrays] = pack_desc;
    if ( (*multi_desc).array_names != NULL )
    {
	m_free ( (char *) (*multi_desc).array_names );
    }
    (*multi_desc).array_names = array_names;
    m_free ( (char *) (*multi_desc).data );
    (*multi_desc).data = data;
    m_free ( (char *) (*multi_desc).headers );
    (*multi_desc).headers = headers;
    ++(*multi_desc).num_arrays;
    return (TRUE);
}   /*  End Function ds_append_gen_struct  */


/*  Private functions follow  */

static void compute_offsets (offsets, num, offset, stride)
/*  This routine will compute a number of address offsets and will write these
    offsets to an array. The address offsets will be monotonically increasing.
    The address offsets are written to the storage pointed to by  offsets  .
    The number of offsets to compute must be given by  num  .
    The initial offset value must be given by  offset  .
    The difference between adjacent offset values must be given by  stride  .
    The routine returns nothing.
*/
unsigned int *offsets;
unsigned int num;
unsigned int offset;
unsigned int stride;
{
    unsigned int count;

    for (count = 0; count < num; ++count, offset += stride)
    {
	offsets[count] = offset;
    }
}   /*  End Function compute_offsets  */
