/*  chlen.c

    Source file for  chlen  (resampling module).

    Copyright (C) 1993  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This Karma module will change the length of a dimension. The dimension is
    either zero padded, truncated or resampled.


    Written by      Richard Gooch   10-NOV-1992

    Updated by      Richard Gooch   10-NOV-1992

    Updated by      Richard Gooch   3-JAN-1993: Took acccount of change to
  ds_alloc_data

    Updated by      Richard Gooch   28-MAR-1993: Took account of changes to
  dsproc_object  function  and explicitly disabled tiling in  generate_desc  .

    Updated by      Richard Gooch   16-NOV-1993: Changed over to  panel_
  package for command line user interface and moved  main  into this file.

    Last updated by Richard Gooch   23-NOV-1993: Fixed bug in call to
  panel_add_item  for  array_names  parameter.


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_m.h>
#include <karma_a.h>

#define VERSION "1.1"

EXTERN_FUNCTION (flag chlen, (char *command, FILE *fp) );

flag pre_process ();
flag process_array ();
flag post_process ();
packet_desc *generate_desc ();
flag process_occurrence ();
flag resample_dimension ();

static int save_unproc = TRUE;

#define NUMARRAYS 100
static char *array_names[NUMARRAYS];
static int num_arrays = 0;
static double new_min = 0.0;
static double new_max = 1.0;
static int num_coordinates = 1;

#define OPTION_MIN_MAX 0
#define OPTION_ZERO_PAD 1
#define OPTION_TRUNCATE 2
#define OPTION_RESAMPLE 3
#define NUM_OPTIONS 4
static char *option_alternatives[NUM_OPTIONS] =
{
    "min_max",
    "zero_pad",
    "truncate",
    "resample"
};
static int option = OPTION_MIN_MAX;

#define RESAMPLE_OPTION_DATA_COPY 0
#define RESAMPLE_OPTION_LINEAR_INTERPOLATION 1
#define NUM_RESAMPLE_OPTIONS 2
static char *resample_option_alternatives[NUM_RESAMPLE_OPTIONS] =
{
    "data_copy",
    "linear_interpolation"
};
static int resample_option = RESAMPLE_OPTION_DATA_COPY;

static char *dim_name = NULL;


main (argc, argv)
int argc;       /*  Count of parameters on command line */
char **argv;    /*  List of command line parameters     */
{
    KControlPanel panel;
    extern char *data_type_names[NUMTYPES];
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "array_names", "", K_VSTRING, array_names,
		    PIA_ARRAY_LENGTH, &num_arrays, PIA_ARRAY_MIN_LENGTH, 0,
		    PIA_ARRAY_MAX_LENGTH, NUMARRAYS,
		    PIA_END);
    panel_add_item (panel, "save_unproc_data", "", PIT_FLAG, &save_unproc,
		    PIA_END);
    panel_add_item (panel, "new_max", "co-ordinate value", K_DOUBLE, &new_max,
		    PIA_END);
    panel_add_item (panel, "new_min", "co-ordinate value", K_DOUBLE, &new_min,
		    PIA_END);
    panel_add_item (panel, "num_coordinates", "co-ordinates", K_INT,
		    &num_coordinates,
		    PIA_END);
    panel_add_item (panel, "option", "", PIT_CHOICE_INDEX, &option,
		    PIA_NUM_CHOICE_STRINGS, NUM_OPTIONS,
		    PIA_CHOICE_STRINGS, option_alternatives,
		    PIA_END);
    panel_add_item (panel, "resample_option", "", PIT_CHOICE_INDEX,
		    &resample_option,
		    PIA_NUM_CHOICE_STRINGS, NUM_RESAMPLE_OPTIONS,
		    PIA_CHOICE_STRINGS, resample_option_alternatives,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "chlen", VERSION, chlen, 1, 0, TRUE);
    return (RV_OK);
}   /*  End Function main   */

