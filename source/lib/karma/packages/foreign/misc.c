/*LINTLIBRARY*/
/*  misc.c

    This code provides miscellaneous functions for the <foreign> package.

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

    This file contains the various utility routines for the <foreign> package.


    Written by      Richard Gooch   20-APR-1995: 

    Updated by      Richard Gooch   20-APR-1995

    Updated by      Richard Gooch   6-MAY-1995: Added  #include <karma_st.h>

    Updated by      Richard Gooch   21-MAY-1995: Added support for Sun
  rasterfile format.

    Updated by      Richard Gooch   28-SEP-1995: Added support for Miriad Image
  format.

    Updated by      Richard Gooch   10-JAN-1996: Recognise *.mt files as FITS
  files (blunder in GIPSY which thinks all FITS is on tape).

    Updated by      Richard Gooch   12-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   21-JUN-1996: Added support for GIPSY files.

    Last updated by Richard Gooch   3-DEC-1996: Added support for PGM files.


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
#include <karma_st.h>



/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
unsigned int foreign_guess_format_from_filename (CONST char *filename)
/*  [SUMMARY] Attempt to guess the format of a file by examining its filename.
    <filename> The name of the file.
    [RETURNS] A value indicating the format of the file. The value
    FOREIGN_FILE_FORMAT_KARMA is returned if the extension is ".kf". See
    [<FOREIGN_TYPES>] for a list of possible values.
*/
{
    CONST char *end;

    if (strcmp (filename,"connection") ==0) return (FOREIGN_FILE_FORMAT_KARMA);
    end = filename + strlen (filename);
    if (strcmp  (end - 3, ".kf")   == 0) return (FOREIGN_FILE_FORMAT_KARMA);
    if (st_icmp (end - 3, ".mt")  == 0) return (FOREIGN_FILE_FORMAT_FITS);
    if (strcmp  (end - 4, ".pgm")  == 0) return (FOREIGN_FILE_FORMAT_PGM);
    if (strcmp  (end - 4, ".ppm")  == 0) return (FOREIGN_FILE_FORMAT_PPM);
    if (st_icmp (end - 4, ".fts")  == 0) return (FOREIGN_FILE_FORMAT_FITS);
    if (st_icmp (end - 4, ".fit")  == 0) return (FOREIGN_FILE_FORMAT_FITS);
    if (strcmp  (end - 4, ".ras")  == 0) return (FOREIGN_FILE_FORMAT_SUNRAS);
    if (st_icmp (end - 5, ".fits") == 0) return (FOREIGN_FILE_FORMAT_FITS);
    if ( foreign_miriad_test (filename) ) return (FOREIGN_FILE_FORMAT_MIRIAD);
    if ( foreign_gipsy_test (filename) ) return (FOREIGN_FILE_FORMAT_GIPSY);
    return (FOREIGN_FILE_FORMAT_UNKNOWN);
}   /*  End Function foreign_guess_format_from_filename  */
