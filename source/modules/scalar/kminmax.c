/*  kminmax.c

    Source file for  kminmax  (scalar min-max determination module).

    Copyright (C) 1994  Richard Gooch

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

/*  This Karma module will determine the minimum and maximum values of a scalar


    Written by      Richard Gooch   21-APR-1994

    Last updated by Richard Gooch   21-APR-1994


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>

#define VERSION "1.0"

#define NUMARRAYS 100
#define BLOCK_LENGTH 256

EXTERN_FUNCTION (flag kminmax, (char *command, FILE *fp) );

void process_arrayfile ();
flag process_array ();
flag find_scale ();
flag find_array_minmax ();

unsigned int toobig_count;
double elem_real_min;
double elem_imag_min;
double elem_abs_min;
double elem_real_max;
double elem_imag_max;
double elem_abs_max;
char *element_name = NULL;


/*  Put globals here to force functions to be explicit  */
char *array_names[NUMARRAYS];
unsigned int num_arrays = 0;


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
    panel_push_onto_stack (panel);
    module_run (argc, argv, "kminmax", VERSION, kminmax, -1, -1, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag kminmax (p, fp)
char *p;
FILE *fp;
{
    char *arrayfile;
    extern unsigned int num_arrays;
    extern char *element_name;
    extern char *array_names[NUMARRAYS];
    static char function_name[] = "kminmax";

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
	process_arrayfile (arrayfile, array_names, num_arrays, element_name);
	m_free (arrayfile);
	m_free (element_name);
    }
    return (TRUE);
}   /*  End Function kminmax  */

void process_arrayfile (arrayfile, array_names, num_arrays, elem_name)
/*  This routine will get element statistics for an element in an arrayfile
    specified by  arrayfile  .
    The general data structure arrays to process must be named by the array
    of names pointed to by  array_names  .The number of names in this array
    must be in  num_arrays  .
    The name of the elemtent to get statistics on must be pointed to by
    elem_name  .
    The routine returns nothing.
*/
char *arrayfile;
char *array_names[];
unsigned int num_arrays;
char *elem_name;
{
    flag found_name;
    unsigned int array_count;
    unsigned int name_count;
    char *data;
    multi_array *multi_desc;
    packet_desc *pack_desc;
    static char function_name[] = "process_arrayfile";

    if (arrayfile == NULL)
    {
	(void) fprintf (stderr, "NULL arrayfile name\n");
	a_prog_bug (function_name);
    }
    if (elem_name == NULL)
    {
	(void) fprintf (stderr,
			"NULL external variable element name pointer\n");
	a_prog_bug (function_name);
    }
    /*  Read in arrayfile  */
    if ( ( multi_desc = dsxfr_get_multi (arrayfile, FALSE, K_CH_MAP_LOCAL,
					 FALSE) ) == NULL )
    {
	return;
    }
    if ( (*multi_desc).num_arrays < 2 )
    {
	/*  Only one array to process  */
	if (num_arrays != 0)
	{
	    (void) fprintf (stderr,
			    "Arrayfile has only one general data structure");
	    (void) fprintf (stderr, ": naming not allowed\n");
	    return;
	}
	pack_desc = (*multi_desc).headers[0];
	data = (*multi_desc).data[0];
	(void) process_array (pack_desc, data, elem_name);
	return;
    }
    if (num_arrays == 0)
    {
	/*  Iterate through arrays  */
	for (array_count = 0; array_count < (*multi_desc).num_arrays;
	     ++array_count)
	{
	    pack_desc = (*multi_desc).headers[array_count];
	    data = (*multi_desc).data[array_count];
	    (void) fprintf (stderr, "Stats for array: \"%s\"\n",
			    (*multi_desc).array_names[array_count]);
	    if (process_array (pack_desc, data, elem_name) != TRUE)
	    {
		return;
	    }
	}
	return;
    }
    /*  Iterate through array names specified by user  */
    for (name_count = 0; name_count < num_arrays; ++name_count)
    {
	/*  Find name  */
	found_name = FALSE;
	for (array_count = 0; array_count < (*multi_desc).num_arrays;
	     ++array_count)
	{
	    if (strcmp (array_names[name_count],
			(*multi_desc).array_names[array_count]) == 0)
	    {
		found_name = TRUE;
		pack_desc = (*multi_desc).headers[array_count];
		data = (*multi_desc).data[array_count];
		(void) fprintf (stderr, "Stats for array: \"%s\"\n",
				(*multi_desc).array_names[array_count]);
		if (process_array (pack_desc, data, elem_name) != TRUE)
		{
		    return;
		}
		/*  Do it for this array  */
	    }
	    if (found_name != TRUE)
	    {
		/*  Array name not found  */
		(void) fprintf (stderr, "Array name: \"%s\" not found\n",
				array_names[name_count]);
		return;
	    }
	}
    }
    return;
}   /*  End Function process_arrayfile  */