flag chlen (p, fp)
char *p;
FILE *fp;
{
    char *arrayfile;
    extern int num_coordinates;
    extern int save_unproc;
    extern int num_arrays;
    extern char *dim_name;
    extern char *array_names[NUMARRAYS];
    extern double new_min;
    extern double new_max;
    static char function_name[] = "chlen";

    if (num_coordinates < 1)
    {
	(void) fprintf (stderr, "num_coordinates must be greater than zero\n");
        return (TRUE);
    }
    if (new_max <= new_min)
    {
	(void) fprintf (stderr, "new_max must be greater than new_min\n");
        return (TRUE);
    }
    for ( ; p; p = ex_word_skip (p) )
    {
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (stderr, "Error extracting arrayfile name\n");
	    return (TRUE);
	}
	if ( ( dim_name = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Must specify dimension name to change length of\n");
	    m_free (arrayfile);
	    return (TRUE);
	}
	dsproc_object (arrayfile, array_names, num_arrays, (flag) save_unproc,
		       pre_process, process_array, post_process,
		       K_CH_MAP_LOCAL);
	m_free (arrayfile);
	m_free (dim_name);
    }
    return (TRUE);
}   /*  End Function chlen    */

flag pre_process (multi_desc)
/*  This routine will pre process the multi array general data structure
    pointed to by  multi_desc  .
    The routine returns TRUE if processing is to continue, else it
    returns FALSE.
*/
multi_array *multi_desc;
{
    char txt[STRING_LENGTH];
    extern char *dim_name;
    static char function_name[] = "pre_process";

    if (ds_f_array_name (multi_desc, dim_name, (char **) NULL,
			 (unsigned int *) NULL)
        != IDENT_NOT_FOUND)
    {
	(void) sprintf (txt, "array name must not be: \"%s\"", dim_name);
        a_func_abort (function_name, txt);
        return (FALSE);
    }
    return (TRUE);
}   /*  End Function pre_process    */

