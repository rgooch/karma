/*  convert.c

    Source file for  convert  (data type conversion module).

    Copyright (C) 1993-1996  Richard Gooch

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

/*  This Karma module will convert an atomic element of a Karma data file.
    The changes are written to a new Karma arrayfile.


    Written by      Richard Gooch   4-NOV-1992

    Updated by      Richard Gooch   5-NOV-1992

    Updated by      Richard Gooch   3-JAN-1993: Took acccount of change to
  ds_alloc_data

    Updated by      Richard Gooch   25-MAR-1993: Took account of changes to
  list_header  structure.

    Updated by      Richard Gooch   28-MAR-1993: Took account of changes to
  dsproc_object  function.

    Updated by      Richard Gooch   15-NOV-1993: Changed over to  panel_
  package for command line user interface and moved  main  into this file.

    Updated by      Richard Gooch   23-NOV-1993: Fixed bug in call to
  panel_add_item  for  array_names  parameter.

    Updated by      Richard Gooch   29-JUN-1994: Explained precedence of
  factor  and  offset  .

    Updated by      Richard Gooch   7-AUG-1994: Removed  blank_value  parameter
  since  panel_  package did not faithfully maintain the value (besides, the
  parameter was not that useful anyway).

    Last updated by Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_dsproc.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>


#define VERSION "1.1"

#define DEFAULT_LOG_CYCLES 8
#define NUMARRAYS 100
#define BLOCK_SIZE 128

flag pre_process ();
flag process_array ();
flag post_process ();
packet_desc *generate_desc ();
flag process_occurrence ();
flag find_scale ();
double extract_value ();
flag find_average ();
flag convert_values ();

#define K_COMPLEX_REAL 0
#define K_COMPLEX_IMAG 1
#define K_COMPLEX_ABS 2
#define K_COMPLEX_PHASE 3
#define K_COMPLEX_SQUARE 4
#define K_COMPLEX_CONT_PHASE 5

static char *complex_alternatives[] =
{
    "real",
    "imaginary",
    "absolute_value",
    "phase",
    "square_abs",
    "continuous_phase"
};
static int complex_option = K_COMPLEX_REAL;

#define SCALE_FACTOR (unsigned int) 0
#define SCALE_NORMALISE (unsigned int) 1
#define SCALE_RANGE (unsigned int) 2
#define SCALE_SATURATE (unsigned int) 3
#define SCALE_LIMIT_LOG_CYCLES (unsigned int) 4
#define SCALE_SUBTRACT_AVERAGE (unsigned int) 5
#define SCALE_RECIPROCATE (unsigned int) 6
#define NUM_SCALE_OPTIONS (unsigned int) 7

EXTERN_FUNCTION (flag convert, (char *command, FILE *fp) );

static char *scale_alternatives[NUM_SCALE_OPTIONS] =
{
    "factor_offset",
    "normalize",
    "range",
    "saturate",
    "limit_log_cycles",
    "subtract_average",
    "reciprocate"
};
static int scale_option = SCALE_FACTOR;

static int save_unproc = TRUE;

static char *array_names[NUMARRAYS];
static unsigned int num_arrays = 0;
static int type = K_FLOAT;

static double factor = 1.0;
static double offset = 0.0;
static double norm = 1.0;
static double low_range = -1.0;
static double up_range = 1.0;
static double saturate_min = 1.0 / TOOBIG;
static double saturate_max = TOOBIG;
static int log_cycle_limit = DEFAULT_LOG_CYCLES;


int main (int argc, char **argv)
{
    KControlPanel panel;
    extern char *array_names[NUMARRAYS];
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
    panel_add_item (panel, "complex_option", "", PIT_CHOICE_INDEX,
		    &complex_option,
		    PIA_NUM_CHOICE_STRINGS, 5,
		    PIA_CHOICE_STRINGS, complex_alternatives,
		    PIA_END);
    panel_add_item (panel, "factor", "multiplier applied to input", K_DOUBLE,
		    &factor,
		    PIA_END);
    panel_add_item (panel, "log_cycle_limit", "", K_INT, &log_cycle_limit,
		    PIA_END);
    panel_add_item (panel, "lower_range", "absolute value", K_DOUBLE,
		    &low_range,
		    PIA_END);
    panel_add_item (panel, "normalise", "absolute value", K_DOUBLE, &norm,
		    PIA_END);
    panel_add_item (panel, "offset", "absolute value applied after factor",
		    K_DOUBLE, &offset,
		    PIA_END);
    panel_add_item (panel, "saturate_max", "absolute value", K_DOUBLE,
		    &saturate_max,
		    PIA_END);
    panel_add_item (panel, "saturate_min", "absolute value", K_DOUBLE,
		    &saturate_min,
		    PIA_END);
    panel_add_item (panel, "save_unproc_data", "", PIT_FLAG, &save_unproc,
		    PIA_END);
    panel_add_item (panel, "scale_option", "", PIT_CHOICE_INDEX, &scale_option,
		    PIA_NUM_CHOICE_STRINGS, NUM_SCALE_OPTIONS,
		    PIA_CHOICE_STRINGS, scale_alternatives,
		    PIA_END);
    panel_add_item (panel, "type", "data type", PIT_CHOICE_INDEX, &type,
		    PIA_NUM_CHOICE_STRINGS, NUMTYPES,
		    PIA_CHOICE_STRINGS, data_type_names,
		    PIA_END);
    panel_add_item (panel, "upper_range", "absolute value", K_DOUBLE,
		    &up_range,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "convert", VERSION, convert, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag convert (char *p, FILE *fp)
{
    double tmp;
    char *arrayfile;
    extern int scale_option;
    extern int type;
    extern int save_unproc;
    extern unsigned int num_arrays;
    extern double norm;
    extern double low_range;
    extern double up_range;
    extern double saturate_min;
    extern double saturate_max;
    extern char *element_name;
    extern char *new_element_name;
    extern char *array_names[NUMARRAYS];
    extern char *data_type_names[NUMTYPES];
    static char function_name[] = "convert";

    if ( (type == NONE) || (type == MULTI_ARRAY) )
    {
	(void) fprintf (fp,
			"type: \"%s\" not allowed: defaulting to K_FLOAT\n",
			data_type_names[type] );
	type = K_FLOAT;
	return (TRUE);
    }
    if (ds_element_is_atomic ( (unsigned int) type ) == FALSE)
    {
	(void) fprintf (fp, "type must be atomic: defaulting to K_FLOAT\n");
	type = K_FLOAT;
	return (TRUE);
    }
    switch (scale_option)
    {
      case SCALE_FACTOR:
	break;
      case SCALE_NORMALISE:
	if (norm < 0.0)
	{
	    (void) fprintf (fp,
			    "Normalise parameter must be positive: taking abs\n");
	    norm = fabs (norm);
	}
	if (norm == 0.0)
	{
	    (void) fprintf (fp,
			    "Can't normalize to zero: I'll clean up after you\n");
	    norm = 1.0;
	}
	break;
      case SCALE_RANGE:
	if (low_range > up_range)
	{
	    (void) fprintf (fp,
			    "Lower range must be less than upper range\n");
	    (void) fprintf (fp,
			    "Since I'm smarter than you, I'll swap them\n");
	    tmp = low_range;
	    low_range = up_range;
	    up_range = tmp;
	}
	if (low_range == up_range)
	{
	    (void) fprintf (fp,
			    "Can't have zero size range: I'll move them apart 0.5 each way\n");
	    low_range -= 0.5;
	    up_range += 0.5;
	}
	break;
      case SCALE_SATURATE:
	if (saturate_min > saturate_max)
	{
	    (void) fprintf (fp,
			    "Lower saturation point must be less than upper saturation point\n");
	    (void) fprintf (fp,
			    "Since I'm smarter than you, I'll swap them\n");
	    tmp = saturate_min;
	    saturate_min = saturate_max;
	    saturate_max = tmp;
	}
	if (saturate_min == saturate_max)
	{
	    (void) fprintf (fp,
			    "Can't have equal saturation points: using: %g and %g\n",
			    1.0 / TOOBIG, TOOBIG);
	    saturate_min = 1.0 / TOOBIG;
	    saturate_max = TOOBIG;
	}
	break;
      case SCALE_LIMIT_LOG_CYCLES:
      case SCALE_SUBTRACT_AVERAGE:
      case SCALE_RECIPROCATE:
	break;
      default:
	(void) fprintf (fp, "Bad scale_option value: %d\n", scale_option);
	a_prog_bug (function_name);
	break;
    }
    for ( ; p; p = ex_word_skip (p) )
    {
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting arrayfile name\n");
	    return (TRUE);
	}
	if ( ( element_name = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp,
			    "Must specify element name to change type of\n");
	    m_free (arrayfile);
	    return (TRUE);
	}
	new_element_name = ex_str (p, &p);
	dsproc_object (arrayfile, array_names, num_arrays, (flag) save_unproc,
		       pre_process, process_array, post_process,
		       K_CH_MAP_LOCAL);
	m_free (arrayfile);
	m_free (element_name);
	if (new_element_name != NULL)
	{
	    m_free (new_element_name);
	}
    }
    return (TRUE);
}   /*  End Function convert    */