flag process_array (pack_desc, data, elem_name)
/*  This routine will get statistics on an element in a general data structure.
    The packet descriptor must be pointed to by  pack_desc  .
    The data must be pointed to by  data  .
    The name of the element to get statistics for must be pointed to by
    elem_name  .
    The routine returns TRUE on succes, else it returns FALSE.
*/
packet_desc *pack_desc;
char *data;
char *elem_name;
{
    extern unsigned int toobig_count;
    extern double elem_real_min;
    extern double elem_imag_min;
    extern double elem_abs_min;
    extern double elem_real_max; 
    extern double elem_imag_max;
    extern double elem_abs_max;
    static char function_name[] = "process_array";

    if ( (pack_desc == NULL) || (data == NULL) || (elem_name == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Do tests on input descriptor    */
    switch ( ds_f_name_in_packet (pack_desc, elem_name,
				  (char **) NULL, (unsigned int *) NULL) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr,
			"Atomic element: \"%s\" not found\n", elem_name);
	return (FALSE);
/*
	break;
*/
      case IDENT_ELEMENT:
	break;
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Item \"%s\" found more than once\n", elem_name);
	return (FALSE);
/*
	break;
*/
      case IDENT_DIMENSION:
	(void) fprintf (stderr,
			"Item: \"%s\" must be an element, not a dimension name\n",
			elem_name);
	return (FALSE);
/*
	break;
*/
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"Bad return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    /*  Start statistics gathering  */
    toobig_count = 0;
    elem_real_min = TOOBIG;
    elem_imag_min = TOOBIG;
    elem_abs_min = TOOBIG;
    elem_real_max = -TOOBIG;
    elem_imag_max = -TOOBIG;
    elem_abs_max = -TOOBIG;
    if (ds_foreach_occurrence (pack_desc, data, elem_name, TRUE, find_scale)
	== FALSE)
    {
	a_func_abort (function_name, "error traversing data structures");
	return (FALSE);
    }
    if (toobig_count > 0)
    {
	(void) fprintf (stderr,
			"%u occurrences of TOOBIG\n", toobig_count);
    }
    if ( (elem_imag_min >= TOOBIG) || (elem_imag_max <= -TOOBIG) )
    {
	/*  Data is purely real  */
	(void) fprintf (stderr, "Minimum value: %g  maximum_value: %g\n",
			elem_real_min, elem_real_max);
    }
    else
    {
	/*  Data is complex  */
	(void) fprintf (stderr,
			"Minimum magnitude: %g  maximum magnitude: %g\n",
			elem_abs_min, elem_abs_max);
	(void) fprintf (stderr,
			"Minimum real value: %g  maximum real value: %g\n",
			elem_real_min, elem_real_max);
	(void) fprintf (stderr,
			"Minimum imaginary value: %g  maximum imaginary value: %g\n",
			elem_imag_min, elem_imag_max);
    }
    return (TRUE);
}   /*  End Function process_array  */

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
    extern double element_min;
    extern double element_max;
    static char function_name[] = "find_scale";

    if ( (encls_desc == NULL) || (data == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
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
    flag complex;
    int count;
    int block_count;
    int num_blocks;
    int num_left;
    double abs;
    double toobig = TOOBIG;
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
	if (ds_get_elements (data, type, stride, values, &complex,
			     BLOCK_LENGTH)
	    != TRUE)
	{
	    (void) fprintf (stderr, "Error converting values\n");
	    return (FALSE);
	}
	for (count = 0, val = values; count < BLOCK_LENGTH; ++count, val += 2)
	{
	    /*  Process this value  */
	    if (val[0] >= toobig)
	    {
		++toobig_count;
		continue;
	    }
	    if (val[0] > elem_real_max) elem_real_max = val[0];
	    if (val[0] < elem_real_min) elem_real_min = val[0];
	    if (complex)
	    {
		/*  Complex value  */
		if (val[1] >= toobig)
		{
		    ++toobig_count;
		    continue;
		}
		abs = sqrt (val[0] * val[0] + val[1] * val[1]);
		if (abs > elem_abs_max) elem_abs_max = abs;
		if (abs < elem_abs_min) elem_abs_min = abs;
		if (val[1] > elem_imag_max) elem_imag_max = val[1];
		if (val[1] < elem_imag_min) elem_imag_min = val[1];
	    }
	}
    }
    /*  Process residual  */
    num_left = length - num_blocks * BLOCK_LENGTH;
    /*  Process this block  */
    if (ds_get_elements (data, type, stride, values, &complex,
			 (unsigned int) num_left)
	!= TRUE)
    {
	(void) fprintf (stderr, "Error converting residual values\n");
	return (FALSE);
    }
    for (count = 0, val = values; count < num_left; ++count, val += 2)
    {
	/*  Process this value  */
	if (val[0] >= toobig)
	{
	    ++toobig_count;
	    continue;
	}
	if (val[0] > elem_real_max) elem_real_max = val[0];
	if (val[0] < elem_real_min) elem_real_min = val[0];
	if (complex)
	{
	    /*  Complex value  */
	    if (val[1] >= toobig)
	    {
		++toobig_count;
		continue;
	    }
	    abs = sqrt (val[0] * val[0] + val[1] * val[1]);
	    if (abs > elem_abs_max) elem_abs_max = abs;
	    if (abs < elem_abs_min) elem_abs_min = abs;
	    if (val[1] > elem_imag_max) elem_imag_max = val[1];
	    if (val[1] < elem_imag_min) elem_imag_min = val[1];
	}
    }
    return (TRUE);
}   /*  End Function find_array_minmax  */
