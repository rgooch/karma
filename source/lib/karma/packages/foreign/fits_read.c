/*LINTLIBRARY*/
/*  fits_read.c

    This code provides a FITS read facility.

    Copyright (C) 1995-1996  Richard Gooch

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

    This file contains the various utility routines for reading data files in
  FITS format.


    Written by      Richard Gooch   17-APR-1995: Copied skeleton from
  ppm_read.c  and guts from  modules/fits/fits2karma.c

    Updated by      Richard Gooch   20-APR-1995

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   17-MAY-1995: Fixed bug when BUNIT is an
  empty string.

    Updated by      Richard Gooch   1-JUN-1995: Tolerate header lines with
  tab in first column: print warning and skip them.

    Updated by      Richard Gooch   4-JUN-1995: Allow for floating point values
  with no 'E', only '.' character.

    Updated by      Richard Gooch   18-JUL-1995: Translate '\0' characters to
  ' ' characters.

    Updated by      Richard Gooch   31-JUL-1995: Made use of
  <pio_read_float_nantrap> and <pio_read_double_nantrap> and junked massive
  #if blocks testing for NaN.

    Updated by      Richard Gooch   29-FEB-1996: Cope with multiple instances
  of keywords if at all possible.

    Updated by      Richard Gooch   12-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   3-JUN-1996: No longer fiddle reversed axes.

    Updated by      Richard Gooch   15-JUN-1996: Add *__SCALE and *__OFFSET
  keywords if necessary.

    Updated by      Richard Gooch   17-JUN-1996: Subtracted 1.0 from CRPIXn
  values.

    Updated by      Richard Gooch   24-JUN-1996: Trapped "NAXISn" and other
  "?????n" keywords coming before "NAXIS" keyword.

    Updated by      Richard Gooch   6-AUG-1996: Copied "CRVALn" value to
  reference field of dimension descriptor and now add header keywords in the
  same order as in the FITS file. Now also copy history.

    Updated by      Richard Gooch   15-AUG-1996: No longer update <<reference>>
  field of dimension descriptor.

    Updated by      Richard Gooch   29-AUG-1996: Allowed some axes to not have
  names but still use as many supplied names as possible. Fail if an axis does
  not have length specified. Issue warnings if CRVAL, CRPIX or CDELT not found.

    Updated by      Richard Gooch   5-SEP-1996: Made use of <pio_read_floats>.

    Updated by      Richard Gooch   7-SEP-1996: Made use of <pio_read_doubles>.

    Updated by      Richard Gooch   25-SEP-1996: Changed to implement the new
  FITS-style co-ordinate handling, where dimension co-ordinates range from 0
  to length - 1.

    Last updated by Richard Gooch   15-OCT-1996: No longer throw away last
  column in each line of the header.


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


struct keyword_type
{
    char *keyword;
    char *value;
    struct keyword_type *next;
};

struct card_info_type
{
    double bzero;
    double bscale;
    int bitpix;
    long blank;
    flag have_blank;
    int naxis;
    int *dim_lengths;
    char **dim_names;  /*  Trailing whitespace is removed      */
    double *crpix;     /*  1.0 is substracted from FITS value  */
    double *crval;
    double *cdelta;
    char *elem_name;
    flag simple;
    flag end_found;
    struct keyword_type *first_keyword;
    struct keyword_type *last_keyword;
    multi_array *history_holder;
};

typedef struct
{
    double bzero;
    double bscale;
    long blank;
    flag have_blank;
    int bitpix;
} KforeignFITSinfo;


/*  Declarations of private functions follow  */
STATIC_FUNCTION (flag read_card,
		 (Channel channel, char card[CARD_LENGTH][CARD_WIDTH]) );
STATIC_FUNCTION (flag extract_info_from_header,
		 (char card[CARD_LENGTH][CARD_WIDTH],
		  struct card_info_type *info,
		  flag strip_trailing_whitespace) );
STATIC_FUNCTION (void redeclaration_message, (char *string) );
STATIC_FUNCTION (void conversion_error_message,
		 (char *parameter, char *value) );
