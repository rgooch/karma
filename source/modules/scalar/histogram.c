/*  histogram.c

    Source file for  histogram  (histogram generation module).

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

/*  This Karma module will compute a histogram of atomic data.
    The histogram array is written to a new Karma arrayfile.


    Written by      Richard Gooch   4-FEB-1993

    Updated by      Richard Gooch   4-FEB-1993

    Updated by      Richard Gooch   21-MAR-1993: Increased speed.

    Updated by      Richard Gooch   25-MAR-1993: Took account of changes to
  list_header  structure.

    Updated by      Richard Gooch   28-MAR-1993: Took account of changes to
  dsproc_object  function.

    Updated by      Richard Gooch   16-NOV-1993: Changed over to  panel_
  package for command line user interface and moved  main  into this file.

    Updated by      Richard Gooch   23-NOV-1993: Fixed bug in call to
  panel_add_item  for  array_names  parameter.

    Updated by      Richard Gooch   29-NOV-1993: Fixed bug in
  compute_histogram  .

    Updated by      Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   28-SEP-1996: Copy and append new history.


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
#include <karma_im.h>
#include <karma_a.h>
#include <karma_m.h>

#define VERSION "1.2"

#define NUMARRAYS 100
#define BLOCK_LENGTH 256

EXTERN_FUNCTION (flag histogram, (char *command, FILE *fp) );

flag pre_process ();
flag process_array ();
STATIC_FUNCTION (flag post_process,
		 (CONST multi_array *inp_desc, multi_array *out_desc) );
packet_desc *generate_desc ();
flag find_scale ();
flag find_array_minmax ();
flag process_occurrence ();
flag compute_histogram ();

static double element_min = TOOBIG;
static double element_max = -TOOBIG;


/*  Put globals here to force functions to be explicit  */
char *array_names[NUMARRAYS];
unsigned int num_arrays = 0;

int num_bins = 256;

int save_unproc = TRUE;

char *element_name = NULL;
unsigned int *hist_array = NULL;
unsigned int elem_index;


int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    im_register_lib_version (KARMA_VERSION);
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "array_names", "", K_VSTRING, array_names,
		    PIA_ARRAY_LENGTH, &num_arrays, PIA_ARRAY_MIN_LENGTH, 0,
		    PIA_ARRAY_MAX_LENGTH, NUMARRAYS,
		    PIA_END);
    panel_add_item (panel, "num_bins", "absolute value", K_INT, &num_bins,
		    PIA_END);
    panel_add_item (panel, "save_unproc_data", "", PIT_FLAG, &save_unproc,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "histogram", VERSION, histogram, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag histogram (p, fp)
char *p;
FILE *fp;
{
    char *arrayfile;
    extern unsigned int num_arrays;
    extern int save_unproc;
    extern char *element_name;
    extern char *array_names[NUMARRAYS];
    /*static char function_name[] = "histogram";*/

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
	dsproc_object (arrayfile, array_names, num_arrays, (flag) save_unproc,
		       pre_process, process_array, post_process,
		       K_CH_MAP_LOCAL);
	m_free (arrayfile);
	m_free (element_name);
    }
    return (TRUE);
}   /*  End Function histogram  */

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
}   /*  End Function pre_process  */

