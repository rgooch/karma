/*LINTLIBRARY*/
/*  fits_write.c

    This code provides a FITS write facility.

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
  FITS format.


    Written by      Richard Gooch   5-AUG-1996

    Updated by      Richard Gooch   6-AUG-1996: Added history support.

    Updated by      Richard Gooch   15-AUG-1996: Removed use of <<reference>>
  field for dimension descriptor (use "CRVALn" instead) and improved merging
  algorithm for header information.

    Updated by      Richard Gooch   26-SEP-1996: Changed to implement the new
  FITS-style co-ordinate handling, where dimension co-ordinates range from 0 to
  length - 1. This means that "CRVALn", "CRPIXn" and "CDELTn" values are passed
  through unmolested, irrespective of the dimension co-ordinates.

    Last updated by Richard Gooch   27-NOV-1996: Ensure "NAXISn" keywords are
  written before most other keywords, to keep saoimage and other broken FITS
  readers happy.


*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>


#define CARD_WIDTH 80
#define CARD_LENGTH 36
#define EQUALS_POSITION 8
#define CARD_SIZE (CARD_WIDTH * CARD_LENGTH)
#define BUF_LENGTH 4096


/*  Declarations of private functions follow  */
STATIC_FUNCTION (flag fits_write,
		 (Channel channel, multi_array *multi_desc, va_list argp) );
STATIC_FUNCTION (void append_int,
		 (packet_desc *desc, char **packet, CONST char *name,
		  int value) );
STATIC_FUNCTION (flag write_fits_header,
		 (Channel channel, CONST packet_desc *header_desc,
		  CONST char *header_packet, CONST history *first_hist) );
