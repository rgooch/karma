/*LINTLIBRARY*/
/*  gipsy_write.c

    This code provides a GIPSY write facility.

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
  GIPSY format.


    Written by      Richard Gooch   7-AUG-1996

    Last updated by Richard Gooch   8-AUG-1996: Made public routines to write
  header and data.


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
#include <karma_p.h>
#include <karma_m.h>
#include <karma_a.h>


#define BUF_LENGTH 4096

#define KEY_LEN 21
#define MAXDIM  20
#define REC_SIZ 200
#define KEY_AL 4
#define KEY_DL 140


/*  Structure declarations  */


/*  Private data follows  */
static unsigned char inf_bytes[4] = {0x7f, 0x80, 0, 0};


/*  Declarations of private functions follow  */
STATIC_FUNCTION (flag gipsy_write,
		 (CONST char *basename, multi_array *multi_desc,
		  va_list argp) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag foreign_gipsy_write (CONST char *basename, multi_array *multi_desc, ...)
/*  [SUMMARY] Write a GIPSY file.
    [PURPOSE] This routine will write a Karma data structure to a GIPSY
    file. The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <basename> The base filename of the GIPSY file. The ".descr" and ".image"
    extensions are added automatically.
    <multi_desc> The multi_array descriptor pointer. The routine will find a
    n-dimensional array within the data structure.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_GIPSY_WRITE_END.
    See [<FOREIGN_ATT_GIPSY_WRITE>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    va_list argp;
    flag ok;
    static char function_name[] = "foreign_gipsy_write";

    va_start (argp, multi_desc);
    if ( (basename == NULL) || (multi_desc == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    ok = gipsy_write (basename, multi_desc, argp);
    va_end (argp);
    return (ok);
}   /*  End Function foreign_gipsy_write  */

/*PUBLIC_FUNCTION*/
flag foreign_gipsy_write_iarray (CONST char *basename, iarray array, ...)
/*  [SUMMARY] Write a GIPSY file.
    [PURPOSE] This routine will write an Intelligent Array to a GIPSY
    file. The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn", "BITPIX",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <basename> The base filename of the GIPSY file. The ".descr" and ".image"
    extensions are added automatically.
    <array> The Intelligent Array.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_GIPSY_WRITE_END.
    See [<FOREIGN_ATT_GIPSY_WRITE>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    va_list argp;
    flag ok;
    static char function_name[] = "foreign_gipsy_write_iarray";

    va_start (argp, array);
    if ( (basename == NULL) || (array == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    ok = gipsy_write (basename, array->multi_desc, argp);
    va_end (argp);
    return (ok);
}   /*  End Function foreign_gipsy_write_iarray  */

/*EXPERIMENTAL_FUNCTION*/
flag foreign_gipsy_write_header (Channel channel,
				 CONST packet_desc *header_pack_desc,
				 CONST char *header_packet, ...)
/*  [SUMMARY] Write a GIPSY header.
    <channel> The channel object to write the header to.
    <header_desc> The FITS-style header packet descriptor.
    <header_packet> The FITS-style header packet data.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_GIPSY_WRITE_HEADER_END.
    See [<FOREIGN_ATT_GIPSY_WRITE_HEADER>] for a list of defined attributes.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    uaddr elem_offset;
    flag ok, complex;
    char desc_type = '\0';
    unsigned int att_key;
    unsigned int count, record_count;
    unsigned int elem_type, naxis, axis_length, factor, num_items, item_size;
    double crpix;
    char *elem_name, *ptr;
    char txt[STRING_LENGTH];
    char item_data[KEY_DL];
    char desc_name[KEY_LEN];
    unsigned long ax_size[MAXDIM];
    unsigned long ax_factor[MAXDIM + 1];
    double value[2];
    double ax_origin[MAXDIM];
    extern char *sys_errlist[];
    static char function_name[] = "foreign_gipsy_write_header";

    va_start (argp, header_packet);
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) )
	    != FA_GIPSY_WRITE_HEADER_END )
    {
	switch (att_key)
	{
	  default:
	    fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    /*  Clear some buffers  */
    m_clear ( (char *) ax_size, sizeof *ax_size * MAXDIM );
    m_clear ( (char *) ax_factor, sizeof *ax_factor * (MAXDIM + 1) );
    m_clear ( (char *) ax_origin, sizeof *ax_origin * MAXDIM );
    /*  Write the version, subversion, number one and osdep values.
	Fix output to v2.2, big-endian and -Inf blanks  */
    if ( !pio_write32 (channel, 2) ) return (FALSE);
    if ( !pio_write32 (channel, 2) ) return (FALSE);
    if ( !pio_write32 (channel, 1) ) return (FALSE);
    if ( !pio_write32 (channel, 0) ) return (FALSE);
    /*  Construct the arrays describing the axes  */
    if ( !ds_get_unique_named_value (header_pack_desc, header_packet, "NAXIS",
				     NULL, value) ) return (FALSE);
    if ( (naxis = value[0]) > MAXDIM )
    {
	fprintf (stderr, "NAXIS: %u is too large!\n", naxis);
	return (FALSE);
    }
    for (count = 0, factor = 1; count < naxis;
	 ++count, factor *= axis_length + 1)
    {
	sprintf (txt, "NAXIS%d", count + 1);
	if ( ds_get_unique_named_value (header_pack_desc, header_packet, txt,
					NULL, value) )
	{
	    axis_length = value[0];
	}
	else axis_length = 1;
	sprintf (txt, "CRPIX%d", count + 1);
	if ( ds_get_unique_named_value (header_pack_desc, header_packet, txt,
					NULL, value) )
	{
	    crpix = value[0];
	}
	else crpix = 1.0;
	ax_origin[count] = crpix;
	ax_size[count] = axis_length;
	ax_factor[count] = factor;
    }
    ax_factor[count] = factor;
    /*  Write the axis-describing arrays  */
    for (count = 0; count < MAXDIM; ++count)
    {
	if ( !pio_write_double (channel, ax_origin[count]) ) return (FALSE);
    }
    for (count = 0; count < MAXDIM; ++count)
    {
	if ( !pio_write32 (channel, ax_size[count]) ) return (FALSE);
    }
    for (count = 0; count < MAXDIM + 1; ++count)
    {
	if ( !pio_write32 (channel, ax_factor[count]) ) return (FALSE);
    }
    /*  Write number of axes  */
    if ( !pio_write32 (channel, naxis) ) return (FALSE);
    /*  Write number of descriptor items: first compute number of items  */
    for (count = 0, num_items = 0; count < header_pack_desc->num_elements;
	 ++count)
    {
	elem_type = header_pack_desc->element_types[count];
	elem_offset = ds_get_element_offset (header_pack_desc, count);
	elem_name = header_pack_desc->element_desc[count];
	/*  Discard FITS-specific stuff  */
	if (strcmp (elem_name, "BITPIX") == 0) continue;
	if (strcmp (elem_name, "BSCALE") == 0) continue;
	if (strcmp (elem_name, "BZERO") == 0) continue;
	if (strcmp (elem_name, "BLANK") == 0) continue;
	if (strlen (elem_name) > KEY_LEN)
	{
	    fprintf (stderr, "Item name: \"%s\" too long: discarding\n",
		     elem_name);
	    continue;
	}
	++num_items;
    }
    if ( !pio_write32 (channel, num_items) ) return (FALSE);
    /*  Write reserved stuff  */
    if ( !pio_write32 (channel, 0) || !pio_write32 (channel, 0) ) return FALSE;
    /*  Write index to first descriptor item  */
    if ( !pio_write32 (channel, 2) ) return (FALSE);
    /*  Write number of records allocated  */
    if ( !pio_write32 (channel, 2 + num_items) ) return (FALSE);
    /*  Write current maximum number of records in file  */
    if ( !pio_write32 (channel, 2 + num_items) ) return (FALSE);
    /*  Write hash table size  */
    if ( !pio_write32 (channel, 0) ) return (FALSE);
    /*  Write some spares  */
    if (ch_fill (channel, 32, 0) < 32)
    {
	fprintf (stderr, "Error writing spares\t%s\n", sys_errlist[errno]);
	return (FALSE);
    }
    /*  Write pointer to free list  */
    if ( !pio_write32 (channel, 0) ) return (FALSE);
    /*  Write first hash table entry  */
    if ( !pio_write32 (channel, 0) ) return (FALSE);
    /*  Pad to record size  */
    if ( !ch_fill_to_boundary (channel, REC_SIZ, 0) ) return (FALSE);
    /*  Now start writing the keywords  */
    for (count = 0, record_count = 2; count < header_pack_desc->num_elements;
	 ++count)
    {
	elem_type = header_pack_desc->element_types[count];
	elem_offset = ds_get_element_offset (header_pack_desc, count);
	elem_name = header_pack_desc->element_desc[count];
	/*  Discard FITS-specific stuff  */
	if (strcmp (elem_name, "BITPIX") == 0) continue;
	if (strcmp (elem_name, "BSCALE") == 0) continue;
	if (strcmp (elem_name, "BZERO") == 0) continue;
	if (strcmp (elem_name, "BLANK") == 0) continue;
	if (strlen (elem_name) > KEY_LEN)
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
	    ptr = ds_get_unique_named_string (header_pack_desc, header_packet,
					      elem_name);
	    ok = (ptr == NULL) ? FALSE : TRUE;
	}
	if (!ok)
	{
	    fprintf (stderr, "Error getting header item\n");
	    a_prog_bug (function_name);
	}
	/*  Limit output to certain data types for convenience  */
	m_clear (item_data, KEY_DL);
	switch (elem_type)
	{
	  case K_FLOAT:
	  case K_DOUBLE:
	    sprintf (item_data, "FITS DBLE %e", value[0]);
	    break;
	  case K_VSTRING:
	    sprintf (item_data, "FITS CHAR '%s'", ptr);
	    m_free (ptr);
	    break;
	  default:
	    sprintf (item_data, "FITS INT  %d", (int) value[0]);
	    break;
	}
	item_size = strlen (item_data);
	if (item_size > KEY_DL) item_size = KEY_DL;
	/*  Write record type  */
	if ( !pio_write32 (channel, 1) ) return (FALSE);
	/*  Write length of descriptor item  */
	if ( !pio_write32 (channel, item_size) ) return (FALSE);
	/*  Write readpos  */
	if ( !pio_write32 (channel, 0) ) return (FALSE);
	/*  Write level  */
	if ( !pio_write32 (channel, 0) ) return (FALSE);
	/*  Write next_key  */
	if ( !pio_write32 (channel, 0) ) return (FALSE);
	/*  Write next_ext  */
	if ( !pio_write32 (channel, 0) ) return (FALSE);
	/*  Write last_ext  */
	if ( !pio_write32 (channel, record_count) ) return (FALSE);
	/*  Write curr_ext  */
	if ( !pio_write32 (channel, record_count) ) return (FALSE);
	/*  Write type  */
	if (ch_write (channel, &desc_type, 1) < 1)
	{
	    fprintf (stderr, "Error writing type\t%s\n", sys_errlist[errno]);
	    return (FALSE);
	}
	/*  Write name of item  */
	m_clear (desc_name, KEY_LEN);
	strncpy (desc_name, elem_name, KEY_LEN);
	if (ch_write (channel, desc_name, KEY_LEN) < KEY_LEN)
	{
	    fprintf (stderr, "Error writing name\t%s\n", sys_errlist[errno]);
	    return (FALSE);
	}
	/*  Pad to 4 byte boundary  */
	if ( !ch_fill_to_boundary (channel, 4, 0) ) return (FALSE);
	/*  Write alignment bytes  */
	if (ch_fill (channel, KEY_AL, 0) < KEY_AL)
	{
	    fprintf (stderr, "Error writing alignement bytes\t%s\n",
		     sys_errlist[errno]);
	    return (FALSE);
	}
	/*  Write item data  */
	if (ch_write (channel, item_data, KEY_DL) < KEY_DL)
	{
	    fprintf (stderr, "Error writing alignement item data\t%s\n",
		     sys_errlist[errno]);
	    return (FALSE);
	}
	/*  And finally pad to record boundary  */
	if ( !ch_fill_to_boundary (channel, REC_SIZ, 0) ) return (FALSE);
	++record_count;
    }
    /*  Pad to 512 byte boundary  */
    return ( ch_fill_to_boundary (channel, 512, 0) );
}   /*  End Function foreign_gipsy_write_header  */