flag pre_process (multi_desc)
/*  This routine will pre process the multi array general data structure
    pointed to by  multi_desc  .
    The routine returns TRUE if processing is to continue, else it
    returns FALSE.
*/
multi_array *multi_desc;
{
    char txt[STRING_LENGTH];
    extern char *element_name;
    static char function_name[] = "pre_process";

    if (ds_f_array_name (multi_desc, element_name, (char **) NULL,
			 (unsigned int *) NULL)
        != IDENT_NOT_FOUND)
    {
	(void) sprintf (txt, "array name must not be: \"%s\"", element_name);
        a_func_abort (function_name, txt);
        return (FALSE);
    }
    return (TRUE);
}   /*  End Function pre_process    */

static unsigned int toobig_count = 0;

flag process_array (inp_desc, inp_data, out_desc, out_data)
/*  This routine will process a general data structure, converting each
    occurrence of the element name in the external string  element_name  to
    another type.
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
    extern int scale_option;
    extern unsigned int toobig_count;
    extern unsigned int total_elements;
    extern char *element_name;
    extern double element_sum[2];
    static char function_name[] = "process_array";

    if ( (inp_desc == NULL) || (out_desc == NULL) )
    {
	(void) fprintf (stderr, "NULL descriptor pointer(s)\n");
        a_prog_bug (function_name);
    }
    if ( (inp_data == NULL) || (out_data == NULL) )
    {
	(void) fprintf (stderr, "NULL data pointer(s)\n");
	a_prog_bug (function_name);
    }
    if (element_name == NULL)
    {
	(void) fprintf (stderr,
			"NULL external variable element name pointer\n");
	a_prog_bug (function_name);
    }
    toobig_count = 0;
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
    (void) ds_copy_data (inp_desc, inp_data, *out_desc, *out_data);
    switch (scale_option)
    {
      case SCALE_FACTOR:
      case SCALE_SATURATE:
      case SCALE_RECIPROCATE:
	break;
      case SCALE_NORMALISE:
      case SCALE_RANGE:
      case SCALE_LIMIT_LOG_CYCLES:
	/*  Need to determine minimum and maximum values of the element  */
	if (ds_foreach_occurrence (inp_desc, inp_data, element_name, FALSE,
				   find_scale) == FALSE)
	{
	    (void) fprintf (stderr,
			    "Error getting minimum and maximum of input\n");
	    a_prog_bug (function_name);
	}
	break;
      case SCALE_SUBTRACT_AVERAGE:
	/*  Need to determine the average value of the element  */
	total_elements = 0;
	element_sum[0] = 0.0;
	element_sum[1] = 0.0;
	if (ds_foreach_occurrence (inp_desc, inp_data, element_name, FALSE,
				   find_average) == FALSE)
	{
	    (void) fprintf (stderr, "Error getting average of input\n");
	    a_prog_bug (function_name);
	}
	break;
      default:
	(void) fprintf (stderr, "Bad scale_option value: %d\n", scale_option);
	a_prog_bug (function_name);
	break;
    }
    if (ds_traverse_and_process (inp_desc, inp_data, *out_desc, *out_data,
				 TRUE, process_occurrence) == FALSE)
    {
	a_func_abort (function_name, "error traversing data structures");
	return (FALSE);
    }
    if (toobig_count > 0)
    {
	(void) fprintf (stderr,
			"Converted: %u occurrences of TOOBIG values\n",
			toobig_count);
    }
    return (TRUE);
}   /*  End Function process_array  */