STATIC_FUNCTION (flag write_fits_header_line,
		 (Channel channel, CONST char *line) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag foreign_fits_write (Channel channel, multi_array *multi_desc, ...)
/*  [SUMMARY] Write a FITS file to a channel.
    [PURPOSE] This routine will write a Karma data structure to a FITS file.
    The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn", "BITPIX",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <channel> The channel to write to. The channel is not flushed.
    <multi_desc> The multi_array descriptor pointer. The routine will find a
    n-dimensional array within the data structure.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with FA_FITS_WRITE_END.
    See [<FOREIGN_ATT_FITS_WRITE>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    va_list argp;
    flag ok;
    static char function_name[] = "foreign_fits_write";

    va_start (argp, multi_desc);
    if ( (channel == NULL) || (multi_desc == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    ok = fits_write (channel, multi_desc, argp);
    va_end (argp);
    return (ok);
}   /*  End Function foreign_fits_write  */

/*PUBLIC_FUNCTION*/
flag foreign_fits_write_iarray (Channel channel, iarray array, ...)
/*  [SUMMARY] Write a FITS file to a channel.
    [PURPOSE] This routine will write an Intelligent Array to a FITS file.
    The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn", "BITPIX",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <channel> The channel to write to. The channel is not flushed.
    <array> The Intelligent Array.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with FA_FITS_WRITE_END.
    See [<FOREIGN_ATT_FITS_WRITE>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    va_list argp;
    flag ok;
    static char function_name[] = "foreign_fits_write_iarray";

    va_start (argp, array);
    if ( (channel == NULL) || (array == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    ok = fits_write (channel, array->multi_desc, argp);
    va_end (argp);
    return (ok);
}   /*  End Function foreign_fits_write_iarray  */

/*EXPERIMENTAL_FUNCTION*/
flag foreign_fits_write_data (Channel channel, multi_array *multi_desc,
			      CONST packet_desc *header_pack_desc,
			      CONST char *header_packet,
			      char *data, uaddr num_values, ...)
/*  [SUMMARY] Write data to a FITS file.
    [PURPOSE] This routine will write the data section of a FITS file to a
    channel. The header section is NOT written.
    <channel> The channel to write to.
    <multi_desc> The Karma data structure containing the data.
    <header_pack_desc> The header packet descriptor.
    <header_packet> The header packet data.
    <data> An alternate data array to read the data from. If this is
    NULL, the routine will read the data from the Karma data structure.
    <num_values> The number of values to write to the FITS file. This is
    only used when data is not NULL.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_FITS_WRITE_DATA_END. See [<FOREIGN_ATT_FITS_WRITE_DATA>] for a list of
    defined attributes.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    int bitpix;
    unsigned int att_key, remainder;
    unsigned int elem_type, elem_size, block_length, count;
    unsigned long read_pos, write_pos;
    double bscale, bzero, d_val;
    packet_desc *top_pack_desc;
    array_desc *arr_desc;
    char *top_packet;
    unsigned char ub_values[BUF_LENGTH];
    double d_value[2];
    double d_values[BUF_LENGTH * 2];
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "foreign_fits_write_data";

    va_start (argp, num_values);
    if ( (channel == NULL) || (multi_desc == NULL) ||
	 (header_pack_desc == NULL) || (header_packet == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp,unsigned int) )
	    != FA_FITS_WRITE_DATA_END )
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
    top_pack_desc = multi_desc->headers[0];
    top_packet = multi_desc->data[0];
    if (top_pack_desc->element_types[0] != K_ARRAY)
    {
	fprintf (stderr,
			"First element in top level packet must be K_ARRAY\n");
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
    if ( !ds_get_unique_named_value (header_pack_desc, header_packet, "BITPIX",
				     NULL, d_value) )
    {
	fprintf (stderr, "BITPIX not found\n");
	a_prog_bug (function_name);
    }
    bitpix = d_value[0];
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
    /*  Convert blocks of values  */
    while (num_values > 0)
    {
	block_length = (num_values > BUF_LENGTH) ? BUF_LENGTH : num_values;
	if ( !ds_get_elements (data, elem_type, elem_size, d_values, NULL,
			       block_length) )
	{
	    fprintf (stderr, "Error converting data\n");
	    a_prog_bug (function_name);
	}
	/*  TODO: test for TOOBIGs  */
	switch (bitpix)
	{
	  case 8:
	    for (count = 0; count < block_length; ++count)
	    {
		ub_values[count] = d_values[count * 2];
	    }
	    if (ch_write (channel, (CONST char *) ub_values, count) < count)
	    {
		return (FALSE);
	    }
	    break;
	  case 16:
	    for (count = 0; count < block_length; ++count)
	    {
		if ( !pio_write16s (channel,
				    (signed long) d_values[count * 2]) )
		{
		    return (FALSE);
		}
	    }
	    break;
	  case 32:
	    for (count = 0; count < block_length; ++count)
	    {
		if ( !pio_write32s (channel,
				    (signed long) d_values[count * 2]) )
		{
		    return (FALSE);
		}
	    }
	    break;
	  case -32:
	    for (count = 0; count < block_length; ++count)
	    {
		d_val = d_values[count * 2] * bscale + bzero;
		if ( !pio_write_float (channel, (float) d_val) ) return FALSE;
	    }
	    break;
	  case -64:
	    for (count = 0; count < block_length; ++count)
	    {
		d_val = d_values[count * 2] * bscale + bzero;
		if ( !pio_write_double (channel, d_val) ) return (FALSE);
	    }
	    break;
	}
	num_values -= block_length;
	data += elem_size * block_length;
    }
    if ( !ch_tell (channel, &read_pos, &write_pos) ) return (FALSE);
    remainder = write_pos % CARD_SIZE;
    if (remainder == 0) return (TRUE);
    /*  Pad remainder  */
    for (count = remainder; count < CARD_SIZE; ++count)
    {
	if (ch_write (channel, " ", 1) < 1) return (FALSE);
    }
    return (TRUE);
}   /*  End Function foreign_fits_write_data  */

/*UNPUBLISHED_FUNCTION*/
flag foreign_fits_generate_header (packet_desc **header_pack_desc,
				   char **header_packet,
				   CONST multi_array *multi_desc, ...)
