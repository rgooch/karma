/*LINTLIBRARY*/
/*  ppm_read.c

    This code provides a PPM read facility.

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

    This file contains the various utility routines for reading colour images
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
#include <ctype.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_chs.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_m.h>
#include <karma_a.h>


/*  Declarations of private functions follow  */
STATIC_FUNCTION (unsigned int get_value,
		 (Channel channel, char *string, unsigned int length,
		  flag *newline) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
multi_array *foreign_ppm_read (Channel channel, ...)
/*  [PURPOSE] This routine will read a colour image in PPM format from a
    channel.
    <channel> The channel to read from.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. The last argument must be FA_PPM_READ_END.
    The attributes are passed using varargs.
    [RETURNS] A pointer to the multi_array data structure on success, else NULL
*/
{
    flag binary, newline;
    int max_value;
    int width, height, y;
    int value;
    char ch;
    unsigned int att_key;
    unsigned int bytes_to_read, bytes_read;
    va_list argp;
    multi_array *multi_desc;
    char *array, *p;
    char txt[STRING_LENGTH];
    uaddr lengths[2];
    extern char *sys_errlist[];
    static unsigned int elem_types[3] = {K_UBYTE, K_UBYTE, K_UBYTE};
    static char *elem_names[3] =
    {   "Red Intensity", "Green Intensity", "Blue Intensity"   };
    static char function_name[] = "foreign_ppm_read";

    va_start (argp, channel);
    if ( channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_PPM_READ_END )
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
    /*  Read magic number  */
    if (ch_read (channel, txt, 3) < 3)
    {
	(void) fprintf (stderr, "Error reading\t%s\n", sys_errlist[errno]);
	return (FALSE);
    }
    if (strncmp (txt, "P3", 2) == 0) binary = FALSE;
    else if (strncmp (txt, "P6", 2) == 0) binary = TRUE;
    else
    {
	(void) fprintf (stderr, "Input not of PPM format\n");
	return (FALSE);
    }
    if ( !isspace (txt[2]) )
    {
	(void) fprintf (stderr,
			"Input not of PPM format (whitespace missing)\n");
	return (FALSE);
    }
    /*  Read width  */
    if (get_value (channel, txt, STRING_LENGTH, &newline) < 1)
    {
	(void) fprintf (stderr, "Error reading width\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if ( ( width = ex_int (txt, &p) ) < 1 )
    {
	(void) fprintf (stderr, "Bad width: \"%s\"\n", txt);
	return (FALSE);
    }
    /*  Read height  */
    if (get_value (channel, txt, STRING_LENGTH, &newline) < 1)
    {
	(void) fprintf (stderr, "Error reading height\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if ( ( height = ex_int (txt, &p) ) < 1 )
    {
	(void) fprintf (stderr, "Bad height: \"%s\"\n", txt);
	return (FALSE);
    }
    /*  Read max_value  */
    if (get_value (channel, txt, STRING_LENGTH, &newline) < 1)
    {
	(void) fprintf (stderr, "Error reading max_value\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if ( ( max_value = ex_int (txt, &p) ) < 1 )
    {
	(void) fprintf (stderr, "Bad max_value: \"%s\"\n", txt);
	return (FALSE);
    }
    lengths[0] = height;
    lengths[1] = width;
    if ( ( array = ds_easy_alloc_n_element_array (&multi_desc, 2, lengths,
						  (double *) NULL,
						  (double *) NULL,
						  (char **) NULL,
						  3, elem_types,
						  elem_names) ) == NULL )
    {
	return (NULL);
    }
    /*  Read the pixel data  */
    bytes_to_read = width * 3;
    if (binary)
    {
	if (!newline)
	{
	    /*  Drain until the next line  */
	    ch = ' ';
	    while ( (ch != '\n') && (ch_read (channel, &ch, 1) == 1) );
	}
	for (y = height - 1; y >= 0; --y)
	{
	    p = array + y * bytes_to_read;
	    if ( ( bytes_read = ch_read (channel, p, bytes_to_read) )
		< bytes_to_read )
	    {
		(void) fprintf (stderr,
				"Error reading: %u bytes, got: %u\t%s\n",
				bytes_to_read, bytes_read, sys_errlist[errno]);
		ds_dealloc_multi (multi_desc);
		return (NULL);
	    }
	}
	return (multi_desc);
    }
    /*  Read ASCII  */
    for (y = height - 1; y >= 0; --y)
    {
	p = array + y * bytes_to_read;
	for (bytes_read = 0; bytes_read < bytes_to_read; ++bytes_read)
	{
	    if (get_value (channel, txt, STRING_LENGTH, &newline) < 1)
	    {
		ds_dealloc_multi (multi_desc);
		return (NULL);
	    }
	    if ( ( value = ex_int (txt, &p) ) < 0 )
	    {
		ds_dealloc_multi (multi_desc);
		return (NULL);
	    }
	    *(unsigned char *) (p + bytes_read) = value;
	}
    }
    return (multi_desc);
}   /*  End Function foreign_ppm_read  */


/*  Private functions follow  */

static unsigned int get_value (Channel channel, char *string,
			       unsigned int length, flag *newline)
/*  [PURPOSE] This routine will scan a channel object for a whitespace
    separated value. '#' is the comment character.
    <channel> The channel to read from.
    <string> The buffer to write the value to.
    <length> The size of the buffer (in bytes).
    <newline> The value TRUE is written here if a newline was read, else FALSE
    is written here.
    [RETURNS] The length of the string scanned on success, else 0.
*/
{
    unsigned int char_pos;
    char ch;
    static char function_name[] = "chs_get_value";

    char_pos = 0;
    *newline = FALSE;
    while (ch_read (channel, string + char_pos, 1) == 1)
    {
	if (string[char_pos] == '#')
	{
	    /*  Skip until next newline and count as whitespace  */
	    ch = '\0';
	    while ( (ch != '\n') && (ch_read (channel, &ch, 1) == 1) );
	    if (ch != '\n') return (char_pos);
	    string[char_pos] = '\n';
	}
	/*  Have another character  */
	if ( isspace (string[char_pos]) )
	{
	    /*  Have a whitespace character  */
	    if (char_pos > 0)
	    {
		/*  Have read in some non-whitespace already  */
		if (string[char_pos] == '\n') *newline = TRUE;
		return (char_pos);
	    }
	}
	else
	{
	    /*  Non-whitespace  */
	    if (++char_pos >= length)
	    {
		a_func_abort (function_name, "value too large for buffer");
		return (0);
	    }
	}
    }
    return (char_pos);
}   /*  End Function get_value  */
