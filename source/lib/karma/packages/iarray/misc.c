/*LINTLIBRARY*/
/*  misc.c

    This code provides miscellaneous Intelligent Array routines.

    Copyright (C) 1996  Richard Gooch

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


    Written by      Richard Gooch   28-FEB-1996

    Updated by      Richard Gooch   1-APR-1996: Moved remaing functions to new
  documentation style.

    Updated by      Richard Gooch   16-JUN-1996: Created
  <iarray_get_data_scaling> routine.

    Updated by      Richard Gooch   19-JUN-1996: Created <iarray_format_value>
  routine.

    Updated by      Richard Gooch   31-JUL-1996: Created
  <iarray_create_from_template> routine.

    Updated by      Richard Gooch   2-AUG-1996: Created <iarray_get_coordinate>
  routine.

    Updated by      Richard Gooch   5-AUG-1996: Created <iarray_set_dim_name>
  routine.

    Updated by      Richard Gooch   5-AUG-1996: Created
  <iarray_append_history_string> routine.

    Updated by      Richard Gooch   8-AUG-1996: Created
  <iarray_copy_named_element> routine.

    Updated by      Richard Gooch   9-AUG-1996: Added <fail_if_not_found>
  parameter to <iarray_copy_named_element> routine.

    Updated by      Richard Gooch   15-AUG-1996: Created <iarray_get_fits_axis>
  routine.

    Updated by      Richard Gooch   29-SEP-1996: Made use of <ds_get_fits_axis>
  routine.

    Updated by      Richard Gooch   17-OCT-1996: "Cosmetic" name change of
  <<template>> parameter in <iarray_create_from_template> because of stupid C++
  theft from namespace.

    Last updated by Richard Gooch   2-NOV-1996: Made use of <ds_format_value>.


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>


#define VERIFY_IARRAY(array) if (array == NULL) \
{fprintf (stderr, "NULL iarray passed\n"); a_prog_bug (function_name); }
#ifdef dummy
if (array->magic_number != MAGIC_NUMBER) \
{fprintf (stderr, "Invalid iarray\n"); a_prog_bug (function_name); }
#endif


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
unsigned int iarray_dim_index (iarray array, CONST char *name)
/*  [SUMMARY] This routine will find the index of a named dimension.
    <array> The array.
    <name> The name of the dimension.
    [RETURNS] The dimension index if found, else the number of dimension in the
    array is returned.
*/
{
    unsigned int count, num_dim;
    static char function_name[] = "iarray_dim_index";

    VERIFY_IARRAY (array);
    num_dim = iarray_num_dim (array);
    for (count = 0; count < num_dim; ++count)
    {
	if (strcmp (iarray_dim_name (array, count), name) == 0) return (count);
    }
    return (num_dim);
}   /*  End Function iarray_dim_index  */

/*PUBLIC_FUNCTION*/
flag iarray_get_data_scaling (iarray array, double *scale, double *offset)
/*  [SUMMARY] Get the scale and offset for data in an Intelligent Array.
    [PURPOSE] This routine will determine the scale and offset for data in an
    Intelligent Array. This may be important when a floating-point array has
    been converted to an integer array to save space. Scaling information
    should be attached to the array so that the original data values may be
    reconstructed (aside from quantisation effects). The following expression
    may be used to convert scaled values to real values:
    (output = input * scale + offset). The scaling and offset values should
    previously have been attached to the Intelligent Array using the
    [<iarray_set_data_scaling>] routine.
    <array> The array.
    <scale> The scaling value will be written here. The name of the scaling
    value is constructed by appending "__SCALE" to the array value name (see
    [<iarray_get_value_name>]). If no scaling value is found, 1.0 is written
    here.
    <offset> The offset value will be written here. The name of the offset
    value is constructed by appending "__OFFSET" to the array value name (see
    [<iarray_get_value_name>]). If no offset value is found, 0.0 is written
    here.
    [RETURNS] TRUE if either the scaling or offset value were found, else FALSE
*/
{
    static char function_name[] = "iarray_get_data_scaling";

    VERIFY_IARRAY (array);
    return ds_get_data_scaling (iarray_value_name (array),
				array->top_pack_desc, *array->top_packet,
				scale, offset);
}   /*  End Function iarray_get_data_scaling  */

