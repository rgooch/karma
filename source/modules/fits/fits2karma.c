/*  fits2karma.c

    Source file for  fits2karma  (module to convert FITS format to Karma).

    Copyright (C) 1992,1993,1994  Richard Gooch

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

    Last updated by Richard Gooch   29-NOV-1994: Default to "Data Value"
  if BUNIT is an empty string.


*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#ifdef OS_ULTRIX
#  define _IEEE 1
#endif
#ifndef OS_ConvexOS
#  include <nan.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_m.h>

EXTERN_FUNCTION (flag fits2karma, (char *command, FILE *fp) );

#define VERSION "1.4"

#define CARD_WIDTH 80
#define CARD_LENGTH 36
#define EQUALS_POSITION 8

/*  Copied from  lib/karma/level4/dsrw.c  */
#define MAGIC_STRING_LENGTH (unsigned int) 16
#define VERSION_NUMBER (unsigned long) 0
#define ARRAY_BOUNDARY (unsigned int) 16
static char magic_string[] = "KarmaRHD Version";


void generate_file ();
flag read_card ();
flag extract_info_from_header ();
void redeclaration_message ();
void conversion_error_message ();
void cleanup_string ();
static void free_keywords (/* keywords */);
static flag process_keywords (/* keywords, multi_desc */);
STATIC_FUNCTION (multi_array *alloc_descriptor,
		 (unsigned int num_dim, unsigned long *lengths,
		  double *minima, double *maxima, char **names,
		  unsigned int data_type, char *data_name) );
static char *convert_object_to_filename (/* object_name */);
Channel open_file (/* arrayfile */);


main (argc, argv)
int argc;       /*  Count of parameters on command line */
char **argv;    /*  List of command line parameters     */
{
    KControlPanel panel;
    extern flag ignore_excess;
    extern flag sanitise;
    extern flag convert_to_float;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "ignore_excess", "flag", PIT_FLAG, &ignore_excess,
		    PIA_END);
    panel_add_item (panel, "sanitise", "flag", PIT_FLAG, &sanitise,
		    PIA_END);
    panel_add_item (panel, "convert_to_float", "flag",
		    PIT_FLAG, &convert_to_float,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "fits2karma", VERSION, fits2karma, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag fits2karma (p, fp)
char *p;
FILE *fp;
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
}   /*  End Function fits2karma  */

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
    int naxis;
    long blank;
    int *dim_lengths;
    char **dim_names;
    double *crpix;
    double *crval;
    double *cdelta;
    char *elem_name;
    flag simple;
    flag end_found;
    flag have_blank;
    struct keyword_type *unknown_keywords;
};

