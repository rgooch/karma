/*  karma_st.h

    Header for  st_  package.

    Copyright (C) 1992,1993,1994,1995  Richard Gooch

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
  needed to interface to the st_ routines in the Karma library.


    Written by      Richard Gooch   17-SEP-1992

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_ST_H
#define KARMA_ST_H


/*  File:   strings.c   */
EXTERN_FUNCTION (unsigned int st_find, (CONST char **string_list,
					unsigned int list_length,
					CONST char *string,
					int (*function) () ) );
EXTERN_FUNCTION (CONST char *st_chr, (CONST char *string, char c) );
EXTERN_FUNCTION (int st_cmp_wild, (CONST char *a, CONST char *b) );
EXTERN_FUNCTION (int st_cspn, (CONST char *string, CONST char *charset) );
EXTERN_FUNCTION (int st_icmp, (CONST char *string1, CONST char *string2) );
EXTERN_FUNCTION (char *st_lwr, (char *string) );
EXTERN_FUNCTION (int st_nicmp, (CONST char *string1, CONST char *string2,
				int str_len) );
EXTERN_FUNCTION (char *st_nupr, (char *string, int str_len) );
EXTERN_FUNCTION (char *st_nlwr, (char *string, int str_len) );
EXTERN_FUNCTION (char *st_pbrk, (CONST char *string,
				       CONST char *brkset) );
EXTERN_FUNCTION (CONST char *st_rchr, (CONST char *string, char c) );
EXTERN_FUNCTION (int st_spn, (CONST char *string, CONST char *charset) );
EXTERN_FUNCTION (char *st_tok, (char *string, CONST char *sepset) );
EXTERN_FUNCTION (long st_tol, (CONST char *str, char **ptr, int base) );
EXTERN_FUNCTION (char *st_upr, (char *string) );
EXTERN_FUNCTION (char *st_dup, (CONST char *input) );
EXTERN_FUNCTION (void st_qsort, (char **v, int left, int right) );


#endif /*  KARMA_ST_H  */
