/*  tile1.c

    Source file for  tile1  (retiling module).

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

/*  This Karma module will retile an array.
    The changes are written to a new Karma arrayfile.


    Written by      Richard Gooch   30-JAN-1993

    Updated by      Richard Gooch   30-JAN-1993

    Updated by      Richard Gooch   28-MAR-1993: Took account of changes to
  dsproc_object  function.

    Updated by      Richard Gooch   16-NOV-1993: Changed over to  panel_
  package for command line user interface and moved  main  into this file.

    Updated by      Richard Gooch   23-NOV-1993: Fixed bug in call to
  panel_add_item  for  array_names  parameter.

    Updated by      Richard Gooch   1-JUN-1996: Cleaned code to keep
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

#define MAX_LENGTHS 10
#define NUMARRAYS 100

EXTERN_FUNCTION (flag tile1, (char *command, FILE *fp) );

flag pre_process ();
flag process_array ();
STATIC_FUNCTION (flag post_process,
		 (CONST multi_array *inp_desc, multi_array *out_desc) );
packet_desc *generate_desc ();
flag process_occurrence ();


static int save_unproc = TRUE;

static char *array_names[NUMARRAYS];
static int num_arrays = 0;

static int tile_lengths[MAX_LENGTHS];
static int num_lengths = 0;

static char *dim_name = NULL;


int main (int argc, char **argv)
{
    KControlPanel panel;
    extern int tile_lengths[MAX_LENGTHS];
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
    panel_add_item (panel, "tile_lengths", "bottom tile lengths", K_INT,
		    tile_lengths,
		    PIA_ARRAY_LENGTH, &num_lengths,
		    PIA_ARRAY_MAX_LENGTH, MAX_LENGTHS,
		    PIA_END);
    panel_add_item (panel, "save_unproc_data", "", PIT_FLAG, &save_unproc,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "tile1", VERSION, tile1, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag tile1 (char *p, FILE *fp)
{
    char *arrayfile;
    extern int save_unproc;
    extern int num_arrays;
    extern char *dim_name;
    extern char *array_names[NUMARRAYS];
    /*static char function_name[] = "tile1";*/

    for ( ; p; p = ex_word_skip (p) )
    {
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting arrayfile name\n");
	    return (TRUE);
	}
	if ( ( dim_name = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp,
			    "Must specify dimension name of array to tile\n");
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
}   /*  End Function tile1  */

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
/*  This routine will process a general data structure, tiling each occurrence
    of an array with a dimension name in the external string  dim_name  .
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
	(void) fprintf (stderr, "NULL descriptor pointer(s)\n");
        a_prog_bug (function_name);
    }
    if ( (inp_data == NULL) || (out_data == NULL) )
    {
	(void) fprintf (stderr, "NULL data pointer(s)\n");
	a_prog_bug (function_name);
    }
    if (dim_name == NULL)
    {
	(void) fprintf (stderr,
			"NULL external variable dim name pointer\n");
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
    (void) ds_copy_data (inp_desc, inp_data, *out_desc, *out_data);
    return (TRUE);
}   /*  End Function process_array  */

