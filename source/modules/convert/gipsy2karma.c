/*  gipsy2karma.c

    Source file for  gipsy2karma  (module to convert GIPSY format to Karma).

    Copyright (C) 1996  Richard Gooch

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
    GIPSY format file and will write the data to a Karma data file.


    Written by      Richard Gooch   7-AUG-1996: Copied from fits2karma.c

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


#define VERSION "2.0"
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
    module_run (argc, argv, "gipsy2karma", VERSION, command_parse, -1, 0,
		FALSE);
    return (RV_OK);
}   /*  End Function main   */

void generate_file (char *gipsy_filename, char *arrayfile)
/*  [PURPOSE] This routine will read in a GIPSY file and will convert it
    to a Karma arrayfile with a multi-dimensional array of a single atomic
    element. The data will be converted from the input file.
    <gipsy_filename> The name of the GIPSY file.
    <arrayfile> The name of the Karma arrayfile.
    [RETURNS] Nothing.
*/
{
    Channel gipsy_ch;
    Channel karma_ch;
    unsigned int array_size;
    unsigned int array_count, num_values, block_length;
    unsigned int elem_type, elem_size;
    unsigned long toobig_count;
    unsigned long toobig_count_tmp;
    multi_array *multi_desc;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    uaddr *dim_lengths, *coords;
    char *ptr;
    char fname[STRING_LENGTH];
    char header_name[STRING_LENGTH];
    char image_name[STRING_LENGTH];
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
    if ( !foreign_gipsy_test (gipsy_filename) ) return;
    strcpy (fname, gipsy_filename);
    if ( ( ptr = strrchr (fname, '.') ) == NULL ) return;
    *ptr = '\0';
    if ( (strcmp (ptr + 1, "descr") != 0) &&
	 (strcmp (ptr + 1, "image") != 0) &&
	 (strcmp (ptr + 1, "gipsy") != 0) )
    {
	return;
    }
    sprintf (header_name, "%s.descr", fname);
    sprintf (image_name, "%s.image", fname);
    if ( ( gipsy_ch = ch_open_file (header_name, "r") ) == NULL )
    {
	fprintf (stderr, "Error opening: \"%s\"\t%s\n",
		 header_name, sys_errlist[errno]);
	return;
    }
    multi_desc = foreign_gipsy_read_header (gipsy_ch, FALSE, sanitise,
					    FA_GIPSY_READ_HEADER_END);
    ch_close (gipsy_ch);
    if (multi_desc == NULL)
    {
	fprintf (stderr, "Error reading GIPSY header\n");
	return;
    }
    /*  Try to open image file  */
    if ( ( gipsy_ch = ch_open_file (image_name, "r") ) == NULL )
    {
	fprintf (stderr, "Error opening: \"%s\"\t%s\n",
		 image_name, sys_errlist[errno]);
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
	if ( !foreign_gipsy_read_data (gipsy_ch, multi_desc,
				       buffer, num_values,
				       FA_GIPSY_READ_DATA_NUM_BLANKS,
				       &toobig_count_tmp,
				       FA_GIPSY_READ_DATA_END) )
	{
	    fprintf (stderr, "Error reading GIPSY file\n");
	    ch_close (gipsy_ch);
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
	    ch_close (gipsy_ch);
	    cleanup (karma_ch, dim_lengths, coords);
	    ds_dealloc_multi (multi_desc);
	    return;
	}
    }
    ch_close (gipsy_ch);
    /*  Array should be copied by now: write rest of top level packet  */
    if ( !write_tail (karma_ch, multi_desc, gipsy_filename, arrayfile) )
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
