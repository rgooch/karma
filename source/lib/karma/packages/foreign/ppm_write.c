/*LINTLIBRARY*/
/*  ppm_write.c

    This code provides a PPM write facility.

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

    This file contains the various utility routines for writing colour images
    in PPM format.


    Written by      Richard Gooch   15-APR-1995

    Last updated by Richard Gooch   16-APR-1995


*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_iarray.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>



/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag foreign_ppm_write (Channel channel, multi_array *multi_desc, flag binary,
			...)
/*  [PURPOSE] This routine will write a colour image to a channel in PPM format
    <channel> The channel to write to. The channel is not flushed.
    <multi_desc> The multi_array descriptor pointer. The routine will find a
    TrueColour image or a PseudoColour image within the data structure.
    <binary> If TRUE, the pixels will be written in binary mode.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. The last argument must be FA_PPM_WRITE_END.
    The attributes are passed using varargs.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    iarray image_pseudo;
    iarray image_red, image_green, image_blue;
    unsigned int cmap_index;
    unsigned int att_key;
    int width, height, x, y;
    va_list argp;
    char txt[STRING_LENGTH];
    unsigned char pixel[3];
    extern char module_lib_version[STRING_LENGTH + 1];
    extern char karma_library_version[STRING_LENGTH + 1];
    static char function_name[] = "foreign_ppm_write";

    va_start (argp, binary);
    FLAG_VERIFY (binary);
    if ( ( channel == NULL) || (multi_desc == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_PPM_WRITE_END )
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
    if ( !iarray_get_image_from_multi (multi_desc, &image_pseudo,
				       &image_red, &image_green, &image_blue,
				       &cmap_index) )
    {
	return (FALSE);
    }
    /*  Sanity checks  */
    if (image_red != NULL)
    {
	/*  Have RGB image  */
	if ( (iarray_type (image_red) != K_UBYTE) ||
	     (iarray_type (image_green) != K_UBYTE) ||
	     (iarray_type (image_blue) != K_UBYTE) )
	{
	    (void) fprintf (stderr,
			    "TrueColour image data must be of type K_UBYTE\n");
	    iarray_dealloc (image_red);
	    iarray_dealloc (image_green);
	    iarray_dealloc (image_blue);
	    return (FALSE);
	}
	width = iarray_dim_length (image_red, 1);
	height = iarray_dim_length (image_red, 0);
    }
    else
    {
	a_func_abort (function_name,  "PseudoColour images not supported yet");
	return (FALSE);
    }
    if (binary)
    {
	if ( !ch_puts (channel, "P6\n# Binary", FALSE) ) return (FALSE);
    }
    else
    {
	if ( !ch_puts (channel, "P3\n# ASCII", FALSE) ) return (FALSE);
    }
    if ( !ch_puts (channel, " PPM file written by  foreign_ppm_write",
		   TRUE) ) return (FALSE);
    (void) sprintf (txt,
		    "# Karma library version: %s\n# Module compiled with library version: %s",
		    karma_library_version, module_lib_version);
    if ( !ch_puts (channel, txt, TRUE) ) return (FALSE);
    (void) sprintf (txt, "%u %u # width height\n255 # max value",
		    width, height);
    if ( !ch_puts (channel, txt, TRUE) ) return (FALSE);
    if (image_red != NULL)
    {
	for (y = height - 1; y >= 0; --y) for (x = 0; x < width; ++x)
	{
	    pixel[0] = UB2 (image_red, y, x);
	    pixel[1] = UB2 (image_green, y, x);
	    pixel[2] = UB2 (image_blue, y, x);
	    if (binary)
	    {
		if (ch_write (channel, (char *) pixel, 3) < 3)
		{
		    iarray_dealloc (image_red);
		    iarray_dealloc (image_green);
		    iarray_dealloc (image_blue);
		    return (FALSE);
		}
	    }
	    else
	    {
		(void) sprintf (txt, "%u %u %u", pixel[0], pixel[1], pixel[2]);
		if ( !ch_puts (channel, txt, TRUE) )
		{
		    iarray_dealloc (image_red);
		    iarray_dealloc (image_green);
		    iarray_dealloc (image_blue);
		    return (FALSE);
		}
	    }
	}
	iarray_dealloc (image_red);
	iarray_dealloc (image_green);
	iarray_dealloc (image_blue);
    }
    else
    {
	iarray_dealloc (image_pseudo);
    }
    return (TRUE);
}   /*  End Function foreign_ppm_write  */