flag process_array (inp_desc, inp_data, out_desc, out_data)
/*  This routine will process a general data structure, changing the length of
    each occurrence of the dimension name in the external string  dim_name  .
    The input data descriptor and data must be pointed to by  inp_desc  and
    inp_data  ,respectively.
    The output data descriptor pointer will be written to the storage pointed
    to by  out_desc  and the output data pointer will be written to the storage
    pointed to by  out_data  .
    The routine returns TRUE on successful processing, else it displays an
    error message and returns FALSE.
*/
packet_desc *inp_desc;
char *inp_data;
packet_desc **out_desc;
char **out_data;
{
    extern char *dim_name;
    static char function_name[] = "process_array";

    if ( (inp_desc == NULL) || (out_desc == NULL) )
    {
	(void) fprintf (stderr, "\nNULL descriptor pointer(s)");
        a_prog_bug (function_name);
    }
    if ( (inp_data == NULL) || (out_data == NULL) )
    {
	(void) fprintf (stderr, "\nNULL data pointer(s)");
	a_prog_bug (function_name);
    }
    if (dim_name == NULL)
    {
	(void) fprintf (stderr,
			"\nNULL external variable dimension name pointer");
	a_prog_bug (function_name);
    }
    /*  Generate output descriptor  */
    if ( ( *out_desc = generate_desc (inp_desc) ) == NULL )
    {
	a_func_abort (function_name, "no output descriptor");
        return (FALSE);
    }
    if ( ( *out_data = ds_alloc_data (*out_desc, TRUE, TRUE) ) == NULL )
    {
	a_func_abort (function_name, "could not allocate output data");
        return (FALSE);
    }
    ds_copy_data (inp_desc, inp_data, *out_desc, *out_data);
    if (ds_traverse_and_process (inp_desc, inp_data, *out_desc, *out_data,
				 FALSE, process_occurrence) == FALSE)
    {
	a_func_abort (function_name, "error traversing data structures");
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function process_array  */

packet_desc *generate_desc (inp_desc)
/*  This routine will generate a general data structure descriptor for
    the output of a dimension length changing program.
    The input descriptor, which is partly copied, must be pointed to by
    inp_desc  .
    The input descriptor must contain (at any depth) a dimension of name
    pointed to by the external variable  dim_name  .
    The length of this dimension will be altered.
    The routine returns a pointer to the output descriptor on success, else
    it returns NULL.
*/
packet_desc *inp_desc;
{
    unsigned int new_length;
    unsigned int dim_num;
    unsigned int coord_count;
    unsigned int pad_coords;
    double min;
    double max;
    double inp_dim_spacing;
    packet_desc *return_value;
    array_desc *arr_desc;
    dim_desc *inp_dim;
    dim_desc *out_dim;
    extern unsigned int dim_index;
    extern int option;
    extern unsigned int inp_start_coord;
    extern unsigned int inp_stop_coord;
    extern unsigned int out_start_coord;
    extern double new_min;
    extern double new_max;
    extern char *dim_name;
    static char function_name[] = "generate_desc";

    /*  Do tests on input descriptor    */
    switch ( ds_f_name_in_packet (inp_desc, dim_name,
			       (char **) &arr_desc, &dim_index) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr, "\nDimension: \"%s\" not found", dim_name);
	return (NULL);
	break;
      case IDENT_ELEMENT:
	(void) fprintf (stderr,
			"\nItem: \"%s\" must be a dimension name, not an element",
			dim_name);
	return (NULL);
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr, "\nItem \"%s\" found more than once",
			dim_name);
	return (NULL);
	break;
      case IDENT_DIMENSION:
	break;
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"\nBad return value from function: ds_f_name_in_packet");
	a_prog_bug (function_name);
	break;
    }
    if ( (*arr_desc).num_levels > 0 )
    {
	(void) fprintf (stderr, "Tiling not supported yet\n");
	return (NULL);
    }
    if (ds_packet_all_data ( (*arr_desc).packet ) == FALSE)
    {
	(void) fprintf (stderr, "\nArray must have only atomic elements");
	return (NULL);
    }
    inp_dim = (*arr_desc).dimensions[dim_index];
    if ( (*inp_dim).coordinates == NULL )
    {
	/*  Compute dimension spacing  */
	inp_dim_spacing = ( ( (*inp_dim).maximum - (*inp_dim).minimum ) /
			   (double) ( (*inp_dim).length - 1 ) );
    }
    /*  Compute output dimension info  */
    switch (option)
    {
      case OPTION_MIN_MAX:
	/*  Compute new minimum and maximum  */
	if ( ( (new_min < (*inp_dim).minimum) ||
	      (new_max > (*inp_dim).maximum) ) &&
	    ( (*inp_dim).coordinates != NULL ) )
	{
	    /*  Dimension extended and co-ordinates not evenly spaced  */
	    (void) fprintf (stderr,
			    "\nCannot extend irregularly spaced dimension");
	    return (NULL);
	}
	/*  Compute new minimum  */
	if (new_min < (*inp_dim).minimum)
	{
	    /*  Outside old dimension bounds: extrapolate  */
	    inp_start_coord = 0;
	    out_start_coord = floor ( ( (*inp_dim).minimum - new_min ) /
				     inp_dim_spacing );
	    min = (*inp_dim).minimum;
	    min -= (double) out_start_coord * inp_dim_spacing;
	}
	else
	{
	    /*  Inside old dimension bounds  */
	    inp_start_coord = ds_get_coord_num (inp_dim, new_min,
						SEARCH_BIAS_UPPER);
	    out_start_coord = 0;
	    min = ds_get_coordinate (inp_dim, inp_start_coord);
	}
	/*  Compute new maximum  */
	if (new_max > (*inp_dim).maximum)
	{
	    /*  Outside old dimension bounds: extrapolate  */
	    pad_coords = floor ( (new_max - (*inp_dim).maximum) /
				inp_dim_spacing );
	    inp_stop_coord = (*inp_dim).length;
	    max = (*inp_dim).maximum;
	    max += (double) pad_coords * inp_dim_spacing;
	}
	else
	{
	    /*  Inside old dimension bounds  */
	    pad_coords = 0;
	    inp_stop_coord = ds_get_coord_num (inp_dim, new_max,
					       SEARCH_BIAS_LOWER);
	    max = ds_get_coordinate (inp_dim, inp_stop_coord - 1);
	}
	new_length = out_start_coord + pad_coords;
	new_length += inp_stop_coord - inp_start_coord;
	break;
      case OPTION_ZERO_PAD:
	inp_start_coord = 0;
	out_start_coord = 0;
	inp_stop_coord = (*inp_dim).length;
	min = (*inp_dim).minimum;
	max = (*inp_dim).maximum + (double) num_coordinates * inp_dim_spacing;
	new_length = (*inp_dim).length + (unsigned int) num_coordinates;
	break;
      case OPTION_TRUNCATE:
	inp_start_coord = 0;
	out_start_coord = 0;
	inp_stop_coord = (*inp_dim).length - (unsigned int) num_coordinates;
	min = (*inp_dim).minimum;
	max = ds_get_coordinate (inp_dim, inp_stop_coord - 1);
	new_length = (*inp_dim).length - (unsigned int) num_coordinates;
	break;
      case OPTION_RESAMPLE:
	inp_start_coord = 0;
	out_start_coord = 0;
	inp_stop_coord = (*inp_dim).length;
	min = (*inp_dim).minimum;
	max = (*inp_dim).maximum;
	new_length = (unsigned int) num_coordinates;
	break;
      default:
	(void) fprintf (stderr, "\nBad option value: %d", option);
	a_prog_bug (function_name);
    }

    /*  Copy data structure descriptor  */
    if ( ( return_value = ds_copy_desc_until (inp_desc, (char *) NULL) )
	== NULL )
    {
	a_func_abort (function_name, "error copying descriptors");
	return (NULL);
    }
    /*  Do tests on output descriptor    */
    switch ( ds_f_name_in_packet (return_value, dim_name,
				  (char **) &arr_desc, &dim_num) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr,
			"\nOutput Dimension: \"%s\" not found", dim_name);
	a_prog_bug (function_name);
	break;
      case IDENT_ELEMENT:
	(void) fprintf (stderr,
			"\nOutput item: \"%s\" must be a dimension name, not an element",
			dim_name);
	a_prog_bug (function_name);
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"\nOutput item \"%s\" found more than once", dim_name);
	a_prog_bug (function_name);
	break;
      case IDENT_DIMENSION:
	break;
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"\nBad return value from function: ds_f_name_in_packet");
	a_prog_bug (function_name);
	break;
    }
    if (dim_num != dim_index)
    {
	(void) fprintf (stderr,
			"\nDimension index has changed from: %u to: %u",
			dim_index, dim_num);
	a_prog_bug (function_name);
    }
    /*  Change length of output dimension  */
    out_dim = (*arr_desc).dimensions[dim_num];
    (*out_dim).length = new_length;
    (*arr_desc).lengths[dim_num] = (*out_dim).length;
    /*  Change minimum and maximum co-ordinates of output dimension  */
    (*out_dim).minimum = min;
    (*out_dim).maximum = max;
    if (option == OPTION_RESAMPLE)
    {
	/*  Kill any output dimension co-ordinates: force a regular grid  */
	if ( (*out_dim).coordinates != NULL )
	{
	    m_free ( (char *) (*out_dim).coordinates );
	    (*out_dim).coordinates = NULL;
	}
    }
    if ( (*out_dim).coordinates == NULL )
    {
	/*  No more work needs to be done  */
	return (return_value);
    }
    /*  Create new co-ordinate array  */
    m_free ( (char *) (*out_dim).coordinates );
    (*out_dim).coordinates = NULL;
    if ( ( (*out_dim).coordinates = (double *) m_alloc
	  (sizeof *(*out_dim).coordinates * new_length) ) == NULL )
    {
	m_error_notify (function_name, "new co-ordinate array");
	ds_dealloc_packet (return_value, (char *) NULL);
	return (NULL);
    }
    for (coord_count = inp_start_coord; coord_count < inp_stop_coord;
	 ++coord_count)
    {
	(*out_dim).coordinates[coord_count - inp_start_coord] =
	(*inp_dim).coordinates[coord_count];
    }
    return (return_value);
}   /*  End Function generate_desc  */

