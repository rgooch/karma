/*  miriad2gipsy.c

    Source file for  miriad2gipsy  (module to convert Miriad Image format to
    GIPSY).

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
    Miriad Image format file and will write the data to a GIPSY data file.
    The Miriad Image data file must be in network format (as per the FITS
    standard).


    Written by      Richard Gooch   8-AUG-1996: Copied from miriad2karma.c

    Last updated by Richard Gooch   8-AUG-1996


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
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_im.h>
#include <karma_m.h>
#include <karma_a.h>


#define VERSION "1.0"
#define BUF_SIZE 1048576



STATIC_FUNCTION (flag miriad2gipsy, (char *command, FILE *fp) );
STATIC_FUNCTION (void generate_file,
		 (CONST char *dirname, CONST char *gipsy_filename) );


int main (int argc, char **argv)
{
    KControlPanel panel;
    extern flag sanitise;
    static char function_name[] = "main";

    im_register_lib_version (KARMA_VERSION);
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "sanitise", "flag", PIT_FLAG, &sanitise,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "miriad2gipsy", VERSION, miriad2gipsy, -1, 0,
		FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag miriad2gipsy (char *p, FILE *fp)
{
    char *gipsy_filename;
    char *input_filename;

    for ( ; p; p = ex_word_skip (p) )
    {
	if ( ( input_filename = ex_str (p, &p) ) == NULL )
	{
	    fprintf (stderr, "Error extracting input filename\n");
	    return (TRUE);
	}
	if ( ( gipsy_filename = ex_str (p, &p) ) == NULL )
	{
	    fprintf (stderr, "Error extracting gipsy_filename name\n");
	    m_free (input_filename);
	    return (TRUE);
	}
	generate_file (input_filename, gipsy_filename);
	m_free (input_filename);
	m_free (gipsy_filename);
    }
    return (TRUE);
}   /*  End Function miriad2gipsy  */

static void generate_file (CONST char *dirname, CONST char *gipsy_filename)
/*  [PURPOSE] This routine will read in a Miriad Image file and will convert it
    to a GIPSY file. The data will be converted from the input file.
    <dirname> The name of the input Miriad Image dataset.
    <gipsy_filename> The basename of the GIPSY file.
    [RETURNS] Nothing.
*/
{
    Channel channel;
    KMiriadDataContext context;
    unsigned int array_size;
    unsigned int array_count, num_values;
    unsigned int elem_type, elem_size;
    unsigned long toobig_count;
    unsigned long toobig_count_tmp;
    char *header_packet;
    multi_array *multi_desc;
    packet_desc *pack_desc, *header_pack_desc;
    array_desc *arr_desc;
    char header_file[STRING_LENGTH];
    char txt[STRING_LENGTH];
    extern flag sanitise;
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
	fprintf (stderr, "\"%s\": not a Miriad Image dataset\n",
			dirname);
	return;
    }
    sprintf (header_file, "%s/header", dirname);
    /*  Try to open header file  */
    if ( ( channel = ch_open_file (header_file, "r") ) == NULL )
    {
	fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			header_file, sys_errlist[errno]);
	return;
    }
    multi_desc = foreign_miriad_read_header (channel, FALSE, sanitise,
					     FA_MIRIAD_READ_HEADER_END);
    ch_close (channel);
    if (multi_desc == NULL)
    {
	fprintf (stderr, "Error reading Miriad header\n");
	return;
    }
    /*  Create FITS-style header  */
    if ( !foreign_fits_generate_header (&header_pack_desc, &header_packet,
					multi_desc,
					FA_FITS_GENERATE_HEADER_END) )
    {
	ds_dealloc_multi (multi_desc);
	return;
    }
    /*  Try to open image file  */
    if ( ( context = foreign_miriad_create_data_context (dirname) ) == NULL )
    {
	fprintf (stderr, "Error creating KMiriadDataContext object\n");
	ds_dealloc_packet (header_pack_desc, header_packet);
	ds_dealloc_multi (multi_desc);
	return;
    }
    pack_desc = multi_desc->headers[0];
    arr_desc = (array_desc *) pack_desc->element_desc[0];
    array_size = ds_get_array_size (arr_desc);
    elem_type = arr_desc->packet->element_types[0];
    elem_size = host_type_sizes[elem_type];
    toobig_count = 0;
    /*  Write the GIPSY header  */
    sprintf (txt, "%s.descr", gipsy_filename);
    if ( ( channel = ch_open_file (txt, "w") ) == NULL )
    {
	fprintf (stderr, "Error opening GIPSY header file\t%s\n",
		 sys_errlist[errno]);
	ch_close (channel);
	foreign_miriad_close_data_context (context);
	ds_dealloc_packet (header_pack_desc, header_packet);
	ds_dealloc_multi (multi_desc);
	return;
    }
    if ( !foreign_gipsy_write_header (channel,
				      header_pack_desc, header_packet,
				      FA_GIPSY_WRITE_HEADER_END) )
    {
	ch_close (channel);
	foreign_miriad_close_data_context (context);
	ds_dealloc_packet (header_pack_desc, header_packet);
	ds_dealloc_multi (multi_desc);
	return;
    }
    if ( !ch_close (channel) )
    {
	foreign_miriad_close_data_context (context);
	ds_dealloc_packet (header_pack_desc, header_packet);
	ds_dealloc_multi (multi_desc);
	return;
    }
    /*  Create the GIPSY image file  */
    sprintf (txt, "%s.image", gipsy_filename);
    if ( ( channel = ch_open_file (txt, "w") ) == NULL )
    {
	fprintf (stderr, "Error opening GIPSY data file\t%s\n",
		 sys_errlist[errno]);
	ch_close (channel);
	foreign_miriad_close_data_context (context);
	ds_dealloc_packet (header_pack_desc, header_packet);
	ds_dealloc_multi (multi_desc);
	return;
    }
    /*  Start converting data. Do it in blocks so that we don't have to
	allocate possibly huge amounts of virtual memory  */
    for (array_count = 0; array_count < array_size; array_count += num_values)
    {
	num_values = array_size - array_count;
	if (num_values *elem_size > BUF_SIZE) num_values = BUF_SIZE /elem_size;
	/*  Read a block of data into memory  */
	toobig_count_tmp = 0;
	if ( !foreign_miriad_read_data (context, multi_desc, buffer,num_values,
					FA_MIRIAD_READ_DATA_NUM_BLANKS,
					&toobig_count_tmp,
					FA_MIRIAD_READ_DATA_END) )
	{
	    fprintf (stderr, "Error reading Miriad file\n");
	    foreign_miriad_close_data_context (context);
	    ch_close (channel);
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	toobig_count += toobig_count_tmp;
	/*  Write a block of data out  */
	if ( !foreign_gipsy_write_data (channel, multi_desc,
					header_pack_desc, header_packet,
					buffer, num_values,
					FA_GIPSY_WRITE_DATA_END) )
	{
	    fprintf (stderr, "Error writing Karma file\n");
	    foreign_miriad_close_data_context (context);
	    ds_dealloc_packet (header_pack_desc, header_packet);
	    ch_close (channel);
	    ds_dealloc_multi (multi_desc);
	    return;
	}
    }
    foreign_miriad_close_data_context (context);
    ds_dealloc_packet (header_pack_desc, header_packet);
    ch_close (channel);
    ds_dealloc_multi (multi_desc);
    if (toobig_count > 0)
    {
	fprintf (stderr, "Number of blank values: %lu\n", toobig_count);
    }
}   /*  End Function generate_file  */


/*  Put globals here to force functions to be explicit  */
flag sanitise = TRUE;