/*PUBLIC_FUNCTION*/
flag iarray_set_data_scaling (iarray array, double scale, double offset)
/*  [SUMMARY] Set the scale and offset for data in an Intelligent Array.
    [PURPOSE] This routine will set the scale and offset for data in an
    Intelligent Array. This may be important when a floating-point array has
    been converted to an integer array to save space. The scaling information
    will be attached to the array so that the original data values may be
    reconstructed (aside from quantisation effects). The following expression
    may be used to convert scaled values to real values:
    (output = input * scale + offset).
    <array> The array.
    <scale> The scaling value. The name of the scaling value is constructed by
    appending "__SCALE" to the array value name (see [<iarray_get_value_name>])
    <offset> The offset value. The name of the offset value is constructed by
    appending "__OFFSET" to the array value name(see [<iarray_get_value_name>])
    [RETURNS] TRUE if the scaling information is different from what was
    already attached to the array, else FALSE.
*/
{
    double sc, off;
    double value[2];
    char txt[STRING_LENGTH];
    static char function_name[] = "iarray_set_data_scaling";

    VERIFY_IARRAY (array);
    iarray_get_data_scaling (array, &sc, &off);
    if ( (scale == sc) && (offset == off) ) return (FALSE);
    sprintf ( txt, "%s__SCALE", iarray_value_name (array) );
    value[0] = scale;
    value[1] = 0.0;
    if ( !iarray_put_named_value (array, txt, K_DOUBLE, value) )
    {
	fprintf (stderr, "Error attaching \"%s\" element\n", txt);
	return (TRUE);
    }
    sprintf ( txt, "%s__OFFSET", iarray_value_name (array) );
    value[0] = offset;
    if ( !iarray_put_named_value (array, txt, K_DOUBLE, value) )
    {
	fprintf (stderr, "Error attaching \"%s\" element\n", txt);
	return (TRUE);
    }
    return (TRUE);
}   /*  End Function iarray_set_data_scaling  */

/*EXPERIMENTAL_FUNCTION*/
void iarray_format_value (iarray array, char string[STRING_LENGTH],
			  double value, double scale, double offset)
/*  [SUMMARY] Format a data value into a string.
    <array> The Intelligent Array the value is associated with.
    <string> The string to write to.
    <value> The value to format.
    <scale> The scale value to apply to the data. If this is TOOBIG the routine
    uses the scale and offset attached to the array. See
    [<iarray_set_data_scaling>] for details.
    <offset> The offset to apply after scaling the data.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "iarray_format_value";

    VERIFY_IARRAY (array);
    ds_format_value (string, value, iarray_value_name (array), scale, offset,
		     array->top_pack_desc, *array->top_packet);
}   /*  End Function iarray_format_value  */

/*PUBLIC_FUNCTION*/
iarray iarray_create_from_template (iarray template_arr,unsigned int elem_type,
				    flag copy_world_coords, flag copy_names,
				    flag copy_aux_info)
/*  [SUMMARY] Create an Intelligent Array from a template array.
    [PURPOSE] This routine will create an Intelligent Array of a specified type
    using an existing Intelligent Array as the template for the size,
    dimensionality and other attributes.
    <template_arr> The template Intelligent Array.
    <elem_type> The element type for the new Intelligent Array.
    <copy_world_coords> If TRUE, the world co-ordinates for each dimension in
    the template array are copied, otherwise the defaults are used.
    <copy_names> If TRUE, the dimension names and the value name of the
    template array are copied, else the defaults are used.
    <copy_aux_info> If TRUE, auxilary information (attachments) in the template
    array is copied to the new array.
    [RETURNS] An Intelligent Array on success, else FALSE.
*/
{
    iarray new;
    unsigned int count, num_dim;
    double first_coord, last_coord;
    char *elem_name;
    char **dim_names;
    unsigned long *dim_lengths;
    static char function_name[] = "iarray_create_from_template";

    VERIFY_IARRAY (template_arr);
    num_dim = iarray_num_dim (template_arr);
    if ( ( dim_lengths = (unsigned long *)
	   m_alloc (num_dim * sizeof *dim_lengths) ) == NULL )
    {
	m_abort (function_name, "dimension lengths");
    }
    for (count = 0; count < num_dim; ++count)
    {
	dim_lengths[count] = iarray_dim_length (template_arr, count);
    }
    if (copy_names)
    {
	elem_name = iarray_value_name (template_arr);
	if ( ( dim_names = (char **) m_alloc (num_dim * sizeof *dim_names) )
	     == NULL )
	{
	    m_abort (function_name, "dimension names array");
	}
	for (count = 0; count < num_dim; ++count)
	{
	    dim_names[count] = (char *) iarray_dim_name (template_arr, count);
	}
    }
    else
    {
	elem_name = NULL;
	dim_names = NULL;
    }
    new = iarray_create (elem_type, num_dim, (CONST char **) dim_names,
			 dim_lengths, elem_name,
			 copy_aux_info ? template_arr : NULL);
    m_free ( (char *) dim_lengths );
    if (dim_names != NULL) m_free ( (char *) dim_names );
    if (new == NULL) return (NULL);
    if (copy_world_coords)
    {
	for (count = 0; count < num_dim; ++count)
	{
	    iarray_get_world_coords (template_arr, count,
				     &first_coord, &last_coord);
	    iarray_set_world_coords (new, count, first_coord, last_coord);
	}
    }
    return (new);
}   /*  End Function iarray_create_from_template  */