flag process_array (inp_desc, inp_data, out_desc, out_data)
/*  This routine will process a general data structure, calculating the
    histogram of atomic elements with name pointed to by the external variable
    element_name  .
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
    extern unsigned int *hist_array;
    extern char *element_name;
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
			"NULL external variable dimension name pointer\n");
	a_prog_bug (function_name);
    }
    /*  Generate output descriptor  */
    if ( ( *out_desc = generate_desc (inp_desc, inp_data) ) == NULL )
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
    /*  Get pointer to histogram array  */
    hist_array = (unsigned int *) *(char **) *out_data;
    if (ds_foreach_occurrence (inp_desc, inp_data, element_name, TRUE,
			       process_occurrence) == FALSE)
    {
	a_func_abort (function_name, "error traversing data structures");
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function process_array  */

packet_desc *generate_desc (inp_desc, inp_data)
/*  This routine will generate a general data structure descriptor for
    the output of a histogram programme.
    The input descriptor, which is partly copied, must be pointed to by
    inp_desc  .
    The input data must be pointed to by  inp_data  .
    The input descriptor must contain (at any depth) an element of name
    pointed to by the external variable  element_name  .
    The length of this dimension will be altered.
    The routine returns a pointer to the output descriptor on success, else
    it returns NULL.
*/
packet_desc *inp_desc;
char *inp_data;
{
    packet_desc *enclosing_packet;
    packet_desc *return_value;
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *dim;
    extern int num_bins;
    extern unsigned int elem_index;
    extern double element_min;
    extern double element_max;
    extern char *element_name;
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
    if (ds_element_is_complex ( (*enclosing_packet).element_types[elem_index]))
    {
	(void) fprintf (stderr, "Atomic element: \"%s\" must be real\n",
			element_name);
	return (NULL);
    }
    /*  Need to determine minimum and maximum values of the element  */
    element_min = TOOBIG;
    element_max = -TOOBIG;
    if (ds_foreach_occurrence (inp_desc, inp_data, element_name, TRUE,
			       find_scale) == FALSE)
    {
	(void) fprintf (stderr,
			"Error getting minimum and maximum of input\n");
	a_prog_bug (function_name);
    }
    (void) fprintf (stdout, "Min: %e  max: %e\n", element_min, element_max);
    if ( ( return_value = ds_alloc_packet_desc (1) ) == NULL )
    {
	m_error_notify (function_name, "top level packet descriptor");
	return (NULL);
    }
    if ( ( arr_desc = ds_alloc_array_desc (1, 0) ) == NULL )
    {
	m_error_notify (function_name, "array descriptor");
	ds_dealloc_packet (return_value, NULL);
	return (NULL);
    }
    (*return_value).element_types[0] = K_ARRAY;
    (*return_value).element_desc[0] = (char *) arr_desc;
    if ( ( dim = ds_alloc_dim_desc ("Bin", num_bins, element_min, element_max,
				    TRUE) )
	== NULL )
    {
	m_error_notify (function_name, "dimension descriptor");
	ds_dealloc_packet (return_value, NULL);
	return (NULL);
    }
    (*arr_desc).dimensions[0] = dim;
    (*arr_desc).lengths[0] = (*dim).length;
    if ( ( pack_desc = ds_alloc_packet_desc (1) ) == NULL )
    {
	m_error_notify (function_name, "array packet descriptor");
	ds_dealloc_packet (return_value, NULL);
	return (NULL);
    }
    (*arr_desc).packet = pack_desc;
    (*pack_desc).element_types[0] = K_UINT;
    (*pack_desc).element_desc[0] = st_dup ("Incidences");
    return (return_value);
}   /*  End Function generate_desc  */

static flag post_process (CONST multi_array *inp_desc, multi_array *out_desc)
/*  [SUMMARY] Perform post-processing on a multi_array data structure.
    <inp_desc> The input multi_array descriptor.
    <out_desc> The output multi_array_descriptor.
    [RETURNS] TRUE if the array is to be saved/transmitted, else FALSE.
*/
{
    char txt[STRING_LENGTH];
    extern char *element_name;
    extern char module_name[STRING_LENGTH + 1];
    /*static char function_name[] = "post_process";*/

    if ( !ds_history_copy (out_desc, inp_desc) ) return (FALSE);
    if ( !panel_put_history_with_stack (out_desc, TRUE) ) return (FALSE);
    sprintf (txt, "%s: Name of selected element: \"%s\"",
	     module_name, element_name);
    return ds_history_append_string (out_desc, txt, TRUE);
}   /*  End Function post_process   */

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
    unsigned int elem_offset;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    list_header *list_head;
    list_entry *curr_entry;
    extern unsigned int elem_index;
    extern double element_min;
    extern double element_max;
    static char function_name[] = "find_scale";

    if ( (encls_desc == NULL) || (data == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (index != elem_index)
    {
	(void) fprintf (stderr, "Element index has changed from: %u to: %u\n",
			elem_index, index);
	a_prog_bug (function_name);
    }
    switch (type)
    {
      case NONE:
	/*  Single packet descriptor  */
	pack_desc = (packet_desc *) encls_desc;
	elem_offset = ds_get_element_offset (pack_desc, index);
	return ( find_array_minmax (data + elem_offset,
				    (*pack_desc).element_types[index],
				    1, 0) );
/*
        break;
*/
      case K_ARRAY:
	/*  Array of packet descriptors  */
	arr_desc = (array_desc *) encls_desc;
	pack_desc = (*arr_desc).packet;
	elem_offset = ds_get_element_offset (pack_desc, index);
	return ( find_array_minmax ( data + elem_offset,
				    (*pack_desc).element_types[index],
				    (int) ds_get_array_size (arr_desc),
				    ds_get_packet_size (pack_desc) ) );
/*
        break;
*/
      case LISTP:
	/*  Linked list  */
	pack_desc = (packet_desc *) encls_desc;
	elem_offset = ds_get_element_offset (pack_desc, index);
	list_head = (list_header *) data;
	if ( (*list_head).magic != MAGIC_LIST_HEADER )
	{
	    (void) fprintf (stderr, "List header has bad magic number\n");
	    a_prog_bug (function_name);
	}
	/*  Process contiguous section of list  */
	if ( (*list_head).contiguous_length > 0 )
	{
	    if (find_array_minmax ( (*list_head).contiguous_data + elem_offset,
				   (*pack_desc).element_types[index],
				   (int) (*list_head).contiguous_length,
				   ds_get_packet_size (pack_desc) ) != TRUE)
	    {
		return (FALSE);
	    }
	}
	/*  Process fragmented section of list  */
	for (curr_entry = (*list_head).first_frag_entry; curr_entry != NULL;
	     curr_entry = (*curr_entry).next)
	{
	    /*  Process this entry  */
	    if (find_array_minmax ( (*curr_entry).data + elem_offset,
				   (*pack_desc).element_types[index],
				   1, 0 ) != TRUE)
	    {
		return (FALSE);
	    }
	}
	break;
      default:
	(void) fprintf (stderr, "Illegal descriptor type: %u\n", type);
	a_prog_bug (function_name);
    }
    return (TRUE);
}   /*  End Function find_scale  */

flag find_array_minmax (data, type, length, stride)
/*  This routine will find the minimum and maximum value of the element with
    enclosing descriptor pointed to by  encls_desc  .
    The maximum real or imaginary value and minimum real or imaginary value is
    written to the external variables  element_max  and  element_min  .
    This routine is meant to be called from the  ds_foreach_occurrence  family
    of routines.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *data;
unsigned int type;
int length;
unsigned int stride;
{
    int count;
    int block_count;
    int num_blocks;
    int num_left;
    double *val;
    double values[2 * BLOCK_LENGTH];
    extern double element_min;
    extern double element_max;
    static char function_name[] = "find_array_minmax";

    if (data == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    num_blocks = length / BLOCK_LENGTH;
    for (block_count = 0; block_count < num_blocks;
	 ++block_count, data += BLOCK_LENGTH * stride)
    {
	/*  Process this block  */
	if (ds_get_elements (data, type, stride, values, (flag *) NULL,
			     BLOCK_LENGTH)
	    != TRUE)
	{
	    (void) fprintf (stderr, "Error converting values\n");
	    return (FALSE);
	}
	for (count = 0, val = values; count < BLOCK_LENGTH; ++count, val += 2)
	{
	    /*  Process this value  */
	    if (val[0] >= TOOBIG) continue;
	    if (val[0] > element_max)
	    {
		element_max = val[0];
	    }
	    if (val[0] < element_min)
	    {
		element_min = val[0];
	    }
	}
    }
    /*  Process residual  */
    num_left = length - num_blocks * BLOCK_LENGTH;
    /*  Process this block  */
    if (ds_get_elements (data, type, stride, values, (flag *) NULL,
			 (unsigned int) num_left)
	!= TRUE)
    {
	(void) fprintf (stderr, "Error converting residual values\n");
	return (FALSE);
    }
    for (count = 0, val = values; count < num_left; ++count, val += 2)
    {
	/*  Process this value  */
	if (val[0] >= TOOBIG) continue;
	if (val[0] > element_max)
	{
	    element_max = val[0];
	}
	if (val[0] < element_min)
	{
	    element_min = val[0];
	}
    }
    return (TRUE);
}   /*  End Function find_array_minmax  */

