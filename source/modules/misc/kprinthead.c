/*  kprinthead.c

    Source file for  kprinthead  (print header in Karma format file).

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

/*  This Karma module will print the header information contained in a Karma
  format file.


    Written by      Richard Gooch   29-MAY-1996

    Updated by      Richard Gooch   15-JUN-1996: Print array descriptor too.

    Last updated by Richard Gooch   6-AUG-1996: Print history information too.


*/
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_dsrw.h>
#include <karma_pio.h>
#include <karma_dmp.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_ex.h>
#include <karma_m.h>
#include <karma_a.h>

#define VERSION "1.0"

STATIC_FUNCTION (flag kprinthead, (char *command, FILE *fp) );
STATIC_FUNCTION (void process_file, (CONST char *infile) );
STATIC_FUNCTION (void process_file_fast, (CONST char *infile) );
STATIC_FUNCTION (void show_vm_structure, (CONST multi_array *multi_desc) );
STATIC_FUNCTION (multi_array *read_karma, (CONST char *infile) );
STATIC_FUNCTION (flag read_packet,
		 (Channel channel, CONST packet_desc *descriptor,
		  char *packet) );


/*  Private data  */
static flag fast = TRUE;


int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "fast", "flag", PIT_FLAG, &fast,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "kprinthead", VERSION, kprinthead, -1, -1, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag kprinthead (char *p, FILE *fp)
{
    char *infile;

    for ( ; p; p = ex_command_skip (p) )
    {
	if ( ( infile = ex_str (p, &p) ) == NULL )
	{
	    fprintf (fp, "Error extracting infile name\n");
	    return (TRUE);
	}
	if (fast) process_file_fast (infile);
	else process_file (infile);
	m_free (infile);
    }
    return (TRUE);
}   /*  End Function kprinthead  */

static void process_file (CONST char *infile)
/*  [SUMMARY] Process an input file.
    <infile> The name of the input file.
    [RETURNS] Nothing.
*/
{
    unsigned int ftype;
    multi_array *multi_desc;
    /*static char function_name[] = "process_file";*/

    /*  Read file in whatever format  */
    if ( ( multi_desc = foreign_guess_and_read (infile,
						K_CH_MAP_IF_AVAILABLE,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	fprintf (stderr, "Error reading\n");
	return;
    }
    show_vm_structure (multi_desc);
    ds_dealloc_multi (multi_desc);
}   /*  End Function process_file  */

static void process_file_fast (CONST char *infile)
/*  [SUMMARY] Process an input file.
    <infile> The name of the input file.
    [RETURNS] Nothing.
*/
{
    Channel channel;
    unsigned int ftype;
    multi_array *multi_desc;
    char *ptr;
    char fname[STRING_LENGTH];
    extern char *sys_errlist[];
    /*static char function_name[] = "process_file_fast";*/

    /*  Determine format  */
    ftype = foreign_guess_format_from_filename (infile);
    if (ftype == FOREIGN_FILE_FORMAT_UNKNOWN) return;
    switch (ftype)
    {
      case FOREIGN_FILE_FORMAT_FITS:
	/*  Read FITS file  */
	if ( ( channel = ch_open_file (infile, "r") ) == NULL )
	{
	    fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		     infile, sys_errlist[errno]);
	    return;
	}
	if ( ( multi_desc =foreign_fits_read_header (channel, FALSE, TRUE,TRUE,
						     FA_FITS_READ_HEADER_END) )
	    == NULL )
	{
	    fprintf (stderr, "Error reading FITS file header\n");
	    ch_close (channel);
	    return;
	}
	ch_close (channel);
	break;
      case FOREIGN_FILE_FORMAT_MIRIAD:
	/*  Read Miriad Image file  */
	sprintf (fname, "%s/header", infile);
	if ( ( channel = ch_open_file (fname, "r") ) == NULL )
	{
	    fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		     fname, sys_errlist[errno]);
	    return;
	}
	if ( ( multi_desc =
	       foreign_miriad_read_header (channel, FALSE, TRUE,
					   FA_MIRIAD_READ_HEADER_END) )
	    == NULL )
	{
	    fprintf (stderr, "Error reading Miriad Image header\n");
	    ch_close (channel);
	    return;
	}
	if ( !foreign_miriad_read_history (infile, multi_desc) )
	{
	    ch_close (channel);
	    return;
	}
	ch_close (channel);
	break;
      case FOREIGN_FILE_FORMAT_GIPSY:
	/*  Read GIPSY file  */
	strcpy (fname, infile);
	if ( ( ptr = strrchr (fname, '.') ) == NULL ) return;
	strcpy (ptr, ".descr");
	if ( ( channel = ch_open_file (fname, "r") ) == NULL )
	{
	    fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		     fname, sys_errlist[errno]);
	    return;
	}
	if ( ( multi_desc =
	       foreign_gipsy_read_header (channel, FALSE, TRUE,
					  FA_GIPSY_READ_HEADER_END) )
	    == NULL )
	{
	    fprintf (stderr, "Error reading Miriad Image header\n");
	    ch_close (channel);
	    return;
	}
	ch_close (channel);
	break;
      case FOREIGN_FILE_FORMAT_KARMA:
	if ( ( multi_desc = read_karma (infile) ) == NULL ) return;
	break;
      default:
	process_file (infile);
	return;
/*
	break;
*/
    }
    show_vm_structure (multi_desc);
    ds_dealloc_multi (multi_desc);
}   /*  End Function process_file_fast  */