static unsigned int elem_index;

packet_desc *generate_desc (inp_desc)
/*  This routine will generate a general data structure descriptor for
    the output of a data type conversion program.
    The input descriptor, which is partly copied, must be pointed to by
    inp_desc  .
    The input descriptor must contain (at any depth) an atomic element of name
    pointed to by the external variable  element_name  .
    The type of the element is changed to the value in the external variable
    type  .
    The routine returns a pointer to the output descriptor on success, else
    it returns NULL.
*/
packet_desc *inp_desc;
{
    unsigned int elem_num;
    packet_desc *return_value;
    packet_desc *enclosing_packet;
    extern int type;
    extern unsigned int elem_index;
    extern char *element_name;
    extern char *new_element_name;
    static char function_name[] = "generate_desc";

    /*  Do tests on input descriptor    */
    switch ( ds_f_name_in_packet (inp_desc, element_name,
				  (char **) &enclosing_packet, &elem_index) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr,
			"Atomic element: \"%s\" not found\n", element_name);
	return (NULL);
	break;
      case IDENT_ELEMENT:
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Item \"%s\" found more than once\n", element_name);
	return (NULL);
	break;
      case IDENT_DIMENSION:
	(void) fprintf (stderr,
			"Item: \"%s\" must be an element, not a dimension name\n",
			element_name);
	return (NULL);
	break;
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"Bad return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    if ( (*enclosing_packet).element_types[elem_index] == (unsigned int) type )
    {
	(void) fprintf (stderr, "Input and output types are the same\n");
	return (NULL);
    }
    /*  Copy data structure descriptor  */
    if ( ( return_value = ds_copy_desc_until (inp_desc, (char *) NULL) )
	== NULL )
    {
	a_func_abort (function_name, "error copying descriptors");
	return (NULL);
    }
    /*  Do tests on output descriptor    */
    switch ( ds_f_name_in_packet (return_value, element_name,
				  (char **) &enclosing_packet, &elem_num) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr,
			"Output atomic element: \"%s\" not found\n",
			element_name);
	a_prog_bug (function_name);
	break;
      case IDENT_ELEMENT:
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Output item \"%s\" found more than once\n",
			element_name);
	a_prog_bug (function_name);
	break;
      case IDENT_DIMENSION:
	(void) fprintf (stderr,
			"Output item: \"%s\" must be an element, not a dimension name\n",
			element_name);
	a_prog_bug (function_name);
	break;
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"Bad return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    if (elem_num != elem_index)
    {
	(void) fprintf (stderr, "Element index has changed from: %u to: %u\n",
			elem_index, elem_num);
	a_prog_bug (function_name);
    }
    /*  Change type of output element  */
    (*enclosing_packet).element_types[elem_num] = (unsigned int) type;
    if (new_element_name != NULL)
    {
	/*  Change name of output element  */
	m_free ( (*enclosing_packet).element_desc[elem_num] );
	if ( ( (*enclosing_packet).element_desc[elem_num] =
	      st_dup (new_element_name) ) == NULL )
	{
	    m_error_notify (function_name, "new element name");
	    ds_dealloc_packet (return_value, (char *) NULL);
	    return (FALSE);
	}
    }
    return (return_value);
}   /*  End Function generate_desc  */