flag foreign_gipsy_write_data (Channel channel, CONST multi_array *multi_desc,
			       CONST packet_desc *header_pack_desc,
			       CONST char *header_packet,
			       char *data, uaddr num_values, ...)
/*  [SUMMARY] Write GIPSY data.
    <channel> The channel object to write the header to.
    <multi_desc> The data to write.
    <header_desc> The header packet descriptor.
    <header_packet> The header packet data.
    <data> An alternate data array to read the data from. If this is
    NULL, the routine will read the data from the Karma data structure.
    <num_values> The number of values to write to the FITS file. This is
    only used when data is not NULL.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_GIPSY_WRITE_DATA_END.
    See [<FOREIGN_ATT_GIPSY_WRITE_DATA>] for a list of defined attributes.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    uaddr value_count;
    unsigned int att_key;
    unsigned int elem_type, elem_size, block_length, count;
    double d_val;
    double bscale, bzero;
    char *top_packet;
    double *d_ptr;
    packet_desc *top_pack_desc;
    array_desc *arr_desc;
    char buffer[BUF_LENGTH * 4];
    float f_values[BUF_LENGTH];
    double d_value[2];
    double d_values[BUF_LENGTH * 2];
    extern char host_type_sizes[NUMTYPES];
    extern char *sys_errlist[];
    static char function_name[] = "foreign_gipsy_write_data";

    va_start (argp, num_values);
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) )
	    != FA_GIPSY_WRITE_DATA_END )
    {
	switch (att_key)
	{
	  default:
	    fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
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
    /*  Convert blocks of values  */
    for (value_count = 0; num_values > 0;
	 num_values -= block_length, data += elem_size * block_length,
	     value_count += block_length)
    {
	block_length = (num_values > BUF_LENGTH) ? BUF_LENGTH : num_values;
	if ( !ds_get_elements (data, elem_type, elem_size, d_values, NULL,
			       block_length) )
	{
	    fprintf (stderr, "Error converting data\n");
	    a_prog_bug (function_name);
	}
	/*  Convert to floating point and apply scale and offset  */
	for (count = 0, d_ptr = d_values; count < block_length;
	     ++count, d_ptr += 2)
	{
	    if ( (d_val = *d_ptr) >= TOOBIG ) f_values[count] = TOOBIG;
	    else f_values[count] = d_val * bscale + bzero;
	}
	/*  Copy floats to generic network buffer  */
	if ( ds_can_transfer_element_as_block (K_FLOAT) )
	{
	    m_copy (buffer, (CONST char *) f_values, 4 * block_length);
	}
	else if ( ds_can_swaptransfer_element (K_FLOAT) )
	{
	    m_copy_and_swap_blocks (buffer, (CONST char *) f_values, 4, 4, 4,
				    block_length);
	}
	else
	{
	    /*  Write data one value at a time  */
	    for (count = 0; count < block_length; count++)
	    {
		if (f_values[count] >= TOOBIG)
		{
		    m_copy (buffer + count * 4, (CONST char *) inf_bytes, 4);
		}
		else
		{
		    if ( !p_write_buf_float (buffer + count * 4,
					     f_values[count]) )
		    {
			fprintf (stderr, "Error writing image data\n");
			return (FALSE);
		    }
		}
	    }
	}
	/*  Write block in one hit  */
	if (ch_write (channel, buffer, block_length * 4) < block_length * 4)
	{
	    fprintf (stderr, "Error writing image data\t%s\n",
		     sys_errlist[errno]);
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function foreign_gipsy_write_data  */


/*  Private functions follow  */

static flag gipsy_write (CONST char *basename, multi_array *multi_desc,
			 va_list argp)
/*  [SUMMARY] Write a GIPSY file.
    [PURPOSE] This routine will write a Karma data structure to a GIPSY
    file. The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn", "BITPIX",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <basename> The base filename of the GIPSY file. The ".descr" and ".image"
    extensions are added automatically.
    <multi_desc> The multi_array descriptor pointer. The routine will find a
    n-dimensional array within the data structure.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_GIPSY_WRITE_END.
    See [<FOREIGN_ATT_GIPSY_WRITE>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    Channel channel;
    unsigned int att_key;
    char *header_packet;
    packet_desc *header_pack_desc;
    char fname[STRING_LENGTH];
    extern char *sys_errlist[];
    static char function_name[] = "gipsy_write";

    if ( (basename == NULL) || (multi_desc == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_GIPSY_WRITE_END )
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
    sprintf (fname, "%s.descr", basename);
    if ( ( channel = ch_open_file (fname, "w") ) == NULL )
    {
	fprintf (stderr, "Error opening: \"%s\"\t%s\n",
		 fname, sys_errlist[errno]);
	ds_dealloc_packet (header_pack_desc, header_packet);
	return (FALSE);
    }
    if ( !foreign_gipsy_write_header (channel,
				      header_pack_desc, header_packet,
				      FA_GIPSY_WRITE_HEADER_END) )
    {
	ds_dealloc_packet (header_pack_desc, header_packet);
	return (FALSE);
    }
    if ( !ch_close (channel) )
    {
	ds_dealloc_packet (header_pack_desc, header_packet);
	return (FALSE);
    }
    sprintf (fname, "%s.image", basename);
    if ( ( channel = ch_open_file (fname, "w") ) == NULL )
    {
	fprintf (stderr, "Error opening: \"%s\"\t%s\n",
		 fname, sys_errlist[errno]);
	ds_dealloc_packet (header_pack_desc, header_packet);
	return (FALSE);
    }
    if ( !foreign_gipsy_write_data (channel, multi_desc,
				    header_pack_desc, header_packet,
				    NULL, 0,
				    FA_GIPSY_WRITE_DATA_END) )
    {
	ch_close (channel);
	ds_dealloc_packet (header_pack_desc, header_packet);
	return (FALSE);
    }
    ds_dealloc_packet (header_pack_desc, header_packet);
    return ( ch_close (channel) );
}   /*  End Function gipsy_write  */
