/*  karma_ex.h

    Header for  ex_  package.

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
  needed to interface to the ex_ routines in the Karma library.


    Written by      Richard Gooch   30-SEP-1992

    Last updated by Richard Gooch   29-NOV-1993

*/

#ifndef KARMA_EX_H
#define KARMA_EX_H


#ifndef KARMA_H
#  include <karma.h>
#endif

/*  File:   extract.c  */
EXTERN_FUNCTION (int ex_int, (char *str, char **rest ) );
EXTERN_FUNCTION (char *ex_word, (char *str, char **rest ) );
EXTERN_FUNCTION (char *ex_command, (char *str, char **rest ) );
EXTERN_FUNCTION (char *ex_word_skip, (char *str) );
EXTERN_FUNCTION (double ex_float, (char *str, char **rest ) );
EXTERN_FUNCTION (double ex_hour, (char *p, char **nxt) );
EXTERN_FUNCTION (char *ex_command_skip, (char *str) );
EXTERN_FUNCTION (int ex_on, (char **ptr) );
EXTERN_FUNCTION (int ex_on_or_off, (char **ptr) );
EXTERN_FUNCTION (int ex_yes, (char **ptr, int default_v) );
EXTERN_FUNCTION (char *ex_str, (char *str, char **rest) );


#endif /*  KARMA_EX_H  */