flag post_process (/* inp_array, out_array */)
/*  This routine will perform post processing on the composite array pointed
    to by  out_array  .
    The routine will copy over history information from the input composite
    array pointed to by  inp_array  .
    The routine returns TRUE if the array is to be saved, else it
    returns FALSE.
*/
/*
struct composite_array *inp_array;
struct composite_array *out_array;
*/
{
#ifdef dummy
    char txt[STRING_LENGTH];
    extern char *element_name;
    extern struct variable_description vble[];
    static char function_name[] = "post_process";

    ez_description (out_array, vble);
    (void) sprintf (txt, "Name of converted element: \"%s\"", element_name);
    composite_descr (txt, out_array);
    ds_copy_composite_descr (inp_array, out_array);
#endif
    return (TRUE);
}   /*  End Function post_process   */

static double element_min = TOOBIG;
static double element_max = -TOOBIG;

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
    The routine is meant to be called from the  ds_traverse_and_process
    family of routines.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *inp_desc;
unsigned int inp_type;
char *inp_data;
char *out_desc;
unsigned int out_type;
char *out_data;
{
    unsigned int array_size = 0;  /*  Initialised to keep compiler happy  */
    unsigned int inp_offset;
    unsigned int out_offset;
    unsigned int out_pack_size;
    char *out_ptr;
    list_header *list_head_inp = NULL; /* Initialised to keep compiler happy */
    list_header *list_head_out = NULL; /* Initialised to keep compiler happy */
    list_entry *list_entry_inp;
    packet_desc *pack_desc_inp = NULL; /* Initialised to keep compiler happy */
    packet_desc *pack_desc_out = NULL; /* Initialised to keep compiler happy */
    extern unsigned int elem_index;
    extern int type;
    extern char *data_type_names[NUMTYPES];
    static char function_name[] = "process_occurrence";

    if ( (inp_desc == NULL) || (inp_data == NULL)
	|| (out_desc == NULL) || (out_data == NULL) )
    {
	(void) fprintf (stderr, "NULL descriptor(s) passed\n");
	a_prog_bug (function_name);
    }
    if (inp_type != out_type)
    {
	/*  Descriptors are of the same type  */
	(void) fprintf (stderr,
			"Descriptors: input: %s output: %s are not of the same type\n",
			data_type_names[inp_type], data_type_names[out_type]);
	a_prog_bug (function_name);
    }
    switch (inp_type)
    {
      case K_ARRAY:
	/*  Array  */
	pack_desc_inp = (* (array_desc *) inp_desc ).packet;
	pack_desc_out = (* (array_desc *) out_desc ).packet;
	array_size = ds_get_array_size ( (array_desc *) inp_desc );
	if ( array_size != ds_get_array_size ( (array_desc *) out_desc ) )
	{
	    (void) fprintf (stderr, "Array sizes have changed\n");
	    a_prog_bug (function_name);
	}
	break;
      case LISTP:
	list_head_inp = (list_header *) inp_data;
	if ( (*list_head_inp).magic != MAGIC_LIST_HEADER )
	{
	    (void) fprintf (stderr,
			    "Input list header has bad magic number\n");
	    a_prog_bug (function_name);
	}
	list_head_out = (list_header *) out_data;
	if ( (*list_head_out).magic != MAGIC_LIST_HEADER )
	{
	    (void) fprintf (stderr,
			    "Output list header has bad magic number\n");
	    a_prog_bug (function_name);
	}
	if ( (*list_head_inp).length != (*list_head_out).length )
	{
	    (void) fprintf (stderr, "List sizes have changed\n");
	    a_prog_bug (function_name);
	}
      case NONE:
	pack_desc_inp = (packet_desc *) inp_desc;
	pack_desc_out = (packet_desc *) out_desc;
	break;
    }
    out_pack_size = ds_get_packet_size (pack_desc_out);
    /*  Do a quick check on the packet descriptors  */
    if ( (*pack_desc_inp).num_elements != (*pack_desc_out).num_elements )
    {
	(void) fprintf (stderr,
			"Packet descriptors have different num. of elements: %u %u\n",
			(*pack_desc_inp).num_elements,
			(*pack_desc_out).num_elements);
	a_prog_bug (function_name);
    }
    if ( (*pack_desc_out).element_types[elem_index] != (unsigned int) type )
    {
	(void) fprintf (stderr,
			"Output element descriptor type has changed to: \"%s\"\n",
			data_type_names[ (*pack_desc_out).element_types[elem_index] ]);
	a_prog_bug (function_name);
    }
    inp_offset = ds_get_element_offset (pack_desc_inp, elem_index);
    out_offset = ds_get_element_offset (pack_desc_out, elem_index);
    switch (inp_type)
    {
      case K_ARRAY:
	/*  Array: process in bulk  */
	return ( convert_values (inp_data + inp_offset,
				 (*pack_desc_inp).element_types[elem_index],
				 ds_get_packet_size (pack_desc_inp),
				 out_data + out_offset,
				 (*pack_desc_out).element_types[elem_index],
				 out_pack_size, array_size) );
	
	break;
      case LISTP:
	/*  List: process each entry  */
	/*  Process input contiguous section in bulk  */
	if (convert_values ( (*list_head_inp).contiguous_data + inp_offset,
			    (*pack_desc_inp).element_types[elem_index],
			    ds_get_packet_size (pack_desc_inp),
			    (*list_head_out).contiguous_data + out_offset,
			    (*pack_desc_out).element_types[elem_index],
			    out_pack_size, (*list_head_inp).contiguous_length )
	    != TRUE)
	{
	    return (FALSE);
	}
	/*  Process input fragmented section  */
	for (list_entry_inp = (*list_head_inp).first_frag_entry,
	     out_ptr = (*list_head_out).contiguous_data + out_offset +
	     out_pack_size * (*list_head_inp).contiguous_length;
	     list_entry_inp != NULL;
	     list_entry_inp = (*list_entry_inp).next, out_ptr += out_pack_size)
	{
	    /*  Process this entry  */
	    if (convert_values ( (*list_entry_inp).data + inp_offset,
				(*pack_desc_inp).element_types[elem_index],
				(unsigned int) 1,
				out_ptr,
				(*pack_desc_out).element_types[elem_index],
				(unsigned int) 1, (unsigned int) 1 ) != TRUE)
	    {
		return (FALSE);
	    }
	}
	break;
      case NONE:
	/*  Single packet: process differing element  */
	return ( convert_values (inp_data + inp_offset,
				 (*pack_desc_inp).element_types[elem_index],
				 ds_get_packet_size (pack_desc_inp),
				 out_data + out_offset,
				 (*pack_desc_out).element_types[elem_index],
				 out_pack_size, (unsigned int) 1) );
	break;
    }
    return (TRUE);
}   /*  End Function process_occurrence  */