/*  [SUMMARY] Generate a FITS header from a Karma data structure.
    [PURPOSE] This routine generate a FITS header which may later be written to
    a FITS file.
    The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn", "BITPIX",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <header_pack_desc> The packet descriptor for the FITS header is written
    here.
    <header_packet> The packet data for the FITS header is written here.
    <multi_desc> The multi_array descriptor pointer. The routine will find a
    n-dimensional array within the data structure.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_FITS_GENERATE_HEADER_END.
    See [<FOREIGN_ATT_FITS_GENERATE_HEADER>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    va_list argp;
    flag discard_axes_info = FALSE;
    int bitpix, count;
    unsigned int att_key;
    unsigned int elem_type, naxis, axis_length, num_non_degenerate;
    unsigned int dim_index;
    double scale, offset;
    char *top_packet, *ptr, *elem_desc, *data_name;
    unsigned int *dim_to_axis;
    packet_desc *top_pack_desc;
    array_desc *arr_desc;
    dim_desc *dim;
    double value[2];
    double crval[2], cdelta[2], crpix[2];
    char txt[STRING_LENGTH], ctype[STRING_LENGTH];
    static char function_name[] = "foreign_fits_generate_header";

    va_start (argp, multi_desc);
    if ( (multi_desc == NULL) ||
	 (header_pack_desc == NULL) || (header_packet == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) )
	    != FA_FITS_GENERATE_HEADER_END )
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
    /*  Sanity checks on the data  */
    if (multi_desc->num_arrays != 1)
    {
	fprintf (stderr, "Only one data structure allowed!\n");
	a_prog_bug (function_name);
    }
    top_pack_desc = multi_desc->headers[0];
    top_packet = multi_desc->data[0];
    /*  Search for array descriptor: make sure there is only one  */
    for (count = 0, arr_desc = NULL;
	 (count < top_pack_desc->num_elements) && (arr_desc == NULL);
	 ++count)
    {
	if (top_pack_desc->element_types[count] != K_ARRAY) continue;
	/*  Found an array  */
	if (arr_desc != NULL)
	{
	    fprintf (stderr, "Only one n-dimensional array allowed\n");
	    a_prog_bug (function_name);
	}
	arr_desc = (array_desc *) top_pack_desc->element_desc[count];
    }
    if (arr_desc == NULL)
    {
	fprintf (stderr, "No n-dimensional array found\n");
	a_prog_bug (function_name);
    }
    if (arr_desc->packet->num_elements != 1)
    {
	fprintf (stderr, "Only one array element allowed\n");
	a_prog_bug (function_name);
    }
    if (arr_desc->num_levels > 0)
    {
	fprintf (stderr, "Tiled array not supported\n");
	a_prog_bug (function_name);
    }
    elem_type = arr_desc->packet->element_types[0];
    data_name = arr_desc->packet->element_desc[0];
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
    /*  It appears we have a single data structure with a single n-dimensional
	array which has in turn only one non-complex atomic element: good.
	Now construct the packet and descriptor to hold the FITS header
	information  */
    if ( ( *header_pack_desc = ds_alloc_packet_desc (0) ) == NULL )
    {
	m_abort (function_name, "packet descriptor");
    }
    if ( ( *header_packet = m_alloc (0) ) == NULL )
    {
	m_abort (function_name, "packet");
    }
    /*  Ensure "BITPIX" is sensible  */
    if ( ds_get_unique_named_value (top_pack_desc, top_packet, "BITPIX",
				    NULL, value) )
    {
	bitpix = value[0];
    }
    else bitpix = 0;
    switch (elem_type)
    {
      case K_UBYTE:
	if (bitpix == 0) bitpix = 8;
	if (bitpix != 8) bitpix = -32;
	break;
      case K_SHORT:
	if (bitpix == 0) bitpix = 16;
	if (bitpix != 16) bitpix = -32;
	break;
      case K_INT:
	if (bitpix == 0) bitpix = 32;
	if (bitpix != 32) bitpix = -32;
	break;
      case K_DOUBLE:
	bitpix = -64;
	break;
      default:
	bitpix = -32;
	break;
    }
    append_int (*header_pack_desc, header_packet, "BITPIX", bitpix);
    /*  Start processing the supplied header data and check for consistency
	with the n-dimensional array  */
    if ( ( dim_to_axis = (unsigned int *)
	   m_alloc (arr_desc->num_dimensions * sizeof *dim_to_axis) ) == NULL )
    {
	m_abort (function_name, "index translation array");
    }
    if ( ds_get_unique_named_value (top_pack_desc, top_packet, "NAXIS",
				    NULL, value) )
    {
	/*  Supplied "NAXIS" keyword: ensure that number of non-degenerate
	    axes equals number of array dimensions  */
	naxis = value[0];
	num_non_degenerate = 0;
	for (count = 1; count <= naxis; ++count)
	{
	    sprintf (txt, "NAXIS%u", count);
	    if ( !ds_get_unique_named_value (top_pack_desc, top_packet, txt,
					     NULL, value) )
	    {
		/*  Not found  */
		discard_axes_info = TRUE;
		continue;
	    }
	    axis_length = value[0];
	    if (axis_length < 2)
	    {
		/*  Degenerate axis: ignore  */
		continue;
	    }
	    ++num_non_degenerate;
	    /*  Check if this corresponds with a dimension  */
	    sprintf (txt, "CTYPE%u", count);
	    if ( ( ptr = ds_get_unique_named_string (top_pack_desc,
						     top_packet, txt) )
		 == NULL )
	    {
		/*  Not found  */
		discard_axes_info = TRUE;
		continue;
	    }
	    strcpy (ctype, ptr);
	    m_free (ptr);
	    if ( ( dim_index = ds_f_dim_in_array (arr_desc, ctype) )
		 >= arr_desc->num_dimensions )
	    {
		/*  Dimension not found  */
		discard_axes_info = TRUE;
		continue;
	    }
	    dim_to_axis[dim_index] = count;
	    if (axis_length != arr_desc->dimensions[dim_index]->length)
	    {
		/*  Lengths do not match  */
		discard_axes_info = TRUE;
		continue;
	    }
	}
	if (num_non_degenerate != arr_desc->num_dimensions)
	{
	    discard_axes_info = TRUE;
	}
    }
    else
    {
	/*  No "NAXIS" value supplied  */
	naxis = arr_desc->num_dimensions;
	for (count = 0; count < arr_desc->num_dimensions; ++count)
	{
	    dim_to_axis[count] = arr_desc->num_dimensions - count;
	}
    }
    if (discard_axes_info)
    {
	for (count = 0; count < arr_desc->num_dimensions; ++count)
	{
	    dim_to_axis[count] = arr_desc->num_dimensions - count;
	}
    }
    /*  Construct "NAXIS" and some "NAXISn" information manually  */
    append_int (*header_pack_desc, header_packet, "NAXIS", naxis);
    for (count = arr_desc->num_dimensions - 1; count >= 0; --count)
    {
	sprintf (txt, "NAXIS%u", dim_to_axis[count]);
	append_int (*header_pack_desc, header_packet, txt,
		    arr_desc->dimensions[count]->length);
    }
    if (!discard_axes_info)
    {
	/*  There may be other "NAXISn" elements which correspond to degenerate
	    axes. Need to write these next because some FITS readers (e.g.
	    saoimage) are broken and want the axis lengths before almost
	    anything else  */
	for (count = 0; count < top_pack_desc->num_elements; ++count)
	{
	    elem_type = top_pack_desc->element_types[count];
	    elem_desc = top_pack_desc->element_desc[count];
	    if ( !ds_element_is_named (elem_type) ) continue;
	    if (strlen (elem_desc) > EQUALS_POSITION)
	    {
		/*  FITS does not define keywords of more than 8 characters,
		    hence this cannot be represented  */
		continue;
	    }
	    if (strncmp (elem_desc, "NAXIS", 5) != 0) continue;
	    if ( !ds_copy_unique_named_element (*header_pack_desc,
						header_packet,
						top_pack_desc, top_packet,
						elem_desc, TRUE, FALSE,
						FALSE) )
	    {
		m_abort (function_name, "header item");
	    }
	}
    }
    /*  Construct "CTYPE", "CRVAL", "CRPIX" and "CDELT" information manually */
    crval[1] = 0.0;
    crpix[1] = 0.0;
    cdelta[1] = 0.0;
    for (count = arr_desc->num_dimensions - 1; count >= 0; --count)
    {
	dim = arr_desc->dimensions[count];
	/*  Attempt to preserve the "CTYPEn" string if possible  */
	sprintf (txt, "CTYPE%u", dim_to_axis[count]);
	if ( ( ptr = ds_get_unique_named_string (top_pack_desc,
						 top_packet, txt) ) == NULL )
	{
	    /*  Not found: use the dimension name  */
	    strcpy (ctype, dim->name);
	}
	else
	{
	    /*  Found: copy the old value over  */
	    strcpy (ctype, ptr);
	    m_free (ptr);
	}
	/*  Attempt to preserve the "CRVALn" value if possible  */
	sprintf (txt, "CRVAL%u", dim_to_axis[count]);
	if ( !ds_get_unique_named_value (top_pack_desc, top_packet, txt,
					 NULL, crval) )
	{
	    crval[0] = TOOBIG;
	}
	/*  Attempt to preserve the "CRPIXn" value if possible  */
	sprintf (txt, "CRPIX%u", dim_to_axis[count]);
	if ( !ds_get_unique_named_value (top_pack_desc, top_packet, txt,
					 NULL, crpix) )
	{
	    crpix[0] = TOOBIG;
	}
	/*  Attempt to preserve the "CDELTn" value if possible  */
	sprintf (txt, "CDELT%u", dim_to_axis[count]);
	if ( !ds_get_unique_named_value (top_pack_desc, top_packet, txt,
					 NULL, cdelta) )
	{
	    cdelta[0] = TOOBIG;
	}
	if ( (crval[0] >= TOOBIG) || (crpix[0] >= TOOBIG) ||
	     (cdelta[0] >= TOOBIG) )
	{
	    /*  Not all the information was available: construct it  */
	    crval[0] = dim->first_coord;
	    crpix[0] = 1.0;
	    cdelta[0] = (dim->last_coord -
			 dim->first_coord) / (double) (dim->length - 1);
	}
	sprintf (txt, "CTYPE%u", dim_to_axis[count]);
	if ( !ds_put_unique_named_string (*header_pack_desc, header_packet,
					  txt, ctype, FALSE) )
	{
	    m_abort (function_name, txt);
	}
	sprintf (txt, "CRVAL%u", dim_to_axis[count]);
	if ( !ds_put_unique_named_value (*header_pack_desc, header_packet,
					 txt, K_DOUBLE, crval, FALSE) )
	{
	    m_abort (function_name, txt);
	}
	sprintf (txt, "CRPIX%u", dim_to_axis[count]);
	if ( !ds_put_unique_named_value (*header_pack_desc, header_packet,
					 txt, K_DOUBLE, crpix, FALSE) )
	{
	    m_abort (function_name, txt);
	}
	sprintf (txt, "CDELT%u", dim_to_axis[count]);
	if ( !ds_put_unique_named_value (*header_pack_desc, header_packet,
					 txt, K_DOUBLE, cdelta, FALSE) )
	{
	    m_abort (function_name, txt);
	}
    }
    /*  Get any data scaling  */
    sprintf (txt, "%s__SCALE", data_name);
    if ( ds_get_unique_named_value (top_pack_desc, top_packet, txt,
				    NULL, value) )
    {
	scale = value[0];
    }
    else scale = 1.0;
    sprintf (txt, "%s__OFFSET", data_name);
    if ( ds_get_unique_named_value (top_pack_desc, top_packet, txt,
				    NULL, value) )
    {
	offset = value[0];
    }
    else offset = 0.0;
    if ( ( (scale != 1.0) || (offset != 0.0) ) &&
	 ( (bitpix != -32) && (bitpix != -64) ) )
    {
	/*  Add scaling information  */
	value[0] = scale;
	value[1] = 0.0;
	if ( !ds_put_unique_named_value (*header_pack_desc, header_packet,
					 "BSCALE", K_DOUBLE, value, FALSE) )
	{
	    m_abort (function_name, txt);
	}
	value[0] = offset;
	value[1] = 0.0;
	if ( !ds_put_unique_named_value (*header_pack_desc, header_packet,
					 "BZERO", K_DOUBLE, value, FALSE) )
	{
	    m_abort (function_name, txt);
	}
    }
    /*  Write the data name  */
    if ( !ds_put_unique_named_string (*header_pack_desc, header_packet,
				      "BUNIT", data_name, FALSE) )
    {
	m_abort (function_name, "BUNIT");
    }
    /*  Copy (with possible discards) header information from supplied header
     */
    for (count = 0; count < top_pack_desc->num_elements; ++count)
    {
	elem_type = top_pack_desc->element_types[count];
	elem_desc = top_pack_desc->element_desc[count];
	if ( !ds_element_is_named (elem_type) ) continue;
	if (strlen (elem_desc) > EQUALS_POSITION)
	{
	    /*  FITS does not define keywords of more than 8 characters, hence
		this cannot be represented  */
	    continue;
	}
	if (strcmp (elem_desc, "SIMPLE") == 0) continue;
	if (strcmp (elem_desc, "BITPIX") == 0) continue;
	if (strncmp (elem_desc, "NAXIS", 5) == 0) continue;
	if (strcmp (elem_desc, "BSCALE") == 0) continue;
	if (strcmp (elem_desc, "BZERO") == 0) continue;
	if (strcmp (elem_desc, "BUNIT") == 0) continue;
	if (strcmp (elem_desc, "BLANK") == 0) continue;
	if (discard_axes_info)
	{
	    if (strncmp (elem_desc, "CTYPE", 5) == 0) continue;
	    if (strncmp (elem_desc, "CRVAL", 5) == 0) continue;
	    if (strncmp (elem_desc, "CRPIX", 5) == 0) continue;
	    if (strncmp (elem_desc, "CDELT", 5) == 0) continue;
	    if (strncmp (elem_desc, "CUNIT", 5) == 0) continue;
	    if (strncmp (elem_desc, "CROTA", 5) == 0) continue;
	}
	if ( !ds_copy_unique_named_element (*header_pack_desc, header_packet,
					    top_pack_desc, top_packet,
					    elem_desc, TRUE, FALSE, FALSE) )
	{
	    m_abort (function_name, "header item");
	}
    }
    return (TRUE);
}   /*  End Function foreign_fits_generate_header  */


