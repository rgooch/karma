/*LINTLIBRARY*/
/*  sunras_read.c

    This code provides a Sun rasterfile read facility.

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

    This file contains the various utility routines for reading images in 
  Sun rasterfile format.


    Written by      Richard Gooch   21-MAY-1995

    Updated by      Richard Gooch   21-MAY-1995

    Updated by      Richard Gooch   24-AUG-1995: Added code to pad image lines
  to 16 bits.

    Last updated by Richard Gooch   12-APR-1996: Changed to new documentation
  format.


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
#include <karma_a.h>


#define RAS_MAGIC 0x59a66a95

#define RT_OLD          0
#define RT_STANDARD     1
#define RT_BYTE_ENCODED 2
#define RT_FORMAT_RGB   3
#define RT_FORMAT_TIFF  4
#define RT_FORMAT_IFF   5
#define RT_EXPERIMENTAL 0xffff

#define RMT_NONE        0
#define RMT_EQUAL_RGB   1
#define RMT_RAW         2

/*  Rasterfile structure  */
struct rasterfile
{
    int ras_magic;
    int ras_width;
    int ras_height;
    int ras_depth;
    int ras_length;
    int ras_type;
    int ras_maptype;
    int ras_maplength;
};


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
multi_array *foreign_sunras_read (Channel channel, ...)
/*  [SUMMARY] Read an image in Sun rasterfile format from a channel.
    <channel> The channel to read from.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_SUNRAS_READ_END. See [<FOREIGN_ATT_SUNRAS_READ>] for a list of defined
    attributes.
    [RETURNS] A pointer to the multi_array data structure on success, else NULL
*/
{
    flag format_rgb = TRUE;
    int x, y;
    long val;
    unsigned int att_key;
    va_list argp;
    struct rasterfile header;
    multi_array *multi_desc;
    char *array, *p;
    char pixel[3];
    uaddr lengths[2];
    static unsigned int elem_types[3] = {K_UBYTE, K_UBYTE, K_UBYTE};
    static char *elem_names[3] =
    {   "Red Intensity", "Green Intensity", "Blue Intensity"   };
    static char function_name[] = "foreign_sunras_read";

    va_start (argp, channel);
    if ( channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_SUNRAS_READ_END )
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
    /*  Read header  */
    if ( !pio_read32s (channel, &val) ) return (NULL);
    header.ras_magic = val;
    if ( !pio_read32s (channel, &val) ) return (NULL);
    header.ras_width = val;
    if ( !pio_read32s (channel, &val) ) return (NULL);
    header.ras_height = val;
    if ( !pio_read32s (channel, &val) ) return (NULL);
    header.ras_depth = val;
    if ( !pio_read32s (channel, &val) ) return (NULL);
    header.ras_length = val;
    if ( !pio_read32s (channel, &val) ) return (NULL);
    header.ras_type = val;
    if ( !pio_read32s (channel, &val) ) return (NULL);
    header.ras_maptype = val;
    if ( !pio_read32s (channel, &val) ) return (NULL);
    header.ras_maplength = val;
    /*  Check header  */
    if (header.ras_magic != RAS_MAGIC)
    {
	(void) fprintf (stderr, "Input not of rasterfile format\n");
	return (NULL);
    }
    if (header.ras_width < 1)
    {
	(void) fprintf (stderr, "Bad rasterfile width: %d\n",
			header.ras_width);
	return (NULL);
    }
    if (header.ras_height < 1)
    {
	(void) fprintf (stderr, "Bad rasterfile height: %d\n",
			header.ras_height);
	return (NULL);
    }
    switch (header.ras_depth)
    {
      case 1:
      case 8:
      case 24:
	break;
      default:
	(void) fprintf (stderr, "Bad rasterfile depth: %d\n",
			header.ras_depth);
	return (NULL);
/*
	break;
*/
    }
    switch (header.ras_type)
    {
      case RT_OLD:
      case RT_STANDARD:
	format_rgb = FALSE;
      case RT_FORMAT_RGB:
	format_rgb = TRUE;
	break;
	default:
	(void) fprintf (stderr, "Rasterfile type: %d not supported\n",
			header.ras_type);
	return (NULL);
/*
	break;
*/
    }
    if (header.ras_depth != 24)
    {
	(void) fprintf (stderr, "Depth: %d not supported yet\n",
			header.ras_depth);
	return (NULL);
    }
    (void) fprintf (stderr, "maptype: %d  maplength: %d\n",
		    header.ras_maptype, header.ras_maplength);
    lengths[0] = header.ras_height;
    lengths[1] = header.ras_width;
    if ( ( array = ds_easy_alloc_n_element_array
	   (&multi_desc, 2, lengths,
	    (CONST double *) NULL, (CONST double *) NULL,
	    (CONST char **) NULL, 3, elem_types,
	    (CONST char **) elem_names) ) == NULL )
    {
	return (NULL);
    }
    /*  Read the pixel data  */
    for (y = header.ras_height - 1; y >= 0; --y)
    {
	p = array + y * header.ras_width * 3;
	for (x = 0; x < header.ras_width; ++x, p += 3)
	{
	    if (ch_read (channel, pixel, 3) < 3)
	    {
		ds_dealloc_multi (multi_desc);
		return (NULL);
	    }
	    if (format_rgb)
	    {
		p[0] = pixel[0];
		p[1] = pixel[1];
		p[2] = pixel[2];
	    }
	    else
	    {
		p[0] = pixel[2];
		p[1] = pixel[1];
		p[2] = pixel[0];
	    }
	}
	/*  Line read: maybe need to read pad  */
	if (header.ras_width & 1)
	{
	    if (ch_read (channel, pixel, 1) < 1)
	    {
		ds_dealloc_multi (multi_desc);
		return (NULL);
	    }
	}
    }
    return (multi_desc);
}   /*  End Function foreign_sunras_read  */