flag find_scale (encls_desc, type, data, index)
/*  This routine will find the minimum and maximum value of the element with
    enclosing descriptor pointed to by  encls_desc  .
    The maximum real or imaginary value and minimum real or imaginary value is
    written to the external variables  element_max  and  element_min  .
    This routine is meant to be called from the  ds_foreach_occurrence  family
    of routines.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *encls_desc;
unsigned int type;
char *data;
unsigned int index;
{
    double real;
    double imag;
    packet_desc *pack_desc;
    extern unsigned int elem_index;
    extern double element_min;
    extern double element_max;
    static char function_name[] = "find_scale";

    if ( (encls_desc == NULL) || (data == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (type != NONE)
    {
	(void) fprintf (stderr,
			"Enclosing desciptor must be a packet descriptor\n");
	a_prog_bug (function_name);
    }
    if (index != elem_index)
    {
	(void) fprintf (stderr, "Element index has changed from: %u to: %u\n",
			elem_index, index);
	a_prog_bug (function_name);
    }
    pack_desc = (packet_desc*) encls_desc;
    if (ds_convert_atomic (data + ds_get_element_offset (pack_desc,
							 elem_index),
			   (*pack_desc).element_types[elem_index],
			   &real, &imag) >= TOOBIG)
    {
	/*  Skip this value  */
	return (TRUE);
    }
    if (real > element_max)
    {
	element_max = real;
    }
    if (real < element_min)
    {
	element_min = real;
    }
    switch ( (*pack_desc).element_types[elem_index] )
    {
      case K_COMPLEX:
      case K_DCOMPLEX:
      case K_BCOMPLEX:
      case K_ICOMPLEX:
      case K_SCOMPLEX:
	/*  Data is complex: look at the imaginary bit too  */
	if (imag > element_max)
	{
	    element_max = imag;
	}
	if (imag < element_min)
	{
	    element_min = imag;
	}
	break;
    }
    return (TRUE);
}   /*  End Function find_scale  */