/*  Private functions follow  */

static flag fits_write (Channel channel, multi_array *multi_desc, va_list argp)
/*  [SUMMARY] Write FITS file to a channel.
    [PURPOSE] This routine will write a Karma data structure to a FITS file.
    The routine will automatically generate essential keywords such as:
    "NAXIS", "NAXISn", "CTYPEn", "CRVALn", "CRPIXn", "CDELTn", "BITPIX",
    "BUNIT", "BSCALE" and "BZERO". Also, existing keywords in the data
    structure which do not conflict with the generated keywords are copied.
    <channel> The channel to write to. The channel is not flushed.
    <multi_desc> The multi_array descriptor pointer. The routine will find a
    n-dimensional array within the data structure.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with FA_FITS_WRITE_END.
    See [<FOREIGN_ATT_FITS_WRITE>] for a list of defined attributes.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    flag ok;
    unsigned int att_key;
    char *header_packet;
    packet_desc *header_pack_desc;
    static char function_name[] = "fits_write";

    if ( (channel == NULL) || (multi_desc == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_FITS_WRITE_END )
    {
	switch (att_key)
	{
	  default:
	    fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    if ( !foreign_fits_generate_header (&header_pack_desc, &header_packet,
					multi_desc,
					FA_FITS_GENERATE_HEADER_END) )
    {
	return (FALSE);
    }
    if ( !write_fits_header (channel, header_pack_desc, header_packet,
			     multi_desc->first_hist) )
    {
	ds_dealloc_packet (header_pack_desc, header_packet);
	return (FALSE);
    }
    ok = foreign_fits_write_data (channel, multi_desc,
				  header_pack_desc, header_packet, NULL, 0,
				  FA_FITS_WRITE_DATA_END);
    ds_dealloc_packet (header_pack_desc, header_packet);
    return (ok);
}   /*  End Function fits_write  */

