/*  karma_s.h

    Header for  s_  package.

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
  needed to interface to the s_ routines in the Karma library.


    Written by      Richard Gooch   12-SEP-1992

    Last updated by Richard Gooch   12-APR-1992

*/

#ifndef KARMA_S_H
#define KARMA_S_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#ifndef KARMA_H
#  include <karma.h>
#endif


/*  For the file: sig_block.c  */
EXTERN_FUNCTION (void s_block, (int signal) );
EXTERN_FUNCTION (void s_unblock, (int signal) );
EXTERN_FUNCTION (void s_unblock_all, () );
EXTERN_FUNCTION (void *s_ignore, (int sig) );

/*  For the file: sig_handle.c  */
EXTERN_FUNCTION (void s_int_handler, () );
EXTERN_FUNCTION (void s_term_handler, () );
EXTERN_FUNCTION (flag s_check_for_int, () );


#endif /*  KARMA_S_H  */