static void show_vm_structure (CONST multi_array *multi_desc)
/*  [SUMMARY] Show the header of a structure in virtual memory.
    <multi_desc> The structure.
    [RETURNS] Nothing.
*/
{
    unsigned int count, type;
    packet_desc *top_pack_desc;
    history *hist;
    char *top_packet;

    /* Assume all the interesting information is in the first data structure */
    top_pack_desc = multi_desc->headers[0];
    top_packet = multi_desc->data[0];
    for (count = 0; count < top_pack_desc->num_elements; ++count)
    {
	type = top_pack_desc->element_types[count];
	if (type == K_ARRAY)
	{
	    dmp_array_desc (stdout,
			    (array_desc *) top_pack_desc->element_desc[count],
			    FALSE);
	}
	else if (type == LISTP)
	{
	    puts ("Linked List header");
	}
	else if ( ds_element_is_named (type) )
	{
	    printf ("Element: \"%s\"\t\t",
			   top_pack_desc->element_desc[count]);
	    dmp_element (stdout, type, top_pack_desc->element_desc[count],
			 top_packet +
			 ds_get_element_offset (top_pack_desc, count),
			 FALSE);
	}
	else
	{
	    printf ("Unknown element type: %u\n", type);
	}
    }
    /*  Show history  */
    if (multi_desc->first_hist != NULL) puts ("");
    for (hist = multi_desc->first_hist; hist != NULL; hist = hist->next)
    {
	printf ("HISTORY: %s\n", hist->string);
    }
}   /*  End Function show_vm_structure  */