STATIC_FUNCTION (void cleanup_string, (char *string) );
STATIC_FUNCTION (void free_keywords, (struct keyword_type *keywords) );
STATIC_FUNCTION (flag process_keywords,
		 (struct keyword_type *keywords, multi_array *multi_desc) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
multi_array *foreign_fits_read_header (Channel channel, flag data_alloc,
				       flag convert_int_to_float,
				       flag sanitise, ...)
/*  [SUMMARY] Read a FITS header.
    [PURPOSE] This routine will read the header of a FITS file from a channel.
    The data section is NOT read.
    <channel> The channel to read from.
    <data_alloc> If TRUE, the data space is allocated.
    <convert_int_to_float> If TRUE, integer FITS data is converted to floating
    point data.
    <sanitise> If TRUE, FITS axes with length 1 are ignored. This is highly
    recommended.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_FITS_READ_HEADER_END. See [<FOREIGN_ATT_FITS_READ_HEADER>] for a list of
    defined attributes.
    [RETURNS] A pointer to the multi_array data structure on success, else
    NULL.
*/
{
    va_list argp;
    flag read_another_header_card = TRUE;
    unsigned int att_key;
    unsigned int num_dim;
    unsigned int dim_count;
    unsigned int elem_type;
    unsigned int card_count;
    double value[2];
    struct card_info_type finfo;
    char dummy_array;
    char txt[STRING_LENGTH];
    char card[CARD_LENGTH][CARD_WIDTH];
    char *array;
    uaddr *dim_lengths;
    char **dim_names;
    multi_array *multi_desc;
    static char def_elem_name[] = "Data Value";
    static char function_name[] = "foreign_fits_read_header";

    va_start (argp, sanitise);
    if (channel == NULL)
    {
	fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (data_alloc);
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) )
	   != FA_FITS_READ_HEADER_END )
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
    if (data_alloc) array = NULL;
    else array = &dummy_array;
    /*  Initialise card info  */
    finfo.bitpix = 0;
    finfo.end_found = FALSE;
    finfo.simple = 2 * abs (TRUE) + 2 * abs (FALSE);
    finfo.naxis = -1;
    finfo.dim_lengths = NULL;
    finfo.crpix = NULL;
    finfo.crval = NULL;
    finfo.cdelta = NULL;
    finfo.dim_names = NULL;
    finfo.elem_name = NULL;
    finfo.bzero = TOOBIG;
    finfo.bscale = TOOBIG;
    finfo.have_blank = FALSE;
    finfo.first_keyword = NULL;
    finfo.last_keyword = NULL;
    if ( ( finfo.history_holder = ds_alloc_multi (1) ) == NULL )
    {
	m_abort (function_name, "history holder");
    }
    card_count = 0;
    /*  Read header cards  */
    while (read_another_header_card)
    {
	if ( !read_card (channel, card) )
	{
	    fprintf (stderr, "Error reading another header card\n");
	    free_keywords (finfo.first_keyword);
	    ds_dealloc_multi (finfo.history_holder);
	    return (NULL);
	}
	++card_count;
	if ( !extract_info_from_header (card, &finfo, TRUE) )
	{
	    fprintf (stderr,"Error extracting information from header card\n");
	    free_keywords (finfo.first_keyword);
	    ds_dealloc_multi (finfo.history_holder);
	    return (NULL);
	}
	if (finfo.end_found)
	{
	    read_another_header_card = FALSE;
	}
    }
    if (!finfo.simple)
    {
	fprintf (stderr, "Can only read simple FITS files\n");
	free_keywords (finfo.first_keyword);
	ds_dealloc_multi (finfo.history_holder);
	return (NULL);
    }
    if (finfo.bscale == TOOBIG) finfo.bscale = 1.0;
    if (finfo.bzero == TOOBIG) finfo.bzero = 0.0;
    if (finfo.elem_name == NULL) finfo.elem_name = def_elem_name;
    if ( (finfo.elem_name != NULL) && (strlen (finfo.elem_name) < 1) )
    {
	fprintf (stderr, "WARNING: BUNIT is an empty string: defaulting to ");
	fprintf (stderr, "\"%s\"\n", def_elem_name);
	m_free (finfo.elem_name);
	finfo.elem_name = def_elem_name;
    }
    /*  Determine output data type  */
    switch (finfo.bitpix)
    {
      case 8:
	if (convert_int_to_float) elem_type = K_FLOAT;
	else elem_type = K_UBYTE;
	break;
      case 16:
	if (convert_int_to_float) elem_type = K_FLOAT;
	else elem_type = K_SHORT;
	break;
      case 32:
	elem_type = convert_int_to_float ? K_FLOAT : K_INT;
	break;
      case -32:
	elem_type = K_FLOAT;
	break;
      case -64:
	elem_type = K_DOUBLE;
	break;
      default:
	fprintf (stderr, "Illegal value for BITPIX: %d\n", finfo.bitpix);
	free_keywords (finfo.first_keyword);
	ds_dealloc_multi (finfo.history_holder);
	return (NULL);
/*
	break;
*/
    }
    if (sanitise)
    {
	/*  Strip dimensions with length of 1  */
	for (dim_count = 0, num_dim = 0; dim_count < finfo.naxis; ++dim_count)
	{
	    if (finfo.dim_lengths[dim_count] > 1) ++num_dim;
	}
    }
    else
    {
	/*  All dimensions must be allocated  */
	num_dim = finfo.naxis;
    }
    /*  Create array of dimension lengths  */
    if ( ( dim_lengths = (uaddr *) m_alloc (sizeof *dim_lengths * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "dimension lengths");
	free_keywords (finfo.first_keyword);
	ds_dealloc_multi (finfo.history_holder);
	return (NULL);
    }
    /*  Create array of dimension name pointers  */
    if ( ( dim_names = (char **) m_alloc (sizeof *dim_names * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "dimension name pointers");
	m_free ( (char *) dim_lengths );
	free_keywords (finfo.first_keyword);
	ds_dealloc_multi (finfo.history_holder);
	return (NULL);
    }
    for (dim_count = 0, num_dim = 0; dim_count < finfo.naxis; ++dim_count)
    {
	if ( sanitise && (finfo.dim_lengths[dim_count] < 2) )
	{
	    /*  Dimension length is 1: skip  */
	    continue;
	}
	if (finfo.dim_lengths[dim_count] < 0)
	{
	    fprintf (stderr, "NAXIS%u length: %d invalid\n",
		     finfo.naxis - dim_count, finfo.dim_lengths[dim_count]);
	    m_free ( (char *) dim_lengths );
	    m_free ( (char *) dim_names );
	    free_keywords (finfo.first_keyword);
	    ds_dealloc_multi (finfo.history_holder);
	    return (NULL);
	}
	if (finfo.crval[dim_count] >= TOOBIG)
	{
	    fprintf (stderr,
		     "WARNING: CRVAL%u not specified: defaulting to 1\n",
		     finfo.naxis - dim_count);
	    finfo.crval[dim_count] = 1.0;
	}
	if (finfo.crpix[dim_count] >= TOOBIG)
	{
	    fprintf (stderr,
		     "WARNING: CRPIX%u not specified: defaulting to 1\n",
		     finfo.naxis - dim_count);
	    finfo.crpix[dim_count] = 0.0;
	}
	if (finfo.cdelta[dim_count] >= TOOBIG)
	{
	    fprintf (stderr,
		     "WARNING: CDELT%u not specified: defaulting to 1\n",
		     finfo.naxis - dim_count);
	    finfo.cdelta[dim_count] = 1.0;
	}
	dim_lengths[num_dim] = finfo.dim_lengths[dim_count];
	dim_names[num_dim] = finfo.dim_names[dim_count];
	++num_dim;
    }
    /*  Now allocate the Karma data structure descriptors  */
    if ( ( multi_desc = ds_wrap_preallocated_n_element_array
	   (array, num_dim, dim_lengths, NULL, NULL,
	    NULL, (CONST char **) dim_names, 1, &elem_type,
	    (CONST char **) &finfo.elem_name) ) == NULL )
    {
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_names );
	free_keywords (finfo.first_keyword);
	ds_dealloc_multi (finfo.history_holder);
	return (NULL);
    }
    m_free ( (char *) dim_lengths );
    m_free ( (char *) dim_names );
    if (!data_alloc)
    {
	*(char **) multi_desc->data[0] = NULL;
    }
    /*  Add element scaling information if necessary. If the output type is
	floating point, the scaling information has already been applied  */
    if ( (elem_type != K_FLOAT) && (elem_type != K_DOUBLE) )
    {
	/*  Take bzero and bscale information  */
	if ( (finfo.bzero != 0.0) && (finfo.bscale != 1.0) )
	{
	    sprintf (txt, "%s__SCALE", finfo.elem_name);
	    value[0] = finfo.bscale;
	    value[1] = 0.0;
	    if ( !ds_put_unique_named_value (multi_desc->headers[0],
					     &multi_desc->data[0],
					     txt, K_DOUBLE,
					     value, FALSE) )
	    {
		free_keywords (finfo.first_keyword);
		ds_dealloc_multi (finfo.history_holder);
		ds_dealloc_multi (multi_desc);
		return (NULL);
	    }
	    sprintf (txt, "%s__OFFSET", finfo.elem_name);
	    value[0] = finfo.bzero;
	    value[1] = 0.0;
	    if ( !ds_put_unique_named_value (multi_desc->headers[0],
					     &multi_desc->data[0],
					     txt, K_DOUBLE,
					     value, FALSE) )
	    {
		free_keywords (finfo.first_keyword);
		ds_dealloc_multi (finfo.history_holder);
		ds_dealloc_multi (multi_desc);
		return (NULL);
	    }
	}
    }
    /*  Add FITS keywords to top-level packet  */
    if ( !process_keywords (finfo.first_keyword, multi_desc) )
    {
	free_keywords (finfo.first_keyword);
	ds_dealloc_multi (finfo.history_holder);
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    free_keywords (finfo.first_keyword);
    finfo.first_keyword = NULL;
    /*  Copy over any history  */
    multi_desc->first_hist = finfo.history_holder->first_hist;
    multi_desc->last_hist = finfo.history_holder->last_hist;
    /*  Deallocate temporary container for history  */
    finfo.history_holder->first_hist = NULL;
    finfo.history_holder->last_hist = NULL;
    ds_dealloc_multi (finfo.history_holder);
    return (multi_desc);
}   /*  End Function foreign_fits_read_header  */

/*PUBLIC_FUNCTION*/
flag foreign_fits_read_data (Channel channel, multi_array *multi_desc,
			     char *data, uaddr num_values, ...)
/*  [SUMMARY] Read data in a FITS file.
    [PURPOSE] This routine will read the data of a FITS file from a channel.
    The header section is NOT read.
    <channel> The channel to read from.
    <multi_desc> The Karma data structure to write the data into.
    <data> An alternate data array to write the FITS data into. If this is
    NULL, the routine will write the data into the Karma data structure.
    <num_values> The number of values to write into the data array. This is
    only used when data is not NULL.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_FITS_READ_DATA_END. See [<FOREIGN_ATT_FITS_READ_DATA>] for a list of
    defined attributes.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    KforeignFITSinfo finfo;
    uaddr toobig_count = 0;
    long long_data;
    unsigned char uchar_data;
    unsigned int att_key;
    unsigned int value_count;
    unsigned int elem_type, elem_size;
    unsigned int type;
    float f_toobig = TOOBIG;
    double value[2];
    char *packet;
    unsigned long *blank_count = NULL;
    float f_data;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    extern char *sys_errlist[];
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "foreign_fits_read_data";

    va_start (argp, num_values);
    if ( (channel == NULL) || (multi_desc == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp,unsigned int) ) != FA_FITS_READ_DATA_END )
    {
	switch (att_key)
	{
	  case FA_FITS_READ_DATA_NUM_BLANKS:
	    blank_count = va_arg (argp, unsigned long *);
	    break;
	  default:
	    fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    pack_desc = multi_desc->headers[0];
    packet = multi_desc->data[0];
    if (pack_desc->element_types[0] != K_ARRAY)
    {
	fprintf (stderr,"First element in top level packet must be K_ARRAY\n");
	a_prog_bug (function_name);
    }
    arr_desc = (array_desc *) pack_desc->element_desc[0];
    elem_type = arr_desc->packet->element_types[0];
    elem_size = host_type_sizes[elem_type];
    if (data == NULL)
    {
	data = *(char **) packet;
	if (data == NULL)
	{
	    fprintf (stderr, "No array to write data into!\n");
	    a_prog_bug (function_name);
	}
	num_values = ds_get_array_size (arr_desc);
    }
    if ( ds_get_unique_named_value (pack_desc, packet, "BSCALE", &type,value) )
    {
	finfo.bscale = value[0];
    }
    else finfo.bscale = 1.0;
    if ( ds_get_unique_named_value (pack_desc, packet, "BZERO", &type, value) )
    {
	finfo.bzero = value[0];
    }
    else finfo.bzero = 0.0;
    if ( ds_get_unique_named_value (pack_desc, packet, "BLANK", &type, value) )
    {
	finfo.blank = value[0];
	finfo.have_blank = TRUE;
    }
    else finfo.have_blank = FALSE;
    if ( ds_get_unique_named_value (pack_desc, packet, "BITPIX", &type,value) )
    {
	finfo.bitpix = value[0];
    }
    else
    {
	fprintf (stderr, "BITPIX unknown!\n");
	a_prog_bug (function_name);
    }
    /*  Copy data  */
    if (finfo.bitpix == -32)
    {
	if (pio_read_floats (channel, num_values, (float *) data,
			     &toobig_count) < num_values)
	{
	    fprintf (stderr, "Error reading FITS file\n");
	    return (FALSE);
	}
	if (blank_count != NULL) *blank_count = toobig_count;
	return (TRUE);
    }
    if (finfo.bitpix == -64)
    {
	if (pio_read_doubles (channel, num_values, (double *) data,
			      &toobig_count) < num_values)
	{
	    fprintf (stderr, "Error reading FITS file\n");
	    return (FALSE);
	}
	if (blank_count != NULL) *blank_count = toobig_count;
	return (TRUE);
    }
    for (value_count = 0; value_count < num_values;
	 ++value_count, data += elem_size)
    {
	switch (finfo.bitpix)
	{
#ifdef OBSOLETE
	  case -32:
	    if ( !pio_read_float_nantrap (channel, &f_data, &was_nan) )
	    {
		fprintf (stderr, "Error reading FITS file\n");
		return (FALSE);
	    }
	    if (was_nan)
	    {
		f_data = f_toobig;
		++toobig_count;
	    }
	    else
	    {
		f_data = finfo.bzero + f_data * finfo.bscale;
	    }
	    *(float *) data = f_data;
	    break;
	  case -64:
	    if ( !pio_read_double_nantrap (channel, &d_data, &was_nan) )
	    {
		fprintf (stderr, "Error reading FITS file\n");
		return (FALSE);
	    }
	    if (was_nan)
	    {
		d_data = d_toobig;
		++toobig_count;
	    }
	    else
	    {
		d_data = finfo.bzero + d_data * finfo.bscale;
	    }
	    *(double *) data = d_data;
	    break;
#endif
	  case 8:
	    if (ch_read (channel, (char *) &uchar_data, 1) < 1)
	    {
		fprintf (stderr, "Error reading from file\t%s\n",
			 sys_errlist[errno]);
		fprintf (stderr, "Read in: %u points\n", value_count);
		return (FALSE);
	    }
	    if (elem_type == K_UBYTE) *(unsigned char *) data = uchar_data;
	    else
	    {
		/*  Convert to float  */
		if ( finfo.have_blank && (uchar_data == finfo.blank) )
		{
		    f_data = f_toobig;
		    ++toobig_count;
		}
		else
		{
		    f_data = (float) uchar_data * finfo.bscale + finfo.bzero;
		}
		*(float *) data = f_data;
	    }
	    break;
	  case 16:
	    if ( !pio_read16s (channel, &long_data) )
	    {
		fprintf (stderr, "Error reading from file\t%s\n",
			 sys_errlist[errno]);
		fprintf (stderr, "Read in: %u points\n", value_count);
		return (FALSE);
	    }
	    if (elem_type == K_SHORT)
	    {
		if ( finfo.have_blank && (long_data == finfo.blank) )
		{
		    long_data = -32768;
		    ++toobig_count;
		}
		*(short *) data = long_data;
	    }
	    else
	    {
		/*  Convert to float  */
		if ( finfo.have_blank && (long_data == finfo.blank) )
		{
		    f_data = f_toobig;
		    ++toobig_count;
		}
		else
		{
		    f_data = (float) long_data * finfo.bscale + finfo.bzero;
		}
		*(float *) data = f_data;
	    }
	    break;
	  case 32:
	    if ( !pio_read32s (channel, &long_data) )
	    {
		fprintf (stderr, "Error reading from file\t%s\n",
			 sys_errlist[errno]);
		fprintf (stderr, "Read in: %u points\n", value_count);
		return (FALSE);
	    }
	    if (elem_type == K_INT)
	    {
		if ( finfo.have_blank && (long_data == finfo.blank) )
		{
		    long_data = 0x80000000;
		    ++toobig_count;
		}
		*(int *) data = long_data;
	    }
	    else
	    {
		/*  Convert to float  */
		if ( finfo.have_blank && (long_data == finfo.blank) )
		{
		    f_data = f_toobig;
		    ++toobig_count;
		}
		else
		{
		    f_data = (float) long_data * finfo.bscale + finfo.bzero;
		}
		*(float *) data = f_data;
	    }
	    break;
	  default:
	    fprintf (stderr, "Illegal value for BITPIX: %u\n", finfo.bitpix);
	    a_prog_bug (function_name);
	    break;
	}
    }
    if (blank_count != NULL) *blank_count = toobig_count;
    return (TRUE);
}   /*  End Function foreign_fits_read_data  */


