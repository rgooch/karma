/*  karma_arln.h

    Header for  arln_  package.

    Copyright (C) 1992,1993  Richard Gooch

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

    This include file contains all the definitions and function declarations
  needed to interface to the arln_ routines in the Karma library.


    Written by      Richard Gooch   11-OCT-1992

    Last updated by Richard Gooch   30-DEC-1992

*/

#ifndef KARMA_ARLN_H
#define KARMA_ARLN_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#ifndef KARMA_H
#  include <karma.h>
#endif


/*  File:   arln.c   */
EXTERN_FUNCTION (flag arln_read_from_stdin, (char *buffer, unsigned int length,
					     char *prompt) );
EXTERN_FUNCTION (float arln_read_float, (char *prompt, float default_value) );
EXTERN_FUNCTION (int arln_read_int, (char *prompt, int default_value) );
EXTERN_FUNCTION (flag arln_read_flag, (char *prompt, flag default_value) );
EXTERN_FUNCTION (flag arln_read_line, (char *buffer, unsigned int length,
				       char *prompt) );


#endif /*  KARMA_ARLN_H  */
