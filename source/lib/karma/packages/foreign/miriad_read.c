/*LINTLIBRARY*/
/*  miriad_read.c

    This code provides a Miriad read facility.

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
  Miriad Image format.


    Written by      Richard Gooch   27-SEP-1995

    Updated by      Richard Gooch   28-SEP-1995

    Updated by      Richard Gooch   16-OCT-1995: Fixed bug in <make_desc> where
  axis count + 1 was improperly used.

    Last updated by Richard Gooch   13-DEC-1995: Fixed bug in <make_desc> where
  Miriad units were not translated into FITS units.


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
#include <karma_dsrw.h>
#include <karma_pio.h>
#include <karma_dmp.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>


#define MIN_ITEM_SIZE 16


/*  Structure declarations  */

typedef struct _header_item
{
    char *name;
    char *data;
    unsigned int type;
    struct _header_item *next;
    double d_data;               /*  Used for axis scaling  */
} *HeaderItem;


/*  Declarations of private functions follow  */
STATIC_FUNCTION (unsigned int get_alignment_padding,
		 (uaddr position, uaddr size) );
STATIC_FUNCTION (flag align_read, (Channel channel, uaddr size) );
STATIC_FUNCTION (unsigned int read_miriad_type, (Channel channel) );
STATIC_FUNCTION (multi_array *make_desc,
		 (HeaderItem first_item, unsigned int naxis, flag data_alloc,
		  flag sanitise) );
STATIC_FUNCTION (HeaderItem find_item,
		 (HeaderItem first_item, CONST char *name) );
STATIC_FUNCTION (void free_header, (HeaderItem first_item) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
multi_array *foreign_miriad_read_header (Channel channel, flag data_alloc,
					 flag sanitise, ...)
/*  [PURPOSE] This routine will read the header of a Miriad Image file from a
    channel.
    The data section is NOT read.
    <channel> The channel to read from.
    <data_alloc> If TRUE, the data space is allocated.
    <sanitise> If TRUE, Miriad axes with length 1 are ignored. This is highly
    recommended.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. The last argument must be FA_MIRIAD_READ_HEADER_END.
    The attributes are passed using varargs.
    [RETURNS] A pointer to the multi_array data structure on success, else
    NULL.
*/
{
    va_list argp;
    flag keep_going;
    HeaderItem first_item = NULL;
    HeaderItem item;
    int item_size, string_size, type_size;
    int naxis = -1;
    unsigned int att_key;
    unsigned int drain_size;
    multi_array *multi_desc;
    char *ch_ptr;
    char item_buf[MIN_ITEM_SIZE];
    extern char host_type_sizes[NUMTYPES];
    extern char *data_type_names[NUMTYPES];
    extern char *sys_errlist[];
    static char function_name[] = "foreign_miriad_read_header";

    va_start (argp, sanitise);
    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (data_alloc);
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) )
	   != FA_MIRIAD_READ_HEADER_END )
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
    keep_going = TRUE;
    while ( keep_going &&
	   (ch_read (channel, item_buf, MIN_ITEM_SIZE) >= MIN_ITEM_SIZE) )
    {
	/*  Have a new item  */
	if ( ( item  = (HeaderItem) m_alloc (sizeof *item) ) == NULL )
	{
	    m_abort (function_name, "header item");
	}
	/*  First comes the item name  */
	if (strlen (item_buf) > MIN_ITEM_SIZE + 2)
	{
	    (void) fprintf (stderr, "Item: \"%s\" name too big\n");
	    free_header (first_item);
	    return (NULL);
	}
	if ( ( item->name = st_dup (item_buf) ) == NULL )
	{
	    m_abort (function_name, "item name");
	}
	/*  Make keywords FITS like (yuk)  */
	(void) st_upr (item->name);
	item_size = item_buf[MIN_ITEM_SIZE - 1];
	if (item_size < 4)
	{
	    (void) fprintf (stderr, "Item: \"%s\" size: %d too small\n",
			    item->name, item_size);
	    free_header (first_item);
	    return (NULL);
	}
	/*  Determine the data type  */
	if ( ( item->type = read_miriad_type (channel) ) == NONE )
	{
	    free_header (first_item);
	    return (NULL);
	}
	if (item->type == K_BYTE)
	{
	    /*  Assume string data  */
	    if ( ( item->data = m_alloc (sizeof ch_ptr) ) == NULL )
	    {
		m_abort (function_name, "string pointer");
	    }
	    string_size = item_size - 4;
	    if ( ( ch_ptr = m_alloc (string_size + 1) ) == NULL )
	    {
		m_abort (function_name, "string");
	    }
	    *(char **) item->data = ch_ptr;
	    if (ch_read (channel, ch_ptr, string_size) < string_size)
	    {
		(void) fprintf (stderr, "Error reading string\t%s\n",
				sys_errlist[errno]);
		free_header (first_item);
		return (NULL);
	    }
	    ch_ptr[string_size] = '\0';
	    item->type = K_VSTRING;
	}
	else
	{
	    type_size = host_type_sizes[item->type];
	    if (type_size + 4 > item_size)
	    {
		(void) fprintf (stderr,
				"Item: \"%s\" size: %d too small for data\n",
				item->name, item_size);
		free_header (first_item);
		return (NULL);
	    }
	    drain_size = get_alignment_padding (4, type_size);
	    if (4 + drain_size + type_size != item_size)
	    {
		(void) fprintf (stderr, "Item: \"%s\" has: %u extra bytes\n",
				item->name, item_size - 4 - drain_size);
		free_header (first_item);
		return (NULL);
	    }
	    if (ch_drain (channel, drain_size) < drain_size)
	    {
		return (NULL);
	    }
	    if ( ( item->data = m_alloc (host_type_sizes[item->type]) )
		== NULL )
	    {
		m_abort (function_name, "item data");
	    }
	    /*  Read data value  */
	    if ( !dsrw_read_element (channel, item->type, NULL, item->data) )
	    {
		free_header (first_item);
		return (NULL);
	    }
	}
	/*  Process the item  */
	if (strcmp (item->name, "NAXIS") == 0)
	{
	    if (item->type != K_INT)
	    {
		(void) fprintf (stderr, "NAXIS keyword not of type integer\n");
		free_header (first_item);
		return (NULL);
	    }
	    if ( (naxis = *(int *) item->data) < 1 )
	    {
		(void) fprintf (stderr, "NAXIS value: %d negative!\n", naxis);
		free_header (first_item);
		return (NULL);
	    }
	}
	/*  Add item to list  */
	item->next = first_item;
	first_item = item;
	/*  The last thing to do is read any padding  */
	keep_going = align_read (channel, MIN_ITEM_SIZE);
    }
    if (naxis < 1)
    {
	(void) fprintf (stderr, "No NAXIS keyword found!\n");
	free_header (first_item);
	return (NULL);
    }