/*  Private functions follow  */

static flag read_card (Channel channel, char card[CARD_LENGTH][CARD_WIDTH])
/*  This routine will read a single card of data from a FITS file.
    The file channel must be given by  channel  .
    The storage to write the card data must be pointed to by  card  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    extern char *sys_errlist[];

    if (ch_read (channel, (char *) card, CARD_WIDTH * CARD_LENGTH) !=
	CARD_WIDTH * CARD_LENGTH)
    {
	fprintf (stderr, "Error reading: %u bytes\t%s\n",
		 CARD_WIDTH * CARD_LENGTH, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function read_card  */

static flag extract_info_from_header (char card[CARD_LENGTH][CARD_WIDTH],
				      struct card_info_type *info,
				      flag strip_trailing_whitespace)
/*  This routine will extract information from a single FITS card.
    The card must be pointed to by  card  .
    The information extracted will be written to the storage pointed to by
    info  .
    If the flag  strip_trailing_whitespace  is TRUE, then converted strings
    with trailing whitespace will have the whitespace removed.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    flag quote;
    flag keep_searching;
    flag blank_keyword;
    flag redeclared_keyword;
    char *char_ptr;
    char *value_ptr;
    unsigned int line_count, char_count;
    int dim_count;
    char hist_str[STRING_LENGTH];
    char line[STRING_LENGTH];
    struct keyword_type *new_keyword, *curr;
    static char function_name[] = "extract_info_from_header";

    for (line_count = 0; line_count < CARD_LENGTH; ++line_count)
    {
	redeclared_keyword = FALSE;
	m_copy (line, card[line_count], CARD_WIDTH);
	line[CARD_WIDTH] = '\0';
	/*  Change any '\0' characters to ' '  */
	for (char_count = 0; char_count < CARD_WIDTH - 1; ++char_count)
	{
	    if (line[char_count] == '\0') line[char_count] = ' ';
	}
	/*  Strip trailing whitespace  */
	for (char_ptr = line + CARD_WIDTH - 1;
	     isspace (*char_ptr) && (char_ptr >= line); --char_ptr)
	{
	    *char_ptr = '\0';
	}
	if (line[0] == '\0') continue;
	if (strncmp (line, "END", 3) == 0)
	{
	    info->end_found = TRUE;
	    return (TRUE);
	}
	if (strncmp (line, "HISTORY", 7) == 0)
	{
	    /*  Append history. First clean up line  */
	    char_ptr = line + 7;
	    /*  Skip leading whitespace  */
	    while ( isspace (*char_ptr) ) ++char_ptr;
	    strncpy (hist_str, char_ptr, line + CARD_WIDTH - char_ptr);
	    hist_str[line + CARD_WIDTH - char_ptr] = '\0';
	    /*  Remove comment  */
	    for (quote = FALSE, char_count = 0; char_count < CARD_WIDTH;
		 ++char_count)
	    {
		switch (hist_str[char_count])
		{
		  case '/':
		    if (!quote) hist_str[char_count] = '\0';
		    break;
		  case '\'':
		    quote = quote ? FALSE : TRUE;
		    break;
		}
	    }
	    if ( !ds_history_append_string (info->history_holder, hist_str,
					    TRUE) )
	    {
		m_abort (function_name, "history string");
	    }
	    continue;
	}
	if (strncmp (line, "COMMENT", 7) == 0) continue;
	blank_keyword = TRUE;
	for (char_count = 0; blank_keyword && (char_count < 8); ++char_count)
	{
	    if ( !isspace (line[char_count]) ) blank_keyword = FALSE;
	}
	if (blank_keyword) continue;
	if (line[0] == '\t')
	{
	    fprintf (stderr, "WARNING: tab in first column: ignoring line\n");
	    continue;
	}
	/*  Tidy up line  */
	/*  Remove '=' and any whitespace before '='  */
	if (line[EQUALS_POSITION] != '=')
	{
	    line[CARD_WIDTH] = '\0';
	    fprintf (stderr,
		     "WARNING: Expected '=' at column: %u in line: \"%s\"\n",
		     EQUALS_POSITION, line);
	    fprintf (stderr, "Skipping line\n");
	    continue;
	}
	line[EQUALS_POSITION] = '\0';
	char_ptr = line + EQUALS_POSITION - 1;
	while ( isspace (*char_ptr) )
	{
	    *char_ptr-- = '\0';
	}
	/*  Search for value field  */
	value_ptr = line + EQUALS_POSITION + 1;
	while ( isspace (*value_ptr) ) ++value_ptr;
	/*  Search for '/' (comment) or '\'' (quote) character  */
	char_ptr = value_ptr;
	quote = FALSE;
	keep_searching = TRUE;
	while (keep_searching)
	{
	    switch (*char_ptr)
	    {
	      case '/':
		/*  Comment  */
		if (quote)
		{
		    /*  Ignore  */
		    ++char_ptr;
		}
		else
		{
		    /*  Comment (unquoted)  */
		    keep_searching = FALSE;
		}
		break;
	      case '\'':
		quote = quote ? FALSE : TRUE;
		++char_ptr;
		break;
	      default:
		++char_ptr;
		break;
	    }
	    if (char_ptr - line >= CARD_WIDTH) keep_searching = FALSE;
	}
	if (*char_ptr == '/')
	{
	    /*  Found comment: kill it  */
	    *char_ptr = '\0';
	}
	/*  Remove trailing whitespace  */
	--char_ptr;
	while ( isspace (*char_ptr) )
	{
	    *char_ptr-- = '\0';
	}
	/*  Now start extacting info  */
	if (strcmp (line, "SIMPLE") == 0)
	{
	    if ( (info->simple == TRUE) || (info->simple == FALSE) )
	    {
		redeclaration_message (line);
		redeclared_keyword = TRUE;
		continue;
	    }
	    if (*value_ptr == 'T')
	    {
		info->simple = TRUE;
	    }
	    else if (*value_ptr == 'F')
	    {
		info->simple = FALSE;
	    }
	    else
	    {
		conversion_error_message (line, value_ptr);
		info->simple = FALSE;
		continue;
	    }
	}
	else if (strcmp (line, "BITPIX") == 0)
	{
	    if (info->bitpix != 0)
	    {
		redeclaration_message (line);
		redeclared_keyword = TRUE;
	    }
	    if (sscanf (value_ptr, "%d", &info->bitpix) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	}
	else if (strcmp (line, "BZERO") == 0)
	{
	    if (info->bzero != TOOBIG)
	    {
		redeclaration_message (line);
		redeclared_keyword = TRUE;
	    }
	    if (sscanf (value_ptr, "%le", &info->bzero) != 1)
	    {
		conversion_error_message (line, value_ptr);
		info->bzero = 0.0;
		continue;
	    }
	}
	else if (strcmp (line, "BSCALE") == 0)
	{
	    if (info->bscale != TOOBIG)
	    {
		redeclaration_message (line);
		redeclared_keyword = TRUE;
	    }
	    if (sscanf (value_ptr, "%le", &info->bscale) != 1)
	    {
		conversion_error_message (line, value_ptr);
		info->bscale = 1.0;
	    }
	}
	else if (strcmp (line, "BUNIT") == 0)
	{
	    if (info->elem_name != NULL)
	    {
		redeclaration_message (line);
		redeclared_keyword = TRUE;
	    }
	    if ( ( info->elem_name = ex_str (value_ptr, (char **) NULL) )
		== NULL )
	    {
		m_abort (function_name, "element name");
	    }
	    if (strip_trailing_whitespace)
	    {
		cleanup_string (info->elem_name);
	    }
	}
	else if (strcmp (line, "BLANK") == 0)
	{
	    if (sscanf (value_ptr, "%ld", &info->blank) != 1)
	    {
		conversion_error_message (line, value_ptr);
		continue;
	    }
	    info->have_blank = TRUE;
	}
	else if (strcmp (line, "NAXIS") == 0)
	{
	    int new_naxis;

	    if (sscanf (value_ptr, "%d", &new_naxis) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	    if (info->naxis != -1)
	    {
		redeclaration_message (line);
		redeclared_keyword = TRUE;
		if (info->naxis != new_naxis)
		{
		    fprintf (stderr, "NAXIS: old value: %d  new value: %d\n",
			     info->naxis, new_naxis);
		    continue;
		}
	    }
	    info->naxis = new_naxis;
	    /*  Allocate dimension info arrays  */
	    if ( ( info->dim_lengths = (int *)
		  m_alloc (sizeof *info->dim_lengths * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "dimension lengths");
	    }
	    if ( ( info->dim_names = (char **)
		  m_alloc (sizeof *info->dim_names * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "dimension names");
	    }
	    if ( ( info->crpix = (double *)
		  m_alloc (sizeof *info->crpix * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "CRPIX array");
	    }
	    if ( ( info->crval = (double *)
		  m_alloc (sizeof *info->crval * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "CRVAL array");
	    }
	    if ( ( info->cdelta = (double *)
		  m_alloc (sizeof *info->cdelta * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "CDELT? array");
	    }
	    /*  Clear dimension lengths and set minima and maxima  */
	    for (dim_count = 0; dim_count < info->naxis; ++dim_count)
	    {
		info->dim_lengths[dim_count] = -1;
		info->crpix[dim_count] = 0.0;
		info->crval[dim_count] = TOOBIG;
		info->cdelta[dim_count] = TOOBIG;
		info->dim_names[dim_count] = NULL;
	    }
	}
	else if (strncmp (line, "NAXIS", 5) == 0)
	{
	    if (info->dim_lengths == NULL)
	    {
		fprintf (stderr, "Keyword: \"%s\" came before \"NAXIS\"\n",
			 line);
		return (FALSE);
	    }
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		continue;
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->dim_lengths[dim_count] > -1)
		{
		    redeclaration_message (line);
		    redeclared_keyword = TRUE;
		}
		if (sscanf (value_ptr, "%d", &info->dim_lengths[dim_count])
		    != 1)
		{
		    conversion_error_message (line, value_ptr);
		    return (FALSE);
		}
	    }
	}
	else if (strncmp (line, "CRPIX", 5) == 0)
	{
	    if (info->crpix == NULL)
	    {
		fprintf (stderr, "Keyword: \"%s\" came before \"NAXIS\"\n",
			 line);
		return (FALSE);
	    }
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		continue;
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->crpix[dim_count] != 0.0)
		{
		    redeclaration_message (line);
		    redeclared_keyword = TRUE;
		}
		if (sscanf (value_ptr, "%le", &info->crpix[dim_count]) != 1)
		{
		    conversion_error_message (line, value_ptr);
		    return (FALSE);
		}
		info->crpix[dim_count] -= 1.0;
	    }
	}
	else if (strncmp (line, "CRVAL", 5) == 0)
	{
	    if (info->crval == NULL)
	    {
		fprintf (stderr, "Keyword: \"%s\" came before \"NAXIS\"\n",
			 line);
		return (FALSE);
	    }
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		continue;
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->crval[dim_count] < TOOBIG)
		{
		    redeclaration_message (line);
		    redeclared_keyword = TRUE;
		}
		if (sscanf (value_ptr, "%le", &info->crval[dim_count]) != 1)
		{
		    conversion_error_message (line, value_ptr);
		    return (FALSE);
		}
	    }
	}
	else if (strncmp (line, "CDELT", 5) == 0)
	{
	    if (info->cdelta == NULL)
	    {
		fprintf (stderr, "Keyword: \"%s\" came before \"NAXIS\"\n",
			 line);
		return (FALSE);
	    }
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		continue;
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->cdelta[dim_count] < TOOBIG)
		{
		    redeclaration_message (line);
		    redeclared_keyword = TRUE;
		}
		if (sscanf (value_ptr, "%le", &info->cdelta[dim_count]) != 1)
		{
		    conversion_error_message (line, value_ptr);
		    return (FALSE);
		}
	    }
	}
	else if (strncmp (line, "CTYPE", 5) == 0)
	{
	    if (info->dim_names == NULL)
	    {
		fprintf (stderr, "Keyword: \"%s\" came before \"NAXIS\"\n",
			 line);
		return (FALSE);
	    }
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		continue;
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->dim_names[dim_count] != NULL)
		{
		    redeclaration_message (line);
		    redeclared_keyword = TRUE;
		}
		if ( ( info->dim_names[dim_count] = ex_str (value_ptr,
							    (char **) NULL) )
		    == NULL )
		{
		    m_abort (function_name, "dimension name");
		    return (FALSE);
		}
		if (strip_trailing_whitespace)
		{
		    cleanup_string (info->dim_names[dim_count]);
		}
	    }
	}
	/*  Save all keywords  */
	/*  If keyword is duplicated, update  */
	new_keyword = NULL;
	for (curr = info->first_keyword;
	     (curr != NULL) && (new_keyword == NULL);
	     curr = curr->next)
	{
	    if (strcmp (line, curr->keyword) == 0) new_keyword = curr;
	}
	/*  Append to list only if this is a new keyword  */
	if (new_keyword == NULL)
	{
	    if ( ( new_keyword = (struct keyword_type *)
		   m_alloc (sizeof *new_keyword) ) == NULL )
	    {
		m_abort (function_name, "keyword struct");
	    }
	    /*  Append  */
	    new_keyword->next = NULL;
	    if (info->first_keyword == NULL) info->first_keyword = new_keyword;
	    else info->last_keyword->next = new_keyword;
	    info->last_keyword = new_keyword;
	}
	else
	{
	    /*  Deallocate old name and value  */
	    m_free (new_keyword->keyword);
	    m_free (new_keyword->value);
	}
	if ( ( new_keyword->keyword = st_dup (line) ) == NULL )
	{
	    m_abort (function_name, "keyword name");
	}
	if ( ( new_keyword->value = st_dup (value_ptr) ) == NULL )
	{
	    m_abort (function_name, "keyword value");
	}
	/*  Scan next line  */
    }
    return (TRUE);
}   /*  End Function extract_info_from_header  */

