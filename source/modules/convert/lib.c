/*  lib.c

    Support source file for various conversion modules.

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

/*  This file contains support source code for various conversion modules.


    Written by      Richard Gooch   16-AUG-1996

    Updated by      Richard Gooch   17-AUG-1996

    Last updated by Richard Gooch   27-SEP-1996: Do not write empty history
  strings otherwise the reader will be fooled into thinking no more history.


*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_dsxfr.h>
#include <karma_dsrw.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_im.h>
#include <karma_m.h>
#include <karma_a.h>
#include "lib.h"


/*  Copied from  lib/karma/packages/dsrw/main.c  */
#define ARRAY_BOUNDARY (unsigned int) 16


/*  External functions  */
EXTERN_FUNCTION (void generate_file, (char *fits_filename, char *arrayfile) );


/*  Private functions  */

STATIC_FUNCTION (char *convert_object_to_filename, (CONST char *object_name) );
STATIC_FUNCTION (Channel open_file, (CONST char *arrayfile) );


/*  Public functions follow  */

flag command_parse (char *p, FILE *fp)
{
    char *arrayfile;
    char *input_filename;

    for ( ; p; p = ex_word_skip (p) )
    {
	if ( ( input_filename = ex_str (p, &p) ) == NULL )
	{
	    fprintf (stderr, "Error extracting input filename\n");
	    return (TRUE);
	}
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    fprintf (stderr, "Error extracting arrayfile name\n");
	    m_free (input_filename);
	    return (TRUE);
	}
	generate_file (input_filename, arrayfile);
	m_free (input_filename);
	m_free (arrayfile);
    }
    return (TRUE);
}   /*  End Function command_parse  */

Channel open_karma (CONST char *filename, CONST multi_array *multi_desc)
/*  [SUMMARY] Open a Karma file and write part of a data stucture.
    [PURPOSE] This routine will open a Karma file for writing and will write
    the multi_array descriptor header, the first packet descriptor and some
    array padding information. The array data may then be written.
    <filename> The name of the file to create. The ".kf" extension is
    automatically added.
    <multi_desc> The multi_array descriptor.
    [RETURNS] A channel object on success, else NULL.
*/
{
    Channel channel;
    unsigned int bytes_to_pad;
    unsigned long read_pos;
    unsigned long write_pos;
    extern char *sys_errlist[];

    if ( ( channel = open_file (filename) ) == NULL )
    {
	fprintf (stderr, "Error opening Karma arrayfile file\n");
	return (NULL);
    }
    /*  Write magic string and version number  */
    if ( !dsrw_write_multi_header (channel, multi_desc) )
    {
	exit (RV_WRITE_ERROR);
    }
    dsrw_write_packet_desc (channel, multi_desc->headers[0]);
    /*  Pad  */
    if ( !ch_tell (channel, &read_pos, &write_pos) )
    {
	fprintf (stderr, "Error getting channel position\n");
	exit (RV_UNDEF_ERROR);
    }
    /*  Add 4 bytes for pad size  */
    write_pos += 4;
    bytes_to_pad = ARRAY_BOUNDARY - write_pos % ARRAY_BOUNDARY;
    if ( !pio_write32 (channel, (unsigned long) bytes_to_pad) )
    {
	fprintf (stderr, "Error writing pad size\n");
	exit (RV_WRITE_ERROR);
    }
    if (bytes_to_pad > 0)
    {
	if (ch_fill (channel, bytes_to_pad, 0) < bytes_to_pad)
	{
	    fprintf (stderr, "Error padding: %u bytes\t%s\n",
		     bytes_to_pad, sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
    }
    return (channel);
}   /*  End Function open_karma  */

flag setup_for_writing (multi_array *multi_desc, flag tile,
			flag allow_truncation, uaddr **dim_lengths,
			uaddr **coords, Channel *karma_ch,
			CONST char *arrayfile)
/*  [SUMMARY] Setup for writing.
    <multi_desc> The multi_array descriptor.
    <tile> If TRUE, the array should be tiled.
    <allow_truncation> If TRUE, dimension truncation is allowed.
    <dim_lengths> The pointer to the array of original dimension lengths is
    written here.
    <coords> The pointer to the co-ordinate array is written here.
    <karma_ch> The Karma channel is written here.
    <arrayfile> The name of the file to create. The ".kf" extension is
    automatically added.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int array_size, count;
    unsigned int elem_type, elem_size, num_dim;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    char *packet, *array;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "setup_for_writing";

    pack_desc = multi_desc->headers[0];
    packet = multi_desc->data[0];
    arr_desc = (array_desc *) pack_desc->element_desc[0];
    array_size = ds_get_array_size (arr_desc);
    elem_type = arr_desc->packet->element_types[0];
    elem_size = host_type_sizes[elem_type];
    num_dim = arr_desc->num_dimensions;
    if (tile)
    {
	/*  Create a snapshot of the dimension lengths (prior to tiling)  */
	if ( ( *dim_lengths = (uaddr *)
	       m_dup ( (char *) arr_desc->lengths,
		       num_dim * sizeof **dim_lengths ) )
	     == NULL )
	{
	    m_abort (function_name, "dimension length array");
	}
	if ( ds_autotile_array (arr_desc, allow_truncation) )
	{
	    fprintf (stderr, "Array will be tiled with lengths: ");
	    for (count = 0; count < num_dim; ++count)
	    {
		fprintf (stderr, "%lu ", arr_desc->lengths[count]);
	    }
	    putc ('\n', stderr);
	    /*  Create co-ordinate array  */
	    if ( ( *coords = (uaddr *) m_alloc (num_dim * sizeof **coords) )
		 == NULL )
	    {
		m_abort (function_name, "co-ordinate array");
	    }
	    m_clear ( (char *) *coords, num_dim * sizeof **coords);
	    *karma_ch = NULL;
	    /*  Create space for array data  */
	    if ( ( array = m_alloc (array_size * elem_size) ) == NULL )
	    {
		m_error_notify (function_name, "data array");
		m_free ( (char *) *dim_lengths );
		m_free ( (char *) *coords );
		return (FALSE);
	    }
	    /*  Link into data structure  */
	    *(char **) packet = array;
	    *(unsigned int *) (sizeof (char *) + packet) = K_ARRAY_M_ALLOC;
	    return (TRUE);
	}
	else
	{
	    m_free ( (char *) *dim_lengths );
	    *dim_lengths = NULL;
	    *coords = NULL;
	    fprintf (stderr, "Array not tiled\n");
	}
    }
    /*  Now write the Karma descriptors  */
    if ( ( *karma_ch = open_karma (arrayfile, multi_desc) ) == NULL )
    {
	return (FALSE);
    }
    *dim_lengths = NULL;
    *coords = NULL;
    return (TRUE);
}   /*  End Function setup_for_writing  */

