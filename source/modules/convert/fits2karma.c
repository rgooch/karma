/*  fits2karma.c

    Source file for  fits2karma  (module to convert FITS format to Karma).

    Copyright (C) 1992-1996  Richard Gooch

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
    FITS format file and will write the data to a Karma data file.
    The FITS data file must be in network format (as per the FITS standard).


    Written by      Richard Gooch   4-NOV-1992

    Updated by      Richard Gooch   5-NOV-1992

    updated by      Richard Gooch   3-JAN-1993: Took acccount of change to
  dsrw_read_array  .

    Updated by      Richard Gooch   15-FEB-1993: Disabled memory mapped arrays.

    Updated by      Richard Gooch   18-MAR-1993: Added  sanitise  option
  (remove dimensions of length 1).

    Updated by      Richard Gooch   20-MAR-1993: Added stripping of trailing
  whitespace in converted strings to  sanitise  option.

    Updated by      Richard Gooch   13-JUL-1993: Added protection for absent
  dimension names ("CTYPE" FITS parameter).

    Updated by      Richard Gooch   18-JUL-1993: Added test for GNU C.

    Updated by      Richard Gooch   24-JUL-1993: Changed to  ch_open_file  .

    Updated by      Richard Gooch   29-JUL-1993: Fixed bug with blank value
  "BLANK".

    Updated by      Richard Gooch   6-OCT-1993: Changed over to  panel_
  package for command line user interface and moved  main  into this file.

    Updated by      Richard Gooch   16-NOV-1993: Added copying of unknown
  keywords into Karma data structure.

    Updated by      Richard Gooch   23-NOV-1993: Added  #ifndef  for Convex
  around #include <nan.h>

    Updated by      Richard Gooch   23-MAR-1994: Added  convert_to_float
  parameter and changed from creating the entire output array in virtual
  memory to using small buffers.

    Updated by      Richard Gooch   3-APR-1994: Fixed previous change.

    Updated by      Richard Gooch   20-APR-1994: Trapped axis parameters with
  illegal dimension index and now recognise COMMENT keyword.

    Updated by      Richard Gooch   21-APR-1994: Tolerate missing BUNIT
  keyword.

    Updated by      Richard Gooch   8-MAY-1994: Fixed writing of data values
  for header keywords

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   6-JUL-1994: Added  #ifdef  for OSF1  test
  for  isnan  .

    Updated by      Richard Gooch   14-OCT-1994: Allow preceeding '.' character
  in floating point keyword values.

    Updated by      Richard Gooch   28-OCT-1994: Default BZERO to 0.0 and
  BSCALE to 1.0 if not specified.

    Updated by      Richard Gooch   3-NOV-1994: Fixed declaration of
  alloc_descriptor  .

    Updated by      Richard Gooch   29-NOV-1994: Default to "Data Value"
  if BUNIT is an empty string.

    Updated by      Richard Gooch   15-APR-1995: Added #include <karma_dsrw.h>

    Updated by      Richard Gooch   18-APR-1995: Made use of
  <foreign_fits_read_header>.

    Updated by      Richard Gooch   21-APR-1995: Fixed bug when writing data in
  blocks.

    Updated by      Richard Gooch   17-MAY-1995: Made use of
  FA_FITS_READ_DATA_NUM_BLANKS attribute.

    Updated by      Richard Gooch   1-OCT-1995: Cosmetic changes.

    Updated by      Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   6-AUG-1996: Now write history.

    Last updated by Richard Gooch   17-AUG-1996: Supported tiling.


*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
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

#define CARD_WIDTH 80
#define CARD_LENGTH 36


/*  External functions  */


int main (int argc, char **argv)
{
    KControlPanel panel;
    extern flag ignore_excess;
    extern flag sanitise;
    extern flag convert_to_float;
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
    panel_add_item (panel, "ignore_excess", "ignore excess data at end",
		    PIT_FLAG, &ignore_excess,
		    PIA_END);
    panel_add_item (panel, "convert_to_float",
		    "convert data to floating point", PIT_FLAG,
		    &convert_to_float,
		    PIA_END);
    panel_add_item (panel, "allow_truncation", "shrink axes to allow tiling",
		    PIT_FLAG, &allow_truncation,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "fits2karma", VERSION, command_parse, -1, 0,
		FALSE);
    return (RV_OK);
}   /*  End Function main   */

void generate_file (char *fits_filename, char *arrayfile)
/*  This routine will read in a FITS file and will convert it to a Karma
    arrayfile with a multi-dimensional array of a single atomic element.
    The data will be converted from the input file.
    The name of the input file must be pointed to by  fits_filename  .
    the name of the Karma arrayfile must be pointed to by  arrayfile  .
    The routine returns nothing.
*/
{
    Channel fits_ch;
    Channel karma_ch;
    unsigned int array_size;
    unsigned int array_count, num_values, block_length;
    unsigned int elem_type, elem_size;
    unsigned long toobig_count;
    unsigned long toobig_count_tmp;
    struct stat stat_buf;
    multi_array *multi_desc;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    uaddr *dim_lengths, *coords;
    extern flag convert_to_float;
/*
    extern flag ignore_excess;
*/
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
    /*  Get stats on FITS file  */
    if (stat (fits_filename, &stat_buf) != 0)
    {
	fprintf (stderr, "Error getting stats on file: \"%s\"\t%s\n",
		 fits_filename, sys_errlist[errno]);
	return;
    }
    /*  Compare with specified array size  */
    if (stat_buf.st_size % (CARD_WIDTH * CARD_LENGTH) != 0)
    {
	fprintf (stderr, "File: \"%s\" is not an integral number of cards\n",
		 fits_filename);
	return;
    }
    /*  Try to open input file  */
    if ( ( fits_ch = ch_open_file (fits_filename, "r") ) == NULL )
    {
	fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		 fits_filename, sys_errlist[errno]);
	return;
    }
    if ( ( multi_desc = foreign_fits_read_header (fits_ch, FALSE,
						  convert_to_float, sanitise,
						  FA_FITS_READ_HEADER_END) )
	== NULL )
    {
	fprintf (stderr, "Error reading FITS header\n");
	ch_close (fits_ch);
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
	if ( !foreign_fits_read_data (fits_ch, multi_desc, buffer, num_values,
				      FA_FITS_READ_DATA_NUM_BLANKS,
				      &toobig_count_tmp,
				      FA_FITS_READ_DATA_END) )
	{
	    fprintf (stderr, "Error reading FITS file\n");
	    ch_close (fits_ch);
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
	    ch_close (fits_ch);
	    cleanup (karma_ch, dim_lengths, coords);
	    ds_dealloc_multi (multi_desc);
	    return;
	}
    }
    ch_close (fits_ch);
    /*  Array should be copied by now: write rest of top level packet  */
    if ( !write_tail (karma_ch, multi_desc, fits_filename, arrayfile) )
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
flag ignore_excess = FALSE;
flag sanitise = TRUE;
flag convert_to_float = TRUE;
char *default_extension = ".kf";
flag tile = TRUE;
flag allow_truncation = TRUE;
