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
    The Miriad Image data file must be in network format (as per the FITS
    standard).


    Written by      Richard Gooch   1-OCT-1995

    Updated by      Richard Gooch   6-MAY-1996: Made use of KMiriadDataContext
  manipulation routines.

    Updated by      Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   28-JUN-1996: Removed distinction between
  blanked and masked data.


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
#include <karma_dsrw.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_im.h>
#include <karma_m.h>
#include <karma_a.h>


#define VERSION "2.0"
#define BUF_SIZE 1048576

/*  Copied from  lib/karma/level4/dsrw.c  */
#define MAGIC_STRING_LENGTH (unsigned int) 16
#define VERSION_NUMBER (unsigned long) 0
#define ARRAY_BOUNDARY (unsigned int) 16
static char magic_string[] = "KarmaRHD Version";


STATIC_FUNCTION (flag miriad2karma, (char *command, FILE *fp) );
STATIC_FUNCTION (void generate_file,
		 (CONST char *dirname, CONST char *arrayfile) );
STATIC_FUNCTION (char *convert_object_to_filename, (CONST char *object_name) );
STATIC_FUNCTION (Channel open_file, (CONST char *arrayfile) );


int main (argc, argv)
int argc;       /*  Count of parameters on command line */
char **argv;    /*  List of command line parameters     */
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
    module_run (argc, argv, "miriad2karma", VERSION, miriad2karma, -1, 0,
		FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag miriad2karma (char *p, FILE *fp)
{
    char *arrayfile;
    char *input_filename;

    for ( ; p; p = ex_word_skip (p) )
    {
	if ( ( input_filename = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (stderr, "Error extracting input filename\n");
	    return (TRUE);
	}
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (stderr, "Error extracting arrayfile name\n");
	    m_free (input_filename);
	    return (TRUE);
	}
	generate_file (input_filename, arrayfile);
	m_free (input_filename);
	m_free (arrayfile);
    }
    return (TRUE);
}   /*  End Function miriad2karma  */

static void generate_file (CONST char *dirname, CONST char *arrayfile)
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
    unsigned int array_count, num_values;
    unsigned int elem_type, elem_size;
    unsigned int element_count;
    unsigned int bytes_to_pad;
    unsigned long read_pos;
    unsigned long write_pos;
    unsigned long toobig_count;
    unsigned long toobig_count_tmp;
    char padding[ARRAY_BOUNDARY];
    char header_file[STRING_LENGTH];
    char *packet, *ptr;
    multi_array *multi_desc;
    packet_desc *pack_desc;
    array_desc *arr_desc;
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
    /*  Try to open image file  */
    if ( ( context = foreign_miriad_create_data_context (dirname) ) == NULL )
    {
	(void) fprintf (stderr, "Error creating KMiriadDataContext object\n");
	ds_dealloc_multi (multi_desc);
	return;
    }
    pack_desc = multi_desc->headers[0];
    arr_desc = (array_desc *) pack_desc->element_desc[0];
    array_size = ds_get_array_size (arr_desc);
    elem_type = arr_desc->packet->element_types[0];
    elem_size = host_type_sizes[elem_type];
    toobig_count = 0;
    /*  Now write the Karma descriptors  */
    if ( ( karma_ch = open_file (arrayfile) ) == NULL )
    {
	(void) fprintf (stderr, "Error opening Karma arrayfile file\n");
	foreign_miriad_close_data_context (context);
	return;
    }
    /*  Write magic string and version number  */
    if (ch_write (karma_ch, magic_string, MAGIC_STRING_LENGTH)
	< MAGIC_STRING_LENGTH)
    {
	(void) fprintf (stderr, "Error writing magic string\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    if ( !pio_write32 (karma_ch, VERSION_NUMBER) ) exit (RV_WRITE_ERROR);
    /*  Write number of data structures  */
    if ( !pio_write32 (karma_ch, (unsigned long) 1) ) exit (RV_WRITE_ERROR);
    dsrw_write_packet_desc (karma_ch, multi_desc->headers[0]);
    /*  Pad  */
    /*  Clear padding array (to avoid random file differences)  */
    m_clear (padding, ARRAY_BOUNDARY);
    if ( !ch_tell (karma_ch, &read_pos, &write_pos) )
    {
	(void) fprintf (stderr, "Error getting channel position\n");
	exit (RV_UNDEF_ERROR);
    }
    /*  Add 4 bytes for pad size  */
    write_pos += 4;
    bytes_to_pad = ARRAY_BOUNDARY - write_pos % ARRAY_BOUNDARY;
    if ( !pio_write32 (karma_ch, (unsigned long) bytes_to_pad) )
    {
	(void) fprintf (stderr, "Error writing pad size\n");
	exit (RV_WRITE_ERROR);
    }
    if (bytes_to_pad > 0)
    {
	if (ch_write (karma_ch, padding, bytes_to_pad) < bytes_to_pad)
	{
	    (void) fprintf (stderr, "Error padding: %u bytes\t%s\n",
			    bytes_to_pad, sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
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
	    (void) fprintf (stderr, "Error reading Miriad file\n");
	    foreign_miriad_close_data_context (context);
	    (void) ch_close (karma_ch);
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	toobig_count += toobig_count_tmp;
	/*  Write a block of data out  */
	if ( ds_can_transfer_element_as_block (elem_type) )
	{
	    if (ch_write (karma_ch, buffer, num_values * elem_size) <
		num_values * elem_size)
	    {
		(void) fprintf (stderr, "Error writing Karma file\n");
		foreign_miriad_close_data_context (context);
		(void) ch_close (karma_ch);
		ds_dealloc_multi (multi_desc);
		return;
	    }
	    continue;
	}
	/*  Write elements one at a time  */
	for (element_count = 0, ptr = buffer; element_count < num_values;
	     ++element_count, ptr += elem_size)
	{
	    dsrw_write_element (karma_ch, elem_type,
				pack_desc->element_desc[0], ptr);
	}
    }
    foreign_miriad_close_data_context (context);
    /*  Array should be copied by now: write rest of top level packet  */
    packet = multi_desc->data[0] + host_type_sizes[K_ARRAY];
    for (element_count = 1; element_count < pack_desc->num_elements;
	 ++element_count)
    {
	/*  Write element   */
        elem_type = pack_desc->element_types[element_count];
        dsrw_write_element (karma_ch, elem_type,
			    pack_desc->element_desc[element_count],
			    packet);
        packet += host_type_sizes[elem_type];
    }
    /*  Write dummy history  */
    if ( !pio_write_string (karma_ch, (char *) NULL) )
    {
	(void) fprintf (stderr, "Error writing NULL history string\n");
	exit (RV_WRITE_ERROR);
    }
    if ( !ch_close (karma_ch) )
    {
	(void) fprintf (stderr, "Error closing channel\n");
	ds_dealloc_multi (multi_desc);
	return;
    }
    ds_dealloc_multi (multi_desc);
    if (toobig_count > 0)
    {
	(void) fprintf (stderr, "Number of blank values: %lu\n", toobig_count);
    }
}   /*  End Function generate_file  */

static char *convert_object_to_filename (CONST char *object_name)
/*  This routine will convert an object name to a Unix filename. This involves
    appending the default extension name to the object name. If the object name
    already contains the default extension name, no change is made.
    The object name must be pointed to by  object_name  .
    The routine returns a pointer to a dynamically allocated filename on
    success, else it returns NULL.
*/
{
    int name_length;
    int ext_length;
    char *filename;
    extern char *default_extension;
    static char function_name[] = "convert_object_to_filename";

    name_length = strlen (object_name);
    ext_length = strlen (default_extension);
    if (name_length >= ext_length)
    {
	if (strcmp (object_name + name_length - ext_length, default_extension)
	    == 0)
	{
	    /*  Extension already there  */
	    return ( st_dup (object_name) );
	}
    }
    /*  Must append default extension  */
    if ( ( filename = m_alloc (name_length + ext_length + 1) ) == NULL )
    {
	m_error_notify (function_name, "filename");
	return (NULL);
    }
    (void) strcpy (filename, object_name);
    (void) strcat (filename, default_extension);
    return (filename);
}   /*  End Function convert_object_to_filename  */

static Channel open_file (CONST char *arrayfile)
/*  This routine will open an arrayfile for writing.
    The arrayfile name must be pointed to by  arrayfile  .
    The routine returns a channel object on success, else it returns NULL.
*/
{
    Channel channel;
    flag rename_file;
    char *filename;
    char *tilde_filename;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "open_file";

    if ( ( filename = convert_object_to_filename (arrayfile) ) == NULL )
    {
	m_error_notify (function_name, "filename");
	return (NULL);
    }
    if (access (filename, F_OK) == 0)
    {
	/*  Old file exists  */
	rename_file = TRUE;
    }
    else
    {
	/*  Error accessing old file  */
	if (errno != ENOENT)
	{
	    /*  Error, not simply missing file  */
	    (void) fprintf (stderr, "Error accessing file: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	    m_free (filename);
	    return (NULL);
	}
	/*  File does not exist  */
	rename_file = FALSE;
    }
    if (rename_file)
    {
	if ( ( tilde_filename = m_alloc (strlen (filename) + 2) ) == NULL )
	{
	    m_error_notify (function_name, "tilde filename");
	    m_free (filename);
	    return (NULL);
	}
	(void) strcpy (tilde_filename, filename);
	(void) strcat (tilde_filename, "~");
	if (rename (filename, tilde_filename) != 0)
	{
	    (void) fprintf (stderr, "Error renaming file: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	    m_free (tilde_filename);
	    m_free (filename);
	    return (NULL);
	}
	m_free (tilde_filename);
    }
    if ( ( channel = ch_open_file (filename, "w") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening file: \"%s\" for output\t%s\n",
			filename, sys_errlist[errno]);
	m_free (filename);
	return (NULL);
    }
    m_free (filename);
    return (channel);
}   /*  End Function open_file  */


/*  Put globals here to force functions to be explicit  */
flag sanitise = TRUE;
char *default_extension = ".kf";