flag write_blocks (Channel channel, multi_array *multi_desc,
		   CONST uaddr *dim_lengths, uaddr *coords,
		   CONST char *data, unsigned int num_values)
/*  [SUMMARY] Write blocks of data.
    <channel> The Karma channel to write to.
    <multi_desc> The multi_array descriptor.
    <dim_lengths> The array of original dimension lengths.
    <coords> The co-ordinate array.
    <data> A pointer to a block of values.
    <num_values> The number of values in the block.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag want_block;
    uaddr offset;
    int dim_count;
    unsigned int array_size;
    unsigned int in_block_length, out_block_length, count;
    unsigned int elem_type, elem_size, num_dim;
    char *packet, *array;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "write_blocks";

    pack_desc = multi_desc->headers[0];
    packet = multi_desc->data[0];
    arr_desc = (array_desc *) pack_desc->element_desc[0];
    array_size = ds_get_array_size (arr_desc);
    elem_type = arr_desc->packet->element_types[0];
    elem_size = host_type_sizes[elem_type];
    num_dim = arr_desc->num_dimensions;
    if (channel != NULL)
    {
	/*  Streaming mode required  */
	if ( !dsrw_write_packets (channel, arr_desc->packet, data,num_values) )
	{
	    fprintf (stderr, "Error writing Karma file\n");
	    return (FALSE);
	}
	return (TRUE);
    }
    /*  Tiling (virtual memory) mode required  */
    in_block_length = dim_lengths[num_dim - 1];
    out_block_length = arr_desc->lengths[num_dim - 1];
    if (num_values % in_block_length != 0)
    {
	fprintf (stderr, "Bad number of values: %u\n", num_values);
	a_prog_bug (function_name);
    }
    if ( !ds_compute_array_offsets (arr_desc) ) return (FALSE);
    array = *(char **) packet;
    /*  Process one block at a time  */
    while (num_values > 0)
    {
	/*  Test if this input block is desired  */
	for (want_block = TRUE, dim_count = num_dim - 2;
	     want_block && (dim_count >= 0);
	     --dim_count)
	{
	    if (coords[dim_count] >= arr_desc->dimensions[dim_count]->length)
	    {
		/*  Current co-ordinate goes outside the array bounds  */
		want_block = FALSE;
	    }
	}
	if (want_block)
	{
	    /*  Write a block of data out: compute offset of first block. Note
		that the input block is many output blocks  */
	    for (dim_count = num_dim - 2, offset = 0; dim_count >= 0;
		 --dim_count)
	    {
		offset += arr_desc->offsets[dim_count][ coords[dim_count] ];
	    }
	    for (count = 0; count < arr_desc->dimensions[num_dim - 1]->length;
		 count += out_block_length)
	    {
		m_copy (array + offset + arr_desc->offsets[num_dim - 1][count],
			data + count * elem_size,
			out_block_length * elem_size);
	    }
	}
	/*  Increment co-ordinates  */
	dim_count = num_dim - 2;
	while ( (dim_count >= 0) &&
		(++coords[dim_count] >= dim_lengths[dim_count]) )
	    coords[dim_count--] = 0;
	/*  Decrement value counter  */
	num_values -= in_block_length;
	data += in_block_length * elem_size;
    }
    return (TRUE);
}   /*  End Function write_blocks  */

