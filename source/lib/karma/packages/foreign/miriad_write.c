/*LINTLIBRARY*/
/*  miriad_write.c

    This code provides a Miriad write facility.

    Copyright (C) 1996  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*

    This file contains the various utility routines for writing data files in
  Miriad Image format.


    Written by      Richard Gooch   7-AUG-1996

    Updated by      Richard Gooch   5-SEP-1996: Fixed bug in writing mask file.

    Updated by      Richard Gooch   26-SEP-1996: "RESTFREQ" keyword now scaled.

    Updated by      Richard Gooch   6-OCT-1996: Fixed another bug in writing
  mask file: every group of 31 bits were reversed.

    Last updated by Richard Gooch   29-OCT-1996: Tidied up macros to keep
  Solaris 2 compiler happy.


*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_pio.h>
#include <karma_dmp.h>
#include <karma_dir.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>


#define MIN_ITEM_SIZE 16
#define BUF_LENGTH (128 * 31)  /* Must be a multiple of 31 for the mask file */
#define MAGIC_NUMBER 486734598

#define VERIFY_CONTEXT(ctx) if (ctx == NULL) \
{(void) fprintf (stderr, "NULL context passed\n"); \
 a_prog_bug (function_name); } \
if (ctx->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid context object\n"); \
 a_prog_bug (function_name); }

/*  Structure declarations  */

typedef struct _header_item
{
    char *name;
    char *data;
    unsigned int type;
    struct _header_item *next;
    double d_data;               /*  Used for axis scaling  */
} *HeaderItem;

struct miriad_data_context_type
{
    unsigned int magic_number;
    Channel image_channel;
    Channel mask_channel;
    int last_mask_bit_in_word;
    unsigned long mask_word;
};


/*  Declarations of private functions follow  */
STATIC_FUNCTION (flag miriad_write,
		 (CONST char *dirname, multi_array *multi_desc,va_list argp) );
STATIC_FUNCTION (flag write_miriad_header,
		 (CONST char *dirname, CONST packet_desc *header_desc,
		  CONST char *header_packet) );
STATIC_FUNCTION (flag remove_directory, (CONST char *dirname) );
STATIC_FUNCTION (flag write_miriad_type,
		 (Channel channel, unsigned int ktype) );
STATIC_FUNCTION (flag write_miriad_data,
		 (CONST char *dirname, CONST multi_array *multi_desc,
		  double bscale, double bzero) );
STATIC_FUNCTION (flag write_mask_buf,
		 (CONST char *dirname, Channel *channel,
		  CONST flag *masks, unsigned int num_masks, uaddr open_pad) );
STATIC_FUNCTION (flag write_miriad_history,
		 (CONST char *dirname, CONST multi_array *multi_desc) );
STATIC_FUNCTION (flag convert_units,
		 (packet_desc *header_pack_desc, char **header_packet) );
STATIC_FUNCTION (flag scale_element,
		 (packet_desc *header_pack_desc, char **header_packet,
		  CONST char *name, double scale) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag foreign_miriad_write (CONST char *dirname, multi_array *multi_desc, ...)
/*  [SUMMARY] Write a Miriad Image file.
    [PURPOSE] This routine will write a Karma data structure to a Miriad Image
    file. The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <dirname> The directory name of the Miriad Image file.
    <multi_desc> The multi_array descriptor pointer. The routine will find a
    n-dimensional array within the data structure.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_MIRIAD_WRITE_END.
    See [<FOREIGN_ATT_MIRIAD_WRITE>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    va_list argp;
    flag ok;
    static char function_name[] = "foreign_miriad_write";

    va_start (argp, multi_desc);
    if ( (dirname == NULL) || (multi_desc == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    ok = miriad_write (dirname, multi_desc, argp);
    va_end (argp);
    return (ok);
}   /*  End Function foreign_miriad_write  */

/*PUBLIC_FUNCTION*/
flag foreign_miriad_write_iarray (CONST char *dirname, iarray array, ...)
/*  [SUMMARY] Write a Miriad Image file.
    [PURPOSE] This routine will write an Intelligent Array to a Miriad Image
    file. The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn", "BITPIX",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <dirname> The directory name of the Miriad Image file.
    <array> The Intelligent Array.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_MIRIAD_WRITE_END.
    See [<FOREIGN_ATT_MIRIAD_WRITE>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    va_list argp;
    flag ok;
    static char function_name[] = "foreign_miriad_write_iarray";

    va_start (argp, array);
    if ( (dirname == NULL) || (array == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    ok = miriad_write (dirname, array->multi_desc, argp);
    va_end (argp);
    return (ok);
}   /*  End Function foreign_miriad_write_iarray  */


/*  Private functions follow  */

static flag miriad_write (CONST char *dirname, multi_array *multi_desc,
			  va_list argp)
/*  [SUMMARY] Write a Miriad Image file.
    [PURPOSE] This routine will write a Karma data structure to a Miriad Image
    file. The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn", "BITPIX",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <dirname> The directory name of the Miriad Image file.
    <multi_desc> The multi_array descriptor pointer. The routine will find a
    n-dimensional array within the data structure.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_MIRIAD_WRITE_END.
    See [<FOREIGN_ATT_MIRIAD_WRITE>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    unsigned int att_key;
    double bscale, bzero;
    char *header_packet;
    packet_desc *header_pack_desc;
    double d_value[2];
    static char function_name[] = "miriad_write";

    if ( (dirname == NULL) || (multi_desc == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_MIRIAD_WRITE_END )
    {
	switch (att_key)
	{
	  default:
	    fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    /*  Save work by generating a FITS header, and ignoring select keywords
	later  */
    if ( !foreign_fits_generate_header (&header_pack_desc, &header_packet,
					multi_desc,
					FA_FITS_GENERATE_HEADER_END) )
    {
	return (FALSE);
    }
    /*  Convert units from FITS standard to Miriad  */
    if ( !convert_units (header_pack_desc, &header_packet) )
    {
	ds_dealloc_packet (header_pack_desc, header_packet);
	return (FALSE);
    }
    if ( !write_miriad_header (dirname, header_pack_desc, header_packet) )
    {
	ds_dealloc_packet (header_pack_desc, header_packet);
	return (FALSE);
    }
    if ( ds_get_unique_named_value (header_pack_desc, header_packet, "BSCALE",
				    NULL, d_value) )
    {
	bscale = d_value[0];
    }
    else bscale = 1.0;
    if ( ds_get_unique_named_value (header_pack_desc, header_packet, "BZERO",
				    NULL, d_value) )
    {
	bzero = d_value[0];
    }
    else bzero = 0.0;
    ds_dealloc_packet (header_pack_desc, header_packet);
    if ( !write_miriad_data (dirname, multi_desc, bscale, bzero) )
	return (FALSE);
    return (write_miriad_history (dirname, multi_desc) );
}   /*  End Function miriad_write  */

static flag write_miriad_header (CONST char *dirname,
				 CONST packet_desc *header_desc,
				 CONST char *header_packet)
/*  [SUMMARY] Write a Miriad header.
    <dirname> The directory name of the Miriad Image file.
    <header_desc> The FITS-style header packet descriptor.
    <header_packet> The FITS-style header packet data.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    uaddr elem_offset;
    flag ok, complex;
    int count;
    unsigned int elem_type, rep_type, item_size, pad_size;
    struct stat statbuf;
    char *elem_name, *ptr;
    char txt[STRING_LENGTH];
    char item_buf[MIN_ITEM_SIZE];
    double value[2];
    extern char *sys_errlist[];
    static char function_name[] = "write_miriad_header";

    /*  Ensure directory does not exist. If it does: remove or rename it  */
    if (stat (dirname, &statbuf) == 0)
    {
	/*  Something already exists  */
	if ( S_ISDIR (statbuf.st_mode) )
	{
	    if ( !remove_directory (dirname) ) return (FALSE);
	}
	else
	{
	    sprintf (txt, "%s~", dirname);
	    if (rename (dirname, txt) == -1)
	    {
		fprintf (stderr, "Error renaming: \"%s\"\t%s\n",
			 dirname, sys_errlist[errno]);
		return (FALSE);
	    }
	}
    }
    else
    {
	/*  Error accessing  */
	if (errno != ENOENT)
	{
	    fprintf (stderr, "Error statting file: \"%s\"\t%s\n",
		     dirname, sys_errlist[errno]);
	    return (FALSE);
	}
    }
    /*  Create directory  */
    if (mkdir (dirname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
    {
	fprintf (stderr, "Error creating directory: \"%s\"\t%s\n",
		 dirname, sys_errlist[errno]);
	return (FALSE);
    }
    sprintf (txt, "%s/header", dirname);
    if ( ( channel = ch_open_file (txt, "w") ) == NULL )
    {
	fprintf (stderr, "Error creating file: \"%s\"\t%s\n",
		 txt, sys_errlist[errno]);
	return (FALSE);
    }
    /*  Now start writing the keywords: do this in reverse order because Miriad
	seems to do it this way. Makes the output of <kprinthead> nicer  */
    for (count = header_desc->num_elements - 1; count >= 0; --count)
    {
	elem_type = header_desc->element_types[count];
	elem_offset = ds_get_element_offset (header_desc, count);
	elem_name = header_desc->element_desc[count];
	/*  Discard FITS-specific stuff  */
	if (strcmp (elem_name, "BITPIX") == 0) continue;
	if (strcmp (elem_name, "BSCALE") == 0) continue;
	if (strcmp (elem_name, "BZERO") == 0) continue;
	if (strcmp (elem_name, "BLANK") == 0) continue;
	if (strlen (elem_name) > MIN_ITEM_SIZE - 2)
	{
	    fprintf (stderr, "Item name: \"%s\" too long: discarding\n",
		     elem_name);
	    continue;
	}
	/*  Convert data to cannonical forms  */
	if ( ds_element_is_atomic (elem_type) )
	{
	    ok = ds_get_element (header_packet + elem_offset, elem_type, value,
				 &complex);
	    ptr = NULL;  /*  Keep compiler happy  */
	    if (complex)
	    {
		fprintf (stderr, "Complex header item\n");
		a_prog_bug (function_name);
	    }
	}
	else
	{
	    ptr = ds_get_unique_named_string (header_desc, header_packet,
					      elem_name);
	    ok = (ptr == NULL) ? FALSE : TRUE;
	}
	if (!ok)
	{
	    fprintf (stderr, "Error getting header item\n");
	    a_prog_bug (function_name);
	}
	/*  Limit output to certain data types for convenience  */
	switch (elem_type)
	{
	  case K_FLOAT:
	  case K_DOUBLE:
	    rep_type = K_DOUBLE;
	    item_size = 8;
	    pad_size = 4;
	    break;
	  case K_VSTRING:
	    rep_type = K_BYTE;
	    item_size = strlen (ptr);
	    pad_size = 0;
	    break;
	  default:
	    rep_type = K_INT;
	    item_size = 4;
	    pad_size = 0;
	    break;
	}
	/*  Copy item name to buffer  */
	strcpy (item_buf, elem_name);
	st_lwr (item_buf);
	/*  Write the item size to buffer  */
	item_buf[MIN_ITEM_SIZE - 1] = 4 + item_size + pad_size;
	if (ch_write (channel, item_buf, MIN_ITEM_SIZE) < MIN_ITEM_SIZE)
	{
	    fprintf (stderr, "Error writing\t%s\n", sys_errlist[errno]);
	    ch_close (channel);
	    return (FALSE);
	}
	/*  Write Miriad type  */
	if ( !write_miriad_type (channel, rep_type ) )
	{
	    ch_close (channel);
	    return (FALSE);
	}
	/*  Write padding  */
	if (ch_fill (channel, pad_size, 0) < pad_size)
	{
	    fprintf (stderr, "Error writing\t%s\n", sys_errlist[errno]);
	    ch_close (channel);
	    return (FALSE);
	}
	/*  Write item data  */
	switch (rep_type)
	{
	  case K_INT:
	    if ( !pio_write32s (channel, (signed long) value[0]) )
	    {
		ch_close (channel);
		return (FALSE);
	    }
	    break;
	  case K_DOUBLE:
	    if ( !pio_write_double (channel, value[0]) )
	    {
		ch_close (channel);
		return (FALSE);
	    }
	    break;
	  case K_BYTE:
	    if (ch_write (channel, ptr, item_size) < item_size)
	    {
		fprintf (stderr, "Error writing\t%s\n", sys_errlist[errno]);
		ch_close (channel);
		m_free (ptr);
		return (FALSE);
	    }
	    break;
	  default:
	    fprintf (stderr, "Illegal representation type: %u\n", rep_type);
	    a_prog_bug (function_name);
	    break;
	}
	/*  Pad to next boundary  */
	if ( !ch_fill_to_boundary (channel, MIN_ITEM_SIZE, 0) )
	{
	    fprintf (stderr, "Error writing\t%s\n", sys_errlist[errno]);
	    ch_close (channel);
	    return (FALSE);
	}
    }
    return ( ch_close (channel) );
}   /*  End Function write_miriad_header  */

static flag remove_directory (CONST char *dirname)
/*  [SUMMARY] Empty and delete a directory.
    <dirname> The directory name.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    KDir dir;
    KFileInfo *file;
    char fname[STRING_LENGTH];
    extern char *sys_errlist[];

    if ( ( dir = dir_open (dirname) ) == NULL ) return (FALSE);
    while ( ( file = dir_read (dir, KDIR_NO_DOTS) ) != NULL )
    {
	sprintf (fname, "%s/%s", dirname, file->filename);
	if (unlink (fname) == -1)
	{
	    fprintf (stderr, "Error removing file: \"%s\"\t%s\n",
		     fname, sys_errlist[errno]);
	    dir_close (dir);
	    return (FALSE);
	}
    }
    dir_close (dir);
    if (rmdir (dirname) == -1)
    {
	fprintf (stderr, "Error removing directory: \"%s\"\t%s\n",
		 dirname, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function remove_directory  */

static flag write_miriad_type (Channel channel, unsigned int ktype)
/*  [PURPOSE] This routine will convert a Karma data type to a Miriad data type
    and write the Miriad type to a channel.
    <channel> The channel to write to.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned long mtype = 0;  /*  Initialised to keep compiler happy  */
    static char function_name[] = "__foreign_miriad_write_miriad_type";

    switch (ktype)
    {
      case K_BYTE:
	mtype = 1;
	break;
      case K_INT:
	mtype = 2;
	break;
      case K_SHORT:
	mtype = 3;
	break;
      case K_FLOAT:
	mtype = 4;
	break;
      case K_DOUBLE:
	mtype = 5;
	break;
      case K_VSTRING:
	mtype = 6;
	break;
      case K_COMPLEX:
	mtype = 7;
	break;
      default:
	fprintf (stderr, "Bad Karma type: %u\n", ktype);
	a_prog_bug (function_name);
	break;
    }
    if ( !pio_write32 (channel, mtype) )
    {
	fprintf (stderr, "Error writing type\n");
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function write_miriad_type  */

static flag write_miriad_data (CONST char *dirname,
			       CONST multi_array *multi_desc,
			       double bscale, double bzero)
/*  [SUMMARY] Write Miriad Image data.
    <dirname> The directory name of the Miriad Image file.
    <multi_desc> The data to write.
    <bscale> The scale factor to apply to the data.
    <bzero> The offset to apply to the scaled data.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel image_ch;
    Channel mask_ch = NULL;
    uaddr value_count, num_values = 0;
    flag block_has_toobig;
    unsigned int elem_type, elem_size, block_length, count;
    double d_val;
    char *data = NULL;
    char *top_packet;
    double *d_ptr;
    packet_desc *top_pack_desc;
    array_desc *arr_desc;
    char image_name[STRING_LENGTH];
    flag mask_buf[BUF_LENGTH];
    float f_values[BUF_LENGTH];
    double d_values[BUF_LENGTH * 2];
    extern char host_type_sizes[NUMTYPES];
    extern char *sys_errlist[];
    static char function_name[] = "__foreign_miriad_write_miriad_data";

    top_pack_desc = multi_desc->headers[0];
    top_packet = multi_desc->data[0];
    if (top_pack_desc->element_types[0] != K_ARRAY)
    {
	fprintf (stderr,"First element in top level packet must be K_ARRAY\n");
	a_prog_bug (function_name);
    }
    arr_desc = (array_desc *) top_pack_desc->element_desc[0];
    elem_type = arr_desc->packet->element_types[0];
    elem_size = host_type_sizes[elem_type];
    if (data == NULL)
    {
	data = *(char **) top_packet;
	if (data == NULL)
	{
	    fprintf (stderr, "No array to read data from!\n");
	    a_prog_bug (function_name);
	}
	num_values = ds_get_array_size (arr_desc);
    }
    if ( !ds_element_is_atomic (elem_type) )
    {
	fprintf (stderr, "Non-atomic element not allowed\n");
	a_prog_bug (function_name);
    }
    if ( ds_element_is_complex (elem_type) )
    {
	fprintf (stderr, "Complex element not allowed\n");
	a_prog_bug (function_name);
    }
    sprintf (image_name, "%s/image", dirname);
    if ( ( image_ch = ch_open_file (image_name, "w") ) == NULL )
    {
	fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		 image_name, sys_errlist[errno]);
	return (FALSE);
    }
    /*  Write K_FLOAT type  */
    if ( !write_miriad_type (image_ch, K_FLOAT) )
    {
	ch_close (image_ch);
	return (FALSE);
    }
    /*  Convert blocks of values  */
    for (value_count = 0; num_values > 0;
	 num_values -= block_length, data += elem_size * block_length,
	     value_count += block_length)
    {
	block_length = (num_values > BUF_LENGTH) ? BUF_LENGTH : num_values;
	block_has_toobig = FALSE;
	if ( !ds_get_elements (data, elem_type, elem_size, d_values, NULL,
			       block_length) )
	{
	    fprintf (stderr, "Error converting data\n");
	    a_prog_bug (function_name);
	}
	for (count = 0, d_ptr = d_values; count < block_length;
	     ++count, d_ptr += 2)
	{
	    if ( (d_val = *d_ptr) >= TOOBIG )
	    {
		mask_buf[count] = FALSE;  /*  Data not valid  */
		block_has_toobig = TRUE;
		f_values[count] = 0.0;
	    }
	    else
	    {
		mask_buf[count] = TRUE;   /*  Data valid  */
		f_values[count] = d_val * bscale + bzero;
	    }
	}
	if ( ds_can_transfer_element_as_block (K_FLOAT) )
	{
	    if (ch_write (image_ch, (CONST char *) f_values,
			  block_length * 4) < block_length * 4)
	    {
		fprintf (stderr, "Error writing image data\t%s\n",
			 sys_errlist[errno]);
		ch_close (image_ch);
		if (mask_ch != NULL) ch_close (mask_ch);
		return (FALSE);
	    }
	}
	else if ( ds_can_swaptransfer_element (K_FLOAT) )
	{
	    if (ch_swap_and_write_blocks (image_ch, (CONST char *) f_values,
					  block_length, 4) < block_length * 4)
	    {
		fprintf (stderr, "Error writing image data\t%s\n",
			 sys_errlist[errno]);
		ch_close (image_ch);
		if (mask_ch != NULL) ch_close (mask_ch);
		return (FALSE);
	    }
	}
	else
	{
	    /*  Write data one value at a time  */
	    for (count = 0; count < block_length; count++)
	    {
		if ( !pio_write_float (image_ch, f_values[count]) )
		{
		    fprintf (stderr, "Error writing image data\n");
		    ch_close (image_ch);
		    if (mask_ch != NULL) ch_close (mask_ch);
		    return (FALSE);
		}
	    }
	}
	if ( !block_has_toobig && (mask_ch == NULL) ) continue;
	/*  This block has a TOOBIG, or some previous block did (if a previous
	    block had a TOOBIG, the mask channel is open  */
	if ( !write_mask_buf (dirname, &mask_ch, mask_buf,
			      block_length, value_count) )
	{
	    ch_close (image_ch);
	    if (mask_ch != NULL) ch_close (mask_ch);
	    return (FALSE);
	}
    }
    if ( !ch_close (image_ch) )
    {
	if (mask_ch != NULL) ch_close (mask_ch);
	return (FALSE);
    }
    if (mask_ch != NULL)
    {
	return ( ch_close (mask_ch) );
    }
    return (TRUE);
}   /*  End Function write_miriad_data  */

static flag write_mask_buf (CONST char *dirname, Channel *channel,
			    CONST flag *masks, unsigned int num_masks,
			    uaddr open_pad)
/*  [SUMMARY] Write mask values.
    <dirname> The directory name of the Miriad Image file.
    <channel> The mask channel. This is updated.
    <masks> The array of mask flags. TRUE values indicated data is OK, FALSE
    values indicate the data is bad (and should be masked/blanked).
    <num_masks> The number of mask flags.
    <open_pad> The number of mask flags to pad the beginning of the file with.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int bit_count;
    unsigned long mask_word, mask_bit;
    char mask_name[STRING_LENGTH];
    extern char *sys_errlist[];
    static unsigned char pad_buf[4] = {0x7f, 0xff, 0xff, 0xff};
    static char function_name[] = "__foreign_miriad_write_mask_buf";

    if (*channel == NULL)
    {
	if (open_pad % 31 != 0)
	{
	    fprintf (stderr, "open_pad: %lu is not a multiple of 31\n",
		     open_pad);
	    a_prog_bug (function_name);
	}
	sprintf (mask_name, "%s/mask", dirname);
	if ( ( *channel = ch_open_file (mask_name, "w") ) == NULL )
	{
	    fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		     mask_name, sys_errlist[errno]);
	    return (FALSE);
	}
	/*  Write K_INT type  */
	if ( !write_miriad_type (*channel, K_INT) )
	{
	    return (FALSE);
	}
	/*  Write pad at the beginning  */
	for (; open_pad > 0; open_pad -= 31)
	{
	    if (ch_write (*channel, (CONST char *) pad_buf, 4) < 4)
	    {
		fprintf (stderr, "Error writing mask pad\t%s\n",
			 sys_errlist[errno]);
		return (FALSE);
	    }
	}
    }
    /*  Write mask values as single bits, with MSB being a pad  */
    for (; num_masks > 0; num_masks -= bit_count)
    {
	mask_word = 0;
	mask_bit = 0x00000001;
	for (bit_count = 0; (bit_count < num_masks) && (bit_count < 31);
	     ++bit_count, ++masks)
	{
	    if (*masks) mask_word |= mask_bit;
	    mask_bit = mask_bit << 1;
	}
	/*  Ready to write a word  */
	if ( !pio_write32 (*channel, mask_word) )
	{
	    fprintf (stderr, "Error writing mask word\n");
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function write_mask_buf  */

static flag write_miriad_history (CONST char *dirname,
				  CONST multi_array *multi_desc)
/*  [SUMMARY] Write a Miriad history file.
    <dirname> The directory name of the Miriad Image file.
    <multi_desc> The data structure with the history.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    history *hist;
    char txt[STRING_LENGTH];
    extern char *sys_errlist[];
    /*static char function_name[] = "write_miriad_history";*/

    if ( (hist = multi_desc->first_hist) == NULL ) return (TRUE);
    sprintf (txt, "%s/history", dirname);
    if ( ( channel = ch_open_file (txt, "w") ) == NULL )
    {
	fprintf (stderr, "Error creating file: \"%s\"\t%s\n",
		 txt, sys_errlist[errno]);
	return (FALSE);
    }
    for (; hist != NULL; hist = hist->next)
    {
	if ( !ch_puts (channel, hist->string, TRUE) )
	{
	    fprintf (stderr, "Error writing history string\t%s\n",
		     sys_errlist[errno]);
	    ch_close (channel);
	    return (FALSE);
	}
    }
    return ( ch_close (channel) );
}   /*  End Function write_miriad_history  */

static flag convert_units (packet_desc *header_pack_desc,
			   char **header_packet)
/*  [SUMMARY] Convert header units from FITS to Miriad.
    <header_pack_desc> The header packet descriptor.
    <header_packet> The header packet data. This is modified.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag keep_going;
    unsigned int axis_count;
    double scale;
    char *ctype;
    char txt[STRING_LENGTH];

    /*  First process "CRVALn" and "CDELTn" keywords associated with each
	"CTYPEn" keyword found  */
    for (axis_count = 1, keep_going = TRUE; keep_going; ++axis_count)
    {
	sprintf (txt, "CTYPE%u", axis_count);
	if ( ( ctype = ds_get_unique_named_string (header_pack_desc,
						   *header_packet, txt) )
	     == NULL )
	{
	    keep_going = FALSE;
	    continue;
	}
	scale = 1.0 / foreign_miriad_get_units_scale (ctype);
	m_free (ctype);
	ctype = NULL;  /*  For my own protection  */
	if (scale == 1.0)
	{
	    /*  No scaling: ignore  */
	    continue;
	}
	/*  Update "CRVALn" if present  */
	sprintf (txt, "CRVAL%u", axis_count);
	if ( !scale_element (header_pack_desc, header_packet, txt, scale) )
	{
	    return (FALSE);
	}
	/*  Update "CDELTn" if present  */
	sprintf (txt, "CDELT%u", axis_count);
	if ( !scale_element (header_pack_desc, header_packet, txt, scale) )
	{
	    return (FALSE);
	}
    }
    /*  Update "RESTFREQ" if present  */
    scale = 1.0 / foreign_miriad_get_units_scale ("RESTFREQ");
    if ( !scale_element (header_pack_desc, header_packet, "RESTFREQ", scale) )
    {
	return (FALSE);
    }
    /*  Update "BMIN" if present  */
    scale = 1.0 / foreign_miriad_get_units_scale ("BMIN");
    if ( !scale_element (header_pack_desc, header_packet, "BMIN", scale) )
    {
	return (FALSE);
    }
    /*  Update "BMAJ" if present  */
    scale = 1.0 / foreign_miriad_get_units_scale ("BMAJ");
    if ( !scale_element (header_pack_desc, header_packet, "BMAJ", scale) )
    {
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function convert_units  */

static flag scale_element (packet_desc *header_pack_desc, char **header_packet,
			   CONST char *name, double scale)
/*  [SUMMARY] Scale an atomic element.
    <header_pack_desc> The header packet descriptor.
    <header_packet> The header packet data. This is modified.
    <name> The name of the element to scale.
    <scale> The scale value to apply.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int type;
    double value[2];

    if ( ds_get_unique_named_value (header_pack_desc, *header_packet, name,
				    &type, value) )
    {
	value[0] *= scale;
	value[1] *= scale;
	if ( !ds_put_unique_named_value (header_pack_desc, header_packet,
					 name, type, value, TRUE) )
	{
	    fprintf (stderr, "Error updating: \"%s\" element\n", name);
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function scale_element  */
