/*  miriad2karma.c

    Source file for  miriad2karma  (module to convert Miriad Image format to
    Karma).

    Copyright (C) 1995-1996  Richard Gooch

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

/*  This Karma module will generate a multi-dimensional array of data from a
    Miriad Image format file and will write the data to a Karma data file.
    The Miriad Image data file must be in network format.


    Written by      Richard Gooch   1-OCT-1995

    Updated by      Richard Gooch   6-MAY-1996: Made use of KMiriadDataContext
  manipulation routines.

    Updated by      Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   28-JUN-1996: Removed distinction between
  blanked and masked data.

    Last updated by Richard Gooch   17-AUG-1996: Supported tiling.


*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_im.h>
#include <karma_m.h>
#include "lib.h"


#define VERSION "3.0"
#define BUF_SIZE 1048576


/*  External functions  */


int main (int argc, char **argv)
{
    KControlPanel panel;
    extern flag sanitise;
    extern flag tile, allow_truncation;
    static char function_name[] = "main";

    im_register_lib_version (KARMA_VERSION);
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "tile", "automatically tile output", PIT_FLAG,
		    &tile,
		    PIA_END);
    panel_add_item (panel, "sanitise", "clean descriptor", PIT_FLAG, &sanitise,
		    PIA_END);
    panel_add_item (panel, "allow_truncation", "shrink axes to allow tiling",
		    PIT_FLAG, &allow_truncation,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "miriad2karma", VERSION, command_parse, -1, 0,
		FALSE);
    return (RV_OK);
}   /*  End Function main   */

void generate_file (char *dirname, char *arrayfile)
/*  [PURPOSE] This routine will read in a Miriad Image file and will convert it
    to a Karma arrayfile with a multi-dimensional array of a single atomic
    element. The data will be converted from the input file.
    <dirname> The name of the input Miriad Image dataset.
    <arrayfile> The name of the Karma arrayfile.
    [RETURNS] Nothing.
*/
{
    Channel miriad_ch;
    Channel karma_ch;
    KMiriadDataContext context;
    unsigned int array_size;
    unsigned int array_count, num_values, block_length;
    unsigned int elem_type, elem_size;
    unsigned long toobig_count;
    unsigned long toobig_count_tmp;
    multi_array *multi_desc;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    uaddr *dim_lengths, *coords;
    char header_file[STRING_LENGTH];
    extern flag sanitise;
    extern flag tile, allow_truncation;
    extern char host_type_sizes[NUMTYPES];
    extern char *sys_errlist[];
    static char *buffer = NULL;
    static char function_name[] = "generate_file";

    if (buffer == NULL)
    {
	if ( ( buffer = m_alloc (BUF_SIZE) ) == NULL )
	{
	    m_abort (function_name, "buffer");
	}
    }
    if ( !foreign_miriad_test (dirname) )
    {
	(void) fprintf (stderr, "\"%s\": not a Miriad Image dataset\n",
			dirname);
	return;
    }
    (void) sprintf (header_file, "%s/header", dirname);
    /*  Try to open header file  */
    if ( ( miriad_ch = ch_open_file (header_file, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			header_file, sys_errlist[errno]);
	return;
    }
    multi_desc = foreign_miriad_read_header (miriad_ch, FALSE, sanitise,
					     FA_MIRIAD_READ_HEADER_END);
    (void) ch_close (miriad_ch);
    if (multi_desc == NULL)
    {
	(void) fprintf (stderr, "Error reading Miriad header\n");
	return;
    }
    foreign_miriad_read_history (dirname, multi_desc);
    /*  Try to open image file  */
    if ( ( context = foreign_miriad_create_data_context (dirname) ) == NULL )
    {
	(void) fprintf (stderr, "Error creating KMiriadDataContext object\n");
	ds_dealloc_multi (multi_desc);
	return;
    }
    /*  Capture some data before the descriptors are fiddled  */
    pack_desc = multi_desc->headers[0];
    arr_desc = (array_desc *) pack_desc->element_desc[0];
    array_size = ds_get_array_size (arr_desc);
    elem_type = arr_desc->packet->element_types[0];
    elem_size = host_type_sizes[elem_type];
    toobig_count = 0;
    if ( !setup_for_writing (multi_desc, tile, allow_truncation, &dim_lengths,
			     &coords, &karma_ch, arrayfile) )
    {
	ds_dealloc_multi (multi_desc);
	return;
    }
    block_length = BUF_SIZE / elem_size;
    if (dim_lengths != NULL)
    {
	/*  Tiling being used: ensure block size is a multiple  */
	if (block_length % dim_lengths[arr_desc->num_dimensions - 1] != 0)
	{
	    block_length -= block_length % dim_lengths[arr_desc->num_dimensions-1];
	}
    }
    for (array_count = 0; array_count < array_size; array_count += num_values)
    {
	num_values = array_size - array_count;
	if (num_values > block_length) num_values = block_length;
	/*  Read a block of data into memory  */
	toobig_count_tmp = 0;
	if ( !foreign_miriad_read_data (context, multi_desc, buffer,num_values,
					FA_MIRIAD_READ_DATA_NUM_BLANKS,
					&toobig_count_tmp,
					FA_MIRIAD_READ_DATA_END) )
	{
	    (void) fprintf (stderr, "Error reading Miriad file\n");
	    foreign_miriad_close_data_context (context);
	    cleanup (karma_ch, dim_lengths, coords);
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	toobig_count += toobig_count_tmp;
	/*  Write a block of data out  */
	if ( !write_blocks (karma_ch, multi_desc, dim_lengths, coords, buffer,
			    num_values) )
	{
	    fprintf (stderr, "Error writing Karma file\n");
	    foreign_miriad_close_data_context (context);
	    cleanup (karma_ch, dim_lengths, coords);
	    ds_dealloc_multi (multi_desc);
	    return;
	}
    }
    foreign_miriad_close_data_context (context);
    /*  Array should be copied by now: write rest of top level packet  */
    if ( !write_tail (karma_ch, multi_desc, dirname, arrayfile) )
    {
	ds_dealloc_multi (multi_desc);
	cleanup (karma_ch, dim_lengths, coords);
	return;
    }
    cleanup (karma_ch, dim_lengths, coords);
    ds_dealloc_multi (multi_desc);
    if (toobig_count > 0)
    {
	fprintf (stderr, "Number of blank values: %lu\n", toobig_count);
    }
}   /*  End Function generate_file  */


/*  Private functions follow  */


/*  Put globals here to force functions to be explicit  */
flag sanitise = TRUE;
char *default_extension = ".kf";
flag tile = TRUE;
flag allow_truncation = TRUE;