double extract_value (input, operation)
/*  This routine will extract a double precision floating point number from a
    complex double pointed to by  input  .
    The conversion from complex to real is determined by the value of
    operation  .
    The routine returns the real value.
*/
double *input;
unsigned int operation;
{
    static char function_name[] = "extract_value";

    if (input == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    switch (operation)
    {
      case K_COMPLEX_REAL:
	return (input[0]);
	break;
      case K_COMPLEX_IMAG:
	return (input[1]);
	break;
      case K_COMPLEX_ABS:
	return ( sqrt (input[0] * input[0] + input[1] * input[1]) );
	break;
      case K_COMPLEX_PHASE:
	if ( (input[0] == 0.0) && (input[1] == 0.0) )
	{
	    return (0.0);
	}
	else
	{
	    return ( atan2 (input[1], input[0]) / PION180 );
	}
	break;
      case K_COMPLEX_SQUARE:
	return (input[0] * input[0] + input[1] * input[1]);
	break;
      case K_COMPLEX_CONT_PHASE:
	(void) fprintf (stderr, "Continuous phase not implemented yet\n");
	return (TOOBIG);
	break;
      default:
	(void) fprintf (stderr, "Bad operation value: %u\n", operation);
	a_prog_bug (function_name);
	break;
    }
    return (TOOBIG);
}   /*  End Function extract_value  */

flag find_average (encls_desc, type, data, index)
/*  This routine will find the average value of the element with enclosing
    descriptor pointed to by  encls_desc  .
    The sum of the values is written to the external array variable
    element_sum  .
    This routine is meant to be called from the  ds_foreach_occurrence  family
    of routines.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *encls_desc;
unsigned int type;
char *data;
unsigned int index;
{
    double real;
    double imag;
    packet_desc *pack_desc;
    extern unsigned int elem_index;
    extern unsigned int total_elements;
    extern double element_sum[2];
    static char function_name[] = "find_average";

    if ( (encls_desc == NULL) || (data == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (type != NONE)
    {
	(void) fprintf (stderr,
			"Enclosing desciptor must be a packet descriptor\n");
	a_prog_bug (function_name);
    }
    if (index != elem_index)
    {
	(void) fprintf (stderr, "Element index has changed from: %u to: %u\n",
			elem_index, index);
	a_prog_bug (function_name);
    }
    pack_desc = (packet_desc*) encls_desc;
    if (ds_convert_atomic (data + ds_get_element_offset (pack_desc,
							 elem_index),
			   (*pack_desc).element_types[elem_index],
			   &real, &imag) >= TOOBIG)
    {
	/*  Skip this value  */
	return (TRUE);
    }
    /*  Sum real component  */
    element_sum[0] += real;
    /*  Sum imaginary component  */
    element_sum[1] += imag;
    /* Increment count of good elements  */
    ++total_elements;
    return (TRUE);
}   /*  End Function find_average  */

flag convert_values (inp_data, inp_type, inp_stride,
		     out_data, out_type, out_stride, num_values)