flag process_occurrence (encls_desc, type, data, index)
/*  This routine will process an occurrence of an element with enclosing
    descriptor pointed to by  encls_desc  .
    The histogram data is written to the external array pointed to by
    hist_array  .
    This routine is meant to be called from the  ds_foreach_occurrence  family
    of routines.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *encls_desc;
unsigned int type;
char *data;
unsigned int index;
{
    unsigned int elem_offset;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    list_header *list_head;
    list_entry *curr_entry;
    extern unsigned int elem_index;
    static char function_name[] = "process_occurrence";

    if ( (encls_desc == NULL) || (data == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (index != elem_index)
    {
	(void) fprintf (stderr, "Element index has changed from: %u to: %u\n",
			elem_index, index);
	a_prog_bug (function_name);
    }
    switch (type)
    {
      case NONE:
	/*  Single packet descriptor  */
	pack_desc = (packet_desc *) encls_desc;
	elem_offset = ds_get_element_offset (pack_desc, index);
	return ( compute_histogram (data + elem_offset,
				    (*pack_desc).element_types[index],
				    1, 0) );
/*
        break;
*/
      case K_ARRAY:
	/*  Array of packet descriptors  */
	arr_desc = (array_desc *) encls_desc;
	pack_desc = (*arr_desc).packet;
	elem_offset = ds_get_element_offset (pack_desc, index);
	return ( compute_histogram ( data + elem_offset,
				    (*pack_desc).element_types[index],
				    (int) ds_get_array_size (arr_desc),
				    ds_get_packet_size (pack_desc) ) );
