/*  karma_p.h

    Header for  p_  package.

    Copyright (C) 1992-1996  Richard Gooch

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

    Last updated by Richard Gooch   7-SEP-1996

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_P_H
#define KARMA_P_H



/*  For the file: s16.c  */
EXTERN_FUNCTION (flag p_write_buf16s, (char *buffer, long data) );
EXTERN_FUNCTION (flag p_read_buf16s, (char *buffer, long *data) );

/*  For the file: s32.c  */
EXTERN_FUNCTION (flag p_write_buf32s, (char *buffer, long data) );
EXTERN_FUNCTION (flag p_read_buf32s, (char *buffer, long *data) );

/*  For the file: u16.c  */
EXTERN_FUNCTION (flag p_write_buf16, (char *buffer, unsigned long data) );
EXTERN_FUNCTION (flag p_read_buf16, (char *buffer, unsigned long *data) );

/*  For the file: u32.c  */
EXTERN_FUNCTION (flag p_write_buf32, (char *buffer, unsigned long data) );
EXTERN_FUNCTION (flag p_read_buf32, (char *buffer, unsigned long *data) );

/*  For the file: u64.c  */
EXTERN_FUNCTION (flag p_write_buf64, (char *buffer, unsigned long data) );
EXTERN_FUNCTION (flag p_read_buf64, (char *buffer, unsigned long *data) );

/*  For the file: float.c  */
EXTERN_FUNCTION (flag p_write_buf_float, (char *buffer, float data) );
EXTERN_FUNCTION (flag p_read_buf_float, (CONST char *buffer, float *data) );
EXTERN_FUNCTION (flag p_read_buf_floats,
		 (CONST char *buffer, uaddr num_values, float *data,
		  uaddr *num_nan) );
EXTERN_FUNCTION (flag p_write_buf_double, (char *buffer, double data) );
EXTERN_FUNCTION (flag p_read_buf_double, (CONST char *buffer, double *data) );
EXTERN_FUNCTION (flag p_read_buf_doubles,
		 (CONST char *buffer, uaddr num_values, double *data,
		  uaddr *num_nan) );


#endif /*  KARMA_P_H  */