/*  This routine will convert a number of data values from one type to another.
    The input data must be pointed to by  inp_data  .
    The type of the input data must be given by  inp_type  .
    The stride of input data (in bytes) must be given by  inp_stride  .
    The output data must be pointed to by  out_data  .
    The type of the output data must be given by  out_type  .
    The stride of output data (in bytes) must be goven by  out_stride  .
    The number of data values to convert must be given by  num_values  .
    The routine returns TRUE on success, else it returns FALSE.
*/
char *inp_data;
unsigned int inp_type;
unsigned int inp_stride;
char *out_data;
unsigned int out_type;
unsigned int out_stride;
unsigned int num_values;
{
    flag complex_flag;
    flag complex_to_real;
    int elem_count;
    int block_size;
    unsigned int data_left;
    double sq_abs;
    double input[2 * BLOCK_SIZE];
    double output[2 * BLOCK_SIZE];
    double log_cycle_limit_scale;
    extern int scale_option;
    extern int complex_option;
    extern int log_cycle_limit;
    extern unsigned int toobig_count;
    extern unsigned int total_elements;
    extern double factor;
    extern double offset;
    extern double norm;
    extern double low_range;
    extern double up_range;
    extern double element_min;
    extern double element_max;
    extern double saturate_min;
    extern double saturate_max;
    extern double element_sum[2];
    static char function_name[] = "convert_values";

    log_cycle_limit_scale = pow (10.0, (double) log_cycle_limit);
    /*  Loop through blocks of data  */
    for (data_left = num_values; data_left > 0;
	 data_left -= block_size,
	 inp_data += inp_stride * block_size,
	 out_data += out_stride * block_size)
    {
	block_size = (data_left > BLOCK_SIZE) ? BLOCK_SIZE : data_left;
	/*  Process (convert) block of data  */
	/*  Get input values  */
	if (ds_get_elements (inp_data, inp_type, inp_stride,
			     input, &complex_flag, block_size) != TRUE)
	{
	    return (FALSE);
	}
	/* Do something special if input is a complex type and output is not */
	if (complex_flag)
	{
	    /*  Input is a complex type  */
	    switch (out_type)
	    {
	      case K_FLOAT:
	      case K_DOUBLE:
	      case K_INT:
	      case K_SHORT:
	      case K_BYTE:
	      case K_LONG:
	      case K_UBYTE:
	      case K_UINT:
	      case K_USHORT:
	      case K_ULONG:
		/*  Output is not a complex type  */
		complex_to_real = TRUE;
		break;
	      default:
		complex_to_real = FALSE;
		break;
	    }
	}
	else
	{
	    complex_to_real = FALSE;
	}
	if (complex_to_real)
	{
	    switch (complex_option)
	    {
	      case K_COMPLEX_REAL:
		break;
	      case K_COMPLEX_IMAG:
		/*  Copy Imaginary values  */
		for (elem_count = 0; elem_count < block_size; ++elem_count)
		{
		    input[elem_count * 2] = input[elem_count * 2 + 1];
		}
		break;
	      case K_COMPLEX_ABS:
		for (elem_count = 0; elem_count < block_size; ++elem_count)
		{
		    input[elem_count * 2] = sqrt (input[elem_count * 2]
						  * input[elem_count * 2] +
						  input[elem_count * 2 + 1]
						  * input[elem_count*2+1]);
		}
		break;
	      case K_COMPLEX_PHASE:
		for (elem_count = 0; elem_count < block_size; ++elem_count)
		{
		    if ( (input[elem_count * 2] == 0.0) &&
			(input[elem_count * 2 + 1] == 0.0) )
		    {
			input[elem_count * 2] = 0.0;
		    }
		    else if ( (input[elem_count * 2] >= TOOBIG) ||
			     (input[elem_count * 2 + 1] >= TOOBIG) )
		    {
			input[elem_count * 2] = TOOBIG;
		    }
		    else
		    {
			input[elem_count * 2] = atan2 (input[elem_count
							     * 2 + 1],
						       input[elem_count *
							     2]) / PION180;
		    }
		}
		break;
	      case K_COMPLEX_SQUARE:
		for (elem_count = 0; elem_count < block_size; ++elem_count)
		{
		    input[elem_count * 2] = (input[elem_count * 2] *
					     input[elem_count * 2] +
					     input[elem_count * 2 + 1] *
					     input[elem_count * 2 + 1]);
		}
		break;
	      case K_COMPLEX_CONT_PHASE:
		(void) fprintf (stderr,
				"Continuous phase not implemented yet\n");
		return (FALSE);
		break;
	      default:
		(void) fprintf (stderr, "Bad operation value: %u\n",
				complex_option);
		a_prog_bug (function_name);
		break;
	    }
	    /*  Clear Imaginary values  */
	    for (elem_count = 0; elem_count < block_size; ++elem_count)
	    {
		input[elem_count * 2 + 1] = 0.0;
	    }
	}
	/*  Convert output values  */
	switch (scale_option)
	{
	  case SCALE_FACTOR:
	    for (elem_count = 0; elem_count < block_size; ++elem_count)
	    {
		if ( (input[elem_count * 2] >= TOOBIG) ||
		    (input[elem_count * 2 + 1] >= TOOBIG) )
		{
		    output[elem_count * 2] = TOOBIG;
		    output[elem_count * 2 + 1] = TOOBIG;
		    ++toobig_count;
		}
		else
		{
		    output[elem_count * 2] = offset + (input[elem_count * 2] *
						       factor);
		    output[elem_count * 2 + 1] = (input[elem_count * 2 + 1] *
						  factor) + offset;
		}
	    }
	    break;
	  case SCALE_NORMALISE:
	    for (elem_count = 0; elem_count < block_size; ++elem_count)
	    {
		if ( (input[elem_count * 2] >= TOOBIG) ||
		    (input[elem_count * 2 + 1] >= TOOBIG) )
		{
		    output[elem_count * 2] = TOOBIG;
		    output[elem_count * 2 + 1] = TOOBIG;
		    ++toobig_count;
		}
		else
		{
		    output[elem_count * 2] = ( (input[elem_count * 2] -
						element_min) * 2.0 * norm /
					      (element_max - element_min) ) - norm;
		    output[elem_count * 2 + 1] = ( (input[elem_count * 2 + 1] -
						    element_min) * 2.0 * norm /
						  (element_max - element_min) ) -
						  norm;
		}
	    }
	    break;
	  case SCALE_RANGE:
	    for (elem_count = 0; elem_count < block_size; ++elem_count)
	    {
		if ( (input[elem_count * 2] >= TOOBIG) ||
		    (input[elem_count * 2 + 1] >= TOOBIG) )
		{
		    output[elem_count * 2] = TOOBIG;
		    output[elem_count * 2 + 1] = TOOBIG;
		    ++toobig_count;
		}
		else
		{
		    output[elem_count * 2] = ( (input[elem_count * 2] -
						element_min) * (up_range -
								low_range) /
					      (element_max - element_min) ) +
					      low_range;
		    output[elem_count * 2 + 1] = ( (input[elem_count * 2 + 1] -
						    element_min) * (up_range -
								    low_range) /
						  (element_max - element_min) ) +
						  low_range;
		}
	    }
	    break;
	  case SCALE_SATURATE:
	    for (elem_count = 0; elem_count < block_size; ++elem_count)
	    {
		if ( (input[elem_count * 2] >= TOOBIG) ||
		    (input[elem_count * 2 + 1] >= TOOBIG) )
		{
		    output[elem_count * 2] = TOOBIG;
		    output[elem_count * 2 + 1] = TOOBIG;
		    ++toobig_count;
		}
		else
		{
		    output[elem_count * 2] = (input[elem_count * 2] <
					      saturate_min) ? saturate_min :
					      input[elem_count * 2];
		    output[elem_count * 2] = (output[elem_count * 2] >
					      saturate_max) ? saturate_max :
					      output[elem_count * 2];
		    output[elem_count * 2 + 1] = (input[elem_count * 2 + 1] <
						  saturate_min) ? saturate_min :
						  input[elem_count * 2 + 1];
		    output[elem_count * 2 + 1] = (output[elem_count * 2 + 1] >
						  saturate_max) ? saturate_max :
						  output[elem_count * 2 + 1];
		}
	    }
	    break;
	  case SCALE_LIMIT_LOG_CYCLES:
	    for (elem_count = 0; elem_count < block_size; ++elem_count)
	    {
		if ( (input[elem_count * 2] >= TOOBIG) ||
		    (input[elem_count * 2 + 1] >= TOOBIG) )
		{
		    output[elem_count * 2] = TOOBIG;
		    output[elem_count * 2 + 1] = TOOBIG;
		    ++toobig_count;
		}
		else
		{
		    output[elem_count * 2] = ( (input[elem_count * 2] <
						element_max /
						log_cycle_limit_scale)
					      ? element_max /
					      log_cycle_limit_scale :
					      input[elem_count * 2] );
		    output[elem_count * 2 + 1] = ( (input[elem_count * 2 + 1] <
						    element_max /
						    log_cycle_limit_scale)
						  ? element_max /
						  log_cycle_limit_scale :
						  input[elem_count * 2 + 1] );
		}
	    }
	    break;
	  case SCALE_SUBTRACT_AVERAGE:
	    for (elem_count = 0; elem_count < block_size; ++elem_count)
	    {
		if ( (input[elem_count * 2] >= TOOBIG) ||
		    (input[elem_count * 2 + 1] >= TOOBIG) )
		{
		    output[elem_count * 2] = TOOBIG;
		    output[elem_count * 2 + 1] = TOOBIG;
		    ++toobig_count;
		}
		else
		{
		    output[elem_count * 2] = (input[elem_count * 2] -
					      element_sum[0] /
					      (double) total_elements);
		    output[elem_count * 2 + 1] = (input[elem_count * 2 + 1] -
						  element_sum[1] /
						  (double) total_elements);
		}
	    }
	    break;
	  case SCALE_RECIPROCATE:
	    for (elem_count = 0; elem_count < block_size; ++elem_count)
	    {
		if ( (input[elem_count * 2] >= TOOBIG) ||
		    (input[elem_count * 2 + 1] >= TOOBIG) )
		{
		    output[elem_count * 2] = TOOBIG;
		    output[elem_count * 2 + 1] = TOOBIG;
		    ++toobig_count;
		}
		else
		{
		    sq_abs = input[elem_count * 2 + 1] * input[elem_count * 2 + 1];
		    sq_abs += input[elem_count * 2] * input[elem_count * 2];
		    output[elem_count * 2] = input[elem_count * 2] / sq_abs;
		    output[elem_count * 2 + 1] = -input[elem_count * 2 +1] /sq_abs;
		}
	    }
	    break;
	  default:
	    (void) fprintf (stderr, "Bad scale_option value: %d\n",
			    scale_option);
	    a_prog_bug (function_name);
	    break;
	}
	/*  Write output values  */
	(void) ds_put_elements (out_data, out_type, out_stride, output,
				block_size);
    }
    return (TRUE);
}   /*  End Function convert_values  */


char *element_name = NULL;
char *new_element_name = NULL;
unsigned int total_elements;
double element_sum[2];