flag write_tail (Channel channel, multi_array *multi_desc, CONST char *infile,
		 CONST char *arrayfile)
/*  [SUMMARY] Write the tail part of a Karma data structure.
    <channel> The channel object to write to.
    <multi_desc> The multi_array descriptor.
    <infile> The input filename.
    <arrayfile> The output Karma filename.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int count, elem_type;
    packet_desc *pack_desc;
    history *entry;
    char *packet;
    char txt[STRING_LENGTH];
    extern char host_type_sizes[NUMTYPES];
    extern char module_name[STRING_LENGTH + 1];
    extern char module_version_date[STRING_LENGTH + 1];
    extern char module_lib_version[STRING_LENGTH + 1];
    extern char karma_library_version[STRING_LENGTH + 1];

    /*  Append a bit of history  */
    sprintf (txt, "%s: Module version %s  Karma v%s  compiled with v%s",
	     module_name, module_version_date,
	     karma_library_version, module_lib_version);
    ds_history_append_string (multi_desc, txt, TRUE);
    sprintf (txt, "%s: infile: %s  outfile: %s",
	     module_name, infile, arrayfile);
    ds_history_append_string (multi_desc, txt, TRUE);
    if (channel == NULL) return ( dsxfr_put_multi (arrayfile, multi_desc) );
    pack_desc = multi_desc->headers[0];
    packet = multi_desc->data[0] + host_type_sizes[K_ARRAY];
    for (count = 1; count < pack_desc->num_elements; ++count)
    {
	/*  Write element   */
        elem_type = pack_desc->element_types[count];
        dsrw_write_element (channel, elem_type,
			    pack_desc->element_desc[count],
			    packet);
        packet += host_type_sizes[elem_type];
    }
    /*  Write history  */
    fprintf (stderr, "writing history...\n");
    for (entry = multi_desc->first_hist; entry != NULL;
	 entry = entry->next)
    {
	if (*entry->string == '\0') continue;
	if ( !pio_write_string (channel, entry->string) )
	{
	    fprintf (stderr, "Error writing history string: \"%s\"\n",
			    entry->string);
	    return (FALSE);
	}
    }
    if ( !pio_write_string (channel, (char *) NULL) )
    {
	fprintf (stderr, "Error writing NULL history string\n");
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function write_tail  */

void cleanup (Channel karma_ch, uaddr *dim_lengths, uaddr *coords)
/*  [SUMMARY] Clean up.
    <karma_ch> The Karma channel object.
    <dim_lengths> The array of original dimension lengths.
    <coords> The co-ordinate array.
    [RETURNS] Nothing.
*/
{
    if (karma_ch != NULL) ch_close (karma_ch);
    if (dim_lengths != NULL) m_free ( (char *) dim_lengths );
    if (coords != NULL) m_free ( (char *) coords );
}   /*  End Function cleanup  */


/*  Private functions follow  */

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
    strcpy (filename, object_name);
    strcat (filename, default_extension);
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
	    fprintf (stderr, "Error accessing file: \"%s\"\t%s\n",
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
	strcpy (tilde_filename, filename);
	strcat (tilde_filename, "~");
	if (rename (filename, tilde_filename) != 0)
	{
	    fprintf (stderr, "Error renaming file: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	    m_free (tilde_filename);
	    m_free (filename);
	    return (NULL);
	}
	m_free (tilde_filename);
    }
    if ( ( channel = ch_open_file (filename, "w") ) == NULL )
    {
	fprintf (stderr, "Error opening file: \"%s\" for output\t%s\n",
			filename, sys_errlist[errno]);
	m_free (filename);
	return (NULL);
    }
    m_free (filename);
    return (channel);
}   /*  End Function open_file  */