static void redeclaration_message (char *string)
/*  This routine will display a message stating that the parameter with name
    pointed to by  string  was re-declared.
    The routine returns nothing.
*/
{
    fprintf (stderr, "FITS keyword: \"%s\" redeclared\n", string);
}   /*  End Function redeclaration_message  */

static void conversion_error_message (char *parameter, char *value)
/*  This routine will print an error message indicating that there was an
    error scanning the value string for the parameter.
    The parameter name must be pointed to by  parameter  .
    The value string must be pointed to by  value  .
    The routine returns nothing.
*/
{
    fprintf (stderr,
	     "Error converting value string: \"%s\" for FITS keyword: \"%s\"\n",
	     value, parameter);
}   /*  End Function conversion_error_message  */

static void cleanup_string (char *string)
/*  This routine will strip trailing whitespace in a string by replacing that
    whitespace with NULL characters.
    The string must be pointed to by  string  .
    The routine returns nothing.
*/
{
    char *ptr;

    ptr = string + strlen (string) - 1;
    while (ptr >= string)
    {
	if ( isspace (*ptr) )
	{
	    /*  Some whitespace: kill it  */
	    *ptr = '\0';
	    --ptr;
	}
	else
	{
	    /*  Not whitespace: finished  */
	    return;
	}
    }
}   /*  End Function cleanup_string  */