/*
        break;
*/
      case LISTP:
	/*  Linked list  */
	pack_desc = (packet_desc *) encls_desc;
	elem_offset = ds_get_element_offset (pack_desc, index);
	list_head = (list_header *) data;
	if ( (*list_head).magic != MAGIC_LIST_HEADER )
	{
	    (void) fprintf (stderr, "List header has bad magic number\n");
	    a_prog_bug (function_name);
	}
	/*  Process contiguous section of list  */
	if ( (*list_head).contiguous_length > 0 )
	{
	    if (compute_histogram ( (*list_head).contiguous_data + elem_offset,
				   (*pack_desc).element_types[index],
				   (int) (*list_head).contiguous_length,
				   ds_get_packet_size (pack_desc) ) != TRUE)
	    {
		return (FALSE);
	    }
	}
	/*  Process fragmented section of list  */
	for (curr_entry = (*list_head).first_frag_entry; curr_entry != NULL;
	     curr_entry = (*curr_entry).next)
	{
	    /*  Process this entry  */
	    if (compute_histogram ( (*curr_entry).data + elem_offset,
				   (*pack_desc).element_types[index],
				   1, 0 ) != TRUE)
	    {
		return (FALSE);
	    }
	}
	break;
      default:
	(void) fprintf (stderr, "Illegal descriptor type: %u\n", type);
	a_prog_bug (function_name);
    }
    return (TRUE);
}   /*  End Function process_occurrence  */

flag compute_histogram (data, type, length, stride)
/*  This routine will process an occurrence of an element with enclosing
    descriptor pointed to by  encls_desc  .
    The histogram data is written to the external array pointed to by
    hist_array  .
    This routine is meant to be called from the  ds_foreach_occurrence  family
    of routines.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *data;
unsigned int type;
int length;
unsigned int stride;
{
    int count;
    int block_count;
    int num_blocks;
    int num_left;
    unsigned int bin_num;
    double bin_size;
    double *val;
    double values[2 * BLOCK_LENGTH];
    extern double element_min;
    extern double element_max;
    extern unsigned int *hist_array;
    extern int num_bins;
    static char function_name[] = "compute_histogram";

    if (data == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    bin_size = (element_max - element_min) / (num_bins - 1);
    num_blocks = length / BLOCK_LENGTH;
    for (block_count = 0; block_count < num_blocks;
	 ++block_count, data += BLOCK_LENGTH * stride)
    {
	/*  Process this block  */
	if (ds_get_elements (data, type, stride, values, (flag *) NULL,
			     BLOCK_LENGTH)
	    != TRUE)
	{
	    (void) fprintf (stderr, "Error converting values\n");
	    return (FALSE);
	}
	for (count = 0, val = values; count < BLOCK_LENGTH; ++count, val += 2)
	{
	    /*  Process this value  */
	    if (val[0] >= TOOBIG) continue;
	    bin_num = ( (val[0] - element_min) / bin_size );
	    ++hist_array[bin_num];
	}
    }
    /*  Process residual  */
    num_left = length - num_blocks * BLOCK_LENGTH;
    /*  Process this block  */
    if (ds_get_elements (data, type, stride, values, (flag *) NULL,
			 (unsigned int) num_left)
	!= TRUE)
    {
	(void) fprintf (stderr, "Error converting residual values\n");
	return (FALSE);
    }
    for (count = 0, val = values; count < num_left; ++count, val += 2)
    {
	/*  Process this value  */
	if (val[0] >= TOOBIG) continue;
	bin_num = ( (val[0] - element_min) / bin_size );
	++hist_array[bin_num];
    }
    return (TRUE);
}   /*  End Function compute_histogram  */
