/*LINTLIBRARY*/
/*  guess_read.c

    This code provides miscellaneous functions for the <foreign_> package.

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

    This file contains the various utility routines for the <foreign_> package.


    Written by      Richard Gooch   23-APR-1995

    Updated by      Richard Gooch   24-APR-1995: Created
  FA_GUESS_READ_FITS_TO_FLOAT  attribute.

    Updated by      Richard Gooch   6-MAY-1995: Placate gcc -Wall

    Updated by      Richard Gooch   21-MAY-1995: Added support for Sun
  rasterfile format.

    Updated by      Richard Gooch   28-SEP-1995: Added support for Miriad Image
  format.

    Updated by      Richard Gooch   12-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   21-JUN-1996: Added support for GIPSY format


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
#include <karma_dsxfr.h>
#include <karma_ch.h>
#include <karma_a.h>



/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
multi_array *foreign_guess_and_read (CONST char *filename,
				     unsigned int mmap_option, flag writeable,
				     unsigned int *ftype, ...)
/*  [SUMMARY] Guess file type and read.
    [PURPOSE] This routine will attempt to guess the filetype of a file and
    in the file, converting to the Karma data format if possible.
    <filename> The name of the file to read.
    <mmap_option> This has the same meaning as for the <dsxfr_get_multi>
    routine.
    <writeable> This has the same meaning as for the <dsxfr_get_multi> routine.
    <ftype> The type of the file that was read in is written here.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must terminated with FA_GUESS_READ_END.
    See [<FOREIGN_ATT_GUESS>] for a list of defined attributes.
    [RETURNS] A pointer to the multi_array data structure on success, else
    NULL.
*/
{
    Channel inp;
    flag fits_convert_to_float = FALSE;
    unsigned int att_key;
    va_list argp;
    multi_array *multi_desc = NULL;  /*  Initialised to keep compiler happy  */
    extern char *sys_errlist[];
    static char function_name[] = "foreign_guess_and_read";

    va_start (argp, ftype);
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_GUESS_READ_END )
    {
	switch (att_key)
	{
	  case FA_GUESS_READ_FITS_TO_FLOAT:
	    fits_convert_to_float = va_arg (argp, flag);
	    FLAG_VERIFY (fits_convert_to_float);
	    break;
	  default:
	    (void) fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    *ftype = foreign_guess_format_from_filename (filename);
    if (*ftype == FOREIGN_FILE_FORMAT_UNKNOWN) return (NULL);
    if ( (mmap_option == K_CH_MAP_ALWAYS) &&
	(*ftype != FOREIGN_FILE_FORMAT_KARMA) ) return (NULL);
    switch (*ftype)
    {
      case FOREIGN_FILE_FORMAT_KARMA:
	if ( ( multi_desc = dsxfr_get_multi (filename, FALSE,
					     mmap_option, writeable) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error getting arrayfile: \"%s\"\n",
			    filename);
	    return (NULL);
	}
	break;
      case FOREIGN_FILE_FORMAT_PPM:
	/*  Read PPM file  */
	if ( ( inp = ch_open_file (filename, "r") ) == NULL )
	{
	    (void) fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	    return (NULL);
	}
	if ( ( multi_desc = foreign_ppm_read (inp,
					      FA_PPM_READ_END) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error reading PPM file\n");
	    (void) ch_close (inp);
	    return (NULL);
	}
	(void) ch_close (inp);
	break;
      case FOREIGN_FILE_FORMAT_FITS:
	/*  Read FITS file  */
	if ( ( inp = ch_open_file (filename, "r") ) == NULL )
	{
	    (void) fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	    return (NULL);
	}
	if ( ( multi_desc =foreign_fits_read_header (inp, TRUE,
						     fits_convert_to_float,
						     TRUE,
						     FA_FITS_READ_HEADER_END) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error reading FITS file header\n");
	    (void) ch_close (inp);
	    return (NULL);
	}
	if ( !foreign_fits_read_data (inp, multi_desc, NULL, 0,
				      FA_FITS_READ_DATA_END) )
	{
	    (void) fprintf (stderr, "Error reading FITS file data\n");
	    (void) ch_close (inp);
	    return (NULL);
	}
	(void) ch_close (inp);
	break;
      case FOREIGN_FILE_FORMAT_SUNRAS:
	/*  Read Sun rasterfile  */
	if ( ( inp = ch_open_file (filename, "r") ) == NULL )
	{
	    (void) fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	    return (NULL);
	}
	if ( ( multi_desc = foreign_sunras_read (inp,
						 FA_SUNRAS_READ_END) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error reading Sun rasterfile\n");
	    (void) ch_close (inp);
	    return (NULL);
	}
	(void) ch_close (inp);
	break;
      case FOREIGN_FILE_FORMAT_MIRIAD:
	if ( ( multi_desc = foreign_miriad_read (filename, TRUE,
						 FA_MIRIAD_READ_END) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error reading Miriad Image file\n");
	    return (NULL);
	}
	break;
      case FOREIGN_FILE_FORMAT_GIPSY:
	if ( ( multi_desc = foreign_gipsy_read (filename, TRUE,
						FA_GIPSY_READ_END) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error reading GIPSY file\n");
	    return (NULL);
	}
	break;
      default:
	(void) fprintf (stderr, "Illegal filetype: %u\n", *ftype);
	a_prog_bug (function_name);
	break;
    }
    return (multi_desc);
}   /*  End Function foreign_guess_and_read  */