static void append_int (packet_desc *desc, char **packet, CONST char *name,
			int value)
/*  [SUMMARY] Append named integer to a packet.
    <desc> The packet descriptor to add the name to. This descriptor will be
    modified.
    <packet> The pointer to the unique packet. Note that the existing packet
    data is copied to a new packet, and a pointer to this packet is written
    back here.
    <name> The name of the element.
    <value> The value of the data.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    double d_val[2];
    static char function_name[] = "append_int";

    d_val[0] = value;
    d_val[1] = 0.0;
    if ( !ds_put_unique_named_value (desc, packet, name, K_INT,
				     d_val, FALSE) )
    {
	m_abort (function_name, "new value");
    }
}   /*  End Function append_int  */

static flag write_fits_header (Channel channel, CONST packet_desc *header_desc,
			       CONST char *header_packet,
			       CONST history *first_hist)
/*  [SUMMARY] Write a FITS header to a channel object.
    <channel> The channel to write to. The channel is not flushed.
    <header_desc> The header packet descriptor.
    <header_packet> The header packet data.
    <first_hist> The first history string.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    uaddr elem_offset;
    flag ok, complex;
    unsigned int count, type, remainder;
    unsigned long read_pos, start_write_pos, end_write_pos;
    char *elem_name, *ptr;
    char txt[STRING_LENGTH];
    double value[2];
    static char function_name[] = "write_fits_header";

    if ( !ch_tell (channel, &read_pos, &start_write_pos) ) return (FALSE);
    if ( !write_fits_header_line (channel, "SIMPLE  = T") ) return (FALSE);
    for (count = 0; count < header_desc->num_elements; ++count)
    {
	type = header_desc->element_types[count];
	elem_offset = ds_get_element_offset (header_desc, count);
	elem_name = header_desc->element_desc[count];
	if ( ds_element_is_atomic (type) )
	{
	    ok = ds_get_element (header_packet + elem_offset, type, value,
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
	switch (type)
	{
	  case K_FLOAT:
	  case K_DOUBLE:
	    sprintf (txt, "%-8s= %.12e", elem_name, value[0]);
	    break;
	  case K_VSTRING:
	    sprintf (txt, "%-8s= '%s'", elem_name, ptr);
	    m_free (ptr);
	    break;
	  default:
	    sprintf (txt, "%-8s= %ld", elem_name, (signed long) value[0]);
	    break;
	}
	if ( !write_fits_header_line (channel, txt) ) return (FALSE);
    }
    /*  Write any history  */
    while (first_hist != NULL)
    {
	sprintf (txt, "HISTORY %s", first_hist->string);
	if ( !write_fits_header_line (channel, txt) ) return (FALSE);
	first_hist = first_hist->next;
    }
    /*  Write "END" and pad the remainder  */
    if ( !write_fits_header_line (channel, "END") ) return (FALSE);
    if ( !ch_tell (channel, &read_pos, &end_write_pos) ) return (FALSE);
    remainder = (end_write_pos - start_write_pos) % CARD_SIZE;
    if (remainder == 0) return (TRUE);
    /*  Pad remainder  */
    for (count = remainder; count < CARD_SIZE; ++count)
    {
	if (ch_write (channel, " ", 1) < 1) return (FALSE);
    }
    return (TRUE);
}   /*  End Function write_fits_header  */

static flag write_fits_header_line (Channel channel, CONST char *line)
/*  [SUMMARY] Write a FITS header line to a channel object.
    <channel> The channel to write to. The channel is not flushed.
    <line> The line to write. This is padded to 80 characters.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int len, count;
    char txt[STRING_LENGTH];

    len = strlen (line);
    strncpy (txt, line, CARD_WIDTH);
    for (count = len; count < CARD_WIDTH; ++count) txt[count] = ' ';
    if (ch_write (channel, txt, CARD_WIDTH) < CARD_WIDTH) return (FALSE);
    return (TRUE);
}   /*  End Function write_fits_header_line  */