/*PUBLIC_FUNCTION*/
double iarray_get_coordinate (iarray array, unsigned int dim_index,
			      double coord_index)
/*  [SUMMARY] Get a co-ordinate along a dimension.
    <array> The Intelligent Array.
    <dim_index> The dimension index.
    <coord_index> The co-ordinate index.
    [RETURNS] The co-ordinate on success, else TOOBIG.
*/
{
    flag found;
    uaddr count;
    unsigned int orig_dim_index;
    dim_desc *dim;
    static char function_name[] = "iarray_get_coordinate";

    VERIFY_IARRAY (array);
    dim = iarray_get_dim_desc (array, dim_index);
    if (array->offsets == array->arr_desc->offsets)
    {
	/*  Not an alias array  */
	return ( ds_get_coordinate (dim, coord_index) );
    }
    /*  It is an alias array: bugger. Search for offset along dimension
	descriptor which equals first offset along iarray dimension  */
    orig_dim_index = array->orig_dim_indices[dim_index];
    count = 0;
    found = FALSE;
    while ( (count < dim->length) && !found )
    {
	if (array->offsets[dim_index][0] ==
	    array->arr_desc->offsets[orig_dim_index][count]) found = TRUE;
	else ++count;
    }
    if (!found)
    {
	fprintf (stderr, "Could not find offset index!\n");
	a_prog_bug (function_name);
    }
    return ( ds_get_coordinate (dim, coord_index + count) );
}   /*  End Function iarray_get_coordinate  */

/*PUBLIC_FUNCTION*/
void iarray_set_dim_name (iarray array, unsigned int index, CONST char *name,
			  flag new_alloc)
/*  [SUMMARY] Change the name of a dimension of an Intelligent Array.
    <array> The Intelligent Array.
    <index> The index of the dimension.
    <name> The new dimension name.
    <new_alloc> If TRUE, the routine will allocate a new copy of the dimension
    name, else it will copy the pointer (in which case the name must never be
    externally deallocated or changed).
    [RETURNS] Nothing.
*/
{
    char *ptr;
    dim_desc *dim;
    static char function_name[] = "iarray_set_dim_name";

    VERIFY_IARRAY (array);
    dim = iarray_get_dim_desc (array, index);
    if (!new_alloc)
    {
	m_free (dim->name);
	dim->name = (char *) name;
	return;
    }
    if ( ( ptr = st_dup (name) ) == NULL )
    {
	m_abort (function_name, "dimension name");
    }
    m_free (dim->name);
    dim->name = ptr;
}   /*  End Function iarray_set_dim_name  */

/*PUBLIC_FUNCTION*/
flag iarray_append_history_string (iarray array, CONST char *string,
				   flag new_alloc)
/*  [SUMMARY] Add a history string to an Intelligent Array.
    <array> The Intelligent Array.
    <string> The history string to add.
    <new_alloc> If TRUE, the routine will allocate a new copy of the history
    string, else it will copy the pointer (in which case the string must never
    be externally deallocated or changed).
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "iarray_append_history_string";

    VERIFY_IARRAY (array);
    return ( ds_history_append_string (array->multi_desc, string, new_alloc) );
}   /*  End Function iarray_append_history_string  */

/*PUBLIC_FUNCTION*/
flag iarray_copy_named_element (iarray out, iarray in, CONST char *name,
				flag fail_if_not_found,
				flag fail_on_duplicate, flag replace)
/*  [SUMMARY] Copy a named element from one Intelligent Array to another.
    <out> The output Intelligent Array.
    <in> The input Intelligent Array.
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
    return ( ds_copy_unique_named_element (out->top_pack_desc, out->top_packet,
					   in->top_pack_desc, *in->top_packet,
					   name, fail_if_not_found,
					   fail_on_duplicate, replace) );
}   /*  End Function iarray_copy_named_element  */

/*PUBLIC_FUNCTION*/
unsigned int iarray_get_fits_axis (iarray array, unsigned int index)
/*  [SUMMARY] Get the FITS axis number of a dimension.
    <array> The Intelligent Array.
    <index> The index of the dimension.
    [RETURNS] The FITS axis number on success, else 0.
*/
{
    static char function_name[] = "iarray_get_fits_axis";

    VERIFY_IARRAY (array);
    return ds_get_fits_axis ( array->top_pack_desc, *array->top_packet,
			      iarray_dim_name (array, index) );
}   /*  End Function iarray_get_fits_axis  */