flag post_process (inp_array, out_array)
/*  This routine will perform post processing on the composite array pointed
    to by  out_array  .
    The routine will copy over history information from the input composite
    array pointed to by  inp_array  .
    The routine returns TRUE if the array is to be saved, else it
    returns FALSE.
*/
struct composite_array *inp_array;
struct composite_array *out_array;
{
    char txt[STRING_LENGTH];
    extern char *dim_name;
    static char function_name[] = "post_process";

#ifdef dummy
    ez_description (out_array, vble);
    (void) sprintf (txt, "Changed length of dimension: \"%s\"", dim_name);
    composite_descr (txt, out_array);
    copy_composite_descr (inp_array, out_array);
    return (TRUE);
#endif
}   /*  End Function post_process   */

flag process_occurrence (inp_desc, inp_type, inp_data,
			 out_desc, out_type, out_data)
/*  This routine will process an occurrence of a difference in the input and
    output data structures.
    The input data structure descriptor is pointed to by  inp_desc  ,the input
    descriptor type is in  inp_type  and the input data is pointed to by
    inp_data  .
    The output data structure descriptor is pointed to by  out_desc  ,the
    output descriptor type in in  out_type  and the output data is pointed to
    by  out_data  .
    The routine is meant to be called from the  traverse_and_process  family of
    routines.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *inp_desc;
unsigned int inp_type;
char *inp_data;
char *out_desc;
unsigned int out_type;
char *out_data;
{
    unsigned int num_dim;
    unsigned int dim_count;
    unsigned int iter_above_num;
    unsigned int iter_above_count;
    unsigned int iter_below_num;
    unsigned int iter_below_count;
    unsigned int stride;
    unsigned int pack_size;
    unsigned int copy_bytes;
    char *inp_ptr;
    char *out_ptr;
    array_desc *arr_desc_inp;
    array_desc *arr_desc_out;
    dim_desc *inp_dim;
    dim_desc *out_dim;
    extern int option;
    extern int resample_option;
    extern unsigned int dim_index;
    extern unsigned int inp_start_coord;
    extern unsigned int inp_stop_coord;
    extern unsigned int out_start_coord;
    extern char *data_type_names[NUMTYPES];
    static char function_name[] = "process_occurrence";

    if ( (inp_desc == NULL) || (inp_data == NULL)
	|| (out_desc == NULL) || (out_data == NULL) )
    {
	(void) fprintf (stderr, "\nNULL descriptor(s) passed");
	a_prog_bug (function_name);
    }
    if ( (inp_type != K_ARRAY) || (out_type != K_ARRAY) )
    {
	/*  Descriptors are not array descriptors  */
	(void) fprintf ( stderr,
			"\nDescriptors: input: %s output: %s are not both K_ARRAY",
			data_type_names[inp_type], data_type_names[out_type] );
	a_prog_bug (function_name);
    }
    arr_desc_inp = (array_desc *) inp_desc;
    arr_desc_out = (array_desc *) out_desc;
    num_dim = (*arr_desc_inp).num_dimensions;
    /*  Do a quick check on the descriptors  */
    if (num_dim != (*arr_desc_out).num_dimensions)
    {
	(void) fprintf (stderr,
			"\nArray descriptors have different number of dim.: %u %u",
			(*arr_desc_inp).num_dimensions,
			(*arr_desc_out).num_dimensions);
	a_prog_bug (function_name);
    }
    inp_dim = (*arr_desc_inp).dimensions[dim_index];
    out_dim = (*arr_desc_out).dimensions[dim_index];
    /*  Determine stride of dimension co-ordinates  */
    pack_size = ds_get_packet_size ( (*arr_desc_inp).packet );
    /*  Determine inner loop iteration count  */
    iter_below_num = 1;
    for (dim_count = dim_index + 1; dim_count < num_dim; ++dim_count)
    {
	iter_below_num *= (* (*arr_desc_inp).dimensions[dim_count] ).length;
    }
    stride = pack_size * iter_below_num;
    /*  Determine outer loop iteration count  */
    iter_above_num = 1;
    for (dim_count = 0; dim_count < dim_index; ++dim_count)
    {
	iter_above_num *= (* (*arr_desc_inp).dimensions[dim_count] ).length;
    }
    if (option != OPTION_RESAMPLE)
    {
	/*  Compute copy size  */
	copy_bytes = stride * (inp_stop_coord - inp_start_coord);
	/*  First offset data pointers for starting co-ordinates  */
	inp_data += stride * inp_start_coord;
	out_data += stride * out_start_coord;
    }
    /*  Iterate above dimension to be changed  */
    for (iter_above_count = 0; iter_above_count < iter_above_num;
	 ++iter_above_count,
	 inp_data += stride * (*inp_dim).length,
	 out_data += stride * (*out_dim).length)
    {
	if (option == OPTION_RESAMPLE)
	{
	    /*  Iterate below dimension to be resampled  */
	    inp_ptr = inp_data;
	    out_ptr = out_data;
	    for (iter_below_count = 0; iter_below_count < iter_below_num;
		 ++iter_below_count,
		 inp_ptr += pack_size, out_ptr += pack_size)
	    {
		if (resample_dimension (inp_ptr, stride, inp_dim,
					out_ptr, stride, out_dim,
					(*arr_desc_inp).packet,
					(unsigned int) resample_option)
		    != TRUE)
		{
		    return (FALSE);
		}
	    }
	}
	else
	{
	    m_copy (out_data, inp_data, copy_bytes);
	}
    }
    return (TRUE);
}   /*  End Function process_occurrence  */