packet_desc *generate_desc (inp_desc)
/*  This routine will generate a general data structure descriptor for
    the output of a tiling programme.
    The input descriptor, which is partly copied, must be pointed to by
    inp_desc  .
    The input descriptor must contain (at any depth) a dimension of name
    pointed to by the external variable  dim_name  .
    The type of the element is changed to the value in the external variable
    type  .
    The routine returns a pointer to the output descriptor on success, else
    it returns NULL.
*/
packet_desc *inp_desc;
{
    unsigned int dim_index;
    unsigned int dim_num;
    packet_desc *return_value;
    array_desc *arr_desc;
    dim_desc *dim;
    extern int num_lengths;
    extern int tile_lengths[MAX_LENGTHS];
    extern char *dim_name;
    static char function_name[] = "generate_desc";

    /*  Do tests on input descriptor    */
    switch ( ds_f_name_in_packet (inp_desc, dim_name,
				  (char **) &arr_desc, &dim_index) )
    {
      case IDENT_NOT_FOUND:
	(void) fprintf (stderr,
			"Dimension: \"%s\" not found\n", dim_name);
	return (NULL);
/*
	break;
*/
      case IDENT_ELEMENT:
	(void) fprintf (stderr,
			"Item: \"%s\" must be a dimension, not an element",
			dim_name);
	return (NULL);
/*
	break;
*/
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Item \"%s\" found more than once\n", dim_name);
	return (NULL);
/*
	break;
*/
      case IDENT_DIMENSION:
	break;
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"Bad return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    if ( arr_desc->num_dimensions < 2 )
    {
	(void) fprintf (stderr, "Array must have at least 2 dimensions\n");
	return (NULL);
    }
    if (num_lengths != arr_desc->num_dimensions)
    {
	(void) fprintf (stderr,
			"Array has: %u dimensions but: %u lengths specified\n",
			arr_desc->num_dimensions, num_lengths);
	return (NULL);
    }
    /*  Check if tiling OK  */
    for (dim_num = 0; dim_num < arr_desc->num_dimensions; ++dim_num)
    {
	dim = arr_desc->dimensions[dim_num];
	if (dim->length % tile_lengths[dim_num] != 0)
	{
	    (void) fprintf (stderr,
			    "Dimension: \"%s\" length: %lu is not divisible by tile length: %u\n",
			    dim->name, dim->length, tile_lengths[dim_num]);
	    return (NULL);
	}
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
			"Output dimension: \"%s\" not found\n", dim_name);
	ds_dealloc_packet (return_value, (char *) NULL);
	return (NULL);
/*
	break;
*/
      case IDENT_ELEMENT:
	(void) fprintf (stderr,
			"Output item: \"%s\" must be a dimension, not an element",
			dim_name);
	ds_dealloc_packet (return_value, (char *) NULL);
	return (NULL);
/*
	break;
*/
      case IDENT_MULTIPLE:
	(void) fprintf (stderr,
			"Output item \"%s\" found more than once\n", dim_name);
	ds_dealloc_packet (return_value, (char *) NULL);
	return (NULL);
/*
	break;
*/
      case IDENT_DIMENSION:
	break;
      case IDENT_GEN_STRUCT:
      default:
	(void) fprintf (stderr,
			"Bad return value from function: ds_f_name_in_packet\n");
	a_prog_bug (function_name);
	break;
    }
    if (dim_num != dim_index)
    {
	(void) fprintf (stderr,
			"Dimension index has changed from: %u to: %u\n",
			dim_index, dim_num);
	a_prog_bug (function_name);
    }
    /*  Remove any tiling information  */
    ds_remove_tiling_info (arr_desc);
    if (ds_alloc_tiling_info (arr_desc, 1) != TRUE)
    {
	m_error_notify (function_name, "tiling information");
	ds_dealloc_packet (return_value, (char *) NULL);
	return (NULL);
    }
    for (dim_num = 0; dim_num < arr_desc->num_dimensions; ++dim_num)
    {
	dim = arr_desc->dimensions[dim_num];
	arr_desc->lengths[dim_num] = tile_lengths[dim_num];
	arr_desc->tile_lengths[dim_num][0] = dim->length / tile_lengths[dim_num];
    }
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
    extern char *dim_name;
    extern char module_name[STRING_LENGTH + 1];
    /*static char function_name[] = "post_process";*/

    if ( !ds_history_copy (out_desc, inp_desc) ) return (FALSE);
    if ( !panel_put_history_with_stack (out_desc, TRUE) ) return (FALSE);
    sprintf (txt, "%s: Name of dimension: \"%s\"", module_name, dim_name);
    return ds_history_append_string (out_desc, txt, TRUE);
}   /*  End Function post_process   */
