/*  karma_md.h

    Header for  md_  package.

    Copyright (C) 1994,1995  Richard Gooch

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
  needed to interface to the md_ routines in the Karma library.


    Written by      Richard Gooch   24-AUG-1994

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_MD_H
#define KARMA_MD_H


typedef struct md5_context_type * MD5Context;


/*  File:  md5.c  */
EXTERN_FUNCTION (MD5Context md_md5_init, () );
EXTERN_FUNCTION (void md_md5_update,
		 (MD5Context ctx, CONST unsigned char *buf,unsigned int len) );
EXTERN_FUNCTION (void md_md5_final,
		 (MD5Context ctx, unsigned char digest[16]) );
EXTERN_FUNCTION (void md_md5_transform,
		 (unsigned char buf[16], CONST unsigned char in[64]) );


#endif /*  KARMA_MD_H  */