static multi_array *read_karma (CONST char *infile)
/*  [SUMMARY] Read a Karma file but skip n-dimensional arrays.
    <infile> The name of the input file.
    [RETURNS] Nothing.
*/
{
    Channel channel;
    unsigned int count, num_arrays, length;
    multi_array *multi_desc;
    packet_desc *pack_desc;
    char *string;
    char *data;
    extern char *sys_errlist[];
    static char function_name[] = "read_karma";

    if ( ( channel = ch_open_file (infile, "r") ) == NULL )
    {
	fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		 infile, sys_errlist[errno]);
	return (NULL);
    }
    if ( ( multi_desc = dsrw_read_multi_header (channel) ) == NULL )
	return (NULL);
    num_arrays = multi_desc->num_arrays;
    for (count = 0; count < num_arrays; ++count)
    {
	/*  Load the descriptor for one array   */
        if ( ( pack_desc = dsrw_read_packet_desc (channel) ) == NULL )
        {
	    ds_dealloc_multi (multi_desc);
            return (NULL);
        }
        multi_desc->headers[count] = pack_desc;
	/*  Allocate the required data space   */
	if ( ( data = ds_alloc_data (multi_desc->headers[count],
				     FALSE, FALSE) )
	    == NULL )
        {
	    m_error_notify (function_name, "general data structure");
            ds_dealloc_multi (multi_desc);
            return (NULL);
        }
        multi_desc->data[count] = data;
	/*  Read the data   */
        if ( !read_packet (channel, multi_desc->headers[count],
			   multi_desc->data[count]) )
        {
	    fprintf (stderr, "Error reading array number %u\n", count);
            ds_dealloc_multi (multi_desc);
            return (NULL);
        }
    }
    /*  May need to read arraynames  */
    for (count = 0; (count < num_arrays) && (num_arrays > 1);
	 ++count)
    {
	/*  Load the names of the arrays    */
	if ( ( multi_desc->array_names[count] =
	      pio_read_string (channel, &length) ) == NULL )
        {
	    a_func_abort (function_name, "Error reading array name");
            ds_dealloc_multi (multi_desc);
            return (NULL);
        }
        if (length == 0)
        {
	    a_func_abort (function_name,
			  "Array name must not be a null string");
            ds_dealloc_multi (multi_desc);
            return (NULL);
        }
    }
    /*  Read history strings  */
    if ( ( string = pio_read_string (channel, &length) )
	== NULL )
    {
	a_func_abort (function_name, "Error reading first history string");
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    while (length > 0)
    {
	if ( !ds_history_append_string (multi_desc, string, FALSE) )
	{
	    m_abort (function_name, "history entry");
	}
	if ( ( string = pio_read_string (channel, &length) )
	    == NULL )
	{
	    a_func_abort (function_name, "Error reading history string");
	    ds_dealloc_multi (multi_desc);
	    return (NULL);
	}
    }
    m_free (string);
    return (multi_desc);
}   /*  End Function process_file_karma  */

static flag read_packet (Channel channel, CONST packet_desc *descriptor,
			 char *packet)
/*  [SUMMARY] Read the binary representation of a data packet.
    [PURPOSE] This routine will read the binary representation of a data packet
    from a channel object. The routine will recursively read in sub arrays and
    linked lists.
    <channel> The channel object.
    <descriptor> The descriptor for the packet.
    <packet> The packet data will be written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int count;
    unsigned int type;
    unsigned long pad_bytes, read_pos, write_pos;
    array_desc *arr_desc;
    extern char host_type_sizes[NUMTYPES];
    extern char *sys_errlist[];
/*
    static char function_name[] = "read_packet";
*/

    for (count = 0; count < descriptor->num_elements;
	 ++count, packet += host_type_sizes[type])
    {
	type = descriptor->element_types[count];
	switch (type)
	{
	  case K_ARRAY:
	    /*  Try and skip past the array  */
	    arr_desc = (array_desc *) descriptor->element_desc[count];
	    if ( !ds_packet_all_data (arr_desc->packet) )
	    {
		fprintf (stderr, "Cannot read non-atomic arrays\n");
		return (FALSE);
	    }
	    /*  Read the array padding  */
	    if (arr_desc->padded)
	    {
		if ( !pio_read32 (channel, &pad_bytes) )
		{
		    fprintf (stderr, "Error reading pad size\n");
		    return (FALSE);
		}
		if (ch_drain (channel, pad_bytes) < pad_bytes)
		{
		    fprintf (stderr, "Error reading byte\t%s\n",
			     sys_errlist[errno]);
		    return (FALSE);
		}
	    }
	    /*  Skip past the array  */
	    if ( !ch_tell (channel, &read_pos, &write_pos) )
	    {
		fprintf (stderr, "Error telling\n");
		return (FALSE);
	    }
	    pad_bytes = ds_get_packet_size (arr_desc->packet);
	    read_pos += ds_get_array_size (arr_desc) * pad_bytes;
	    if ( !ch_seek (channel, read_pos) )
	    {
		fprintf (stderr, "Error seeking\n");
		return (FALSE);
	    }
	    break;
	  case LISTP:
	    fprintf (stderr, "Cannot read linked lists\n");
	    return (FALSE);
	    break;
	  default:
	    if ( !dsrw_read_element (channel, type,
				     (char *) descriptor->element_desc[count],
				     packet) )
	    {
		return (FALSE);
	    }
	    break;
	}
    }
    return (TRUE);
}   /*  End Function read_packet  */