void generate_file (fits_filename, arrayfile)
/*  This routine will read in a FITS file and will convert it to a Karma
    arrayfile with a multi-dimensional array of a single atomic element.
    The data will be converted from the input file.
    The name of the input file must be pointed to by  fits_filename  .
    the name of the Karma arrayfile must be pointed to by  arrayfile  .
    The routine returns nothing.
*/
char *fits_filename;
char *arrayfile;
{
    Channel fits_ch;
    Channel karma_ch;
    flag read_another_header_card = TRUE;
    flag direct_copy = FALSE;
    flag all_dim_names_given = TRUE;
    long long_data;
    char char_data;
    unsigned int array_size;
    unsigned int num_dim;
    unsigned int dim_count;
    unsigned int array_count;
    unsigned int elem_type;
    unsigned int type, element_count;
    unsigned int card_count;
    unsigned int toobig_count;
    unsigned int bytes_to_pad;
    unsigned long read_pos;
    unsigned long write_pos;
    struct stat stat_buf;
    struct card_info_type info;
    char card[CARD_LENGTH][CARD_WIDTH];
    char txt[STRING_LENGTH];
    char padding[ARRAY_BOUNDARY];
    char *array;
    char *packet;
    unsigned long *dim_lengths;
    float f_data;
    double d_data;
    double *dim_minima;
    double *dim_maxima;
    char **dim_names;
    multi_array *multi_desc;
    packet_desc *pack_desc;
    struct keyword_type *curr_keyword;
    extern flag convert_to_float;
    extern int ignore_excess;
    extern int sanitise;
    extern char host_type_sizes[NUMTYPES];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "generate_file";

    /*  Get stats on FITS file  */
    if (stat (fits_filename, &stat_buf) != 0)
    {
	(void) fprintf (stderr, "Error getting stats on file: \"%s\"\t%s\n",
			fits_filename, sys_errlist[errno]);
	return;
    }
    /*  Compare with specified array size  */
    if (stat_buf.st_size % (CARD_WIDTH * CARD_LENGTH) != 0)
    {
	(void) fprintf (stderr,
			"File: \"%s\" is not an integral number of cards\n",
			fits_filename);
	return;
    }
    /*  Try to open input file  */
    if ( ( fits_ch = ch_open_file (fits_filename, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			fits_filename, sys_errlist[errno]);
	return;
    }
    /*  Initialise card info  */
    info.bitpix = 0;
    info.end_found = FALSE;
    info.simple = 2 * abs (TRUE) + 2 * abs (FALSE);
    info.naxis = -1;
    info.dim_lengths = NULL;
    info.crpix = NULL;
    info.crval = NULL;
    info.cdelta = NULL;
    info.dim_names = NULL;
    info.elem_name = NULL;
    info.bzero = TOOBIG;
    info.bscale = TOOBIG;
    info.have_blank = FALSE;
    info.unknown_keywords = NULL;
    card_count = 0;
    /*  Read header cards  */
    while (read_another_header_card == TRUE)
    {
	if (read_card (fits_ch, card) != TRUE)
	{
	    (void) fprintf (stderr, "Error reading another header card\n");
	    (void) ch_close (fits_ch);
	    free_keywords (info.unknown_keywords);
	    return;
	}
	++card_count;
	if (extract_info_from_header (card, &info, sanitise) != TRUE)
	{
	    (void) fprintf (stderr,
			    "Error extracting information from header card\n");
	    (void) ch_close (fits_ch);
	    free_keywords (info.unknown_keywords);
	    return;
	}
	if (info.end_found)
	{
	    read_another_header_card = FALSE;
	}
    }
    if (info.simple != TRUE)
    {
	(void) fprintf (stderr, "Can only read simple FITS files\n");
	free_keywords (info.unknown_keywords);
	return;
    }
    if (info.bzero == TOOBIG)
    {
	(void) fprintf (stderr,
			"WARNING: BZERO not specified: assuming 0.0\n");
	info.bzero = 0.0;
    }
    if (info.bscale == TOOBIG)
    {
	(void) fprintf (stderr,
			"WARNING: BSCALE not specified: assuming 1.0\n");
	info.bscale = 1.0;
    }
    if ( (info.elem_name != NULL) && (strlen (info.elem_name) < 1) )
    {
	(void) fprintf (stderr,
			"WARNING: BUNIT is an empty string: defaulting to ");
	(void) fprintf (stderr, "\"Data Value\"\n");
	m_free (info.elem_name);
	info.elem_name = NULL;
    }
    (void) fprintf (stderr, "FITS header read in:\t%u cards\n", card_count);
    /*  Determine output data type  */
    switch (info.bitpix)
    {
      case 8:
	if (convert_to_float)
	{
	    elem_type = K_FLOAT;
	}
	else
	{
	    elem_type = K_UBYTE;
	    direct_copy = TRUE;
	}
	break;
      case 16:
	if (convert_to_float)
	{
	    elem_type = K_FLOAT;
	}
	else
	{
	    elem_type = K_SHORT;
	    direct_copy = TRUE;
	}
	break;
      case 32:
	if (convert_to_float)
	{
	    elem_type = K_FLOAT;
	}
	else
	{
	    elem_type = K_INT;
	    direct_copy = TRUE;
	}
	break;
      case -32:
	elem_type = K_FLOAT;
	direct_copy = TRUE;
	break;
      case -64:
	elem_type = K_DOUBLE;
	direct_copy = TRUE;
	break;
      default:
	(void) fprintf (stderr, "Illegal value for BITPIX: %d\n", info.bitpix);
	(void) ch_close (fits_ch);
	free_keywords (info.unknown_keywords);
	return;
	break;
    }
    if (sanitise)
    {
	/*  Stip dimensions with length of 1  */
	for (dim_count = 0, num_dim = 0; dim_count < info.naxis; ++dim_count)
	{
	    if (info.dim_lengths[dim_count] > 1) ++num_dim;
	}
    }
    else
    {
	/*  All dimensions must be allocated  */
	num_dim = info.naxis;
    }
    /*  Create array of dimension lengths  */
    if ( ( dim_lengths = (unsigned long *)
	  m_alloc (sizeof *dim_lengths * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "dimension lengths");
	free_keywords (info.unknown_keywords);
	return;
    }
    /*  Compute dimension minima and maxima  */
    if ( ( dim_minima = (double *) m_alloc (sizeof *dim_minima * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "dimension minima");
	m_free ( (char *) dim_lengths );
	free_keywords (info.unknown_keywords);
	return;
    }
    if ( ( dim_maxima = (double *) m_alloc (sizeof *dim_maxima * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "dimension maxima");
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_minima );
	free_keywords (info.unknown_keywords);
	return;
    }
    /*  Create array of dimension name pointers  */
    if ( ( dim_names = (char **) m_alloc (sizeof *dim_names * num_dim) )
	== NULL )
    {
	m_error_notify (function_name, "dimension name pointers");
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_minima );
	m_free ( (char *) dim_maxima );
	free_keywords (info.unknown_keywords);
	return;
    }
    for (dim_count = 0, num_dim = 0; dim_count < info.naxis; ++dim_count)
    {
	if ( sanitise && (info.dim_lengths[dim_count] < 2) )
	{
	    /*  Dimension length is 1: skip  */
	    continue;
	}
	dim_lengths[num_dim] = info.dim_lengths[dim_count];
	dim_minima[num_dim] = (info.crval[dim_count] -
			       info.crpix[dim_count] * info.cdelta[dim_count]);
	dim_maxima[num_dim] = ( dim_minima[num_dim] +
			       fabs (info.cdelta[dim_count]) *
			       (double) (info.dim_lengths[dim_count] -1) );
	if (info.dim_names[dim_count] == NULL) all_dim_names_given = FALSE;
	dim_names[num_dim] = info.dim_names[dim_count];
	++num_dim;
    }
    /*  Now allocate the Karma data structure descriptors  */
    if ( ( multi_desc =
	  alloc_descriptor (num_dim, dim_lengths, dim_minima, dim_maxima,
			    all_dim_names_given ? dim_names : NULL,
			    elem_type, info.elem_name) )
	== NULL )
    {
	(void) fprintf (stderr,
			"NULL return value from function: easy_alloc_array\n");
	(void) ch_close (fits_ch);
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_minima );
	m_free ( (char *) dim_maxima );
	m_free ( (char *) dim_names );
	free_keywords (info.unknown_keywords);
	return;
    }
    
    if ( ( (*multi_desc).data[0] = ds_alloc_data ( (*multi_desc).headers[0],
						  FALSE, FALSE ) )
	== NULL )
    {
	(void) fprintf (stderr, "Error allocating top level packet\n");
	(void) ch_close (fits_ch);
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_minima );
	m_free ( (char *) dim_maxima );
	m_free ( (char *) dim_names );
	free_keywords (info.unknown_keywords);
	ds_dealloc_multi (multi_desc);
	return;
    }
    m_free ( (char *) dim_lengths );
    m_free ( (char *) dim_minima );
    m_free ( (char *) dim_maxima );
    m_free ( (char *) dim_names );
    if (process_keywords (info.unknown_keywords, multi_desc) != TRUE)
    {
	free_keywords (info.unknown_keywords);
	ds_dealloc_multi (multi_desc);
	return;
    }
    free_keywords (info.unknown_keywords);
    info.unknown_keywords = NULL;
    array_size = ds_get_array_size ( (array_desc *)
				    (*(*multi_desc).headers[0]).element_desc[0] );
    toobig_count = 0;
    /*  Now write the Karma descriptors  */
    if ( ( karma_ch = open_file (arrayfile) ) == NULL )
    {
	(void) fprintf (stderr, "Error opening Karma arrayfile file\n");
	(void) ch_close (fits_ch);
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
    if (pio_write32 (karma_ch, VERSION_NUMBER) != TRUE)
    {
	exit (RV_WRITE_ERROR);
    }
    /*  Write number of data structures  */
    if (pio_write32 (karma_ch, (unsigned long) 1) != TRUE)
    {
	exit (RV_WRITE_ERROR);
    }
    dsrw_write_packet_desc (karma_ch, (*multi_desc).headers[0]);
    /*  Pad  */
    /*  Clear padding array (to avoid random file differences)  */
    m_clear (padding, ARRAY_BOUNDARY);
    if (ch_tell (karma_ch, &read_pos, &write_pos) != TRUE)
    {
	(void) fprintf (stderr, "Error getting channel position\n");
	exit (RV_UNDEF_ERROR);
    }
    /*  Add 4 bytes for pad size  */
    write_pos += 4;
    bytes_to_pad = ARRAY_BOUNDARY - write_pos % ARRAY_BOUNDARY;
    if (pio_write32 (karma_ch, (unsigned long) bytes_to_pad) != TRUE)
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
    /*  Copy data  */
    if (direct_copy)
    {
	/*  Data type of FITS file is same as in Karma arrayfile  */
	if ( (info.bzero != 0.0) || (info.bscale != 1.0) )
	{
	    (void) fprintf (stderr, "Applying BZERO: %le  BSCALE: %le\n",
			    info.bzero, info.bscale);
	}
	switch (elem_type)
	{
	  case K_FLOAT:
	    for (array_count = 0; array_count < array_size; ++array_count)
	    {
		if (pio_read_float (fits_ch, &f_data) != TRUE)
		{
		    (void) fprintf (stderr, "Error reading FITS file\n");
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
#ifdef NAN
		if (NAN == (double) f_data)
#else
#  if defined(OS_ConvexOS) || defined(OS_OSF1)
		if ( isnanf (f_data) )
#  else
		if ( NaN (f_data) )
#  endif
#endif
		{
		    f_data = TOOBIG;
		    ++toobig_count;
		}
		else
		{
		    f_data = info.bzero + f_data * info.bscale;
		}
		if (pio_write_float (karma_ch, f_data) != TRUE)
		{
		    (void) fprintf (stderr, "Error writing Karma file\n");
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
	    }
	    break;
	  case K_DOUBLE:
	    for (array_count = 0; array_count < array_size; ++array_count)
	    {
		if (pio_read_double (fits_ch, &d_data) != TRUE)
		{
		    (void) fprintf (stderr, "Error reading FITS file\n");
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
#ifdef NAN
		if (d_data == NAN)
#else
#  if defined(OS_ConvexOS) || defined(OS_OSF1)
		if ( isnan (d_data) )
#  else
		if ( NaN (d_data) )
#  endif
#endif
		{
		    d_data = TOOBIG;
		    ++toobig_count;
		}
		else
		{
		    d_data = info.bzero + d_data * info.bscale;
		}
		if (pio_write_double (karma_ch, d_data) != TRUE)
		{
		    (void) fprintf (stderr, "Error writing Karma file\n");
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
	    }
	    break;
	  case K_UBYTE:
	    for (array_count = 0; array_count < array_size; ++array_count)
	    {
		if (ch_read (fits_ch, &char_data, 1) < 1)
		{
		    (void) fprintf (stderr, "Error reading from file\t%s\n",
				    sys_errlist[errno]);
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    (void) fprintf (stderr, "Read in: %u points\n",
				    array_count);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
		if (ch_write (karma_ch, &char_data, 1) < 1)
		{
		    (void) fprintf (stderr, "Error writing to file\t%s\n",
				    sys_errlist[errno]);
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    (void) fprintf (stderr, "Wrote out: %u points\n",
				    array_count);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
	    }
	    break;
	  case K_SHORT:
	    for (array_count = 0; array_count < array_size; ++array_count)
	    {
		if (pio_read16s (fits_ch, &long_data) != TRUE)
		{
		    (void) fprintf (stderr, "Error reading from file\t%s\n",
				    sys_errlist[errno]);
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    (void) fprintf (stderr, "Read in: %u points\n",
				    array_count);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
		if (pio_write16s (karma_ch, long_data) != TRUE)
		{
		    (void) fprintf (stderr, "Error writing to file\t%s\n",
				    sys_errlist[errno]);
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    (void) fprintf (stderr, "Wrote out: %u points\n",
				    array_count);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
	    }
	    break;
	  case K_INT:
	    for (array_count = 0; array_count < array_size; ++array_count)
	    {
		if (pio_read32s (fits_ch, &long_data) != TRUE)
		{
		    (void) fprintf (stderr, "Error reading from file\t%s\n",
				    sys_errlist[errno]);
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    (void) fprintf (stderr, "Read in: %u points\n",
				    array_count);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
		if (pio_write32s (karma_ch, long_data) != TRUE)
		{
		    (void) fprintf (stderr, "Error writing to file\t%s\n",
				    sys_errlist[errno]);
		    (void) ch_close (fits_ch);
		    (void) ch_close (karma_ch);
		    (void) fprintf (stderr, "Wrote out: %u points\n",
				    array_count);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
	    }
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal value for  elem_type: %u\n",
			    elem_type);
	    (void) ch_close (fits_ch);
	    ds_dealloc_multi (multi_desc);
	    return;
	    break;
	}
    }
    else
    {
	/*  Must read data one datum at a time and convert  */
	(void) fprintf (stderr,
			"Converting integer to float\toffset: %e\tfactor: %e\n"
			, info.bzero, info.bscale);
	for (array_count = 0; array_count < array_size; ++array_count)
	{
	    switch (info.bitpix)
	    {
	      case 8:
		if (ch_read (fits_ch, &char_data, 1) < 1)
		{
		    (void) fprintf (stderr, "Error reading from file\t%s\n",
				    sys_errlist[errno]);
		    (void) ch_close (fits_ch);
		    (void) fprintf (stderr, "Read in: %u points\n",
				    array_count);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
		long_data = char_data;
		break;
	      case 16:
		if (pio_read16s (fits_ch, &long_data) != TRUE)
		{
		    (void) ch_close (fits_ch);
		    (void) fprintf (stderr, "Read in: %u points\n",
				    array_count);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
		break;
	      case 32:
		if (pio_read32s (fits_ch, &long_data) != TRUE)
		{
		    (void) ch_close (fits_ch);
		    (void) fprintf (stderr, "Read in: %u points\n",
				    array_count);
		    ds_dealloc_multi (multi_desc);
		    return;
		}
		break;
	      default:
		(void) fprintf (stderr, "Illegal value for BITPIX: %d\n",
				info.bitpix);
		(void) ch_close (fits_ch);
		ds_dealloc_multi (multi_desc);
		return;
		break;
	    }
	    if (long_data == info.blank)
	    {
		f_data = TOOBIG;
		++toobig_count;
	    }
	    else
	    {
		f_data = info.bzero + info.bscale * (double) long_data;
	    }
	    if (pio_write_float (karma_ch, f_data) != TRUE)
	    {
		(void) fprintf (stderr, "Error writing Karma file\n");
		(void) ch_close (fits_ch);
		(void) ch_close (karma_ch);
		ds_dealloc_multi (multi_desc);
		return;
	    }
	}
    }
    (void) ch_close (fits_ch);
    /*  Array should be copied by now: write rest of top level packet  */
    pack_desc = (*multi_desc).headers[0];
    packet = (*multi_desc).data[0] + host_type_sizes[K_ARRAY];
    for (element_count = 1; element_count < (*pack_desc).num_elements;
	 ++element_count)
    {
	/*  Write element   */
        type = (*pack_desc).element_types[element_count];
        dsrw_write_element (karma_ch, type,
			    (*pack_desc).element_desc[element_count],
			    packet);
        packet += host_type_sizes[type];
    }
    /*  Write dummy history  */
    if (pio_write_string (karma_ch, (char *) NULL) != TRUE)
    {
	(void) fprintf (stderr, "Error writing NULL history string\n");
	exit (RV_WRITE_ERROR);
    }
    if (ch_close (karma_ch) != TRUE)
    {
	(void) fprintf (stderr, "Error closing channel\n");
	return;
    }
    ds_dealloc_multi (multi_desc);
    if (toobig_count > 0)
    {
	(void) fprintf (stderr, "Number of blank values: %u\n", toobig_count);
    }
    /*  Write comments to new array  */
#ifdef dummy
    ez_description (comp_array, vble);
    (void) sprintf (txt, "Read FITS data file: \"%s\"", fits_filename);
    composite_descr (txt, comp_array);
#endif
}   /*  End Function generate_file  */

flag read_card (channel, card)
/*  This routine will read a single card of data from a FITS file.
    The file channel must be given by  channel  .
    The storage to write the card data must be pointed to by  card  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Channel channel;
char card[CARD_LENGTH][CARD_WIDTH];
{
    ERRNO_TYPE errno;
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

flag extract_info_from_header (card, info, strip_trailing_whitespace)
/*  This routine will extract information from a single FITS card.
    The card must be pointed to by  card  .
    The information extracted will be written to the storage pointed to by
    info  .
    If the flag  strip_trailing_whitespace  is TRUE, then converted strings
    with trailing whitespace will have the whitespace removed.
    The routine returns TRUE on success, else it returns FALSE.
*/
char card[CARD_LENGTH][CARD_WIDTH];
struct card_info_type *info;
flag strip_trailing_whitespace;
{
    flag quote;
    flag keep_searching;
    char *line;
    char *char_ptr;
    char *value_ptr;
    unsigned int line_count;
    int dim_count;
    struct keyword_type *new_keyword;
    static char function_name[] = "extract_info_from_header";

    for (line_count = 0; line_count < CARD_LENGTH; ++line_count)
    {
	line = &card[line_count][0];
	line[CARD_WIDTH - 1] = '\0';
	if (strncmp (line, "END", 3) == 0)
	{
	    (*info).end_found = TRUE;
	    return (TRUE);
	}
	if (strncmp (line, "HISTORY", 7) == 0)
	{
	    continue;
	}
	if (strncmp (line, "COMMENT", 7) == 0)
	{
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
	while ( isspace (*value_ptr) )
	{
	    ++value_ptr;
	}
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
	    if (char_ptr - line >= CARD_WIDTH)
	    {
		keep_searching = FALSE;
	    }
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
	    if ( ( (*info).simple == TRUE ) || ( (*info).simple == FALSE ) )
	    {
		redeclaration_message (line);
		return (FALSE);
	    }
	    if (*value_ptr == 'T')
	    {
		(*info).simple = TRUE;
	    }
	    else if (*value_ptr == 'F')
	    {
		(*info).simple = FALSE;
	    }
	    else
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	}
	else if (strcmp (line, "BITPIX") == 0)
	{
	    if ( (*info).bitpix != 0 )
	    {
		redeclaration_message (line);
		return (FALSE);
	    }
	    if (sscanf (value_ptr, "%d", &(*info).bitpix) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	}
	else if (strcmp (line, "BZERO") == 0)
	{
	    if ( (*info).bzero != TOOBIG )
	    {
		redeclaration_message (line);
		return (FALSE);
	    }
	    if (sscanf (value_ptr, "%le", &(*info).bzero) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	}
	else if (strcmp (line, "BSCALE") == 0)
	{
	    if ( (*info).bscale != TOOBIG )
	    {
		redeclaration_message (line);
		return (FALSE);
	    }
	    if (sscanf (value_ptr, "%le", &(*info).bscale) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	}
	else if (strcmp (line, "BUNIT") == 0)
	{
	    if ( (*info).elem_name != NULL )
	    {
		redeclaration_message (line);
		return (FALSE);
	    }
	    if ( ( (*info).elem_name = ex_str (value_ptr, (char **) NULL) )
		== NULL )
	    {
		m_abort (function_name, "element name");
		return (FALSE);
	    }
	    if (strip_trailing_whitespace)
	    {
		cleanup_string ( (*info).elem_name );
	    }
	}
	else if (strcmp (line, "BLANK") == 0)
	{
	    if (sscanf (value_ptr, "%ld", &(*info).blank) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	    (*info).have_blank = TRUE;
	}
	else if (strcmp (line, "NAXIS") == 0)
	{
	    if ( (*info).naxis != -1 )
	    {
		redeclaration_message (line);
		return (FALSE);
	    }
	    if (sscanf (value_ptr, "%d", &(*info).naxis) != 1)
	    {
		conversion_error_message (line, value_ptr);
		return (FALSE);
	    }
	    /*  Allocate dimension info arrays  */
	    if ( ( (*info).dim_lengths = (int *)
		  m_alloc (sizeof *(*info).dim_lengths * (*info).naxis) )
		== NULL )
	    {
		m_abort (function_name, "dimension lengths");
		return (FALSE);
	    }
	    if ( ( (*info).dim_names = (char **)
		  m_alloc (sizeof *(*info).dim_names * (*info).naxis) )
		== NULL )
	    {
		m_abort (function_name, "dimension names");
		return (FALSE);
	    }
	    if ( ( (*info).crpix = (double *)
		  m_alloc (sizeof *(*info).crpix * (*info).naxis) )
		== NULL )
	    {
		m_abort (function_name, "CRPIX array");
		return (FALSE);
	    }
	    if ( ( (*info).crval = (double *)
		  m_alloc (sizeof *(*info).crval * (*info).naxis) )
		== NULL )
	    {
		m_abort (function_name, "CRVAL array");
		return (FALSE);
	    }
	    if ( ( (*info).cdelta = (double *)
		  m_alloc (sizeof *(*info).cdelta * (*info).naxis) )
		== NULL )
	    {
		m_abort (function_name, "CDELT? array");
		return (FALSE);
	    }
	    /*  Clear dimension lengths and set minima and maxima  */
	    for (dim_count = 0; dim_count < (*info).naxis; ++dim_count)
	    {
		(*info).dim_lengths[dim_count] = -1;
		(*info).crpix[dim_count] = 0.0;
		(*info).crval[dim_count] = TOOBIG;
		(*info).cdelta[dim_count] = TOOBIG;
		(*info).dim_names[dim_count] = NULL;
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
	    if ( ( dim_count = (*info).naxis - dim_count ) >= 0 )
	    {
		if ( (*info).dim_lengths[dim_count] > -1 )
		{
		    redeclaration_message (line);
		    return (FALSE);
		}
		if (sscanf (value_ptr, "%d", &(*info).dim_lengths[dim_count])
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
	    if ( ( dim_count = (*info).naxis - dim_count ) >= 0 )
	    {
		if ( (*info).crpix[dim_count] != 0.0 )
		{
		    redeclaration_message (line);
		    return (FALSE);
		}
		if (sscanf (value_ptr, "%le", &(*info).crpix[dim_count]) != 1)
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
	    if ( ( dim_count = (*info).naxis - dim_count ) >= 0 )
	    {
		if ( (*info).crval[dim_count] < TOOBIG )
		{
		    redeclaration_message (line);
		    return (FALSE);
		}
		if (sscanf (value_ptr, "%le", &(*info).crval[dim_count]) != 1)
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
	    if ( ( dim_count = (*info).naxis - dim_count ) >= 0 )
	    {
		if ( (*info).cdelta[dim_count] < TOOBIG )
		{
		    redeclaration_message (line);
		    return (FALSE);
		}
		if (sscanf (value_ptr, "%le", &(*info).cdelta[dim_count]) != 1)
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
	    if ( ( dim_count = (*info).naxis - dim_count ) >= 0 )
	    {
		if ( (*info).dim_names[dim_count] != NULL )
		{
		    redeclaration_message (line);
		    return (FALSE);
		}
		if ( ( (*info).dim_names[dim_count] = ex_str (value_ptr,
							      (char **) NULL) )
		    == NULL )
		{
		    m_abort (function_name, "dimension name");
		    return (FALSE);
		}
		if (strip_trailing_whitespace)
		{
		    cleanup_string ( (*info).dim_names[dim_count] );
		}
	    }
	}
	/*  Save all keywords  */
	if ( ( new_keyword = (struct keyword_type *)
	      m_alloc (sizeof *new_keyword) ) == NULL )
	{
	    m_abort (function_name, "keyword struct");
	}
	if ( ( (*new_keyword).keyword = st_dup (line) ) == NULL )
	{
	    m_abort (function_name, "keyword name");
	}
	if ( ( (*new_keyword).value = st_dup (value_ptr) ) == NULL )
	{
	    m_abort (function_name, "keyword value");
	    return (FALSE);
	}
	(*new_keyword).next = (*info).unknown_keywords;
	(*info).unknown_keywords = new_keyword;
	/*  Scan next line  */
    }
    return (TRUE);
}   /*  End Function extract_info_from_header  */

void redeclaration_message (string)
/*  This routine will display a message stating that the parameter with name
    pointed to by  string  was re-declared.
    The routine returns nothing.
*/
char *string;
{
    (void) fprintf (stderr, "FITS keyword: \"%s\" redeclared\n", string);
}   /*  End Function redeclaration_message  */

void conversion_error_message (parameter, value)
/*  This routine will print an error message indicating that there was an
    error scanning the value string for the parameter.
    The parameter name must be pointed to by  parameter  .
    The value string must be pointed to by  value  .
    The routine returns nothing.
*/
char *parameter;
char *value;
{
    (void) fprintf (stderr,
		    "Error converting value string: \"%s\" for FITS keyword: \"%s\"\n",
		    value, parameter);
}   /*  End Function conversion_error_message  */

void cleanup_string (string)
/*  This routine will strip trailing whitespace in a string by replacing that
    whitespace with NULL characters.
    The string must be pointed to by  string  .
    The routine returns nothing.
*/
char *string;
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

static void free_keywords (keywords)
/*  This routine will free the storage associated with unknown keywords.
    The keywords must be pointed to by  keywords  .
    The routine returns nothing.
*/
struct keyword_type *keywords;
{
    struct keyword_type *next;

    while (keywords != NULL)
    {
	if ( (*keywords).keyword != NULL ) m_free ( (*keywords).keyword );
	if ( (*keywords).value != NULL ) m_free ( (*keywords).value );
	next = (*keywords).next;
	m_free ( (char *) keywords );
	keywords = next;
    }
}   /*  End Function free_keywords  */

static flag process_keywords (keywords, multi_desc)
/*  This routine will process the unknown keywords and add them to a Karma
    data structure.
    The keywords must be pointed to by  keywords  .
    The multi_array data structure must be pointed to by  multi_desc  .
    The routine returns TRUE on success, else it returns FALSE.
*/
struct keyword_type *keywords;
multi_array *multi_desc;
{
    int bool;
    double value[2];
    char *dummy;
    static char function_name[] = "process_keywords";

    for (; keywords != NULL; keywords = (*keywords).next)
    {
	/*  Try to determine type of keyword  */
	switch ( (*keywords).value[0] )
	{
	  case 'T':
	  case 'F':
	    /*  TRUE or FALSE: assign to K_INT  */
	    bool = ( (*keywords).value[0] == 'T' ) ? TRUE : FALSE;
	    value[0] = bool;
	    value[1] = 0.0;
	    if (ds_put_unique_named_value ( (*multi_desc).headers[0],
					   &(*multi_desc).data[0],
					   (*keywords).keyword, K_INT,
					   value,
					   FALSE ) != TRUE) return (FALSE);
	    break;
	  case '\'':
	    /*  String  */
	    if (ds_put_unique_named_string ( (*multi_desc).headers[0],
					    (*multi_desc).data,
					    (*keywords).keyword,
					    (*keywords).value,
					    FALSE ) != TRUE) return (FALSE);
	    break;
	  default:
	    /*  You better be numeric: you ...  */
	    if (isdigit ( (*keywords).value[0] ) == 0)
	    {
		/*  Not numeric!  */
		(void) fprintf (stderr, "Non-numeric: \"%s\"\n",
				(*keywords).value);
		return (FALSE);
	    }
	    /*  Drop through  */
	  case '-':
	  case '.':
	    /*  Check for 'E' (indicates float value)  */
	    if (strchr ( (*keywords).value, 'E' ) == NULL)
	    {
		/*  Integer  */
		value[0] = ex_int ( (*keywords).value, &dummy );
		value[1] = 0.0;
		if (ds_put_unique_named_value ( (*multi_desc).headers[0],
					       &(*multi_desc).data[0],
					       (*keywords).keyword, K_INT,
					       value, FALSE )
		    != TRUE) return (FALSE);
		continue;
	    }
	    /*  Floating point  */
	    value[0] = ex_float ( (*keywords).value, &dummy );
	    value[1] = 0.0;
	    if (ds_put_unique_named_value ( (*multi_desc).headers[0],
					   &(*multi_desc).data[0],
					   (*keywords).keyword, K_FLOAT,
					   value,
					   FALSE ) != TRUE) return (FALSE);
	    break;
	}
    }
    return (TRUE);
}   /*  End Function process_keywords  */

static multi_array *alloc_descriptor (unsigned int num_dim,
				      unsigned long *lengths,
				      double *minima, double *maxima,
				      char **names, unsigned int data_type,
				      char *data_name)
/*  This routine will allocate memory for the descriptors for a
    multi-dimensional, regular array. No data space is allocated.
    The number of dimensions of the array must be in  num_dim  ,the lengths
    of each dimension (axis) must be in the array pointed to by  lengths  .
    The minimum and maximum co-ordinate values of each dimension must be in the
    arrays pointed to by  minima  and maxima  ,respectively. If either of
    these pointers is NULL, the range of the co-ordinates will be the lengths
    of the dimensions minus 1. If one of the pointers is NULL, the other
    array is used to tie one end of the range for each dimension. If both
    pointers are NULL,the minima are 0.0 and the maxima are the lengths minus 1
    The names of each dimension must be pointed to by the list of character
    pointers  names  .If this is NULL, the names "Axis 0", "Axis 1", ...etc
    will be used. Note: the character arrays are copied, so the arrays of
    characters and the array of pointers may be subsequently deallocated.
    The data packet that may be stored in the array may contain a number of
    atomic elements.
    The data type of the element must be given by  data_type  .
    The following data types are not permitted:
        NONE, K_ARRAY, LISTP, MULTI_ARRAY.
    The name of the elements must be pointed to by  data_name  .The name
    string is copied, thus the memory used for the input string may be
    subsequently deallocated.
    The routine returns a pointer to the multi_array descriptor on succes,
    else it returns NULL.
*/
{
    unsigned int dim_count;
    double minimum;
    double maximum;
    char *return_value;
    char *tmp_pointer;
    multi_array *multi_desc;
    packet_desc *pack_descriptor;
    array_desc *arr_descriptor;
    dim_desc *dimension;
    char tmp_name[129];
    static char *def_data_name = "Data Value";
    static char function_name[] = "alloc_descriptor";

    if (lengths == NULL)
    {
	(void) fprintf (stderr, "NULL dimension lengths pointer passed\n");
	a_prog_bug (function_name);
    }
    if (data_name == NULL) data_name = def_data_name;
    if (names == NULL)
    {
	/*  No names supplied  */
        if ( (int) log10 ( (double) num_dim ) + 1 > 122 )
        {
	    /*  Too many dimensions for sprintf to print  */
	    a_func_abort (function_name, "Too many dimensions");
            return (NULL);
        }
    }
    if ( ( multi_desc = ds_alloc_multi ( (unsigned int) 1 ) ) == NULL )
    {
	m_error_notify (function_name, "multi_desc descriptor");
        return (NULL);
    }
    if ( ( pack_descriptor = ds_alloc_packet_desc ( (unsigned) 1 ) )
	== NULL )
    {
	ds_dealloc_multi (multi_desc);
	m_error_notify (function_name, "packet descriptor");
        return (NULL);
    }
    (*multi_desc).headers[0] = pack_descriptor;
    if ( ( arr_descriptor = ds_alloc_array_desc (num_dim, 0) ) == NULL )
    {
	ds_dealloc_multi (multi_desc);
	m_error_notify (function_name, "array descriptor");
        return (NULL);
    }
    (*pack_descriptor).element_desc[0] = (char *) arr_descriptor;
    (*pack_descriptor).element_types[0] = K_ARRAY;
    for (dim_count = 0; dim_count < num_dim; ++dim_count)
    {
	/*  Allocate and add adimension descriptor  */
        if (names == NULL)
        {
	    /*  Create temporary dimension name  */
/*  *******  PATCH  **********  */
            (void) sprintf (tmp_name, "Axis %d", (int) dim_count);
/*
            (void) sprintf (tmp_name, "Axis %u", dim_count);
*/
            tmp_pointer = tmp_name;
        }
        else
        {
	    /*  Copy over character pointer  */
            tmp_pointer = names[dim_count];
        }
        if ( (minima == NULL) && (maxima == NULL) )
        {
	    /*  Range is from  0.0  to  lengths - 1  */
            minimum = 0.0;
            maximum = (lengths[dim_count] - 1);
        }
        else
        {
	    /*  Range has at least one boundary supplied  */
            if (minima == NULL)
            {
		/*  Maximum only supplied  */
                maximum = maxima[dim_count];
                minimum = maximum - (lengths[dim_count] - 1);
            }
            else
            {
		/*  Minimum supplied  */
                minimum = minima[dim_count];
                if (maxima == NULL)
                {
		    /*  Minimum only supplied  */
                    maximum = minimum + (lengths[dim_count] - 1);
                }
                else
                {
		    /*  Maximum also supplied  */
                    maximum = maxima[dim_count];
                }
            }
        }
        if ( ( dimension = ds_alloc_dim_desc (tmp_pointer,
					      lengths[dim_count],
					      minimum, maximum, TRUE) )
	    == NULL )
        {
	    ds_dealloc_multi (multi_desc);
	    m_error_notify (function_name, "dimension descriptor");
            return (NULL);
        }
        (*arr_descriptor).dimensions[dim_count] = dimension;
	(*arr_descriptor).lengths[dim_count] = (*dimension).length;
    }
    if ( ( pack_descriptor = ds_alloc_packet_desc (1) ) == NULL )
    {
	ds_dealloc_multi (multi_desc);
	m_error_notify (function_name, "array packet descriptor");
        return (NULL);
    }
    (*arr_descriptor).packet = pack_descriptor;
    (*pack_descriptor).element_types[0] = data_type;
    if ( ( (*pack_descriptor).element_desc[0] = st_dup (data_name) ) == NULL )
    {
	ds_dealloc_multi (multi_desc);
	m_error_notify (function_name, "element name");
	return (NULL);
    }
    return (multi_desc);
}   /*  End Function alloc_descriptor  */

static char *convert_object_to_filename (object_name)
/*  This routine will convert an object name to a Unix filename. This involves
    appending the default extension name to the object name. If the object name
    already contains the default extension name, no change is made.
    The object name must be pointed to by  object_name  .
    The routine returns a pointer to a dynamically allocated filename on
    success, else it returns NULL.
*/
char *object_name;
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

Channel open_file (arrayfile)
/*  This routine will open an arrayfile for writing.
    The arrayfile name must be pointed to by  arrayfile  .
    The routine returns a channel object on success, else it returns NULL.
*/
char *arrayfile;
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
flag ignore_excess = FALSE;
flag sanitise = TRUE;
flag convert_to_float = TRUE;
char *default_extension = ".kf";
