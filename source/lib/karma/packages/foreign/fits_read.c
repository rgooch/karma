/*LINTLIBRARY*/
/*  fits_read.c

    This code provides a FITS read facility.

    Copyright (C) 1995  Richard Gooch

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

    Last updated by Richard Gooch   31-JUL-1995: Made use of
  <pio_read_float_nantrap> and <pio_read_double_nantrap> and junked massive
  #if blocks testing for NaN.


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
#include <karma_dsrw.h>
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
    char **dim_names;
    double *crpix;
    double *crval;
    double *cdelta;
    char *elem_name;
    flag simple;
    flag end_found;
    struct keyword_type *unknown_keywords;
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
/*  [PURPOSE] This routine will read the header of a FITS file from a channel.
    The data section is NOT read.
    <channel> The channel to read from.
    <data_alloc> If TRUE, the data space is allocated.
    <convert_int_to_float> If TRUE, integer FITS data is converted to floating
    point data.
    <sanitise> If TRUE, FITS axes with length 1 are ignored. This is highly
    recommended.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. The last argument must be FA_FITS_READ_HEADER_END.
    The attributes are passed using varargs.
    [RETURNS] A pointer to the multi_array data structure on success, else
    NULL.
*/
{
    va_list argp;
    flag read_another_header_card = TRUE;
    flag all_dim_names_given = TRUE;
    unsigned int att_key;
    unsigned int num_dim;
    unsigned int dim_count;
    unsigned int elem_type;
    unsigned int card_count;
    struct card_info_type finfo;
    char dummy_array;
    char card[CARD_LENGTH][CARD_WIDTH];
    char *array;
    uaddr *dim_lengths;
    double *dim_minima;
    double *dim_maxima;
    char **dim_names;
    multi_array *multi_desc;
    static char def_elem_name[] = "Data Value";
    static char function_name[] = "foreign_fits_read_header";

    va_start (argp, sanitise);
    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
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
	    (void) fprintf (stderr, "Unknown attribute key: %u\n", att_key);
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
    finfo.unknown_keywords = NULL;
    card_count = 0;
    /*  Read header cards  */
    while (read_another_header_card)
    {
	if ( !read_card (channel, card) )
	{
	    (void) fprintf (stderr, "Error reading another header card\n");
	    free_keywords (finfo.unknown_keywords);
	    return (NULL);
	}
	++card_count;
	if ( !extract_info_from_header (card, &finfo, sanitise) )
	{
	    (void) fprintf (stderr,
			    "Error extracting information from header card\n");
	    free_keywords (finfo.unknown_keywords);
	    return (NULL);
	}
	if (finfo.end_found)
	{
	    read_another_header_card = FALSE;
	}
    }
    if (!finfo.simple)
    {
	(void) fprintf (stderr, "Can only read simple FITS files\n");
	free_keywords (finfo.unknown_keywords);
	return (NULL);
    }
    if (finfo.bzero == TOOBIG)
    {
	(void) fprintf (stderr,
			"WARNING: BZERO not specified: assuming 0.0\n");
	finfo.bzero = 0.0;
    }
    if (finfo.bscale == TOOBIG)
    {
	(void) fprintf (stderr,
			"WARNING: BSCALE not specified: assuming 1.0\n");
	finfo.bscale = 1.0;
    }
    if (finfo.elem_name == NULL) finfo.elem_name = def_elem_name;
    if ( (finfo.elem_name != NULL) && (strlen (finfo.elem_name) < 1) )
    {
	(void) fprintf (stderr,
			"WARNING: BUNIT is an empty string: defaulting to ");
	(void) fprintf (stderr, "\"%s\"\n", def_elem_name);
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
	(void) fprintf (stderr, "Illegal value for BITPIX: %d\n",
			finfo.bitpix);
	free_keywords (finfo.unknown_keywords);
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
	free_keywords (finfo.unknown_keywords);
	return (NULL);
    }
    /*  Compute dimension minima and maxima  */
    if ( ( dim_minima = (double *) m_alloc (sizeof *dim_minima * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "dimension minima");
	m_free ( (char *) dim_lengths );
	free_keywords (finfo.unknown_keywords);
	return (NULL);
    }
    if ( ( dim_maxima = (double *) m_alloc (sizeof *dim_maxima * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "dimension maxima");
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_minima );
	free_keywords (finfo.unknown_keywords);
	return (NULL);
    }
    /*  Create array of dimension name pointers  */
    if ( ( dim_names = (char **) m_alloc (sizeof *dim_names * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "dimension name pointers");
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_minima );
	m_free ( (char *) dim_maxima );
	free_keywords (finfo.unknown_keywords);
	return (NULL);
    }
    for (dim_count = 0, num_dim = 0; dim_count < finfo.naxis; ++dim_count)
    {
	if ( sanitise && (finfo.dim_lengths[dim_count] < 2) )
	{
	    /*  Dimension length is 1: skip  */
	    continue;
	}
	dim_lengths[num_dim] = finfo.dim_lengths[dim_count];
	dim_minima[num_dim] = (finfo.crval[dim_count] -
			       finfo.crpix[dim_count]*finfo.cdelta[dim_count]);
	dim_maxima[num_dim] = ( dim_minima[num_dim] +
			       fabs (finfo.cdelta[dim_count]) *
			       (double) (finfo.dim_lengths[dim_count] -1) );
	if (finfo.dim_names[dim_count] == NULL) all_dim_names_given = FALSE;
	dim_names[num_dim] = finfo.dim_names[dim_count];
	++num_dim;
    }
    /*  Now allocate the Karma data structure descriptors  */
    if ( ( multi_desc = ds_wrap_preallocated_n_element_array
	  (array, num_dim, dim_lengths, dim_minima, dim_maxima,
	   (double **) NULL, all_dim_names_given ? dim_names : NULL,
	   1, &elem_type, &finfo.elem_name) ) == NULL )
    {
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_minima );
	m_free ( (char *) dim_maxima );
	m_free ( (char *) dim_names );
	free_keywords (finfo.unknown_keywords);
	return (NULL);
    }
    m_free ( (char *) dim_lengths );
    m_free ( (char *) dim_minima );
    m_free ( (char *) dim_maxima );
    m_free ( (char *) dim_names );
    if (!data_alloc)
    {
	*(char **) multi_desc->data[0] = NULL;
    }
    if ( !process_keywords (finfo.unknown_keywords, multi_desc) )
    {
	free_keywords (finfo.unknown_keywords);
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    free_keywords (finfo.unknown_keywords);
    finfo.unknown_keywords = NULL;
    return (multi_desc);
}   /*  End Function foreign_fits_read_header  */

/*PUBLIC_FUNCTION*/
flag foreign_fits_read_data (Channel channel, multi_array *multi_desc,
			     char *data, uaddr num_values, ...)
/*  [PURPOSE] This routine will read the data of a FITS file from a channel.
    The header section is NOT read.
    <channel> The channel to read from.
    <multi_desc> The Karma data structure to write the data into.
    <data> An alternate data array to write the FITS data into. If this is
    NULL, the routine will write the data into the Karma data structure.
    <num_values> The number of values to write into the data array. This is
    only used when data is not NULL.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. The last argument must be FA_FITS_READ_DATA_END.
    The attributes are passed using varargs.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    KforeignFITSinfo finfo;
    flag was_nan;
    long long_data;
    unsigned char uchar_data;
    unsigned int att_key;
    unsigned int value_count;
    unsigned int elem_type, elem_size;
    unsigned int type;
    unsigned long toobig_count;
    float f_toobig = TOOBIG;
    float d_toobig = TOOBIG;
    double value[2];
    char *packet;
    unsigned long *blank_count = NULL;
    float f_data;
    double d_data;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    extern char *sys_errlist[];
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "foreign_fits_read_data";

    va_start (argp, num_values);
    if ( (channel == NULL) || (multi_desc == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
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
	    (void) fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    pack_desc = multi_desc->headers[0];
    packet = multi_desc->data[0];
    if (pack_desc->element_types[0] != K_ARRAY)
    {
	(void) fprintf (stderr,
			"First element in top level packet must be K_ARRAY\n");
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
	    (void) fprintf (stderr, "No array to write data into!\n");
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
	(void) fprintf (stderr, "BITPIX unknown!\n");
	a_prog_bug (function_name);
    }
    toobig_count = 0;
    /*  Copy data  */
    for (value_count = 0; value_count < num_values;
	 ++value_count, data += elem_size)
    {
	switch (finfo.bitpix)
	{
	  case -32:
	    if ( !pio_read_float_nantrap (channel, &f_data, &was_nan) )
	    {
		(void) fprintf (stderr, "Error reading FITS file\n");
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
		(void) fprintf (stderr, "Error reading FITS file\n");
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
	  case 8:
	    if (ch_read (channel, (char *) &uchar_data, 1) < 1)
	    {
		(void) fprintf (stderr, "Error reading from file\t%s\n",
				sys_errlist[errno]);
		(void) fprintf (stderr, "Read in: %u points\n",
				value_count);
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
		(void) fprintf (stderr, "Error reading from file\t%s\n",
				sys_errlist[errno]);
		(void) fprintf (stderr, "Read in: %u points\n",
				value_count);
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
		(void) fprintf (stderr, "Error reading from file\t%s\n",
				sys_errlist[errno]);
		(void) fprintf (stderr, "Read in: %u points\n",
				value_count);
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
	    (void) fprintf (stderr, "Illegal value for BITPIX: %u\n",
			    finfo.bitpix);
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
	(void) fprintf (stderr, "Error reading: %u bytes\t%s\n",
			CARD_WIDTH * CARD_LENGTH,
			sys_errlist[errno]);
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
    char *line;
    char *char_ptr;
    char *value_ptr;
    unsigned int line_count, char_count;
    int dim_count;
    struct keyword_type *new_keyword;
    static char function_name[] = "extract_info_from_header";

    for (line_count = 0; line_count < CARD_LENGTH; ++line_count)
    {
	line = &card[line_count][0];
	/*  Change any '\0' characters to ' '  */
	for (char_count = 0; char_count < CARD_WIDTH - 1; ++char_count)
	{
	    if (line[char_count] == '\0') line[char_count] = ' ';
	}
	line[CARD_WIDTH - 1] = '\0';
	/*  Strip trailing whitespace  */
	for (char_ptr = line + CARD_WIDTH - 2;
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
	if (strncmp (line, "HISTORY", 7) == 0) continue;
	if (strncmp (line, "COMMENT", 7) == 0) continue;
	blank_keyword = TRUE;
	for (char_count = 0; blank_keyword && (char_count < 8); ++char_count)
	{
	    if ( !isspace (line[char_count]) ) blank_keyword = FALSE;
	}
	if (blank_keyword) continue;
	if (line[0] == '\t')
	{
	    (void) fprintf (stderr,
			    "WARNING: tab in first column: ignoring line\n");
	    continue;
	}
	/*  Tidy up line  */
	/*  Remove '=' and any whitespace before '='  */
	if (line[EQUALS_POSITION] != '=')
	{
	    line[CARD_WIDTH] = '\0';
	    (void) fprintf (stderr,
			    "Expected '=' at column: %u in line: \"%s\"\n",
			    EQUALS_POSITION, line);
	    return (FALSE);
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
		return (FALSE);
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
		return (FALSE);
	    }
	}
	else if (strcmp (line, "BITPIX") == 0)
	{
	    if (info->bitpix != 0)
	    {
		redeclaration_message (line);
		return (FALSE);
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
		return (FALSE);
	    }
	    if (sscanf (value_ptr, "%le", &info->bzero) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	}
	else if (strcmp (line, "BSCALE") == 0)
	{
	    if (info->bscale != TOOBIG)
	    {
		redeclaration_message (line);
		return (FALSE);
	    }
	    if (sscanf (value_ptr, "%le", &info->bscale) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	}
	else if (strcmp (line, "BUNIT") == 0)
	{
	    if (info->elem_name != NULL)
	    {
		redeclaration_message (line);
		return (FALSE);
	    }
	    if ( ( info->elem_name = ex_str (value_ptr, (char **) NULL) )
		== NULL )
	    {
		m_abort (function_name, "element name");
		return (FALSE);
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
		return (FALSE);
	    }
	    info->have_blank = TRUE;
	}
	else if (strcmp (line, "NAXIS") == 0)
	{
	    if (info->naxis != -1)
	    {
		redeclaration_message (line);
		return (FALSE);
	    }
	    if (sscanf (value_ptr, "%d", &info->naxis) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	    /*  Allocate dimension info arrays  */
	    if ( ( info->dim_lengths = (int *)
		  m_alloc (sizeof *info->dim_lengths * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "dimension lengths");
		return (FALSE);
	    }
	    if ( ( info->dim_names = (char **)
		  m_alloc (sizeof *info->dim_names * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "dimension names");
		return (FALSE);
	    }
	    if ( ( info->crpix = (double *)
		  m_alloc (sizeof *info->crpix * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "CRPIX array");
		return (FALSE);
	    }
	    if ( ( info->crval = (double *)
		  m_alloc (sizeof *info->crval * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "CRVAL array");
		return (FALSE);
	    }
	    if ( ( info->cdelta = (double *)
		  m_alloc (sizeof *info->cdelta * info->naxis) )
		== NULL )
	    {
		m_abort (function_name, "CDELT? array");
		return (FALSE);
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
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		return (FALSE);
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->dim_lengths[dim_count] > -1)
		{
		    redeclaration_message (line);
		    return (FALSE);
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
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		return (FALSE);
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->crpix[dim_count] != 0.0)
		{
		    redeclaration_message (line);
		    return (FALSE);
		}
		if (sscanf (value_ptr, "%le", &info->crpix[dim_count]) != 1)
		{
		    conversion_error_message (line, value_ptr);
		    return (FALSE);
		}
	    }
	}
	else if (strncmp (line, "CRVAL", 5) == 0)
	{
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		return (FALSE);
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->crval[dim_count] < TOOBIG)
		{
		    redeclaration_message (line);
		    return (FALSE);
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
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		return (FALSE);
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->cdelta[dim_count] < TOOBIG)
		{
		    redeclaration_message (line);
		    return (FALSE);
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
	    /*  Get info for this dimension  */
	    if (sscanf (line + 5, "%d", &dim_count) != 1)
	    {
		conversion_error_message (line, line + 5);
		return (FALSE);
	    }
	    if ( (dim_count = info->naxis - dim_count) >= 0 )
	    {
		if (info->dim_names[dim_count] != NULL)
		{
		    redeclaration_message (line);
		    return (FALSE);
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
	if ( ( new_keyword = (struct keyword_type *)
	      m_alloc (sizeof *new_keyword) ) == NULL )
	{
	    m_abort (function_name, "keyword struct");
	}
	if ( ( new_keyword->keyword = st_dup (line) ) == NULL )
	{
	    m_abort (function_name, "keyword name");
	}
	if ( ( new_keyword->value = st_dup (value_ptr) ) == NULL )
	{
	    m_abort (function_name, "keyword value");
	    return (FALSE);
	}
	new_keyword->next = info->unknown_keywords;
	info->unknown_keywords = new_keyword;
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
    (void) fprintf (stderr, "FITS keyword: \"%s\" redeclared\n", string);
}   /*  End Function redeclaration_message  */

static void conversion_error_message (char *parameter, char *value)
/*  This routine will print an error message indicating that there was an
    error scanning the value string for the parameter.
    The parameter name must be pointed to by  parameter  .
    The value string must be pointed to by  value  .
    The routine returns nothing.
*/
{
    (void) fprintf (stderr,
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
		(void) fprintf (stderr,
				"Error extracing string from: \"%s\"\n",
				keywords->value);
		return (FALSE);
	    }
	    /*  String trailing whitespace  */
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
		(void) fprintf (stderr,
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
	    /*  Check for 'E' (indicates float value)  */
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
	    value[0] = ex_float (keywords->value, &ptr);
	    value[1] = 0.0;
	    if ( !ds_put_unique_named_value (multi_desc->headers[0],
					     &multi_desc->data[0],
					     keywords->keyword, K_FLOAT,
					     value, FALSE) ) return (FALSE);
	    break;
	}
    }
    return (TRUE);
}   /*  End Function process_keywords  */