static void free_keywords (struct keyword_type *keywords)
/*  This routine will free the storage associated with unknown keywords.
    The keywords must be pointed to by  keywords  .
    The routine returns nothing.
*/
{
    struct keyword_type *next;

    while (keywords != NULL)
    {
	if (keywords->keyword != NULL) m_free (keywords->keyword);
	if (keywords->value != NULL) m_free (keywords->value);
	next = keywords->next;
	m_free ( (char *) keywords );
	keywords = next;
    }
}   /*  End Function free_keywords  */

static flag process_keywords (struct keyword_type *keywords,
			      multi_array *multi_desc)
/*  This routine will process the unknown keywords and add them to a Karma
    data structure.
    The keywords must be pointed to by  keywords  .
    The multi_array data structure must be pointed to by  multi_desc  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    int bool;
    double value[2];
    char *string, *ptr;

    for (; keywords != NULL; keywords = keywords->next)
    {
	/*  Try to determine type of keyword  */
	switch (keywords->value[0])
	{
	  case 'T':
	  case 'F':
	    /*  TRUE or FALSE: assign to K_INT  */
	    bool = ( keywords->value[0] == 'T' ) ? TRUE : FALSE;
	    value[0] = bool;
	    value[1] = 0.0;
	    if ( !ds_put_unique_named_value (multi_desc->headers[0],
					     &multi_desc->data[0],
					     keywords->keyword, K_INT,
					     value, FALSE) ) return (FALSE);
	    break;
	  case '\'':
	    /*  String  */
	    if ( ( string = ex_str (keywords->value, &ptr) ) == NULL )
	    {
		fprintf (stderr, "Error extracting string from: \"%s\"\n",
			 keywords->value);
		return (FALSE);
	    }
	    /*  Strip trailing whitespace  */
	    for (ptr = string + strlen (string) - 1;
		 isspace (*ptr) && ptr >= string; --ptr) *ptr = '\0';
	    if ( !ds_put_unique_named_string (multi_desc->headers[0],
					      multi_desc->data,
					      keywords->keyword,
					      string, FALSE ) )
	    {
		m_free (string);
		return (FALSE);
	    }
	    m_free (string);
	    break;
	  default:
	    /*  You better be numeric: you ...  */
	    if (isdigit (keywords->value[0]) == 0)
	    {
		/*  Not numeric!  */
		fprintf (stderr,
			 "WARNING: bad value: \"%s\" for keyword: \"%s\" assuming string\n",
			 keywords->value, keywords->keyword);
		if ( !ds_put_unique_named_string (multi_desc->headers[0],
						  multi_desc->data,
						  keywords->keyword,
						  keywords->value,
						  FALSE ) ) return (FALSE);
		break;
	    }
	    /*  Drop through  */
	  case '-':
	  case '.':
	    /*  Check for 'E' or '.' (indicates float value)  */
	    if ( (strchr (keywords->value, 'E') == NULL) &&
		 (strchr (keywords->value, '.') == NULL) )
	    {
		/*  Integer  */
		value[0] = ex_int (keywords->value, &ptr);
		value[1] = 0.0;
		if ( !ds_put_unique_named_value (multi_desc->headers[0],
						 &multi_desc->data[0],
						 keywords->keyword, K_INT,
						 value,FALSE) ) return (FALSE);
		continue;
	    }
	    /*  Floating point  */
	    sscanf (keywords->value, "%le", value);
	    value[1] = 0.0;
	    if ( !ds_put_unique_named_value (multi_desc->headers[0],
					     &multi_desc->data[0],
					     keywords->keyword, K_DOUBLE,
					     value, FALSE) ) return (FALSE);
	    break;
	}
    }
    return (TRUE);
}   /*  End Function process_keywords  */