#ifdef DEBUG
    for (item = first_item; item != NULL; item = item->next)
    {
	(void) fprintf (stderr, "Item name: \"%s\" type: %s\tvalue:\t",
			item->name, data_type_names[item->type]);
	dmp_element (stderr, item->type, NULL, item->data, FALSE);
    }
#endif
    /*  Construct Karma data structure  */
    if ( ( multi_desc = make_desc (first_item, naxis, data_alloc, sanitise) )
	== NULL )
    {
	free_header (first_item);
	return (NULL);
    }
    free_header (first_item);
    return (multi_desc);
}   /*  End Function foreign_miriad_read_header  */

/*PUBLIC_FUNCTION*/
multi_array *foreign_miriad_read (CONST char *dirname, flag sanitise, ...)
/*  [PURPOSE] This routine will read a Miriad image file.
    <dirname> The directory name of the Miriad dataset name.
    <sanitise> If TRUE, Miriad axes with length 1 are ignored. This is highly
    recommended.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. The last argument must be FA_MIRIAD_READ_END.
    The attributes are passed using varargs.
    [RETURNS] A multi_array descriptor on success, else NULL.
*/
{
    va_list argp;
    Channel channel;
    unsigned int att_key;
    unsigned long type;
    multi_array *multi_desc;
    char header_name[STRING_LENGTH];
    char image_name[STRING_LENGTH];
    extern char *sys_errlist[];
    static char function_name[] = "foreign_miriad_read";

    va_start (argp, sanitise);
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_MIRIAD_READ_END )
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
    (void) sprintf (header_name, "%s/header", dirname);
    (void) sprintf (image_name, "%s/image", dirname);
    if ( ( channel = ch_open_file (header_name, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening: \"%s\"\t%s\n",
			header_name, sys_errlist[errno]);
	return (NULL);
    }
    multi_desc = foreign_miriad_read_header (channel, TRUE, sanitise,
					     FA_MIRIAD_READ_HEADER_END);
    (void) ch_close (channel);
    if (multi_desc == NULL) return (NULL);
    if ( ( channel = ch_open_file (image_name, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening: \"%s\"\t%s\n",
			image_name, sys_errlist[errno]);
	return (NULL);
    }
    if ( !pio_read32 (channel, &type) )
    {
	(void) fprintf (stderr, "Error reading image data type\t%s\n",
			sys_errlist[errno]);
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    if (type != 4)
    {
	(void) fprintf (stderr, "Image data type: %lu is not 4!\n", type);
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    if ( !foreign_fits_read_data (channel, multi_desc, NULL, 0,
				  FA_FITS_READ_DATA_END) )
    {
	(void) fprintf (stderr, "Error reading image data\t%s\n",
			sys_errlist[errno]);
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    return (multi_desc);
}   /*  End Function foreign_miriad_read  */

/*PUBLIC_FUNCTION*/
flag foreign_miriad_test (CONST char *dirname)
/*  [PURPOSE] This routine will test if a directory is a Miriad Image file.
    <dirname> The directory name of the Miriad dataset name.
    [RETURNS] TRUE if the directory is a Miriad Image file, else FALSE.
*/
{
    struct stat statbuf;
    char header_name[STRING_LENGTH];
    char image_name[STRING_LENGTH];
    extern char *sys_errlist[];

    if (stat (dirname, &statbuf) != 0)
    {
	if (errno == ENOENT) return (FALSE);
	(void) fprintf (stderr, "Error statting file: \"%s\"\t%s\n",
			dirname, sys_errlist[errno]);
	return (FALSE);
    }
    if ( !S_ISDIR (statbuf.st_mode) ) return (FALSE);
    (void) sprintf (header_name, "%s/header", dirname);
    (void) sprintf (image_name, "%s/image", dirname);
    if (stat (header_name, &statbuf) != 0)
    {
	if (errno == ENOENT) return (FALSE);
	(void) fprintf (stderr, "Error statting file: \"%s\"\t%s\n",
			header_name, sys_errlist[errno]);
	return (FALSE);
    }
    if ( !S_ISREG (statbuf.st_mode) ) return (FALSE);
    if (stat (image_name, &statbuf) != 0)
    {
	if (errno == ENOENT) return (FALSE);
	(void) fprintf (stderr, "Error statting file: \"%s\"\t%s\n",
			image_name, sys_errlist[errno]);
	return (FALSE);
    }
    if ( !S_ISREG (statbuf.st_mode) ) return (FALSE);
    return (TRUE);
}   /*  End Function foreign_miriad_test  */


/*  Private functions follow  */

static unsigned int get_alignment_padding (uaddr position, uaddr size)
/*  [PURPOSE] This routine will compute the padding required to ensure a word
    of specified size is aligned to a word boundary.
    <position> The current position.
    <size> The size of the word in bytes.
    [RETURNS] The number of padding bytes required.
*/
{
    uaddr pad;

    pad = position % size;
    if (pad == 0) return (0);
    return (size - pad);
}   /*  End Function get_alignment_padding  */

static flag align_read (Channel channel, uaddr size)
/*  [PURPOSE] This routine will align the next read from a channel to a
    specified boundary.
    <channel> The channel.
    <size> The size to align to.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char ch;
    unsigned long read_pos, write_pos;

    if ( !ch_tell (channel, &read_pos, &write_pos) ) return (FALSE);
    size = get_alignment_padding (read_pos, size);
    if (ch_drain (channel, size) < size) return (FALSE);
    return (TRUE);
}   /*  End Function align_read  */

static unsigned int read_miriad_type (Channel channel)
/*  [PURPOSE] This routine will read the 4 bytes in a Miriad file which contain
    a type value and will convert to Karma type values.
    <channel> The channel to read from.
    [RETURNS] The type value on success, else NONE on error.
*/
{
    unsigned long in_type;
    unsigned int out_type;

    if ( !pio_read32 (channel, &in_type) ) return (NONE);
    switch (in_type)
    {
      case 1:
	out_type = K_BYTE;
	break;
      case 2:
	out_type = K_INT;
	break;
      case 3:
	out_type = K_SHORT;
	break;
      case 4:
	out_type = K_FLOAT;
	break;
      case 5:
	out_type = K_DOUBLE;
	break;
      case 6:
	out_type = K_VSTRING;
	break;
      case 7:
	out_type = K_COMPLEX;
	break;
      default:
	(void) fprintf (stderr, "Bad Miriad out_type: %lu\n", in_type);
	out_type = NONE;
	break;
    }
    return (out_type);
}   /*  End Function read_miriad_type  */

static multi_array *make_desc (HeaderItem first_item, unsigned int naxis,
			       flag data_alloc, flag sanitise)
/*  [PURPOSE] This routine will make a Karma descriptor from a list of Miriad
    header items.
    <first_item> The first header item.
    <naxis> The value of the naxis keyword.
    <data_alloc> If TRUE, the data space is allocated.
    <sanitise> If TRUE, Miriad axes with length 1 are ignored. This is highly
    recommended.
    [RETURN] A multi array descriptor pointer on success, else NULL.
*/
{
    HeaderItem item, crpix_item, crval_item, cdelt_item;
    flag all_dim_names_given = TRUE;
    int axis_length;
    unsigned int num_items, num_dim, axis_count;
    unsigned int elem_type = K_FLOAT;
    double scale;
    packet_desc *top_pack_desc;
    array_desc *arr_desc;
    packet_desc *pack_desc;
    char *top_packet, *array, *element;
    char *elem_name, *name;
    uaddr *dim_lengths;
    double *dim_minima;
    double *dim_maxima;
    char **dim_names;
    multi_array *multi_desc;
    char txt[STRING_LENGTH];
    extern char host_type_sizes[NUMTYPES];
    static char def_elem_name[] = "Data Value";
    static char function_name[] = "__foreign_read_miriad_make_desc";

    /*  Construct the lengths, minima, maxima and names arrays  */
    if ( ( dim_lengths = (uaddr *) m_alloc (sizeof *dim_lengths * naxis) )
	== NULL )
    {
	m_error_notify (function_name, "dimension lengths");
	return (NULL);
    }
    /*  Compute dimension minima and maxima  */
    if ( ( dim_minima = (double *) m_alloc (sizeof *dim_minima * naxis) )
	== NULL )
    {
	m_error_notify (function_name, "dimension minima");
	m_free ( (char *) dim_lengths );
	return (NULL);
    }
    if ( ( dim_maxima = (double *) m_alloc (sizeof *dim_maxima * naxis) )
	== NULL )
    {
	m_error_notify (function_name, "dimension maxima");
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_minima );
	return (NULL);
    }
    /*  Create array of dimension name pointers  */
    if ( ( dim_names = (char **) m_alloc (sizeof *dim_names * naxis) )
	== NULL )
    {
	m_error_notify (function_name, "dimension name pointers");
	m_free ( (char *) dim_lengths );
	m_free ( (char *) dim_minima );
	m_free ( (char *) dim_maxima );
	return (NULL);
    }
    /*  Grab dimension information from header  */
    for (axis_count = naxis, num_dim = 0; axis_count > 0; --axis_count)
    {
	/*  Search for this axis name  */
	(void) sprintf (txt, "NAXIS%d", axis_count);
	if ( ( item = find_item (first_item, txt) ) == NULL )
	{
	    (void) fprintf (stderr, "%s: naxis: %d but no \"%s\" item found\n",
			    function_name, txt);
	    m_free ( (char *) dim_lengths );
	    m_free ( (char *) dim_minima );
	    m_free ( (char *) dim_maxima );
	    m_free ( (char *) dim_names );
	    return (NULL);
	}
	if (item->type != K_INT)
	{
	    (void) fprintf (stderr, "%s: item: \"%s\" not integer type\n",
			  function_name, txt);
	    m_free ( (char *) dim_lengths );
	    m_free ( (char *) dim_minima );
	    m_free ( (char *) dim_maxima );
	    m_free ( (char *) dim_names );
	    return (NULL);
	}
	axis_length = *(int *) item->data;
	if ( sanitise && (axis_length < 2) ) continue;
	/*  Put entry in arrays  */
	/*  First the name  */
	(void) sprintf (txt, "CTYPE%d", axis_count);
	if ( ( item = find_item (first_item, txt) ) == NULL )
	{
	    all_dim_names_given = FALSE;
	    scale = 1.0;
	}
	else
	{
	    dim_names[num_dim] = *(char **) item->data;
	    name = dim_names[num_dim];
	    if (st_nicmp (name, "RA---", 5) == 0) scale = 180.0 / PI;
	    else if (st_nicmp (name, "DEC--", 5) == 0) scale = 180.0 / PI;
	    else if (st_nicmp (name, "GLON-", 5) == 0) scale = 180.0 / PI;
	    else if (st_nicmp (name, "GLAT-", 5) == 0) scale = 180.0 / PI;
	    else if (st_nicmp (name, "ELON-", 5) == 0) scale = 180.0 / PI;
	    else if (st_nicmp (name, "ELAT-", 5) == 0) scale = 180.0 / PI;
	    else if (st_nicmp (name, "FREQ", 4) == 0) scale = 1e9;
	    else if (st_nicmp (name, "VELO", 4) == 0) scale = 1e3;
	    else if (st_nicmp (name, "FELO", 4) == 0) scale = 1e3;
	    else if (st_nicmp (name, "RESTFREQ", 8) == 0) scale = 1e3;
	    else scale = 1.0;
	}
	dim_lengths[num_dim] = axis_length;
	(void) sprintf (txt, "CRPIX%d", axis_count);
	if ( ( crpix_item = find_item (first_item, txt) ) != NULL )
	{
	    if (crpix_item->type == K_DOUBLE)
	    {
		crpix_item->d_data = *(double *) crpix_item->data;
	    }
	    else if (crpix_item->type == K_FLOAT)
	    {
		crpix_item->d_data = *(float *) crpix_item->data;
	    }
	    else
	    {
		(void) fprintf (stderr,
				"%s: warning: \"%s\" not float or double\n",
				function_name, txt);
		crpix_item = NULL;
	    }
	}
	(void) sprintf (txt, "CRVAL%d", axis_count);
	if ( ( crval_item = find_item (first_item, txt) ) != NULL )
	{
	    if (crval_item->type == K_DOUBLE)
	    {
		*(double *) crval_item->data *= scale;
		crval_item->d_data = *(double *) crval_item->data;
	    }
	    else if (crval_item->type == K_FLOAT)
	    {
		*(float *) crval_item->data *= scale;
		crval_item->d_data = *(float *) crval_item->data;
	    }
	    else
	    {
		(void) fprintf (stderr,
				"%s: warning: \"%s\" not float or double\n",
				function_name, txt);
		crval_item = NULL;
	    }
	}
	(void) sprintf (txt, "CDELT%d", axis_count);
	if ( ( cdelt_item = find_item (first_item, txt) ) != NULL )
	{
	    if (cdelt_item->type == K_DOUBLE)
	    {
		*(double *) cdelt_item->data *= scale;
		cdelt_item->d_data = *(double *) cdelt_item->data;
	    }
	    else if (cdelt_item->type == K_FLOAT)
	    {
		*(float *) cdelt_item->data *= scale;
		cdelt_item->d_data = *(float *) cdelt_item->data;
	    }
	    else
	    {
		(void) fprintf (stderr,
				"%s: warning: \"%s\" not float or double\n",
				function_name, txt);
		cdelt_item = NULL;
	    }
	}
	if ( (crpix_item == NULL) || (crval_item == NULL) ||
	    (cdelt_item == NULL) )
	{
	    dim_minima[num_dim] = 0.0;
	    dim_maxima[num_dim] = (double) (axis_length - 1);
	}
	else
	{
	    dim_minima[num_dim] = (crval_item->d_data - crpix_item->d_data *
				   cdelt_item->d_data);
	    dim_maxima[num_dim] = ( dim_minima[num_dim] +
				   fabs (cdelt_item->d_data) *
				   (double) (axis_length - 1) );
	}
	++num_dim;
    }
    (void) sprintf (txt, "BUNIT");
    if ( ( item = find_item (first_item, txt) ) == NULL )
    {
	elem_name = def_elem_name;
    }
    else elem_name = *(char **) item->data;
    if ( (elem_name != NULL) && (strlen (elem_name) < 1) )
    {
	(void) fprintf (stderr,
			"WARNING: BUNIT is an empty string: defaulting to ");
	(void) fprintf (stderr, "\"%s\"\n", def_elem_name);
	elem_name = def_elem_name;
    }
    /*  Create the array descriptor  */
    arr_desc = ds_easy_alloc_array_desc (num_dim, dim_lengths, dim_minima,
					 dim_maxima, (double **) NULL,
					 all_dim_names_given ? dim_names :NULL,
					 1, &elem_type, &elem_name);
    
    m_free ( (char *) dim_lengths );
    m_free ( (char *) dim_minima );
    m_free ( (char *) dim_maxima );
    m_free ( (char *) dim_names );
    if (arr_desc == NULL) return (NULL);
    if ( ( multi_desc = ds_alloc_multi (1) ) == NULL )
    {
	m_abort (function_name, "multi array header");
    }
    for (item = first_item, num_items = 0; item != NULL; item = item->next)
    {
	++num_items;
    }
    if ( ( top_pack_desc = ds_alloc_packet_desc (num_items + 2) )
	== NULL )
    {
	m_abort (function_name, "top level packet descriptor");
    }
    multi_desc->headers[0] = top_pack_desc;
    /*  Fill in the packet descriptor  */
    top_pack_desc->element_desc[0] = (char *) arr_desc;
    top_pack_desc->element_types[0] = K_ARRAY;
    for (item = first_item, num_items = 0; item != NULL;
	 item = item->next, ++num_items)
    {
	top_pack_desc->element_types[num_items + 1] = item->type;
	if ( ( top_pack_desc->element_desc[num_items + 1] =
	      st_dup (item->name) ) == NULL )
	{
	    m_abort (function_name, "element name");
	}
    }
    top_pack_desc->element_types[num_items + 1] = K_INT;
    top_pack_desc->element_desc[num_items + 1] = st_dup ("BITPIX");
    /*  Create and fill the packet  */
    if ( ( top_packet = ds_alloc_packet (top_pack_desc) ) == NULL )
    {
	m_abort (function_name, "top level packet");
    }
    multi_desc->data[0] = top_packet;
    if (data_alloc)
    {
	if ( !ds_alloc_array (arr_desc, top_packet, TRUE, TRUE) )
	{
	    m_abort (function_name, "array");
	}
    }
    else
    {
	*(array_desc **) top_packet = NULL;
	*(unsigned int *) ( top_packet+sizeof (char *) ) = K_ARRAY_UNALLOCATED;
    }
    for (item = first_item, num_items = 0; item != NULL;
	 item = item->next, ++num_items)
    {
	element = top_packet + ds_get_element_offset (top_pack_desc,
						      num_items + 1);
	m_copy (element, item->data, host_type_sizes[item->type]);
    }
    element = top_packet + ds_get_element_offset (top_pack_desc,
						  num_items + 1);
    *(int *) element = -32;
    return (multi_desc);
}   /*  End Function make_desc  */

static HeaderItem find_item (HeaderItem first_item, CONST char *name)
/*  [PURPOSE] This routine will find an item in the header with given name.
    <first_item> The first item in the header.
    <name> The name of the item.
    [RETURNS] The item on sucess, else NULL.
*/
{
    HeaderItem item;

    for (item = first_item; item != NULL; item = item->next)
    {
	if (strcmp (item->name, name) == 0) return (item);
    }
    return (NULL);
}   /*  End Function find_item  */

static void free_header (HeaderItem first_item)
/*  [PURPOSE] This routine will free a header.
    <first_item> The first item in the header.
    [RETURNS] Nothing.
*/
{
    HeaderItem next;

    while (first_item != NULL)
    {
	m_free (first_item->name);
	m_free (first_item->data);
	next = first_item->next;
	m_free ( (char *) first_item );
	first_item = next;
    }
}   /*  End Function free_header  */
