/*  karma_p.h

    Header for  p_  package.

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
  needed to interface to the p_ routines in the Karma library.


    Written by      Richard Gooch   12-SEP-1992

    Last updated by Richard Gooch   13-FEB-1993

*/

#ifndef KARMA_P_H
#define KARMA_P_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#ifndef KARMA_H
#  include <karma.h>
#endif


/*  For the file: portable.c  */
EXTERN_FUNCTION (flag p_write_buf64, (char *buffer, unsigned long data) );
EXTERN_FUNCTION (flag p_read_buf64, (char *buffer, unsigned long *data) );
EXTERN_FUNCTION (flag p_write_buf32, (char *buffer, unsigned long data) );
EXTERN_FUNCTION (flag p_read_buf32, (char *buffer, unsigned long *data) );
EXTERN_FUNCTION (flag p_write_buf16, (char *buffer, unsigned long data) );
EXTERN_FUNCTION (flag p_read_buf16, (char *buffer, unsigned long *data) );
EXTERN_FUNCTION (flag p_write_buf32s, (char *buffer, long data) );
EXTERN_FUNCTION (flag p_read_buf32s, (char *buffer, long *data) );
EXTERN_FUNCTION (flag p_write_buf16s, (char *buffer, long data) );
EXTERN_FUNCTION (flag p_read_buf16s, (char *buffer, long *data) );


#endif /*  KARMA_P_H  */