flag resample_dimension (inp_data, inp_stride, inp_dim_desc,
			 out_data, out_stride, out_dim_desc, pack_desc, option)
/*  This routine will resample a dimension of data.
    The input data must be pointed to by  inp_data  .
    The stride of the input data packets must be given by  inp_stride  .
    The input dimension descriptor must be pointed to by  inp_dim_desc  .
    and  inp_dim_desc  ,respectively.
    The output data must be pointed to by  out_data  .
    The stride of the output data packets must be given by  out_stride  .
    The output dimension descriptor must be pointed to by  out_dim_desc  .
    and  out_dim_desc  ,respectively.
    The packet descriptor for the data packets must be given pointed to by
    pack_desc  .
    The resampling option to use must be given by  option  .Legal values are:
        RESAMPLE_OPTION_DATA_COPY    RESAMPLE_OPTION_LINEAR_INTERPOLATION
    The routine returns TRUE on success, else it returns FALSE.
*/
char *inp_data;
unsigned int inp_stride;
dim_desc *inp_dim_desc;
char *out_data;
unsigned int out_stride;
dim_desc *out_dim_desc;
packet_desc *pack_desc;
unsigned int option;
{
    flag complex;
    unsigned int inp_coord_index;
    unsigned int out_coord_count;
    unsigned int elem_count;
    unsigned int elem_type;
    unsigned int elem_size;
    double inp_coord1;
    double inp_coord2;
    double out_coord;
    double factor;
    double inp_value[2];
    double inp_sec_value[2];
    double out_value[2];
    char *inp_ptr;
    char *out_ptr;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "resample_dimension";

    /*  Loop on elements  */
    for (elem_count = 0; elem_count < (*pack_desc).num_elements; ++elem_count)
    {
	elem_type = (*pack_desc).element_types[elem_count];
	elem_size = host_type_sizes[elem_type];
	/*  Loop around output dimension co-ordinates  */
	inp_ptr = inp_data;
	out_ptr = out_data;
	for (out_coord_count = 0; out_coord_count < (*out_dim_desc).length;
	     ++out_coord_count, out_ptr += out_stride)
	{
	    if ( ( out_coord = ds_get_coordinate (out_dim_desc,
						  out_coord_count) )
		>= TOOBIG )
	    {
		(void) fprintf (stderr,
				"\nError getting co-ordinate number: %u",
				out_coord_count);
		return (FALSE);
	    }
	    /*  Sample  */
	    switch (option)
	    {
	      case RESAMPLE_OPTION_DATA_COPY:
		inp_coord_index = ds_get_coord_num (inp_dim_desc, out_coord,
						    SEARCH_BIAS_CLOSEST);
		if (ds_get_element (inp_ptr + inp_coord_index * inp_stride,
				    elem_type, inp_value, &complex) != TRUE)
		{
		    (void) fprintf (stderr, "\nError converting data");
		    return (FALSE);
		}
		out_value[0] = inp_value[0];
		out_value[1] = inp_value[1];
		break;
	      case RESAMPLE_OPTION_LINEAR_INTERPOLATION:
		inp_coord_index = ds_get_coord_num (inp_dim_desc, out_coord,
						    SEARCH_BIAS_LOWER);
		if (ds_get_element (inp_ptr + inp_coord_index * inp_stride,
				    elem_type, inp_value, &complex) != TRUE)
		{
		    (void) fprintf (stderr, "\nError converting data");
		    return (FALSE);
		}
		if (inp_coord_index + 1 < (*inp_dim_desc).length)
		{
		    /*  Not last co-ordinate of input dimension  */
		    if (ds_get_element (inp_ptr + (inp_coord_index + 1) *
					inp_stride,
					elem_type, inp_sec_value, &complex)
			!= TRUE)
		    {
			(void) fprintf (stderr, "\nError converting data");
			return (FALSE);
		    }
		    if ( ( inp_coord1 = ds_get_coordinate (inp_dim_desc,
							   inp_coord_index) )
			>= TOOBIG )
		    {
			(void) fprintf (stderr,
					"\nError getting co-ordinate number: %u",
					inp_coord_index);
			return (FALSE);
		    }
		    if ( ( inp_coord2 = ds_get_coordinate (inp_dim_desc,
							   inp_coord_index
							   + 1) )
			>= TOOBIG )
		    {
			(void) fprintf (stderr,
					"\nError getting co-ordinate number: %u",
					inp_coord_index + 1);
			return (FALSE);
		    }
		    factor = (out_coord -inp_coord1) / (inp_coord2-inp_coord1);
		    out_value[0] = inp_value[0] + factor * (inp_sec_value[0] -
							    inp_value[0]);
		    out_value[1] = inp_value[1] + factor * (inp_sec_value[1] -
							    inp_value[1]);
		}
		else
		{
		    /*  At last co-ordinate of input dimension: copy data  */
		    out_value[0] = inp_value[0];
		    out_value[1] = inp_value[1];
		}
		break;
	      default:
		(void) fprintf (stderr, "\nIllegal value of  option  : %u",
				option);
		a_prog_bug (function_name);
		break;
	    }
	    /*  Write out resampled data  */
	    if (ds_put_element (out_ptr, elem_type, out_value) == NULL)
	    {
		(void) fprintf (stderr, "\nError writing data to memory");
		return (FALSE);
	    }
	    /*  Do the next output co-ordinate  */
	}
	/*  Increment data pointers to next element  */
	inp_data += elem_size;
	out_data += elem_size;
    }
    return (TRUE);
}   /*  End Function resample_dimension  */

/*  Put global variables last to force functions to be explicit  */
unsigned int dim_index;
unsigned int inp_start_coord;
unsigned int inp_stop_coord;
unsigned int out_start_coord;
